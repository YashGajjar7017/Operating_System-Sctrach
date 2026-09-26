/**
 * preload.cjs — Xenithra OS Electron Preload Script
 *
 * Exposes the kernel IPC API to the React renderer via contextBridge.
 * All APIs are validated before exposure for security.
 *
 * Available as:
 *   window.kernelAPI.launchApp(appName, url?)
 *   window.kernelAPI.closeApp(appName)
 *   window.kernelAPI.systemPower(action)
 *   window.kernelAPI.queryKernel(target)
 *   window.kernelAPI.setAudio(volume, mute)
 *   window.kernelAPI.pipeStatus()
 *   window.kernelAPI.onKernelEvent(channel, callback)
 *   window.kernelAPI.removeKernelListener(channel, callback)
 *
 * Also exposes legacy:
 *   window.shellAPI.launchApp()
 *   window.shellAPI.systemAction()
 *   window.shellAPI.onSystemMessage()
 */

'use strict';

const { contextBridge, ipcRenderer } = require('electron');

/* ------------------------------------------------------------------ */
/* Validation helpers                                                  */
/* ------------------------------------------------------------------ */

const VALID_APPS = [
  'browser', 'explorer', 'terminal', 'taskmgr', 'defender',
  'vlc', 'installer', 'diskclone', 'firewall', 'calculator',
  'editor', 'store', 'web3', 'pipeline', 'media', 'services'
];

const VALID_POWER_ACTIONS = ['shutdown', 'reboot', 'sleep', 'hibernate', 'lock'];

const VALID_QUERY_TARGETS = [
  'rtos_threads', 'cpu_usage', 'mem_usage', 'services',
  'disk_stats', 'network_stats', 'audio_sessions'
];

const VALID_KERNEL_CHANNELS = [
  'kernel:mouse', 'kernel:key', 'kernel:window', 'kernel:service',
  'kernel:rtos', 'kernel:notify', 'kernel:power', 'kernel:wmi',
  'kernel:ipc', 'kernel:raw', 'kernel:log'
];

/* ------------------------------------------------------------------ */
/* New kernelAPI (full feature set)                                    */
/* ------------------------------------------------------------------ */

contextBridge.exposeInMainWorld('kernelAPI', {

  /** Launch a desktop application. */
  launchApp: (appName, url = '') => {
    if (typeof appName !== 'string') {
      return Promise.reject(new Error('appName must be a string'));
    }
    return ipcRenderer.invoke('shell:launch', appName, url);
  },

  /** Close a desktop application. */
  closeApp: (appName) => {
    if (typeof appName !== 'string') {
      return Promise.reject(new Error('appName must be a string'));
    }
    return ipcRenderer.invoke('shell:close', appName);
  },

  /** Send a system power command. */
  systemPower: (action) => {
    if (!VALID_POWER_ACTIONS.includes(action)) {
      return Promise.reject(new Error(`Invalid power action: ${action}`));
    }
    return ipcRenderer.invoke('shell:power', action);
  },

  /** Query kernel information (returns queued — result arrives via onKernelEvent). */
  queryKernel: (target) => {
    if (!VALID_QUERY_TARGETS.includes(target)) {
      return Promise.reject(new Error(`Invalid query target: ${target}`));
    }
    return ipcRenderer.invoke('shell:query', target);
  },

  /** Set master audio volume (0–100) or mute. */
  setAudio: (volume, mute = false) => {
    const vol = Math.min(100, Math.max(0, Number(volume) || 0));
    return ipcRenderer.invoke('shell:audio', { volume: vol, mute: Boolean(mute) });
  },

  /** Check kernel pipe connection status. */
  pipeStatus: () => ipcRenderer.invoke('shell:pipe_status'),

  /**
   * Subscribe to kernel events.
   * channel: one of VALID_KERNEL_CHANNELS
   * callback: (data) => void
   */
  onKernelEvent: (channel, callback) => {
    if (!VALID_KERNEL_CHANNELS.includes(channel)) {
      throw new Error(`Unknown kernel channel: ${channel}`);
    }
    if (typeof callback !== 'function') {
      throw new Error('callback must be a function');
    }
    const wrapped = (_event, data) => callback(data);
    ipcRenderer.on(channel, wrapped);
    return wrapped; // return so caller can remove it
  },

  /** Remove a previously registered kernel event listener. */
  removeKernelListener: (channel, wrappedCallback) => {
    ipcRenderer.removeListener(channel, wrappedCallback);
  },

  /** Shorthand: subscribe to all RTOS thread events. */
  onRTOS: (callback) => {
    const w = (_event, data) => callback(data);
    ipcRenderer.on('kernel:rtos', w);
    return w;
  },

  /** Shorthand: subscribe to all service telemetry events. */
  onService: (callback) => {
    const w = (_event, data) => callback(data);
    ipcRenderer.on('kernel:service', w);
    return w;
  },

  /** Shorthand: subscribe to kernel notifications. */
  onNotification: (callback) => {
    const w = (_event, data) => callback(data);
    ipcRenderer.on('kernel:notify', w);
    return w;
  },

  /** Shorthand: subscribe to window events from DWM proxy. */
  onWindowEvent: (callback) => {
    const w = (_event, data) => callback(data);
    ipcRenderer.on('kernel:window', w);
    return w;
  },
});

/* ------------------------------------------------------------------ */
/* Legacy shellAPI (backward compatibility with existing components)  */
/* ------------------------------------------------------------------ */

contextBridge.exposeInMainWorld('shellAPI', {
  launchApp: (appName) => {
    if (typeof appName !== 'string') {
      return Promise.reject(new Error('Invalid application target'));
    }
    return ipcRenderer.invoke('shell:launch', appName, '');
  },

  systemAction: (actionType) => {
    const valid = ['shutdown', 'reboot', 'restore_explorer', 'sleep'];
    if (!valid.includes(actionType)) {
      return Promise.reject(new Error('Invalid system action type'));
    }
    return ipcRenderer.invoke('shell:action', { type: actionType });
  },

  onSystemMessage: (callback) => {
    if (typeof callback === 'function') {
      ipcRenderer.on('system-message', (_event, data) => callback(data));
    }
  }
});
