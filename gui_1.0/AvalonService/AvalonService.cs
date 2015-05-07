using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Configuration;
using System.Data;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.ServiceProcess;
using System.Text;
using System.Threading;

using System.Management;
using NamedPipeWrapper;
using AvalonDevice;
using Avalon.Utils;

namespace Avalon.Service
{
    public partial class AvalonService : ServiceBase
    {
        private static readonly log4net.ILog LOG
             = log4net.LogManager.GetLogger(System.Reflection.MethodBase.GetCurrentMethod().DeclaringType);

        const string GuiApplicationName = "AvalonGui.exe";
        const string BfgMinerApplicationName = "BfgMiner.exe";

        const string NAMEDPIPE_SERVER_NAME = "avalon_np_server_0989";
        const string NamedPipe_Command_USBCounting = "usbcounting";
        const string NamedPipe_Command_Quit = "quit";

        private readonly NamedPipeClient<string> _namedpipeClient = new NamedPipeClient<string>(NAMEDPIPE_SERVER_NAME);
        bool _namedpipeServerConnected = false;
        bool _serviceRunning;
        bool _requestNotifyUSBCount = false;
        static int _usbCount = 0;
        string _deviceTag = "NXP-77";
        string _deviceId = "\\VID_1FC9&PID_0083\\";

        string _avalonGuiDir = string.Empty;
        string _avalonGuiPath = string.Empty;

        USBWatcher usbMonitor = new USBWatcher();

        // 自动更新USB数量，防止计数错误
        DateTime _lastUSBEventTime = DateTime.Now;
        const int MaxUSBEventGapMilliSeconds = 3000; // three seconds

        public AvalonService()
        {
            InitializeComponent();
        }

        protected override void OnStart(string[] args)
        {
            _serviceRunning = true;

            LOG.Info("enter Service.OnStart()");

            try
            {
                string deviceTag = ConfigurationManager.AppSettings["DeviceTag"];
                if (!string.IsNullOrEmpty(deviceTag))
                {
                    _deviceTag = deviceTag;
                    LOG.Info(deviceTag);
                }
            }
            catch { }

            _usbCount = DeviceCounter.GetNanoCount();

            usbMonitor.AddUSBEventWatcher(USBEventHandler, USBEventHandler, new TimeSpan(0, 0, 3));

            _namedpipeClient.ServerMessage += OnServerMessage;
            _namedpipeClient.Disconnected += OnDisconnected;
            _namedpipeClient.AutoReconnect = true;

            ThreadPool.QueueUserWorkItem(new WaitCallback(ConnectProc), this);

            _namedpipeClient.Start();
        }

        protected override void OnStop()
        {
            _serviceRunning = false;
            usbMonitor.RemoveUSBEventWatcher();

            StopUIProcesses();
        }

        private void StopUIProcesses()
        {
            ProcessKiller.KillProcessByName(GuiApplicationName);
            ProcessKiller.KillProcessByName(BfgMinerApplicationName);
        }


        private void ConnectProc(object instance)
        {
            LOG.Info("enter ConnectProc()");

            while (_serviceRunning)
            {
                if (_namedpipeServerConnected)
                {
                    if (_requestNotifyUSBCount)
                    {
                        string message = string.Format("{0} {1}", NamedPipe_Command_USBCounting, _usbCount);
                        LOG.Info("service send message: " + message);
                        _namedpipeClient.PushMessage(message);

                        _requestNotifyUSBCount = false;
                    }
                }

                // wait for 5 seconds
                for (int i = 0; i < 25; i++)
                {
                    if (!_serviceRunning || UpdateUSBCount()) break;
                    Thread.Sleep(200);
                }
            }
        }

        private bool UpdateUSBCount()
        {
            bool needUpdate = false;

            if (DateTime.Now.Subtract(_lastUSBEventTime).TotalMilliseconds > MaxUSBEventGapMilliSeconds)
            {
                int usbCount = DeviceCounter.GetNanoCount();
                if (_usbCount != usbCount)
                {
                    _usbCount = usbCount;
                    _requestNotifyUSBCount = true;
                    needUpdate = true;
                    _lastUSBEventTime = DateTime.Now;
                }
            }

            return needUpdate;
        }

        private void USBEventHandler(Object sender, EventArrivedEventArgs e)
        {
            int byTimes = 0;

            if (e.NewEvent.ClassPath.ClassName == "__InstanceCreationEvent")
            {
                byTimes = 1;
            }
            else if (e.NewEvent.ClassPath.ClassName == "__InstanceDeletionEvent")
            {
                byTimes = -1;
            }

            LOG.Info(string.Format("old count = {0}, plugin/out: {1}", _usbCount, byTimes));

            int deviceCount = 0;
            foreach (USBWatcher.USBControllerDevice Device in USBWatcher.WhoUSBControllerDevice(e))
            {
                string line = Device.Dependent.TrimEnd("\"\\".ToCharArray());
                LOG.Info("line = " + line);

                //if (line.EndsWith(string.Format("\\{0}", _deviceTag), StringComparison.InvariantCultureIgnoreCase))
                if (line.IndexOf(_deviceId) > 0)
                {
                    deviceCount++;
                }
            }

            _usbCount += (deviceCount * byTimes);


            if (_usbCount < 0)
            {
                _usbCount = 0;
            }

            int count = DeviceCounter.GetNanoCount();
            if (_usbCount > count)
            {
                _usbCount = count;
            }

            LOG.Info("=======usb count: " + _usbCount);

            _lastUSBEventTime = DateTime.Now;

            RunAndNotifyGui(_usbCount);
        }

        private void OnServerMessage(NamedPipeConnection<string, string> connection, string message)
        {
            if (!_namedpipeServerConnected)
            {
                _namedpipeServerConnected = true;
            }

            if (message.Equals("usbcounting"))
            {
                _namedpipeClient.PushMessage(string.Format("usbcounting {0}", _usbCount));
            }
        }

        private void OnDisconnected(NamedPipeConnection<string, string> connection)
        {
            _namedpipeServerConnected = false;

            //_namedpipeClient.Stop();
        }

        private void RunAndNotifyGui(int usbCount)
        {
            _requestNotifyUSBCount = true;
        }
    }
}
