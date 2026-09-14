"""
Django Channels WebSocket Consumer for High-Frequency OS Telemetry Streaming
"""

import json
import asyncio
import psutil
from channels.generic.websocket import AsyncWebsocketConsumer

class SystemTelemetryConsumer(AsyncWebsocketConsumer):
    async def connect(self):
        await self.accept()
        self.keep_running = True
        self.broadcast_task = asyncio.create_task(self.stream_telemetry())

    async def disconnect(self, close_code):
        self.keep_running = False
        if hasattr(self, 'broadcast_task'):
            self.broadcast_task.cancel()

    async def stream_telemetry(self):
        while self.keep_running:
            try:
                vm = psutil.virtual_memory()
                payload = {
                    "cpu_percent": psutil.cpu_percent(interval=None),
                    "memory_used_mb": round(vm.used / (1024 * 1024), 1),
                    "memory_percent": vm.percent,
                    "process_count": len(psutil.pids()),
                    "active_sessions": 1
                }
                await self.send(text_data=json.dumps(payload))
                await asyncio.sleep(1.0)
            except asyncio.CancelledError:
                break
            except Exception:
                await asyncio.sleep(2.0)
