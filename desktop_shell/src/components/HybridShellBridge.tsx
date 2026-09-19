import React, { useState, useEffect, useCallback } from 'react';

// ============================================================================
// TypeScript Definitions for Injected C++/WinRT Host Object Bridge
// ============================================================================
export interface INativeBridge {
  LaunchProcess(executablePath: string): Promise<void>;
  MinimizeWindow(): Promise<void>;
  CloseShell(): Promise<void>;
  GetSystemTelemetry(): Promise<string>;
}

declare global {
  interface Window {
    chrome: {
      webview: {
        hostObjects: {
          nativeHost: INativeBridge;
          sync: {
            nativeHost: any;
          };
        };
        addEventListener(type: 'message', listener: (event: { data: any }) => void): void;
        removeEventListener(type: 'message', listener: (event: { data: any }) => void): void;
        postMessage(message: any): void;
      };
    };
  }
}

interface TelemetryData {
  cpuUsage: number;
  ramPercent: number;
  totalPhysicalBytes: number;
  availPhysicalBytes: number;
}

interface AppShortcut {
  id: string;
  name: string;
  executable: string;
  iconText: string;
}

const DOCK_APPS: AppShortcut[] = [
  { id: 'term', name: 'Terminal', executable: 'wt.exe', iconText: '>_' },
  { id: 'exp', name: 'Explorer', executable: 'explorer.exe', iconText: 'FILES' },
  { id: 'mgr', name: 'Task Manager', executable: 'taskmgr.exe', iconText: 'PERF' },
  { id: 'code', name: 'VS Code', executable: 'code.cmd', iconText: 'CODE' },
  { id: 'calc', name: 'Calculator', executable: 'calc.exe', iconText: 'CALC' },
];

