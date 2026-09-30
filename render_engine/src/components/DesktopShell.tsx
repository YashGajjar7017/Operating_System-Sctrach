/**
 * @file DesktopShell.tsx — Xenithra OS v3.0 — Main Desktop Shell
 *
 * The primary render engine shell component. Implements:
 *   - Windows 11 Fluent Design desktop
 *   - Mica / Acrylic glass surfaces
 *   - Animated taskbar with Fluent pill indicators
 *   - Multi-window manager with snap zones
 *   - Start Menu, Action Center, Notification toasts
 *   - Kernel IPC event integration
 *   - NetworkPanel + ManagementPanel integration
 */

import React, { useState, useEffect, useRef, useCallback } from 'react';
import {
  Folder, HardDrive, Cpu, Terminal, Play, Pause, RotateCcw,
  Power, ShieldCheck, Activity, RefreshCw, Moon, Disc,
  Layers, Search, Volume2, Wifi, Calendar, CheckCircle2,
  Globe, Shield, Sliders, Box, Code, Calculator, Package,
  Maximize2, Minimize2, X, Minus, Sparkles, Sun, Battery,
  Bluetooth, Bell, Settings, Radio, ChevronRight, Eye, Monitor,
  Network, Zap, Database, Lock, MemoryStick
} from 'lucide-react';

import { NetworkPanel }    from './NetworkPanel';
import { ManagementPanel } from './ManagementPanel';
import type { WindowState } from '../types';

// ── Window type registry ──────────────────────────────────────────────────────
type AppTag =
  | 'browser' | 'defender' | 'services' | 'media' | 'editor'
  | 'calculator' | 'store' | 'pipeline' | 'web3' | 'terminal'
  | 'network' | 'management' | 'settings' | 'explorer';

// ── Wallpaper variants ────────────────────────────────────────────────────────
type Wallpaper = 'bloom' | 'cyber' | 'cosmos' | 'mist';

// ── Toast ─────────────────────────────────────────────────────────────────────
interface Toast { id: string; title: string; body: string; icon?: string; }

// ── Kernel IPC type (window.xenithra from preload.cjs) ───────────────────────
declare global {
  interface Window {
    xenithra?: {
      launch: (app: string, url?: string) => Promise<unknown>;
      close: (app: string) => Promise<unknown>;
      power: (action: string) => Promise<unknown>;
      query: (target: string) => Promise<unknown>;
      audio: (opts: { volume?: number; mute?: boolean }) => Promise<unknown>;
      network: (action: string, payload?: unknown) => Promise<unknown>;
      driver: (action: string, deviceId?: string) => Promise<unknown>;
      manage: (action: string, target?: string) => Promise<unknown>;
      pipeStatus: () => Promise<{ connected: boolean; pipe: string; engine: string }>;
      onKernelEvent: (cb: (d: unknown) => void) => void;
      onNetworkEvent: (cb: (d: unknown) => void) => void;
      onNotifyEvent: (cb: (d: { title: string; body: string; icon: string }) => void) => void;
      onIpcStatus: (cb: (d: { connected: boolean }) => void) => void;
      removeAllListeners: (ch: string) => void;
    };
  }
}

// ── App registry ──────────────────────────────────────────────────────────────
const APP_REGISTRY: Record<AppTag, { title: string; icon: string; defaultW: number; defaultH: number }> = {
  browser:    { title: 'Xenithra Browser',       icon: '🌐', defaultW: 900, defaultH: 580 },
  defender:   { title: 'Security Center',         icon: '🛡️', defaultW: 860, defaultH: 560 },
  services:   { title: 'Services Manager',        icon: '⚙️', defaultW: 840, defaultH: 560 },
  media:      { title: 'Media Player',            icon: '🎬', defaultW: 780, defaultH: 520 },
  editor:     { title: 'Code Editor',             icon: '📝', defaultW: 860, defaultH: 580 },
  calculator: { title: 'Calculator',              icon: '🔢', defaultW: 340, defaultH: 480 },
  store:      { title: 'App Store',               icon: '🏪', defaultW: 820, defaultH: 560 },
  pipeline:   { title: 'Pipeline Inspector',      icon: '🔬', defaultW: 860, defaultH: 580 },
  web3:       { title: 'Web3 Wallet',             icon: '🔗', defaultW: 860, defaultH: 580 },
  terminal:   { title: 'Kernel Terminal',         icon: '💻', defaultW: 780, defaultH: 500 },
  network:    { title: 'Network & Internet',      icon: '🌐', defaultW: 820, defaultH: 580 },
  management: { title: 'System Management',       icon: '🔧', defaultW: 860, defaultH: 600 },
  settings:   { title: 'System Settings',         icon: '⚙️', defaultW: 820, defaultH: 560 },
  explorer:   { title: 'File Explorer',           icon: '📁', defaultW: 820, defaultH: 540 },
};

