import React, { useState } from 'react';
import { FileCode, Save, Plus, X, Search, FileText, Check, Settings, Copy } from 'lucide-react';
import { EditorFile } from '../types';

export const EditorApp: React.FC = () => {
  const [files, setFiles] = useState<EditorFile[]>([
    {
      id: 'f-1',
      name: 'main.c',
      path: '/kernel/main.c',
      language: 'c',
      content: `/**
 * @file main.c
 * @brief Xenithra OS 64-bit Kernel Main Entry & Windows 11 Desktop Boot
 */

#include <stdint.h>
#include <stddef.h>
#include "../shared/bootinfo.h"
#include "security/session.h"
#include "security/firewall.h"
#include "sched/sched.h"

void kmain(XenithraBootInfo *boot_info) {
    /* 1. Initialize Hardware Sound & Ambient Startup Chime */
    sound_init();
    play_system_startup_chime();

    /* 2. Initialize Preemptive Multilevel Feedback Queue (MLFQ) */
    sched_init();

    /* 3. Initialize Kernel Security & Anti-Hijack Guard */
    security_init();
    security_enable_smep_smap();

    /* 4. Initialize Kernel Private Firewall & DNS Filter */
    firewall_init();

    /* 5. Initialize V8 Scripting Engine & Windows 11 Shell */
    compositor_init(boot_info->framebuffer);
    explorer_app_launch();
}`,
    },
    {
      id: 'f-2',
      name: 'firewall.c',
      path: '/kernel/security/firewall.c',
      language: 'c',
      content: `/**
 * @file firewall.c
 * @brief Stateful Deep-Packet Inspection Firewall
 */

#include "firewall.h"

int firewall_inspect_packet(const PacketHeader *pkt) {
    if (!pkt) return FIREWALL_DROP;

    /* Rule: Protect Port 445 SMB against WannaCry exploitation */
    if (pkt->port == 445 && pkt->protocol == PROTO_TCP) {
        firewall_log_threat(pkt->src_ip, "Blocked SMB vulnerability probe");
        return FIREWALL_DROP;
    }

    /* Allow standard HTTPS & DNS */
    if (pkt->port == 443 || pkt->port == 53) {
        return FIREWALL_ALLOW;
    }

    return FIREWALL_ALLOW;
}`,
    },
    {
      id: 'f-3',
      name: 'sysconfig.json',
      path: '/system/config/sysconfig.json',
      language: 'json',
      content: `{\n  "os_name": "Xenithra OS",\n  "version": "2.0.0-PROD",\n  "architecture": "x86_64",\n  "security": {\n    "smep_enabled": true,\n    "smap_enabled": true,\n    "session_entropy_bits": 128,\n    "firewall_default_policy": "ALLOW_TRUSTED"\n  },\n  "gui": {\n    "compositor": "fluent_mica",\n    "target_fps": 60,\n    "v8_jit_heap_mb": 512\n  }\n}`,
    },
    {
      id: 'f-4',
      name: 'notes.txt',
      path: '/desktop/notes.txt',
      language: 'text',
      content: `Xenithra OS Integration Notes:
1. Kernel boots in UEFI Higher-Half memory space.
2. OS services spin up under Ring 0 scheduler.
3. Firewall filters DNS requests before forwarding to web host.
4. V8 Engine bridges to Electron React 19 Desktop Shell.
5. All applications running smoothly!`,
    }
  ]);

  const [activeFileId, setActiveFileId] = useState('f-1');
  const [savedNotification, setSavedNotification] = useState(false);

  const activeFile = files.find((f) => f.id === activeFileId) || files[0];

  const handleContentChange = (newText: string) => {
    setFiles((prev) =>
      prev.map((f) => (f.id === activeFileId ? { ...f, content: newText, isDirty: true } : f))
    );
  };

  const handleSave = () => {
    setFiles((prev) =>
      prev.map((f) => (f.id === activeFileId ? { ...f, isDirty: false } : f))
    );
    setSavedNotification(true);
    setTimeout(() => setSavedNotification(false), 1500);
  };

  const createNewFile = () => {
    const newId = `f-${Date.now()}`;
    const newFile: EditorFile = {
      id: newId,
      name: 'untitled.txt',
      path: `/documents/untitled.txt`,
      language: 'text',
      content: '',
      isDirty: true,
    };
    setFiles((prev) => [...prev, newFile]);
    setActiveFileId(newId);
  };

  const closeFile = (id: string, e: React.MouseEvent) => {
    e.stopPropagation();
    if (files.length === 1) return;
    const nextFiles = files.filter((f) => f.id !== id);
    setFiles(nextFiles);
    if (activeFileId === id) {
      setActiveFileId(nextFiles[nextFiles.length - 1].id);
    }
  };

  const lineCount = activeFile.content.split('\n').length;

  return (
    <div className="flex flex-col h-full w-full bg-[#111827] text-slate-100 select-none overflow-hidden font-sans">
      {/* 1. Header Tab Bar */}
      <div className="h-10 bg-[#0A0F1D] flex items-end px-2 pt-1 border-b border-slate-800 gap-1">
        <div className="flex items-center gap-1 overflow-x-auto max-w-[calc(100%-120px)]">
          {files.map((f) => {
            const isActive = f.id === activeFileId;
            return (
              <div
                key={f.id}
                onClick={() => setActiveFileId(f.id)}
                className={`group flex items-center gap-2 px-3 py-1.5 rounded-t-md text-xs cursor-pointer max-w-[180px] min-w-[120px] transition-all ${
                  isActive
                    ? 'bg-[#182238] text-white border-t-2 border-blue-500 font-medium'
                    : 'text-slate-400 hover:bg-slate-800/60 hover:text-slate-200'
                }`}
              >
                <FileCode className="w-3.5 h-3.5 text-cyan-400 shrink-0" />
                <span className="truncate flex-1 text-[11px]">
                  {f.name} {f.isDirty ? '•' : ''}
                </span>
                {files.length > 1 && (
                  <button
                    onClick={(e) => closeFile(f.id, e)}
                    className="opacity-0 group-hover:opacity-100 hover:bg-slate-700/80 p-0.5 rounded text-slate-400 hover:text-white"
                  >
                    <X className="w-3 h-3" />
                  </button>
                )}
              </div>
            );
          })}
        </div>

        <button
          onClick={createNewFile}
          className="p-1 hover:bg-slate-800 rounded text-slate-400 hover:text-white mb-1"
          title="New File"
        >
          <Plus className="w-4 h-4" />
        </button>

        <div className="ml-auto flex items-center gap-2 mb-1 pr-2">
          {savedNotification && (
            <span className="text-[10px] text-emerald-400 font-bold flex items-center gap-1">
              <Check className="w-3 h-3" /> Saved
            </span>
          )}
          <button
            onClick={handleSave}
            className="px-2.5 py-1 bg-blue-600 hover:bg-blue-500 text-white rounded text-[11px] font-semibold flex items-center gap-1"
          >
            <Save className="w-3 h-3" /> Save
          </button>
        </div>
      </div>

      {/* 2. Main Code Area with Line Numbers */}
      <div className="flex-1 flex overflow-hidden font-mono text-xs select-text bg-[#0E1524]">
        {/* Line Numbers Gutter */}
        <div className="w-12 bg-[#0A0F1D] text-slate-500 p-3 text-right select-none border-r border-slate-800/80 font-mono text-[11px] leading-relaxed">
          {Array.from({ length: Math.max(lineCount, 15) }).map((_, i) => (
            <div key={i}>{i + 1}</div>
          ))}
        </div>

        {/* Text Area */}
        <textarea
          value={activeFile.content}
          onChange={(e) => handleContentChange(e.target.value)}
          spellCheck={false}
          className="flex-1 p-3 bg-transparent text-slate-100 outline-none resize-none leading-relaxed font-mono text-[11px] selection:bg-blue-600 selection:text-white"
        />
      </div>

      {/* 3. Bottom Status Bar */}
      <div className="h-6 bg-[#070B14] px-4 flex items-center justify-between text-[10px] text-slate-400 border-t border-slate-800 font-sans">
        <div className="flex items-center gap-3">
          <span className="text-cyan-400">{activeFile.path}</span>
          <span>{lineCount} lines</span>
          <span>{activeFile.content.length} characters</span>
        </div>
        <div className="flex items-center gap-4">
          <span>UTF-8</span>
          <span>Spaces: 4</span>
          <span className="text-purple-400 uppercase font-bold">{activeFile.language}</span>
        </div>
      </div>
    </div>
  );
};
