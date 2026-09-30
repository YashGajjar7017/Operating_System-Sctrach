/**
 * @file preload.cjs — Xenithra OS v3.0 Render Engine — Context Bridge
 *
 * Exposes a safe, typed API surface to the React renderer process.
 * All kernel IPC is routed through contextBridge — renderer never
 * gets direct Node.js / Electron API access.
 */

'use strict';

const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('xenithra', {
  // ── Shell Commands ──────────────────────────────────────────
  launch:    (app, url)    => ipcRenderer.invoke('shell:launch', app, url),
  close:     (app)         => ipcRenderer.invoke('shell:close', app),
  power:     (action)      => ipcRenderer.invoke('shell:power', action),
  query:     (target)      => ipcRenderer.invoke('shell:query', target),
  audio:     (opts)        => ipcRenderer.invoke('shell:audio', opts),
  action:    (opts)        => ipcRenderer.invoke('shell:action', opts),
  pipeStatus:()            => ipcRenderer.invoke('shell:pipe_status'),

  // ── Network Panel ───────────────────────────────────────────
  network:   (action, payload) => ipcRenderer.invoke('shell:network', { action, payload }),

  // ── Driver Manager ──────────────────────────────────────────
  driver:    (action, deviceId) => ipcRenderer.invoke('shell:driver', { action, deviceId }),

  // ── Management Panel ────────────────────────────────────────
  manage:    (action, target) => ipcRenderer.invoke('shell:manage', { action, target }),

  // ── Event Listeners (kernel → renderer) ─────────────────────
  onKernelEvent:   (cb) => ipcRenderer.on('kernel:event',   (_e, d) => cb(d)),
  onMouseEvent:    (cb) => ipcRenderer.on('kernel:mouse',   (_e, d) => cb(d)),
  onKeyEvent:      (cb) => ipcRenderer.on('kernel:key',     (_e, d) => cb(d)),
  onWindowEvent:   (cb) => ipcRenderer.on('kernel:window',  (_e, d) => cb(d)),
  onServiceEvent:  (cb) => ipcRenderer.on('kernel:service', (_e, d) => cb(d)),
  onRtosEvent:     (cb) => ipcRenderer.on('kernel:rtos',    (_e, d) => cb(d)),
  onPowerEvent:    (cb) => ipcRenderer.on('kernel:power',   (_e, d) => cb(d)),
  onNetworkEvent:  (cb) => ipcRenderer.on('kernel:network', (_e, d) => cb(d)),
  onDriverEvent:   (cb) => ipcRenderer.on('kernel:driver',  (_e, d) => cb(d)),
  onNotifyEvent:   (cb) => ipcRenderer.on('kernel:notify',  (_e, d) => cb(d)),
  onAudioEvent:    (cb) => ipcRenderer.on('kernel:audio',   (_e, d) => cb(d)),
  onIpcStatus:     (cb) => ipcRenderer.on('ipc:status',     (_e, d) => cb(d)),
  onIpcLog:        (cb) => ipcRenderer.on('ipc:log',        (_e, d) => cb(d)),

  // ── Cleanup ──────────────────────────────────────────────────
  removeAllListeners: (channel) => ipcRenderer.removeAllListeners(channel),
});
