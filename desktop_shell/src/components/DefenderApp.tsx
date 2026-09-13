import React, { useState, useEffect } from 'react';
import { 
  ShieldCheck, ShieldAlert, Shield, AlertTriangle, CheckCircle, RefreshCw, 
  Lock, Activity, Cpu, HardDrive, Filter, Trash2, Eye, Play, Zap, Radio
} from 'lucide-react';
import { ThreatItem, FirewallRuleItem } from '../types';

export const DefenderApp: React.FC = () => {
  const [activeTab, setActiveTab] = useState<'overview' | 'scan' | 'protection' | 'firewall' | 'threats' | 'kernel'>('overview');
  
  // Real-time protection toggles
  const [realtimeShield, setRealtimeShield] = useState(true);
  const [cloudProtection, setCloudProtection] = useState(true);
  const [tamperProtection, setTamperProtection] = useState(true);
  const [ransomwareShield, setRansomwareShield] = useState(true);
  const [sessionGuard, setSessionGuard] = useState(true);
  
  // Scanning state
  const [scanning, setScanning] = useState(false);
  const [scanProgress, setScanProgress] = useState(0);
  const [scannedFilesCount, setScannedFilesCount] = useState(0);
  const [currentScanFile, setCurrentScanFile] = useState('');
  const [scanType, setScanType] = useState<'quick' | 'full' | 'memory'>('quick');
  const [lastScanTime, setLastScanTime] = useState('Today, 12:45 PM (Quick scan)');

  // Threat history
  const [threats, setThreats] = useState<ThreatItem[]>([
    {
      id: 'TH-0921',
      name: 'Exploit:x86_64/Ring0.NullDeref',
      severity: 'High',
      category: 'Privilege Escalation',
      affectedFile: '\\Device\\Harddisk0\\Partition1\\efi\\boot\\payload.bin',
      dateDetected: '2026-09-13 11:20:14',
      status: 'Quarantined',
    },
    {
      id: 'TH-0844',
      name: 'Backdoor:Win64/SrvInjector.B',
      severity: 'Severe',
      category: 'Trojan',
      affectedFile: 'C:\\Windows\\System32\\drivers\\suspicious_hook.sys',
      dateDetected: '2026-09-12 18:04:09',
      status: 'Quarantined',
    },
    {
      id: 'TH-0711',
      name: 'NetFilter:Flood/ICMP.Amplifier',
      severity: 'Medium',
      category: 'Packet Flood',
      affectedFile: 'Kernel Network Stack (Port 80/Raw)',
      dateDetected: '2026-09-11 09:15:30',
      status: 'Blocked',
    }
  ]);

  // Firewall Rules
  const [firewallRules, setFirewallRules] = useState<FirewallRuleItem[]>([
    { id: 1, protocol: 'TCP', port: 80, action: 'ALLOW', description: 'HTTP Web Traffic (Google & CDNs)', enabled: true, packetCount: 4280 },
    { id: 2, protocol: 'TCP', port: 443, action: 'ALLOW', description: 'HTTPS Secure Sockets / TLS 1.3', enabled: true, packetCount: 19842 },
    { id: 3, protocol: 'UDP', port: 53, action: 'ALLOW', description: 'DNS Domain Name Resolution (Dnscache)', enabled: true, packetCount: 1530 },
    { id: 4, protocol: 'TCP', port: 22, action: 'DROP', description: 'SSH Secure Shell Remote Admin Port', enabled: true, packetCount: 142 },
    { id: 5, protocol: 'TCP', port: 445, action: 'DROP', description: 'SMB Direct File Sharing (WannaCry Shield)', enabled: true, packetCount: 68 },
    { id: 6, protocol: 'ICMP', port: 0, action: 'DROP', description: 'ICMP Ping & Echo Broadcast Flood Guard', enabled: true, packetCount: 924 },
  ]);

  // Kernel Session Security Telemetry
  const [sessionToken, setSessionToken] = useState('0x9F4C...B28A');
  const [activeSessions, setActiveSessions] = useState(3);
  const [blockedHijacks, setBlockedHijacks] = useState(14);
  const [entropyHealth, setEntropyHealth] = useState(99.4);

  // Scan simulation loop
  useEffect(() => {
    let timer: any = null;
    if (scanning) {
      const fileList = [
        'C:\\Windows\\System32\\ntoskrnl.exe',
        'C:\\Windows\\System32\\drivers\\acpi.sys',
        'C:\\Windows\\System32\\services.exe',
        'C:\\Windows\\System32\\lsass.exe',
        'C:\\Windows\\System32\\svchost.exe',
        'C:\\Kernel\\Memory\\PagingTable.cr3',
        'C:\\Program Files\\Google\\Chrome\\chrome.exe',
        'C:\\Windows\\System32\\dnsapi.dll',
        'C:\\Kernel\\Security\\firewall_rules.dat',
        'C:\\EFI\\BOOT\\BOOTX64.EFI',
      ];
      timer = setInterval(() => {
        setScanProgress((prev) => {
          if (prev >= 100) {
            setScanning(false);
            setLastScanTime(`Today, ${new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })} (${scanType} scan)`);
            return 100;
          }
          const next = prev + 5;
          const sampleFile = fileList[Math.floor((next / 10) % fileList.length)] || fileList[0];
          setCurrentScanFile(sampleFile);
          setScannedFilesCount((c) => c + Math.floor(Math.random() * 85 + 40));
          return next;
        });
      }, 120);
    }
    return () => clearInterval(timer);
  }, [scanning, scanType]);

  const startScan = (type: 'quick' | 'full' | 'memory') => {
    setScanType(type);
    setScanProgress(0);
    setScannedFilesCount(0);
    setScanning(true);
    setActiveTab('scan');
  };

  const toggleFirewallRule = (id: number) => {
    setFirewallRules((prev) =>
      prev.map((r) =>
        r.id === id ? { ...r, action: r.action === 'ALLOW' ? 'DROP' : 'ALLOW' } : r
      )
    );
  };

  const rotateSessionKeys = () => {
    const hex = Array.from({ length: 8 }, () => Math.floor(Math.random() * 256).toString(16).padStart(2, '0')).join('');
    setSessionToken(`0x${hex.toUpperCase().slice(0, 4)}...${hex.toUpperCase().slice(12, 16)}`);
    setBlockedHijacks((prev) => prev + 1);
  };

  const handleThreatAction = (id: string, action: 'clean' | 'delete') => {
    if (action === 'clean') {
      setThreats((prev) => prev.map((t) => t.id === id ? { ...t, status: 'Cleaned' } : t));
    } else {
      setThreats((prev) => prev.filter((t) => t.id !== id));
    }
  };

  return (
    <div className="flex h-full w-full bg-[#0B1220] text-slate-100 select-none overflow-hidden">
      {/* Sidebar Navigation */}
      <div className="w-64 bg-[#080E1A] border-r border-slate-800 p-4 flex flex-col justify-between">
        <div className="space-y-6">
          <div className="flex items-center gap-3 px-2 py-1">
            <div className="w-9 h-9 rounded-xl bg-gradient-to-tr from-blue-600 to-cyan-400 flex items-center justify-center shadow-lg shadow-blue-500/20">
              <ShieldCheck className="w-5 h-5 text-white" />
            </div>
            <div>
              <h2 className="text-sm font-bold text-slate-100 tracking-tight">Windows Defender</h2>
              <p className="text-[10px] text-emerald-400 font-semibold flex items-center gap-1">
                <span className="w-1.5 h-1.5 rounded-full bg-emerald-400 animate-pulse"></span>
                Real-time Guard Active
              </p>
            </div>
          </div>

          <nav className="space-y-1">
            {[
              { id: 'overview', label: 'Security Dashboard', icon: Shield },
              { id: 'scan', label: 'Virus & Threat Scan', icon: Zap },
              { id: 'protection', label: 'Protection Settings', icon: Lock },
              { id: 'firewall', label: 'Firewall & Network', icon: Filter },
              { id: 'threats', label: 'Threat Quarantine', icon: AlertTriangle, badge: threats.filter(t => t.status === 'Quarantined').length },
              { id: 'kernel', label: 'Kernel SMEP / Guard', icon: Cpu },
            ].map((tab) => {
              const Icon = tab.icon;
              const isActive = activeTab === tab.id;
              return (
                <button
                  key={tab.id}
                  onClick={() => setActiveTab(tab.id as any)}
                  className={`w-full flex items-center justify-between px-3 py-2.5 rounded-lg text-xs font-medium transition-all ${
                    isActive
                      ? 'bg-blue-600 text-white shadow-md shadow-blue-600/30'
                      : 'text-slate-400 hover:text-slate-200 hover:bg-slate-800/60'
                  }`}
                >
                  <div className="flex items-center gap-2.5">
                    <Icon className={`w-4 h-4 ${isActive ? 'text-white' : 'text-slate-400'}`} />
                    <span>{tab.label}</span>
                  </div>
                  {tab.badge !== undefined && tab.badge > 0 && (
                    <span className="px-1.5 py-0.5 text-[10px] rounded-full bg-red-500/80 text-white font-bold">
                      {tab.badge}
                    </span>
                  )}
                </button>
              );
            })}
          </nav>
        </div>

        {/* Engine Build Stamp */}
        <div className="p-3 bg-slate-900/80 border border-slate-800/80 rounded-xl">
          <div className="text-[10px] text-slate-400">Defender Antivirus Core</div>
          <div className="text-xs font-semibold text-slate-200">v19.4.2026.09 (Ring 0)</div>
          <div className="text-[10px] text-cyan-400 mt-1">Signatures: Up to date</div>
        </div>
      </div>

      {/* Main Content Pane */}
      <div className="flex-1 p-6 overflow-y-auto space-y-6">
        {/* TAB 1: OVERVIEW */}
        {activeTab === 'overview' && (
          <div className="space-y-6">
            {/* Status Hero Card */}
            <div className="p-6 bg-gradient-to-r from-blue-950/60 via-slate-900/80 to-slate-900/60 border border-blue-500/30 rounded-2xl flex items-center justify-between">
              <div className="space-y-2">
                <div className="flex items-center gap-2">
                  <CheckCircle className="w-6 h-6 text-emerald-400" />
                  <h3 className="text-lg font-bold text-white">Your PC is actively protected</h3>
                </div>
                <p className="text-xs text-slate-300 max-w-lg">
                  Xenithra Defender and Kernel Session Guard are operating without threat interference. No unauthorized ring elevation detected.
                </p>
                <div className="text-[11px] text-slate-400 pt-1">Last scan: {lastScanTime}</div>
              </div>
              <div className="flex gap-3">
                <button 
                  onClick={() => startScan('quick')}
                  className="px-5 py-2.5 bg-blue-600 hover:bg-blue-500 font-semibold rounded-xl text-xs flex items-center gap-2 shadow-lg shadow-blue-600/30 transition-all"
                >
                  <Zap className="w-4 h-4" />
                  Quick Scan Now
                </button>
              </div>
            </div>

            {/* Quick Metrics Grid */}
            <div className="grid grid-cols-3 gap-4">
              <div className="p-4 bg-slate-900/70 border border-slate-800 rounded-xl space-y-2">
                <div className="flex justify-between items-center text-xs text-slate-400">
                  <span>Virus & Threat Shield</span>
                  <ShieldCheck className="w-4 h-4 text-emerald-400" />
                </div>
                <div className="text-lg font-bold text-emerald-400">No active threats</div>
                <div className="text-[11px] text-slate-400">Cloud protection is on</div>
              </div>

              <div className="p-4 bg-slate-900/70 border border-slate-800 rounded-xl space-y-2">
                <div className="flex justify-between items-center text-xs text-slate-400">
                  <span>Firewall Packet Filter</span>
                  <Filter className="w-4 h-4 text-cyan-400" />
                </div>
                <div className="text-lg font-bold text-cyan-400">{firewallRules.filter(r => r.enabled).length} Rules Enforced</div>
                <div className="text-[11px] text-slate-400">Drop rules blocking probes</div>
              </div>

              <div className="p-4 bg-slate-900/70 border border-slate-800 rounded-xl space-y-2">
                <div className="flex justify-between items-center text-xs text-slate-400">
                  <span>Kernel Anti-Hijack</span>
                  <Cpu className="w-4 h-4 text-purple-400" />
                </div>
                <div className="text-lg font-bold text-purple-300">SMEP / SMAP Locked</div>
                <div className="text-[11px] text-slate-400">128-bit Token: {sessionToken}</div>
              </div>
            </div>

            {/* Scan Launchers */}
            <div className="p-5 bg-slate-900/50 border border-slate-800/80 rounded-2xl space-y-4">
              <h4 className="text-sm font-bold text-slate-200">On-Demand Security Scans</h4>
              <div className="grid grid-cols-3 gap-4">
                <div 
                  onClick={() => startScan('quick')}
                  className="p-4 bg-slate-800/40 hover:bg-slate-800/80 border border-slate-700/60 rounded-xl cursor-pointer transition-all space-y-2"
                >
                  <div className="flex items-center gap-2 text-cyan-400 font-semibold text-xs">
                    <Zap className="w-4 h-4" />
                    Quick Scan
                  </div>
                  <p className="text-[11px] text-slate-400">Scans memory, startup registry, and active user executable paths.</p>
                  <span className="text-[10px] text-blue-400 font-medium">~30 seconds</span>
                </div>

                <div 
                  onClick={() => startScan('full')}
                  className="p-4 bg-slate-800/40 hover:bg-slate-800/80 border border-slate-700/60 rounded-xl cursor-pointer transition-all space-y-2"
                >
                  <div className="flex items-center gap-2 text-blue-400 font-semibold text-xs">
                    <HardDrive className="w-4 h-4" />
                    Full System Scan
                  </div>
                  <p className="text-[11px] text-slate-400">Inspects all NVMe sectors, ESP partition, and kernel modules.</p>
                  <span className="text-[10px] text-blue-400 font-medium">~2 minutes</span>
                </div>

                <div 
                  onClick={() => startScan('memory')}
                  className="p-4 bg-slate-800/40 hover:bg-slate-800/80 border border-slate-700/60 rounded-xl cursor-pointer transition-all space-y-2"
                >
                  <div className="flex items-center gap-2 text-purple-400 font-semibold text-xs">
                    <Cpu className="w-4 h-4" />
                    Kernel Memory Scan
                  </div>
                  <p className="text-[11px] text-slate-400">Scans higher-half paging tables, IDT, and GDT integrity.</p>
                  <span className="text-[10px] text-purple-400 font-medium">Deep Ring 0</span>
                </div>
              </div>
            </div>
          </div>
        )}

        {/* TAB 2: SCAN PROGRESS */}
        {activeTab === 'scan' && (
          <div className="space-y-6">
            <div className="p-6 bg-slate-900/80 border border-slate-800 rounded-2xl space-y-4">
              <div className="flex justify-between items-center">
                <div className="flex items-center gap-3">
                  <div className={`p-2.5 rounded-xl ${scanning ? 'bg-cyan-500/20 text-cyan-400 animate-spin' : 'bg-emerald-500/20 text-emerald-400'}`}>
                    <RefreshCw className="w-5 h-5" />
                  </div>
                  <div>
                    <h3 className="text-base font-bold text-white">
                      {scanning ? `${scanType.toUpperCase()} Scan In Progress...` : 'Scan Finished'}
                    </h3>
                    <p className="text-xs text-slate-400">
                      {scanning ? 'Inspecting system binaries and memory structures' : 'Zero malicious binaries found'}
                    </p>
                  </div>
                </div>
                {scanning && (
                  <button 
                    onClick={() => setScanning(false)} 
                    className="px-4 py-1.5 bg-red-600/80 hover:bg-red-500 text-white rounded-lg text-xs font-semibold"
                  >
                    Cancel Scan
                  </button>
                )}
              </div>

              {/* Progress Bar */}
              <div className="space-y-2">
                <div className="flex justify-between text-xs font-medium">
                  <span className="text-slate-300">Progress</span>
                  <span className="text-cyan-400 font-bold">{scanProgress}%</span>
                </div>
                <div className="w-full bg-slate-800 h-3 rounded-full overflow-hidden">
                  <div 
                    className="h-full bg-gradient-to-r from-blue-600 to-cyan-400 transition-all duration-150"
                    style={{ width: `${scanProgress}%` }}
                  />
                </div>
              </div>

              {/* Live Scanned File Feed */}
              <div className="p-4 bg-[#070D18] border border-slate-800/80 rounded-xl space-y-2">
                <div className="text-[11px] text-slate-400">Currently scanning:</div>
                <div className="text-xs font-mono text-cyan-300 truncate">
                  {currentScanFile || 'C:\\Windows\\System32\\ntoskrnl.exe'}
                </div>
                <div className="flex justify-between text-[11px] text-slate-400 pt-2 border-t border-slate-800">
                  <span>Objects Scanned: <strong className="text-white">{scannedFilesCount.toLocaleString()}</strong></span>
                  <span>Estimated Time: <strong className="text-white">{scanning ? '12s remaining' : 'Completed'}</strong></span>
                </div>
              </div>
            </div>
          </div>
        )}

        {/* TAB 3: PROTECTION SETTINGS */}
        {activeTab === 'protection' && (
          <div className="space-y-4">
            <h3 className="text-base font-bold text-white mb-2">Real-Time Threat Protection Policies</h3>
            
            {[
              { 
                title: 'Real-Time Protection', 
                desc: 'Locates and stops malware from running or installing on your device.', 
                state: realtimeShield, 
                toggle: () => setRealtimeShield(!realtimeShield) 
              },
              { 
                title: 'Cloud-Delivered Protection', 
                desc: 'Provides increased, faster protection with access to the latest Defender cloud intelligence.', 
                state: cloudProtection, 
                toggle: () => setCloudProtection(!cloudProtection) 
              },
              { 
                title: 'Tamper Protection', 
                desc: 'Prevents unauthorized apps and kernel hooks from changing security settings.', 
                state: tamperProtection, 
                toggle: () => setTamperProtection(!tamperProtection) 
              },
              { 
                title: 'Ransomware / Controlled Folder Access', 
                desc: 'Protects documents and sector images from unauthorized encryption locks.', 
                state: ransomwareShield, 
                toggle: () => setRansomwareShield(!ransomwareShield) 
              },
              { 
                title: 'Kernel Session Guard & Anti-Hijack', 
                desc: 'Rotates 128-bit session entropy and validates user token authenticity on every syscall.', 
                state: sessionGuard, 
                toggle: () => setSessionGuard(!sessionGuard) 
              },
            ].map((item, idx) => (
              <div key={idx} className="p-4 bg-slate-900/70 border border-slate-800 rounded-xl flex items-center justify-between">
                <div className="space-y-1 max-w-xl">
                  <div className="text-xs font-bold text-slate-200">{item.title}</div>
                  <div className="text-[11px] text-slate-400">{item.desc}</div>
                </div>
                <button
                  onClick={item.toggle}
                  className={`w-12 h-6 flex items-center rounded-full p-1 transition-colors ${
                    item.state ? 'bg-blue-600 justify-end' : 'bg-slate-700 justify-start'
                  }`}
                >
                  <div className="w-4 h-4 rounded-full bg-white shadow-md"></div>
                </button>
              </div>
            ))}
          </div>
        )}

        {/* TAB 4: FIREWALL & NETWORK */}
        {activeTab === 'firewall' && (
          <div className="space-y-4">
            <div className="flex justify-between items-center">
              <div>
                <h3 className="text-base font-bold text-white">Kernel Private Firewall & Packet Filter</h3>
                <p className="text-xs text-slate-400">Stateful deep-packet inspection enforced before routing to V8 / Browser layer.</p>
              </div>
              <span className="px-3 py-1 bg-emerald-500/20 text-emerald-400 border border-emerald-500/40 rounded-full text-xs font-semibold">
                Firewall: ENFORCING
              </span>
            </div>

            <div className="border border-slate-800 rounded-xl overflow-hidden text-xs">
              <div className="grid grid-cols-[80px_80px_100px_1fr_100px_100px] bg-[#101827] p-3 font-bold text-slate-400">
                <span>Rule ID</span>
                <span>Protocol</span>
                <span>Port</span>
                <span>Traffic Description</span>
                <span>Packets</span>
                <span>Action</span>
              </div>
              {firewallRules.map((rule) => (
                <div 
                  key={rule.id} 
                  className="grid grid-cols-[80px_80px_100px_1fr_100px_100px] p-3 border-t border-slate-800/80 bg-[#0C1322] hover:bg-slate-800/50 items-center"
                >
                  <span className="font-mono text-slate-400">#{rule.id}</span>
                  <span className="font-semibold text-cyan-400">{rule.protocol}</span>
                  <span className="text-slate-300 font-mono">{rule.port === 0 ? 'ALL' : rule.port}</span>
                  <span className="text-slate-200">{rule.description}</span>
                  <span className="text-slate-400">{rule.packetCount.toLocaleString()} pkts</span>
                  <div>
                    <button
                      onClick={() => toggleFirewallRule(rule.id)}
                      className={`px-3 py-1 rounded text-xs font-bold transition-all ${
                        rule.action === 'ALLOW'
                          ? 'bg-emerald-600/30 text-emerald-400 border border-emerald-500/50 hover:bg-emerald-600/50'
                          : 'bg-red-600/30 text-red-400 border border-red-500/50 hover:bg-red-600/50'
                      }`}
                    >
                      {rule.action}
                    </button>
                  </div>
                </div>
              ))}
            </div>
          </div>
        )}

        {/* TAB 5: THREAT QUARANTINE */}
        {activeTab === 'threats' && (
          <div className="space-y-4">
            <div className="flex justify-between items-center">
              <div>
                <h3 className="text-base font-bold text-white">Threat Protection History & Quarantine</h3>
                <p className="text-xs text-slate-400">Isolated threats and malicious memory injectors blocked by kernel guard.</p>
              </div>
              <button 
                onClick={() => setThreats([])}
                className="px-3 py-1.5 bg-slate-800 hover:bg-slate-700 text-xs text-slate-300 rounded-lg flex items-center gap-1.5"
              >
                <Trash2 className="w-3.5 h-3.5" />
                Clear Cleaned Log
              </button>
            </div>

            {threats.length === 0 ? (
              <div className="p-12 text-center text-slate-400 bg-slate-900/40 rounded-xl border border-slate-800">
                <CheckCircle className="w-8 h-8 text-emerald-400 mx-auto mb-2" />
                <p className="text-sm font-medium">Quarantine is empty. No threats detected.</p>
              </div>
            ) : (
              <div className="space-y-3">
                {threats.map((threat) => (
                  <div key={threat.id} className="p-4 bg-slate-900/80 border border-slate-800 rounded-xl space-y-3">
                    <div className="flex justify-between items-start">
                      <div>
                        <div className="flex items-center gap-2">
                          <span className="font-bold text-sm text-red-400">{threat.name}</span>
                          <span className="px-2 py-0.5 rounded text-[10px] font-bold bg-red-950 text-red-400 border border-red-800">
                            {threat.severity}
                          </span>
                          <span className="px-2 py-0.5 rounded text-[10px] font-medium bg-slate-800 text-slate-300">
                            {threat.category}
                          </span>
                        </div>
                        <div className="text-xs font-mono text-slate-400 mt-1">{threat.affectedFile}</div>
                      </div>
                      <span className={`px-2 py-1 rounded text-xs font-semibold ${
                        threat.status === 'Quarantined' ? 'bg-amber-500/20 text-amber-400' : 'bg-emerald-500/20 text-emerald-400'
                      }`}>
                        {threat.status}
                      </span>
                    </div>

                    <div className="flex justify-between items-center pt-2 border-t border-slate-800/80 text-[11px] text-slate-400">
                      <span>Detected: {threat.dateDetected}</span>
                      <div className="flex gap-2">
                        {threat.status === 'Quarantined' && (
                          <button 
                            onClick={() => handleThreatAction(threat.id, 'clean')}
                            className="px-3 py-1 bg-blue-600 hover:bg-blue-500 text-white rounded text-xs font-medium"
                          >
                            Clean & Disinfect
                          </button>
                        )}
                        <button 
                          onClick={() => handleThreatAction(threat.id, 'delete')}
                          className="px-3 py-1 bg-red-600/80 hover:bg-red-500 text-white rounded text-xs font-medium"
                        >
                          Permanent Delete
                        </button>
                      </div>
                    </div>
                  </div>
                ))}
              </div>
            )}
          </div>
        )}

        {/* TAB 6: KERNEL SMEP / GUARD */}
        {activeTab === 'kernel' && (
          <div className="space-y-4">
            <h3 className="text-base font-bold text-white">Ring 0 Security & Hardware Execution Guard</h3>
            
            <div className="grid grid-cols-2 gap-4">
              <div className="p-4 bg-slate-900/80 border border-purple-500/30 rounded-xl space-y-3">
                <div className="flex items-center gap-2 text-purple-300 font-bold text-xs">
                  <Cpu className="w-4 h-4" />
                  Hardware SMEP / SMAP CPU Bit
                </div>
                <p className="text-xs text-slate-300">
                  CR4.SMEP (Supervisor Mode Execution Prevention) blocks user-space memory code execution in Ring 0.
                </p>
                <div className="p-2.5 bg-black/40 rounded font-mono text-[11px] text-emerald-400">
                  CR4 Register: 0x00000000001406E0 [SMEP=1, SMAP=1]
                </div>
              </div>

              <div className="p-4 bg-slate-900/80 border border-blue-500/30 rounded-xl space-y-3">
                <div className="flex items-center gap-2 text-cyan-300 font-bold text-xs">
                  <Lock className="w-4 h-4" />
                  Session Key Dynamic Rotation
                </div>
                <p className="text-xs text-slate-300">
                  128-bit randomized cryptographic tokens prevent cross-process token replay and session hijack attacks.
                </p>
                <div className="flex items-center justify-between">
                  <span className="font-mono text-xs text-yellow-300">{sessionToken}</span>
                  <button 
                    onClick={rotateSessionKeys}
                    className="px-3 py-1 bg-blue-600 hover:bg-blue-500 text-white rounded text-xs font-medium flex items-center gap-1.5"
                  >
                    <RefreshCw className="w-3.5 h-3.5" />
                    Rotate Token
                  </button>
                </div>
              </div>
            </div>

            <div className="p-4 bg-slate-900/70 border border-slate-800 rounded-xl space-y-2">
              <h4 className="text-xs font-bold text-slate-200">Security Telemetry Metrics</h4>
              <div className="grid grid-cols-3 gap-4 pt-2">
                <div className="p-3 bg-slate-800/40 rounded-lg">
                  <div className="text-[11px] text-slate-400">Active System Sessions</div>
                  <div className="text-lg font-bold text-white mt-1">{activeSessions}</div>
                </div>
                <div className="p-3 bg-slate-800/40 rounded-lg">
                  <div className="text-[11px] text-slate-400">Blocked Hijack Injections</div>
                  <div className="text-lg font-bold text-emerald-400 mt-1">{blockedHijacks}</div>
                </div>
                <div className="p-3 bg-slate-800/40 rounded-lg">
                  <div className="text-[11px] text-slate-400">Entropy Pool Health</div>
                  <div className="text-lg font-bold text-cyan-400 mt-1">{entropyHealth}%</div>
                </div>
              </div>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};
