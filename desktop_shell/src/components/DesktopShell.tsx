import React, { useState, useEffect } from 'react';
import { 
  Folder, HardDrive, Cpu, Terminal, Play, Pause, RotateCcw, 
  Power, ShieldCheck, Activity, RefreshCw, Moon, Disc, 
  ExternalLink, Layers, Search, Volume2, Wifi, Calendar, CheckCircle2
} from 'lucide-react';

export const DesktopShell: React.FC = () => {
  // Window State Management
  const [activeWindows, setActiveWindows] = useState<string[]>(['explorer']);
  const [focusedWindow, setFocusedWindow] = useState<string>('explorer');
  const [startMenuOpen, setStartMenuOpen] = useState(false);
  const [powerMenuOpen, setPowerMenuOpen] = useState(false);
  const [selectedIcon, setSelectedIcon] = useState<string | null>(null);

  // Sector Cloner Telemetry
  const [cloneState, setCloneState] = useState<'idle' | 'cloning' | 'paused' | 'complete'>('idle');
  const [currentLba, setCurrentLba] = useState(0);
  const totalLba = 131072; // 64 MB
  const [elapsedSec, setElapsedSec] = useState(0);

  // Task Manager Telemetry
  const [cpuUsage, setCpuUsage] = useState(58);
  const [cpuWaveform, setCpuWaveform] = useState<number[]>([45, 52, 60, 58, 64, 55, 62, 70, 65, 58, 54, 59, 63, 68, 58, 55, 62, 67, 59, 56, 61, 64, 58, 60]);

  // Current Time
  const [timeStr, setTimeStr] = useState('12:19 PM');
  const [dateStr, setDateStr] = useState('2026-09-13');

  useEffect(() => {
    const timer = setInterval(() => {
      const now = new Date();
      setTimeStr(now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }));
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
      const nextVal = Math.floor(52 + Math.random() * 20);
      setCpuUsage(nextVal);
      setCpuWaveform((prev) => [...prev.slice(1), nextVal]);
    }, 1000);
    return () => clearInterval(jitter);
  }, []);

  const toggleWindow = (appTag: string) => {
    if (activeWindows.includes(appTag)) {
      if (focusedWindow === appTag) {
        setActiveWindows((prev) => prev.filter((w) => w !== appTag));
      } else {
        setFocusedWindow(appTag);
      }
    } else {
      setActiveWindows((prev) => [...prev, appTag]);
      setFocusedWindow(appTag);
    }
  };

  const closeWindow = (appTag: string) => {
    setActiveWindows((prev) => prev.filter((w) => w !== appTag));
  };

  // 192 Sector Blocks for Disk Cloner Visual Matrix
  const sectorBlocks = 192;
  const activeSectorIndex = Math.floor((currentLba / totalLba) * sectorBlocks);

  return (
    <div className="relative w-screen h-screen overflow-hidden bg-[#060B18] select-none text-slate-100 flex flex-col justify-between">
      {/* 1. Deep Midnight Navy Windows 11 Bloom Wallpaper */}
      <div className="absolute inset-0 bg-gradient-to-b from-[#060B18] via-[#0E192E] to-[#040810] pointer-events-none" />
      <div className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 w-[520px] h-[360px] bg-gradient-to-tr from-blue-600/30 to-cyan-400/20 rounded-full blur-[100px] pointer-events-none" />

      {/* 2. Desktop Icon Shortcuts Grid */}
      <div className="relative z-10 grid grid-cols-[84px_84px_1fr_84px_84px] h-[calc(100vh-48px)] p-4 gap-4">
        {/* Left Column 1 */}
        <div className="flex flex-col gap-3">
          {[
            { id: 'pc', title: 'This PC', icon: '💻', tag: 'explorer' },
            { id: 'bin', title: 'Recycle Bin', icon: '🗑️', tag: 'explorer' },
            { id: 'edge', title: 'Personal - Edge', icon: '🌐', tag: 'terminal' },
            { id: 'chrome', title: 'Google Chrome', icon: '🔴', tag: 'terminal' },
            { id: 'itunes', title: 'iTunes', icon: '🎵', tag: 'vlc' },
            { id: 'agy', title: 'Antigravity IDE', icon: '🚀', tag: 'terminal' },
            { id: 'vlc', title: 'VLC player', icon: '🟠', tag: 'vlc' },
            { id: 'arduino', title: 'Arduino IDE', icon: '♾️', tag: 'terminal' },
          ].map((item) => (
            <div
              key={item.id}
              onClick={() => { setSelectedIcon(item.id); toggleWindow(item.tag); }}
              className={`flex flex-col items-center justify-center p-2 rounded-lg cursor-pointer transition-all ${
                selectedIcon === item.id ? 'bg-blue-600/30 border border-blue-400/50' : 'hover:bg-slate-800/40'
              }`}
            >
              <span className="text-2xl drop-shadow-md">{item.icon}</span>
              <span className="text-[11px] text-center font-medium mt-1 drop-shadow leading-tight">{item.title}</span>
            </div>
          ))}
        </div>

        {/* Left Column 2 */}
        <div className="flex flex-col gap-3">
          {[
            { id: 'modbus1', title: 'Modbus Poll', icon: '📊', tag: 'installer' },
            { id: 'postman', title: 'Postman', icon: '📮', tag: 'installer' },
            { id: 'modbus2', title: 'Modbus Slave', icon: '🔌', tag: 'installer' },
            { id: 'localsend', title: 'LocalSend', icon: '📲', tag: 'installer' },
            { id: 'fdm', title: 'Free Downlo...', icon: '⬇️', tag: 'installer' },
            { id: 'vbox', title: 'VirtualBox', icon: '📦', tag: 'installer' },
            { id: 'dcloner', title: 'Sector Cloner', icon: '💽', tag: 'diskclone' },
          ].map((item) => (
            <div
              key={item.id}
              onClick={() => { setSelectedIcon(item.id); toggleWindow(item.tag); }}
              className={`flex flex-col items-center justify-center p-2 rounded-lg cursor-pointer transition-all ${
                selectedIcon === item.id ? 'bg-blue-600/30 border border-blue-400/50' : 'hover:bg-slate-800/40'
              }`}
            >
              <span className="text-2xl drop-shadow-md">{item.icon}</span>
              <span className="text-[11px] text-center font-medium mt-1 drop-shadow leading-tight">{item.title}</span>
            </div>
          ))}
        </div>

        {/* Center Canvas Area (Windows Rendering Workspace) */}
        <div className="relative w-full h-full pointer-events-none">
          {/* Active Windows Stack */}
          
          {/* WINDOW 1: File Explorer */}
          {activeWindows.includes('explorer') && (
            <div 
              onClick={() => setFocusedWindow('explorer')}
              className={`pointer-events-auto absolute top-6 left-12 w-[740px] h-[480px] mica-surface rounded-xl flex flex-col window-enter overflow-hidden ${
                focusedWindow === 'explorer' ? 'ring-1 ring-blue-500/50 z-30' : 'z-20 opacity-90'
              }`}
            >
              {/* Explorer Titlebar */}
              <div className="h-10 bg-[#141D2E]/90 flex items-center justify-between px-4 border-b border-slate-700/50">
                <div className="flex items-center gap-2">
                  <Folder className="w-4 h-4 text-blue-400" />
                  <span className="text-xs font-semibold text-slate-200">This PC - Xenithra Explorer</span>
                </div>
                <div className="flex items-center gap-2">
                  <button onClick={() => toggleWindow('explorer')} className="px-3 py-1 hover:bg-slate-700/50 rounded text-xs">-</button>
                  <button className="px-3 py-1 hover:bg-slate-700/50 rounded text-xs">□</button>
                  <button onClick={() => closeWindow('explorer')} className="px-3 py-1 hover:bg-red-600 rounded text-xs">✕</button>
                </div>
              </div>

              {/* Explorer Ribbon & Navigation */}
              <div className="bg-[#101827] px-4 py-2 border-b border-slate-800 flex items-center justify-between">
                <div className="flex items-center gap-4 text-xs font-medium text-slate-300">
                  <span className="text-slate-400 cursor-pointer">File</span>
                  <span className="text-blue-400 border-b-2 border-blue-500 pb-0.5 cursor-pointer">Computer</span>
                  <span className="text-slate-400 cursor-pointer">View</span>
                </div>
                <div className="flex items-center gap-2 bg-[#0B1120] px-3 py-1 rounded-md border border-slate-700 text-xs w-72 text-slate-400">
                  <Search className="w-3.5 h-3.5" />
                  <span>Search This PC</span>
                </div>
              </div>

              {/* Explorer Content Body */}
              <div className="flex-1 p-6 flex flex-col gap-6 overflow-y-auto">
                <div>
                  <h3 className="text-xs font-semibold text-slate-400 uppercase tracking-wider mb-3">Folders (7)</h3>
                  <div className="grid grid-cols-4 gap-4">
                    {['3D Objects', 'Desktop', 'Documents', 'Downloads', 'Music', 'Pictures', 'Videos'].map((f) => (
                      <div key={f} className="flex items-center gap-3 p-3 bg-slate-800/40 hover:bg-slate-800/80 border border-slate-700/40 rounded-lg cursor-pointer transition-all">
                        <Folder className="w-6 h-6 text-blue-400" />
                        <span className="text-xs font-medium">{f}</span>
                      </div>
                    ))}
                  </div>
                </div>

                <div>
                  <h3 className="text-xs font-semibold text-slate-400 uppercase tracking-wider mb-3">Devices and drives (1)</h3>
                  <div className="flex items-center gap-4 p-4 bg-slate-800/40 border border-slate-700/40 rounded-lg max-w-sm">
                    <HardDrive className="w-10 h-10 text-cyan-400" />
                    <div className="flex-1">
                      <div className="text-xs font-semibold">Local Disk (C:)</div>
                      <div className="w-full bg-slate-700 h-2 rounded-full overflow-hidden my-1.5">
                        <div className="bg-blue-500 h-full w-[54%]" />
                      </div>
                      <div className="text-[11px] text-slate-400">103 GB free of 222 GB</div>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          )}

          {/* WINDOW 2: Raw Sector Cloner Tool */}
          {activeWindows.includes('diskclone') && (
            <div 
              onClick={() => setFocusedWindow('diskclone')}
              className={`pointer-events-auto absolute top-10 left-20 w-[760px] h-[520px] mica-surface rounded-xl flex flex-col window-enter overflow-hidden ${
                focusedWindow === 'diskclone' ? 'ring-1 ring-emerald-500/50 z-30' : 'z-20 opacity-90'
              }`}
            >
              {/* Titlebar */}
              <div className="h-10 bg-[#141D2E]/90 flex items-center justify-between px-4 border-b border-slate-700/50">
                <div className="flex items-center gap-2">
                  <Disc className="w-4 h-4 text-emerald-400" />
                  <span className="text-xs font-semibold text-slate-200">Raw Sector-by-Sector Disk Cloner (Bitstream Imager)</span>
                </div>
                <div className="flex items-center gap-2">
                  <button onClick={() => toggleWindow('diskclone')} className="px-3 py-1 hover:bg-slate-700/50 rounded text-xs">-</button>
                  <button onClick={() => closeWindow('diskclone')} className="px-3 py-1 hover:bg-red-600 rounded text-xs">✕</button>
                </div>
              </div>

              {/* Cloner Deck */}
              <div className="flex-1 p-6 flex flex-col gap-4 overflow-y-auto">
                <div className="grid grid-cols-2 gap-4">
                  <div className="p-4 bg-slate-800/40 border border-blue-500/30 rounded-lg">
                    <div className="text-xs font-bold text-cyan-400 mb-1">SOURCE DISK (PhysicalDrive0)</div>
                    <div className="text-xs font-semibold">Xenithra FAT32 ESP (NVMe/AHCI)</div>
                    <div className="text-[11px] text-slate-400">Total: 131,072 LBA Sectors (64.0 MB)</div>
                  </div>
                  <div className="p-4 bg-slate-800/40 border border-emerald-500/30 rounded-lg">
                    <div className="text-xs font-bold text-emerald-400 mb-1">TARGET DESTINATION (Bitstream)</div>
                    <div className="text-xs font-semibold">PhysicalDrive1 (backup_disk.img)</div>
                    <div className="text-[11px] text-slate-400">Direct Byte-for-Byte Hardware Mirroring</div>
                  </div>
                </div>

                {/* Progress Card */}
                <div className="p-4 bg-[#0A101C] border border-slate-700 rounded-lg">
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

                {/* 192 Sector Allocation Matrix */}
                <div className="p-4 bg-[#080D1A] border border-slate-800 rounded-lg">
                  <div className="text-[10px] font-bold text-slate-400 tracking-wider uppercase mb-2">Physical Sector Map (192 Block Chunks)</div>
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

                {/* Action Buttons */}
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
                    className="px-5 py-2 bg-blue-600 hover:bg-blue-500 font-semibold rounded-md text-xs flex items-center gap-2"
                  >
                    <Play className="w-3.5 h-3.5" />
                    {cloneState === 'paused' ? 'Resume Clone' : 'Start Clone'}
                  </button>
                  <button 
                    onClick={() => setCloneState('paused')}
                    disabled={cloneState !== 'cloning'}
                    className="px-4 py-2 bg-slate-800 hover:bg-slate-700 disabled:opacity-50 font-semibold rounded-md text-xs flex items-center gap-2"
                  >
                    <Pause className="w-3.5 h-3.5" />
                    Pause
                  </button>
                  <button 
                    onClick={() => { setCloneState('idle'); setCurrentLba(0); }}
                    className="px-4 py-2 bg-slate-800 hover:bg-slate-700 font-semibold rounded-md text-xs flex items-center gap-2"
                  >
                    <RotateCcw className="w-3.5 h-3.5" />
                    Reset
                  </button>
                  <span className="ml-auto text-xs text-emerald-400 flex items-center gap-1.5">
                    <CheckCircle2 className="w-3.5 h-3.5" />
                    Bad Sectors: 0 [None]
                  </span>
                </div>
              </div>
            </div>
          )}

          {/* WINDOW 3: Task Manager with React/Electron V8 Telemetry */}
          {activeWindows.includes('taskmgr') && (
            <div 
              onClick={() => setFocusedWindow('taskmgr')}
              className={`pointer-events-auto absolute top-14 left-28 w-[760px] h-[480px] mica-surface rounded-xl flex flex-col window-enter overflow-hidden ${
                focusedWindow === 'taskmgr' ? 'ring-1 ring-cyan-500/50 z-30' : 'z-20 opacity-90'
              }`}
            >
              {/* Titlebar */}
              <div className="h-10 bg-[#141D2E]/90 flex items-center justify-between px-4 border-b border-slate-700/50">
                <div className="flex items-center gap-2">
                  <Activity className="w-4 h-4 text-cyan-400" />
                  <span className="text-xs font-semibold text-slate-200">Task Manager (React 19 / V8 Engine Telemetry)</span>
                </div>
                <div className="flex items-center gap-2">
                  <button onClick={() => toggleWindow('taskmgr')} className="px-3 py-1 hover:bg-slate-700/50 rounded text-xs">-</button>
                  <button onClick={() => closeWindow('taskmgr')} className="px-3 py-1 hover:bg-red-600 rounded text-xs">✕</button>
                </div>
              </div>

              {/* Task Manager Tabs */}
              <div className="p-6 flex flex-col gap-4 overflow-y-auto">
                <div className="grid grid-cols-4 gap-3">
                  <div className="p-3 bg-blue-600/20 border border-blue-500 rounded-lg">
                    <div className="text-xs text-cyan-400 font-bold">CPU Utilization</div>
                    <div className="text-lg font-bold mt-1">{cpuUsage}%</div>
                    <div className="text-[10px] text-slate-400">3.60 GHz | 16 JIT Threads</div>
                  </div>
                  <div className="p-3 bg-slate-800/40 border border-slate-700 rounded-lg">
                    <div className="text-xs text-slate-300 font-bold">Memory</div>
                    <div className="text-lg font-bold mt-1 text-cyan-300">5.07 GB</div>
                    <div className="text-[10px] text-slate-400">of 8.00 GB (63% In Use)</div>
                  </div>
                  <div className="p-3 bg-slate-800/40 border border-slate-700 rounded-lg">
                    <div className="text-xs text-slate-300 font-bold">Disk 0 (NVMe)</div>
                    <div className="text-lg font-bold mt-1 text-emerald-400">84% Active</div>
                    <div className="text-[10px] text-slate-400">148.5 MB/s Transfer</div>
                  </div>
                  <div className="p-3 bg-slate-800/40 border border-slate-700 rounded-lg">
                    <div className="text-xs text-slate-300 font-bold">V8 Runtime</div>
                    <div className="text-lg font-bold mt-1 text-purple-400">Active</div>
                    <div className="text-[10px] text-slate-400">React Reconciler Shell</div>
                  </div>
                </div>

                {/* Real-time Waveform */}
                <div className="p-4 bg-[#0A101C] border border-blue-500/40 rounded-lg">
                  <div className="text-xs font-semibold text-slate-300 mb-2">CPU & V8 JIT Waveform (60-Second Telemetry)</div>
                  <div className="flex items-end gap-1.5 h-24 pt-2">
                    {cpuWaveform.map((val, i) => (
                      <div 
                        key={i} 
                        className="flex-1 bg-gradient-to-t from-blue-600 to-cyan-400 rounded-t-xs transition-all duration-300"
                        style={{ height: `${val}%` }}
                      />
                    ))}
                  </div>
                </div>

                {/* Top Process List */}
                <div className="border border-slate-700 rounded-lg overflow-hidden text-xs">
                  <div className="grid grid-cols-[240px_80px_100px_100px_1fr] bg-[#141C2E] p-2.5 font-bold text-slate-400">
                    <span>Process Name</span>
                    <span>PID</span>
                    <span>CPU</span>
                    <span>Memory</span>
                    <span>Subsystem Engine</span>
                  </div>
                  {[
                    { name: 'React 19 Shell (V8 JIT)', pid: 10, cpu: '31 %', mem: '1,480 MB', cap: 'REACT 19 | ELECTRON | V8' },
                    { name: 'TypeScript IPC Host', pid: 11, cpu: '11 %', mem: '890 MB', cap: 'TS-NODE | ASYNC IPC' },
                    { name: 'Sector Cloner (Raw DMA)', pid: 12, cpu: '16 %', mem: '512 MB', cap: 'PHYSICAL_LBA | BITSTREAM' },
                    { name: 'dwm.exe (Fluent DWM)', pid: 1, cpu: '8 %', mem: '640 MB', cap: 'COMPOSITOR | BLOOM 60FPS' },
                    { name: 'explorer.exe (WinUI)', pid: 3, cpu: '4 %', mem: '380 MB', cap: 'SHELL | USER_GUI' },
                  ].map((p) => (
                    <div key={p.pid} className="grid grid-cols-[240px_80px_100px_100px_1fr] p-2.5 border-t border-slate-800 bg-[#0E1626]/80 hover:bg-slate-800/80">
                      <span className="font-semibold text-slate-200">{p.name}</span>
                      <span className="text-slate-400">{p.pid}</span>
                      <span className="text-cyan-400">{p.cpu}</span>
                      <span className="text-slate-300">{p.mem}</span>
                      <span className="text-slate-400 text-[11px]">{p.cap}</span>
                    </div>
                  ))}
                </div>
              </div>
            </div>
          )}
        </div>

        {/* Right Column 1 */}
        <div className="flex flex-col gap-3">
          {[
            { id: 'rec', title: 'recovery_ac...', icon: '📄', tag: 'explorer' },
            { id: 'node', title: 'nodejsRe...', icon: '⚙️', tag: 'terminal' },
            { id: 'force', title: 'Force_Virtu...', icon: '📄', tag: 'explorer' },
            { id: 'content', title: 'content.txt', icon: '📄', tag: 'explorer' },
            { id: 'verb', title: 'Verborse.bat', icon: '⚙️', tag: 'terminal' },
          ].map((item) => (
            <div
              key={item.id}
              onClick={() => { setSelectedIcon(item.id); toggleWindow(item.tag); }}
              className={`flex flex-col items-center justify-center p-2 rounded-lg cursor-pointer transition-all ${
                selectedIcon === item.id ? 'bg-blue-600/30 border border-blue-400/50' : 'hover:bg-slate-800/40'
              }`}
            >
              <span className="text-2xl drop-shadow-md">{item.icon}</span>
              <span className="text-[11px] text-center font-medium mt-1 drop-shadow leading-tight">{item.title}</span>
            </div>
          ))}
        </div>

        {/* Right Column 2 */}
        <div className="flex flex-col gap-3">
          {[
            { id: 'main', title: 'main.txt', icon: '📄', tag: 'explorer' },
            { id: 'log', title: 'Log.txt', icon: '📄', tag: 'explorer' },
            { id: 'games', title: 'Games_Lis...', icon: '📄', tag: 'explorer' },
            { id: 'link', title: 'link.txt', icon: '📄', tag: 'explorer' },
            { id: 'restart', title: 'Restart_Net...', icon: '🔄', tag: 'terminal' },
          ].map((item) => (
            <div
              key={item.id}
              onClick={() => { setSelectedIcon(item.id); toggleWindow(item.tag); }}
              className={`flex flex-col items-center justify-center p-2 rounded-lg cursor-pointer transition-all ${
                selectedIcon === item.id ? 'bg-blue-600/30 border border-blue-400/50' : 'hover:bg-slate-800/40'
              }`}
            >
              <span className="text-2xl drop-shadow-md">{item.icon}</span>
              <span className="text-[11px] text-center font-medium mt-1 drop-shadow leading-tight">{item.title}</span>
            </div>
          ))}
        </div>
      </div>

      {/* 3. Centered Windows 11 Start Menu */}
      {startMenuOpen && (
        <div className="absolute bottom-14 left-1/2 -translate-x-1/2 w-[520px] h-[540px] mica-surface rounded-2xl z-50 flex flex-col window-enter overflow-hidden p-6 border border-slate-700/80 shadow-2xl">
          <div className="flex items-center gap-3 bg-[#0B1120] px-4 py-2 rounded-full border border-slate-700 text-xs text-slate-400 mb-6">
            <Search className="w-4 h-4" />
            <span>Type here to search apps, settings, and files...</span>
          </div>

          <div className="text-xs font-bold text-slate-300 mb-4">Pinned</div>
          <div className="grid grid-cols-4 gap-4 mb-6">
            {[
              { title: 'File Explorer', icon: '📁', tag: 'explorer' },
              { title: 'Sector Cloner', icon: '💽', tag: 'diskclone' },
              { title: 'Task Manager', icon: '📊', tag: 'taskmgr' },
              { title: 'VLC Player', icon: '🟠', tag: 'vlc' },
              { title: 'App Installer', icon: '📦', tag: 'installer' },
              { title: 'Terminal CLI', icon: '⌨️', tag: 'terminal' },
              { title: 'Edge Browser', icon: '🌐', tag: 'terminal' },
              { title: 'Antigravity IDE', icon: '🚀', tag: 'terminal' },
            ].map((app) => (
              <div 
                key={app.title}
                onClick={() => { toggleWindow(app.tag); setStartMenuOpen(false); }}
                className="flex flex-col items-center gap-1.5 p-2.5 rounded-xl hover:bg-slate-800/60 cursor-pointer transition-all"
              >
                <span className="text-3xl">{app.icon}</span>
                <span className="text-xs font-medium text-center">{app.title}</span>
              </div>
            ))}
          </div>

          <div className="text-xs font-bold text-slate-300 mb-3">Recommended</div>
          <div className="space-y-2 flex-1">
            {['main.txt - Desktop Document', 'backup_disk.img - Sector Cloner Target', 'matrix_intro_64.mp4 - VLC Player'].map((rec) => (
              <div key={rec} className="flex items-center gap-3 p-2 bg-slate-800/40 rounded-lg text-xs hover:bg-slate-800/80 cursor-pointer">
                <span>📄</span>
                <span className="text-slate-300">{rec}</span>
              </div>
            ))}
          </div>

          {/* User Account & Power Bar */}
          <div className="mt-auto pt-4 border-t border-slate-800 flex items-center justify-between">
            <div className="flex items-center gap-3">
              <div className="w-8 h-8 rounded-full bg-blue-600 flex items-center justify-center font-bold text-xs">A</div>
              <div>
                <div className="text-xs font-semibold">Administrator</div>
                <div className="text-[10px] text-emerald-400">Ring 0 Full Privilege</div>
              </div>
            </div>

            <div className="relative">
              <button 
                onClick={() => setPowerMenuOpen(!powerMenuOpen)}
                className="p-2 hover:bg-slate-800 rounded-lg text-red-400"
              >
                <Power className="w-4 h-4" />
              </button>

              {powerMenuOpen && (
                <div className="absolute bottom-10 right-0 w-44 bg-[#141C2E] border border-slate-700 rounded-xl shadow-xl p-1 text-xs z-50">
                  <div className="flex items-center gap-2 p-2 hover:bg-slate-800 rounded-lg cursor-pointer">
                    <Moon className="w-3.5 h-3.5 text-cyan-400" />
                    <span>Hibernate / Sleep</span>
                  </div>
                  <div className="flex items-center gap-2 p-2 hover:bg-slate-800 rounded-lg cursor-pointer text-red-400">
                    <Power className="w-3.5 h-3.5" />
                    <span>Shut down (ACPI)</span>
                  </div>
                  <div className="flex items-center gap-2 p-2 hover:bg-slate-800 rounded-lg cursor-pointer text-amber-400">
                    <RefreshCw className="w-3.5 h-3.5" />
                    <span>Restart (Reset)</span>
                  </div>
                </div>
              )}
            </div>
          </div>
        </div>
      )}

      {/* 4. Windows 11 Acrylic Taskbar Dock */}
      <div className="relative z-40 h-12 acrylic-dock flex items-center justify-between px-3">
        {/* Left / Center Taskbar Icons */}
        <div className="flex items-center gap-1.5 mx-auto">
          {/* Start Menu Button */}
          <button 
            onClick={() => setStartMenuOpen(!startMenuOpen)}
            className={`p-2 rounded-lg taskbar-icon-hover ${startMenuOpen ? 'bg-blue-600/30' : ''}`}
          >
            <div className="grid grid-cols-2 gap-0.5 w-4 h-4">
              <div className="bg-cyan-400 rounded-xs" />
              <div className="bg-cyan-400 rounded-xs" />
              <div className="bg-cyan-400 rounded-xs" />
              <div className="bg-cyan-400 rounded-xs" />
            </div>
          </button>

          {/* Dock Icons */}
          {[
            { id: 'search', icon: '🔍', action: () => toggleWindow('terminal') },
            { id: 'edge', icon: '🌐', action: () => toggleWindow('terminal') },
            { id: 'installer', icon: '📦', action: () => toggleWindow('installer') },
            { id: 'explorer', icon: '📁', action: () => toggleWindow('explorer'), active: activeWindows.includes('explorer') },
            { id: 'chrome', icon: '🔴', action: () => toggleWindow('terminal') },
            { id: 'taskmgr', icon: '📊', action: () => toggleWindow('taskmgr'), active: activeWindows.includes('taskmgr') },
            { id: 'agy', icon: '🚀', action: () => toggleWindow('terminal') },
            { id: 'diskclone', icon: '💽', action: () => toggleWindow('diskclone'), active: activeWindows.includes('diskclone') },
            { id: 'vlc', icon: '🟠', action: () => toggleWindow('vlc'), active: activeWindows.includes('vlc') },
          ].map((item) => (
            <button
              key={item.id}
              onClick={item.action}
              className={`p-2 rounded-lg taskbar-icon-hover relative flex flex-col items-center ${
                item.active ? 'bg-slate-800/80' : ''
              }`}
            >
              <span className="text-lg">{item.icon}</span>
              {item.active && <div className="absolute bottom-0.5 w-3 h-0.5 bg-cyan-400 rounded-full" />}
            </button>
          ))}
        </div>

        {/* Right System Tray */}
        <div className="flex items-center gap-3 text-xs text-slate-300">
          <div className="flex items-center gap-2 p-1.5 hover:bg-slate-800 rounded-lg cursor-pointer">
            <Wifi className="w-3.5 h-3.5 text-cyan-400" />
            <Volume2 className="w-3.5 h-3.5" />
          </div>
          <div className="p-1.5 hover:bg-slate-800 rounded-lg cursor-pointer font-medium">ENG</div>
          <div className="flex flex-col items-end leading-none p-1 hover:bg-slate-800 rounded cursor-pointer">
            <span className="font-semibold text-[11px]">{timeStr}</span>
            <span className="text-[9px] text-slate-400 mt-0.5">{dateStr}</span>
          </div>
        </div>
      </div>
    </div>
  );
};
