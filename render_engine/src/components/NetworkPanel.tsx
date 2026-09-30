/**
 * @file NetworkPanel.tsx — Xenithra OS v3.0 — Network & Connectivity Panel
 *
 * Full-featured network management panel:
 *   - Network interfaces (WiFi, Ethernet, VPN, Loopback)
 *   - WiFi scanner with signal strength
 *   - VPN profile manager
 *   - DNS configuration
 *   - Firewall rules
 *   - Real-time traffic graphs
 *   - Kernel IPC integration via window.xenithra
 */

import React, { useState, useEffect, useCallback, useRef } from 'react';
import {
  Wifi, WifiOff, Globe, Shield, Settings, RefreshCw, Plus,
  ChevronRight, Lock, Unlock, Signal, Activity, Upload, Download,
  Server, AlertTriangle, CheckCircle, XCircle, Eye, EyeOff,
  Network, Radio, Zap, HardDrive, Layers, Link2, X, Search,
  ToggleLeft, ToggleRight, Terminal, Info
} from 'lucide-react';
import type {
  NetworkInterface, WifiNetwork, VpnProfile, DnsServer, NetworkFirewallRule
} from '../types';

// ── Mock data (replaced by kernel IPC in production) ────────────────────────

const MOCK_INTERFACES: NetworkInterface[] = [
  {
    id: 'eth0', name: 'eth0', displayName: 'Ethernet 0', type: 'ethernet',
    status: 'connected', ipv4: '192.168.1.42', ipv6: 'fe80::1a2b:3c4d:5e6f:7a8b',
    mac: 'AA:BB:CC:DD:EE:01', gateway: '192.168.1.1', dns: ['8.8.8.8', '1.1.1.1'],
    subnet: '255.255.255.0', speed: '1 Gbps', rxBytes: 5_832_945_600, txBytes: 1_203_456_000,
    rxSpeed: '12.4 MB/s', txSpeed: '2.1 MB/s', driverName: 'e1000', driverVersion: '1.0.0'
  },
  {
    id: 'wlan0', name: 'wlan0', displayName: 'Wi-Fi', type: 'wifi',
    status: 'connected', ipv4: '192.168.1.55', mac: 'AA:BB:CC:DD:EE:02',
    gateway: '192.168.1.1', dns: ['8.8.8.8', '8.8.4.4'], subnet: '255.255.255.0',
    speed: '867 Mbps', signalStrength: 82, ssid: 'HomeNetwork-5G', encrypted: true,
    rxBytes: 2_345_678_900, txBytes: 456_789_000, rxSpeed: '4.2 MB/s', txSpeed: '0.8 MB/s',
    driverName: 'rtl8188ee', driverVersion: '2.3.1'
  },
  {
    id: 'vpn0', name: 'vpn0', displayName: 'VPN Tunnel', type: 'vpn',
    status: 'disconnected', ipv4: '—', mac: '—', gateway: '—', dns: [], subnet: '—',
    speed: '—', rxBytes: 0, txBytes: 0, rxSpeed: '0 B/s', txSpeed: '0 B/s',
    driverName: 'wireguard', driverVersion: '1.0.0'
  },
  {
    id: 'lo', name: 'lo', displayName: 'Loopback', type: 'loopback',
    status: 'connected', ipv4: '127.0.0.1', mac: '00:00:00:00:00:00',
    gateway: '—', dns: [], subnet: '255.0.0.0', speed: '—',
    rxBytes: 128_000, txBytes: 128_000, rxSpeed: '0 B/s', txSpeed: '0 B/s',
    driverName: 'loopback', driverVersion: 'built-in'
  },
];