// ── Taskbar app defs ──────────────────────────────────────────────────────────
const TASKBAR_PINS: { tag: AppTag; icon: React.ReactNode; label: string }[] = [
  { tag: 'browser',    icon: <Globe size={18} />,      label: 'Browser' },
  { tag: 'explorer',   icon: <Folder size={18} />,     label: 'Explorer' },
  { tag: 'terminal',   icon: <Terminal size={18} />,   label: 'Terminal' },
  { tag: 'editor',     icon: <Code size={18} />,       label: 'Editor' },
  { tag: 'media',      icon: <Play size={18} />,       label: 'Media' },
  { tag: 'defender',   icon: <Shield size={18} />,     label: 'Defender' },
  { tag: 'network',    icon: <Network size={18} />,    label: 'Network' },
  { tag: 'management', icon: <Sliders size={18} />,    label: 'Manage' },
  { tag: 'store',      icon: <Package size={18} />,    label: 'Store' },
  { tag: 'web3',       icon: <Zap size={18} />,        label: 'Web3' },
  { tag: 'settings',   icon: <Settings size={18} />,   label: 'Settings' },
];

// ── START MENU APPS ──────────────────────────────────────────────────────────
const START_MENU_APPS: { tag: AppTag; icon: React.ReactNode; label: string; color: string }[] = [
  { tag: 'browser',    icon: <Globe size={20} />,      label: 'Browser',      color: 'hsl(210,90%,55%)' },
  { tag: 'explorer',   icon: <Folder size={20} />,     label: 'Explorer',     color: 'hsl(38,90%,55%)' },
  { tag: 'editor',     icon: <Code size={20} />,       label: 'Code Editor',  color: 'hsl(250,80%,65%)' },
  { tag: 'terminal',   icon: <Terminal size={20} />,   label: 'Terminal',     color: 'hsl(142,70%,50%)' },
  { tag: 'media',      icon: <Play size={20} />,       label: 'Media Player', color: 'hsl(350,80%,60%)' },
  { tag: 'calculator', icon: <Calculator size={20} />, label: 'Calculator',   color: 'hsl(180,70%,50%)' },
  { tag: 'defender',   icon: <Shield size={20} />,     label: 'Defender',     color: 'hsl(220,80%,60%)' },
  { tag: 'network',    icon: <Network size={20} />,    label: 'Network',      color: 'hsl(200,80%,55%)' },
  { tag: 'management', icon: <Sliders size={20} />,    label: 'Management',   color: 'hsl(28,85%,55%)' },
  { tag: 'services',   icon: <Settings size={20} />,   label: 'Services',     color: 'hsl(260,75%,65%)' },
  { tag: 'store',      icon: <Package size={20} />,    label: 'App Store',    color: 'hsl(320,80%,60%)' },
  { tag: 'web3',       icon: <Zap size={20} />,        label: 'Web3',         color: 'hsl(45,90%,55%)' },
  { tag: 'pipeline',   icon: <Layers size={20} />,     label: 'Pipeline',     color: 'hsl(165,70%,50%)' },
  { tag: 'settings',   icon: <Settings size={20} />,   label: 'Settings',     color: 'hsl(240,50%,60%)' },
];

// ── Utility ───────────────────────────────────────────────────────────────────
let wid = 100;
function makeWindow(tag: AppTag, existing: WindowState[]): WindowState {
  const reg = APP_REGISTRY[tag];
  const offset = existing.length * 22;
  return {
    id: `${tag}-${++wid}`,
    title: reg.title, icon: reg.icon, tag,
    x: 80 + offset, y: 40 + offset,
    width: reg.defaultW, height: reg.defaultH,
    minimized: false, maximized: false, zIndex: 10 + existing.length,
    isFocused: true,
  };
}

