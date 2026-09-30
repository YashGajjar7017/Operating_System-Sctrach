/**
 * @file types.ts — Xenithra OS v3.0 Render Engine — Global Types
 *
 * Extends the original desktop_shell types with:
 *   - NetworkPanel types (WiFi, Ethernet, VPN, DNS)
 *   - ManagementPanel types (devices, performance, event log)
 *   - DriverManager types (PnP devices, driver info)
 *   - Enhanced Window types (snap zones, compositor state)
 */

// ── Window Management ────────────────────────────────────────────────────────

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
  accentColor?: string; // per-window Fluent accent
  isFocused?: boolean;
  opacity?: number;
}

// ── Services ─────────────────────────────────────────────────────────────────

export type ServiceStatus = 'Running' | 'Stopped' | 'Paused' | 'Starting' | 'Stopping';
export type StartupType   = 'Automatic' | 'Manual' | 'Disabled';

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

// ── Security / Defender ───────────────────────────────────────────────────────

export type ScanType        = 'quick' | 'full' | 'memory' | 'custom';
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

// ── Browser ───────────────────────────────────────────────────────────────────

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

export interface BookmarkItem { id: string; title: string; url: string; icon: string; }
export interface HistoryItem  { id: string; title: string; url: string; time: string; }

// ── Media Player ──────────────────────────────────────────────────────────────

export interface TrackItem {
  id: string;
  title: string;
  artist: string;
  album: string;
  duration: number;
  genre: string;
  coverArt?: string;
  freqPattern?: number[];
  type: 'audio' | 'video';
  videoUrl?: string;
}

// ── Editor ────────────────────────────────────────────────────────────────────

export interface EditorFile {
  id: string;
  name: string;
  path: string;
  language: 'c' | 'typescript' | 'json' | 'markdown' | 'assembly' | 'text';
  content: string;
  isDirty?: boolean;
}

// ── Store ─────────────────────────────────────────────────────────────────────

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

// ── Pipeline / Telemetry ──────────────────────────────────────────────────────

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

// ── Web3 ──────────────────────────────────────────────────────────────────────

export interface Web3WalletInfo {
  uuid: string;
  name: string;
  icon: string;
  rdns: string;
  connected: boolean;
  account: string | null;
  chainId: number | null;
}

export interface JsonRpcRequest  { jsonrpc: '2.0'; id: number; method: string; params: unknown[]; }
export interface JsonRpcResponse<T> { jsonrpc: '2.0'; id: number; result?: T; error?: { code: number; message: string; data?: unknown }; }

export interface EIP1559Transaction {
  chainId: string; nonce: string; maxPriorityFeePerGas: string; maxFeePerGas: string;
  gasLimit: string; to: string; value: string; data: string;
}

export interface EIP712TypedData {
  types: Record<string, Array<{ name: string; type: string }>>;
  primaryType: string;
  domain: { name: string; version: string; chainId: number; verifyingContract: string };
  message: Record<string, unknown>;
}

export interface EnsRecord { domain: string; node: string; resolverAddress: string; contenthash: string; rawContenthash: string; }
export interface IpfsCid   { cid: string; url: string; gateway: string; size?: string; }
export interface RpcCallLog { id: number; method: string; params: string; result: string; status: 'pending' | 'success' | 'error'; latencyMs: number; timestamp: string; }

// ── NEW: Network Panel ────────────────────────────────────────────────────────

export type NetworkInterfaceType = 'wifi' | 'ethernet' | 'vpn' | 'loopback' | 'tunnel';
export type NetworkStatus = 'connected' | 'disconnected' | 'connecting' | 'limited' | 'error';

export interface NetworkInterface {
  id: string;
  name: string;
  displayName: string;
  type: NetworkInterfaceType;
  status: NetworkStatus;
  ipv4: string;
  ipv6?: string;
  mac: string;
  gateway: string;
  dns: string[];
  subnet: string;
  speed: string;          // e.g. "1 Gbps", "867 Mbps"
  signalStrength?: number; // 0–100 for WiFi
  ssid?: string;           // WiFi network name
  encrypted?: boolean;
  rxBytes: number;
  txBytes: number;
  rxSpeed: string;
  txSpeed: string;
  driverName: string;
  driverVersion: string;
}

