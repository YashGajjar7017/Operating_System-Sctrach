/**
 * RTOSPanel.tsx — Xenithra OS Real-Time Scheduler Monitor
 *
 * Displays live RTOS thread telemetry from the kernel IPC bridge.
 * Shows: TID, name (if available), priority level, state, CPU µs, CPU affinity.
 * Color-coded by priority: REALTIME=red, HIGH=orange, NORMAL=blue, LOW=gray, IDLE=dark.
 */

import React, { useMemo } from 'react';
import { Cpu, Zap, Clock, Activity } from 'lucide-react';
import type { KernelRtosEvent } from '../kernel/KernelBridge';

interface RTOSPanelProps {
  threads: Map<number, KernelRtosEvent>;
  connected: boolean;
}

const PRIORITY_LABELS = ['IDLE', 'LOW', 'NORMAL', 'HIGH', 'REALTIME', 'ISR'];
const PRIORITY_COLORS: Record<number, string> = {
  0: 'rgba(100,116,139,0.15)',  // IDLE    — slate
  1: 'rgba(51,65,85,0.20)',     // LOW     — dark slate
  2: 'rgba(30,64,175,0.20)',    // NORMAL  — blue
  3: 'rgba(180,83,9,0.25)',     // HIGH    — amber
  4: 'rgba(185,28,28,0.30)',    // REALTIME — red
  5: 'rgba(124,58,237,0.30)',   // ISR     — violet
};
const PRIORITY_BADGE: Record<number, string> = {
  0: '#475569', 1: '#64748b', 2: '#3b82f6',
  3: '#f59e0b', 4: '#ef4444', 5: '#8b5cf6',
};
const STATE_COLORS: Record<string, string> = {
  running:  '#10b981',
  ready:    '#3b82f6',
  blocked:  '#f59e0b',
  sleeping: '#64748b',
  dpc:      '#8b5cf6',
  dead:     '#374151',
};

function PriorityBar({ priority }: { priority: number }) {
  const filled = priority + 1;
  return (
    <div style={{ display: 'flex', gap: 2 }}>
      {[0,1,2,3,4].map(i => (
        <div key={i} style={{
          width: 4, height: 12,
          borderRadius: 2,
          backgroundColor: i < filled
            ? PRIORITY_BADGE[priority] ?? '#3b82f6'
            : 'rgba(255,255,255,0.08)',
        }} />
      ))}
    </div>
  );
}

function ThreadRow({ t }: { t: KernelRtosEvent }) {
  const stateDot = STATE_COLORS[t.state] ?? '#475569';
  return (
    <div style={{
      display: 'grid',
      gridTemplateColumns: '40px 60px 90px 80px 80px 1fr',
      alignItems: 'center',
      gap: 8,
      padding: '5px 10px',
      borderRadius: 6,
      backgroundColor: PRIORITY_COLORS[t.prio] ?? 'transparent',
      borderLeft: `3px solid ${PRIORITY_BADGE[t.prio] ?? '#3b82f6'}`,
      marginBottom: 2,
      fontSize: 11,
      fontFamily: "'JetBrains Mono', monospace",
    }}>
      {/* TID */}
      <span style={{ color: '#94a3b8' }}>#{t.tid}</span>

      {/* Priority bar */}
      <PriorityBar priority={t.prio} />

      {/* Priority label */}
      <span style={{ color: PRIORITY_BADGE[t.prio] ?? '#3b82f6', fontWeight: 600, fontSize: 10 }}>
        {PRIORITY_LABELS[t.prio] ?? '?'}
      </span>

      {/* State */}
      <span style={{ display: 'flex', alignItems: 'center', gap: 4 }}>
        <span style={{ width: 6, height: 6, borderRadius: '50%', backgroundColor: stateDot, flexShrink: 0 }} />
        <span style={{ color: stateDot, textTransform: 'uppercase', fontSize: 10, letterSpacing: '0.05em' }}>
          {t.state}
        </span>
      </span>

      {/* CPU µs */}
      <span style={{ color: '#94a3b8' }}>
        {t.cpu_us > 0 ? `${t.cpu_us}µs` : '—'}
      </span>

      {/* Affinity */}
      <span style={{ color: '#475569', fontSize: 10 }}>
        {t.affinity ? `CPU${t.affinity === 1 ? '0' : t.affinity === 2 ? '1' : '*'}` : 'ANY'}
      </span>
    </div>
  );
}

