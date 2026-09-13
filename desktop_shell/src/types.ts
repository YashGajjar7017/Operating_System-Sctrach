/**
 * @file types.ts
 * @brief Global TypeScript type definitions for Xenithra OS Windows 11 Desktop Shell
 */

export interface WindowState {
  id: string;
  title: string;
  icon: string;
  tag: string;
  x: number;
  y: number;
  width: number;
  height: number;
  minimized: boolean;
  maximized: boolean;
  zIndex: number;
  snapState?: 'left' | 'right' | 'top-left' | 'top-right' | 'bottom-left' | 'bottom-right' | null;
}

/* 1. Windows Services Manager (services.msc) */
export type ServiceStatus = 'Running' | 'Stopped' | 'Paused' | 'Starting' | 'Stopping';
export type StartupType = 'Automatic' | 'Manual' | 'Disabled';

export interface ServiceItem {
  name: string;
  displayName: string;
  status: ServiceStatus;
  startupType: StartupType;
  logOnAs: string;
  pid: number | null;
  memoryMb: number;
  cpuPercent: number;
  description: string;
  category: 'Core Kernel' | 'Security' | 'Networking' | 'System Runtime' | 'UI & Graphics';
}

export interface ServiceLogEntry {
  id: string;
  timestamp: string;
  source: string;
  eventId: number;
  level: 'Information' | 'Warning' | 'Error' | 'Critical';
  message: string;
}

/* 2. Windows Defender & Security Center */
export type ScanType = 'quick' | 'full' | 'memory' | 'custom';
export type ProtectionLevel = 'Secure' | 'Warning' | 'Vulnerable' | 'Action Required';

export interface ThreatItem {
  id: string;
  name: string;
  severity: 'Low' | 'Medium' | 'High' | 'Severe';
  category: 'Trojan' | 'Ransomware' | 'Privilege Escalation' | 'Memory Exploit' | 'Packet Flood';
  affectedFile: string;
  dateDetected: string;
  status: 'Quarantined' | 'Active' | 'Cleaned' | 'Blocked';
}

export interface FirewallRuleItem {
  id: number;
  protocol: 'TCP' | 'UDP' | 'ICMP' | 'RAW';
  port: number;
  action: 'ALLOW' | 'DROP';
  description: string;
  enabled: boolean;
  packetCount: number;
}

/* 3. Edge / Chrome Web Browser */
export interface BrowserTab {
  id: string;
  title: string;
  url: string;
  favicon?: string;
  isLoading: boolean;
  canGoBack: boolean;
  canGoForward: boolean;
  isSecure: boolean;
  contentMode: 'google' | 'url' | 'iframe' | 'portal' | 'docs' | 'search';
  searchQuery?: string;
}

export interface BookmarkItem {
  id: string;
  title: string;
  url: string;
  icon: string;
}

export interface HistoryItem {
  id: string;
  title: string;
  url: string;
  time: string;
}

/* 4. VLC Media Player */
export interface TrackItem {
  id: string;
  title: string;
  artist: string;
  album: string;
  duration: number; // in seconds
  genre: string;
  coverArt?: string;
  freqPattern?: number[];
  type: 'audio' | 'video';
  videoUrl?: string;
}

/* 5. VS Code Lite / Code Editor */
export interface EditorFile {
  id: string;
  name: string;
  path: string;
  language: 'c' | 'typescript' | 'json' | 'markdown' | 'assembly' | 'text';
  content: string;
  isDirty?: boolean;
}

/* 6. App Store */
export interface StoreAppItem {
  id: string;
  name: string;
  developer: string;
  icon: string;
  category: 'Developer' | 'Productivity' | 'Media' | 'Utilities' | 'Security';
  rating: number;
  downloads: string;
  size: string;
  installed: boolean;
  description: string;
  version: string;
}

/* 7. End-to-End Architectural Pipeline Telemetry */
export interface PipelineStage {
  id: string;
  name: string;
  subsystem: string;
  ring: 'Ring 0' | 'Ring 3' | 'V8 Host' | 'Electron IPC';
  status: 'ONLINE' | 'ACTIVE' | 'SYNCED' | 'STANDBY';
  latencyMs: number;
  throughput: string;
  description: string;
}

export interface DnsQueryLog {
  domain: string;
  resolvedIp: string;
  ttl: number;
  time: string;
  status: 'SUCCESS' | 'FILTERED_FIREWALL' | 'CACHED';
}

export interface V8Telemetry {
  heapUsedMb: number;
  heapTotalMb: number;
  jitCompilations: number;
  ipcMessagesPerSec: number;
  gcPauseMs: number;
}
