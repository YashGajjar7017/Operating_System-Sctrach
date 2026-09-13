import React, { useState } from 'react';
import { Download, Check, Star, Search, Package, Sparkles, RefreshCw } from 'lucide-react';
import { StoreAppItem } from '../types';

export const StoreApp: React.FC = () => {
  const [activeCategory, setActiveCategory] = useState<string>('All');
  const [searchQuery, setSearchQuery] = useState('');
  const [installingId, setInstallingId] = useState<string | null>(null);

  const [apps, setApps] = useState<StoreAppItem[]>([
    {
      id: 'app-edge',
      name: 'Microsoft Edge Browser',
      developer: 'Microsoft Corporation',
      icon: '🌐',
      category: 'Productivity',
      rating: 4.8,
      downloads: '10M+',
      size: '95 MB',
      installed: true,
      description: 'Fast, secure browser with built-in Microsoft Defender SmartScreen & Google search.',
      version: '128.0.2739',
    },
    {
      id: 'app-vlc',
      name: 'VLC Media Player',
      developer: 'VideoLAN Organization',
      icon: '🟠',
      category: 'Media',
      rating: 4.9,
      downloads: '50M+',
      size: '42 MB',
      installed: true,
      description: 'Plays all audio, video formats, streams, and bare-metal frequency synthesizers.',
      version: '3.0.20',
    },
    {
      id: 'app-vscode',
      name: 'VS Code Lite',
      developer: 'Microsoft',
      icon: '💻',
      category: 'Developer',
      rating: 4.9,
      downloads: '25M+',
      size: '88 MB',
      installed: true,
      description: 'Code editor with C/C++, TypeScript, Assembly syntax highlighting and Git tools.',
      version: '1.92.2',
    },
    {
      id: 'app-dcloner',
      name: 'Raw Sector Bitstream Cloner',
      developer: 'Xenithra Storage Labs',
      icon: '💽',
      category: 'Utilities',
      rating: 4.7,
      downloads: '500K+',
      size: '12 MB',
      installed: true,
      description: 'Byte-for-byte physical drive imaging and sector mirroring across NVMe/AHCI.',
      version: '2.1.0',
    },
    {
      id: 'app-postman',
      name: 'Postman API Platform',
      developer: 'Postman Inc.',
      icon: '📮',
      category: 'Developer',
      rating: 4.6,
      downloads: '8M+',
      size: '110 MB',
      installed: false,
      description: 'API testing, mock servers, and socket network debugging tool.',
      version: '11.4.0',
    },
    {
      id: 'app-vbox',
      name: 'Oracle VM VirtualBox',
      developer: 'Oracle Corporation',
      icon: '📦',
      category: 'Utilities',
      rating: 4.5,
      downloads: '30M+',
      size: '160 MB',
      installed: false,
      description: 'Powerful x86 and AMD64/Intel64 virtualization product for enterprise and home.',
      version: '7.0.18',
    },
    {
      id: 'app-localsend',
      name: 'LocalSend LAN Share',
      developer: 'LocalSend Community',
      icon: '📲',
      category: 'Productivity',
      rating: 4.9,
      downloads: '2M+',
      size: '18 MB',
      installed: false,
      description: 'An open source app for sharing files and messages between nearby devices.',
      version: '1.14.0',
    },
  ]);

  const handleToggleInstall = (id: string) => {
    setInstallingId(id);
    setTimeout(() => {
      setApps((prev) =>
        prev.map((a) => (a.id === id ? { ...a, installed: !a.installed } : a))
      );
      setInstallingId(null);
    }, 800);
  };

  const filteredApps = apps.filter((a) => {
    const matchesSearch =
      a.name.toLowerCase().includes(searchQuery.toLowerCase()) ||
      a.description.toLowerCase().includes(searchQuery.toLowerCase());
    const matchesCat = activeCategory === 'All' || a.category === activeCategory;
    return matchesSearch && matchesCat;
  });

  return (
    <div className="flex flex-col h-full w-full bg-[#0B111E] text-slate-100 select-none overflow-hidden font-sans">
      {/* Header */}
      <div className="h-14 bg-[#080D18] px-6 flex items-center justify-between border-b border-slate-800">
        <div className="flex items-center gap-3">
          <div className="w-8 h-8 rounded-lg bg-gradient-to-tr from-blue-600 to-cyan-500 flex items-center justify-center text-white shadow-md">
            <Package className="w-4 h-4" />
          </div>
          <div>
            <h2 className="text-sm font-bold text-slate-100">Microsoft Store (Winget Hub)</h2>
            <p className="text-[10px] text-slate-400">Discover and install verified applications for Xenithra OS</p>
          </div>
        </div>

        <div className="flex items-center gap-2 bg-[#101726] border border-slate-700/80 rounded-full px-3 py-1.5 text-xs text-slate-300 w-64">
          <Search className="w-3.5 h-3.5 text-slate-400" />
          <input
            type="text"
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            placeholder="Search apps, games, tools..."
            className="w-full bg-transparent outline-none text-[11px]"
          />
        </div>
      </div>

      {/* Categories Bar */}
      <div className="px-6 py-2.5 bg-[#0F1626] border-b border-slate-800 flex items-center gap-2 overflow-x-auto text-xs">
        {['All', 'Developer', 'Productivity', 'Media', 'Utilities'].map((cat) => (
          <button
            key={cat}
            onClick={() => setActiveCategory(cat)}
            className={`px-3 py-1 rounded-full font-medium transition-all ${
              activeCategory === cat
                ? 'bg-blue-600 text-white shadow'
                : 'text-slate-400 hover:text-slate-200 hover:bg-slate-800/60'
            }`}
          >
            {cat}
          </button>
        ))}
      </div>

      {/* App Grid */}
      <div className="flex-1 p-6 overflow-y-auto">
        <div className="grid grid-cols-2 md:grid-cols-3 gap-4">
          {filteredApps.map((app) => {
            const isInstalling = installingId === app.id;
            return (
              <div
                key={app.id}
                className="p-4 bg-slate-900/60 hover:bg-slate-900 border border-slate-800/80 hover:border-slate-700 rounded-2xl flex flex-col justify-between space-y-3 transition-all group"
              >
                <div className="flex items-start gap-3">
                  <span className="text-3xl p-2 bg-slate-800/80 rounded-xl group-hover:scale-105 transition-transform">
                    {app.icon}
                  </span>
                  <div className="flex-1 min-w-0">
                    <h4 className="text-xs font-bold text-white truncate">{app.name}</h4>
                    <div className="text-[10px] text-slate-400 truncate">{app.developer}</div>
                    <div className="flex items-center gap-2 mt-1 text-[10px] text-slate-400">
                      <span className="flex items-center gap-0.5 text-amber-400">
                        <Star className="w-3 h-3 fill-amber-400" /> {app.rating}
                      </span>
                      <span>•</span>
                      <span>{app.size}</span>
                    </div>
                  </div>
                </div>

                <p className="text-[11px] text-slate-400 line-clamp-2 leading-relaxed">
                  {app.description}
                </p>

                <div className="flex items-center justify-between pt-2 border-t border-slate-800/60 text-[10px]">
                  <span className="text-slate-500 font-mono">v{app.version}</span>
                  <button
                    onClick={() => handleToggleInstall(app.id)}
                    disabled={isInstalling}
                    className={`px-4 py-1.5 rounded-lg font-semibold flex items-center gap-1.5 transition-all ${
                      app.installed
                        ? 'bg-slate-800 hover:bg-red-600/30 text-emerald-400 hover:text-red-400 border border-slate-700'
                        : 'bg-blue-600 hover:bg-blue-500 text-white shadow-md shadow-blue-600/20'
                    }`}
                  >
                    {isInstalling ? (
                      <RefreshCw className="w-3.5 h-3.5 animate-spin" />
                    ) : app.installed ? (
                      <>
                        <Check className="w-3.5 h-3.5" /> Installed
                      </>
                    ) : (
                      <>
                        <Download className="w-3.5 h-3.5" /> Install
                      </>
                    )}
                  </button>
                </div>
              </div>
            );
          })}
        </div>
      </div>
    </div>
  );
};
