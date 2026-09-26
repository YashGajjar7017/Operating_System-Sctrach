/**
 * KernelBridge.ts
 * React hook for consuming real-time kernel IPC events.
 *
 * Usage:
 *   const { connected, mousePos, lastKey, rtosThreads, services } = useKernelBridge();
 */

import { useEffect, useRef, useState, useCallback } from 'react';

/* ------------------------------------------------------------------ */
/* Types matching the kernel JSON protocol                            */
/* ------------------------------------------------------------------ */

export interface KernelMouseEvent {
  type: 'mouse';
  x: number;
  y: number;
  l: number;
  r: number;
  m: number;
}

export interface KernelKeyEvent {
  type: 'key';
  ascii: number;
  scan: number;
  pressed: number;
}

export interface KernelWindowEvent {
  type: 'window';
  action: 'open' | 'close' | 'minimize' | 'maximize' | 'focus';
  tag: string;
  title: string;
}

export interface KernelServiceEvent {
  type: 'service';
  name: string;   // "SysMain" | "DWM" | "MMCSS" | "AudioSrv" | "WMI"
  cpu: number;    // 0–100 %
  mem: number;    // KB
  status: string; // "running" | "stressed" | "muted" | "vsync-ok" | ...
}

export interface KernelRtosEvent {
  type: 'rtos';
  tid: number;
  prio: number;   // 0=idle 1=low 2=normal 3=high 4=realtime 5=isr
  state: string;  // "ready" | "running" | "blocked" | "sleeping" | "dpc" | "dead"
  cpu_us: number;
  affinity: number;
}

export interface KernelNotification {
  type: 'notify';
  title: string;
  body: string;
  icon: string;
}

export interface KernelIpcStatus {
  type: 'ipc';
  action: string;
}

/* ------------------------------------------------------------------ */
/* Kernel API type (from preload)                                      */
/* ------------------------------------------------------------------ */

type KernelEventChannel =
  | 'kernel:mouse' | 'kernel:key' | 'kernel:window' | 'kernel:service'
  | 'kernel:rtos'  | 'kernel:notify' | 'kernel:power' | 'kernel:wmi'
  | 'kernel:ipc'   | 'kernel:raw' | 'kernel:log';

declare global {
  interface Window {
    kernelAPI?: {
      launchApp: (appName: string, url?: string) => Promise<any>;
      closeApp: (appName: string) => Promise<any>;
      systemPower: (action: string) => Promise<any>;
      queryKernel: (target: string) => Promise<any>;
      setAudio: (volume: number, mute?: boolean) => Promise<any>;
      pipeStatus: () => Promise<{ connected: boolean; pipe: string }>;
      onKernelEvent: (channel: KernelEventChannel, cb: (data: any) => void) => Function;
      removeKernelListener: (channel: KernelEventChannel, wrapped: Function) => void;
      onRTOS: (cb: (data: KernelRtosEvent) => void) => Function;
      onService: (cb: (data: KernelServiceEvent) => void) => Function;
      onNotification: (cb: (data: KernelNotification) => void) => Function;
      onWindowEvent: (cb: (data: KernelWindowEvent) => void) => Function;
    };
    shellAPI?: {
      launchApp: (appName: string) => Promise<any>;
      systemAction: (action: string) => Promise<any>;
      onSystemMessage: (cb: (data: any) => void) => void;
    };
  }
}

/* ------------------------------------------------------------------ */
/* useKernelBridge Hook                                               */
/* ------------------------------------------------------------------ */

export interface KernelBridgeState {
  connected: boolean;
  mousePos: { x: number; y: number } | null;
  lastKey: KernelKeyEvent | null;
  rtosThreads: Map<number, KernelRtosEvent>;
  services: Map<string, KernelServiceEvent>;
  notifications: KernelNotification[];
  windowEvents: KernelWindowEvent[];
}

export function useKernelBridge(): KernelBridgeState {
  const [connected, setConnected]       = useState(false);
  const [mousePos, setMousePos]         = useState<{ x: number; y: number } | null>(null);
  const [lastKey, setLastKey]           = useState<KernelKeyEvent | null>(null);
  const [rtosThreads, setRtosThreads]   = useState<Map<number, KernelRtosEvent>>(new Map());
  const [services, setServices]         = useState<Map<string, KernelServiceEvent>>(new Map());
  const [notifications, setNotifications] = useState<KernelNotification[]>([]);
  const [windowEvents, setWindowEvents] = useState<KernelWindowEvent[]>([]);

  const listenersRef = useRef<Array<{ channel: KernelEventChannel; wrapped: Function }>>([]);

  useEffect(() => {
    const api = window.kernelAPI;
    if (!api) {
      console.warn('[KernelBridge] window.kernelAPI not available (not in Electron)');
      return;
    }

    const subscribe = (channel: KernelEventChannel, cb: (data: any) => void) => {
      const wrapped = api.onKernelEvent(channel, cb);
      listenersRef.current.push({ channel, wrapped });
    };

    // IPC connection status
    subscribe('kernel:ipc', (data: KernelIpcStatus) => {
      setConnected(data.action === 'connected');
    });

    // Mouse events
    subscribe('kernel:mouse', (data: KernelMouseEvent) => {
      setMousePos({ x: data.x, y: data.y });
    });

    // Key events
    subscribe('kernel:key', (data: KernelKeyEvent) => {
      if (data.pressed) setLastKey(data);
    });

    // RTOS thread telemetry
    subscribe('kernel:rtos', (data: KernelRtosEvent) => {
      setRtosThreads(prev => {
        const next = new Map(prev);
        next.set(data.tid, data);
        return next;
      });
    });

    // Service telemetry
    subscribe('kernel:service', (data: KernelServiceEvent) => {
      setServices(prev => {
        const next = new Map(prev);
        next.set(data.name, data);
        return next;
      });
    });

    // Notifications
    subscribe('kernel:notify', (data: KernelNotification) => {
      setNotifications(prev => [...prev.slice(-9), data]);
    });

    // Window events
    subscribe('kernel:window', (data: KernelWindowEvent) => {
      setWindowEvents(prev => [...prev.slice(-19), data]);
    });

    // Check initial connection
    api.pipeStatus().then(s => setConnected(s.connected)).catch(() => {});

    return () => {
      const { current: listeners } = listenersRef;
      listeners.forEach(({ channel, wrapped }) => {
        api.removeKernelListener(channel, wrapped);
      });
      listenersRef.current = [];
    };
  }, []);

  return { connected, mousePos, lastKey, rtosThreads, services, notifications, windowEvents };
}

/* ------------------------------------------------------------------ */
/* Standalone helpers (no hook needed)                                */
/* ------------------------------------------------------------------ */

export async function kernelLaunchApp(appName: string, url = ''): Promise<any> {
  return window.kernelAPI?.launchApp(appName, url)
    ?? window.shellAPI?.launchApp(appName)
    ?? Promise.resolve({ status: 'no-api' });
}

export async function kernelPower(action: string): Promise<any> {
  return window.kernelAPI?.systemPower(action)
    ?? window.shellAPI?.systemAction(action)
    ?? Promise.resolve({ status: 'no-api' });
}

export async function kernelQuery(target: string): Promise<any> {
  return window.kernelAPI?.queryKernel(target)
    ?? Promise.resolve({ status: 'no-api' });
}

export async function kernelSetAudio(volume: number, mute = false): Promise<any> {
  return window.kernelAPI?.setAudio(volume, mute)
    ?? Promise.resolve({ status: 'no-api' });
}
