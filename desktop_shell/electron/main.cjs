/**
 * main.cjs — Xenithra OS Electron Main Process
 *
 * Boot chain managed here:
 *   [1] app.whenReady() → createShellWindow() (fullscreen kiosk)
 *   [2] Attach ipc_bridge → connect to kernel Named Pipe
 *   [3] Register all ipcMain handlers (launch, power, query, audio, etc.)
 *   [4] Forward kernel events to renderer via bridge._sendToRenderer()
 *
 * Window config:
 *   - Frameless, fullscreen kiosk mode for OS shell appearance
 *   - Hardware acceleration ON (Chromium GPU compositor = DWM replacement)
 *   - contextIsolation + preload for safe renderer API exposure
 */

'use strict';

const { app, BrowserWindow, ipcMain, screen, globalShortcut } = require('electron');
const path   = require('path');
const bridge = require('./ipc_bridge.cjs');

let mainWindow = null;
let isQuitting = false;

/* ------------------------------------------------------------------ */
/* Window Creation                                                     */
/* ------------------------------------------------------------------ */

function createShellWindow() {
  const { width, height } = screen.getPrimaryDisplay().workAreaSize;

  mainWindow = new BrowserWindow({
    width,
    height,
    x: 0,
    y: 0,
    frame: false,
    fullscreen: true,
    kiosk: false,            // set true for locked kiosk mode
    resizable: false,
    movable: false,
    alwaysOnTop: false,
    skipTaskbar: true,
    title: 'Xenithra OS — Desktop Shell',
    backgroundColor: '#06090f',
    show: false,             // show after content is ready (prevents white flash)
    webPreferences: {
      preload: path.join(__dirname, 'preload.cjs'),
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: false,
      webSecurity: true,
      allowRunningInsecureContent: false,
      backgroundThrottling: false,  // keep shell alive when "backgrounded"
      hardwareAcceleration: true,
    },
  });

  /* Load Vite dev server or production build */
  const isDev = process.env.NODE_ENV === 'development' ||
                !app.isPackaged ||
                process.argv.includes('--dev');

  if (isDev) {
    mainWindow.loadURL('http://localhost:5173').catch(() => {
      mainWindow.loadFile(path.join(__dirname, '../dist/index.html'));
    });
  } else {
    mainWindow.loadFile(path.join(__dirname, '../dist/index.html'));
  }

  /* Show window only after renderer is painted (no white flash) */
  mainWindow.once('ready-to-show', () => {
    mainWindow.show();
    mainWindow.focus();
  });

  /* Attach kernel IPC bridge to this window */
  bridge.attachWindow(mainWindow);
  bridge.connect();

  mainWindow.on('closed', () => {
    mainWindow = null;
    bridge.disconnect();
  });

  /* Prevent window close unless OS is shutting down */
  mainWindow.on('close', (e) => {
    if (!isQuitting) {
      e.preventDefault();
    }
  });
}

/* ------------------------------------------------------------------ */
/* App Lifecycle                                                        */
/* ------------------------------------------------------------------ */

app.whenReady().then(() => {
  /* Disable GPU sandbox for bare-metal / QEMU environments */
  app.commandLine.appendSwitch('no-sandbox');
  app.commandLine.appendSwitch('disable-gpu-sandbox');
  app.commandLine.appendSwitch('enable-accelerated-2d-canvas');
  app.commandLine.appendSwitch('enable-gpu-rasterization');

  createShellWindow();

  /* Dev shortcut: Ctrl+Shift+I → DevTools */
  globalShortcut.register('CommandOrControl+Shift+I', () => {
    if (mainWindow) mainWindow.webContents.toggleDevTools();
  });

  /* Dev shortcut: Ctrl+R → Reload renderer */
  globalShortcut.register('CommandOrControl+R', () => {
    if (mainWindow) mainWindow.webContents.reload();
  });

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createShellWindow();
  });
});

app.on('window-all-closed', () => {
  globalShortcut.unregisterAll();
  if (process.platform !== 'darwin') app.quit();
});

app.on('before-quit', () => {
  isQuitting = true;
});

/* ------------------------------------------------------------------ */
/* IPC Main Handlers — Renderer ↔ Kernel                             */
/* ------------------------------------------------------------------ */

/** Launch an application (tells kernel DWM to open the window) */
ipcMain.handle('shell:launch', async (event, appName, url) => {
  const result = bridge.sendCommand({
    cmd: 'launch',
    app: appName,
    url: url || ''
  });
  return result
    ? { status: 'ok', app: appName }
    : { status: 'fallback', app: appName, message: 'Kernel pipe unavailable' };
});

/** Close an application window */
ipcMain.handle('shell:close', async (event, appName) => {
  bridge.sendCommand({ cmd: 'close', app: appName });
  return { status: 'ok' };
});

/** System power actions (shutdown, reboot, sleep) */
ipcMain.handle('shell:power', async (event, action) => {
  const valid = ['shutdown', 'reboot', 'sleep', 'hibernate', 'lock'];
  if (!valid.includes(action)) {
    return { status: 'error', message: 'Invalid action' };
  }

  if (action === 'shutdown' || action === 'reboot') {
    isQuitting = true;
    bridge.sendCommand({ cmd: 'power', action });
    setTimeout(() => app.quit(), 800);
    return { status: 'ok' };
  }

  bridge.sendCommand({ cmd: 'power', action });
  return { status: 'ok' };
});

/** Query kernel data (RTOS threads, service status, memory, CPU) */
ipcMain.handle('shell:query', async (event, target) => {
  bridge.sendCommand({ cmd: 'query', target });
  return { status: 'queued', target };
});

/** Audio control (volume, mute) */
ipcMain.handle('shell:audio', async (event, { volume, mute }) => {
  bridge.sendCommand({ cmd: 'audio', volume, mute });
  return { status: 'ok' };
});

/** Restore Explorer / Desktop */
ipcMain.handle('shell:action', async (event, { type }) => {
  if (type === 'restore_explorer') {
    bridge.sendCommand({ cmd: 'launch', app: 'explorer' });
    return { status: 'ok' };
  }
  return bridge.sendCommand({ cmd: 'power', type })
    ? { status: 'ok' }
    : { status: 'fallback' };
});

/** Pipe connection status check */
ipcMain.handle('shell:pipe_status', async () => ({
  connected: bridge._connected,
  pipe: '\\\\.\\pipe\\XenithraGUI'
}));
