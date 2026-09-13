import React, { useState, useEffect } from 'react';
import { 
  Layers, Cpu, ShieldCheck, Globe, Zap, ArrowRight, Activity, 
  CheckCircle2, RefreshCw, Send, Terminal, Play, Flame, Server
} from 'lucide-react';
import { PipelineStage, DnsQueryLog, V8Telemetry } from '../types';

export const PipelineInspectorApp: React.FC = () => {
  const [activeStage, setActiveStage] = useState<number>(2); // 0 to 4
  const [streamActive, setStreamActive] = useState(true);
  const [packetCount, setPacketCount] = useState(48920);
  const [dnsQueries, setDnsQueries] = useState<DnsQueryLog[]>([
    { domain: 'www.google.com', resolvedIp: '142.250.190.46', ttl: 300, time: '12:45:02', status: 'SUCCESS' },
    { domain: 'docs.xenithra.org', resolvedIp: '192.168.1.100', ttl: 3600, time: '12:43:18', status: 'CACHED' },
    { domain: 'api.github.com', resolvedIp: '140.82.121.6', ttl: 60, time: '12:41:50', status: 'SUCCESS' },
    { domain: 'malicious-probe.xyz', resolvedIp: '0.0.0.0', ttl: 0, time: '12:40:09', status: 'FILTERED_FIREWALL' },
  ]);

  const [v8Stats, setV8Stats] = useState<V8Telemetry>({
    heapUsedMb: 148,
    heapTotalMb: 512,
    jitCompilations: 1879,
    ipcMessagesPerSec: 124,
    gcPauseMs: 1.4,
  });

  const stages: PipelineStage[] = [
    {
      id: 's-kernel',
      name: '1. Higher-Half Kernel',
      subsystem: 'x86_64 Higher-Half ELF + UEFI GOP',
      ring: 'Ring 0',
      status: 'ONLINE',
      latencyMs: 0.2,
      throughput: '3.6 GHz (MLFQ)',
      description: 'Preemptive scheduler, PML4 higher-half paging, IDT interrupts, and physical memory manager.',
    },
    {
      id: 's-osstart',
      name: '2. OS Core Services',
      subsystem: 'Session Guard & SCM Daemons',
      ring: 'Ring 0',
      status: 'ACTIVE',
      latencyMs: 0.8,
      throughput: '128-bit Entropy',
      description: 'SMEP/SMAP hardware execution protection, token authorization, and ACPI power management.',
    },
    {
      id: 's-firewall',
      name: '3. Firewall & DNS Filter',
      subsystem: 'NetFilter Stateful Packet Inspection',
      ring: 'Ring 3',
      status: 'SYNCED',
      latencyMs: 1.2,
      throughput: '1.2 GB/s WireSpeed',
      description: 'Deep-packet filter inspecting TCP/UDP/ICMP, domain caching (www.google.com), and drop rules.',
    },
    {
      id: 's-v8',
      name: '4. V8 JavaScript Engine',
      subsystem: 'V8 JIT + nativeUI Bridge Reconciler',
      ring: 'V8 Host',
      status: 'ACTIVE',
      latencyMs: 2.5,
      throughput: '60 FPS Render AST',
      description: 'Translates JSX component trees into direct hardware draw calls or Web DOM nodes.',
    },
    {
      id: 's-electron',
      name: '5. Electron Desktop Shell',
      subsystem: 'React 19 Windows 11 Fluent Shell',
      ring: 'Electron IPC',
      status: 'ONLINE',
      latencyMs: 3.1,
      throughput: 'Async Dual-Port IPC',
      description: 'Windows 11 Acrylic Compositor, Start Menu, Window Manager, and interactive app suite.',
    },
  ];

  // Dynamic packet flow simulation
  useEffect(() => {
    let interval: any = null;
    if (streamActive) {
      interval = setInterval(() => {
        setPacketCount((p) => p + Math.floor(Math.random() * 8 + 2));
        setV8Stats((prev) => ({
          ...prev,
          ipcMessagesPerSec: Math.floor(115 + Math.random() * 25),
          heapUsedMb: Math.floor(145 + Math.random() * 8),
        }));
      }, 1000);
    }
    return () => clearInterval(interval);
  }, [streamActive]);

  const simulateDnsLookup = (domain: string) => {
    const isThreat = domain.includes('threat') || domain.includes('malware');
    const newLog: DnsQueryLog = {
      domain,
      resolvedIp: isThreat ? '0.0.0.0 (BLOCKED)' : `142.250.${Math.floor(Math.random() * 200)}.${Math.floor(Math.random() * 254 + 1)}`,
      ttl: 300,
      time: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }),
      status: isThreat ? 'FILTERED_FIREWALL' : 'SUCCESS',
    };
    setDnsQueries((prev) => [newLog, ...prev.slice(0, 7)]);
  };

  return (
    <div className="flex flex-col h-full w-full bg-[#080D1A] text-slate-100 select-none overflow-hidden font-sans">
      {/* 1. Header Toolbar */}
      <div className="h-14 bg-[#050914] px-6 flex items-center justify-between border-b border-slate-800">
        <div className="flex items-center gap-3">
          <div className="w-8 h-8 rounded-lg bg-gradient-to-tr from-cyan-500 to-blue-600 flex items-center justify-center text-white shadow-md">
            <Layers className="w-4 h-4" />
          </div>
          <div>
            <h2 className="text-sm font-bold text-slate-100">End-to-End Architectural Pipeline Inspector</h2>
            <p className="text-[10px] text-cyan-400 font-mono">Kernel ➔ OS Start ➔ Firewall/DNS ➔ V8 Engine ➔ Electron App</p>
          </div>
        </div>

        <div className="flex items-center gap-3">
          <span className="text-xs text-slate-400">Total Packets Inspected: <strong className="text-emerald-400 font-mono">{packetCount.toLocaleString()}</strong></span>
          <button
            onClick={() => setStreamActive(!streamActive)}
            className={`px-3 py-1 rounded-lg text-xs font-semibold flex items-center gap-1.5 transition-all ${
              streamActive ? 'bg-emerald-600/30 text-emerald-400 border border-emerald-500/50' : 'bg-slate-800 text-slate-400'
            }`}
          >
            <Activity className="w-3.5 h-3.5" />
            {streamActive ? 'Telemetry Streaming' : 'Paused'}
          </button>
        </div>
      </div>

      {/* 2. Interactive 5-Stage Pipeline Node Visualizer */}
      <div className="p-6 bg-[#0A1020] border-b border-slate-800">
        <div className="flex items-center justify-between gap-2">
          {stages.map((stg, idx) => {
            const isSelected = activeStage === idx;
            return (
              <React.Fragment key={stg.id}>
                {/* Stage Box */}
                <div
                  onClick={() => setActiveStage(idx)}
                  className={`flex-1 p-3.5 rounded-2xl cursor-pointer transition-all flex flex-col justify-between space-y-2 border ${
                    isSelected
                      ? 'bg-blue-600/20 border-blue-500 shadow-lg shadow-blue-500/20 scale-[1.02]'
                      : 'bg-slate-900/60 hover:bg-slate-800/60 border-slate-800'
                  }`}
                >
                  <div className="flex justify-between items-center text-[10px]">
                    <span className="font-bold text-cyan-400 uppercase">{stg.ring}</span>
                    <span className="px-1.5 py-0.5 rounded bg-emerald-500/20 text-emerald-400 font-semibold text-[9px]">
                      {stg.status}
                    </span>
                  </div>

                  <div>
                    <h4 className="text-xs font-bold text-white truncate">{stg.name}</h4>
                    <p className="text-[10px] text-slate-400 truncate mt-0.5">{stg.subsystem}</p>
                  </div>

                  <div className="flex justify-between text-[10px] text-slate-400 pt-1 border-t border-slate-800/80">
                    <span>Lat: <strong className="text-slate-200">{stg.latencyMs}ms</strong></span>
                    <span className="text-cyan-300 font-mono">{stg.throughput}</span>
                  </div>
                </div>

                {/* Arrow connector */}
                {idx < stages.length - 1 && (
                  <div className="text-slate-600 flex items-center justify-center">
                    <ArrowRight className={`w-4 h-4 ${streamActive ? 'text-cyan-400 animate-pulse' : ''}`} />
                  </div>
                )}
              </React.Fragment>
            );
          })}
        </div>
      </div>

      {/* 3. Deep Telemetry & Test Simulation Center */}
      <div className="flex-1 flex overflow-hidden">
        {/* Left Stage Details Pane */}
        <div className="flex-1 p-6 overflow-y-auto space-y-6">
          <div className="p-5 bg-slate-900/70 border border-slate-800 rounded-2xl space-y-3">
            <div className="flex justify-between items-center">
              <div>
                <span className="text-[10px] text-cyan-400 font-bold uppercase tracking-wider">{stages[activeStage].ring} Subsystem</span>
                <h3 className="text-base font-bold text-white">{stages[activeStage].name}</h3>
              </div>
              <span className="px-3 py-1 bg-blue-600/30 text-blue-400 border border-blue-500/50 rounded-full text-xs font-semibold">
                Live Node Inspected
              </span>
            </div>
            <p className="text-xs text-slate-300 leading-relaxed">
              {stages[activeStage].description}
            </p>
          </div>

          {/* Interactive Actions for the Pipeline */}
          <div className="space-y-3">
            <h4 className="text-xs font-bold text-slate-300 uppercase tracking-wider">Trigger Pipeline Events</h4>
            <div className="grid grid-cols-3 gap-3">
              <button
                onClick={() => simulateDnsLookup('www.google.com')}
                className="p-3 bg-slate-900/80 hover:bg-slate-800 border border-slate-700/80 rounded-xl text-left space-y-1 transition-all"
              >
                <div className="flex items-center gap-2 text-xs font-bold text-cyan-400">
                  <Globe className="w-4 h-4" />
                  DNS Resolve (Google)
                </div>
                <p className="text-[10px] text-slate-400">Queries UDP:53 via Kernel Dnscache to resolve www.google.com</p>
              </button>

              <button
                onClick={() => simulateDnsLookup('threat-payload-miner.com')}
                className="p-3 bg-slate-900/80 hover:bg-slate-800 border border-slate-700/80 rounded-xl text-left space-y-1 transition-all"
              >
                <div className="flex items-center gap-2 text-xs font-bold text-red-400">
                  <ShieldCheck className="w-4 h-4" />
                  Test Malicious Threat
                </div>
                <p className="text-[10px] text-slate-400">Verifies Kernel Private Firewall drops unauthorized domain probes</p>
              </button>

              <button
                onClick={() => {
                  setV8Stats((prev) => ({ ...prev, jitCompilations: prev.jitCompilations + 12 }));
                }}
                className="p-3 bg-slate-900/80 hover:bg-slate-800 border border-slate-700/80 rounded-xl text-left space-y-1 transition-all"
              >
                <div className="flex items-center gap-2 text-xs font-bold text-purple-400">
                  <Zap className="w-4 h-4" />
                  Trigger V8 JIT Emit
                </div>
                <p className="text-[10px] text-slate-400">Forces bytecode compilation for dynamic JSX reconciliation</p>
              </button>
            </div>
          </div>

          {/* V8 Engine Telemetry Box */}
          <div className="p-4 bg-slate-900/60 border border-slate-800 rounded-2xl space-y-3">
            <h4 className="text-xs font-bold text-slate-200 flex items-center gap-2">
              <Zap className="w-4 h-4 text-purple-400" />
              V8 Engine & IPC Telemetry Metrics
            </h4>
            <div className="grid grid-cols-4 gap-3">
              <div className="p-3 bg-black/40 rounded-xl">
                <div className="text-[10px] text-slate-400">V8 Heap In Use</div>
                <div className="text-sm font-bold text-purple-400 mt-1">{v8Stats.heapUsedMb} MB / {v8Stats.heapTotalMb} MB</div>
              </div>
              <div className="p-3 bg-black/40 rounded-xl">
                <div className="text-[10px] text-slate-400">JIT Compilations</div>
                <div className="text-sm font-bold text-cyan-400 mt-1">{v8Stats.jitCompilations} Modules</div>
              </div>
              <div className="p-3 bg-black/40 rounded-xl">
                <div className="text-[10px] text-slate-400">IPC Throughput</div>
                <div className="text-sm font-bold text-emerald-400 mt-1">{v8Stats.ipcMessagesPerSec} msg/sec</div>
              </div>
              <div className="p-3 bg-black/40 rounded-xl">
                <div className="text-[10px] text-slate-400">GC Pause Latency</div>
                <div className="text-sm font-bold text-slate-200 mt-1">{v8Stats.gcPauseMs} ms</div>
              </div>
            </div>
          </div>
        </div>

        {/* Right Live DNS & Packet Log Stream */}
        <div className="w-84 bg-[#050914] border-l border-slate-800 p-4 flex flex-col justify-between overflow-y-auto font-mono text-xs">
          <div className="space-y-3">
            <div className="flex justify-between items-center border-b border-slate-800 pb-2">
              <span className="font-bold text-slate-200 font-sans">Live DNS & NetFilter Log</span>
              <span className="text-[10px] text-emerald-400 animate-pulse">● LIVE</span>
            </div>

            <div className="space-y-2">
              {dnsQueries.map((log, i) => (
                <div key={i} className="p-2.5 bg-[#090F1C] border border-slate-800/80 rounded-xl space-y-1 text-[11px]">
                  <div className="flex justify-between items-center">
                    <span className="text-cyan-300 font-bold truncate max-w-[140px]">{log.domain}</span>
                    <span className={`px-1.5 py-0.2 rounded text-[9px] font-bold ${
                      log.status === 'SUCCESS' ? 'bg-emerald-500/20 text-emerald-400' :
                      log.status === 'CACHED' ? 'bg-blue-500/20 text-blue-400' : 'bg-red-500/20 text-red-400'
                    }`}>
                      {log.status}
                    </span>
                  </div>
                  <div className="text-slate-400 text-[10px]">
                    IP: <span className="text-white">{log.resolvedIp}</span> (TTL: {log.ttl}s)
                  </div>
                  <div className="text-[9px] text-slate-500 text-right">{log.time}</div>
                </div>
              ))}
            </div>
          </div>

          <div className="p-2 bg-slate-900/60 rounded text-[10px] text-slate-400 text-center font-sans">
            Xenithra Integrated Architectural Gateway
          </div>
        </div>
      </div>
    </div>
  );
};
