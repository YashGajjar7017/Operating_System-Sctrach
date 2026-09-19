import React, { useState, useEffect } from 'react';

declare global {
  interface Window {
    shellAPI?: {
      launchApp(target: string): Promise<any>;
      systemAction(type: 'shutdown' | 'reboot' | 'restore_explorer'): Promise<any>;
      onSystemMessage(callback: (msg: any) => void): void;
    };
  }
}

export const CustomShellHost: React.FC = () => {
  const [timeStr, setTimeStr] = useState<string>('');
  const [dateStr, setDateStr] = useState<string>('');
  const [menuOpen, setMenuOpen] = useState<boolean>(false);
  const [statusMsg, setStatusMsg] = useState<string>('');

  useEffect(() => {
    const updateClock = () => {
      const now = new Date();
      setTimeStr(now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }));
      setDateStr(now.toLocaleDateString([], { weekday: 'short', month: 'short', day: 'numeric' }));
    };
    updateClock();
    const timer = setInterval(updateClock, 1000);
    return () => clearInterval(timer);
  }, []);

  const handleLaunch = async (appName: string) => {
    try {
      if (window.shellAPI) {
        const res = await window.shellAPI.launchApp(appName);
        if (res && (res.status === 'ok' || res.status === 'fallback')) {
          setStatusMsg(`Launched: ${appName}`);
        } else {
          setStatusMsg(`Failed to launch: ${appName}`);
        }
      } else {
        setStatusMsg(`Local Launch Simulation: ${appName}`);
      }
    } catch (err: any) {
      setStatusMsg(`Error: ${err.message}`);
    }
    setMenuOpen(false);
    setTimeout(() => setStatusMsg(''), 4000);
  };

  const handleSystemAction = async (actionType: 'shutdown' | 'reboot' | 'restore_explorer') => {
    try {
      if (window.shellAPI) {
        await window.shellAPI.systemAction(actionType);
        setStatusMsg(`System action: ${actionType}`);
      } else {
        setStatusMsg(`Simulated system action: ${actionType}`);
      }
    } catch (err: any) {
      setStatusMsg(`Action failed: ${err.message}`);
    }
  };

  return (
    <div style={styles.container}>
      {/* Desktop Canvas */}
      <main style={styles.desktopCanvas} onClick={() => menuOpen && setMenuOpen(false)}>
        <div style={styles.watermark}>
          <h1 style={styles.watermarkTitle}>XENITHRA OS</h1>
          <p style={styles.watermarkSub}>Custom Native-Web Hybrid Desktop Shell Subsystem</p>
        </div>

        {statusMsg && <div style={styles.toast}>{statusMsg}</div>}

        {/* Start Drawer Flyout */}
        {menuOpen && (
          <div style={styles.startDrawer} onClick={(e) => e.stopPropagation()}>
            <div style={styles.drawerHeader}>
              <h3 style={styles.drawerTitle}>Subsystem Applications & Tools</h3>
            </div>
            <div style={styles.drawerGrid}>
              <button style={styles.drawerCard} onClick={() => handleLaunch('notepad.exe')}>
                <span style={styles.cardIcon}>📝</span>
                <span style={styles.cardLabel}>Notepad</span>
              </button>
              <button style={styles.drawerCard} onClick={() => handleLaunch('cmd.exe')}>
                <span style={styles.cardIcon}>⚡</span>
                <span style={styles.cardLabel}>Command Prompt</span>
              </button>
              <button style={styles.drawerCard} onClick={() => handleLaunch('taskmgr.exe')}>
                <span style={styles.cardIcon}>📊</span>
                <span style={styles.cardLabel}>Task Manager</span>
              </button>
              <button style={styles.drawerCard} onClick={() => handleLaunch('explorer.exe')}>
                <span style={styles.cardIcon}>📁</span>
                <span style={styles.cardLabel}>File Explorer</span>
              </button>
            </div>

            <div style={styles.drawerFooter}>
              <button 
                style={styles.emergencyBtn}
                onClick={() => handleSystemAction('restore_explorer')}
              >
                🔄 Restore Explorer Shell
              </button>
            </div>
          </div>
        )}
      </main>

      {/* Dock / Taskbar */}
      <footer style={styles.dock}>
        <button 
          style={{
            ...styles.dockBtn,
            ...styles.startBtn,
            ...(menuOpen ? styles.startBtnActive : {}),
          }}
          onClick={() => setMenuOpen((prev) => !prev)}
        >
          ❖
        </button>

        <div style={styles.separator} />

        <button style={styles.dockBtn} onClick={() => handleLaunch('msedge.exe')} title="Browser">
          🌐
        </button>
        <button style={styles.dockBtn} onClick={() => handleLaunch('wt.exe')} title="Terminal">
          💻
        </button>
        <button style={styles.dockBtn} onClick={() => handleLaunch('control.exe')} title="Settings">
          ⚙️
        </button>

        <div style={styles.tray}>
          <div style={styles.clockGroup}>
            <span style={styles.clockTime}>{timeStr}</span>
            <span style={styles.clockDate}>{dateStr}</span>
          </div>
        </div>
      </footer>
    </div>
  );
};

