import React, { useState, useEffect } from 'react';
import { 
  ArrowLeft, ArrowRight, RotateCw, Home, Lock, Plus, X, Search, 
  Star, MoreVertical, ShieldCheck, Download, Code, Globe, ExternalLink, Bookmark
} from 'lucide-react';
import { BrowserTab, BookmarkItem, HistoryItem } from '../types';

export const BrowserApp: React.FC = () => {
  const [tabs, setTabs] = useState<BrowserTab[]>([
    {
      id: 'tab-1',
      title: 'Google',
      url: 'https://www.google.com',
      isLoading: false,
      canGoBack: false,
      canGoForward: false,
      isSecure: true,
      contentMode: 'google',
      searchQuery: '',
    },
    {
      id: 'tab-2',
      title: 'Xenithra OS Documentation',
      url: 'https://docs.xenithra.org',
      isLoading: false,
      canGoBack: false,
      canGoForward: false,
      isSecure: true,
      contentMode: 'docs',
    }
  ]);

  const [activeTabId, setActiveTabId] = useState('tab-1');
  const [urlInput, setUrlInput] = useState('https://www.google.com');
  const [googleSearchQuery, setGoogleSearchQuery] = useState('');
  const [showDevTools, setShowDevTools] = useState(false);
  const [showBookmarks, setShowBookmarks] = useState(true);
  const [showHistory, setShowHistory] = useState(false);
  const [showDownloads, setShowDownloads] = useState(false);

  const bookmarks: BookmarkItem[] = [
    { id: 'bm-1', title: 'Google', url: 'https://www.google.com', icon: '🔍' },
    { id: 'bm-2', title: 'GitHub', url: 'https://github.com', icon: '🐙' },
    { id: 'bm-3', title: 'OSDev Wiki', url: 'https://wiki.osdev.org', icon: '💻' },
    { id: 'bm-4', title: 'Wikipedia', url: 'https://www.wikipedia.org', icon: '📚' },
    { id: 'bm-5', title: 'YouTube', url: 'https://www.youtube.com', icon: '▶️' },
    { id: 'bm-6', title: 'Xenithra Portal', url: 'https://xenithra.internal/kernel', icon: '⚡' },
  ];

  const [history, setHistory] = useState<HistoryItem[]>([
    { id: 'h-1', title: 'Google', url: 'https://www.google.com', time: '12:40 PM' },
    { id: 'h-2', title: 'Xenithra OS Kernel Architecture', url: 'https://docs.xenithra.org/kernel', time: '12:35 PM' },
    { id: 'h-3', title: 'V8 JIT Compiler Internals', url: 'https://v8.dev/docs', time: '12:15 PM' },
  ]);

  const activeTab = tabs.find((t) => t.id === activeTabId) || tabs[0];

  useEffect(() => {
    if (activeTab) {
      setUrlInput(activeTab.url);
    }
  }, [activeTabId]);

  const navigateTo = (url: string) => {
    let cleanUrl = url.trim();
    if (!cleanUrl) return;

    let targetMode: 'google' | 'url' | 'iframe' | 'portal' | 'docs' | 'search' = 'iframe';
    let targetTitle = cleanUrl;
    let query = '';

    if (!cleanUrl.startsWith('http://') && !cleanUrl.startsWith('https://')) {
      if (cleanUrl.includes('.') && !cleanUrl.includes(' ')) {
        cleanUrl = 'https://' + cleanUrl;
      } else {
        // Search query
        query = cleanUrl;
        cleanUrl = `https://www.google.com/search?q=${encodeURIComponent(query)}`;
        targetMode = 'search';
        targetTitle = `${query} - Google Search`;
      }
    }

    if (cleanUrl.includes('google.com') && !cleanUrl.includes('/search')) {
      targetMode = 'google';
      targetTitle = 'Google';
    } else if (cleanUrl.includes('google.com/search')) {
      targetMode = 'search';
      const q = new URL(cleanUrl).searchParams.get('q') || '';
      query = q;
      targetTitle = `${q} - Google Search`;
    } else if (cleanUrl.includes('docs.xenithra')) {
      targetMode = 'docs';
      targetTitle = 'Xenithra OS Documentation';
    } else if (cleanUrl.includes('xenithra.internal')) {
      targetMode = 'portal';
      targetTitle = 'Xenithra Kernel Dashboard';
    }

    setTabs((prev) =>
      prev.map((t) =>
        t.id === activeTabId
          ? {
              ...t,
              url: cleanUrl,
              title: targetTitle,
              contentMode: targetMode,
              searchQuery: query,
              isLoading: true,
              canGoBack: true,
            }
          : t
      )
    );

    // Simulate page load finished
    setTimeout(() => {
      setTabs((prev) =>
        prev.map((t) => (t.id === activeTabId ? { ...t, isLoading: false } : t))
      );
    }, 400);

    // Add to history
    setHistory((prev) => [
      {
        id: `h-${Date.now()}`,
        title: targetTitle,
        url: cleanUrl,
        time: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
      },
      ...prev,
    ]);
  };

  const handleUrlSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    navigateTo(urlInput);
  };

  const handleGoogleSearchSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (googleSearchQuery.trim()) {
      navigateTo(`https://www.google.com/search?q=${encodeURIComponent(googleSearchQuery)}`);
    }
  };

  const createTab = () => {
    const newId = `tab-${Date.now()}`;
    const newTab: BrowserTab = {
      id: newId,
      title: 'New Tab - Google',
      url: 'https://www.google.com',
      isLoading: false,
      canGoBack: false,
      canGoForward: false,
      isSecure: true,
      contentMode: 'google',
    };
    setTabs((prev) => [...prev, newTab]);
    setActiveTabId(newId);
  };

  const closeTab = (tabId: string, e: React.MouseEvent) => {
    e.stopPropagation();
    if (tabs.length === 1) return; // Keep at least one tab
    const nextTabs = tabs.filter((t) => t.id !== tabId);
    setTabs(nextTabs);
    if (activeTabId === tabId) {
      setActiveTabId(nextTabs[nextTabs.length - 1].id);
    }
  };

  return (
    <div className="flex flex-col h-full w-full bg-[#101726] text-slate-100 select-none overflow-hidden font-sans">
      {/* 1. Chrome / Edge Window Header & Tabs Bar */}
      <div className="h-10 bg-[#0A101D] flex items-end px-2 pt-1 border-b border-slate-800 gap-1">
        <div className="flex items-center gap-1 overflow-x-auto max-w-[calc(100%-140px)]">
          {tabs.map((tab) => {
            const isActive = tab.id === activeTabId;
            return (
              <div
                key={tab.id}
                onClick={() => setActiveTabId(tab.id)}
                className={`group relative flex items-center gap-2 px-3 py-1.5 rounded-t-lg text-xs cursor-pointer max-w-[200px] min-w-[140px] transition-all ${
                  isActive
                    ? 'bg-[#152033] text-white border-t border-x border-slate-700 font-medium shadow-sm'
                    : 'text-slate-400 hover:bg-slate-800/60 hover:text-slate-200'
                }`}
              >
                <span className="text-xs">
                  {tab.contentMode === 'google' || tab.contentMode === 'search' ? '🔍' : '🌐'}
                </span>
                <span className="truncate flex-1 text-[11px]">{tab.title}</span>
                {tabs.length > 1 && (
                  <button
                    onClick={(e) => closeTab(tab.id, e)}
                    className="opacity-0 group-hover:opacity-100 hover:bg-slate-700/80 p-0.5 rounded text-slate-400 hover:text-white"
                  >
                    <X className="w-3 h-3" />
                  </button>
                )}
              </div>
            );
          })}
        </div>

        {/* New Tab Button */}
        <button
          onClick={createTab}
          className="p-1.5 hover:bg-slate-800 rounded-lg text-slate-400 hover:text-white mb-1 transition-colors"
          title="New tab"
        >
          <Plus className="w-4 h-4" />
        </button>
      </div>

      {/* 2. Navigation Bar (Omnibox, Controls, SSL Indicator) */}
      <div className="h-12 bg-[#152033] px-3 flex items-center gap-2 border-b border-slate-800">
        {/* Nav Buttons */}
        <div className="flex items-center gap-1">
          <button
            onClick={() => activeTab.canGoBack && navigateTo('https://www.google.com')}
            className={`p-1.5 rounded-lg text-slate-400 transition-colors ${
              activeTab.canGoBack ? 'hover:bg-slate-800 hover:text-white' : 'opacity-40 cursor-not-allowed'
            }`}
          >
            <ArrowLeft className="w-4 h-4" />
          </button>
          <button
            className="p-1.5 rounded-lg text-slate-400 opacity-40 cursor-not-allowed"
          >
            <ArrowRight className="w-4 h-4" />
          </button>
          <button
            onClick={() => navigateTo(activeTab.url)}
            className={`p-1.5 rounded-lg text-slate-400 hover:bg-slate-800 hover:text-white transition-colors ${
              activeTab.isLoading ? 'animate-spin text-cyan-400' : ''
            }`}
          >
            <RotateCw className="w-4 h-4" />
          </button>
          <button
            onClick={() => navigateTo('https://www.google.com')}
            className="p-1.5 rounded-lg text-slate-400 hover:bg-slate-800 hover:text-white transition-colors"
          >
            <Home className="w-4 h-4" />
          </button>
        </div>

        {/* Omnibox Address Field */}
        <form onSubmit={handleUrlSubmit} className="flex-1 flex items-center">
          <div className="w-full flex items-center bg-[#0C1322] border border-slate-700/80 focus-within:border-blue-500 rounded-full px-3 py-1.5 text-xs text-slate-200 shadow-inner">
            <div className="flex items-center gap-1.5 mr-2 text-emerald-400" title="Secure TLS 1.3 Connection (Xenithra NetFilter Verified)">
              <Lock className="w-3.5 h-3.5" />
            </div>
            <input
              type="text"
              value={urlInput}
              onChange={(e) => setUrlInput(e.target.value)}
              className="flex-1 bg-transparent outline-none text-slate-100 placeholder-slate-500 font-mono text-[11px]"
              placeholder="Search Google or enter a URL (e.g. www.google.com)..."
            />
            <button
              type="button"
              onClick={() => setShowBookmarks(!showBookmarks)}
              className={`p-1 rounded-full text-slate-400 hover:text-amber-400 ${showBookmarks ? 'text-amber-400' : ''}`}
            >
              <Star className="w-3.5 h-3.5" />
            </button>
          </div>
        </form>

        {/* Quick Tools */}
        <div className="flex items-center gap-1">
          <button
            onClick={() => setShowDevTools(!showDevTools)}
            className={`p-1.5 rounded-lg transition-colors ${
              showDevTools ? 'bg-blue-600/30 text-blue-400' : 'text-slate-400 hover:bg-slate-800 hover:text-white'
            }`}
            title="Developer Tools / Packet Inspector"
          >
            <Code className="w-4 h-4" />
          </button>
          <button
            onClick={() => setShowHistory(!showHistory)}
            className="p-1.5 rounded-lg text-slate-400 hover:bg-slate-800 hover:text-white transition-colors"
            title="Browsing History"
          >
            <Bookmark className="w-4 h-4" />
          </button>
        </div>
      </div>

      {/* 3. Bookmarks Bar */}
      {showBookmarks && (
        <div className="h-8 bg-[#111A2C] px-3 flex items-center gap-2 border-b border-slate-800/80 text-[11px] text-slate-300 overflow-x-auto">
          {bookmarks.map((bm) => (
            <button
              key={bm.id}
              onClick={() => navigateTo(bm.url)}
              className="flex items-center gap-1.5 px-2.5 py-1 rounded hover:bg-slate-800/80 hover:text-white transition-all shrink-0"
            >
              <span>{bm.icon}</span>
              <span>{bm.title}</span>
            </button>
          ))}
        </div>
      )}

      {/* 4. Main Web Content Area */}
      <div className="flex-1 flex overflow-hidden relative">
        {/* Render Engine Container */}
        <div className="flex-1 flex flex-col overflow-y-auto bg-[#070D18]">
          {/* MODE: GOOGLE HOMEPAGE */}
          {activeTab.contentMode === 'google' && (
            <div className="flex-1 flex flex-col items-center justify-center p-8 select-text">
              <div className="w-full max-w-2xl flex flex-col items-center space-y-6">
                {/* Google Logo */}
                <div className="flex items-center text-5xl font-bold tracking-tight select-none">
                  <span className="text-[#4285F4]">G</span>
                  <span className="text-[#EA4335]">o</span>
                  <span className="text-[#FBBC05]">o</span>
                  <span className="text-[#4285F4]">g</span>
                  <span className="text-[#34A853]">l</span>
                  <span className="text-[#EA4335]">e</span>
                </div>

                {/* Google Search Bar */}
                <form onSubmit={handleGoogleSearchSubmit} className="w-full">
                  <div className="flex items-center bg-[#182338] border border-slate-700 hover:border-slate-500 focus-within:border-blue-500 rounded-full px-5 py-3 shadow-lg transition-all">
                    <Search className="w-5 h-5 text-slate-400 mr-3 shrink-0" />
                    <input
                      type="text"
                      value={googleSearchQuery}
                      onChange={(e) => setGoogleSearchQuery(e.target.value)}
                      placeholder="Search Google or type a URL..."
                      className="w-full bg-transparent text-sm text-slate-100 outline-none"
                      autoFocus
                    />
                    <div className="text-xs text-slate-500 font-semibold px-2 py-0.5 bg-slate-800 rounded">
                      ↵ Enter
                    </div>
                  </div>
                </form>

                {/* Search Buttons */}
                <div className="flex gap-3 text-xs">
                  <button
                    onClick={handleGoogleSearchSubmit}
                    className="px-4 py-2 bg-slate-800/80 hover:bg-slate-700 text-slate-200 rounded-md border border-slate-700/60 font-medium"
                  >
                    Google Search
                  </button>
                  <button
                    onClick={() => navigateTo('https://docs.xenithra.org')}
                    className="px-4 py-2 bg-slate-800/80 hover:bg-slate-700 text-slate-200 rounded-md border border-slate-700/60 font-medium"
                  >
                    I'm Feeling Lucky
                  </button>
                </div>

                {/* Popular Topics & Shortcuts */}
                <div className="grid grid-cols-4 gap-3 pt-6 w-full">
                  {[
                    { title: 'Operating Systems', query: 'x86_64 operating system development' },
                    { title: 'Windows 11 Fluent UI', query: 'fluent design system components' },
                    { title: 'V8 JavaScript Engine', query: 'v8 engine architecture and JIT compiler' },
                    { title: 'Kernel Security', query: 'SMEP SMAP and KASLR kernel defense' },
                  ].map((topic, i) => (
                    <div
                      key={i}
                      onClick={() => navigateTo(`https://www.google.com/search?q=${encodeURIComponent(topic.query)}`)}
                      className="p-3 bg-slate-900/60 hover:bg-slate-800/80 border border-slate-800 rounded-xl cursor-pointer transition-all text-center"
                    >
                      <div className="text-xs font-semibold text-cyan-400">{topic.title}</div>
                      <div className="text-[10px] text-slate-400 mt-1 truncate">{topic.query}</div>
                    </div>
                  ))}
                </div>
              </div>
            </div>
          )}

          {/* MODE: GOOGLE SEARCH RESULTS */}
          {activeTab.contentMode === 'search' && (
            <div className="p-8 max-w-4xl select-text space-y-6">
              {/* Header Search Info */}
              <div className="text-xs text-slate-400 border-b border-slate-800 pb-3 flex justify-between items-center">
                <span>About 4,820,000 results (0.24 seconds) for <strong className="text-white font-mono">"{activeTab.searchQuery}"</strong></span>
                <span className="text-[11px] text-emerald-400 flex items-center gap-1">
                  <ShieldCheck className="w-3.5 h-3.5" /> DNS Verified: 142.250.190.46
                </span>
              </div>

              {/* Result 1 */}
              <div className="space-y-1">
                <div className="text-[11px] text-slate-400 flex items-center gap-1">
                  <span>https://wiki.osdev.org</span>
                  <span>›</span>
                  <span>Kernel_Development</span>
                </div>
                <h3 
                  onClick={() => navigateTo('https://wiki.osdev.org')}
                  className="text-base font-semibold text-blue-400 hover:underline cursor-pointer"
                >
                  OSDev Wiki - Building 64-bit Higher Half Operating Systems
                </h3>
                <p className="text-xs text-slate-300 leading-relaxed">
                  Comprehensive guide for x86_64 higher-half kernels, UEFI GOP framebuffers, paging tables (PML4), MLFQ schedulers, and SMEP/SMAP security barriers.
                </p>
              </div>

              {/* Result 2 */}
              <div className="space-y-1">
                <div className="text-[11px] text-slate-400 flex items-center gap-1">
                  <span>https://v8.dev</span>
                  <span>›</span>
                  <span>docs</span>
                  <span>›</span>
                  <span>embed</span>
                </div>
                <h3 
                  onClick={() => navigateTo('https://v8.dev')}
                  className="text-base font-semibold text-blue-400 hover:underline cursor-pointer"
                >
                  V8 JavaScript Engine - Embedding and Bare-Metal nativeUI Bridge
                </h3>
                <p className="text-xs text-slate-300 leading-relaxed">
                  Learn how V8 compiles modern ECMAScript to machine code, interfaces with custom React reconcilers, and connects with OS kernel IPC messaging loops.
                </p>
              </div>

              {/* Result 3 */}
              <div className="space-y-1">
                <div className="text-[11px] text-slate-400 flex items-center gap-1">
                  <span>https://docs.xenithra.org</span>
                  <span>›</span>
                  <span>security</span>
                </div>
                <h3 
                  onClick={() => navigateTo('https://docs.xenithra.org')}
                  className="text-base font-semibold text-blue-400 hover:underline cursor-pointer"
                >
                  Xenithra OS Security Center & Kernel Private Firewall
                </h3>
                <p className="text-xs text-slate-300 leading-relaxed">
                  Documentation on Xenithra's anti-hijack session guard, real-time packet inspection firewall, and dynamic token rotation in Ring 0.
                </p>
              </div>
            </div>
          )}

          {/* MODE: XENITHRA DOCUMENTATION */}
          {activeTab.contentMode === 'docs' && (
            <div className="p-8 max-w-4xl select-text space-y-6">
              <div className="p-6 bg-gradient-to-r from-blue-950/60 to-slate-900/60 border border-blue-500/30 rounded-2xl">
                <h1 className="text-2xl font-bold text-white mb-2">Xenithra OS Documentation & API Reference</h1>
                <p className="text-xs text-slate-300">
                  Detailed technical specification of the Kernel &rarr; OS Start &rarr; Firewall/DNS &rarr; V8 Engine &rarr; Electron Desktop Shell architecture.
                </p>
              </div>

              <div className="grid grid-cols-2 gap-4">
                <div className="p-4 bg-slate-900/60 border border-slate-800 rounded-xl space-y-2">
                  <h3 className="text-xs font-bold text-cyan-400">1. Higher-Half 64-bit Kernel</h3>
                  <p className="text-[11px] text-slate-300">
                    Compiled with Clang <code>x86_64-unknown-none-elf</code>, booted via UEFI BOOTX64.EFI with GOP 32-bit linear framebuffer rendering.
                  </p>
                </div>

                <div className="p-4 bg-slate-900/60 border border-slate-800 rounded-xl space-y-2">
                  <h3 className="text-xs font-bold text-emerald-400">2. Kernel Private Firewall & DNS</h3>
                  <p className="text-[11px] text-slate-300">
                    Packet inspection state machine filtering Port 80/443/53 with stateful drop rules and anti-hijack token guards.
                  </p>
                </div>

                <div className="p-4 bg-slate-900/60 border border-slate-800 rounded-xl space-y-2">
                  <h3 className="text-xs font-bold text-purple-400">3. V8 Engine & Headless Reconciler</h3>
                  <p className="text-[11px] text-slate-300">
                    Direct JSX-to-framebuffer rendering using custom V8 nativeUI bridge and DOM layout engines.
                  </p>
                </div>

                <div className="p-4 bg-slate-900/60 border border-slate-800 rounded-xl space-y-2">
                  <h3 className="text-xs font-bold text-blue-400">4. Windows 11 Fluent Desktop Shell</h3>
                  <p className="text-[11px] text-slate-300">
                    React 19 + TypeScript + Tailwind CSS desktop with Mica glassmorphism, Window Manager, and live OS telemetry.
                  </p>
                </div>
              </div>
            </div>
          )}

          {/* MODE: IFRAME / REAL WEB VIEW */}
          {activeTab.contentMode === 'iframe' && (
            <div className="flex-1 w-full h-full bg-white">
              <iframe
                src={activeTab.url}
                className="w-full h-full border-none"
                title={activeTab.title}
                sandbox="allow-scripts allow-same-origin allow-forms"
              />
            </div>
          )}
        </div>

        {/* Developer Tools / Packet Inspector Side Panel */}
        {showDevTools && (
          <div className="w-80 bg-[#0A101D] border-l border-slate-800 p-4 flex flex-col justify-between select-text font-mono text-xs">
            <div className="space-y-4">
              <div className="flex justify-between items-center text-slate-200 border-b border-slate-800 pb-2">
                <span className="font-bold">DevTools & Network Inspector</span>
                <button onClick={() => setShowDevTools(false)} className="hover:text-red-400">✕</button>
              </div>

              <div className="space-y-2">
                <div className="text-[10px] uppercase text-slate-500 font-bold">Network Waterfall (DNS & Firewall)</div>
                <div className="p-2.5 bg-[#060A14] rounded-lg border border-slate-800 space-y-1.5 text-[11px]">
                  <div className="flex justify-between">
                    <span className="text-slate-400">DNS Resolution:</span>
                    <span className="text-emerald-400">1.8 ms (Dnscache)</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">Firewall Inspection:</span>
                    <span className="text-cyan-400">PASSED [Rule #2]</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">TLS Handshake:</span>
                    <span className="text-slate-200">TLS_AES_256_GCM</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-slate-400">V8 DOM Parse:</span>
                    <span className="text-purple-400">4.2 ms</span>
                  </div>
                </div>
              </div>

              <div className="space-y-2">
                <div className="text-[10px] uppercase text-slate-500 font-bold">Security Context</div>
                <div className="p-2.5 bg-[#060A14] rounded-lg border border-slate-800 text-[11px] text-slate-300 space-y-1">
                  <div>Protocol: <span className="text-white">HTTPS (Port 443)</span></div>
                  <div>Certificate: <span className="text-emerald-400">Valid (2048-bit RSA)</span></div>
                  <div>Token Sandbox: <span className="text-cyan-400">Ring 3 Enforced</span></div>
                </div>
              </div>
            </div>

            <div className="p-2 bg-slate-900 rounded text-[10px] text-slate-400 text-center">
              Xenithra NetFilter Socket Bridge
            </div>
          </div>
        )}
      </div>
    </div>
  );
};
