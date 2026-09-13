/**
 * @file reconciler.ts
 * @brief Custom Headless React Reconciler Bridge to Bare-Metal V8 nativeUI
 */

export interface NativeUIBridge {
  drawRect: (x: number, y: number, w: number, h: number, color: number) => void;
  drawRoundedRect: (x: number, y: number, w: number, h: number, r: number, color: number) => void;
  drawText: (x: number, y: number, text: string, color: number, scale: number) => void;
  swapBuffers: () => void;
}

// Check for bare-metal V8 nativeUI injection or browser fallback
export const nativeUI: NativeUIBridge = (typeof window !== 'undefined' && (window as any).nativeUI)
  ? (window as any).nativeUI
  : {
      drawRect: (x, y, w, h, color) => {
        // Fallback for web preview
        if (typeof document !== 'undefined') {
          const canvas = document.getElementById('native-canvas') as HTMLCanvasElement;
          if (canvas) {
            const ctx = canvas.getContext('2d');
            if (ctx) {
              const hex = '#' + (color & 0x00FFFFFF).toString(16).padStart(6, '0');
              ctx.fillStyle = hex;
              ctx.fillRect(x, y, w, h);
            }
          }
        }
      },
      drawRoundedRect: (x, y, w, h, r, color) => {
        if (typeof document !== 'undefined') {
          const canvas = document.getElementById('native-canvas') as HTMLCanvasElement;
          if (canvas) {
            const ctx = canvas.getContext('2d');
            if (ctx) {
              const hex = '#' + (color & 0x00FFFFFF).toString(16).padStart(6, '0');
              ctx.fillStyle = hex;
              ctx.beginPath();
              ctx.roundRect(x, y, w, h, r);
              ctx.fill();
            }
          }
        }
      },
      drawText: (x, y, text, color, scale) => {
        if (typeof document !== 'undefined') {
          const canvas = document.getElementById('native-canvas') as HTMLCanvasElement;
          if (canvas) {
            const ctx = canvas.getContext('2d');
            if (ctx) {
              const hex = '#' + (color & 0x00FFFFFF).toString(16).padStart(6, '0');
              ctx.fillStyle = hex;
              ctx.font = `${14 * scale}px 'Segoe UI', sans-serif`;
              ctx.fillText(text, x, y + 12 * scale);
            }
          }
        }
      },
      swapBuffers: () => {}
    };

export interface NodeProps {
  x?: number;
  y?: number;
  width?: number;
  height?: number;
  borderRadius?: number;
  backgroundColor?: number;
  color?: number;
  text?: string;
  onClick?: () => void;
  children?: any;
}

export class NativeNode {
  type: string;
  props: NodeProps;
  children: NativeNode[] = [];
  parent: NativeNode | null = null;

  constructor(type: string, props: NodeProps) {
    this.type = type;
    this.props = props;
  }

  render() {
    const { x = 0, y = 0, width = 0, height = 0, borderRadius = 0, backgroundColor, text, color = 0xFFFFFFFF } = this.props;

    if (backgroundColor !== undefined) {
      if (borderRadius > 0) {
        nativeUI.drawRoundedRect(x, y, width, height, borderRadius, backgroundColor);
      } else {
        nativeUI.drawRect(x, y, width, height, backgroundColor);
      }
    }

    if (this.type === 'Text' && text) {
      nativeUI.drawText(x, y, text, color, 1);
    }

    for (const child of this.children) {
      child.render();
    }
  }
}
