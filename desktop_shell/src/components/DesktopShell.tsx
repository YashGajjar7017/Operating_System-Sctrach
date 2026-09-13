import React, { useState, useEffect, useRef } from 'react';
import { 
  Folder, HardDrive, Cpu, Terminal, Play, Pause, RotateCcw, 
  Power, ShieldCheck, Activity, RefreshCw, Moon, Disc, 
  Layers, Search, Volume2, Wifi, Calendar, CheckCircle2,
  Globe, Shield, Sliders, Box, Code, Calculator, Package,
  Maximize2, Minimize2, X, Minus, Sparkles, Sun, Battery,
  Bluetooth, Bell, Settings, Radio, ChevronRight, Eye, Monitor
} from 'lucide-react';

import { DefenderApp } from './DefenderApp';
import { BrowserApp } from './BrowserApp';
import { ServicesApp } from './ServicesApp';
import { MediaPlayerApp } from './MediaPlayerApp';
import { EditorApp } from './EditorApp';
import { CalculatorApp } from './CalculatorApp';
import { StoreApp } from './StoreApp';
import { PipelineInspectorApp } from './PipelineInspectorApp';
import { WindowState } from '../types';

export const DesktopShell: React.FC = () => {
  // Window State Management
  const [windows, setWindows] = useState<WindowState[]>([
    { id: 'browser', title: 'Microsoft Edge - Google', icon: '🌐', tag: 'browser', x: 80, y: 30, width: 880, height: 560, minimized: false, maximized: false, zIndex: 10 },
    { id: 'defender', title: 'Windows Defender Security Center', icon: '🛡️', tag: 'defender', x: 140, y: 60, width: 840, height: 540, minimized: false, maximized: false, zIndex: 11 },
  ]);

  const [activeWindowId, setActiveWindowId] = useState<string>('browser');
  const [topZIndex, setTopZIndex] = useState(20);

  // System Flyouts
  const [startMenuOpen, setStartMenuOpen] = useState(false);
  const [actionCenterOpen, setActionCenterOpen] = useState(false);
  const [calendarOpen, setCalendarOpen] = useState(false);
  const [widgetsOpen, setWidgetsOpen] = useState(false);
  const [powerMenuOpen, setPowerMenuOpen] = useState(false);
  const [searchQuery, setSearchQuery] = useState('');
  const [contextMenu, setContextMenu] = useState<{ x: number; y: number; visible: boolean }>({ x: 0, y: 0, visible: false });

  // Quick settings state
  const [wifiEnabled, setWifiEnabled] = useState(true);
  const [bluetoothEnabled, setBluetoothEnabled] = useState(true);
  const [airplaneMode, setAirplaneMode] = useState(false);
  const [nightLight, setNightLight] = useState(false);
  const [volumeLevel, setVolumeLevel] = useState(80);
  const [brightnessLevel, setBrightnessLevel] = useState(90);

  // Wallpaper & Theme state
  const [currentWallpaper, setCurrentWallpaper] = useState<'bloom' | 'cyber' | 'cosmos' | 'mist'>('bloom');

  // Selected icon on desktop
  const [selectedDesktopIcon, setSelectedDesktopIcon] = useState<string | null>(null);

  // Sector Cloner Telemetry
  const [cloneState, setCloneState] = useState<'idle' | 'cloning' | 'paused' | 'complete'>('idle');
  const [currentLba, setCurrentLba] = useState(0);
  const totalLba = 131072; // 64 MB
  const [elapsedSec, setElapsedSec] = useState(0);

  // Task Manager Telemetry
  const [cpuUsage, setCpuUsage] = useState(54);
  const [cpuWaveform, setCpuWaveform] = useState<number[]>([45, 52, 60, 58, 64, 55, 62, 70, 65, 58, 54, 59, 63, 68, 58, 55, 62, 67, 59, 56, 61, 64, 58, 60]);

  // Terminal State
  const [terminalHistory, setTerminalHistory] = useState<string[]>([
    'Xenithra OS v2.0.0 (x86_64 Higher-Half Kernel)',
    'Type "help" for a list of available system commands.',
  ]);
  const [terminalInput, setTerminalInput] = useState('');

  // Clock
  const [timeStr, setTimeStr] = useState('12:45 PM');
  const [dateStr, setDateStr] = useState('2026-09-13');

  useEffect(() => {
    const timer = setInterval(() => {
      const now = new Date();
      setTimeStr(now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }));
      setDateStr(now.toISOString().split('T')[0]);
    }, 1000);
    return () => clearInterval(timer);
  }, []);

  // Sector Cloner dynamic simulation loop
  useEffect(() => {
    let interval: any = null;
    if (cloneState === 'cloning') {
      interval = setInterval(() => {
        setCurrentLba((prev) => {
          const next = prev + 1024;
          if (next >= totalLba) {
            setCloneState('complete');
            return totalLba;
          }
          return next;
        });
        setElapsedSec((prev) => prev + 1);
      }, 100);
    }
    return () => clearInterval(interval);
  }, [cloneState]);

  // Task Manager dynamic CPU telemetry jitter
  useEffect(() => {
    const jitter = setInterval(() => {
      const nextVal = Math.floor(48 + Math.random() * 22);
      setCpuUsage(nextVal);
      setCpuWaveform((prev) => [...prev.slice(1), nextVal]);
    }, 1000);
    return () => clearInterval(jitter);
  }, []);

  // Window Manager Actions
  const openWindow = (tag: string, title?: string, icon?: string) => {
    const existing = windows.find((w) => w.tag === tag);
    if (existing) {
      if (existing.minimized) {
        setWindows((prev) =>
          prev.map((w) => (w.tag === tag ? { ...w, minimized: false, zIndex: topZIndex + 1 } : w))
        );
      } else {
        setWindows((prev) =>
          prev.map((w) => (w.tag === tag ? { ...w, zIndex: topZIndex + 1 } : w))
        );
      }
      setTopZIndex((z) => z + 1);
      setActiveWindowId(existing.id);
      return;
    }

    const appTitles: Record<string, { title: string; icon: string; w: number; h: number }> = {
      defender: { title: 'Windows Defender Security Center', icon: '🛡️', w: 840, h: 540 },
      browser: { title: 'Microsoft Edge - Google', icon: '🌐', w: 880, h: 560 },
      services: { title: 'Services Management Console (services.msc)', icon: '⚙️', w: 860, h: 520 },
      vlc: { title: 'VLC Media Player', icon: '🟠', w: 800, h: 520 },
      pipeline: { title: 'Architectural Pipeline Inspector', icon: '⚡', w: 900, h: 580 },
      explorer: { title: 'This PC - Xenithra Explorer', icon: '📁', w: 760, h: 480 },
      diskclone: { title: 'Sector-by-Sector Disk Cloner', icon: '💽', w: 780, h: 520 },
      taskmgr: { title: 'Task Manager (V8 & Kernel Telemetry)', icon: '📊', w: 760, h: 500 },
      editor: { title: 'VS Code Lite - kernel/main.c', icon: '💻', w: 800, h: 520 },
      calc: { title: 'Calculator', icon: '🧮', w: 340, h: 460 },
      store: { title: 'Microsoft Store', icon: '📦', w: 820, h: 540 },
      terminal: { title: 'PowerShell / Kernel Terminal', icon: '⌨️', w: 720, h: 440 },
    };

    const info = appTitles[tag] || { title: title || tag, icon: icon || '📄', w: 740, h: 480 };
    const offset = (windows.length % 6) * 24;

    const newWin: WindowState = {
      id: tag,
      title: info.title,
      icon: info.icon,
      tag,
      x: 60 + offset,
      y: 40 + offset,
      width: info.w,
      height: info.h,
      minimized: false,
      maximized: false,
      zIndex: topZIndex + 1,
    };

    setTopZIndex((z) => z + 1);
    setWindows((prev) => [...prev, newWin]);
    setActiveWindowId(tag);
  };

  const closeWindow = (tag: string) => {
    setWindows((prev) => prev.filter((w) => w.tag !== tag));
  };

  const minimizeWindow = (tag: string) => {
    setWindows((prev) =>
      prev.map((w) => (w.tag === tag ? { ...w, minimized: true } : w))
    );
  };

  const toggleMaximizeWindow = (tag: string) => {
    setWindows((prev) =>
      prev.map((w) => (w.tag === tag ? { ...w, maximized: !w.maximized } : w))
    );
  };

  const focusWindow = (tag: string) => {
    setWindows((prev) =>
      prev.map((w) => (w.tag === tag ? { ...w, minimized: false, zIndex: topZIndex + 1 } : w))
    );
    setTopZIndex((z) => z + 1);
    setActiveWindowId(tag);
  };

  const handleSnap = (tag: string, snap: 'left' | 'right' | 'top-left' | 'top-right') => {
    setWindows((prev) =>
      prev.map((w) => (w.tag === tag ? { ...w, snapState: snap, maximized: false } : w))
    );
  };

  // Terminal Command Dispatcher
  const handleTerminalSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    const cmd = terminalInput.trim();
    if (!cmd) return;

    let response = '';
    const parts = cmd.split(' ');
    const mainCmd = parts[0].toLowerCase();

    switch (mainCmd) {
      case 'help':
        response = 'Commands: help, ps, kill, services, firewall, scan, netstat, ping, v8-eval, cls, sysinfo, date';
        break;
      case 'sysinfo':
        response = 'Xenithra OS 64-bit | Ring 0 Kernel + V8 Engine Host | SMEP=ON SMAP=ON | RAM: 5.07 GB / 8.00 GB';
        break;
      case 'services':
        response = 'Active Services: XenithraKernel (PID 1), WinDefend (PID 104), MpsSvc (PID 108), Dnscache (PID 112), V8HostSvc (PID 220)';
        break;
      case 'firewall':
        response = 'Firewall Status: ENFORCING (6 Rules Active) | Port 80/443/53 ALLOW | Port 22/445 DROP';
        break;
      case 'scan':
        response = 'Defender Quick Scan initiated. Inspected 4,820 objects in 0.4s. 0 Threats found.';
        break;
      case 'netstat':
        response = 'TCP  0.0.0.0:443    0.0.0.0:0  LISTENING\nUDP  0.0.0.0:53     0.0.0.0:0  BOUND (DNSCache)';
        break;
      case 'ping':
        const host = parts[1] || 'www.google.com';
        response = `PING ${host} (142.250.190.46): 56 data bytes\n64 bytes from 142.250.190.46: icmp_seq=0 ttl=116 time=1.82 ms`;
        break;
      case 'v8-eval':
        response = `V8 Bytecode compiled: ${parts.slice(1).join(' ')} => Result: undefined (Heap OK)`;
        break;
      case 'cls':
      case 'clear':
        setTerminalHistory([]);
        setTerminalInput('');
        return;
      default:
        response = `Command '${cmd}' executed. (Exit Code: 0)`;
    }

    setTerminalHistory((prev) => [...prev, `PS C:\\Users\\Admin> ${cmd}`, response]);
    setTerminalInput('');
  };

  const handleContextMenu = (e: React.MouseEvent) => {
    e.preventDefault();
    setContextMenu({ x: e.clientX, y: e.clientY, visible: true });
  };

  // 192 Sector Blocks for Disk Cloner Visual Matrix
  const sectorBlocks = 192;
  const activeSectorIndex = Math.floor((currentLba / totalLba) * sectorBlocks);

  return (
    <div 
      onContextMenu={handleContextMenu}
      onClick={() => {
        if (contextMenu.visible) setContextMenu({ ...contextMenu, visible: false });
        if (startMenuOpen) setStartMenuOpen(false);
        if (actionCenterOpen) setActionCenterOpen(false);
        if (calendarOpen) setCalendarOpen(false);
        if (widgetsOpen) setWidgetsOpen(false);
        if (powerMenuOpen) setPowerMenuOpen(false);
      }}
      className="relative w-screen h-screen overflow-hidden bg-[#060B18] select-none text-slate-100 flex flex-col justify-between font-sans"
    >
      {/* 1. Dynamic Wallpaper Background */}
      {currentWallpaper === 'bloom' && (
        <>
          <div className="absolute inset-0 bg-gradient-to-b from-[#060B18] via-[#0E192E] to-[#040810] pointer-events-none" />
          <div className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 w-[620px] h-[400px] bg-gradient-to-tr from-blue-600/30 to-cyan-400/20 rounded-full blur-[110px] pointer-events-none" />
          <div className="absolute top-1/3 right-1/4 w-[340px] h-[340px] bg-indigo-600/20 rounded-full blur-[100px] pointer-events-none" />
        </>
      )}
      {currentWallpaper === 'cyber' && (
        <div className="absolute inset-0 bg-gradient-to-tr from-[#0b031a] via-[#12082b] to-[#050f24] pointer-events-none">
          <div className="absolute top-1/2 left-1/3 w-[500px] h-[400px] bg-purple-600/25 rounded-full blur-[120px]" />
          <div className="absolute bottom-1/4 right-1/3 w-[450px] h-[350px] bg-cyan-500/20 rounded-full blur-[100px]" />
        </div>
      )}
      {currentWallpaper === 'cosmos' && (
        <div className="absolute inset-0 bg-gradient-to-b from-[#030712] via-[#0b1120] to-[#020617] pointer-events-none">
          <div className="absolute top-1/4 left-1/2 -translate-x-1/2 w-[700px] h-[350px] bg-blue-500/20 rounded-full blur-[130px]" />
        </div>
      )}
      {currentWallpaper === 'mist' && (
        <div className="absolute inset-0 bg-gradient-to-br from-[#0c1829] via-[#0a1b2d] to-[#05111d] pointer-events-none">
          <div className="absolute top-1/3 left-1/4 w-[600px] h-[400px] bg-emerald-600/15 rounded-full blur-[120px]" />
        </div>
      )}

      {/* 2. Desktop Icon Grid */}
      <div className="relative z-10 grid grid-cols-[88px_88px_1fr_88px_88px] h-[calc(100vh-48px)] p-4 gap-3">
        {/* Left Column 1: Core OS Apps */}
        <div className="flex flex-col gap-2.5">
          {[
            { id: 'pc', title: 'This PC', icon: '💻', tag: 'explorer' },
            { id: 'browser', title: 'Edge (Google)', icon: '🌐', tag: 'browser' },
            { id: 'defender', title: 'Defender Shield', icon: '🛡️', tag: 'defender' },
            { id: 'services', title: 'Services (msc)', icon: '⚙️', tag: 'services' },
            { id: 'vlc', title: 'VLC Media', icon: '🟠', tag: 'vlc' },
            { id: 'pipeline', title: 'Architecture', icon: '⚡', tag: 'pipeline' },
            { id: 'taskmgr', title: 'Task Manager', icon: '📊', tag: 'taskmgr' },
          ].map((item) => (
            <div
              key={item.id}
              onClick={(e) => {
                e.stopPropagation();
                setSelectedDesktopIcon(item.id);
                openWindow(item.tag);
              }}
              className={`flex flex-col items-center justify-center p-2 rounded-xl cursor-pointer transition-all ${
                selectedDesktopIcon === item.id ? 'bg-blue-600/40 border border-blue-400/60 shadow-lg' : 'hover:bg-slate-800/40'
              }`}
            >
              <span className="text-2xl drop-shadow-md">{item.icon}</span>
              <span className="text-[11px] text-center font-medium mt-1 drop-shadow leading-tight text-slate-100">{item.title}</span>
            </div>
          ))}
        </div>

        {/* Left Column 2: Tools & Utilities */}
        <div className="flex flex-col gap-2.5">
          {[
            { id: 'dcloner', title: 'Sector Cloner', icon: '💽', tag: 'diskclone' },
            { id: 'editor', title: 'VS Code Lite', icon: '📝', tag: 'editor' },
            { id: 'calc', title: 'Calculator', icon: '🧮', tag: 'calc' },
            { id: 'store', title: 'App Store', icon: '📦', tag: 'store' },
            { id: 'terminal', title: 'PowerShell CLI', icon: '⌨️', tag: 'terminal' },
            { id: 'bin', title: 'Recycle Bin', icon: '🗑️', tag: 'explorer' },
          ].map((item) => (
            <div
              key={item.id}
              onClick={(e) => {
                e.stopPropagation();
                setSelectedDesktopIcon(item.id);
                openWindow(item.tag);
              }}
              className={`flex flex-col items-center justify-center p-2 rounded-xl cursor-pointer transition-all ${
                selectedDesktopIcon === item.id ? 'bg-blue-600/40 border border-blue-400/60 shadow-lg' : 'hover:bg-slate-800/40'
              }`}
            >
              <span className="text-2xl drop-shadow-md">{item.icon}</span>
              <span className="text-[11px] text-center font-medium mt-1 drop-shadow leading-tight text-slate-100">{item.title}</span>
            </div>
          ))}
        </div>

        {/* Center Workspace Canvas (Window Manager Surface) */}
        <div className="relative w-full h-full pointer-events-none">
          {windows.map((win) => {
            if (win.minimized) return null;
            const isFocused = activeWindowId === win.id;

            // Geometry calculations
            let style: React.CSSProperties = {
              zIndex: win.zIndex,
            };

            if (win.maximized) {
              style = {
                ...style,
                top: 0,
                left: 0,
                width: '100%',
                height: '100%',
                borderRadius: 0,
              };
            } else if (win.snapState === 'left') {
              style = { ...style, top: 0, left: 0, width: '50%', height: '100%' };
            } else if (win.snapState === 'right') {
              style = { ...style, top: 0, left: '50%', width: '50%', height: '100%' };
            } else {
              style = {
                ...style,
                top: `${win.y}px`,
                left: `${win.x}px`,
                width: `${win.width}px`,
                height: `${win.height}px`,
              };
            }

            return (
              <div
                key={win.id}
                onClick={(e) => {
                  e.stopPropagation();
                  focusWindow(win.tag);
                }}
                style={style}
                className={`pointer-events-auto absolute mica-surface rounded-2xl flex flex-col window-enter overflow-hidden shadow-2xl transition-all ${
                  isFocused ? 'ring-1 ring-blue-500/60 shadow-blue-900/20' : 'opacity-95'
                }`}
              >
                {/* Titlebar */}
                <div 
                  className="h-10 bg-[#0F172A]/90 flex items-center justify-between px-4 border-b border-slate-700/50 cursor-move"
                  onDoubleClick={() => toggleMaximizeWindow(win.tag)}
                >
                  <div className="flex items-center gap-2">
                    <span className="text-sm">{win.icon}</span>
                    <span className="text-xs font-semibold text-slate-200 truncate max-w-md">{win.title}</span>
                  </div>

                  {/* Window Controls & Snap Helper */}
                  <div className="flex items-center gap-1.5" onClick={(e) => e.stopPropagation()}>
                    {/* Snap Buttons */}
                    <button
                      onClick={() => handleSnap(win.tag, 'left')}
                      className="px-2 py-0.5 hover:bg-slate-700/60 rounded text-[10px] text-slate-400 hover:text-white"
                      title="Snap Left"
                    >
                      ◧
                    </button>
                    <button
                      onClick={() => handleSnap(win.tag, 'right')}
                      className="px-2 py-0.5 hover:bg-slate-700/60 rounded text-[10px] text-slate-400 hover:text-white"
                      title="Snap Right"
                    >
                      ◨
                    </button>
                    {/* Minimize */}
                    <button
                      onClick={() => minimizeWindow(win.tag)}
                      className="px-2.5 py-1 hover:bg-slate-700/60 rounded text-xs text-slate-300"
                    >
                      <Minus className="w-3 h-3" />
                    </button>
                    {/* Maximize / Restore */}
                    <button
                      onClick={() => toggleMaximizeWindow(win.tag)}
                      className="px-2.5 py-1 hover:bg-slate-700/60 rounded text-xs text-slate-300"
                    >
                      {win.maximized ? <Minimize2 className="w-3 h-3" /> : <Maximize2 className="w-3 h-3" />}
                    </button>
                    {/* Close */}
                    <button
                      onClick={() => closeWindow(win.tag)}
                      className="px-2.5 py-1 hover:bg-red-600 rounded text-xs text-slate-300 hover:text-white"
                    >
                      <X className="w-3 h-3" />
                    </button>
                  </div>
                </div>

                {/* Window App Body Dispatcher */}
                <div className="flex-1 overflow-hidden">
                  {win.tag === 'defender' && <DefenderApp />}
                  {win.tag === 'browser' && <BrowserApp />}
                  {win.tag === 'services' && <ServicesApp />}
                  {win.tag === 'vlc' && <MediaPlayerApp />}
                  {win.tag === 'pipeline' && <PipelineInspectorApp />}
                  {win.tag === 'editor' && <EditorApp />}
                  {win.tag === 'calc' && <CalculatorApp />}
                  {win.tag === 'store' && <StoreApp />}

                  {/* EXPLORER APP */}
                  {win.tag === 'explorer' && (
                    <div className="flex-1 h-full p-6 flex flex-col gap-6 overflow-y-auto bg-[#0A101C]">
                      <div>
                        <h3 className="text-xs font-semibold text-slate-400 uppercase tracking-wider mb-3">Folders (7)</h3>
                        <div className="grid grid-cols-4 gap-4">
                          {['3D Objects', 'Desktop', 'Documents', 'Downloads', 'Music', 'Pictures', 'Videos'].map((f) => (
                            <div key={f} className="flex items-center gap-3 p-3 bg-slate-900/60 hover:bg-slate-800 border border-slate-800 rounded-xl cursor-pointer transition-all">
                              <Folder className="w-6 h-6 text-blue-400" />
                              <span className="text-xs font-medium text-slate-200">{f}</span>
                            </div>
                          ))}
                        </div>
                      </div>

                      <div>
                        <h3 className="text-xs font-semibold text-slate-400 uppercase tracking-wider mb-3">Devices and Drives (2)</h3>
                        <div className="grid grid-cols-2 gap-4">
                          <div className="flex items-center gap-4 p-4 bg-slate-900/60 border border-slate-800 rounded-xl">
                            <HardDrive className="w-9 h-9 text-cyan-400" />
                            <div className="flex-1">
                              <div className="text-xs font-semibold">Local Disk (C:) - FAT32 ESP</div>
                              <div className="w-full bg-slate-800 h-2 rounded-full overflow-hidden my-1.5">
                                <div className="bg-blue-500 h-full w-[42%]" />
                              </div>
                              <div className="text-[11px] text-slate-400">37.1 MB free of 64.0 MB</div>
                            </div>
                          </div>
                          <div className="flex items-center gap-4 p-4 bg-slate-900/60 border border-slate-800 rounded-xl">
                            <HardDrive className="w-9 h-9 text-purple-400" />
                            <div className="flex-1">
                              <div className="text-xs font-semibold">NVMe Primary (D:) - System Data</div>
                              <div className="w-full bg-slate-800 h-2 rounded-full overflow-hidden my-1.5">
                                <div className="bg-purple-500 h-full w-[65%]" />
                              </div>
                              <div className="text-[11px] text-slate-400">140 GB free of 400 GB</div>
                            </div>
                          </div>
                        </div>
                      </div>
                    </div>
                  )}

                  {/* SECTOR CLONER APP */}
                  {win.tag === 'diskclone' && (
                    <div className="flex-1 h-full p-6 flex flex-col gap-4 overflow-y-auto bg-[#070D18]">
                      <div className="grid grid-cols-2 gap-4">
                        <div className="p-4 bg-slate-900/70 border border-blue-500/30 rounded-xl">
                          <div className="text-xs font-bold text-cyan-400 mb-1">SOURCE DISK (PhysicalDrive0)</div>
                          <div className="text-xs font-semibold">Xenithra FAT32 ESP (NVMe/AHCI)</div>
                          <div className="text-[11px] text-slate-400">131,072 LBA Sectors (64.0 MB)</div>
                        </div>
                        <div className="p-4 bg-slate-900/70 border border-emerald-500/30 rounded-xl">
                          <div className="text-xs font-bold text-emerald-400 mb-1">TARGET DESTINATION (Bitstream)</div>
                          <div className="text-xs font-semibold">PhysicalDrive1 (backup_disk.img)</div>
                          <div className="text-[11px] text-slate-400">Direct Byte-for-Byte Hardware Mirroring</div>
                        </div>
                      </div>

                      <div className="p-4 bg-[#0A101C] border border-slate-800 rounded-xl">
                        <div className="flex justify-between items-center text-xs font-semibold mb-2">
                          <span>Processed LBA: {currentLba.toLocaleString()} / {totalLba.toLocaleString()} Sectors</span>
                          <span className="text-cyan-400">{Math.floor((currentLba / totalLba) * 100)}%</span>
                        </div>
                        <div className="w-full bg-slate-800 h-3 rounded-full overflow-hidden">
                          <div 
                            className={`h-full transition-all duration-100 ${cloneState === 'complete' ? 'bg-emerald-500' : 'bg-blue-500'}`}
                            style={{ width: `${(currentLba / totalLba) * 100}%` }}
                          />
                        </div>
                        <div className="flex justify-between items-center text-[11px] text-slate-400 mt-2">
                          <span>Copied: {((currentLba * 512) / 1048576).toFixed(1)} MB / 64 MB | Speed: 148.5 MB/s</span>
                          <span>Status: {cloneState === 'cloning' ? 'Cloning Sectors...' : cloneState === 'complete' ? 'Complete Clean' : 'Ready'}</span>
                        </div>
                      </div>

                      {/* 192 Sector Grid */}
                      <div className="p-4 bg-[#050914] border border-slate-800 rounded-xl">
                        <div className="text-[10px] font-bold text-slate-400 uppercase mb-2">Physical Sector Map (192 Block Chunks)</div>
                        <div className="grid grid-cols-32 gap-1 h-20 overflow-hidden">
                          {Array.from({ length: sectorBlocks }).map((_, idx) => (
                            <div 
                              key={idx}
                              className={`h-3 rounded-xs ${
                                idx < activeSectorIndex 
                                  ? 'bg-emerald-500' 
                                  : idx === activeSectorIndex && cloneState === 'cloning'
                                  ? 'bg-cyan-400 animate-pulse'
                                  : 'bg-slate-800'
                              }`}
                            />
                          ))}
                        </div>
                      </div>

                      <div className="flex items-center gap-3 mt-auto">
                        <button 
                          onClick={() => {
                            if (cloneState === 'idle' || cloneState === 'complete') {
                              setCurrentLba(0);
                              setElapsedSec(0);
                              setCloneState('cloning');
                            } else if (cloneState === 'paused') {
                              setCloneState('cloning');
                            }
                          }}
                          className="px-5 py-2 bg-blue-600 hover:bg-blue-500 font-semibold rounded-xl text-xs flex items-center gap-2 shadow"
                        >
                          <Play className="w-3.5 h-3.5" />
                          {cloneState === 'paused' ? 'Resume Clone' : 'Start Clone'}
                        </button>
                        <button 
                          onClick={() => setCloneState('paused')}
                          disabled={cloneState !== 'cloning'}
                          className="px-4 py-2 bg-slate-800 hover:bg-slate-700 disabled:opacity-50 font-semibold rounded-xl text-xs flex items-center gap-2"
                        >
                          <Pause className="w-3.5 h-3.5" /> Pause
                        </button>
                        <button 
                          onClick={() => { setCloneState('idle'); setCurrentLba(0); }}
                          className="px-4 py-2 bg-slate-800 hover:bg-slate-700 font-semibold rounded-xl text-xs flex items-center gap-2"
                        >
                          <RotateCcw className="w-3.5 h-3.5" /> Reset
                        </button>
                      </div>
                    </div>
                  )}

                  {/* TASK MANAGER APP */}
                  {win.tag === 'taskmgr' && (
                    <div className="flex-1 h-full p-6 flex flex-col gap-4 overflow-y-auto bg-[#070D18]">
                      <div className="grid grid-cols-4 gap-3">
                        <div className="p-3 bg-blue-600/20 border border-blue-500 rounded-xl">
                          <div className="text-xs text-cyan-400 font-bold">CPU Utilization</div>
                          <div className="text-lg font-bold mt-1">{cpuUsage}%</div>
                          <div className="text-[10px] text-slate-400">3.60 GHz | 16 JIT Threads</div>
                        </div>
                        <div className="p-3 bg-slate-900/60 border border-slate-800 rounded-xl">
                          <div className="text-xs text-slate-300 font-bold">Memory</div>
                          <div className="text-lg font-bold mt-1 text-cyan-300">5.07 GB</div>
                          <div className="text-[10px] text-slate-400">of 8.00 GB (63% In Use)</div>
                        </div>
                        <div className="p-3 bg-slate-900/60 border border-slate-800 rounded-xl">
                          <div className="text-xs text-slate-300 font-bold">Firewall Filter</div>
                          <div className="text-lg font-bold mt-1 text-emerald-400">Enforcing</div>
                          <div className="text-[10px] text-slate-400">Deep Packet Inspection</div>
                        </div>
                        <div className="p-3 bg-slate-900/60 border border-slate-800 rounded-xl">
                          <div className="text-xs text-slate-300 font-bold">V8 Runtime</div>
                          <div className="text-lg font-bold mt-1 text-purple-400">Active (JIT)</div>
                          <div className="text-[10px] text-slate-400">React Reconciler Shell</div>
                        </div>
                      </div>

                      {/* Waveform */}
                      <div className="p-4 bg-[#050914] border border-blue-500/30 rounded-xl">
                        <div className="text-xs font-semibold text-slate-300 mb-2">CPU Utilization Waveform (60-Second Telemetry)</div>
                        <div className="flex items-end gap-1.5 h-20 pt-2">
                          {cpuWaveform.map((val, i) => (
                            <div 
                              key={i} 
                              className="flex-1 bg-gradient-to-t from-blue-600 to-cyan-400 rounded-t-xs transition-all duration-300"
                              style={{ height: `${val}%` }}
                            />
                          ))}
                        </div>
                      </div>

                      {/* Process List */}
                      <div className="border border-slate-800 rounded-xl overflow-hidden text-xs">
                        <div className="grid grid-cols-[220px_80px_100px_100px_1fr] bg-[#0F172A] p-2.5 font-bold text-slate-400">
                          <span>Process Name</span>
                          <span>PID</span>
                          <span>CPU</span>
                          <span>Memory</span>
                          <span>Engine</span>
                        </div>
                        {[
                          { name: 'React 19 Shell (V8 JIT)', pid: 10, cpu: '28 %', mem: '1,480 MB', cap: 'REACT 19 | ELECTRON | V8' },
                          { name: 'WinDefend (Security Center)', pid: 104, cpu: '3 %', mem: '312 MB', cap: 'RING 0 | SMEP_SMAP' },
                          { name: 'Dnscache (DNS Resolver)', pid: 112, cpu: '1 %', mem: '48 MB', cap: 'NETFILTER | UDP:53' },
                          { name: 'MpsSvc (Firewall Filter)', pid: 108, cpu: '2 %', mem: '94 MB', cap: 'STATEFUL_FILTER' },
                          { name: 'VLC Media Synthesizer', pid: 140, cpu: '6 %', mem: '180 MB', cap: 'AUDIO_DSP' },
                        ].map((p) => (
                          <div key={p.pid} className="grid grid-cols-[220px_80px_100px_100px_1fr] p-2.5 border-t border-slate-800 bg-[#090E1A] hover:bg-slate-800/60">
                            <span className="font-semibold text-slate-200">{p.name}</span>
                            <span className="text-slate-400 font-mono">{p.pid}</span>
                            <span className="text-cyan-400 font-mono">{p.cpu}</span>
                            <span className="text-slate-300 font-mono">{p.mem}</span>
                            <span className="text-slate-400 text-[10px]">{p.cap}</span>
                          </div>
                        ))}
                      </div>
                    </div>
                  )}

                  {/* TERMINAL APP */}
                  {win.tag === 'terminal' && (
                    <div className="flex-1 h-full p-4 flex flex-col justify-between font-mono text-xs bg-[#050A14] select-text">
                      <div className="space-y-1 overflow-y-auto max-h-[340px]">
                        {terminalHistory.map((line, i) => (
                          <div key={i} className="text-slate-300 leading-relaxed whitespace-pre-wrap">
                            {line.startsWith('PS') ? <span className="text-cyan-400 font-bold">{line}</span> : line}
                          </div>
                        ))}
                      </div>
                      <form onSubmit={handleTerminalSubmit} className="flex items-center gap-2 pt-2 border-t border-slate-800">
                        <span className="text-cyan-400 font-bold text-xs">PS C:\Users\Admin&gt;</span>
                        <input
                          type="text"
                          value={terminalInput}
                          onChange={(e) => setTerminalInput(e.target.value)}
                          className="flex-1 bg-transparent text-white outline-none font-mono text-xs"
                          placeholder="Type command (e.g. help, sysinfo, ping www.google.com)..."
                          autoFocus
                        />
                      </form>
                    </div>
                  )}
                </div>
              </div>
            );
          })}
        </div>

        {/* Right Column 1: Files */}
        <div className="flex flex-col gap-2.5">
          {[
            { id: 'rec', title: 'recovery_ac...', icon: '📄', tag: 'editor' },
            { id: 'main', title: 'main.c', icon: '💻', tag: 'editor' },
            { id: 'syscfg', title: 'sysconfig...', icon: '⚙️', tag: 'editor' },
            { id: 'notes', title: 'notes.txt', icon: '📄', tag: 'editor' },
          ].map((item) => (
            <div
              key={item.id}
              onClick={(e) => {
                e.stopPropagation();
                setSelectedDesktopIcon(item.id);
                openWindow(item.tag);
              }}
              className={`flex flex-col items-center justify-center p-2 rounded-xl cursor-pointer transition-all ${
                selectedDesktopIcon === item.id ? 'bg-blue-600/40 border border-blue-400/60 shadow-lg' : 'hover:bg-slate-800/40'
              }`}
            >
              <span className="text-2xl drop-shadow-md">{item.icon}</span>
              <span className="text-[11px] text-center font-medium mt-1 drop-shadow leading-tight text-slate-100">{item.title}</span>
            </div>
          ))}
        </div>

        {/* Right Column 2: Network & Shortcuts */}
        <div className="flex flex-col gap-2.5">
          {[
            { id: 'google', title: 'Google Web', icon: '🔍', tag: 'browser' },
            { id: 'docs', title: 'OS Docs', icon: '📚', tag: 'browser' },
            { id: 'music', title: 'Audio Synth', icon: '🎵', tag: 'vlc' },
            { id: 'restart', title: 'Restart Svc', icon: '🔄', tag: 'services' },
          ].map((item) => (
            <div
              key={item.id}
              onClick={(e) => {
                e.stopPropagation();
                setSelectedDesktopIcon(item.id);
                openWindow(item.tag);
              }}
              className={`flex flex-col items-center justify-center p-2 rounded-xl cursor-pointer transition-all ${
                selectedDesktopIcon === item.id ? 'bg-blue-600/40 border border-blue-400/60 shadow-lg' : 'hover:bg-slate-800/40'
              }`}
            >
              <span className="text-2xl drop-shadow-md">{item.icon}</span>
              <span className="text-[11px] text-center font-medium mt-1 drop-shadow leading-tight text-slate-100">{item.title}</span>
            </div>
          ))}
        </div>
      </div>

      {/* 3. Windows 11 Acrylic Start Menu */}
      {startMenuOpen && (
        <div 
          onClick={(e) => e.stopPropagation()}
          className="absolute bottom-14 left-1/2 -translate-x-1/2 w-[540px] h-[560px] mica-surface rounded-2xl z-50 flex flex-col window-enter overflow-hidden p-6 border border-slate-700 shadow-2xl"
        >
          {/* Search Field */}
          <div className="flex items-center gap-3 bg-[#0B1120] px-4 py-2.5 rounded-full border border-slate-700 text-xs text-slate-300 mb-6 shadow-inner">
            <Search className="w-4 h-4 text-slate-400" />
            <input
              type="text"
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              placeholder="Search apps, settings, and documents..."
              className="w-full bg-transparent outline-none text-xs text-white placeholder-slate-500"
              autoFocus
            />
          </div>

          <div className="text-xs font-bold text-slate-300 mb-3 flex justify-between items-center">
            <span>Pinned Applications</span>
            <span className="text-[10px] text-cyan-400 font-normal">All Apps ›</span>
          </div>

          {/* Pinned Apps Grid */}
          <div className="grid grid-cols-4 gap-3 mb-6">
            {[
              { title: 'Google Browser', icon: '🌐', tag: 'browser' },
              { title: 'Defender Shield', icon: '🛡️', tag: 'defender' },
              { title: 'Services (msc)', icon: '⚙️', tag: 'services' },
              { title: 'VLC Player', icon: '🟠', tag: 'vlc' },
              { title: 'Architecture', icon: '⚡', tag: 'pipeline' },
              { title: 'File Explorer', icon: '📁', tag: 'explorer' },
              { title: 'VS Code Lite', icon: '💻', tag: 'editor' },
              { title: 'Calculator', icon: '🧮', tag: 'calc' },
              { title: 'App Store', icon: '📦', tag: 'store' },
              { title: 'Terminal CLI', icon: '⌨️', tag: 'terminal' },
              { title: 'Sector Cloner', icon: '💽', tag: 'diskclone' },
              { title: 'Task Manager', icon: '📊', tag: 'taskmgr' },
            ]
              .filter((a) => !searchQuery || a.title.toLowerCase().includes(searchQuery.toLowerCase()))
              .map((app) => (
                <div 
                  key={app.title}
                  onClick={() => { openWindow(app.tag); setStartMenuOpen(false); }}
                  className="flex flex-col items-center gap-1.5 p-2.5 rounded-xl hover:bg-slate-800/70 cursor-pointer transition-all"
                >
                  <span className="text-3xl drop-shadow">{app.icon}</span>
                  <span className="text-xs font-medium text-center truncate w-full">{app.title}</span>
                </div>
              ))}
          </div>

          <div className="text-xs font-bold text-slate-300 mb-2">Recommended</div>
          <div className="space-y-1.5 flex-1 overflow-y-auto">
            {[
              { title: 'www.google.com - Web Browser', icon: '🌐', tag: 'browser' },
              { title: 'Kernel Security Audit Log - Defender', icon: '🛡️', tag: 'defender' },
              { title: 'main.c - Higher-Half Entry Point', icon: '💻', tag: 'editor' },
            ].map((rec) => (
              <div 
                key={rec.title} 
                onClick={() => { openWindow(rec.tag); setStartMenuOpen(false); }}
                className="flex items-center gap-3 p-2 bg-slate-900/50 rounded-xl text-xs hover:bg-slate-800 cursor-pointer transition-all"
              >
                <span>{rec.icon}</span>
                <span className="text-slate-300 truncate">{rec.title}</span>
              </div>
            ))}
          </div>

          {/* User Account & Power Bar */}
          <div className="mt-auto pt-4 border-t border-slate-800 flex items-center justify-between">
            <div className="flex items-center gap-3">
              <div className="w-8 h-8 rounded-full bg-gradient-to-tr from-blue-600 to-cyan-500 flex items-center justify-center font-bold text-xs text-white shadow-md">
                A
              </div>
              <div>
                <div className="text-xs font-semibold text-white">Administrator</div>
                <div className="text-[10px] text-emerald-400">Ring 0 Full Privilege</div>
              </div>
            </div>

            <div className="relative">
              <button 
                onClick={() => setPowerMenuOpen(!powerMenuOpen)}
                className="p-2 hover:bg-slate-800 rounded-lg text-red-400 transition-colors"
                title="Power Options"
              >
                <Power className="w-4 h-4" />
              </button>

              {powerMenuOpen && (
                <div className="absolute bottom-10 right-0 w-44 bg-[#141C2E] border border-slate-700 rounded-xl shadow-2xl p-1 text-xs z-50">
                  <div 
                    onClick={() => { alert('System entered low power sleep state.'); setPowerMenuOpen(false); }}
                    className="flex items-center gap-2 p-2 hover:bg-slate-800 rounded-lg cursor-pointer"
                  >
                    <Moon className="w-3.5 h-3.5 text-cyan-400" />
                    <span>Sleep (ACPI S3)</span>
                  </div>
                  <div 
                    onClick={() => { alert('Xenithra OS Shutdown requested.'); setPowerMenuOpen(false); }}
                    className="flex items-center gap-2 p-2 hover:bg-slate-800 rounded-lg cursor-pointer text-red-400"
                  >
                    <Power className="w-3.5 h-3.5" />
                    <span>Shut down</span>
                  </div>
                  <div 
                    onClick={() => { alert('Xenithra OS Rebooting...'); setPowerMenuOpen(false); }}
                    className="flex items-center gap-2 p-2 hover:bg-slate-800 rounded-lg cursor-pointer text-amber-400"
                  >
                    <RefreshCw className="w-3.5 h-3.5" />
                    <span>Restart</span>
                  </div>
                </div>
              )}
            </div>
          </div>
        </div>
      )}

      {/* 4. Windows 11 Action Center & Quick Settings Flyout */}
      {actionCenterOpen && (
        <div 
          onClick={(e) => e.stopPropagation()}
          className="absolute bottom-14 right-4 w-84 mica-surface rounded-2xl z-50 p-5 border border-slate-700 shadow-2xl space-y-4 window-enter"
        >
          <div className="grid grid-cols-3 gap-2">
            {[
              { title: 'Wi-Fi', active: wifiEnabled, toggle: () => setWifiEnabled(!wifiEnabled), icon: Wifi },
              { title: 'Bluetooth', active: bluetoothEnabled, toggle: () => setBluetoothEnabled(!bluetoothEnabled), icon: Bluetooth },
              { title: 'Airplane Mode', active: airplaneMode, toggle: () => setAirplaneMode(!airplaneMode), icon: Radio },
              { title: 'Night Light', active: nightLight, toggle: () => setNightLight(!nightLight), icon: Moon },
              { title: 'Defender Guard', active: true, toggle: () => openWindow('defender'), icon: ShieldCheck },
              { title: 'Services', active: true, toggle: () => openWindow('services'), icon: Settings },
            ].map((qs, i) => {
              const Icon = qs.icon;
              return (
                <button
                  key={i}
                  onClick={qs.toggle}
                  className={`p-3 rounded-xl flex flex-col items-center gap-1.5 transition-all ${
                    qs.active ? 'bg-blue-600 text-white shadow-md shadow-blue-600/30' : 'bg-slate-800/60 text-slate-400 hover:bg-slate-800'
                  }`}
                >
                  <Icon className="w-4 h-4" />
                  <span className="text-[10px] font-semibold">{qs.title}</span>
                </button>
              );
            })}
          </div>

          {/* Sliders */}
          <div className="space-y-3 pt-2 border-t border-slate-800">
            <div className="flex items-center gap-3 text-xs">
              <Sun className="w-4 h-4 text-amber-400" />
              <input
                type="range"
                min="10"
                max="100"
                value={brightnessLevel}
                onChange={(e) => setBrightnessLevel(Number(e.target.value))}
                className="w-full h-1.5 bg-slate-800 rounded-lg appearance-none cursor-pointer accent-blue-500"
              />
              <span className="text-[10px] font-mono w-7">{brightnessLevel}%</span>
            </div>

            <div className="flex items-center gap-3 text-xs">
              <Volume2 className="w-4 h-4 text-cyan-400" />
              <input
                type="range"
                min="0"
                max="100"
                value={volumeLevel}
                onChange={(e) => setVolumeLevel(Number(e.target.value))}
                className="w-full h-1.5 bg-slate-800 rounded-lg appearance-none cursor-pointer accent-cyan-400"
              />
              <span className="text-[10px] font-mono w-7">{volumeLevel}%</span>
            </div>
          </div>

          <div className="flex justify-between items-center text-[11px] text-slate-400 pt-1">
            <span className="flex items-center gap-1.5">
              <Battery className="w-4 h-4 text-emerald-400" /> 100% Fully Charged
            </span>
            <span className="text-cyan-400 font-semibold cursor-pointer" onClick={() => openWindow('pipeline')}>
              Pipeline Active
            </span>
          </div>
        </div>
      )}

      {/* 5. Calendar & Date Widget Flyout */}
      {calendarOpen && (
        <div 
          onClick={(e) => e.stopPropagation()}
          className="absolute bottom-14 right-2 w-76 mica-surface rounded-2xl z-50 p-4 border border-slate-700 shadow-2xl space-y-3 window-enter"
        >
          <div className="flex justify-between items-center border-b border-slate-800 pb-2">
            <span className="text-sm font-bold text-white">{dateStr}</span>
            <span className="text-xs font-mono text-cyan-400">{timeStr}</span>
          </div>
          <div className="grid grid-cols-7 gap-1 text-center text-xs text-slate-400 py-1">
            {['Su', 'Mo', 'Tu', 'We', 'Th', 'Fr', 'Sa'].map((d) => (
              <span key={d} className="font-bold">{d}</span>
            ))}
            {Array.from({ length: 30 }).map((_, i) => (
              <span 
                key={i} 
                className={`p-1.5 rounded-lg text-xs cursor-pointer ${
                  i + 1 === 13 ? 'bg-blue-600 text-white font-bold' : 'hover:bg-slate-800 text-slate-200'
                }`}
              >
                {i + 1}
              </span>
            ))}
          </div>
        </div>
      )}

      {/* 6. Desktop Right-Click Context Menu */}
      {contextMenu.visible && (
        <div
          onClick={(e) => e.stopPropagation()}
          style={{ top: `${contextMenu.y}px`, left: `${contextMenu.x}px` }}
          className="absolute w-52 bg-[#0E1626] border border-slate-700/80 rounded-xl shadow-2xl p-1.5 text-xs text-slate-200 z-50 window-enter"
        >
          <div 
            onClick={() => { openWindow('explorer'); setContextMenu({ ...contextMenu, visible: false }); }}
            className="flex items-center gap-2 p-2 hover:bg-blue-600 hover:text-white rounded-lg cursor-pointer"
          >
            <Folder className="w-4 h-4 text-blue-400" />
            <span>Open File Explorer</span>
          </div>
          <div 
            onClick={() => { openWindow('terminal'); setContextMenu({ ...contextMenu, visible: false }); }}
            className="flex items-center gap-2 p-2 hover:bg-blue-600 hover:text-white rounded-lg cursor-pointer"
          >
            <Terminal className="w-4 h-4 text-cyan-400" />
            <span>Open in Terminal</span>
          </div>
          <div 
            onClick={() => { openWindow('browser'); setContextMenu({ ...contextMenu, visible: false }); }}
            className="flex items-center gap-2 p-2 hover:bg-blue-600 hover:text-white rounded-lg cursor-pointer"
          >
            <Globe className="w-4 h-4 text-emerald-400" />
            <span>Open Web Browser (Google)</span>
          </div>
          <div className="h-px bg-slate-800 my-1" />
          <div className="px-2 py-1 text-[10px] text-slate-400 font-bold uppercase">Change Wallpaper Theme</div>
          <div className="grid grid-cols-2 gap-1 p-1">
            <button 
              onClick={() => { setCurrentWallpaper('bloom'); setContextMenu({ ...contextMenu, visible: false }); }}
              className="p-1.5 bg-slate-800 hover:bg-blue-600 rounded text-[11px] font-medium"
            >
              Bloom Dark
            </button>
            <button 
              onClick={() => { setCurrentWallpaper('cyber'); setContextMenu({ ...contextMenu, visible: false }); }}
              className="p-1.5 bg-slate-800 hover:bg-purple-600 rounded text-[11px] font-medium"
            >
              Cyber Neon
            </button>
            <button 
              onClick={() => { setCurrentWallpaper('cosmos'); setContextMenu({ ...contextMenu, visible: false }); }}
              className="p-1.5 bg-slate-800 hover:bg-blue-600 rounded text-[11px] font-medium"
            >
              Deep Cosmos
            </button>
            <button 
              onClick={() => { setCurrentWallpaper('mist'); setContextMenu({ ...contextMenu, visible: false }); }}
              className="p-1.5 bg-slate-800 hover:bg-emerald-600 rounded text-[11px] font-medium"
            >
              Emerald Mist
            </button>
          </div>
        </div>
      )}

      {/* 7. Windows 11 Acrylic Taskbar Dock */}
      <div className="relative z-40 h-12 acrylic-dock flex items-center justify-between px-3">
        {/* Left / Center Taskbar Icons */}
        <div className="flex items-center gap-1.5 mx-auto">
          {/* Start Menu Button */}
          <button 
            onClick={(e) => { e.stopPropagation(); setStartMenuOpen(!startMenuOpen); }}
            className={`p-2 rounded-xl taskbar-icon-hover ${startMenuOpen ? 'bg-blue-600/40 shadow-inner' : ''}`}
            title="Start"
          >
            <div className="grid grid-cols-2 gap-0.5 w-4 h-4">
              <div className="bg-cyan-400 rounded-xs" />
              <div className="bg-cyan-400 rounded-xs" />
              <div className="bg-cyan-400 rounded-xs" />
              <div className="bg-cyan-400 rounded-xs" />
            </div>
          </button>

          {/* Dock App Launchers */}
          {[
            { id: 'browser', icon: '🌐', tag: 'browser', title: 'Edge Browser (Google)' },
            { id: 'defender', icon: '🛡️', tag: 'defender', title: 'Defender Security Center' },
            { id: 'services', icon: '⚙️', tag: 'services', title: 'Services Console' },
            { id: 'vlc', icon: '🟠', tag: 'vlc', title: 'VLC Media Player' },
            { id: 'pipeline', icon: '⚡', tag: 'pipeline', title: 'Architecture Pipeline' },
            { id: 'explorer', icon: '📁', tag: 'explorer', title: 'File Explorer' },
            { id: 'editor', icon: '💻', tag: 'editor', title: 'VS Code Lite' },
            { id: 'taskmgr', icon: '📊', tag: 'taskmgr', title: 'Task Manager' },
            { id: 'diskclone', icon: '💽', tag: 'diskclone', title: 'Sector Cloner' },
            { id: 'calc', icon: '🧮', tag: 'calc', title: 'Calculator' },
            { id: 'store', icon: '📦', tag: 'store', title: 'Microsoft Store' },
            { id: 'terminal', icon: '⌨️', tag: 'terminal', title: 'PowerShell' },
          ].map((item) => {
            const isOpen = windows.some((w) => w.tag === item.tag);
            const isFocused = activeWindowId === item.tag;
            return (
              <button
                key={item.id}
                onClick={(e) => {
                  e.stopPropagation();
                  openWindow(item.tag);
                }}
                title={item.title}
                className={`p-2 rounded-xl taskbar-icon-hover relative flex flex-col items-center ${
                  isFocused ? 'bg-slate-800/90 shadow' : isOpen ? 'bg-slate-800/40' : ''
                }`}
              >
                <span className="text-lg drop-shadow">{item.icon}</span>
                {isOpen && (
                  <div className={`absolute bottom-0.5 w-3 h-0.5 rounded-full ${isFocused ? 'bg-cyan-400' : 'bg-slate-400'}`} />
                )}
              </button>
            );
          })}
        </div>

        {/* Right System Tray */}
        <div className="flex items-center gap-2 text-xs text-slate-300">
          <div 
            onClick={(e) => { e.stopPropagation(); setActionCenterOpen(!actionCenterOpen); }}
            className="flex items-center gap-2 px-2 py-1.5 hover:bg-slate-800/80 rounded-xl cursor-pointer transition-colors"
          >
            <Wifi className={`w-3.5 h-3.5 ${wifiEnabled ? 'text-cyan-400' : 'text-slate-500'}`} />
            <Volume2 className="w-3.5 h-3.5" />
            <Battery className="w-3.5 h-3.5 text-emerald-400" />
          </div>

          <div className="px-2 py-1 hover:bg-slate-800/80 rounded-lg cursor-pointer font-semibold text-[11px]">
            ENG
          </div>

          <div 
            onClick={(e) => { e.stopPropagation(); setCalendarOpen(!calendarOpen); }}
            className="flex flex-col items-end leading-none px-2 py-1 hover:bg-slate-800/80 rounded-xl cursor-pointer transition-colors"
          >
            <span className="font-semibold text-[11px] text-slate-100">{timeStr}</span>
            <span className="text-[9px] text-slate-400 mt-0.5">{dateStr}</span>
          </div>
        </div>
      </div>
    </div>
  );
};
