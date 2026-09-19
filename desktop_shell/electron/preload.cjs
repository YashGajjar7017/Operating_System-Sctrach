const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('shellAPI', {
  launchApp: (appName) => {
    if (typeof appName !== 'string') {
      return Promise.reject(new Error('Invalid application target'));
    }
    return ipcRenderer.invoke('shell:launch', appName);
  },

  systemAction: (actionType) => {
    const validActions = ['shutdown', 'reboot', 'restore_explorer'];
    if (!validActions.includes(actionType)) {
      return Promise.reject(new Error('Invalid system action type'));
    }
    return ipcRenderer.invoke('shell:action', { type: actionType });
  },

  onSystemMessage: (callback) => {
    if (typeof callback === 'function') {
      ipcRenderer.on('system-message', (event, data) => callback(data));
    }
  }
});
