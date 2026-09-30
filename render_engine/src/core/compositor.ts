/**
 * @file compositor.ts — Xenithra OS v3.0 Render Engine — JS DWM Compositor
 *
 * JavaScript-side window compositor — replaces the legacy C compositor.c.
 * Implements:
 *   - Window z-order management (focus stacking)
 *   - Snap zone detection (snap left/right/corners)
 *   - Occlusion rectangle calculation
 *   - Dirty region tracking (only re-render changed areas)
 *   - Spring-physics animation state for window transitions
 *
 * This runs in the Electron renderer process (V8/Chromium).
 * It mirrors the logic described in the DWM rendering pipeline diagram.
 */

export interface CompositorWindow {
  id: string;
  x: number; y: number;
  width: number; height: number;
  zIndex: number;
  minimized: boolean;
  maximized: boolean;
  opacity: number;
  snapZone: SnapZone | null;
  dirty: boolean;
}

export type SnapZone =
  | 'left' | 'right'
  | 'top-left' | 'top-right'
  | 'bottom-left' | 'bottom-right'
  | 'maximize';

export interface CompositorState {
  windows: CompositorWindow[];
  focusedId: string | null;
  screenW: number;
  screenH: number;
  taskbarH: number;
}

/* ── Snap thresholds ─────────────────────────────────────────────────── */
const SNAP_THRESHOLD_PX = 24;   /* Pixels from edge to trigger snap */
const SNAP_PREVIEW_OPACITY = 0.35;

/* ── Spring physics parameters ───────────────────────────────────────── */
const SPRING_STIFFNESS = 280;
const SPRING_DAMPING   = 26;
const SPRING_MASS      = 1;

/* ── Compositor class ────────────────────────────────────────────────── */
export class Compositor {
  private state: CompositorState;
  private _topZ = 100;

  constructor(screenW: number, screenH: number, taskbarH = 48) {
    this.state = { windows: [], focusedId: null, screenW, screenH, taskbarH };
  }

  /** Register a new window into the compositor. */
  addWindow(win: Omit<CompositorWindow, 'dirty' | 'opacity' | 'snapZone'>): void {
    this.state.windows.push({ ...win, opacity: 1, snapZone: null, dirty: true });
    this.focus(win.id);
  }

  /** Remove a window from compositor. */
  removeWindow(id: string): void {
    this.state.windows = this.state.windows.filter(w => w.id !== id);
    if (this.state.focusedId === id) {
      const top = this.getTopWindow();
      this.state.focusedId = top?.id ?? null;
    }
  }

  /** Focus a window — brings to top of z-stack. */
  focus(id: string): void {
    const win = this.state.windows.find(w => w.id === id);
    if (!win) return;
    this._topZ++;
    win.zIndex = this._topZ;
    win.minimized = false;
    win.dirty = true;
    this.state.focusedId = id;
  }

  /** Minimize a window. */
  minimize(id: string): void {
    const win = this.state.windows.find(w => w.id === id);
    if (!win) return;
    win.minimized = true;
    win.dirty = true;
    // Focus next visible window
    const next = this.getTopWindow(id);
    this.state.focusedId = next?.id ?? null;
  }

  /** Toggle maximize. */
  toggleMaximize(id: string): void {
    const win = this.state.windows.find(w => w.id === id);
    if (!win) return;
    win.maximized = !win.maximized;
    if (win.maximized) {
      win.snapZone = 'maximize';
      win.x = 0; win.y = 0;
      win.width = this.state.screenW;
      win.height = this.state.screenH - this.state.taskbarH;
    }
    win.dirty = true;
  }

  /** Move a window — triggers snap preview if near edges. */
  move(id: string, x: number, y: number): SnapZone | null {
    const win = this.state.windows.find(w => w.id === id);
    if (!win || win.maximized) return null;
    win.x = x; win.y = Math.max(0, y);
    win.dirty = true;
    return this.detectSnapZone(x, y, win.width, win.height);
  }