export const HybridShellBridge: React.FC = () => {
  const [telemetry, setTelemetry] = useState<TelemetryData>({
    cpuUsage: 0.0,
    ramPercent: 0,
    totalPhysicalBytes: 0,
    availPhysicalBytes: 0,
  });

  const [notification, setNotification] = useState<string | null>(null);

  // Poll system telemetry from C++ NativeBridge via async COM interface
  useEffect(() => {
    let isMounted = true;

    const pollTelemetry = async () => {
      try {
        if (window.chrome?.webview?.hostObjects?.nativeHost) {
          const rawJson = await window.chrome.webview.hostObjects.nativeHost.GetSystemTelemetry();
          const parsed = JSON.parse(rawJson) as TelemetryData;
          if (isMounted) {
            setTelemetry(parsed);
          }
        }
      } catch (err) {
        console.error('Telemetry fetch error:', err);
      }
    };

    pollTelemetry();
    const intervalId = setInterval(pollTelemetry, 1000);

    return () => {
      isMounted = false;
      clearInterval(intervalId);
    };
  }, []);

  // Listen for asynchronous web messages dispatched via CoreWebView2::PostWebMessageAsJson
  useEffect(() => {
    const handleNativeEvent = (event: { data: any }) => {
      try {
        const message = typeof event.data === 'string' ? JSON.parse(event.data) : event.data;
        if (message.event === 'PROCESS_LAUNCHED') {
          setNotification(`Launched: ${message.payload?.path}`);
          setTimeout(() => setNotification(null), 3000);
        } else if (message.event === 'PROCESS_LAUNCH_ERROR') {
          setNotification(`Error launching process (Code ${message.payload?.errorCode})`);
          setTimeout(() => setNotification(null), 4000);
        }
      } catch (err) {
        console.error('WebMessage parse error:', err);
      }
    };

    if (window.chrome?.webview) {
      window.chrome.webview.addEventListener('message', handleNativeEvent);
    }

    return () => {
      if (window.chrome?.webview) {
        window.chrome.webview.removeEventListener('message', handleNativeEvent);
      }
    };
  }, []);

  const handleLaunch = useCallback(async (exePath: string) => {
    try {
      if (window.chrome?.webview?.hostObjects?.nativeHost) {
        await window.chrome.webview.hostObjects.nativeHost.LaunchProcess(exePath);
      }
    } catch (err) {
      console.error('Native process launch invocation error:', err);
    }
  }, []);

  const handleMinimize = useCallback(() => {
    window.chrome?.webview?.hostObjects?.nativeHost?.MinimizeWindow();
  }, []);

  const handleClose = useCallback(() => {
    window.chrome?.webview?.hostObjects?.nativeHost?.CloseShell();
  }, []);

  return (
    <div style={containerStyle}>
      {/* Custom Titlebar with native DWM drag region */}
      <header style={titlebarStyle}>
        <div style={dragRegionStyle}>
          <span style={logoStyle}>❖</span>
          <span style={titleTextStyle}>XENITHRA OS ARCHITECTURE</span>
        </div>
        <div style={windowControlsStyle}>
          <button style={controlButtonStyle} onClick={handleMinimize} title="Minimize">
            &#8211;
          </button>
          <button style={{ ...controlButtonStyle, ...closeButtonStyle }} onClick={handleClose} title="Close">
            &#10005;
          </button>
        </div>
      </header>

      {/* Main Workspace Surface */}
      <main style={workspaceStyle}>
        {notification && <div style={notificationStyle}>{notification}</div>}

        {/* Live System Telemetry Card */}
        <section style={telemetryCardStyle}>
          <h3 style={telemetryHeaderStyle}>SYSTEM HARDWARE TELEMETRY</h3>
          
          <div style={metricRowStyle}>
            <span style={metricLabelStyle}>CPU Load</span>
            <div style={meterTrackStyle}>
              <div
                style={{
                  ...meterFillStyle,
                  width: `${Math.min(telemetry.cpuUsage, 100)}%`,
                  backgroundColor: telemetry.cpuUsage > 80 ? '#ef4444' : '#38bdf8',
                }}
              />
            </div>
            <span style={metricValueStyle}>{telemetry.cpuUsage.toFixed(1)}%</span>
          </div>

          <div style={metricRowStyle}>
            <span style={metricLabelStyle}>RAM Usage</span>
            <div style={meterTrackStyle}>
              <div
                style={{
                  ...meterFillStyle,
                  width: `${Math.min(telemetry.ramPercent, 100)}%`,
                  backgroundColor: telemetry.ramPercent > 85 ? '#ef4444' : '#34d399',
                }}
              />
            </div>
            <span style={metricValueStyle}>{telemetry.ramPercent}%</span>
          </div>

          <div style={memorySubtextStyle}>
            Available Physical RAM: {(telemetry.availPhysicalBytes / (1024 * 1024 * 1024)).toFixed(2)} GB
          </div>
        </section>
      </main>

      {/* Dynamic Subsystem Dock / Taskbar */}
      <footer style={dockStyle}>
        {DOCK_APPS.map((app) => (
          <button
            key={app.id}
            style={dockButtonStyle}
            onClick={() => handleLaunch(app.executable)}
            title={app.name}
          >
            <span style={dockIconStyle}>{app.iconText}</span>
            <span style={dockLabelStyle}>{app.name}</span>
          </button>
        ))}
      </footer>
    </div>
  );
};

// ============================================================================
// Styles (Modern Glassmorphic Dark Theme)
// ============================================================================
const containerStyle: React.CSSProperties = {
  display: 'flex',
  flexDirection: 'column',
  width: '100%',
  height: '100%',
  backgroundColor: 'transparent', // Allows DWM Mica backdrop to show through
  color: '#f8fafc',
  fontFamily: '"Segoe UI Variable Display", "Segoe UI", system-ui, sans-serif',
  userSelect: 'none',
  overflow: 'hidden',
};

const titlebarStyle: React.CSSProperties = {
  display: 'flex',
  height: '36px',
  backgroundColor: 'rgba(15, 23, 42, 0.65)',
  backdropFilter: 'blur(16px)',
  borderBottom: '1px solid rgba(255, 255, 255, 0.08)',
  zIndex: 1000,
};

const dragRegionStyle: React.CSSProperties = {
  flex: 1,
  display: 'flex',
  alignItems: 'center',
  paddingLeft: '14px',
  gap: '10px',
  WebkitAppRegion: 'drag', // Native DWM titlebar drag regions
} as any;

const logoStyle: React.CSSProperties = {
  color: '#38bdf8',
  fontSize: '14px',
};