const MOCK_WIFI_NETWORKS: WifiNetwork[] = [
  { bssid: 'aa:bb:cc:dd:ee:01', ssid: 'HomeNetwork-5G',    signalStrength: 92, frequency: 5180, channel: 36, security: 'WPA3', band: '5 GHz',  vendor: 'TP-Link',    connected: true },
  { bssid: 'aa:bb:cc:dd:ee:02', ssid: 'HomeNetwork-2.4G',  signalStrength: 78, frequency: 2437, channel: 6,  security: 'WPA2', band: '2.4 GHz', vendor: 'TP-Link',    connected: false },
  { bssid: 'aa:bb:cc:dd:ee:03', ssid: 'XenithraLab',        signalStrength: 65, frequency: 5745, channel: 149, security: 'WPA3', band: '5 GHz', vendor: 'Asus',       connected: false },
  { bssid: 'aa:bb:cc:dd:ee:04', ssid: 'GuestNetwork',       signalStrength: 55, frequency: 2462, channel: 11, security: 'WPA2', band: '2.4 GHz', vendor: 'Netgear',   connected: false },
  { bssid: 'aa:bb:cc:dd:ee:05', ssid: 'SecureNet-6GHz',     signalStrength: 88, frequency: 6055, channel: 5,  security: 'WPA3', band: '6 GHz',  vendor: 'UniFi',      connected: false },
  { bssid: 'aa:bb:cc:dd:ee:06', ssid: 'OpenWifi_Cafe',      signalStrength: 40, frequency: 2412, channel: 1,  security: 'Open', band: '2.4 GHz', vendor: 'Unknown',   connected: false },
];

const MOCK_VPN_PROFILES: VpnProfile[] = [
  { id: 'vpn1', name: 'Work VPN',     server: 'vpn.corp.example.com', protocol: 'WireGuard', status: 'disconnected' },
  { id: 'vpn2', name: 'Privacy Gate', server: 'us-east.privacygate.io', protocol: 'OpenVPN',  status: 'disconnected' },
  { id: 'vpn3', name: 'Home Lab VPN', server: '192.168.0.1',           protocol: 'IKEv2',    status: 'disconnected' },
];

const MOCK_DNS: DnsServer[] = [
  { ip: '8.8.8.8',   label: 'Google DNS (Primary)', latencyMs: 12, status: 'active' },
  { ip: '8.8.4.4',   label: 'Google DNS (Secondary)', latencyMs: 15, status: 'backup' },
  { ip: '1.1.1.1',   label: 'Cloudflare DNS',         latencyMs: 8,  status: 'active' },
  { ip: '9.9.9.9',   label: 'Quad9 DNS',              latencyMs: 22, status: 'backup' },
  { ip: '127.0.0.53',label: 'systemd-resolved (local)', status: 'active' },
];

const MOCK_FW_RULES: NetworkFirewallRule[] = [
  { id: 'r1', name: 'Allow HTTP Outbound',  direction: 'outbound', protocol: 'TCP',  localPort: '*',   remotePort: '80',   action: 'ALLOW', enabled: true,  profileScope: 'public'  },
  { id: 'r2', name: 'Allow HTTPS Outbound', direction: 'outbound', protocol: 'TCP',  localPort: '*',   remotePort: '443',  action: 'ALLOW', enabled: true,  profileScope: 'public'  },
  { id: 'r3', name: 'Allow DNS Outbound',   direction: 'outbound', protocol: 'UDP',  localPort: '*',   remotePort: '53',   action: 'ALLOW', enabled: true,  profileScope: 'public'  },
  { id: 'r4', name: 'Block Telnet Inbound', direction: 'inbound',  protocol: 'TCP',  localPort: '23',  remotePort: '*',    action: 'BLOCK', enabled: true,  profileScope: 'public'  },
  { id: 'r5', name: 'Allow SSH Admin',      direction: 'inbound',  protocol: 'TCP',  localPort: '22',  remotePort: '*',    action: 'ALLOW', enabled: false, profileScope: 'private' },
  { id: 'r6', name: 'Block ICMP Inbound',   direction: 'inbound',  protocol: 'ICMP', localPort: '*',   remotePort: '*',    action: 'BLOCK', enabled: true,  profileScope: 'public'  },
];

// ── Utility helpers ───────────────────────────────────────────────────────────

function formatBytes(bytes: number): string {
  if (bytes === 0) return '0 B';
  const k = 1024;
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return `${(bytes / Math.pow(k, i)).toFixed(1)} ${sizes[i]}`;
}

function signalBars(strength: number): string {
  if (strength >= 80) return '████';
  if (strength >= 60) return '███░';
  if (strength >= 40) return '██░░';
  if (strength >= 20) return '█░░░';
  return '░░░░';
}

function signalColor(strength: number): string {
  if (strength >= 70) return 'var(--clr-accent-success)';
  if (strength >= 40) return 'var(--clr-accent-warning)';
  return 'var(--clr-accent-danger)';
}

