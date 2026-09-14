"""
Xenithra OS System Controller Backend (Django / ASGI / psutil)
Provides low-level process execution, power management, telemetry, and file inspection.
"""

import os
import sys
import json
import psutil
import subprocess
from django.http import JsonResponse
from django.views.decorators.csrf import csrf_exempt
from django.views.decorators.http import require_http_methods


def get_system_stats(request):
    """
    Returns live hardware metrics and OS statistics via psutil.
    """
    vm = psutil.virtual_memory()
    disk = psutil.disk_usage('/')
    
    data = {
        "cpu_percent": psutil.cpu_percent(interval=None),
        "cpu_count": psutil.cpu_count(logical=True),
        "memory_used_mb": round(vm.used / (1024 * 1024), 1),
        "memory_total_mb": round(vm.total / (1024 * 1024), 1),
        "memory_percent": vm.percent,
        "disk_percent": disk.percent,
        "disk_free_gb": round(disk.free / (1024 * 1024 * 1024), 2),
        "process_count": len(psutil.pids()),
        "platform": sys.platform,
        "boot_time": psutil.boot_time()
    }
    return JsonResponse(data)


@csrf_exempt
@require_http_methods(["POST"])
def execute_command(request):
    """
    Spawns background or synchronous system processes safely.
    """
    try:
        body = json.loads(request.body.decode('utf-8'))
    except Exception:
        return JsonResponse({"success": False, "error": "Invalid JSON body"}, status=400)

    cmd = body.get("command", "").strip()
    args = body.get("arguments", [])
    run_detached = body.get("run_detached", True)

    if not cmd:
        return JsonResponse({"success": False, "error": "Command is required"}, status=400)

    try:
        full_command = [cmd] + (args if isinstance(args, list) else [args]) if args else [cmd]

        if run_detached:
            # Spawn non-blocking background process
            if os.name == 'nt':
                DETACHED_PROCESS = 0x00000008
                proc = subprocess.Popen(full_command, creationflags=DETACHED_PROCESS, close_fds=True)
            else:
                proc = subprocess.Popen(full_command, start_new_session=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

            return JsonResponse({"success": True, "pid": proc.pid, "message": f"Started {cmd} (PID: {proc.pid})"})
        else:
            # Synchronous execution
            res = subprocess.run(full_command, capture_output=True, text=True, timeout=10)
            return JsonResponse({
                "success": res.returncode == 0,
                "exit_code": res.returncode,
                "stdout": res.stdout.strip(),
                "stderr": res.stderr.strip()
            })
    except Exception as ex:
        return JsonResponse({"success": False, "error": str(ex)}, status=500)


@csrf_exempt
@require_http_methods(["POST"])
def power_action(request):
    """
    Executes hardware ACPI reboot or shutdown commands.
    """
    try:
        body = json.loads(request.body.decode('utf-8'))
        action = body.get("action", "").lower()
    except Exception:
        return JsonResponse({"success": False, "error": "Invalid JSON"}, status=400)

    if action in ["reboot", "restart"]:
        if os.name == 'nt':
            subprocess.Popen(["shutdown", "/r", "/t", "0"])
        else:
            subprocess.Popen(["systemctl", "reboot"])
        return JsonResponse({"success": True, "message": "Reboot initiated"})

    elif action in ["shutdown", "poweroff"]:
        if os.name == 'nt':
            subprocess.Popen(["shutdown", "/s", "/t", "0"])
        else:
            subprocess.Popen(["systemctl", "poweroff"])
        return JsonResponse({"success": True, "message": "Shutdown initiated"})

    return JsonResponse({"success": False, "error": "Invalid power action"}, status=400)
