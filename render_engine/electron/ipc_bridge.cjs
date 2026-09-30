/**
 * @file ipc_bridge.cjs — Xenithra OS v3.0 Render Engine — Kernel IPC Bridge
 *
 * Connects the Electron render engine to the kernel's Named Pipe server.
 * Reads newline-delimited JSON events from \\.\pipe\XenithraGUI and
 * dispatches them to the React renderer via webContents.send().
 *
 * Event types received from kernel:
 *   mouse   → { type, x, y, l, r, m }
 *   key     → { type, ascii, scan, pressed }
 *   window  → { type, action, tag, title }
 *   service → { type, name, cpu, mem, status }
 *   rtos    → { type, tid, prio, state, cpu_us }
 *   power   → { type, action }
 *   network → { type, iface, ip, gateway, dns, status }
 *   driver  → { type, device, vendor, status, version }
 *   notify  → { type, title, body, icon }
 */

'use strict';

const net  = require('net');
const path = require('path');

const PIPE_NAME    = '\\\\.\\pipe\\XenithraGUI';
const RECONNECT_MS = 3000;
const PING_MS      = 5000;

let _window     = null;
let _socket     = null;
let _connected  = false;
let _reconnectTimer = null;
let _pingTimer  = null;
let _buffer     = '';

/* ============================================================
 * Public API
 * ============================================================ */

function attachWindow(win) {
  _window = win;
}

function connect() {
  if (_socket) return;
  _socket = net.createConnection(PIPE_NAME);

  _socket.setEncoding('utf8');

  _socket.on('connect', () => {
    _connected = true;
    _buffer    = '';
    console.log('[IPC Bridge] Connected to kernel pipe:', PIPE_NAME);
    _sendToRenderer('ipc:status', { connected: true, pipe: PIPE_NAME });

    // Periodic ping to keep pipe alive
    _pingTimer = setInterval(() => {
      _sendCommand({ cmd: 'ping' });
    }, PING_MS);
  });

  _socket.on('data', (chunk) => {
    _buffer += chunk;
    const lines = _buffer.split('\n');
    _buffer = lines.pop(); // keep incomplete last line

    for (const line of lines) {
      const trimmed = line.trim();
      if (!trimmed) continue;
      try {
        const evt = JSON.parse(trimmed);
        _dispatchKernelEvent(evt);
      } catch (e) {
        // Non-JSON kernel debug output — forward as log
        _sendToRenderer('ipc:log', { message: trimmed });
      }
    }
  });

  _socket.on('error', (err) => {
    // Silently handle — kernel may not be running (dev mode)
    if (err.code !== 'ENOENT' && err.code !== 'ECONNREFUSED') {
      console.warn('[IPC Bridge] Pipe error:', err.code);
    }
    _onDisconnect();
  });

  _socket.on('close', () => {
    _onDisconnect();
  });
}

function disconnect() {
  _cleanup();
}

/**
 * Send a command to the kernel via the named pipe.
 * @param {object} cmd — JSON-serializable command object
 * @returns {boolean} true if sent, false if pipe not connected
 */
function sendCommand(cmd) {
  if (!_socket || !_connected) return false;
  try {
    _socket.write(JSON.stringify(cmd) + '\n');
    return true;
  } catch (e) {
    return false;
  }
}

/* ============================================================
 * Internal Helpers
 * ============================================================ */

function _onDisconnect() {
  _connected = false;
  _cleanup();
  _sendToRenderer('ipc:status', { connected: false });

  // Auto-reconnect
  _reconnectTimer = setTimeout(() => {
    _reconnectTimer = null;
    connect();
  }, RECONNECT_MS);
}

function _cleanup() {
  if (_pingTimer) { clearInterval(_pingTimer); _pingTimer = null; }
  if (_socket) {
    try { _socket.destroy(); } catch (_) {}
    _socket = null;
  }
}

function _sendToRenderer(channel, data) {
  if (_window && !_window.isDestroyed()) {
    _window.webContents.send(channel, data);
  }
}

function _sendCommand(cmd) {
  if (_socket && _connected) {
    try { _socket.write(JSON.stringify(cmd) + '\n'); } catch (_) {}
  }
}

/**
 * Dispatch a parsed kernel event to the appropriate renderer channel.
 * All events go to 'kernel:event' + a specific typed channel.
 */
function _dispatchKernelEvent(evt) {
  if (!evt || !evt.type) return;

  // Broadcast to generic kernel:event channel (renderer listens here)
  _sendToRenderer('kernel:event', evt);

  // Also send to typed sub-channels for direct component subscriptions
  switch (evt.type) {
    case 'mouse':   _sendToRenderer('kernel:mouse',   evt); break;
    case 'key':     _sendToRenderer('kernel:key',     evt); break;
    case 'window':  _sendToRenderer('kernel:window',  evt); break;
    case 'service': _sendToRenderer('kernel:service', evt); break;
    case 'rtos':    _sendToRenderer('kernel:rtos',    evt); break;
    case 'power':   _sendToRenderer('kernel:power',   evt); break;
    case 'network': _sendToRenderer('kernel:network', evt); break;
    case 'driver':  _sendToRenderer('kernel:driver',  evt); break;
    case 'notify':  _sendToRenderer('kernel:notify',  evt); break;
    case 'audio':   _sendToRenderer('kernel:audio',   evt); break;
    default:        _sendToRenderer('kernel:misc',    evt); break;
  }
}

module.exports = {
  attachWindow,
  connect,
  disconnect,
  sendCommand,
  get _connected() { return _connected; },
};