// ── Window Content Renderer ───────────────────────────────────────────────────
const WindowContent: React.FC<{ tag: AppTag }> = ({ tag }) => {
  const commonStyle: React.CSSProperties = {
    width: '100%', height: '100%', display: 'flex', alignItems: 'center',
    justifyContent: 'center', flexDirection: 'column', gap: 12,
    color: 'var(--text-secondary)', fontSize: 'var(--text-sm)',
    background: 'rgba(255,255,255,0.01)',
  };

  if (tag === 'network')    return <NetworkPanel />;
  if (tag === 'management') return <ManagementPanel />;

  const appInfo: Record<string, { emoji: string; name: string; desc: string }> = {
    browser:    { emoji: '🌐', name: 'Xenithra Browser',     desc: 'Web browser powered by Chromium' },
    defender:   { emoji: '🛡️', name: 'Security Center',      desc: 'Firewall · Anti-hijack · Session guard' },
    services:   { emoji: '⚙️', name: 'Services Manager',     desc: 'SysMain · MMCSS · AudioSrv · WMI' },
    media:      { emoji: '🎬', name: 'Media Player',          desc: 'VLC-compatible media playback' },
    editor:     { emoji: '📝', name: 'Code Editor',           desc: 'VSCode-lite syntax editor' },
    calculator: { emoji: '🔢', name: 'Calculator',            desc: 'System calculator utility' },
    store:      { emoji: '🏪', name: 'App Store',             desc: 'Browse and install applications' },
    pipeline:   { emoji: '🔬', name: 'Pipeline Inspector',    desc: 'Render pipeline telemetry & tracing' },
    web3:       { emoji: '🔗', name: 'Web3 Wallet',           desc: 'EIP-6963 multi-wallet · ENS · IPFS' },
    terminal:   { emoji: '💻', name: 'Kernel Terminal',        desc: 'GDB-capable kernel debug shell' },
    settings:   { emoji: '⚙️', name: 'System Settings',       desc: 'Personalize your OS experience' },
    explorer:   { emoji: '📁', name: 'File Explorer',          desc: 'Browse files and directories' },
  };

  const info = appInfo[tag];
  if (!info) return <div style={commonStyle}>Unknown App</div>;

  return (
    <div style={{ ...commonStyle, gap: 20 }}>
      <div style={{ fontSize: 52 }}>{info.emoji}</div>
      <div style={{ textAlign: 'center' }}>
        <div style={{ fontSize: 'var(--text-xl)', fontWeight: 700, color: 'var(--text-primary)', marginBottom: 6, fontFamily: 'var(--font-brand)' }}>{info.name}</div>
        <div style={{ fontSize: 'var(--text-sm)', color: 'var(--text-tertiary)' }}>{info.desc}</div>
      </div>
      <div style={{ display: 'flex', gap: 12 }}>
        <span className="pill pill-blue" style={{ fontSize: 11 }}>● ONLINE</span>
        <span className="pill pill-gray" style={{ fontSize: 11 }}>Render Engine v3.0</span>
      </div>
      <div style={{ fontSize: 11, color: 'var(--text-tertiary)', fontFamily: 'var(--font-mono)' }}>
        Migrated from desktop_shell → render_engine
      </div>
    </div>
  );
};

