import React, { useState, useEffect, useRef } from 'react';
import { 
  Play, Pause, SkipBack, SkipForward, Volume2, VolumeX, Repeat, 
  Shuffle, Disc, Music, Film, Sliders, List, Maximize2, Sparkles
} from 'lucide-react';
import { TrackItem } from '../types';

export const MediaPlayerApp: React.FC = () => {
  const [tracks, setTracks] = useState<TrackItem[]>([
    {
      id: 't-1',
      title: 'Neon Cyberwave Odyssey (60 FPS)',
      artist: 'Xenithra Audio Core',
      album: 'Kernel Synthwave Vol. 1',
      duration: 214,
      genre: 'Synthwave / Electronic',
      type: 'audio',
      freqPattern: [40, 65, 80, 95, 70, 85, 60, 75, 90, 68, 55, 78, 88, 62, 50, 45],
    },
    {
      id: 't-2',
      title: 'Polyphonic System Boot Chime',
      artist: 'Xenithra Hardware Synthesizer',
      album: 'UEFI GOP Startup Suite',
      duration: 38,
      genre: 'Ambient / Sound FX',
      type: 'audio',
      freqPattern: [80, 90, 75, 60, 50, 45, 65, 70, 85, 90, 95, 80, 70, 60, 50, 40],
    },
    {
      id: 't-3',
      title: 'Lo-Fi Deep Work Coding Beats',
      artist: 'V8 JIT Chillhop',
      album: 'Electron Calm Session',
      duration: 185,
      genre: 'Lo-Fi / Hip-Hop',
      type: 'audio',
      freqPattern: [30, 45, 55, 65, 50, 40, 55, 60, 70, 55, 40, 35, 45, 50, 40, 30],
    },
    {
      id: 't-4',
      title: 'Matrix Sector Stream (4K Demo)',
      artist: 'DirectX / Framebuffer Stream',
      album: 'Raw DMA Bitstream',
      duration: 120,
      genre: 'Visual Canvas Video',
      type: 'video',
      freqPattern: [90, 85, 95, 100, 80, 90, 85, 95, 75, 80, 90, 85, 95, 70, 80, 90],
    }
  ]);

  const [currentTrackIndex, setCurrentTrackIndex] = useState(0);
  const [isPlaying, setIsPlaying] = useState(true);
  const [currentTime, setCurrentTime] = useState(24);
  const [volume, setVolume] = useState(85);
  const [isMuted, setIsMuted] = useState(false);
  const [isShuffle, setIsShuffle] = useState(false);
  const [isLoop, setIsLoop] = useState(true);
  const [equalizerPreset, setEqualizerPreset] = useState<'Flat' | 'Bass Boost' | 'Electronic' | 'Rock'>('Electronic');
  const [showPlaylist, setShowPlaylist] = useState(true);
  const [showEq, setShowEq] = useState(false);

  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  const currentTrack = tracks[currentTrackIndex] || tracks[0];

  // Playback timer simulation
  useEffect(() => {
    let interval: any = null;
    if (isPlaying) {
      interval = setInterval(() => {
        setCurrentTime((prev) => {
          if (prev >= currentTrack.duration) {
            if (isLoop) {
              return 0;
            } else {
              handleNext();
              return 0;
            }
          }
          return prev + 1;
        });
      }, 1000);
    }
    return () => clearInterval(interval);
  }, [isPlaying, currentTrack, isLoop]);

  // Canvas visualizer animation loop
  useEffect(() => {
    let animId: number;
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    let tick = 0;
    const render = () => {
      tick++;
      ctx.clearRect(0, 0, canvas.width, canvas.height);

      const numBars = 32;
      const barWidth = (canvas.width - numBars * 4) / numBars;

      for (let i = 0; i < numBars; i++) {
        let heightMultiplier = isPlaying ? Math.sin(tick * 0.08 + i * 0.3) * 0.4 + 0.6 : 0.08;
        if (equalizerPreset === 'Bass Boost' && i < 8) heightMultiplier *= 1.4;
        if (equalizerPreset === 'Electronic' && (i < 6 || i > 24)) heightMultiplier *= 1.3;

        const baseHeight = (currentTrack.freqPattern ? currentTrack.freqPattern[i % currentTrack.freqPattern.length] : 50) * 1.2;
        const height = Math.min(canvas.height - 10, baseHeight * heightMultiplier);

        const x = i * (barWidth + 4) + 4;
        const y = canvas.height - height;

        // Dynamic gradient
        const grad = ctx.createLinearGradient(0, canvas.height, 0, y);
        grad.addColorStop(0, '#2563eb');
        grad.addColorStop(0.5, '#06b6d4');
        grad.addColorStop(1, '#a855f7');

        ctx.fillStyle = grad;
        ctx.beginPath();
        ctx.roundRect(x, y, barWidth, height, [4, 4, 0, 0]);
        ctx.fill();
      }

      animId = requestAnimationFrame(render);
    };

    render();
    return () => cancelAnimationFrame(animId);
  }, [isPlaying, currentTrack, equalizerPreset]);

  const handleNext = () => {
    if (isShuffle) {
      const nextIdx = Math.floor(Math.random() * tracks.length);
      setCurrentTrackIndex(nextIdx);
    } else {
      setCurrentTrackIndex((prev) => (prev + 1) % tracks.length);
    }
    setCurrentTime(0);
  };

  const handlePrev = () => {
    setCurrentTrackIndex((prev) => (prev - 1 + tracks.length) % tracks.length);
    setCurrentTime(0);
  };

  const formatTime = (secs: number) => {
    const m = Math.floor(secs / 60);
    const s = Math.floor(secs % 60);
    return `${m}:${s.toString().padStart(2, '0')}`;
  };

  return (
    <div className="flex flex-col h-full w-full bg-[#080D1A] text-slate-100 select-none overflow-hidden font-sans">
      {/* 1. Header Toolbar */}
      <div className="h-12 bg-[#050914] px-4 flex items-center justify-between border-b border-slate-800/80">
        <div className="flex items-center gap-2">
          <div className="w-7 h-7 rounded-lg bg-orange-500/20 text-orange-400 flex items-center justify-center font-bold">
            <Disc className="w-4 h-4" />
          </div>
          <span className="text-xs font-bold text-slate-200">VLC Media Player & Audio Synthesizer</span>
        </div>

        <div className="flex items-center gap-2 text-xs">
          <button
            onClick={() => setShowEq(!showEq)}
            className={`px-3 py-1 rounded-lg flex items-center gap-1.5 transition-all ${
              showEq ? 'bg-blue-600 text-white' : 'bg-slate-800/60 text-slate-300 hover:bg-slate-800'
            }`}
          >
            <Sliders className="w-3.5 h-3.5" />
            Equalizer: {equalizerPreset}
          </button>
          <button
            onClick={() => setShowPlaylist(!showPlaylist)}
            className={`px-3 py-1 rounded-lg flex items-center gap-1.5 transition-all ${
              showPlaylist ? 'bg-blue-600 text-white' : 'bg-slate-800/60 text-slate-300 hover:bg-slate-800'
            }`}
          >
            <List className="w-3.5 h-3.5" />
            Playlist ({tracks.length})
          </button>
        </div>
      </div>

      {/* 2. Main Player Deck & Visualizer */}
      <div className="flex-1 flex overflow-hidden">
        {/* Visualizer & Track Info Area */}
        <div className="flex-1 flex flex-col items-center justify-center p-6 space-y-6 relative overflow-hidden bg-gradient-to-b from-[#0A1122] to-[#060A14]">
          {/* Subtle background glow */}
          <div className="absolute w-72 h-72 rounded-full bg-blue-600/10 blur-[90px] pointer-events-none" />

          {/* Album Vinyl / Art Display */}
          <div className="relative group">
            <div className={`w-44 h-44 rounded-2xl bg-gradient-to-tr from-blue-600 via-indigo-700 to-purple-600 p-1 shadow-2xl flex items-center justify-center ${isPlaying ? 'shadow-blue-500/30' : ''}`}>
              <div className="w-full h-full bg-[#0E172A] rounded-xl flex flex-col items-center justify-center relative overflow-hidden">
                {currentTrack.type === 'video' ? (
                  <Film className="w-16 h-16 text-cyan-400" />
                ) : (
                  <Music className={`w-16 h-16 text-blue-400 transition-transform duration-700 ${isPlaying ? 'scale-110' : 'scale-90'}`} />
                )}
                <div className="text-[10px] text-slate-400 mt-2 font-mono">{currentTrack.genre}</div>
              </div>
            </div>
            {/* Spinning disc ring */}
            <div className={`absolute -inset-1 rounded-2xl border border-cyan-400/30 pointer-events-none ${isPlaying ? 'animate-pulse' : ''}`} />
          </div>

          {/* Track Details */}
          <div className="text-center space-y-1 z-10">
            <h3 className="text-base font-bold text-white drop-shadow">{currentTrack.title}</h3>
            <p className="text-xs text-cyan-300 font-medium">{currentTrack.artist} — <span className="text-slate-400">{currentTrack.album}</span></p>
          </div>

          {/* Real-time Spectrum Canvas Visualizer */}
          <div className="w-full max-w-xl h-28 bg-[#040810]/80 rounded-2xl border border-slate-800/80 p-3 shadow-inner z-10 flex flex-col justify-between">
            <div className="flex justify-between text-[10px] text-slate-500 font-mono">
              <span>20 Hz [SUB]</span>
              <span>1 kHz [MID]</span>
              <span>20 kHz [HIGH]</span>
            </div>
            <canvas
              ref={canvasRef}
              width={540}
              height={70}
              className="w-full h-16 rounded"
            />
          </div>
        </div>

        {/* Side Playlist Drawer */}
        {showPlaylist && (
          <div className="w-72 bg-[#060A14] border-l border-slate-800 p-4 flex flex-col justify-between select-none">
            <div className="space-y-3">
              <h4 className="text-xs font-bold text-slate-300 uppercase tracking-wider">Queue Tracks</h4>
              <div className="space-y-1.5 overflow-y-auto max-h-[380px]">
                {tracks.map((track, idx) => {
                  const isActive = idx === currentTrackIndex;
                  return (
                    <div
                      key={track.id}
                      onClick={() => {
                        setCurrentTrackIndex(idx);
                        setCurrentTime(0);
                        setIsPlaying(true);
                      }}
                      className={`p-2.5 rounded-xl cursor-pointer transition-all flex items-center gap-3 ${
                        isActive
                          ? 'bg-blue-600/30 border border-blue-500/50 text-white'
                          : 'bg-slate-900/40 hover:bg-slate-800/60 text-slate-300 border border-transparent'
                      }`}
                    >
                      <span className="text-xs font-mono text-cyan-400">{idx + 1}</span>
                      <div className="flex-1 min-w-0">
                        <div className="text-xs font-semibold truncate">{track.title}</div>
                        <div className="text-[10px] text-slate-400 truncate">{track.artist}</div>
                      </div>
                      <span className="text-[10px] font-mono text-slate-400">{formatTime(track.duration)}</span>
                    </div>
                  );
                })}
              </div>
            </div>

            {/* Equalizer Quick Switcher */}
            {showEq && (
              <div className="p-3 bg-[#0B1220] border border-slate-800 rounded-xl space-y-2">
                <div className="text-[10px] font-bold text-cyan-400 uppercase">Hardware DSP Presets</div>
                <div className="grid grid-cols-2 gap-1.5">
                  {(['Flat', 'Bass Boost', 'Electronic', 'Rock'] as const).map((eq) => (
                    <button
                      key={eq}
                      onClick={() => setEqualizerPreset(eq)}
                      className={`px-2 py-1 rounded text-[11px] font-medium transition-all ${
                        equalizerPreset === eq ? 'bg-blue-600 text-white' : 'bg-slate-800 text-slate-400 hover:text-white'
                      }`}
                    >
                      {eq}
                    </button>
                  ))}
                </div>
              </div>
            )}
          </div>
        )}
      </div>

      {/* 3. Bottom Playback Deck & Scrub Bar */}
      <div className="h-20 bg-[#060A14] border-t border-slate-800 px-6 flex flex-col justify-center gap-2">
        {/* Scrub Bar */}
        <div className="flex items-center gap-3">
          <span className="text-[10px] font-mono text-slate-400 w-10 text-right">{formatTime(currentTime)}</span>
          <div 
            onClick={(e) => {
              const rect = e.currentTarget.getBoundingClientRect();
              const pos = (e.clientX - rect.left) / rect.width;
              setCurrentTime(Math.floor(pos * currentTrack.duration));
            }}
            className="flex-1 bg-slate-800 h-1.5 hover:h-2 rounded-full cursor-pointer relative overflow-hidden transition-all group"
          >
            <div 
              className="h-full bg-gradient-to-r from-blue-500 to-cyan-400 rounded-full group-hover:from-blue-400 group-hover:to-cyan-300"
              style={{ width: `${(currentTime / currentTrack.duration) * 100}%` }}
            />
          </div>
          <span className="text-[10px] font-mono text-slate-400 w-10">{formatTime(currentTrack.duration)}</span>
        </div>

        {/* Controls Row */}
        <div className="flex items-center justify-between">
          {/* Left Toggles */}
          <div className="flex items-center gap-2">
            <button
              onClick={() => setIsShuffle(!isShuffle)}
              className={`p-2 rounded-lg transition-colors ${isShuffle ? 'text-cyan-400' : 'text-slate-500 hover:text-slate-300'}`}
              title="Shuffle"
            >
              <Shuffle className="w-4 h-4" />
            </button>
            <button
              onClick={() => setIsLoop(!isLoop)}
              className={`p-2 rounded-lg transition-colors ${isLoop ? 'text-cyan-400' : 'text-slate-500 hover:text-slate-300'}`}
              title="Repeat"
            >
              <Repeat className="w-4 h-4" />
            </button>
          </div>

          {/* Center Playback Buttons */}
          <div className="flex items-center gap-3">
            <button
              onClick={handlePrev}
              className="p-2 hover:bg-slate-800 rounded-full text-slate-300 hover:text-white transition-colors"
            >
              <SkipBack className="w-4 h-4" />
            </button>
            <button
              onClick={() => setIsPlaying(!isPlaying)}
              className="w-10 h-10 rounded-full bg-gradient-to-tr from-blue-600 to-cyan-500 hover:from-blue-500 hover:to-cyan-400 flex items-center justify-center text-white shadow-lg shadow-blue-600/30 transition-all active:scale-95"
            >
              {isPlaying ? <Pause className="w-4 h-4" /> : <Play className="w-4 h-4 ml-0.5" />}
            </button>
            <button
              onClick={handleNext}
              className="p-2 hover:bg-slate-800 rounded-full text-slate-300 hover:text-white transition-colors"
            >
              <SkipForward className="w-4 h-4" />
            </button>
          </div>

          {/* Right Volume Controls */}
          <div className="flex items-center gap-2 w-36">
            <button
              onClick={() => setIsMuted(!isMuted)}
              className="text-slate-400 hover:text-white transition-colors"
            >
              {isMuted || volume === 0 ? <VolumeX className="w-4 h-4 text-red-400" /> : <Volume2 className="w-4 h-4" />}
            </button>
            <input
              type="range"
              min="0"
              max="100"
              value={isMuted ? 0 : volume}
              onChange={(e) => {
                setVolume(Number(e.target.value));
                if (isMuted) setIsMuted(false);
              }}
              className="w-full h-1 bg-slate-800 rounded-lg appearance-none cursor-pointer accent-cyan-400"
            />
            <span className="text-[10px] font-mono text-slate-400 w-7">{isMuted ? '0%' : `${volume}%`}</span>
          </div>
        </div>
      </div>
    </div>
  );
};
