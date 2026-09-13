const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const dns = require('dns');

let mainWindow = null;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1366,
    height: 768,
    minWidth: 1024,
    minHeight: 600,
    frame: true,
    title: 'Xenithra OS - Integrated Windows 11 Desktop Shell & Kernel Bridge',
    backgroundColor: '#060B18',
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
      webviewTag: true,
      webSecurity: false,
    },
    autoHideMenuBar: true,
  });

  const isDev = process.env.NODE_ENV === 'development' || !app.isPackaged;
  if (isDev) {
    mainWindow.loadURL('http://localhost:5173').catch(() => {
      mainWindow.loadFile(path.join(__dirname, '../dist/index.html'));
    });
  } else {
    mainWindow.loadFile(path.join(__dirname, '../dist/index.html'));
  }
}

// IPC Handlers for Integrated Architecture Pipeline (Kernel -> OS -> Firewall/DNS -> V8 -> Shell)
ipcMain.handle('dns-resolve', async (event, hostname) => {
  return new Promise((resolve) => {
    dns.lookup(hostname || 'www.google.com', (err, address, family) => {
      if (err) {
        resolve({ success: false, error: err.message, ip: '142.250.190.46' });
      } else {
        resolve({ success: true, ip: address, family });
      }
    });
  });
});

ipcMain.handle('firewall-inspect', async (event, packet) => {
  // Simulate Kernel NetFilter deep packet inspection
  const isBlocked = packet.port === 22 || packet.port === 445;
  return {
    action: isBlocked ? 'DROP' : 'ALLOW',
    ruleId: isBlocked ? 4 : 2,
    timestamp: Date.now(),
  };
});

ipcMain.handle('get-kernel-stats', async () => {
  return {
    smepEnabled: true,
    smapEnabled: true,
    sessionEntropy: '128-bit Active',
    activeSessions: 3,
    totalInspectedPackets: 48920,
    v8HeapUsedMb: Math.floor(process.memoryUsage().heapUsed / (1024 * 1024)),
    v8HeapTotalMb: Math.floor(process.memoryUsage().heapTotal / (1024 * 1024)),
  };
});

app.whenReady().then(() => {
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});
