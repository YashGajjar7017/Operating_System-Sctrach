import React, { useState } from 'react';
import { Delete, History, RotateCcw } from 'lucide-react';

export const CalculatorApp: React.FC = () => {
  const [display, setDisplay] = useState('0');
  const [prevVal, setPrevVal] = useState<number | null>(null);
  const [op, setOp] = useState<string | null>(null);
  const [mode, setMode] = useState<'Standard' | 'Programmer'>('Standard');
  const [history, setHistory] = useState<string[]>([]);

  const handleDigit = (digit: string) => {
    setDisplay((prev) => (prev === '0' ? digit : prev + digit));
  };

  const handleOp = (nextOp: string) => {
    const current = parseFloat(display);
    if (prevVal === null) {
      setPrevVal(current);
    } else if (op) {
      const result = compute(prevVal, current, op);
      setPrevVal(result);
      setDisplay(String(result));
    }
    setOp(nextOp);
    setDisplay('0');
  };

  const compute = (a: number, b: number, operator: string): number => {
    switch (operator) {
      case '+': return a + b;
      case '-': return a - b;
      case '×': return a * b;
      case '÷': return b !== 0 ? a / b : 0;
      case 'AND': return (a | 0) & (b | 0);
      case 'OR': return (a | 0) | (b | 0);
      case 'XOR': return (a | 0) ^ (b | 0);
      default: return b;
    }
  };

  const handleEquals = () => {
    if (prevVal === null || !op) return;
    const current = parseFloat(display);
    const result = compute(prevVal, current, op);
    const expr = `${prevVal} ${op} ${current} = ${result}`;
    setHistory((prev) => [expr, ...prev.slice(0, 8)]);
    setDisplay(String(result));
    setPrevVal(null);
    setOp(null);
  };

  const handleClear = () => {
    setDisplay('0');
    setPrevVal(null);
    setOp(null);
  };

  const numVal = parseInt(display, 10) || 0;

  return (
    <div className="flex flex-col h-full w-full bg-[#101726] text-slate-100 select-none p-4 justify-between font-sans">
      {/* Mode Selector */}
      <div className="flex justify-between items-center pb-2 border-b border-slate-800">
        <div className="flex gap-2">
          {(['Standard', 'Programmer'] as const).map((m) => (
            <button
              key={m}
              onClick={() => setMode(m)}
              className={`px-3 py-1 rounded-lg text-xs font-semibold transition-all ${
                mode === m ? 'bg-blue-600 text-white' : 'text-slate-400 hover:text-white'
              }`}
            >
              {m}
            </button>
          ))}
        </div>
        <span className="text-[11px] text-slate-400">Windows 11 Fluent Calc</span>
      </div>

      {/* Programmer Radix Bases */}
      {mode === 'Programmer' && (
        <div className="grid grid-cols-2 gap-2 my-2 text-[11px] font-mono bg-[#090E1A] p-2.5 rounded-lg border border-slate-800">
          <div><span className="text-slate-400">HEX: </span><span className="text-cyan-400">{numVal.toString(16).toUpperCase()}</span></div>
          <div><span className="text-slate-400">DEC: </span><span className="text-white">{numVal.toString(10)}</span></div>
          <div><span className="text-slate-400">OCT: </span><span className="text-amber-400">{numVal.toString(8)}</span></div>
          <div><span className="text-slate-400">BIN: </span><span className="text-emerald-400">{numVal.toString(2)}</span></div>
        </div>
      )}

      {/* Display Screen */}
      <div className="bg-[#090E1A] p-4 rounded-xl border border-slate-800 text-right space-y-1">
        <div className="text-xs text-slate-400 font-mono h-4">
          {prevVal !== null ? `${prevVal} ${op}` : ''}
        </div>
        <div className="text-3xl font-bold font-mono text-white tracking-tight truncate">
          {display}
        </div>
      </div>

      {/* Buttons Grid */}
      <div className="grid grid-cols-4 gap-2 pt-3">
        {mode === 'Standard' ? (
          <>
            <button onClick={handleClear} className="p-3 bg-slate-800/80 hover:bg-slate-700 text-cyan-400 rounded-lg font-bold text-sm">C</button>
            <button onClick={() => setDisplay((p) => (p.startsWith('-') ? p.slice(1) : '-' + p))} className="p-3 bg-slate-800/80 hover:bg-slate-700 text-slate-200 rounded-lg font-bold text-sm">±</button>
            <button onClick={() => setDisplay((p) => String(parseFloat(p) / 100))} className="p-3 bg-slate-800/80 hover:bg-slate-700 text-slate-200 rounded-lg font-bold text-sm">%</button>
            <button onClick={() => handleOp('÷')} className="p-3 bg-blue-600/40 hover:bg-blue-600 text-cyan-300 rounded-lg font-bold text-sm">÷</button>

            {['7', '8', '9'].map((n) => (
              <button key={n} onClick={() => handleDigit(n)} className="p-3 bg-slate-900/90 hover:bg-slate-800 text-white rounded-lg font-bold text-sm">{n}</button>
            ))}
            <button onClick={() => handleOp('×')} className="p-3 bg-blue-600/40 hover:bg-blue-600 text-cyan-300 rounded-lg font-bold text-sm">×</button>

            {['4', '5', '6'].map((n) => (
              <button key={n} onClick={() => handleDigit(n)} className="p-3 bg-slate-900/90 hover:bg-slate-800 text-white rounded-lg font-bold text-sm">{n}</button>
            ))}
            <button onClick={() => handleOp('-')} className="p-3 bg-blue-600/40 hover:bg-blue-600 text-cyan-300 rounded-lg font-bold text-sm">-</button>

            {['1', '2', '3'].map((n) => (
              <button key={n} onClick={() => handleDigit(n)} className="p-3 bg-slate-900/90 hover:bg-slate-800 text-white rounded-lg font-bold text-sm">{n}</button>
            ))}
            <button onClick={() => handleOp('+')} className="p-3 bg-blue-600/40 hover:bg-blue-600 text-cyan-300 rounded-lg font-bold text-sm">+</button>

            <button onClick={() => handleDigit('0')} className="col-span-2 p-3 bg-slate-900/90 hover:bg-slate-800 text-white rounded-lg font-bold text-sm">0</button>
            <button onClick={() => !display.includes('.') && setDisplay((p) => p + '.')} className="p-3 bg-slate-900/90 hover:bg-slate-800 text-white rounded-lg font-bold text-sm">.</button>
            <button onClick={handleEquals} className="p-3 bg-blue-600 hover:bg-blue-500 text-white rounded-lg font-bold text-sm shadow-md">=</button>
          </>
        ) : (
          <>
            <button onClick={handleClear} className="p-2.5 bg-slate-800 hover:bg-slate-700 text-cyan-400 rounded-lg font-bold text-xs">CLR</button>
            <button onClick={() => handleOp('AND')} className="p-2.5 bg-slate-800 hover:bg-slate-700 text-slate-200 rounded-lg font-bold text-xs">AND</button>
            <button onClick={() => handleOp('OR')} className="p-2.5 bg-slate-800 hover:bg-slate-700 text-slate-200 rounded-lg font-bold text-xs">OR</button>
            <button onClick={() => handleOp('XOR')} className="p-2.5 bg-slate-800 hover:bg-slate-700 text-slate-200 rounded-lg font-bold text-xs">XOR</button>

            {['7', '8', '9'].map((n) => (
              <button key={n} onClick={() => handleDigit(n)} className="p-2.5 bg-slate-900 hover:bg-slate-800 text-white rounded-lg font-bold text-xs">{n}</button>
            ))}
            <button onClick={() => handleOp('+')} className="p-2.5 bg-blue-600/40 hover:bg-blue-600 text-cyan-300 rounded-lg font-bold text-xs">+</button>

            {['4', '5', '6'].map((n) => (
              <button key={n} onClick={() => handleDigit(n)} className="p-2.5 bg-slate-900 hover:bg-slate-800 text-white rounded-lg font-bold text-xs">{n}</button>
            ))}
            <button onClick={() => handleOp('-')} className="p-2.5 bg-blue-600/40 hover:bg-blue-600 text-cyan-300 rounded-lg font-bold text-xs">-</button>

            {['1', '2', '3'].map((n) => (
              <button key={n} onClick={() => handleDigit(n)} className="p-2.5 bg-slate-900 hover:bg-slate-800 text-white rounded-lg font-bold text-xs">{n}</button>
            ))}
            <button onClick={handleEquals} className="row-span-2 p-2.5 bg-blue-600 hover:bg-blue-500 text-white rounded-lg font-bold text-xs flex items-center justify-center">=</button>

            <button onClick={() => handleDigit('0')} className="col-span-3 p-2.5 bg-slate-900 hover:bg-slate-800 text-white rounded-lg font-bold text-xs">0</button>
          </>
        )}
      </div>
    </div>
  );
};