const titleTextStyle: React.CSSProperties = {
  fontSize: '11px',
  fontWeight: 600,
  letterSpacing: '0.08em',
  color: '#94a3b8',
};

const windowControlsStyle: React.CSSProperties = {
  display: 'flex',
  WebkitAppRegion: 'no-drag',
} as any;

const controlButtonStyle: React.CSSProperties = {
  width: '46px',
  height: '36px',
  border: 'none',
  backgroundColor: 'transparent',
  color: '#94a3b8',
  cursor: 'pointer',
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  fontSize: '12px',
  transition: 'background-color 0.15s ease',
};

const closeButtonStyle: React.CSSProperties = {
  borderTopRightRadius: '0px',
};

const workspaceStyle: React.CSSProperties = {
  flex: 1,
  position: 'relative',
  padding: '24px',
  display: 'flex',
  flexDirection: 'column',
  alignItems: 'center',
  justifyContent: 'center',
};

const notificationStyle: React.CSSProperties = {
  position: 'absolute',
  top: '20px',
  backgroundColor: 'rgba(30, 41, 59, 0.9)',
  border: '1px solid rgba(56, 189, 248, 0.4)',
  color: '#38bdf8',
  padding: '8px 16px',
  borderRadius: '8px',
  fontSize: '12px',
  boxShadow: '0 10px 25px -5px rgba(0, 0, 0, 0.5)',
};

const telemetryCardStyle: React.CSSProperties = {
  width: '380px',
  backgroundColor: 'rgba(15, 23, 42, 0.75)',
  backdropFilter: 'blur(20px)',
  border: '1px solid rgba(255, 255, 255, 0.1)',
  borderRadius: '16px',
  padding: '20px',
  boxShadow: '0 20px 40px rgba(0, 0, 0, 0.4)',
};

const telemetryHeaderStyle: React.CSSProperties = {
  margin: '0 0 16px 0',
  fontSize: '11px',
  fontWeight: 700,
  letterSpacing: '0.08em',
  color: '#64748b',
};

const metricRowStyle: React.CSSProperties = {
  display: 'flex',
  alignItems: 'center',
  gap: '12px',
  marginBottom: '12px',
};

const metricLabelStyle: React.CSSProperties = {
  width: '70px',
  fontSize: '12px',
  fontWeight: 600,
  color: '#cbd5e1',
};

const meterTrackStyle: React.CSSProperties = {
  flex: 1,
  height: '6px',
  backgroundColor: 'rgba(255, 255, 255, 0.1)',
  borderRadius: '3px',
  overflow: 'hidden',
};

const meterFillStyle: React.CSSProperties = {
  height: '100%',
  transition: 'width 0.4s ease-out, background-color 0.4s ease',
};

const metricValueStyle: React.CSSProperties = {
  width: '45px',
  textAlign: 'right',
  fontSize: '12px',
  fontFamily: 'monospace',
  color: '#f8fafc',
};

const memorySubtextStyle: React.CSSProperties = {
  marginTop: '12px',
  fontSize: '11px',
  color: '#64748b',
  textAlign: 'center',
};

const dockStyle: React.CSSProperties = {
  height: '64px',
  backgroundColor: 'rgba(15, 23, 42, 0.7)',
  backdropFilter: 'blur(24px)',
  borderTop: '1px solid rgba(255, 255, 255, 0.08)',
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  gap: '12px',
  padding: '0 24px',
};

const dockButtonStyle: React.CSSProperties = {
  display: 'flex',
  flexDirection: 'column',
  alignItems: 'center',
  justifyContent: 'center',
  width: '56px',
  height: '48px',
  backgroundColor: 'rgba(255, 255, 255, 0.04)',
  border: '1px solid rgba(255, 255, 255, 0.06)',
  borderRadius: '10px',
  color: '#e2e8f0',
  cursor: 'pointer',
  transition: 'transform 0.15s ease, background-color 0.15s ease',
};

const dockIconStyle: React.CSSProperties = {
  fontSize: '11px',
  fontWeight: 700,
  color: '#38bdf8',
  fontFamily: 'monospace',
};

const dockLabelStyle: React.CSSProperties = {
  fontSize: '9px',
  marginTop: '2px',
  color: '#94a3b8',
};
