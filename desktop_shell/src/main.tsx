import React from 'react';
import ReactDOM from 'react-dom/client';
import { DesktopShell } from './components/DesktopShell';
import './index.css';

const rootEl = document.getElementById('root');
if (rootEl) {
  const root = ReactDOM.createRoot(rootEl);
  root.render(
    <React.StrictMode>
      <DesktopShell />
    </React.StrictMode>
  );
}
