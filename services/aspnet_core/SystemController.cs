using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.SignalR;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;

namespace Xenithra.OS.Backend
{
    public class Program
    {
        public static void Main(string[] args)
        {
            var builder = WebApplication.CreateBuilder(args);

            // Bind explicitly to localhost for secure internal kiosk communication
            builder.WebHost.UseUrls("http://127.0.0.1:5000");

            builder.Services.AddControllers();
            builder.Services.AddSignalR();
            builder.Services.AddCors(options =>
            {
                options.AddDefaultPolicy(policy =>
                {
                    policy.WithOrigins("http://127.0.0.1:5000", "http://localhost:5000")
                          .AllowAnyHeader()
                          .AllowAnyMethod()
                          .AllowCredentials();
                });
            });

            // Register background telemetry broadcaster
            builder.Services.AddHostedService<TelemetryBroadcastService>();

            var app = builder.Build();

            app.UseDefaultFiles();
            app.UseStaticFiles();
            app.UseRouting();
            app.UseCors();

            app.MapControllers();
            app.MapHub<SystemTelemetryHub>("/hubs/telemetry");

            Console.WriteLine("[Xenithra OS] ASP.NET Core Local Service Running on http://127.0.0.1:5000");
            app.Run();
        }
    }

    [ApiController]
    [Route("api/system")]
    public class SystemController : ControllerBase
    {
        [HttpGet("stats")]
        public IActionResult GetSystemStats()
        {
            var process = Process.GetCurrentProcess();
            var stats = new
            {
                CpuUsagePercent = GetCpuUsageEstimate(),
                MemoryUsedMb = (process.WorkingSet64 / (1024 * 1024)),
                TotalMemoryMb = GetTotalPhysicalMemoryMb(),
                OsVersion = RuntimeInformation.OSDescription,
                Architecture = RuntimeInformation.OSArchitecture.ToString(),
                ProcessCount = Process.GetProcesses().Length,
                UptimeSeconds = (int)(DateTime.UtcNow - Process.GetCurrentProcess().StartTime.ToUniversalTime()).TotalSeconds,
                Timestamp = DateTime.UtcNow
            };
            return Ok(stats);
        }

        [HttpPost("exec")]
        public IActionResult ExecuteCommand([FromBody] ExecCommandRequest req)
        {
            if (string.IsNullOrWhiteSpace(req.Command))
            {
                return BadRequest(new { success = false, message = "Command cannot be empty" });
            }

            try
            {
                var psi = new ProcessStartInfo
                {
                    FileName = req.Command,
                    Arguments = req.Arguments ?? string.Empty,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true
                };

                if (req.RunDetached)
                {
                    psi.RedirectStandardOutput = false;
                    psi.RedirectStandardError = false;
                    var proc = Process.Start(psi);
                    return Ok(new { success = true, pid = proc?.Id, message = "Process launched in background" });
                }

                using var procSync = Process.Start(psi);
                if (procSync == null)
                {
                    return StatusCode(500, new { success = false, message = "Failed to start process" });
                }

                string stdout = procSync.StandardOutput.ReadToEnd();
                string stderr = procSync.StandardError.ReadToEnd();
                procSync.WaitForExit(5000);

                return Ok(new
                {
                    success = procSync.ExitCode == 0,
                    exitCode = procSync.ExitCode,
                    stdout = stdout.Trim(),
                    stderr = stderr.Trim()
                });
            }
            catch (Exception ex)
            {
                return StatusCode(500, new { success = false, error = ex.Message });
            }
        }

        [HttpPost("power")]
        public IActionResult ExecutePowerAction([FromBody] PowerActionRequest req)
        {
            try
            {
                switch (req.Action?.ToLowerInvariant())
                {
                    case "restart":
                    case "reboot":
                        TriggerReboot();
                        return Ok(new { success = true, message = "System reboot initiated" });

                    case "shutdown":
                    case "poweroff":
                        TriggerShutdown();
                        return Ok(new { success = true, message = "System shutdown initiated" });

                    default:
                        return BadRequest(new { success = false, message = "Invalid action. Supported: 'restart', 'shutdown'" });
                }
            }
            catch (Exception ex)
            {
                return StatusCode(500, new { success = false, error = ex.Message });
            }
        }

        [HttpGet("fs/list")]
        public IActionResult ListDirectory([FromQuery] string path = "/")
        {
            try
            {
                if (!Directory.Exists(path))
                {
                    path = Directory.GetCurrentDirectory();
                }

                var dirInfo = new DirectoryInfo(path);
                var dirs = dirInfo.GetDirectories();
                var files = dirInfo.GetFiles();

                var result = new
                {
                    CurrentPath = dirInfo.FullName,
                    Directories = Array.ConvertAll(dirs, d => new { d.Name, d.LastWriteTime, Type = "directory" }),
                    Files = Array.ConvertAll(files, f => new { f.Name, f.Length, f.LastWriteTime, Type = "file" })
                };

                return Ok(result);
            }
            catch (Exception ex)
            {
                return StatusCode(500, new { success = false, error = ex.Message });
            }
        }

        private static void TriggerReboot()
        {
            if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
                Process.Start(new ProcessStartInfo("shutdown", "/r /t 0") { CreateNoWindow = true, UseShellExecute = false });
            else
                Process.Start(new ProcessStartInfo("systemctl", "reboot") { CreateNoWindow = true, UseShellExecute = false });
        }

        private static void TriggerShutdown()
        {
            if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
                Process.Start(new ProcessStartInfo("shutdown", "/s /t 0") { CreateNoWindow = true, UseShellExecute = false });
            else
                Process.Start(new ProcessStartInfo("systemctl", "poweroff") { CreateNoWindow = true, UseShellExecute = false });
        }

        private static int GetCpuUsageEstimate()
        {
            return Random.Shared.Next(18, 48); // Baseline live telemetry simulation hook
        }

        private static long GetTotalPhysicalMemoryMb()
        {
            return 8192; // 8GB default target platform baseline
        }
    }

    public class SystemTelemetryHub : Hub
    {
        public async Task RequestUpdate()
        {
            var stats = new
            {
                Cpu = Random.Shared.Next(12, 45),
                MemoryMb = 2048,
                DiskPercent = 38,
                Timestamp = DateTime.UtcNow
            };
            await Clients.Caller.SendAsync("ReceiveTelemetry", stats);
        }
    }

    public class TelemetryBroadcastService : BackgroundService
    {
        private readonly IHubContext<SystemTelemetryHub> _hub;

        public TelemetryBroadcastService(IHubContext<SystemTelemetryHub> hub)
        {
            _hub = hub;
        }

        protected override async Task ExecuteAsync(System.Threading.CancellationToken stoppingToken)
        {
            while (!stoppingToken.IsCancellationRequested)
            {
                var payload = new
                {
                    Cpu = Random.Shared.Next(15, 55),
                    MemoryMb = 2140 + Random.Shared.Next(-50, 50),
                    DiskPercent = 42,
                    ActiveSessions = 1,
                    Timestamp = DateTime.UtcNow.ToString("HH:mm:ss")
                };

                await _hub.Clients.All.SendAsync("ReceiveTelemetry", payload, stoppingToken);
                await Task.Delay(1000, stoppingToken);
            }
        }
    }

    public class ExecCommandRequest
    {
        public string Command { get; set; } = string.Empty;
        public string Arguments { get; set; } = string.Empty;
        public bool RunDetached { get; set; } = true;
    }

    public class PowerActionRequest
    {
        public string Action { get; set; } = string.Empty;
    }
}
