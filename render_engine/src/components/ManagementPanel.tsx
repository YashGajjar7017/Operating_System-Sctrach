/**
 * @file ManagementPanel.tsx — Xenithra OS v3.0 — System Management Panel
 *
 * Comprehensive system management:
 *   - Device Manager (PnP devices, driver info, status)
 *   - Performance Monitor (CPU, RAM, GPU, Disk, Network graphs)
 *   - Event Log viewer with filtering
 *   - Installed Programs list
 *   - Startup Manager
 *   - System Information
 */

import React, { useState, useEffect, useRef, useCallback } from 'react';
import {
  Cpu, HardDrive, Monitor, Layers, Activity, List, Package,
  Zap, AlertTriangle, CheckCircle, XCircle, Info, RefreshCw,
  ChevronRight, ChevronDown, Trash2, Power, ToggleLeft, ToggleRight,
  Clock, Shield, Terminal, Search, Filter, Download, MemoryStick,
  Database, Gauge, Thermometer
} from 'lucide-react';
import type {
  PnpDevice, PerformanceSnapshot, EventLogEntry, InstalledProgram, StartupEntry
} from '../types';

// ── Mock Data ─────────────────────────────────────────────────────────────────

const MOCK_DEVICES: PnpDevice[] = [
  { id: 'd1', instanceId: 'PCI\\VEN_8086&DEV_10D3', name: 'Intel(R) 82574L Gigabit Network Connection', description: 'Ethernet Adapter', manufacturer: 'Intel Corporation', deviceClass: 'Network Adapter', status: 'OK', driverName: 'e1000e', driverVersion: '3.8.7.0', driverDate: '2024-01-15', driverSigned: true, hardwareId: 'PCI\\VEN_8086&DEV_10D3', location: 'PCI Slot 1', irq: 10, ioPort: '0xD000-0xD01F' },
  { id: 'd2', instanceId: 'PCI\\VEN_10EC&DEV_8139', name: 'Realtek RTL8139 Fast Ethernet NIC', description: 'Network Controller', manufacturer: 'Realtek Semiconductor', deviceClass: 'Network Adapter', status: 'OK', driverName: 'rtl8139', driverVersion: '1.2.0', driverDate: '2024-03-22', driverSigned: true, hardwareId: 'PCI\\VEN_10EC&DEV_8139', location: 'PCI Slot 2', irq: 11 },
  { id: 'd3', instanceId: 'PCI\\VEN_1234&DEV_1111', name: 'QEMU / KVM Virtual GPU', description: 'Display Adapter', manufacturer: 'QEMU Project', deviceClass: 'Display', status: 'OK', driverName: 'vbe', driverVersion: '1.0.0', driverDate: '2024-06-01', driverSigned: false, hardwareId: 'PCI\\VEN_1234&DEV_1111', location: 'PCI Slot 0', memRange: '0xFD000000-0xFDFFFFFF' },
  { id: 'd4', instanceId: 'USB\\ROOT_HUB30', name: 'Xenithra xHCI USB 3.0 Host Controller', description: 'USB Root Hub', manufacturer: 'Xenithra OS', deviceClass: 'USB Controller', status: 'OK', driverName: 'xhci_hcd', driverVersion: '1.0.0', driverDate: '2024-09-01', driverSigned: true, hardwareId: 'USB\\ROOT_HUB30', location: 'USB Root Hub' },
  { id: 'd5', instanceId: 'ACPI\\PNP0303', name: 'PS/2 Standard Keyboard', description: 'Keyboard', manufacturer: 'Standard', deviceClass: 'Keyboard', status: 'OK', driverName: 'ps2kbd', driverVersion: '1.0.0', driverDate: '2024-01-01', driverSigned: true, hardwareId: 'ACPI\\PNP0303', location: 'I/O Port 0x60', irq: 1, ioPort: '0x60, 0x64' },
  { id: 'd6', instanceId: 'ACPI\\PNP0F03', name: 'PS/2 Compatible Mouse', description: 'Mouse', manufacturer: 'Standard', deviceClass: 'Mouse', status: 'OK', driverName: 'ps2mouse', driverVersion: '1.0.0', driverDate: '2024-01-01', driverSigned: true, hardwareId: 'ACPI\\PNP0F03', location: 'I/O Port 0x60', irq: 12 },
  { id: 'd7', instanceId: 'PCI\\VEN_8086&DEV_27D8', name: 'Intel High Definition Audio Controller', description: 'Audio Controller', manufacturer: 'Intel Corporation', deviceClass: 'Sound', status: 'OK', driverName: 'snd_hda_intel', driverVersion: '2.4.0', driverDate: '2024-05-10', driverSigned: true, hardwareId: 'PCI\\VEN_8086&DEV_27D8', location: 'PCI Slot 3', irq: 5 },
  { id: 'd8', instanceId: 'PCI\\VEN_8086&DEV_2918', name: 'Intel PIIX3 ACPI Controller', description: 'System Device', manufacturer: 'Intel Corporation', deviceClass: 'System', status: 'OK', driverName: 'acpi', driverVersion: '1.0.0', driverDate: '2024-09-01', driverSigned: true, hardwareId: 'PCI\\VEN_8086&DEV_2918', location: 'Mainboard' },
  { id: 'd9', instanceId: 'STORAGE\\SATA_0', name: 'QEMU HARDDISK (SATA)', description: 'Disk Drive', manufacturer: 'QEMU', deviceClass: 'Disk Drive', status: 'OK', driverName: 'ahci', driverVersion: '3.0.0', driverDate: '2024-01-01', driverSigned: true, hardwareId: 'SCSI\\DISK&VEN_ATA', location: 'SATA Port 0' },
];

