/**
 * Xenithra OS - Kiosk Desktop Shell Frontend Logic
 * Interfaces with ASP.NET Core or Django System Daemon
 */

const API_BASE = window.location.origin;

// 1. Live Telemetry & Stats Loop
async function fetchSystemStats() {
  try {
    const res = await fetch(`${API_BASE}/api/system/stats`);
    if (res.ok) {
      const data = await res.json();
      updateDashboard(data);
    }
  } catch (err) {
    console.warn("REST telemetry polling fallback active:", err);
  }
}

function updateDashboard(data) {
  const cpu = data.CpuUsagePercent ?? data.cpu_percent ?? 24;
  const ram = data.MemoryUsedMb ?? data.memory_used_mb ?? 1840;
  const disk = data.DiskPercent ?? data.disk_percent ?? 42;

  document.getElementById('cpu-val').innerText = `${cpu} %`;
  document.getElementById('cpu-bar').style.width = `${cpu}%`;

  document.getElementById('ram-val').innerText = `${ram} MB`;
  document.getElementById('ram-bar').style.width = `${Math.min(100, (ram / 8192) * 100)}%`;

  document.getElementById('disk-val').innerText = `${disk} %`;
  document.getElementById('disk-bar').style.width = `${disk}%`;
}

// 2. Connect SignalR WebSocket (Option A) or Django Channels (Option B)
function initWebSocketTelemetry() {
  if (typeof signalR !== 'undefined') {
    const connection = new signalR.HubConnectionBuilder()
      .withUrl(`${API_BASE}/hubs/telemetry`)
      .withAutomaticReconnect()
      .build();

    connection.on("ReceiveTelemetry", (data) => {
      updateDashboard({
        CpuUsagePercent: data.cpu,
        MemoryUsedMb: data.memoryMb,
        DiskPercent: data.diskPercent
      });
    });

    connection.start().catch(() => {
      // Fallback to REST polling if SignalR Hub is not available
      setInterval(fetchSystemStats, 1000);
    });
  } else {
    // Django Channels / Generic WebSocket fallback
    try {
      const ws = new WebSocket(`ws://${window.location.host}/ws/telemetry/`);
      ws.onmessage = (event) => {
        const data = JSON.parse(event.data);
        updateDashboard(data);
      };
      ws.onerror = () => setInterval(fetchSystemStats, 1000);
    } catch {
      setInterval(fetchSystemStats, 1000);
    }
  }
}

// 3. System Process Execution
async function executeSystemCommand(cmd) {
  const feedback = document.getElementById('cmd-feedback');
  feedback.innerText = `Executing: ${cmd}...`;

  try {
    const res = await fetch(`${API_BASE}/api/system/exec`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ command: cmd, run_detached: true })
    });

    const result = await res.json();
    if (result.success) {
      feedback.innerText = result.message || `Started successfully (PID: ${result.pid || 'OK'})`;
      feedback.style.color = '#34D399';
    } else {
      feedback.innerText = `Error: ${result.error || result.message}`;
      feedback.style.color = '#F87171';
    }
  } catch (err) {
    feedback.innerText = `Network/Daemon Error: ${err.message}`;
    feedback.style.color = '#F87171';
  }
}

// 4. App Launcher Shortcuts
function launchApp(appType) {
  const appMap = {
    'terminal': 'x-terminal-emulator',
    'explorer': 'thunar',
    'firewall': 'gufw',
    'taskmgr': 'htop'
  };

  const targetBinary = appMap[appType] || appType;
  executeSystemCommand(targetBinary);
}

// 5. System Power Actions (Reboot / Shutdown)
async function systemPower(action) {
  if (!confirm(`Are you sure you want to ${action} the system?`)) return;

  try {
    await fetch(`${API_BASE}/api/system/power`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ action: action })
    });
  } catch (err) {
    alert(`Failed to trigger power action: ${err.message}`);
  }
}

// 6. UI Handlers (Clock, Start Menu, Context Menu)
function updateClock() {
  const now = new Date();
  document.getElementById('time-text').innerText = now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
  document.getElementById('date-text').innerText = now.toISOString().split('T')[0];
}

function toggleStartMenu() {
  const menu = document.getElementById('start-menu');
  menu.classList.toggle('hidden');
}

function togglePowerMenu() {
  toggleStartMenu();
}

// Setup Event Listeners
document.addEventListener('DOMContentLoaded', () => {
  initWebSocketTelemetry();
  updateClock();
  setInterval(updateClock, 1000);

  // Command prompt input
  const cmdInput = document.getElementById('cmd-input');
  const runBtn = document.getElementById('cmd-run-btn');

  runBtn.addEventListener('click', () => {
    if (cmdInput.value.trim()) {
      executeSystemCommand(cmdInput.value.trim());
    }
  });

  cmdInput.addEventListener('keydown', (e) => {
    if (e.key === 'Enter' && cmdInput.value.trim()) {
      executeSystemCommand(cmdInput.value.trim());
    }
  });

  // Start Menu button
  document.getElementById('start-btn').addEventListener('click', (e) => {
    e.stopPropagation();
    toggleStartMenu();
  });

  // Close menus on outside click
  document.addEventListener('click', () => {
    document.getElementById('start-menu').classList.add('hidden');
    document.getElementById('context-menu').classList.add('hidden');
  });

  // Custom Context Menu on Right-Click
  const contextMenu = document.getElementById('context-menu');
  document.addEventListener('contextmenu', (e) => {
    e.preventDefault();
    contextMenu.style.left = `${e.clientX}px`;
    contextMenu.style.top = `${e.clientY}px`;
    contextMenu.classList.remove('hidden');
  });
});
