/**
 * ipc_bridge.cjs
 * Xenithra OS — Node.js Kernel IPC Bridge
 *
 * Connects to the kernel's Named Pipe server ("\\\\.\\pipe\\XenithraGUI")
 * and re-emits all kernel events into the Electron BrowserWindow renderer.
 *
 * Kernel events (JSON lines from pipe → parsed → sent to renderer via ipcMain):
 *   mouse    → 'kernel:mouse'
 *   key      → 'kernel:key'
 *   window   → 'kernel:window'
 *   service  → 'kernel:service'
 *   rtos     → 'kernel:rtos'
 *   notify   → 'kernel:notify'
 *   power    → 'kernel:power'
 *   ipc      → 'kernel:ipc' (control messages)
 *
 * Renderer → kernel commands (via ipcMain → pipe):
 *   shell:launch  → {"cmd":"launch","app":"browser","url":"..."}
 *   shell:close   → {"cmd":"close","app":"terminal"}
 *   shell:power   → {"cmd":"power","action":"shutdown"}
 *   shell:query   → {"cmd":"query","target":"rtos_threads"}
 */

'use strict';

const net    = require('net');
const { EventEmitter } = require('events');

const PIPE_PATH    = '\\\\.\\pipe\\XenithraGUI';
const RECONNECT_MS = 2000;
const HEARTBEAT_MS = 5000;

class KernelIpcBridge extends EventEmitter {
  constructor() {
    super();
    this._client      = null;
    this._connected   = false;
    this._lineBuffer  = '';
    this._heartbeat   = null;
    this._reconnTimer = null;
    this._window      = null;  /** @type {import('electron').BrowserWindow} */
  }

  /**
   * Attach a BrowserWindow so kernel events are forwarded to the renderer.
   * @param {import('electron').BrowserWindow} win
   */
  attachWindow(win) {
    this._window = win;
  }

  /** Start the IPC bridge — connect to kernel pipe. */
  connect() {
    if (this._connected) return;
    this._tryConnect();
  }

  /** Send a command to the kernel. */
  sendCommand(cmd) {
    if (!this._connected || !this._client) {
      console.warn('[IPC Bridge] Not connected — command dropped:', cmd);
      return false;
    }
    try {
      this._client.write(JSON.stringify(cmd) + '\n');
      return true;
    } catch (err) {
      console.error('[IPC Bridge] Write error:', err.message);
      return false;
    }
  }

  /** Gracefully disconnect. */
  disconnect() {
    if (this._heartbeat)   clearInterval(this._heartbeat);
    if (this._reconnTimer) clearTimeout(this._reconnTimer);
    if (this._client)      this._client.destroy();
    this._connected = false;
  }

  // ── Private ─────────────────────────────────────────────────────────

  _tryConnect() {
    const client = net.createConnection(PIPE_PATH);
    this._client = client;

    client.on('connect', () => {
      this._connected  = true;
      this._lineBuffer = '';
      console.log('[IPC Bridge] Connected to kernel pipe:', PIPE_PATH);
      this.emit('connected');
      this._sendToRenderer('kernel:ipc', { action: 'connected' });

      // Send boot acknowledgement to kernel
      client.write(JSON.stringify({
        cmd: 'handshake',
        version: 'XenithraShell-3.0',
        pid: process.pid
      }) + '\n');

      // Start heartbeat
      this._heartbeat = setInterval(() => {
        if (this._connected) {
          client.write(JSON.stringify({ cmd: 'heartbeat' }) + '\n');
        }
      }, HEARTBEAT_MS);
    });

    client.on('data', (data) => {
      this._lineBuffer += data.toString('utf8');
      const lines = this._lineBuffer.split('\n');
      this._lineBuffer = lines.pop() ?? '';
      for (const line of lines) {
        const trimmed = line.trim();
        if (!trimmed) continue;
        this._handleKernelMessage(trimmed);
      }
    });

    client.on('close', () => {
      this._connected = false;
      if (this._heartbeat) clearInterval(this._heartbeat);
      console.warn('[IPC Bridge] Kernel pipe closed — reconnecting in', RECONNECT_MS, 'ms');
      this._sendToRenderer('kernel:ipc', { action: 'disconnected' });
      this._scheduleReconnect();
    });

    client.on('error', (err) => {
      this._connected = false;
      if (err.code !== 'ENOENT' && err.code !== 'ECONNREFUSED') {
        console.error('[IPC Bridge] Pipe error:', err.message);
      }
      this._scheduleReconnect();
    });
  }

  _scheduleReconnect() {
    if (this._reconnTimer) return;
    this._reconnTimer = setTimeout(() => {
      this._reconnTimer = null;
      this._tryConnect();
    }, RECONNECT_MS);
  }

  _handleKernelMessage(json) {
    let msg;
    try {
      msg = JSON.parse(json);
    } catch {
      // Raw text from kernel debug output — forward as log
      this._sendToRenderer('kernel:log', { text: json });
      return;
    }

    const type = msg.type ?? 'unknown';

    switch (type) {
      case 'mouse':
        this._sendToRenderer('kernel:mouse', msg);
        break;

      case 'key':
        this._sendToRenderer('kernel:key', msg);
        break;

      case 'window':
        this._sendToRenderer('kernel:window', msg);
        break;

      case 'service':
        this._sendToRenderer('kernel:service', msg);
        break;

      case 'rtos':
        this._sendToRenderer('kernel:rtos', msg);
        break;

      case 'notify':
        this._sendToRenderer('kernel:notify', msg);
        break;

      case 'power':
        this._sendToRenderer('kernel:power', msg);
        this._handlePowerEvent(msg);
        break;

      case 'wmi':
        this._sendToRenderer('kernel:wmi', msg);
        break;

      case 'ipc':
        this._sendToRenderer('kernel:ipc', msg);
        break;

      default:
        this._sendToRenderer('kernel:raw', msg);
        break;
    }

    this.emit('message', msg);
  }

  _handlePowerEvent(msg) {
    if (msg.action === 'shutdown') {
      const { app } = require('electron');
      setTimeout(() => app.quit(), 500);
    }
  }

  _sendToRenderer(channel, data) {
    if (this._window && !this._window.isDestroyed()) {
      try {
        this._window.webContents.send(channel, data);
      } catch {
        /* renderer may not be ready yet */
      }
    }
  }
}

// Export singleton
const bridge = new KernelIpcBridge();
module.exports = bridge;