const styles: Record<string, React.CSSProperties> = {
  container: {
    display: 'flex',
    flexDirection: 'column',
    width: '100%',
    height: '100%',
    backgroundColor: '#0f172a',
    color: '#f8fafc',
    fontFamily: '"Segoe UI Variable Display", "Segoe UI", system-ui, sans-serif',
    userSelect: 'none',
    overflow: 'hidden',
  },
  desktopCanvas: {
    flex: 1,
    position: 'relative',
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
    background: 'radial-gradient(circle at center, #1e293b 0%, #0f172a 100%)',
  },
  watermark: {
    textAlign: 'center',
    opacity: 0.15,
  },
  watermarkTitle: {
    fontSize: '64px',
    fontWeight: 800,
    letterSpacing: '0.1em',
    margin: 0,
  },
  watermarkSub: {
    fontSize: '16px',
    letterSpacing: '0.2em',
    marginTop: '8px',
  },
  toast: {
    position: 'absolute',
    top: '24px',
    padding: '10px 20px',
    backgroundColor: 'rgba(30, 41, 59, 0.85)',
    border: '1px solid rgba(56, 189, 248, 0.3)',
    borderRadius: '8px',
    color: '#38bdf8',
    fontSize: '13px',
    backdropFilter: 'blur(12px)',
    boxShadow: '0 10px 25px rgba(0, 0, 0, 0.5)',
  },
  startDrawer: {
    position: 'absolute',
    bottom: '16px',
    left: '16px',
    width: '360px',
    backgroundColor: 'rgba(30, 41, 59, 0.85)',
    border: '1px solid rgba(255, 255, 255, 0.1)',
    borderRadius: '16px',
    padding: '20px',
    backdropFilter: 'blur(24px)',
    boxShadow: '0 20px 40px rgba(0, 0, 0, 0.6)',
    zIndex: 100,
  },
  drawerHeader: {
    marginBottom: '16px',
  },
  drawerTitle: {
    fontSize: '12px',
    fontWeight: 600,
    color: '#94a3b8',
    textTransform: 'uppercase',
    letterSpacing: '0.05em',
    margin: 0,
  },
  drawerGrid: {
    display: 'grid',
    gridTemplateColumns: 'repeat(2, 1fr)',
    gap: '12px',
  },
  drawerCard: {
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    padding: '16px 8px',
    backgroundColor: 'rgba(255, 255, 255, 0.03)',
    border: '1px solid rgba(255, 255, 255, 0.05)',
    borderRadius: '10px',
    color: '#e2e8f0',
    cursor: 'pointer',
  },
  cardIcon: {
    fontSize: '24px',
    marginBottom: '6px',
  },
  cardLabel: {
    fontSize: '12px',
  },
  drawerFooter: {
    marginTop: '16px',
    paddingTop: '12px',
    borderTop: '1px solid rgba(255, 255, 255, 0.08)',
  },
  emergencyBtn: {
    width: '100%',
    padding: '10px',
    backgroundColor: 'rgba(225, 29, 72, 0.2)',
    border: '1px solid rgba(225, 29, 72, 0.4)',
    borderRadius: '8px',
    color: '#fecdd3',
    fontSize: '12px',
    fontWeight: 600,
    cursor: 'pointer',
  },
  dock: {
    height: '54px',
    backgroundColor: 'rgba(15, 23, 42, 0.7)',
    borderTop: '1px solid rgba(255, 255, 255, 0.08)',
    backdropFilter: 'blur(20px)',
    display: 'flex',
    alignItems: 'center',
    padding: '0 16px',
    gap: '8px',
    zIndex: 50,
  },
  dockBtn: {
    width: '40px',
    height: '40px',
    borderRadius: '8px',
    border: '1px solid transparent',
    backgroundColor: 'transparent',
    fontSize: '18px',
    color: '#cbd5e1',
    cursor: 'pointer',
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
  },
  startBtn: {
    color: '#38bdf8',
    fontSize: '20px',
  },
  startBtnActive: {
    backgroundColor: 'rgba(56, 189, 248, 0.2)',
    borderColor: 'rgba(56, 189, 248, 0.4)',
  },
  separator: {
    width: '1px',
    height: '20px',
    backgroundColor: 'rgba(255, 255, 255, 0.1)',
    margin: '0 4px',
  },
  tray: {
    marginLeft: 'auto',
    display: 'flex',
    alignItems: 'center',
  },
  clockGroup: {
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'flex-end',
    fontSize: '11px',
  },
  clockTime: {
    fontWeight: 600,
    color: '#e2e8f0',
  },
  clockDate: {
    color: '#64748b',
    fontSize: '10px',
  },
};