const MOCK_STARTUP: StartupEntry[] = [
  { id: 's1', name: 'Xenithra Security Guard', publisher: 'Xenithra OS', command: 'C:\\System\\security\\defender.exe --background', location: 'Registry', enabled: true, impact: 'Low', lastRunTime: '2026-09-30 07:00:00' },
  { id: 's2', name: 'Network Manager',         publisher: 'Xenithra OS', command: 'C:\\System\\services\\netmgr.exe',                  location: 'Registry', enabled: true, impact: 'Low' },
  { id: 's3', name: 'Driver Manager',          publisher: 'Xenithra OS', command: 'C:\\System\\services\\drvmgr.exe',                  location: 'Registry', enabled: true, impact: 'Low' },
  { id: 's4', name: 'XenithraUpdateService',   publisher: 'Xenithra OS', command: 'C:\\System\\updater\\xupdate.exe --tray',           location: 'Task Scheduler', enabled: true, impact: 'Medium' },
  { id: 's5', name: 'Sample Startup App',      publisher: 'Unknown',     command: 'C:\\Users\\User\\AppData\\Roaming\\demo.exe',      location: 'Startup Folder', enabled: false, impact: 'High' },
];

const MOCK_PROGRAMS: InstalledProgram[] = [
  { id: 'p1', name: 'Xenithra Browser',     version: '3.0.0', publisher: 'Xenithra OS', installDate: '2026-09-01', installLocation: 'C:\\Programs\\Browser',     sizeMb: 124 },
  { id: 'p2', name: 'Xenithra Code Editor', version: '2.1.0', publisher: 'Xenithra OS', installDate: '2026-09-01', installLocation: 'C:\\Programs\\Editor',       sizeMb: 87  },
  { id: 'p3', name: 'Xenithra Media Player',version: '1.5.0', publisher: 'Xenithra OS', installDate: '2026-09-01', installLocation: 'C:\\Programs\\Media',        sizeMb: 52  },
  { id: 'p4', name: 'Xenithra App Store',   version: '3.0.0', publisher: 'Xenithra OS', installDate: '2026-09-01', installLocation: 'C:\\Programs\\Store',        sizeMb: 38  },
  { id: 'p5', name: 'Python 3.12',          version: '3.12.0',publisher: 'Python Foundation', installDate: '2026-08-15', installLocation: 'C:\\Programs\\Python', sizeMb: 168 },
];

