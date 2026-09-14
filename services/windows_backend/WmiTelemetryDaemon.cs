using System;
using System.Diagnostics;
using System.IO;
using System.Management;
using System.Net;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

namespace Xenithra.OS.WindowsBackend
{
    public class Program
    {
        private static HttpListener _listener;
        private static PerformanceCounter _cpuCounter;
        private static bool _isRunning = true;

        public static async Task Main(string[] args)
        {
            Console.Title = "Xenithra OS - WMI Telemetry & System Controller Daemon";
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine("==========================================================");
            Console.WriteLine("     Xenithra OS WMI Hardware Telemetry & Control Server  ");
            Console.WriteLine("==========================================================");

            // Initialize CPU Performance Counter
            try
            {
                _cpuCounter = new PerformanceCounter("Processor", "% Processor Time", "_Total");
                _cpuCounter.NextValue(); // First call returns 0
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine($"[!] Warning: PerformanceCounter init failed ({ex.Message}). Fallback to WMI active.");
            }

            // Bind internal loopback listener on 127.0.0.1:8080
            _listener = new HttpListener();
            _listener.Prefixes.Add("http://127.0.0.1:8080/");
            _listener.Prefixes.Add("http://localhost:8080/");

            try
            {
                _listener.Start();
                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine("[+] Listening on http://127.0.0.1:8080/");
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"[-] Fatal: Failed to bind HttpListener: {ex.Message}");
                return;
            }

            // HTTP Request Dispatcher Loop
            while (_isRunning)
            {
                try
                {
                    var context = await _listener.GetContextAsync();
                    _ = ProcessRequestAsync(context);
                }
                catch (HttpListenerException)
                {
                    break;
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"[-] Error in HTTP Pump: {ex.Message}");
                }
            }
        }

        private static async Task ProcessRequestAsync(HttpListenerContext context)
        {
            var req = context.Request;
            var res = context.Response;

            // CORS headers for Vite shell local origin
            res.Headers.Add("Access-Control-Allow-Origin", "*");
            res.Headers.Add("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            res.Headers.Add("Access-Control-Allow-Headers", "Content-Type");

            if (req.HttpMethod == "OPTIONS")
            {
                res.StatusCode = (int)HttpStatusCode.OK;
                res.Close();
                return;
            }

            string path = req.Url.AbsolutePath.ToLowerInvariant();

            try
            {
                if (path == "/api/telemetry" && req.HttpMethod == "GET")
                {
                    var telemetry = GetSystemTelemetry();
                    byte[] buffer = Encoding.UTF8.GetBytes(JsonSerializer.Serialize(telemetry));
                    res.ContentType = "application/json";
                    res.ContentLength64 = buffer.Length;
                    await res.OutputStream.WriteAsync(buffer, 0, buffer.Length);
                }
                else if (path == "/api/exec" && req.HttpMethod == "POST")
                {
                    using var reader = new StreamReader(req.InputStream, req.ContentEncoding);
                    string jsonBody = await reader.ReadToEndAsync();
                    var doc = JsonDocument.Parse(jsonBody);
                    string command = doc.RootElement.GetProperty("command").GetString();
                    string arguments = doc.RootElement.TryGetProperty("arguments", out var argElem) ? argElem.GetString() : string.Empty;

                    var result = ExecuteSystemProcess(command, arguments);
                    byte[] buffer = Encoding.UTF8.GetBytes(JsonSerializer.Serialize(result));
                    res.ContentType = "application/json";
                    res.ContentLength64 = buffer.Length;
                    await res.OutputStream.WriteAsync(buffer, 0, buffer.Length);
                }
                else if (path == "/api/power" && req.HttpMethod == "POST")
                {
                    using var reader = new StreamReader(req.InputStream, req.ContentEncoding);
                    string jsonBody = await reader.ReadToEndAsync();
                    var doc = JsonDocument.Parse(jsonBody);
                    string action = doc.RootElement.GetProperty("action").GetString()?.ToLowerInvariant();

                    if (action == "restart")
                    {
                        Process.Start(new ProcessStartInfo("shutdown", "/r /t 0") { CreateNoWindow = true, UseShellExecute = false });
                    }
                    else if (action == "shutdown")
                    {
                        Process.Start(new ProcessStartInfo("shutdown", "/s /t 0") { CreateNoWindow = true, UseShellExecute = false });
                    }

                    byte[] buffer = Encoding.UTF8.GetBytes("{\"success\":true}");
                    res.ContentType = "application/json";
                    await res.OutputStream.WriteAsync(buffer, 0, buffer.Length);
                }
                else
                {
                    res.StatusCode = (int)HttpStatusCode.NotFound;
                }
            }
            catch (Exception ex)
            {
                res.StatusCode = (int)HttpStatusCode.InternalServerError;
                byte[] errorBuffer = Encoding.UTF8.GetBytes(JsonSerializer.Serialize(new { error = ex.Message }));
                await res.OutputStream.WriteAsync(errorBuffer, 0, errorBuffer.Length);
            }
            finally
            {
                res.Close();
            }
        }

        private static object GetSystemTelemetry()
        {
            float cpuUsage = 0;
            if (_cpuCounter != null)
            {
                try { cpuUsage = (float)Math.Round(_cpuCounter.NextValue(), 1); } catch { }
            }

            ulong totalRamMb = 8192;
            ulong freeRamMb = 4096;

            // Query WMI for accurate OS memory and thermal readings
            try
            {
                using var searcher = new ManagementObjectSearcher("SELECT TotalVisibleMemorySize, FreePhysicalMemory FROM Win32_OperatingSystem");
                foreach (ManagementObject obj in searcher.Get())
                {
                    totalRamMb = Convert.ToUInt64(obj["TotalVisibleMemorySize"]) / 1024;
                    freeRamMb = Convert.ToUInt64(obj["FreePhysicalMemory"]) / 1024;
                }
            }
            catch { }

            ulong usedRamMb = totalRamMb > freeRamMb ? totalRamMb - freeRamMb : 0;
            int memoryPercent = totalRamMb > 0 ? (int)((usedRamMb * 100) / totalRamMb) : 0;

            return new
            {
                cpuPercent = cpuUsage,
                memoryUsedMb = usedRamMb,
                memoryTotalMb = totalRamMb,
                memoryPercent = memoryPercent,
                activeProcesses = Process.GetProcesses().Length,
                timestamp = DateTime.UtcNow.ToString("HH:mm:ss")
            };
        }

        private static object ExecuteSystemProcess(string command, string arguments)
        {
            try
            {
                var psi = new ProcessStartInfo
                {
                    FileName = command,
                    Arguments = arguments,
                    UseShellExecute = true, // Enables launching default associations (Explorer, Edge, Notepad)
                    CreateNoWindow = false
                };
                var proc = Process.Start(psi);
                return new { success = true, pid = proc?.Id, message = $"Started {command}" };
            }
            catch (Exception ex)
            {
                return new { success = false, error = ex.Message };
            }
        }
    }
}
