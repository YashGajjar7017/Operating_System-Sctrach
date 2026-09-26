/**
 * ServicesPanel.tsx — Xenithra OS Windows Services Monitor
 *
 * Displays live telemetry for all OS services (SysMain, DWM, MMCSS, AudioSrv, WMI).
 * Each service card shows: name, status badge, CPU %, memory usage, mini activity graph.
 */

import React, { useMemo, useRef, useEffect } from 'react';
import { Settings, Music, Monitor, Database, Cpu, Activity } from 'lucide-react';
import type { KernelServiceEvent } from '../kernel/KernelBridge';

interface ServicesPanelProps {
  services: Map<string, KernelServiceEvent>;
  connected: boolean;
}

const SERVICE_META: Record<string, {
  icon: React.ReactNode;
  color: string;
  desc: string;
}> = {
  SysMain:  { icon: <Database size={14} />,  color: '#3b82f6',  desc: 'Memory Prefetcher' },
  DWM:      { icon: <Monitor size={14} />,   color: '#8b5cf6',  desc: 'Desktop Compositor' },
  MMCSS:    { icon: <Music size={14} />,     color: '#10b981',  desc: 'Multimedia Scheduler' },
  AudioSrv: { icon: <Music size={14} />,     color: '#f59e0b',  desc: 'Windows Audio' },
  WMI:      { icon: <Database size={14} />,  color: '#64748b',  desc: 'Management Interface' },
};

const FALLBACK_SERVICES: KernelServiceEvent[] = [
  { type: 'service', name: 'SysMain',  cpu: 2,  mem: 18432,  status: 'running' },
  { type: 'service', name: 'DWM',      cpu: 5,  mem: 65536,  status: 'vsync-ok' },
  { type: 'service', name: 'MMCSS',    cpu: 1,  mem: 4096,   status: 'optimal' },
  { type: 'service', name: 'AudioSrv', cpu: 3,  mem: 8192,   status: 'running' },
  { type: 'service', name: 'WMI',      cpu: 1,  mem: 16384,  status: 'running' },
];

function MiniGraph({ value, color }: { value: number; color: string }) {
  const history = useRef<number[]>([...Array(20)].map(() => 0));
  useEffect(() => {
    history.current = [...history.current.slice(1), value];
  }, [value]);

  const bars = history.current;
  const max  = Math.max(...bars, 1);

  return (
    <div style={{ display: 'flex', alignItems: 'flex-end', gap: 1.5, height: 24 }}>
      {bars.map((v, i) => (
        <div key={i} style={{
          flex: 1,
          height: `${Math.max(10, (v / max) * 100)}%`,
          borderRadius: 2,
          backgroundColor: i === bars.length - 1
            ? color
            : `${color}55`,
          transition: 'height 0.3s ease',
        }} />
      ))}
    </div>
  );
}

function ServiceCard({ svc }: { svc: KernelServiceEvent }) {
  const meta = SERVICE_META[svc.name];
  const isOk = !['stressed', 'vsync-late', 'muted'].includes(svc.status);
  const statusColor = isOk ? '#10b981' : svc.status === 'muted' ? '#64748b' : '#f59e0b';
  const color = meta?.color ?? '#3b82f6';

  const memStr = svc.mem >= 1024
    ? `${(svc.mem / 1024).toFixed(1)} MB`
    : `${svc.mem} KB`;

  return (
    <div style={{
      background: 'rgba(255,255,255,0.03)',
      border: `1px solid rgba(255,255,255,0.07)`,
      borderLeft: `3px solid ${color}`,
      borderRadius: 10,
      padding: '10px 12px',
      display: 'grid',
      gridTemplateColumns: '1fr auto',
      gap: '8px',
    }}>
      {/* Left: info */}
      <div>
        <div style={{ display: 'flex', alignItems: 'center', gap: 6, marginBottom: 4 }}>
          <span style={{ color }}>{meta?.icon}</span>
          <span style={{ color: '#e2e8f0', fontWeight: 700, fontSize: 12 }}>{svc.name}</span>
          <span style={{
            padding: '1px 6px', borderRadius: 99, fontSize: 9, fontWeight: 600,
            backgroundColor: `${statusColor}20`, color: statusColor,
            border: `1px solid ${statusColor}40`, letterSpacing: '0.06em', textTransform: 'uppercase',
          }}>
            {svc.status}
          </span>
        </div>
        <div style={{ color: '#64748b', fontSize: 10, marginBottom: 6 }}>
          {meta?.desc}
        </div>
        <div style={{ display: 'flex', gap: 12 }}>
          <div>
            <div style={{ color: '#475569', fontSize: 9, textTransform: 'uppercase', letterSpacing: '0.06em' }}>CPU</div>
            <div style={{ color: '#94a3b8', fontSize: 13, fontWeight: 600, fontFamily: 'monospace' }}>{svc.cpu}%</div>
          </div>
          <div>
            <div style={{ color: '#475569', fontSize: 9, textTransform: 'uppercase', letterSpacing: '0.06em' }}>MEM</div>
            <div style={{ color: '#94a3b8', fontSize: 13, fontWeight: 600, fontFamily: 'monospace' }}>{memStr}</div>
          </div>
        </div>
      </div>

      {/* Right: mini graph */}
      <div style={{ width: 60 }}>
        <MiniGraph value={svc.cpu} color={color} />
      </div>
    </div>
  );
}

export const ServicesPanel: React.FC<ServicesPanelProps> = ({ services, connected }) => {
  const displayServices = useMemo(() => {
    const live = Array.from(services.values());
    if (live.length > 0) return live;
    return FALLBACK_SERVICES;
  }, [services]);

  const totalCpu = displayServices.reduce((acc, s) => acc + s.cpu, 0);

  return (
    <div style={{
      background: 'linear-gradient(135deg, rgba(15,23,42,0.98) 0%, rgba(10,16,35,0.98) 100%)',
      border: '1px solid rgba(139,92,246,0.15)',
      borderRadius: 12,
      overflow: 'hidden',
      fontFamily: "'Inter', sans-serif",
    }}>
      {/* Header */}
      <div style={{
        padding: '12px 16px',
        background: 'rgba(139,92,246,0.06)',
        borderBottom: '1px solid rgba(139,92,246,0.10)',
        display: 'flex', alignItems: 'center', justifyContent: 'space-between',
      }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
          <Settings size={14} color="#8b5cf6" />
          <span style={{ color: '#e2e8f0', fontWeight: 700, fontSize: 13, letterSpacing: '0.05em' }}>
            SYSTEM SERVICES
          </span>
        </div>
        <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
          <span style={{ color: '#64748b', fontSize: 11 }}>
            Total CPU: <strong style={{ color: '#94a3b8' }}>{totalCpu}%</strong>
          </span>
          <div style={{
            padding: '2px 8px', borderRadius: 99,
            backgroundColor: connected ? 'rgba(16,185,129,0.15)' : 'rgba(100,116,139,0.15)',
            border: `1px solid ${connected ? '#10b981' : '#64748b'}`,
            color: connected ? '#10b981' : '#64748b',
            fontSize: 10, fontWeight: 600, letterSpacing: '0.08em',
          }}>
            {connected ? '● LIVE' : '○ DEMO'}
          </div>
        </div>
      </div>

      {/* Service cards */}
      <div style={{ padding: '10px 12px', display: 'flex', flexDirection: 'column', gap: 6 }}>
        {displayServices.map(svc => (
          <ServiceCard key={svc.name} svc={svc} />
        ))}
      </div>
    </div>
  );
};