const MOCK_EVENTS: EventLogEntry[] = [
  { id: 1, timestamp: '09:30:15', level: 'Information', source: 'KernelSecurity', eventId: 1001, category: 'Security', message: 'Session guard audit passed — no anomalies detected.' },
  { id: 2, timestamp: '09:29:52', level: 'Information', source: 'NetworkManager',  eventId: 2001, category: 'Network',   message: 'eth0 connected — IP: 192.168.1.42, GW: 192.168.1.1' },
  { id: 3, timestamp: '09:29:44', level: 'Information', source: 'DriverManager',   eventId: 3001, category: 'System',    message: 'Intel e1000 NIC driver loaded successfully (IRQ 10).' },
  { id: 4, timestamp: '09:29:40', level: 'Information', source: 'SMSS',            eventId: 4001, category: 'System',    message: 'Session 1 initialized — userinit and winlogon started.' },
  { id: 5, timestamp: '09:29:38', level: 'Information', source: 'AudioSrv',        eventId: 5001, category: 'System',    message: 'Intel HDA audio driver initialized at IRQ 5.' },
  { id: 6, timestamp: '09:29:35', level: 'Warning',     source: 'Firewall',        eventId: 6001, category: 'Security', message: 'Blocked 3 inbound probe attempts on port 23 (Telnet).' },
  { id: 7, timestamp: '09:29:30', level: 'Information', source: 'Kernel',          eventId: 7001, category: 'System',    message: 'APIC timer calibrated: 1000 µs/tick. SMP: 2 cores active.' },
  { id: 8, timestamp: '09:29:28', level: 'Information', source: 'VMM',             eventId: 8001, category: 'System',    message: 'Physical memory: 2048 MB. Kernel heap: 64 MB initialized.' },
  { id: 9, timestamp: '09:29:25', level: 'Critical',    source: 'KernelSecurity',  eventId: 9001, category: 'Security', message: 'SMEP/SMAP enforced on all CPUs — ring-3 exploit mitigation active.' },
  { id: 10,timestamp: '09:29:20', level: 'Information', source: 'Bootloader',      eventId: 9999, category: 'System',    message: 'UEFI handoff complete. Kernel entry at 0xFFFFFFFF80000000.' },
];

// ── Perf graph generator ──────────────────────────────────────────────────────
function generatePerfHistory(count = 40, base = 40, variance = 20): number[] {
  return Array.from({ length: count }, () => Math.min(100, Math.max(1, base + (Math.random() - 0.5) * variance * 2)));
}

type Tab = 'devices' | 'performance' | 'events' | 'programs' | 'startup' | 'sysinfo';

// ── Perf Graph Component ──────────────────────────────────────────────────────
const PerfGraph: React.FC<{ data: number[]; color: string; label: string; value: string; unit?: string }> =
  ({ data, color, label, value, unit = '%' }) => {
  const ref = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const c = ref.current;
    if (!c) return;
    const ctx = c.getContext('2d');
    if (!ctx) return;
    const W = c.width, H = c.height;
    ctx.clearRect(0, 0, W, H);
    const max = 100;
    const grad = ctx.createLinearGradient(0, 0, 0, H);
    grad.addColorStop(0, color.replace(')', ', 0.4)').replace('rgb(', 'rgba(').replace('#', 'rgba(').replace(/^(.+)$/, (s) => {
      if (s.startsWith('#')) { const r = parseInt(s.slice(1,3),16), g = parseInt(s.slice(3,5),16), b = parseInt(s.slice(5,7),16); return `rgba(${r},${g},${b},0.35)`; }
      return s;
    }));
    grad.addColorStop(1, 'rgba(0,0,0,0)');
    ctx.beginPath();
    data.forEach((v, i) => {
      const x = (i / (data.length - 1)) * W;
      const y = H - (v / max) * (H - 4) - 2;
      i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
    });
    ctx.lineTo(W, H); ctx.lineTo(0, H); ctx.closePath();
    ctx.fillStyle = grad; ctx.fill();
    ctx.beginPath();
    data.forEach((v, i) => {
      const x = (i / (data.length - 1)) * W;
      const y = H - (v / max) * (H - 4) - 2;
      i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
    });
    ctx.strokeStyle = color; ctx.lineWidth = 1.5; ctx.lineJoin = 'round'; ctx.stroke();
  }, [data, color]);

  return (
    <div style={{ background: 'rgba(255,255,255,0.03)', borderRadius: 'var(--radius-md)', border: '1px solid var(--border-subtle)', padding: '10px 12px' }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: 6 }}>
        <span style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)' }}>{label}</span>
        <span style={{ fontSize: 'var(--text-sm)', fontWeight: 600, color }}>{value}{unit}</span>
      </div>
      <canvas ref={ref} width={320} height={48} style={{ width: '100%', height: 48 }} />
    </div>
  );
};

