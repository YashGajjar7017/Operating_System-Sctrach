/**
 * @file main.cjs — Xenithra OS v3.0 Render Engine — Electron Main Process
 *
 * Architecture:
 *   Kernel (C) → Named Pipe \\.\pipe\XenithraGUI → ipc_bridge.cjs → renderer (React/TS)
 *
 * This is the SEPARATE render engine — not part of the kernel.
 * The kernel only handles hardware, scheduling, security, and IPC.
 * ALL rendering is done here in Chromium/V8.
 *
 * Window Configuration:
 *   - Frameless fullscreen kiosk shell
 *   - Hardware GPU acceleration ON (Chromium = DWM compositor replacement)
 *   - Context isolation + preload for secure renderer API
 *   - WebGL + hardware canvas rasterization enabled
 */

'use strict';

const { app, BrowserWindow, ipcMain, screen, globalShortcut, nativeTheme } = require('electron');
const path   = require('path');
const bridge = require('./ipc_bridge.cjs');

let mainWindow = null;
let isQuitting = false;

/* ============================================================
 * Window Creation — Render Engine Shell
 * ============================================================ */
function createRenderEngineWindow() {
  const primaryDisplay = screen.getPrimaryDisplay();
  const { width, height } = primaryDisplay.bounds;

  nativeTheme.themeSource = 'dark';

  mainWindow = new BrowserWindow({
    width,
    height,
    x: 0,
    y: 0,
    frame: false,
    fullscreen: true,
    kiosk: false,
    resizable: false,
    movable: false,
    alwaysOnTop: false,
    skipTaskbar: true,
    title: 'Xenithra OS v3.0 — Render Engine',
    backgroundColor: '#03050a',
    show: false,
    webPreferences: {
      preload: path.join(__dirname, 'preload.cjs'),
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: false,
      webSecurity: true,
      allowRunningInsecureContent: false,
      backgroundThrottling: false,
      hardwareAcceleration: true,
      // Enable WebGL for GPU shaders
      experimentalFeatures: true,
      // V8 engine flags
      v8CacheOptions: 'bypassHeatCheck',
    },
  });

  /* Load Vite dev server or production build */
  const isDev = process.env.NODE_ENV === 'development' ||
                !app.isPackaged ||
                process.argv.includes('--dev');

  if (isDev) {
    // render_engine uses port 5174
    mainWindow.loadURL('http://localhost:5174').catch(() => {
      mainWindow.loadFile(path.join(__dirname, '../dist/index.html'));
    });
    // Open DevTools in dev mode
    // mainWindow.webContents.openDevTools({ mode: 'detach' });
  } else {
    mainWindow.loadFile(path.join(__dirname, '../dist/index.html'));
  }

  mainWindow.once('ready-to-show', () => {
    mainWindow.show();
    mainWindow.focus();
  });

  /* Attach kernel IPC bridge */
  bridge.attachWindow(mainWindow);
  bridge.connect();

  mainWindow.on('closed', () => {
    mainWindow = null;
    bridge.disconnect();
  });

  mainWindow.on('close', (e) => {
    if (!isQuitting) {
      e.preventDefault();
    }
  });
}

/* ============================================================
 * App Lifecycle
 * ============================================================ */
app.whenReady().then(() => {
  // GPU command switches for bare-metal / QEMU / VirtualBox environments
  app.commandLine.appendSwitch('no-sandbox');
  app.commandLine.appendSwitch('disable-gpu-sandbox');
  app.commandLine.appendSwitch('enable-accelerated-2d-canvas');
  app.commandLine.appendSwitch('enable-gpu-rasterization');
  app.commandLine.appendSwitch('enable-zero-copy');
  app.commandLine.appendSwitch('ignore-gpu-blocklist');
  app.commandLine.appendSwitch('enable-webgl');
  app.commandLine.appendSwitch('enable-webgl2');
  // Enable backdrop-filter (Mica/Acrylic) in Chromium
  app.commandLine.appendSwitch('enable-features', 'CSSBackdropFilter,WebGLDraftExtensions,CanvasOopRasterization');

  createRenderEngineWindow();

  // Dev shortcuts
  globalShortcut.register('CommandOrControl+Shift+I', () => {
    if (mainWindow) mainWindow.webContents.toggleDevTools();
  });
  globalShortcut.register('CommandOrControl+R', () => {
    if (mainWindow) mainWindow.webContents.reload();
  });

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createRenderEngineWindow();
  });
});

app.on('window-all-closed', () => {
  globalShortcut.unregisterAll();
  if (process.platform !== 'darwin') app.quit();
});

app.on('before-quit', () => {
  isQuitting = true;
});

/* ============================================================
 * IPC Main Handlers — Renderer ↔ Kernel Bridge
 * ============================================================ */

/** Launch an application */
ipcMain.handle('shell:launch', async (_event, appName, url) => {
  const ok = bridge.sendCommand({ cmd: 'launch', app: appName, url: url || '' });
  return ok ? { status: 'ok', app: appName } : { status: 'fallback', app: appName };
});

/** Close an application */
ipcMain.handle('shell:close', async (_event, appName) => {
  bridge.sendCommand({ cmd: 'close', app: appName });
  return { status: 'ok' };
});

/** System power actions */
ipcMain.handle('shell:power', async (_event, action) => {
  const valid = ['shutdown', 'reboot', 'sleep', 'hibernate', 'lock'];
  if (!valid.includes(action)) return { status: 'error', message: 'Invalid action' };
  if (action === 'shutdown' || action === 'reboot') {
    isQuitting = true;
    bridge.sendCommand({ cmd: 'power', action });
    setTimeout(() => app.quit(), 800);
    return { status: 'ok' };
  }
  bridge.sendCommand({ cmd: 'power', action });
  return { status: 'ok' };
});

/** Query kernel state */
ipcMain.handle('shell:query', async (_event, target) => {
  bridge.sendCommand({ cmd: 'query', target });
  return { status: 'queued', target };
});

/** Audio control */
ipcMain.handle('shell:audio', async (_event, { volume, mute }) => {
  bridge.sendCommand({ cmd: 'audio', volume, mute });
  return { status: 'ok' };
});

/** Network panel queries */
ipcMain.handle('shell:network', async (_event, { action, payload }) => {
  bridge.sendCommand({ cmd: 'network', action, payload });
  return { status: 'queued', action };
});

/** Driver manager queries */
ipcMain.handle('shell:driver', async (_event, { action, deviceId }) => {
  bridge.sendCommand({ cmd: 'driver', action, deviceId });
  return { status: 'queued', action };
});

/** Management panel actions */
ipcMain.handle('shell:manage', async (_event, { action, target }) => {
  bridge.sendCommand({ cmd: 'manage', action, target });
  return { status: 'queued', action };
});

/** Generic action */
ipcMain.handle('shell:action', async (_event, { type }) => {
  if (type === 'restore_explorer') {
    bridge.sendCommand({ cmd: 'launch', app: 'explorer' });
    return { status: 'ok' };
  }
  return bridge.sendCommand({ cmd: 'power', type })
    ? { status: 'ok' }
    : { status: 'fallback' };
});

/** Pipe status */
ipcMain.handle('shell:pipe_status', async () => ({
  connected: bridge._connected,
  pipe: '\\\\.\\pipe\\XenithraGUI',
  engine: 'XenithraRenderEngine/3.0',
}));
