/**
 * @file main.tsx — Xenithra OS v3.0 Render Engine — React Entry Point
 */
import React from 'react';
import ReactDOM from 'react-dom/client';
import './index.css';
import { DesktopShell } from './components/DesktopShell';

ReactDOM.createRoot(document.getElementById('root')!).render(
  <React.StrictMode>
    <DesktopShell />
  </React.StrictMode>
);