export const RTOSPanel: React.FC<RTOSPanelProps> = ({ threads, connected }) => {
  const sortedThreads = useMemo(() => {
    return Array.from(threads.values()).sort((a, b) => b.prio - a.prio);
  }, [threads]);

  const stats = useMemo(() => {
    const counts = { running: 0, ready: 0, blocked: 0, sleeping: 0, total: 0 };
    threads.forEach(t => {
      counts.total++;
      if (t.state in counts) (counts as any)[t.state]++;
    });
    return counts;
  }, [threads]);

  return (
    <div style={{
      background: 'linear-gradient(135deg, rgba(15,23,42,0.98) 0%, rgba(10,16,35,0.98) 100%)',
      border: '1px solid rgba(59,130,246,0.15)',
      borderRadius: 12,
      overflow: 'hidden',
      fontFamily: "'Inter', sans-serif",
    }}>
      {/* Header */}
      <div style={{
        padding: '12px 16px',
        background: 'rgba(59,130,246,0.06)',
        borderBottom: '1px solid rgba(59,130,246,0.10)',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'space-between',
      }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
          <Zap size={14} color="#3b82f6" />
          <span style={{ color: '#e2e8f0', fontWeight: 700, fontSize: 13, letterSpacing: '0.05em' }}>
            RTOS SCHEDULER MONITOR
          </span>
        </div>
        <div style={{
          padding: '2px 8px',
          borderRadius: 99,
          backgroundColor: connected ? 'rgba(16,185,129,0.15)' : 'rgba(239,68,68,0.15)',
          border: `1px solid ${connected ? '#10b981' : '#ef4444'}`,
          color: connected ? '#10b981' : '#ef4444',
          fontSize: 10,
          fontWeight: 600,
          letterSpacing: '0.08em',
        }}>
          {connected ? '● KERNEL LIVE' : '○ SIMULATED'}
        </div>
      </div>

      {/* Stats row */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: 'repeat(4, 1fr)',
        padding: '10px 16px',
        borderBottom: '1px solid rgba(255,255,255,0.04)',
        gap: 8,
      }}>
        {[
          { label: 'THREADS', value: stats.total, icon: <Cpu size={11} color="#3b82f6" /> },
          { label: 'RUNNING', value: stats.running, icon: <Activity size={11} color="#10b981" /> },
          { label: 'READY',   value: stats.ready,   icon: <Clock size={11} color="#f59e0b" /> },
          { label: 'BLOCKED', value: stats.blocked,  icon: <Clock size={11} color="#ef4444" /> },
        ].map(s => (
          <div key={s.label} style={{
            display: 'flex', flexDirection: 'column', alignItems: 'center',
            background: 'rgba(255,255,255,0.03)', borderRadius: 8, padding: '6px 4px',
          }}>
            <div style={{ display: 'flex', alignItems: 'center', gap: 3, marginBottom: 2 }}>
              {s.icon}
              <span style={{ color: '#64748b', fontSize: 9, letterSpacing: '0.08em' }}>{s.label}</span>
            </div>
            <span style={{ color: '#e2e8f0', fontWeight: 700, fontSize: 18 }}>{s.value}</span>
          </div>
        ))}
      </div>

      {/* Thread table header */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: '40px 60px 90px 80px 80px 1fr',
        gap: 8,
        padding: '4px 10px',
        color: '#475569',
        fontSize: 9,
        letterSpacing: '0.08em',
        textTransform: 'uppercase',
      }}>
        <span>TID</span>
        <span>LEVEL</span>
        <span>PRIORITY</span>
        <span>STATE</span>
        <span>CPU TIME</span>
        <span>AFFINITY</span>
      </div>

      {/* Thread list */}
      <div style={{
        padding: '4px 6px 12px',
        maxHeight: 280,
        overflowY: 'auto',
      }}>
        {sortedThreads.length === 0 ? (
          <div style={{
            textAlign: 'center', color: '#475569', fontSize: 12,
            padding: '24px 0',
          }}>
            {connected
              ? 'Waiting for RTOS thread data...'
              : 'Demo threads shown — connect kernel for live data'}
          </div>
        ) : (
          sortedThreads.map(t => <ThreadRow key={t.tid} t={t} />)
        )}

        {/* Demo threads when not connected */}
        {!connected && sortedThreads.length === 0 && ([
          { tid: 1, prio: 4, state: 'running',  cpu_us: 312, affinity: 2, type: 'rtos' as const },
          { tid: 2, prio: 4, state: 'ready',    cpu_us: 0,   affinity: 2, type: 'rtos' as const },
          { tid: 3, prio: 3, state: 'running',  cpu_us: 156, affinity: 1, type: 'rtos' as const },
          { tid: 4, prio: 3, state: 'sleeping', cpu_us: 0,   affinity: 1, type: 'rtos' as const },
          { tid: 5, prio: 2, state: 'ready',    cpu_us: 0,   affinity: 0, type: 'rtos' as const },
          { tid: 6, prio: 1, state: 'sleeping', cpu_us: 0,   affinity: 1, type: 'rtos' as const },
          { tid: 7, prio: 0, state: 'sleeping', cpu_us: 0,   affinity: 0, type: 'rtos' as const },
        ] as KernelRtosEvent[]).map(t => <ThreadRow key={t.tid} t={t} />)}
      </div>
    </div>
  );
};
