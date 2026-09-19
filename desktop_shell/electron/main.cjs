const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const net = require('net');

const PIPE_PATH = '\\\\.\\pipe\\CustomShellIPC';
let mainWindow = null;

function sendPipeCommand(payload) {
  return new Promise((resolve, reject) => {
    const client = net.connect(PIPE_PATH, () => {
      client.write(JSON.stringify(payload));
    });

    client.on('data', (data) => {
      try {
        const response = JSON.parse(data.toString());
        resolve(response);
      } catch (err) {
        resolve({ status: 'error', message: err.message });
      } finally {
        client.end();
      }
    });

    client.on('error', (err) => {
      // Fallback response if named pipe is unavailable during isolated dev runs
      resolve({ status: 'fallback', message: err.message, payload });
    });
  });
}

function createShellWindow() {
  mainWindow = new BrowserWindow({
    width: 1366,
    height: 768,
    frame: false,
    fullscreen: false,
    title: 'Xenithra OS - Integrated Windows 11 Desktop Shell & Kernel Bridge',
    backgroundColor: '#0f172a',
    webPreferences: {
      preload: path.join(__dirname, 'preload.cjs'),
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: false,
    },
  });

  const isDev = process.env.NODE_ENV === 'development' || !app.isPackaged;
  if (isDev) {
    mainWindow.loadURL('http://localhost:5173').catch(() => {
      mainWindow.loadFile(path.join(__dirname, '../dist/index.html'));
    });
  } else {
    mainWindow.loadFile(path.join(__dirname, '../dist/index.html'));
  }

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

// App lifecycle
app.whenReady().then(() => {
  createShellWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createShellWindow();
    }
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

// IPC Main Handlers connecting Renderer to Native Pipe
ipcMain.handle('shell:launch', async (event, appName) => {
  try {
    return await sendPipeCommand({ action: 'launch', target: appName });
  } catch (err) {
    return { status: 'error', message: err.message };
  }
});

ipcMain.handle('shell:action', async (event, { type }) => {
  try {
    if (type === 'restore_explorer') {
      return await sendPipeCommand({ action: 'restore_explorer' });
    }
    return await sendPipeCommand({ action: 'power', type });
  } catch (err) {
    return { status: 'error', message: err.message };
  }
});