export interface WifiNetwork {
  bssid: string;
  ssid: string;
  signalStrength: number; // 0–100
  frequency: number;      // MHz
  channel: number;
  security: 'Open' | 'WEP' | 'WPA' | 'WPA2' | 'WPA3';
  band: '2.4 GHz' | '5 GHz' | '6 GHz';
  vendor?: string;
  connected: boolean;
}

export interface VpnProfile {
  id: string;
  name: string;
  server: string;
  protocol: 'OpenVPN' | 'WireGuard' | 'IKEv2' | 'L2TP' | 'PPTP';
  status: 'connected' | 'disconnected' | 'connecting';
  ip?: string;
  bytesIn?: number;
  bytesOut?: number;
}

export interface DnsServer {
  ip: string;
  label: string;
  latencyMs?: number;
  status: 'active' | 'backup' | 'unreachable';
}

export interface NetworkFirewallRule {
  id: string;
  name: string;
  direction: 'inbound' | 'outbound';
  protocol: 'TCP' | 'UDP' | 'ICMP' | 'ANY';
  localPort: string;
  remotePort: string;
  action: 'ALLOW' | 'BLOCK';
  enabled: boolean;
  profileScope: 'domain' | 'private' | 'public';
}

// ── NEW: Management Panel ─────────────────────────────────────────────────────

export interface PnpDevice {
  id: string;
  instanceId: string;
  name: string;
  description: string;
  manufacturer: string;
  deviceClass: string;  // 'Display', 'Network Adapter', 'USB Controller', etc.
  status: 'OK' | 'Error' | 'Disabled' | 'Unknown' | 'Not Started';
  driverName: string;
  driverVersion: string;
  driverDate: string;
  driverSigned: boolean;
  hardwareId: string;
  location: string;     // e.g. 'PCI Slot 3'
  irq?: number;
  ioPort?: string;
  memRange?: string;
}

export interface PerformanceSnapshot {
  timestamp: number;
  cpuPercent: number;
  cpuFreqMhz: number;
  cpuTemperature: number;
  ramUsedMb: number;
  ramTotalMb: number;
  gpuPercent: number;
  gpuVramUsedMb: number;
  gpuVramTotalMb: number;
  diskReadMbs: number;
  diskWriteMbs: number;
  netRxMbs: number;
  netTxMbs: number;
  processCount: number;
  threadCount: number;
  handleCount: number;
}

export interface EventLogEntry {
  id: number;
  timestamp: string;
  level: 'Information' | 'Warning' | 'Error' | 'Critical' | 'Verbose';
  source: string;
  eventId: number;
  category: string;
  message: string;
  details?: string;
}

export interface InstalledProgram {
  id: string;
  name: string;
  version: string;
  publisher: string;
  installDate: string;
  installLocation: string;
  sizeMb: number;
  uninstallCommand?: string;
}

export interface StartupEntry {
  id: string;
  name: string;
  publisher: string;
  command: string;
  location: 'Registry' | 'Startup Folder' | 'Task Scheduler';
  enabled: boolean;
  impact: 'Low' | 'Medium' | 'High' | 'Not Measured';
  lastRunTime?: string;
}

// ── Kernel IPC Events ─────────────────────────────────────────────────────────

export interface KernelMouseEvent  { type: 'mouse'; x: number; y: number; l: number; r: number; m: number; }
export interface KernelKeyEvent    { type: 'key'; ascii: number; scan: number; pressed: number; }
export interface KernelWindowEvent { type: 'window'; action: string; tag: string; title: string; }
export interface KernelServiceEvent { type: 'service'; name: string; cpu: number; mem: number; status: string; }
export interface KernelRtosEvent   { type: 'rtos'; tid: number; prio: number; state: string; cpu_us: number; affinity: number; }
export interface KernelNetworkEvent { type: 'network'; iface: string; ip: string; gateway: string; dns: string; netStatus: string; }
export interface KernelDriverEvent  { type: 'driver'; device: string; vendor: string; driverStatus: string; version: string; }
export interface KernelNotifyEvent  { type: 'notify'; title: string; body: string; icon: string; }

export type KernelEvent =
  | KernelMouseEvent | KernelKeyEvent | KernelWindowEvent
  | KernelServiceEvent | KernelRtosEvent | KernelNetworkEvent
  | KernelDriverEvent | KernelNotifyEvent;