  /** Apply snap zone to window. */
  applySnap(id: string, zone: SnapZone): void {
    const win = this.state.windows.find(w => w.id === id);
    if (!win) return;
    const { screenW: W, screenH: H, taskbarH: T } = this.state;
    const halfW = Math.floor(W / 2), halfH = Math.floor((H - T) / 2);

    const snapMap: Record<SnapZone, [number, number, number, number]> = {
      'left':         [0,     0,     halfW, H - T],
      'right':        [halfW, 0,     halfW, H - T],
      'top-left':     [0,     0,     halfW, halfH],
      'top-right':    [halfW, 0,     halfW, halfH],
      'bottom-left':  [0,     halfH, halfW, halfH],
      'bottom-right': [halfW, halfH, halfW, halfH],
      'maximize':     [0,     0,     W,     H - T],
    };

    const [nx, ny, nw, nh] = snapMap[zone];
    win.x = nx; win.y = ny; win.width = nw; win.height = nh;
    win.snapZone = zone;
    win.dirty = true;
  }

  /** Resize a window. */
  resize(id: string, width: number, height: number): void {
    const win = this.state.windows.find(w => w.id === id);
    if (!win) return;
    win.width  = Math.max(320, width);
    win.height = Math.max(200, height);
    win.snapZone = null;
    win.dirty = true;
  }

  /** Detect which snap zone a dragged window is entering. */
  detectSnapZone(x: number, y: number, w: number, h: number): SnapZone | null {
    const { screenW: W, screenH: H } = this.state;
    const T = SNAP_THRESHOLD_PX;
    const atLeft   = x < T;
    const atRight  = x + w > W - T;
    const atTop    = y < T;

    if (atTop && atLeft)  return 'top-left';
    if (atTop && atRight) return 'top-right';
    if (atLeft)           return 'left';
    if (atRight)          return 'right';
    if (atTop)            return 'maximize';
    return null;
  }

  /** Compute occlusion: returns true if window A is fully hidden by B. */
  isOccluded(a: CompositorWindow, b: CompositorWindow): boolean {
    if (b.minimized || b.id === a.id || b.zIndex <= a.zIndex) return false;
    return b.x <= a.x && b.y <= a.y &&
           b.x + b.width  >= a.x + a.width &&
           b.y + b.height >= a.y + a.height;
  }

  /** Get dirty windows (need re-render) and clear dirty flags. */
  flushDirty(): CompositorWindow[] {
    const dirty = this.state.windows.filter(w => w.dirty && !w.minimized);
    dirty.forEach(w => { w.dirty = false; });
    return dirty;
  }

  /** Get the focused (topmost visible) window. */
  getTopWindow(excludeId?: string): CompositorWindow | null {
    return this.state.windows
      .filter(w => !w.minimized && w.id !== excludeId)
      .sort((a, b) => b.zIndex - a.zIndex)[0] ?? null;
  }

  /** Get all windows sorted by z-order (back to front, for rendering). */
  getWindowsInOrder(): CompositorWindow[] {
    return [...this.state.windows]
      .filter(w => !w.minimized)
      .sort((a, b) => a.zIndex - b.zIndex);
  }

  get focusedId(): string | null { return this.state.focusedId; }

  /** Notify compositor of screen resize. */
  resize_screen(w: number, h: number): void {
    this.state.screenW = w;
    this.state.screenH = h;
    this.state.windows.forEach(win => { win.dirty = true; });
  }
}

/* ── Spring physics animation helper ────────────────────────────────── */
export class SpringAnimation {
  private position: number;
  private velocity: number;
  private target: number;

  constructor(initial: number) {
    this.position = initial;
    this.velocity = 0;
    this.target   = initial;
  }

  setTarget(target: number): void { this.target = target; }

  /** Advance by dt seconds. Returns current position. */
  tick(dt: number): number {
    const force = SPRING_STIFFNESS * (this.target - this.position);
    const damping = SPRING_DAMPING * this.velocity;
    const accel = (force - damping) / SPRING_MASS;
    this.velocity += accel * dt;
    this.position += this.velocity * dt;
    return this.position;
  }

  isSettled(threshold = 0.1): boolean {
    return Math.abs(this.target - this.position) < threshold &&
           Math.abs(this.velocity) < threshold;
  }

  get value(): number { return this.position; }
}

/* ── Singleton compositor instance ──────────────────────────────────── */
let _instance: Compositor | null = null;

export function getCompositor(
  screenW = window.innerWidth,
  screenH = window.innerHeight,
  taskbarH = 48
): Compositor {
  if (!_instance) {
    _instance = new Compositor(screenW, screenH, taskbarH);
  }
  return _instance;
}
