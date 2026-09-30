/**
 * @file event_bus.ts — Xenithra OS v3.0 — Render Engine Event Bus
 *
 * Centralized event routing between:
 *   - Kernel IPC events (from Named Pipe via preload.cjs)
 *   - UI components (React state updates)
 *   - System services (network status, driver events)
 *
 * Uses a simple pub/sub pattern — components subscribe to event types
 * and receive typed payloads without prop-drilling or global state.
 */

import type { KernelEvent } from '../types';

type EventHandler<T = unknown> = (data: T) => void;

class EventBus {
  private listeners = new Map<string, Set<EventHandler>>();

  /** Subscribe to an event channel. Returns unsubscribe function. */
  on<T = unknown>(channel: string, handler: EventHandler<T>): () => void {
    if (!this.listeners.has(channel)) {
      this.listeners.set(channel, new Set());
    }
    this.listeners.get(channel)!.add(handler as EventHandler);

    // Return cleanup function
    return () => this.off(channel, handler as EventHandler);
  }

  /** Unsubscribe a handler. */
  off(channel: string, handler: EventHandler): void {
    this.listeners.get(channel)?.delete(handler);
  }

  /** Emit an event to all subscribers. */
  emit<T = unknown>(channel: string, data: T): void {
    this.listeners.get(channel)?.forEach(h => {
      try { h(data); } catch (e) { console.error(`[EventBus] ${channel}:`, e); }
    });
  }

  /** Emit a kernel IPC event — routes to typed sub-channels. */
  emitKernelEvent(evt: KernelEvent): void {
    this.emit('kernel:event', evt);
    if (evt.type) {
      this.emit(`kernel:${evt.type}`, evt);
    }
  }

  /** Remove all listeners for a channel. */
  clear(channel: string): void {
    this.listeners.delete(channel);
  }

  /** Remove ALL listeners (call on app unmount). */
  clearAll(): void {
    this.listeners.clear();
  }
}

/* ── Well-known channel names ────────────────────────────────────────── */
export const BUS_CHANNELS = {
  KERNEL_EVENT:   'kernel:event',
  KERNEL_MOUSE:   'kernel:mouse',
  KERNEL_KEY:     'kernel:key',
  KERNEL_WINDOW:  'kernel:window',
  KERNEL_SERVICE: 'kernel:service',
  KERNEL_RTOS:    'kernel:rtos',
  KERNEL_NETWORK: 'kernel:network',
  KERNEL_DRIVER:  'kernel:driver',
  KERNEL_NOTIFY:  'kernel:notify',
  IPC_STATUS:     'ipc:status',
  WINDOW_FOCUS:   'ui:window:focus',
  WINDOW_OPEN:    'ui:window:open',
  WINDOW_CLOSE:   'ui:window:close',
  TOAST:          'ui:toast',
} as const;

/* ── Singleton ───────────────────────────────────────────────────────── */
export const bus = new EventBus();