function statusColor(status: string): string {
  switch (status) {
    case 'connected':    return 'dot-green';
    case 'disconnected': return 'dot-gray';
    case 'connecting':   return 'dot-amber';
    case 'limited':      return 'dot-amber';
    case 'error':        return 'dot-red';
    default:             return 'dot-gray';
  }
}

// ── Tab definitions ───────────────────────────────────────────────────────────
type Tab = 'overview' | 'wifi' | 'vpn' | 'dns' | 'firewall';

// ── Main Component ────────────────────────────────────────────────────────────
export const NetworkPanel: React.FC = () => {
  const [tab, setTab] = useState<Tab>('overview');
  const [interfaces, setInterfaces] = useState<NetworkInterface[]>(MOCK_INTERFACES);
  const [wifiNetworks, setWifiNetworks] = useState<WifiNetwork[]>(MOCK_WIFI_NETWORKS);
  const [vpnProfiles, setVpnProfiles] = useState<VpnProfile[]>(MOCK_VPN_PROFILES);
  const [dnsServers] = useState<DnsServer[]>(MOCK_DNS);
  const [fwRules, setFwRules] = useState<NetworkFirewallRule[]>(MOCK_FW_RULES);
  const [selectedIface, setSelectedIface] = useState<string>('eth0');
  const [scanning, setScanning] = useState(false);
  const [searchQuery, setSearchQuery] = useState('');
  const [showPassword, setShowPassword] = useState(false);
  const [trafficHistory, setTrafficHistory] = useState<number[]>(Array(30).fill(0));
  const graphRef = useRef<HTMLCanvasElement>(null);

  // Simulate live traffic graph
  useEffect(() => {
    const iv = setInterval(() => {
      setTrafficHistory(prev => {
        const next = [...prev.slice(1), Math.random() * 80 + 5];
        return next;
      });
    }, 800);
    return () => clearInterval(iv);
  }, []);

  // Draw traffic graph on canvas
  useEffect(() => {
    const canvas = graphRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    ctx.clearRect(0, 0, canvas.width, canvas.height);
    const W = canvas.width, H = canvas.height;
    const pts = trafficHistory;
    const max = Math.max(...pts, 1);

    // Gradient fill
    const gradient = ctx.createLinearGradient(0, 0, 0, H);
    gradient.addColorStop(0, 'rgba(96,165,250,0.35)');
    gradient.addColorStop(1, 'rgba(96,165,250,0.00)');

    ctx.beginPath();
    pts.forEach((v, i) => {
      const x = (i / (pts.length - 1)) * W;
      const y = H - (v / max) * (H - 8) - 4;
      i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
    });
    ctx.lineTo(W, H);
    ctx.lineTo(0, H);
    ctx.closePath();
    ctx.fillStyle = gradient;
    ctx.fill();

    // Line stroke
    ctx.beginPath();
    pts.forEach((v, i) => {
      const x = (i / (pts.length - 1)) * W;
      const y = H - (v / max) * (H - 8) - 4;
      i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
    });
    ctx.strokeStyle = 'rgba(96,165,250,0.85)';
    ctx.lineWidth = 2;
    ctx.lineJoin = 'round';
    ctx.stroke();
  }, [trafficHistory]);

  const handleScanWifi = useCallback(() => {
    setScanning(true);
    setTimeout(() => setScanning(false), 2200);
  }, []);

  const handleVpnToggle = useCallback((id: string) => {
    setVpnProfiles(prev => prev.map(p => {
      if (p.id !== id) return p;
      if (p.status === 'connected') return { ...p, status: 'disconnected' };
      if (p.status === 'disconnected') return { ...p, status: 'connecting' };
      return p;
    }));
    // Simulate connection
    setTimeout(() => {
      setVpnProfiles(prev => prev.map(p =>
        p.id === id && p.status === 'connecting'
          ? { ...p, status: 'connected', ip: '10.0.0.' + Math.floor(Math.random()*254+1) }
          : p
      ));
    }, 2000);
  }, []);

  const toggleFwRule = useCallback((id: string) => {
    setFwRules(prev => prev.map(r => r.id === id ? { ...r, enabled: !r.enabled } : r));
  }, []);

  const currentIface = interfaces.find(i => i.id === selectedIface);

  const tabs: { id: Tab; label: string; icon: React.ReactNode }[] = [
    { id: 'overview', label: 'Overview',  icon: <Network size={14} /> },
    { id: 'wifi',     label: 'Wi-Fi',     icon: <Wifi size={14} /> },
    { id: 'vpn',      label: 'VPN',       icon: <Lock size={14} /> },
    { id: 'dns',      label: 'DNS',       icon: <Server size={14} /> },
    { id: 'firewall', label: 'Firewall',  icon: <Shield size={14} /> },
  ];

  return (
    <div style={{ display: 'flex', flexDirection: 'column', height: '100%', fontFamily: 'var(--font-ui)', color: 'var(--text-primary)', background: 'transparent' }}>

      {/* ── Tab Bar ──────────────────────────────────────────────────────── */}
      <div style={{ display: 'flex', gap: 2, padding: '8px 12px 0', borderBottom: '1px solid var(--border-subtle)', flexShrink: 0 }}>
        {tabs.map(t => (
          <button
            key={t.id}
            onClick={() => setTab(t.id)}
            style={{
              display: 'flex', alignItems: 'center', gap: 6,
              padding: '7px 14px 8px', borderRadius: '8px 8px 0 0',
              background: tab === t.id ? 'rgba(96,165,250,0.12)' : 'transparent',
              color: tab === t.id ? 'var(--clr-accent-primary)' : 'var(--text-secondary)',
              border: 'none',
              borderBottom: tab === t.id ? '2px solid var(--clr-accent-primary)' : '2px solid transparent',
              fontSize: 'var(--text-sm)', fontWeight: tab === t.id ? 600 : 400,
              cursor: 'pointer', transition: 'all 0.15s ease', whiteSpace: 'nowrap'
            }}
          >
            {t.icon} {t.label}
          </button>
        ))}
      </div>

      {/* ── Tab Content ──────────────────────────────────────────────────── */}
      <div style={{ flex: 1, overflow: 'auto', padding: 16 }}>

        {/* ── OVERVIEW TAB ─────────────────────────────────────────────── */}
        {tab === 'overview' && (
          <div style={{ display: 'flex', flexDirection: 'column', gap: 14 }}>

            {/* Interface List */}
            <div>
              <div className="section-header" style={{ paddingBottom: 8 }}>Network Interfaces</div>
              <div style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
                {interfaces.map(iface => (
                  <div
                    key={iface.id}
                    className="glass-card"
                    onClick={() => setSelectedIface(iface.id)}
                    style={{
                      padding: '12px 14px', cursor: 'pointer',
                      border: selectedIface === iface.id
                        ? '1px solid rgba(96,165,250,0.4)'
                        : '1px solid var(--border-subtle)',
                      background: selectedIface === iface.id
                        ? 'rgba(96,165,250,0.08)' : 'rgba(255,255,255,0.03)',
                    }}
                  >
                    <div style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
                      {/* Icon */}
                      <div style={{
                        width: 36, height: 36, borderRadius: 'var(--radius-md)',
                        background: iface.status === 'connected' ? 'rgba(96,165,250,0.15)' : 'rgba(255,255,255,0.06)',
                        display: 'flex', alignItems: 'center', justifyContent: 'center',
                        color: iface.status === 'connected' ? 'var(--clr-accent-primary)' : 'var(--text-tertiary)',
                        flexShrink: 0
                      }}>
                        {iface.type === 'wifi' ? <Wifi size={18} /> :
                         iface.type === 'vpn' ? <Lock size={18} /> :
                         iface.type === 'loopback' ? <Radio size={18} /> :
                         <Network size={18} />}
                      </div>

                      {/* Info */}
                      <div style={{ flex: 1, minWidth: 0 }}>
                        <div style={{ display: 'flex', alignItems: 'center', gap: 8, marginBottom: 2 }}>
                          <span style={{ fontWeight: 600, fontSize: 'var(--text-sm)' }}>{iface.displayName}</span>
                          {iface.ssid && (
                            <span style={{ color: 'var(--text-tertiary)', fontSize: 'var(--text-xs)' }}>• {iface.ssid}</span>
                          )}
                          <div className={`dot ${statusColor(iface.status)}`} />
                          <span style={{ fontSize: 'var(--text-xs)', color: 'var(--text-secondary)', textTransform: 'capitalize' }}>
                            {iface.status}
                          </span>
                        </div>
                        <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)', fontFamily: 'var(--font-mono)' }}>
                          {iface.ipv4} &nbsp;|&nbsp; {iface.speed} &nbsp;|&nbsp; {iface.name}
                        </div>
                      </div>

                      {/* WiFi signal bars */}
                      {iface.type === 'wifi' && iface.signalStrength !== undefined && (
                        <div style={{ fontFamily: 'monospace', fontSize: 12, color: signalColor(iface.signalStrength) }}>
                          {signalBars(iface.signalStrength)} {iface.signalStrength}%
                        </div>
                      )}

                      {/* Traffic */}
                      <div style={{ textAlign: 'right', flexShrink: 0 }}>
                        <div style={{ fontSize: 'var(--text-xs)', color: 'var(--clr-accent-success)', display: 'flex', alignItems: 'center', gap: 3 }}>
                          <Download size={10} /> {iface.rxSpeed}
                        </div>
                        <div style={{ fontSize: 'var(--text-xs)', color: 'var(--clr-accent-warning)', display: 'flex', alignItems: 'center', gap: 3 }}>
                          <Upload size={10} /> {iface.txSpeed}
                        </div>
                      </div>
                    </div>
                  </div>
                ))}
              </div>
            </div>

            {/* Selected Interface Detail */}
            {currentIface && (
              <div>
                <div className="section-header" style={{ paddingBottom: 8 }}>Interface Details — {currentIface.displayName}</div>
                <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 10 }}>
                  {[
                    ['IPv4 Address', currentIface.ipv4],
                    ['IPv6 Address', currentIface.ipv6 || 'Not assigned'],
                    ['MAC Address', currentIface.mac],
                    ['Gateway', currentIface.gateway],
                    ['Subnet Mask', currentIface.subnet],
                    ['DNS Servers', currentIface.dns.join(', ') || 'None'],
                    ['Link Speed', currentIface.speed],
                    ['Driver', `${currentIface.driverName} v${currentIface.driverVersion}`],
                    ['Data Received', formatBytes(currentIface.rxBytes)],
                    ['Data Sent', formatBytes(currentIface.txBytes)],
                  ].map(([label, value]) => (
                    <div key={label} style={{
                      background: 'rgba(255,255,255,0.03)', borderRadius: 'var(--radius-md)',
                      padding: '10px 12px', border: '1px solid var(--border-subtle)'
                    }}>
                      <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)', marginBottom: 3 }}>{label}</div>
                      <div style={{ fontSize: 'var(--text-sm)', fontFamily: 'var(--font-mono)', color: 'var(--text-primary)' }}>{value}</div>
                    </div>
                  ))}
                </div>
              </div>
            )}

            {/* Traffic Graph */}
            <div>
              <div className="section-header" style={{ paddingBottom: 8 }}>Real-time Traffic</div>
              <div style={{
                background: 'rgba(255,255,255,0.03)', borderRadius: 'var(--radius-lg)',
                border: '1px solid var(--border-subtle)', padding: 12, overflow: 'hidden'
              }}>
                <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: 8, fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)' }}>
                  <span style={{ color: 'rgba(96,165,250,0.8)' }}>● Network I/O (MB/s)</span>
                  <span>Last 30s</span>
                </div>
                <canvas ref={graphRef} width={500} height={80} style={{ width: '100%', height: 80, display: 'block' }} />
              </div>
            </div>
          </div>
        )}

        {/* ── WIFI TAB ─────────────────────────────────────────────────── */}
        {tab === 'wifi' && (
          <div style={{ display: 'flex', flexDirection: 'column', gap: 14 }}>
            <div style={{ display: 'flex', gap: 8, alignItems: 'center' }}>
              <div style={{ flex: 1, position: 'relative' }}>
                <Search size={13} style={{ position: 'absolute', left: 10, top: '50%', transform: 'translateY(-50%)', color: 'var(--text-tertiary)' }} />
                <input
                  className="input-surface"
                  placeholder="Search networks..."
                  value={searchQuery}
                  onChange={e => setSearchQuery(e.target.value)}
                  style={{ width: '100%', paddingLeft: 30 }}
                />
              </div>
              <button className="btn btn-secondary" onClick={handleScanWifi} disabled={scanning}>
                <RefreshCw size={13} className={scanning ? 'spin' : ''} />
                {scanning ? 'Scanning…' : 'Scan'}
              </button>
            </div>

            <div style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
              {wifiNetworks
                .filter(n => n.ssid.toLowerCase().includes(searchQuery.toLowerCase()))
                .sort((a, b) => b.signalStrength - a.signalStrength)
                .map(net => (
                  <div
                    key={net.bssid}
                    className="glass-card"
                    style={{
                      padding: '12px 14px',
                      border: net.connected ? '1px solid rgba(96,165,250,0.35)' : '1px solid var(--border-subtle)',
                      background: net.connected ? 'rgba(96,165,250,0.08)' : 'rgba(255,255,255,0.025)'
                    }}
                  >
                    <div style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
                      <div style={{ color: signalColor(net.signalStrength), fontFamily: 'monospace', fontSize: 13, flexShrink: 0 }}>
                        {signalBars(net.signalStrength)}
                      </div>
                      <div style={{ flex: 1, minWidth: 0 }}>
                        <div style={{ display: 'flex', alignItems: 'center', gap: 8, marginBottom: 3 }}>
                          <span style={{ fontWeight: 600, fontSize: 'var(--text-sm)' }}>{net.ssid}</span>
                          {net.connected && <span className="pill pill-blue" style={{ fontSize: 10 }}>Connected</span>}
                          {net.security === 'Open' && <Unlock size={11} style={{ color: 'var(--clr-accent-warning)' }} />}
                          {net.security !== 'Open' && <Lock size={11} style={{ color: 'var(--text-tertiary)' }} />}
                        </div>
                        <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)' }}>
                          {net.band} · Ch.{net.channel} · {net.security} · {net.signalStrength}% · {net.vendor || 'Unknown'}
                        </div>
                      </div>
                      <button className="btn btn-secondary" style={{ padding: '5px 12px', fontSize: 11 }}>
                        {net.connected ? 'Disconnect' : 'Connect'}
                      </button>
                    </div>
                  </div>
                ))
              }
            </div>
          </div>
        )}

        {/* ── VPN TAB ──────────────────────────────────────────────────── */}
        {tab === 'vpn' && (
          <div style={{ display: 'flex', flexDirection: 'column', gap: 12 }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
              <span style={{ color: 'var(--text-secondary)', fontSize: 'var(--text-sm)' }}>VPN Profiles</span>
              <button className="btn btn-secondary" style={{ padding: '5px 12px', fontSize: 12 }}>
                <Plus size={12} /> Add Profile
              </button>
            </div>
            {vpnProfiles.map(vpn => (
              <div key={vpn.id} className="glass-card" style={{ padding: '14px 16px' }}>
                <div style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
                  <div style={{
                    width: 40, height: 40, borderRadius: 'var(--radius-md)',
                    background: vpn.status === 'connected' ? 'rgba(74,222,128,0.15)' : 'rgba(255,255,255,0.06)',
                    display: 'flex', alignItems: 'center', justifyContent: 'center',
                    color: vpn.status === 'connected' ? 'var(--clr-accent-success)' : 'var(--text-tertiary)',
                    flexShrink: 0
                  }}>
                    {vpn.status === 'connected' ? <Lock size={18} /> : <Unlock size={18} />}
                  </div>
                  <div style={{ flex: 1 }}>
                    <div style={{ display: 'flex', alignItems: 'center', gap: 8, marginBottom: 2 }}>
                      <span style={{ fontWeight: 600, fontSize: 'var(--text-sm)' }}>{vpn.name}</span>
                      <span className={`pill ${vpn.status === 'connected' ? 'pill-green' : vpn.status === 'connecting' ? 'pill-amber' : 'pill-gray'}`} style={{ fontSize: 10 }}>
                        {vpn.status}
                      </span>
                    </div>
                    <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)' }}>
                      {vpn.protocol} · {vpn.server} {vpn.ip ? `· IP: ${vpn.ip}` : ''}
                    </div>
                  </div>
                  <button
                    className={`btn ${vpn.status === 'connected' ? 'btn-danger' : 'btn-primary'}`}
                    style={{ padding: '6px 16px', fontSize: 12 }}
                    onClick={() => handleVpnToggle(vpn.id)}
                    disabled={vpn.status === 'connecting'}
                  >
                    {vpn.status === 'connected' ? 'Disconnect' : vpn.status === 'connecting' ? 'Connecting…' : 'Connect'}
                  </button>
                </div>
              </div>
            ))}
          </div>
        )}

        {/* ── DNS TAB ──────────────────────────────────────────────────── */}
        {tab === 'dns' && (
          <div style={{ display: 'flex', flexDirection: 'column', gap: 12 }}>
            <div className="section-header">Configured DNS Servers</div>
            {dnsServers.map(dns => (
              <div key={dns.ip} className="glass-card" style={{ padding: '12px 14px' }}>
                <div style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
                  <Server size={16} style={{ color: dns.status === 'active' ? 'var(--clr-accent-primary)' : 'var(--text-tertiary)', flexShrink: 0 }} />
                  <div style={{ flex: 1 }}>
                    <div style={{ fontWeight: 500, fontSize: 'var(--text-sm)', marginBottom: 2 }}>{dns.label}</div>
                    <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)', fontFamily: 'var(--font-mono)' }}>{dns.ip}</div>
                  </div>
                  {dns.latencyMs !== undefined && (
                    <div style={{ fontSize: 'var(--text-xs)', color: dns.latencyMs < 15 ? 'var(--clr-accent-success)' : dns.latencyMs < 30 ? 'var(--clr-accent-warning)' : 'var(--clr-accent-danger)' }}>
                      {dns.latencyMs} ms
                    </div>
                  )}
                  <span className={`pill ${dns.status === 'active' ? 'pill-green' : dns.status === 'backup' ? 'pill-blue' : 'pill-red'}`} style={{ fontSize: 10 }}>
                    {dns.status}
                  </span>
                </div>
              </div>
            ))}
            <div style={{ marginTop: 8 }}>
              <button className="btn btn-secondary" style={{ width: '100%', justifyContent: 'center' }}>
                <Plus size={13} /> Add Custom DNS Server
              </button>
            </div>
          </div>
        )}

        {/* ── FIREWALL TAB ─────────────────────────────────────────────── */}
        {tab === 'firewall' && (
          <div style={{ display: 'flex', flexDirection: 'column', gap: 10 }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
              <div className="section-header" style={{ padding: 0 }}>Firewall Rules</div>
              <button className="btn btn-secondary" style={{ padding: '5px 12px', fontSize: 12 }}>
                <Plus size={12} /> New Rule
              </button>
            </div>
            <div style={{ display: 'flex', flexDirection: 'column', gap: 6 }}>
              {fwRules.map(rule => (
                <div key={rule.id} className="glass-card" style={{
                  padding: '10px 14px',
                  opacity: rule.enabled ? 1 : 0.55,
                  borderLeft: `3px solid ${rule.action === 'ALLOW' ? 'var(--clr-accent-success)' : 'var(--clr-accent-danger)'}`,
                }}>
                  <div style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
                    <div style={{ flex: 1, minWidth: 0 }}>
                      <div style={{ display: 'flex', alignItems: 'center', gap: 8, marginBottom: 2 }}>
                        <span style={{ fontWeight: 500, fontSize: 'var(--text-sm)' }}>{rule.name}</span>
                        <span className={`pill ${rule.action === 'ALLOW' ? 'pill-green' : 'pill-red'}`} style={{ fontSize: 10 }}>
                          {rule.action}
                        </span>
                        <span className="pill pill-gray" style={{ fontSize: 10 }}>
                          {rule.direction}
                        </span>
                      </div>
                      <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-tertiary)', fontFamily: 'var(--font-mono)' }}>
                        {rule.protocol} · Local:{rule.localPort} → Remote:{rule.remotePort} · {rule.profileScope}
                      </div>
                    </div>
                    <button
                      onClick={() => toggleFwRule(rule.id)}
                      style={{
                        background: 'none', border: 'none', cursor: 'pointer',
                        color: rule.enabled ? 'var(--clr-accent-primary)' : 'var(--text-tertiary)',
                        padding: 4
                      }}
                    >
                      {rule.enabled ? <ToggleRight size={22} /> : <ToggleLeft size={22} />}
                    </button>
                  </div>
                </div>
              ))}
            </div>
          </div>
        )}
      </div>
    </div>
  );
};