// ── Main Component ────────────────────────────────────────────────────────────
export const ManagementPanel: React.FC = () => {
  const [tab, setTab] = useState<Tab>('devices');
  const [devices] = useState<PnpDevice[]>(MOCK_DEVICES);
  const [startupEntries, setStartupEntries] = useState<StartupEntry[]>(MOCK_STARTUP);
  const [programs] = useState<InstalledProgram[]>(MOCK_PROGRAMS);
  const [events] = useState<EventLogEntry[]>(MOCK_EVENTS);
  const [expandedDevice, setExpandedDevice] = useState<string | null>(null);
  const [searchQuery, setSearchQuery] = useState('');
  const [eventFilter, setEventFilter] = useState<'all' | 'Information' | 'Warning' | 'Error' | 'Critical'>('all');

  // Simulated live perf data
  const [cpuData, setCpuData] = useState(() => generatePerfHistory(40, 45, 25));
  const [ramData, setRamData] = useState(() => generatePerfHistory(40, 60, 10));
  const [gpuData, setGpuData] = useState(() => generatePerfHistory(40, 30, 20));
  const [diskData, setDiskData] = useState(() => generatePerfHistory(40, 15, 15));
  const [cpuVal, setCpuVal] = useState(48);
  const [ramVal, setRamVal] = useState(62);
  const [gpuVal, setGpuVal] = useState(28);
  const [diskVal, setDiskVal] = useState(14);

  useEffect(() => {
    const iv = setInterval(() => {
      const nextCpu = Math.min(100, Math.max(1, cpuVal + (Math.random() - 0.5) * 14));
      const nextRam = Math.min(95,  Math.max(40, ramVal + (Math.random() - 0.5) * 4));
      const nextGpu = Math.min(100, Math.max(1, gpuVal + (Math.random() - 0.5) * 12));
      const nextDisk = Math.min(100, Math.max(0, diskVal + (Math.random() - 0.5) * 10));
      setCpuVal(nextCpu); setRamVal(nextRam); setGpuVal(nextGpu); setDiskVal(nextDisk);
      setCpuData(prev => [...prev.slice(1), nextCpu]);
      setRamData(prev => [...prev.slice(1), nextRam]);
      setGpuData(prev => [...prev.slice(1), nextGpu]);
      setDiskData(prev => [...prev.slice(1), nextDisk]);
    }, 1000);
    return () => clearInterval(iv);
  }, [cpuVal, ramVal, gpuVal, diskVal]);

  const toggleStartup = useCallback((id: string) => {
    setStartupEntries(prev => prev.map(s => s.id === id ? { ...s, enabled: !s.enabled } : s));
  }, []);

  const deviceGroups = devices.reduce<Record<string, PnpDevice[]>>((acc, d) => {
    (acc[d.deviceClass] = acc[d.deviceClass] || []).push(d); return acc;
  }, {});

  const filteredEvents = events.filter(e => eventFilter === 'all' || e.level === eventFilter);

  const levelColor = (level: string) => {
    switch (level) {
      case 'Information': return 'var(--clr-accent-primary)';
      case 'Warning':     return 'var(--clr-accent-warning)';
      case 'Error':
      case 'Critical':    return 'var(--clr-accent-danger)';
      default:            return 'var(--text-tertiary)';
    }
  };

  const statusIcon = (status: string) => {
    if (status === 'OK') return <CheckCircle size={13} style={{ color: 'var(--clr-accent-success)' }} />;
    if (status === 'Error') return <XCircle size={13} style={{ color: 'var(--clr-accent-danger)' }} />;
    if (status === 'Disabled') return <Info size={13} style={{ color: 'var(--text-tertiary)' }} />;
    return <AlertTriangle size={13} style={{ color: 'var(--clr-accent-warning)' }} />;
  };

  const tabs: { id: Tab; label: string; icon: React.ReactNode }[] = [
    { id: 'devices',     label: 'Devices',      icon: <HardDrive size={13} /> },
    { id: 'performance', label: 'Performance',   icon: <Activity size={13} /> },
    { id: 'events',      label: 'Event Log',     icon: <List size={13} /> },
    { id: 'programs',    label: 'Programs',      icon: <Package size={13} /> },
    { id: 'startup',     label: 'Startup',       icon: <Zap size={13} /> },
    { id: 'sysinfo',     label: 'System Info',   icon: <Cpu size={13} /> },
  ];

  return (
    <div style={{ display: 'flex', flexDirection: 'column', height: '100%', fontFamily: 'var(--font-ui)', color: 'var(--text-primary)' }}>

      {/* ── Tab Bar ──────────────────────────────────────────────────────── */}
      <div style={{ display: 'flex', gap: 2, padding: '8px 12px 0', borderBottom: '1px solid var(--border-subtle)', flexShrink: 0, overflowX: 'auto' }}>
        {tabs.map(t => (
          <button key={t.id} onClick={() => setTab(t.id)} style={{
            display: 'flex', alignItems: 'center', gap: 5, padding: '7px 12px 8px',
            borderRadius: '8px 8px 0 0', whiteSpace: 'nowrap',
            background: tab === t.id ? 'rgba(96,165,250,0.12)' : 'transparent',
            color: tab === t.id ? 'var(--clr-accent-primary)' : 'var(--text-secondary)',
            border: 'none',
            borderBottom: tab === t.id ? '2px solid var(--clr-accent-primary)' : '2px solid transparent',
            fontSize: 'var(--text-xs)', fontWeight: tab === t.id ? 600 : 400,
            cursor: 'pointer', transition: 'all 0.15s'
          }}>
            {t.icon} {t.label}
          </button>
        ))}
      </div>

      {/* ── Content ──────────────────────────────────────────────────────── */}
      <div style={{ flex: 1, overflow: 'auto', padding: 14 }}>

        {/* ── DEVICE MANAGER ───────────────────────────────────────────── */}
        {tab === 'devices' && (
          <div style={{ display: 'flex', flexDirection: 'column', gap: 8 }}>
            <div style={{ position: 'relative', marginBottom: 4 }}>
              <Search size={13} style={{ position: 'absolute', left: 10, top: '50%', transform: 'translateY(-50%)', color: 'var(--text-tertiary)' }} />
              <input className="input-surface" placeholder="Search devices…" value={searchQuery}
                onChange={e => setSearchQuery(e.target.value)} style={{ width: '100%', paddingLeft: 30 }} />
            </div>
            {Object.entries(deviceGroups)
              .filter(([cls, devs]) => !searchQuery || cls.toLowerCase().includes(searchQuery.toLowerCase()) || devs.some(d => d.name.toLowerCase().includes(searchQuery.toLowerCase())))
              .map(([cls, devs]) => (
              <div key={cls}>
                <div className="section-header" style={{ display: 'flex', alignItems: 'center', gap: 6 }}>
                  <HardDrive size={11} /> {cls}
                </div>
                <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
                  {devs.filter(d => !searchQuery || d.name.toLowerCase().includes(searchQuery.toLowerCase())).map(dev => (
                    <div key={dev.id}>
                      <div
                        className="glass-card"
                        onClick={() => setExpandedDevice(expandedDevice === dev.id ? null : dev.id)}
                        style={{ padding: '10px 12px', cursor: 'pointer' }}
                      >
                        <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
                          {statusIcon(dev.status)}
                          <div style={{ flex: 1, minWidth: 0 }}>
                            <div style={{ fontSize: 'var(--text-sm)', fontWeight: 500 }} className="text-truncate">{dev.name}</div>
                            <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)' }}>{dev.manufacturer} · {dev.location}</div>
                          </div>
                          <span className={`pill ${dev.driverSigned ? 'pill-green' : 'pill-amber'}`} style={{ fontSize: 10 }}>
                            {dev.driverSigned ? 'Signed' : 'Unsigned'}
                          </span>
                          <ChevronDown size={13} style={{ color: 'var(--text-tertiary)', transform: expandedDevice === dev.id ? 'rotate(180deg)' : 'none', transition: 'transform 0.2s' }} />
                        </div>
                      </div>
                      {expandedDevice === dev.id && (
                        <div style={{ marginLeft: 12, marginTop: 4, padding: '10px 12px', background: 'rgba(255,255,255,0.025)', borderRadius: 'var(--radius-md)', border: '1px solid var(--border-subtle)', fontSize: 'var(--text-xs)', fontFamily: 'var(--font-mono)' }}>
                          <div style={{ display: 'grid', gridTemplateColumns: '120px 1fr', gap: '4px 10px' }}>
                            {[
                              ['Hardware ID', dev.hardwareId], ['Driver', dev.driverName + ' v' + dev.driverVersion],
                              ['Driver Date', dev.driverDate], ['Status', dev.status],
                              ['Location', dev.location],
                              ...(dev.irq != null ? [['IRQ', String(dev.irq)]] : []),
                              ...(dev.ioPort ? [['I/O Port', dev.ioPort]] : []),
                              ...(dev.memRange ? [['Memory Range', dev.memRange]] : []),
                            ].map(([k, v]) => (
                              <React.Fragment key={k}>
                                <span style={{ color: 'var(--text-tertiary)' }}>{k}:</span>
                                <span style={{ color: 'var(--text-primary)' }}>{v}</span>
                              </React.Fragment>
                            ))}
                          </div>
                          <div style={{ display: 'flex', gap: 8, marginTop: 10 }}>
                            <button className="btn btn-secondary" style={{ fontSize: 11, padding: '4px 10px' }}>Update Driver</button>
                            <button className="btn btn-ghost" style={{ fontSize: 11, padding: '4px 10px' }}>Disable</button>
                            <button className="btn btn-ghost" style={{ fontSize: 11, padding: '4px 10px', color: 'var(--clr-accent-danger)' }}>Uninstall</button>
                          </div>
                        </div>
                      )}
                    </div>
                  ))}
                </div>
              </div>
            ))}
          </div>
        )}

        {/* ── PERFORMANCE ──────────────────────────────────────────────── */}
        {tab === 'performance' && (
          <div style={{ display: 'flex', flexDirection: 'column', gap: 10 }}>
            {/* Summary row */}
            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: 8 }}>
              {[
                { label: 'CPU',  val: `${cpuVal.toFixed(0)}%`, sub: '2 cores · 2.4 GHz', color: 'hsl(220,90%,65%)' },
                { label: 'RAM',  val: `${(ramVal/100*2048).toFixed(0)} MB`, sub: '2048 MB total', color: 'hsl(142,70%,55%)' },
                { label: 'GPU',  val: `${gpuVal.toFixed(0)}%`, sub: 'QEMU VGA · 128 MB', color: 'hsl(250,85%,70%)' },
                { label: 'Disk', val: `${diskVal.toFixed(0)}%`, sub: 'SATA · 64 GB', color: 'hsl(38,95%,60%)' },
              ].map(s => (
                <div key={s.label} style={{ background: 'rgba(255,255,255,0.03)', border: '1px solid var(--border-subtle)', borderRadius: 'var(--radius-md)', padding: '12px', textAlign: 'center' }}>
                  <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)', marginBottom: 4 }}>{s.label}</div>
                  <div style={{ fontSize: 'var(--text-2xl)', fontWeight: 700, color: s.color, fontFamily: 'var(--font-mono)' }}>{s.val}</div>
                  <div style={{ fontSize: 10, color: 'var(--text-tertiary)', marginTop: 2 }}>{s.sub}</div>
                </div>
              ))}
            </div>
            <PerfGraph data={cpuData} color="hsl(220,90%,65%)" label="CPU Usage" value={cpuVal.toFixed(0)} />
            <PerfGraph data={ramData} color="hsl(142,70%,55%)" label="RAM Usage" value={(ramVal/100*2048).toFixed(0)} unit=" MB" />
            <PerfGraph data={gpuData} color="hsl(250,85%,70%)" label="GPU Usage" value={gpuVal.toFixed(0)} />
            <PerfGraph data={diskData} color="hsl(38,95%,60%)" label="Disk I/O" value={diskVal.toFixed(0)} />
          </div>
        )}

        {/* ── EVENT LOG ────────────────────────────────────────────────── */}
        {tab === 'events' && (
          <div style={{ display: 'flex', flexDirection: 'column', gap: 10 }}>
            <div style={{ display: 'flex', gap: 6 }}>
              {(['all','Information','Warning','Error','Critical'] as const).map(f => (
                <button key={f} onClick={() => setEventFilter(f)} className={`btn ${eventFilter === f ? 'btn-primary' : 'btn-secondary'}`} style={{ padding: '4px 10px', fontSize: 11 }}>
                  {f === 'all' ? 'All' : f}
                </button>
              ))}
            </div>
            <div style={{ display: 'flex', flexDirection: 'column', gap: 5 }}>
              {filteredEvents.map(e => (
                <div key={e.id} style={{
                  background: 'rgba(255,255,255,0.025)', borderRadius: 'var(--radius-md)',
                  padding: '9px 12px', border: '1px solid var(--border-subtle)',
                  borderLeft: `3px solid ${levelColor(e.level)}`
                }}>
                  <div style={{ display: 'flex', gap: 10, alignItems: 'flex-start' }}>
                    <span style={{ color: 'var(--text-tertiary)', fontFamily: 'var(--font-mono)', fontSize: 11, flexShrink: 0, marginTop: 1 }}>{e.timestamp}</span>
                    <div style={{ flex: 1 }}>
                      <div style={{ display: 'flex', gap: 8, alignItems: 'center', marginBottom: 2 }}>
                        <span style={{ fontWeight: 600, fontSize: 'var(--text-xs)', color: levelColor(e.level) }}>{e.level}</span>
                        <span style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)' }}>{e.source}</span>
                        <span style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)' }}>Event {e.eventId}</span>
                      </div>
                      <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-secondary)' }}>{e.message}</div>
                    </div>
                  </div>
                </div>
              ))}
            </div>
          </div>
        )}

        {/* ── PROGRAMS ────────────────────────────────────────────────── */}
        {tab === 'programs' && (
          <div style={{ display: 'flex', flexDirection: 'column', gap: 8 }}>
            <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)', marginBottom: 4 }}>
              {programs.length} programs installed · Total: {programs.reduce((s, p) => s + p.sizeMb, 0)} MB
            </div>
            {programs.map(p => (
              <div key={p.id} className="glass-card" style={{ padding: '10px 14px' }}>
                <div style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
                  <div style={{ width: 36, height: 36, borderRadius: 'var(--radius-md)', background: 'rgba(96,165,250,0.12)', display: 'flex', alignItems: 'center', justifyContent: 'center', flexShrink: 0 }}>
                    <Package size={16} style={{ color: 'var(--clr-accent-primary)' }} />
                  </div>
                  <div style={{ flex: 1 }}>
                    <div style={{ fontWeight: 600, fontSize: 'var(--text-sm)', marginBottom: 2 }}>{p.name}</div>
                    <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)' }}>{p.publisher} · v{p.version} · {p.sizeMb} MB · {p.installDate}</div>
                  </div>
                  <button className="btn btn-ghost" style={{ padding: '4px', color: 'var(--clr-accent-danger)' }}>
                    <Trash2 size={14} />
                  </button>
                </div>
              </div>
            ))}
          </div>
        )}

        {/* ── STARTUP ──────────────────────────────────────────────────── */}
        {tab === 'startup' && (
          <div style={{ display: 'flex', flexDirection: 'column', gap: 8 }}>
            <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)', marginBottom: 4 }}>
              {startupEntries.filter(s => s.enabled).length} of {startupEntries.length} entries enabled
            </div>
            {startupEntries.map(s => (
              <div key={s.id} className="glass-card" style={{ padding: '10px 14px', opacity: s.enabled ? 1 : 0.55 }}>
                <div style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
                  <div style={{ width: 36, height: 36, borderRadius: 'var(--radius-md)', background: s.enabled ? 'rgba(96,165,250,0.12)' : 'rgba(255,255,255,0.05)', display: 'flex', alignItems: 'center', justifyContent: 'center', flexShrink: 0 }}>
                    <Zap size={16} style={{ color: s.enabled ? 'var(--clr-accent-primary)' : 'var(--text-tertiary)' }} />
                  </div>
                  <div style={{ flex: 1, minWidth: 0 }}>
                    <div style={{ display: 'flex', alignItems: 'center', gap: 8, marginBottom: 2 }}>
                      <span style={{ fontWeight: 600, fontSize: 'var(--text-sm)' }}>{s.name}</span>
                      <span className={`pill ${s.impact === 'Low' ? 'pill-green' : s.impact === 'Medium' ? 'pill-amber' : 'pill-red'}`} style={{ fontSize: 10 }}>
                        {s.impact} Impact
                      </span>
                    </div>
                    <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)', overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{s.publisher} · {s.location}</div>
                  </div>
                  <button onClick={() => toggleStartup(s.id)} style={{ background: 'none', border: 'none', cursor: 'pointer', color: s.enabled ? 'var(--clr-accent-primary)' : 'var(--text-tertiary)', padding: 4 }}>
                    {s.enabled ? <ToggleRight size={22} /> : <ToggleLeft size={22} />}
                  </button>
                </div>
              </div>
            ))}
          </div>
        )}

        {/* ── SYSTEM INFO ───────────────────────────────────────────────── */}
        {tab === 'sysinfo' && (
          <div style={{ display: 'flex', flexDirection: 'column', gap: 14 }}>
            {[
              { label: 'Operating System',   items: [
                ['Name', 'Xenithra OS'], ['Version', '3.0.0 (Build 20260930)'], ['Architecture', 'x86-64 (AMD64)'],
                ['Kernel', 'Xenithra Kernel v3.0 (64-bit Higher Half)'], ['Boot Mode', 'UEFI 2.8 Secure Boot'],
              ]},
              { label: 'Hardware', items: [
                ['CPU', 'Intel Core (QEMU emulated) — 2 Cores @ 2.4 GHz'],
                ['RAM', '2048 MB DDR4'], ['GPU', 'QEMU/KVM VGA — 128 MB VRAM'],
                ['Storage', 'SATA HDD — 64 GB'], ['Display', '1280×720 @ 60 Hz'],
              ]},
              { label: 'Render Engine', items: [
                ['Engine', 'Xenithra Render Engine v3.0'], ['Runtime', 'Chromium V8 (Electron)'],
                ['UI Framework', 'React 18 + TypeScript'], ['Compositor', 'Chromium GPU Compositor (DWM proxy)'],
                ['Shaders', 'Mica + Acrylic (CSS backdrop-filter)'], ['IPC', 'Named Pipe → XenithraGUI'],
              ]},
              { label: 'Security', items: [
                ['SMEP', 'Enabled (CR4.bit20)'], ['SMAP', 'Enabled (CR4.bit21)'],
                ['Anti-Hijack', 'Session Guard v3.0 — 128-bit token'],
                ['Firewall', 'Xenithra Private Firewall (stateful)'],
              ]},
            ].map(section => (
              <div key={section.label}>
                <div className="section-header" style={{ paddingBottom: 8 }}>{section.label}</div>
                <div style={{ display: 'grid', gap: 5 }}>
                  {section.items.map(([k, v]) => (
                    <div key={k} style={{ display: 'flex', padding: '8px 12px', background: 'rgba(255,255,255,0.03)', borderRadius: 'var(--radius-md)', border: '1px solid var(--border-subtle)', gap: 12 }}>
                      <span style={{ width: 180, flexShrink: 0, fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)' }}>{k}</span>
                      <span style={{ fontSize: 'var(--text-xs)', color: 'var(--text-primary)', fontFamily: 'var(--font-mono)' }}>{v}</span>
                    </div>
                  ))}
                </div>
              </div>
            ))}
          </div>
        )}
      </div>
    </div>
  );
};