// ── Main Shell ────────────────────────────────────────────────────────────────
export const DesktopShell: React.FC = () => {
  const [windows, setWindows]         = useState<WindowState[]>([]);
  const [activeId, setActiveId]       = useState<string>('');
  const [topZ, setTopZ]               = useState(20);

  const [wallpaper, setWallpaper]     = useState<Wallpaper>('bloom');
  const [startOpen, setStartOpen]     = useState(false);
  const [actionOpen, setActionOpen]   = useState(false);
  const [calendarOpen, setCalendarOpen] = useState(false);
  const [searchQuery, setSearchQuery] = useState('');

  const [wifiOn, setWifiOn]           = useState(true);
  const [btOn, setBtOn]               = useState(true);
  const [nightLight, setNightLight]   = useState(false);
  const [volume, setVolume]           = useState(80);
  const [brightness, setBrightness]   = useState(90);
  const [kernelConnected, setKernelConnected] = useState(false);
  const [toasts, setToasts]           = useState<Toast[]>([]);

  const [timeStr, setTimeStr]         = useState('');
  const [dateStr, setDateStr]         = useState('');

  const dragRef = useRef<{ winId: string; startX: number; startY: number; initX: number; initY: number } | null>(null);
  const resizeRef = useRef<{ winId: string; edge: string; startX: number; startY: number; initW: number; initH: number; initX: number; initY: number } | null>(null);

  // ── Clock ──────────────────────────────────────────────────────────────
  useEffect(() => {
    const tick = () => {
      const now = new Date();
      setTimeStr(now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }));
      setDateStr(now.toLocaleDateString([], { month: 'short', day: 'numeric', year: 'numeric' }));
    };
    tick();
    const iv = setInterval(tick, 15000);
    return () => clearInterval(iv);
  }, []);

  // ── Kernel IPC ────────────────────────────────────────────────────────
  useEffect(() => {
    if (!window.xenithra) return;
    window.xenithra.onIpcStatus(d => setKernelConnected(d.connected));
    window.xenithra.onNotifyEvent(d => {
      const id = Math.random().toString(36).slice(2);
      setToasts(prev => [...prev, { id, title: d.title, body: d.body, icon: d.icon }]);
      setTimeout(() => setToasts(prev => prev.filter(t => t.id !== id)), 5000);
    });
    window.xenithra.pipeStatus().then(s => setKernelConnected(s.connected)).catch(() => {});
  }, []);

  // ── Toast dismiss ──────────────────────────────────────────────────────
  const dismissToast = useCallback((id: string) => {
    setToasts(prev => prev.filter(t => t.id !== id));
  }, []);

  // ── Window management ──────────────────────────────────────────────────
  const openApp = useCallback((tag: AppTag) => {
    setWindows(prev => {
      const existing = prev.find(w => w.tag === tag && !w.minimized);
      if (existing) {
        focusWindow(existing.id);
        return prev;
      }
      const win = makeWindow(tag, prev);
      setActiveId(win.id);
      setTopZ(z => z + 1);
      return [...prev, win];
    });
    setStartOpen(false);
  }, []);

  const focusWindow = useCallback((id: string) => {
    setActiveId(id);
    setTopZ(z => {
      const nz = z + 1;
      setWindows(prev => prev.map(w => w.id === id ? { ...w, zIndex: nz, minimized: false } : w));
      return nz;
    });
  }, []);

  const closeWindow = useCallback((id: string, e: React.MouseEvent) => {
    e.stopPropagation();
    setWindows(prev => prev.filter(w => w.id !== id));
  }, []);

  const minimizeWindow = useCallback((id: string, e: React.MouseEvent) => {
    e.stopPropagation();
    setWindows(prev => prev.map(w => w.id === id ? { ...w, minimized: true } : w));
    setActiveId('');
  }, []);

  const maximizeWindow = useCallback((id: string, e: React.MouseEvent) => {
    e.stopPropagation();
    setWindows(prev => prev.map(w => w.id === id ? { ...w, maximized: !w.maximized } : w));
  }, []);

  // ── Drag ──────────────────────────────────────────────────────────────
  const onTitlebarMouseDown = useCallback((e: React.MouseEvent, id: string) => {
    if ((e.target as HTMLElement).closest('button')) return;
    e.preventDefault();
    const win = windows.find(w => w.id === id);
    if (!win || win.maximized) return;
    focusWindow(id);
    dragRef.current = { winId: id, startX: e.clientX, startY: e.clientY, initX: win.x, initY: win.y };
  }, [windows, focusWindow]);

  useEffect(() => {
    const onMove = (e: MouseEvent) => {
      if (!dragRef.current) return;
      const { winId, startX, startY, initX, initY } = dragRef.current;
      setWindows(prev => prev.map(w => w.id === winId ? { ...w, x: initX + e.clientX - startX, y: Math.max(0, initY + e.clientY - startY) } : w));
    };
    const onUp = () => { dragRef.current = null; resizeRef.current = null; };
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
    return () => { window.removeEventListener('mousemove', onMove); window.removeEventListener('mouseup', onUp); };
  }, []);

  // ── Close flyouts on outside click ────────────────────────────────────
  useEffect(() => {
    const close = () => { setStartOpen(false); setActionOpen(false); setCalendarOpen(false); };
    window.addEventListener('mousedown', close);
    return () => window.removeEventListener('mousedown', close);
  }, []);

  const openWindows = windows.filter(w => !w.minimized);
  const TASKBAR_H = 48;

  return (
    <div style={{ width: '100vw', height: '100vh', overflow: 'hidden', position: 'relative' }}>

      {/* ── Wallpaper ─────────────────────────────────────────────────── */}
      <div className={`desktop-bg wallpaper-${wallpaper}`} style={{ position: 'fixed', inset: 0 }} />

      {/* ── Desktop Icons ─────────────────────────────────────────────── */}
      <div style={{ position: 'fixed', top: 20, left: 20, display: 'flex', flexDirection: 'column', gap: 8, zIndex: 1 }}>
        {[
          { tag: 'explorer' as AppTag, label: 'File Explorer', icon: '📁' },
          { tag: 'terminal' as AppTag, label: 'Terminal',       icon: '💻' },
          { tag: 'network'  as AppTag, label: 'Network',        icon: '🌐' },
        ].map(item => (
          <div
            key={item.tag}
            onDoubleClick={() => openApp(item.tag)}
            style={{
              display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 4,
              width: 72, padding: '8px 4px', borderRadius: 'var(--radius-md)',
              cursor: 'pointer', transition: 'background 0.12s',
            }}
            className="hover-lift"
          >
            <span style={{ fontSize: 28 }}>{item.icon}</span>
            <span style={{ fontSize: 11, color: 'var(--text-primary)', textAlign: 'center', textShadow: '0 1px 4px rgba(0,0,0,0.9)', lineHeight: 1.3 }}>{item.label}</span>
          </div>
        ))}
      </div>

      {/* ── Windows ───────────────────────────────────────────────────── */}
      {windows.map(win => {
        if (win.minimized) return null;
        const isActive = win.id === activeId;
        const style: React.CSSProperties = win.maximized
          ? { position: 'fixed', left: 0, top: 0, width: '100vw', height: `calc(100vh - ${TASKBAR_H}px)`, zIndex: win.zIndex }
          : { position: 'fixed', left: win.x, top: win.y, width: win.width, height: win.height, zIndex: win.zIndex };

        return (
          <div
            key={win.id}
            className={`window-surface window-enter ${isActive ? 'window-active-border' : ''}`}
            style={{ ...style, display: 'flex', flexDirection: 'column', transition: win.maximized ? 'all 0.2s var(--ease-fluent)' : 'box-shadow 0.15s' }}
            onMouseDown={() => focusWindow(win.id)}
          >
            {/* Titlebar */}
            <div
              onMouseDown={e => onTitlebarMouseDown(e, win.id)}
              style={{
                height: 36, display: 'flex', alignItems: 'center', gap: 8,
                padding: '0 8px 0 14px', flexShrink: 0,
                background: isActive ? 'rgba(255,255,255,0.04)' : 'rgba(255,255,255,0.02)',
                borderBottom: '1px solid var(--border-subtle)',
                cursor: win.maximized ? 'default' : 'grab',
              }}
            >
              <span style={{ fontSize: 14 }}>{win.icon}</span>
              <span style={{ flex: 1, fontSize: 'var(--text-xs)', fontWeight: 500, color: isActive ? 'var(--text-primary)' : 'var(--text-tertiary)', overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{win.title}</span>

              {/* Window controls */}
              {[
                { icon: <Minus size={12} />, action: (e: React.MouseEvent) => minimizeWindow(win.id, e), color: 'hsl(38,90%,60%)', label: 'minimize' },
                { icon: <Maximize2 size={11} />, action: (e: React.MouseEvent) => maximizeWindow(win.id, e), color: 'hsl(142,70%,50%)', label: 'maximize' },
                { icon: <X size={12} />, action: (e: React.MouseEvent) => closeWindow(win.id, e), color: 'hsl(350,80%,60%)', label: 'close', hover: 'rgba(240,50,50,0.25)' },
              ].map(btn => (
                <button
                  key={btn.label}
                  onClick={btn.action}
                  style={{
                    width: 28, height: 22, border: 'none', borderRadius: 4,
                    background: 'rgba(255,255,255,0.06)', cursor: 'pointer',
                    display: 'flex', alignItems: 'center', justifyContent: 'center',
                    color: 'var(--text-secondary)', transition: 'all 0.12s',
                  }}
                  onMouseEnter={e => {
                    (e.currentTarget as HTMLButtonElement).style.background =
                      btn.label === 'close' ? 'rgba(240,50,50,0.3)' : 'rgba(255,255,255,0.14)';
                    (e.currentTarget as HTMLButtonElement).style.color = btn.color;
                  }}
                  onMouseLeave={e => {
                    (e.currentTarget as HTMLButtonElement).style.background = 'rgba(255,255,255,0.06)';
                    (e.currentTarget as HTMLButtonElement).style.color = 'var(--text-secondary)';
                  }}
                >
                  {btn.icon}
                </button>
              ))}
            </div>

            {/* Content */}
            <div style={{ flex: 1, overflow: 'hidden', position: 'relative' }}>
              <WindowContent tag={win.tag as AppTag} />
            </div>
          </div>
        );
      })}

      {/* ── Taskbar ───────────────────────────────────────────────────── */}
      <div
        className="acrylic-taskbar"
        style={{
          position: 'fixed', bottom: 0, left: 0, right: 0,
          height: TASKBAR_H, zIndex: 'var(--z-taskbar)' as never,
          display: 'flex', alignItems: 'center', justifyContent: 'space-between',
          padding: '0 12px',
        }}
        onMouseDown={e => e.stopPropagation()}
      >
        {/* Start button */}
        <button
          onClick={e => { e.stopPropagation(); setStartOpen(o => !o); setActionOpen(false); }}
          style={{
            width: 38, height: 34, border: 'none', borderRadius: 'var(--radius-sm)',
            background: startOpen ? 'rgba(96,165,250,0.18)' : 'transparent',
            cursor: 'pointer', display: 'flex', alignItems: 'center', justifyContent: 'center',
            transition: 'all 0.15s', color: startOpen ? 'var(--clr-accent-primary)' : 'var(--text-secondary)',
          }}
          onMouseEnter={e => (e.currentTarget.style.background = 'rgba(255,255,255,0.08)')}
          onMouseLeave={e => (e.currentTarget.style.background = startOpen ? 'rgba(96,165,250,0.18)' : 'transparent')}
        >
          <Sparkles size={18} />
        </button>

        {/* Pinned apps + open windows — centered */}
        <div style={{ position: 'absolute', left: '50%', transform: 'translateX(-50%)', display: 'flex', alignItems: 'center', gap: 2 }}>
          {TASKBAR_PINS.map(pin => {
            const openWins = windows.filter(w => w.tag === pin.tag && !w.minimized);
            const isOpen = openWins.length > 0;
            const isActive = openWins.some(w => w.id === activeId);
            return (
              <div
                key={pin.tag}
                className={`taskbar-pill ${isActive ? 'active' : ''}`}
                onClick={() => {
                  if (!isOpen) openApp(pin.tag);
                  else if (isActive) { const w = openWins[0]; setWindows(prev => prev.map(x => x.id === w.id ? { ...x, minimized: true } : x)); setActiveId(''); }
                  else focusWindow(openWins[0].id);
                }}
                title={pin.label}
                style={{
                  width: 42, height: 38, borderRadius: 'var(--radius-md)',
                  display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', gap: 2,
                  cursor: 'pointer', position: 'relative',
                  color: isActive ? 'var(--clr-accent-primary)' : isOpen ? 'var(--text-primary)' : 'var(--text-secondary)',
                  background: isActive ? 'rgba(96,165,250,0.12)' : 'transparent',
                }}
              >
                {pin.icon}
              </div>
            );
          })}
        </div>

        {/* Right tray */}
        <div style={{ display: 'flex', alignItems: 'center', gap: 4 }}>
          {/* Kernel status */}
          <div title={kernelConnected ? 'Kernel pipe connected' : 'Kernel pipe disconnected'}
            style={{ width: 7, height: 7, borderRadius: '50%', background: kernelConnected ? 'var(--clr-accent-success)' : 'var(--clr-accent-danger)' }} />

          {/* System tray icons */}
          {[
            { icon: <Wifi size={14} />, label: 'Network' },
            { icon: <Volume2 size={14} />, label: 'Volume' },
            { icon: <Battery size={14} />, label: 'Battery' },
          ].map(t => (
            <button key={t.label} title={t.label} style={{ width: 30, height: 30, border: 'none', borderRadius: 6, background: 'transparent', cursor: 'pointer', color: 'var(--text-secondary)', display: 'flex', alignItems: 'center', justifyContent: 'center', transition: 'all 0.12s' }}
              onClick={e => { e.stopPropagation(); setActionOpen(o => !o); }}
              onMouseEnter={e => (e.currentTarget.style.background = 'rgba(255,255,255,0.08)')}
              onMouseLeave={e => (e.currentTarget.style.background = 'transparent')}
            >
              {t.icon}
            </button>
          ))}

          {/* Clock */}
          <button
            onClick={e => { e.stopPropagation(); setCalendarOpen(o => !o); setActionOpen(false); }}
            style={{ padding: '4px 8px', border: 'none', borderRadius: 6, background: calendarOpen ? 'rgba(96,165,250,0.14)' : 'transparent', cursor: 'pointer', color: 'var(--text-primary)', textAlign: 'right', transition: 'all 0.12s' }}
            onMouseEnter={e => (e.currentTarget.style.background = 'rgba(255,255,255,0.08)')}
            onMouseLeave={e => (e.currentTarget.style.background = calendarOpen ? 'rgba(96,165,250,0.14)' : 'transparent')}
          >
            <div style={{ fontSize: 12, fontWeight: 600, fontVariantNumeric: 'tabular-nums' }}>{timeStr}</div>
            <div style={{ fontSize: 10, color: 'var(--text-tertiary)' }}>{dateStr}</div>
          </button>

          {/* Notifications */}
          <button onClick={e => { e.stopPropagation(); setActionOpen(o => !o); }} style={{ width: 30, height: 30, border: 'none', borderRadius: 6, background: 'transparent', cursor: 'pointer', color: 'var(--text-secondary)', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
            <Bell size={14} />
            {toasts.length > 0 && <span style={{ position: 'absolute', top: 8, right: 8, width: 6, height: 6, borderRadius: '50%', background: 'var(--clr-accent-primary)' }} />}
          </button>
        </div>
      </div>

      {/* ── Start Menu ────────────────────────────────────────────────── */}
      {startOpen && (
        <div
          className="mica-elevated flyout-enter-up"
          onMouseDown={e => e.stopPropagation()}
          style={{
            position: 'fixed', bottom: TASKBAR_H + 8, left: '50%', transform: 'translateX(-50%)',
            width: 640, maxHeight: 560, borderRadius: 'var(--radius-2xl)',
            zIndex: 'var(--z-flyout)' as never, padding: 20, overflow: 'hidden',
            display: 'flex', flexDirection: 'column', gap: 16,
          }}
        >
          {/* Search */}
          <div style={{ position: 'relative' }}>
            <Search size={14} style={{ position: 'absolute', left: 12, top: '50%', transform: 'translateY(-50%)', color: 'var(--text-tertiary)' }} />
            <input
              className="input-surface"
              placeholder="Search apps, files, and settings…"
              value={searchQuery}
              onChange={e => setSearchQuery(e.target.value)}
              style={{ width: '100%', paddingLeft: 34, paddingRight: 12, height: 38, fontSize: 'var(--text-sm)', borderRadius: 'var(--radius-lg)' }}
              autoFocus
            />
          </div>

          <div className="section-header" style={{ padding: 0 }}>Pinned Apps</div>
          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(7, 1fr)', gap: 6 }}>
            {START_MENU_APPS
              .filter(a => !searchQuery || a.label.toLowerCase().includes(searchQuery.toLowerCase()))
              .map(app => (
                <div
                  key={app.tag}
                  onClick={() => openApp(app.tag)}
                  className="hover-lift"
                  style={{
                    display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 5,
                    padding: '10px 4px', borderRadius: 'var(--radius-md)',
                    background: 'rgba(255,255,255,0.04)', cursor: 'pointer',
                    transition: 'all 0.15s', border: '1px solid var(--border-subtle)',
                  }}
                  onMouseEnter={e => { (e.currentTarget as HTMLDivElement).style.background = 'rgba(255,255,255,0.09)'; }}
                  onMouseLeave={e => { (e.currentTarget as HTMLDivElement).style.background = 'rgba(255,255,255,0.04)'; }}
                >
                  <div style={{ width: 32, height: 32, borderRadius: 8, background: `${app.color}20`, display: 'flex', alignItems: 'center', justifyContent: 'center', color: app.color }}>
                    {app.icon}
                  </div>
                  <span style={{ fontSize: 10, color: 'var(--text-secondary)', textAlign: 'center', lineHeight: 1.2 }}>{app.label}</span>
                </div>
              ))}
          </div>

          {/* Power row */}
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', borderTop: '1px solid var(--border-subtle)', paddingTop: 12 }}>
            <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)' }}>Xenithra OS v3.0 · Render Engine</div>
            <div style={{ display: 'flex', gap: 6 }}>
              {[
                { icon: <Lock size={13} />, label: 'Lock',     action: 'lock' },
                { icon: <Power size={13} />, label: 'Shut down', action: 'shutdown' },
              ].map(btn => (
                <button key={btn.label} className="btn btn-ghost" style={{ fontSize: 11, padding: '5px 10px', gap: 5 }}
                  onClick={() => window.xenithra?.power(btn.action)}>
                  {btn.icon} {btn.label}
                </button>
              ))}
            </div>
          </div>
        </div>
      )}

      {/* ── Action Center ─────────────────────────────────────────────── */}
      {actionOpen && (
        <div
          className="mica-elevated flyout-enter-up"
          onMouseDown={e => e.stopPropagation()}
          style={{
            position: 'fixed', bottom: TASKBAR_H + 8, right: 8,
            width: 340, borderRadius: 'var(--radius-2xl)',
            zIndex: 'var(--z-flyout)' as never, padding: 16, display: 'flex', flexDirection: 'column', gap: 14,
          }}
        >
          {/* Quick settings grid */}
          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: 8 }}>
            {[
              { label: 'Wi-Fi', icon: <Wifi size={16} />, active: wifiOn, toggle: () => setWifiOn(o => !o) },
              { label: 'Bluetooth', icon: <Bluetooth size={16} />, active: btOn, toggle: () => setBtOn(o => !o) },
              { label: 'Night Light', icon: <Moon size={16} />, active: nightLight, toggle: () => setNightLight(o => !o) },
              { label: 'Network', icon: <Network size={16} />, active: true, toggle: () => openApp('network') },
              { label: 'Manage', icon: <Sliders size={16} />, active: true, toggle: () => openApp('management') },
              { label: 'Settings', icon: <Settings size={16} />, active: true, toggle: () => openApp('settings') },
            ].map(q => (
              <button key={q.label} onClick={() => { q.toggle(); }} style={{
                padding: '10px 8px', borderRadius: 'var(--radius-md)', border: '1px solid var(--border-subtle)',
                background: q.active ? 'rgba(96,165,250,0.15)' : 'rgba(255,255,255,0.04)',
                color: q.active ? 'var(--clr-accent-primary)' : 'var(--text-secondary)',
                cursor: 'pointer', display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 5, transition: 'all 0.15s'
              }}>
                {q.icon}
                <span style={{ fontSize: 10 }}>{q.label}</span>
              </button>
            ))}
          </div>

          {/* Sliders */}
          {[
            { label: 'Volume', icon: <Volume2 size={13} />, val: volume, set: setVolume },
            { label: 'Brightness', icon: <Sun size={13} />, val: brightness, set: setBrightness },
          ].map(s => (
            <div key={s.label}>
              <div style={{ display: 'flex', gap: 8, alignItems: 'center', marginBottom: 5 }}>
                <span style={{ color: 'var(--text-tertiary)' }}>{s.icon}</span>
                <span style={{ flex: 1, fontSize: 'var(--text-xs)', color: 'var(--text-secondary)' }}>{s.label}</span>
                <span style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)', fontFamily: 'var(--font-mono)' }}>{s.val}%</span>
              </div>
              <input type="range" min={0} max={100} value={s.val}
                onChange={e => s.set(Number(e.target.value))}
                style={{ width: '100%', accentColor: 'var(--clr-accent-primary)', cursor: 'pointer' }} />
            </div>
          ))}

          {/* Wallpaper switcher */}
          <div>
            <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)', marginBottom: 8 }}>Wallpaper</div>
            <div style={{ display: 'flex', gap: 6 }}>
              {(['bloom', 'cyber', 'cosmos', 'mist'] as Wallpaper[]).map(w => (
                <button key={w} onClick={() => setWallpaper(w)} style={{
                  flex: 1, height: 28, borderRadius: 'var(--radius-sm)', border: wallpaper === w ? '2px solid var(--clr-accent-primary)' : '1px solid var(--border-subtle)',
                  background: w === 'bloom' ? 'linear-gradient(135deg, #f0abfc, #60a5fa)' :
                              w === 'cyber' ? 'linear-gradient(135deg, #00d4ff, #7828c8)' :
                              w === 'cosmos' ? 'linear-gradient(135deg, #a78bfa, #60a5fa)' :
                              'linear-gradient(135deg, #334155, #1e293b)',
                  cursor: 'pointer', fontSize: 9, color: '#fff', fontWeight: 600, textTransform: 'capitalize'
                }}>{w}</button>
              ))}
            </div>
          </div>
        </div>
      )}

      {/* ── Calendar Flyout ────────────────────────────────────────────── */}
      {calendarOpen && (
        <div
          className="mica-elevated flyout-enter-up"
          onMouseDown={e => e.stopPropagation()}
          style={{
            position: 'fixed', bottom: TASKBAR_H + 8, right: 8,
            width: 280, borderRadius: 'var(--radius-2xl)',
            zIndex: 'var(--z-flyout)' as never, padding: 20, display: 'flex', flexDirection: 'column', gap: 12,
          }}
        >
          <div style={{ fontSize: 'var(--text-3xl)', fontWeight: 800, fontFamily: 'var(--font-brand)', textAlign: 'center', background: 'var(--grad-hero)', WebkitBackgroundClip: 'text', WebkitTextFillColor: 'transparent' }}>
            {timeStr}
          </div>
          <div style={{ textAlign: 'center', color: 'var(--text-tertiary)', fontSize: 'var(--text-sm)' }}>{dateStr}</div>
          <div className="divider" />
          <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)', textAlign: 'center' }}>Xenithra OS v3.0 · Render Engine Active</div>
        </div>
      )}

      {/* ── Toast Notifications ────────────────────────────────────────── */}
      <div style={{ position: 'fixed', bottom: TASKBAR_H + 16, right: 16, display: 'flex', flexDirection: 'column-reverse', gap: 10, zIndex: 'var(--z-toast)' as never, width: 320 }}>
        {toasts.map(t => (
          <div key={t.id} className="mica-elevated toast-enter" style={{ padding: '14px 16px', borderRadius: 'var(--radius-xl)', display: 'flex', gap: 12, alignItems: 'flex-start' }}>
            <span style={{ fontSize: 20 }}>{t.icon || '🔔'}</span>
            <div style={{ flex: 1 }}>
              <div style={{ fontSize: 'var(--text-sm)', fontWeight: 600, marginBottom: 3 }}>{t.title}</div>
              <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-secondary)' }}>{t.body}</div>
            </div>
            <button onClick={() => dismissToast(t.id)} style={{ background: 'none', border: 'none', cursor: 'pointer', color: 'var(--text-tertiary)', padding: 2 }}>
              <X size={13} />
            </button>
          </div>
        ))}
      </div>
    </div>
  );
};
