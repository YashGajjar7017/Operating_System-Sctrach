import React, { useState, useEffect } from 'react';
import { 
  Play, Square, RotateCcw, Pause, Search, Filter, Cpu, 
  Activity, CheckCircle2, AlertCircle, Clock, ShieldCheck, Terminal, Layers
} from 'lucide-react';
import { ServiceItem, ServiceLogEntry, ServiceStatus, StartupType } from '../types';

export const ServicesApp: React.FC = () => {
  const [viewMode, setViewMode] = useState<'services' | 'events'>('services');
  const [searchQuery, setSearchQuery] = useState('');
  const [categoryFilter, setCategoryFilter] = useState<string>('All');
  const [selectedServiceName, setSelectedServiceName] = useState<string>('WinDefend');

  const [services, setServices] = useState<ServiceItem[]>([
    {
      name: 'XenithraKernel',
      displayName: 'Xenithra Ring 0 Scheduler & IDT Core',
      status: 'Running',
      startupType: 'Automatic',
      logOnAs: 'LocalSystem (Ring 0)',
      pid: 1,
      memoryMb: 128.4,
      cpuPercent: 4.2,
      description: 'Provides higher-half x86_64 CPU core scheduling, interrupt dispatch, and physical memory allocation.',
      category: 'Core Kernel',
    },
    {
      name: 'WinDefend',
      displayName: 'Windows Defender Antivirus Service',
      status: 'Running',
      startupType: 'Automatic',
      logOnAs: 'NT AUTHORITY\\SYSTEM',
      pid: 104,
      memoryMb: 312.8,
      cpuPercent: 2.1,
      description: 'Scans for malware, rootkits, and unauthorized kernel privilege escalation.',
      category: 'Security',
    },
    {
      name: 'MpsSvc',
      displayName: 'Kernel Private Firewall & Packet Filter',
      status: 'Running',
      startupType: 'Automatic',
      logOnAs: 'NT AUTHORITY\\LocalService',
      pid: 108,
      memoryMb: 94.2,
      cpuPercent: 1.5,
      description: 'Filters inbound and outbound TCP/UDP/ICMP packets based on stateful security rules.',
      category: 'Security',
    },
    {
      name: 'Dnscache',
      displayName: 'DNS Client Resolver Service',
      status: 'Running',
      startupType: 'Automatic',
      logOnAs: 'NT AUTHORITY\\NetworkService',
      pid: 112,
      memoryMb: 48.6,
      cpuPercent: 0.8,
      description: 'Resolves and caches Domain Name System (DNS) names (e.g. www.google.com) for applications.',
      category: 'Networking',
    },
    {
      name: 'V8HostSvc',
      displayName: 'V8 JavaScript JIT Engine Host',
      status: 'Running',
      startupType: 'Automatic',
      logOnAs: 'LocalSystem',
      pid: 220,
      memoryMb: 480.5,
      cpuPercent: 8.4,
      description: 'Hosts the bare-metal V8 engine, executing custom React reconcilers and JavaScript apps.',
      category: 'System Runtime',
    },
    {
      name: 'DwmSvc',
      displayName: 'Desktop Window Manager (Fluent Compositor)',
      status: 'Running',
      startupType: 'Automatic',
      logOnAs: 'NT AUTHORITY\\SYSTEM',
      pid: 80,
      memoryMb: 240.2,
      cpuPercent: 6.0,
      description: 'Composites transparent acrylic window surfaces, animations, and mouse cursors at 60 FPS.',
      category: 'UI & Graphics',
    },
    {
      name: 'Audiosrv',
      displayName: 'Audio Endpoint Builder & Synthesizer',
      status: 'Running',
      startupType: 'Automatic',
      logOnAs: 'LocalSystem',
      pid: 140,
      memoryMb: 36.1,
      cpuPercent: 0.4,
      description: 'Synthesizes polyphonic startup chimes, sound notifications, and media player output.',
      category: 'System Runtime',
    },
    {
      name: 'Spooler',
      displayName: 'Print Spooler & Virtual PDF Export',
      status: 'Stopped',
      startupType: 'Manual',
      logOnAs: 'LocalSystem',
      pid: null,
      memoryMb: 0,
      cpuPercent: 0.0,
      description: 'Manages print jobs and document rendering queues.',
      category: 'System Runtime',
    },
    {
      name: 'wuauserv',
      displayName: 'Xenithra OS Update Agent',
      status: 'Running',
      startupType: 'Manual',
      logOnAs: 'LocalSystem',
      pid: 310,
      memoryMb: 72.0,
      cpuPercent: 0.2,
      description: 'Checks for and applies kernel security patches and driver updates.',
      category: 'Core Kernel',
    },
  ]);

  const [eventLogs, setEventLogs] = useState<ServiceLogEntry[]>([
    { id: 'evt-1', timestamp: '12:45:02', source: 'Dnscache', eventId: 1010, level: 'Information', message: 'Domain "www.google.com" successfully resolved to 142.250.190.46 via UDP:53.' },
    { id: 'evt-2', timestamp: '12:44:18', source: 'WinDefend', eventId: 2004, level: 'Information', message: 'Real-time signature database updated to version 19.4.2026.09.' },
    { id: 'evt-3', timestamp: '12:42:55', source: 'MpsSvc', eventId: 3001, level: 'Warning', message: 'Inbound port scan probe blocked on TCP Port 22 (SSH Drop Rule enforced).' },
    { id: 'evt-4', timestamp: '12:40:11', source: 'V8HostSvc', eventId: 5012, level: 'Information', message: 'V8 JIT compilation completed for DesktopShell.tsx (1879 modules linked).' },
    { id: 'evt-5', timestamp: '12:38:00', source: 'XenithraKernel', eventId: 1, level: 'Information', message: 'Higher-half memory tables initialized with SMEP/SMAP ring barriers.' },
  ]);

  const selectedService = services.find((s) => s.name === selectedServiceName) || services[0];

  const handleStartService = (name: string) => {
    setServices((prev) =>
      prev.map((s) =>
        s.name === name
          ? { ...s, status: 'Running', pid: Math.floor(Math.random() * 800 + 100), memoryMb: 85.0, cpuPercent: 1.2 }
          : s
      )
    );
    addLog(name, 'Information', `Service "${name}" transition state to RUNNING.`);
  };

  const handleStopService = (name: string) => {
    if (name === 'XenithraKernel') {
      alert('Cannot stop Xenithra Ring 0 Scheduler core service.');
      return;
    }
    setServices((prev) =>
      prev.map((s) =>
        s.name === name ? { ...s, status: 'Stopped', pid: null, memoryMb: 0, cpuPercent: 0 } : s
      )
    );
    addLog(name, 'Warning', `Service "${name}" was stopped by Administrator.`);
  };

  const handleRestartService = (name: string) => {
    setServices((prev) =>
      prev.map((s) => (s.name === name ? { ...s, status: 'Starting' } : s))
    );
    setTimeout(() => {
      setServices((prev) =>
        prev.map((s) =>
          s.name === name
            ? { ...s, status: 'Running', pid: Math.floor(Math.random() * 800 + 100), memoryMb: 92.4, cpuPercent: 2.5 }
            : s
        )
      );
      addLog(name, 'Information', `Service "${name}" successfully restarted.`);
    }, 400);
  };

  const handleChangeStartupType = (name: string, type: StartupType) => {
    setServices((prev) =>
      prev.map((s) => (s.name === name ? { ...s, startupType: type } : s))
    );
    addLog(name, 'Information', `Startup type changed to ${type} for service "${name}".`);
  };

  const addLog = (source: string, level: 'Information' | 'Warning' | 'Error', message: string) => {
    const newEntry: ServiceLogEntry = {
      id: `evt-${Date.now()}`,
      timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }),
      source,
      eventId: Math.floor(Math.random() * 9000 + 1000),
      level,
      message,
    };
    setEventLogs((prev) => [newEntry, ...prev]);
  };

  const filteredServices = services.filter((s) => {
    const matchesSearch =
      s.displayName.toLowerCase().includes(searchQuery.toLowerCase()) ||
      s.name.toLowerCase().includes(searchQuery.toLowerCase()) ||
      s.description.toLowerCase().includes(searchQuery.toLowerCase());
    const matchesCat = categoryFilter === 'All' || s.category === categoryFilter;
    return matchesSearch && matchesCat;
  });

  return (
    <div className="flex flex-col h-full w-full bg-[#0C1220] text-slate-100 select-none overflow-hidden font-sans">
      {/* 1. Header Toolbar */}
      <div className="h-14 bg-[#080D18] px-4 flex items-center justify-between border-b border-slate-800">
        <div className="flex items-center gap-3">
          <div className="w-8 h-8 rounded-lg bg-blue-600 flex items-center justify-center text-white shadow-md">
            <Cpu className="w-4 h-4" />
          </div>
          <div>
            <h2 className="text-sm font-bold text-slate-100">Services Management Console (services.msc)</h2>
            <p className="text-[10px] text-slate-400">Manage Ring 0 and Ring 3 background daemons and service hosts</p>
          </div>
        </div>

        {/* View Switcher */}
        <div className="flex items-center gap-2 bg-[#121A2C] p-1 rounded-lg border border-slate-800 text-xs font-medium">
          <button
            onClick={() => setViewMode('services')}
            className={`px-3 py-1 rounded-md transition-all ${
              viewMode === 'services' ? 'bg-blue-600 text-white shadow' : 'text-slate-400 hover:text-white'
            }`}
          >
            Services List
          </button>
          <button
            onClick={() => setViewMode('events')}
            className={`px-3 py-1 rounded-md transition-all ${
              viewMode === 'events' ? 'bg-blue-600 text-white shadow' : 'text-slate-400 hover:text-white'
            }`}
          >
            Event Viewer Logs
          </button>
        </div>
      </div>

      {/* 2. Control Deck & Filters */}
      {viewMode === 'services' && (
        <div className="bg-[#101726] px-4 py-2 border-b border-slate-800 flex items-center justify-between gap-4">
          {/* Action Buttons for Selected Service */}
          <div className="flex items-center gap-2">
            <button
              onClick={() => handleStartService(selectedService.name)}
              disabled={selectedService.status === 'Running'}
              className="px-3 py-1.5 bg-emerald-600/80 hover:bg-emerald-500 disabled:opacity-40 rounded-lg text-xs font-semibold flex items-center gap-1.5 text-white transition-all"
            >
              <Play className="w-3.5 h-3.5" />
              Start
            </button>
            <button
              onClick={() => handleStopService(selectedService.name)}
              disabled={selectedService.status === 'Stopped'}
              className="px-3 py-1.5 bg-red-600/80 hover:bg-red-500 disabled:opacity-40 rounded-lg text-xs font-semibold flex items-center gap-1.5 text-white transition-all"
            >
              <Square className="w-3.5 h-3.5" />
              Stop
            </button>
            <button
              onClick={() => handleRestartService(selectedService.name)}
              className="px-3 py-1.5 bg-slate-800 hover:bg-slate-700 rounded-lg text-xs font-semibold flex items-center gap-1.5 text-slate-200 transition-all"
            >
              <RotateCcw className="w-3.5 h-3.5" />
              Restart
            </button>
          </div>

          {/* Search & Category Filter */}
          <div className="flex items-center gap-3">
            <div className="flex items-center gap-1 text-xs">
              {['All', 'Core Kernel', 'Security', 'Networking', 'System Runtime'].map((cat) => (
                <button
                  key={cat}
                  onClick={() => setCategoryFilter(cat)}
                  className={`px-2.5 py-1 rounded-md transition-all ${
                    categoryFilter === cat ? 'bg-blue-600/30 text-blue-400 font-semibold border border-blue-500/40' : 'text-slate-400 hover:text-slate-200'
                  }`}
                >
                  {cat}
                </button>
              ))}
            </div>

            <div className="flex items-center gap-2 bg-[#090F1C] border border-slate-700/80 rounded-lg px-2.5 py-1 text-xs text-slate-300 w-48">
              <Search className="w-3.5 h-3.5 text-slate-400" />
              <input
                type="text"
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                placeholder="Filter services..."
                className="w-full bg-transparent outline-none text-[11px]"
              />
            </div>
          </div>
        </div>
      )}

      {/* 3. Main Services Table or Event Logs */}
      <div className="flex-1 flex overflow-hidden">
        {viewMode === 'services' ? (
          <div className="flex-1 flex flex-col md:flex-row overflow-hidden">
            {/* Table Area */}
            <div className="flex-1 overflow-y-auto">
              <table className="w-full text-left text-xs border-collapse">
                <thead className="bg-[#090F1C] sticky top-0 border-b border-slate-800 text-slate-400 font-bold">
                  <tr>
                    <th className="p-3">Name</th>
                    <th className="p-3">Description</th>
                    <th className="p-3">Status</th>
                    <th className="p-3">Startup Type</th>
                    <th className="p-3">Log On As</th>
                    <th className="p-3">PID</th>
                    <th className="p-3">Memory</th>
                  </tr>
                </thead>
                <tbody>
                  {filteredServices.map((srv) => {
                    const isSelected = selectedServiceName === srv.name;
                    return (
                      <tr
                        key={srv.name}
                        onClick={() => setSelectedServiceName(srv.name)}
                        className={`border-b border-slate-800/60 cursor-pointer transition-colors ${
                          isSelected ? 'bg-blue-600/20 text-white' : 'hover:bg-slate-800/40 text-slate-300'
                        }`}
                      >
                        <td className="p-3 font-semibold text-slate-100 flex items-center gap-2">
                          <span className={`w-2 h-2 rounded-full ${srv.status === 'Running' ? 'bg-emerald-400 animate-pulse' : 'bg-slate-600'}`} />
                          {srv.displayName}
                        </td>
                        <td className="p-3 text-slate-400 max-w-xs truncate text-[11px]">{srv.description}</td>
                        <td className="p-3">
                          <span className={`px-2 py-0.5 rounded text-[10px] font-bold ${
                            srv.status === 'Running' ? 'bg-emerald-500/20 text-emerald-400' : 'bg-slate-700 text-slate-400'
                          }`}>
                            {srv.status}
                          </span>
                        </td>
                        <td className="p-3 text-slate-300">
                          <select
                            value={srv.startupType}
                            onClick={(e) => e.stopPropagation()}
                            onChange={(e) => handleChangeStartupType(srv.name, e.target.value as StartupType)}
                            className="bg-[#0B1220] border border-slate-700 rounded px-1.5 py-0.5 text-[11px] text-slate-200 outline-none"
                          >
                            <option value="Automatic">Automatic</option>
                            <option value="Manual">Manual</option>
                            <option value="Disabled">Disabled</option>
                          </select>
                        </td>
                        <td className="p-3 text-slate-400 font-mono text-[11px]">{srv.logOnAs}</td>
                        <td className="p-3 font-mono text-cyan-400">{srv.pid || '-'}</td>
                        <td className="p-3 font-mono text-slate-300">{srv.memoryMb ? `${srv.memoryMb} MB` : '0 MB'}</td>
                      </tr>
                    );
                  })}
                </tbody>
              </table>
            </div>

            {/* Side Detail Inspector Pane */}
            <div className="w-80 bg-[#080D18] border-l border-slate-800 p-4 flex flex-col justify-between overflow-y-auto">
              <div className="space-y-4">
                <div className="border-b border-slate-800 pb-3">
                  <div className="text-[10px] text-cyan-400 font-bold uppercase tracking-wider">{selectedService.category}</div>
                  <h3 className="text-sm font-bold text-white mt-0.5">{selectedService.displayName}</h3>
                  <div className="text-xs font-mono text-slate-400">Service Name: {selectedService.name}</div>
                </div>

                <div className="space-y-2">
                  <label className="text-[11px] font-bold text-slate-400">Description</label>
                  <p className="text-xs text-slate-300 leading-relaxed bg-[#0F172A] p-3 rounded-lg border border-slate-800">
                    {selectedService.description}
                  </p>
                </div>

                <div className="space-y-2">
                  <label className="text-[11px] font-bold text-slate-400">Service Telemetry</label>
                  <div className="p-3 bg-[#0F172A] rounded-lg border border-slate-800 space-y-2 text-xs">
                    <div className="flex justify-between">
                      <span className="text-slate-400">Service State:</span>
                      <span className="font-bold text-emerald-400">{selectedService.status}</span>
                    </div>
                    <div className="flex justify-between">
                      <span className="text-slate-400">CPU Load:</span>
                      <span className="font-bold text-cyan-400">{selectedService.cpuPercent}%</span>
                    </div>
                    <div className="flex justify-between">
                      <span className="text-slate-400">Resident Memory:</span>
                      <span className="font-bold text-purple-400">{selectedService.memoryMb} MB</span>
                    </div>
                    <div className="flex justify-between">
                      <span className="text-slate-400">Security Context:</span>
                      <span className="text-slate-300">{selectedService.logOnAs}</span>
                    </div>
                  </div>
                </div>
              </div>

              <div className="p-3 bg-slate-900/60 rounded-xl border border-slate-800/80 text-[10px] text-slate-400 text-center">
                Xenithra Service Control Manager (SCM Ring 0)
              </div>
            </div>
          </div>
        ) : (
          /* EVENT VIEWER VIEW */
          <div className="flex-1 p-4 overflow-y-auto space-y-3">
            <div className="flex justify-between items-center pb-2 border-b border-slate-800">
              <h3 className="text-xs font-bold text-slate-200">System Audit & Service Event Logs (eventvwr.msc)</h3>
              <span className="text-[11px] text-slate-400">{eventLogs.length} Events Logged</span>
            </div>

            <div className="space-y-2">
              {eventLogs.map((log) => (
                <div key={log.id} className="p-3 bg-slate-900/70 border border-slate-800 rounded-xl flex items-start gap-3">
                  <div className={`p-1.5 rounded-lg shrink-0 ${
                    log.level === 'Information' ? 'bg-blue-500/20 text-blue-400' :
                    log.level === 'Warning' ? 'bg-amber-500/20 text-amber-400' : 'bg-red-500/20 text-red-400'
                  }`}>
                    <Activity className="w-4 h-4" />
                  </div>
                  <div className="flex-1 space-y-1">
                    <div className="flex justify-between items-center text-xs">
                      <span className="font-bold text-white">{log.source} (Event ID: {log.eventId})</span>
                      <span className="text-[10px] text-slate-400">{log.timestamp}</span>
                    </div>
                    <p className="text-xs text-slate-300 font-mono">{log.message}</p>
                  </div>
                </div>
              ))}
            </div>
          </div>
        )}
      </div>
    </div>
  );
};
