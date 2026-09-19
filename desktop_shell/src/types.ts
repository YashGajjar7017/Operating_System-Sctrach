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

/* 8. Web3 / Blockchain Architecture Types */

/** EIP-6963 Multi-Injected Provider Info */
export interface Web3WalletInfo {
  uuid: string;
  name: string;
  icon: string;          // data:image/svg+xml or data:image/png base64
  rdns: string;          // Reverse-DNS identifier: 'io.metamask', 'app.rabby'
  connected: boolean;
  account: string | null;
  chainId: number | null;
}

/** JSON-RPC 2.0 Request */
export interface JsonRpcRequest {
  jsonrpc: '2.0';
  id: number;
  method: string;
  params: unknown[];
}

/** JSON-RPC 2.0 Response */
export interface JsonRpcResponse<T> {
  jsonrpc: '2.0';
  id: number;
  result?: T;
  error?: {
    code: number;
    message: string;
    data?: unknown;
  };
}

/** EIP-1559 Transaction Type 2 Envelope */
export interface EIP1559Transaction {
  chainId: string;            // hex string: '0x1' = Ethereum mainnet
  nonce: string;              // hex string
  maxPriorityFeePerGas: string; // hex string (wei)
  maxFeePerGas: string;       // hex string (wei)
  gasLimit: string;           // hex string
  to: string;                 // 0x-prefixed 20-byte address
  value: string;              // hex string (wei)
  data: string;               // 0x-prefixed hex calldata
}

/** EIP-712 Typed Structured Data */
export interface EIP712TypedData {
  types: Record<string, Array<{ name: string; type: string }>>;
  primaryType: string;
  domain: {
    name: string;
    version: string;
    chainId: number;
    verifyingContract: string;
  };
  message: Record<string, unknown>;
}

/** ENS Domain Record */
export interface EnsRecord {
  domain: string;        // e.g. 'vitalik.eth'
  node: string;          // namehash bytes32 hex
  resolverAddress: string;
  contenthash: string;   // decoded URI: 'ipfs://Qm...' | 'ar://...'
  rawContenthash: string; // raw hex from contract
}

/** IPFS Content Identifier */
export interface IpfsCid {
  cid: string;           // e.g. 'QmXoypiz...'
  url: string;           // 'ipfs://...' or 'https://ipfs.io/ipfs/...'
  gateway: string;       // Gateway used to fetch
  size?: string;
}

/** Web3 RPC Call Log Entry */
export interface RpcCallLog {
  id: number;
  method: string;
  params: string;
  result: string;
  status: 'pending' | 'success' | 'error';
  latencyMs: number;
  timestamp: string;
}
