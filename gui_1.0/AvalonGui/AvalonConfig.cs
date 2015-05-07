using System;
using System.Collections.Generic;
using System.Configuration;
using System.Linq;
using System.Text;


using AvalonGui.Utils;
using AvalonDevice;

//[PoolInfo_1]
//PoolURL=stratum+tcp://p2pool.org
//PoolPort=9332
//WorkerName=mikeqin.avalon
//WorkerPassword=1234

//[PoolInfo_2]
//PoolURL=stratum+tcp://au.ozco.in
//PoolPort=80
//WorkerName=mikeqin.avalon
//WorkerPassword=1234

//[PoolInfo_3]
//PoolURL=stratum+tcp://cn.ozco.in
//PoolPort=80
//WorkerName=mikeqin.avalon
//WorkerPassword=1234

//[Avalon]
//Log=avalon.log
//Parameters=-T, -S, ICA:\\\\.\\COM3, 

namespace AvalonGui
{
    public class AvalonConfig
    {
        private static readonly log4net.ILog LOG
             = log4net.LogManager.GetLogger(System.Reflection.MethodBase.GetCurrentMethod().DeclaringType);

        const string PoolSectionHead = "PoolInfo_";
        const string PoolURLKey = "PoolURL";
        const string PoolPortKey = "PoolPort";
        const string PoolWorkerNameKey = "WorkerName";
        const string PoolWorkerPasswordKey = "WorkerPassword";

        const string AvalonSectionName = "Avalon";
        const string LogKey = "Log";
        const string ParametersKey = "Parameters";

        private MinerInfo _minerInfo;

        IniWrapper _iniWrapper = null;

        private int _MHSSeconds = 20;
        private bool _DebugData = false;

        public AvalonConfig(string configPath)
        {
            _iniWrapper = new IniWrapper(configPath);
            _minerInfo = new MinerInfo(this);
        }


        public MinerInfo MinerInfo
        {
            get
            {
                return this._minerInfo;
            }
        }

        public int MHSSeconds
        {
            get
            {
                return _MHSSeconds;
            }
        }

        public bool DebugData
        {
            get
            {
                return _DebugData;
            }
        }

        public void LoadConfig()
        {
            string mhsSecondsValue = string.Empty;

            try
            {
                mhsSecondsValue = ConfigurationManager.AppSettings["MHSSeconds"];
                int v = 0;
                if (Int32.TryParse(mhsSecondsValue, out v))
                {
                    switch (v)
                    {
                        case 20:
                            _MHSSeconds = v;
                            break;
                        case 300:
                            _MHSSeconds = v;
                            break;
                    }
                }

                string debug = ConfigurationManager.AppSettings["DebugData"];
                if (debug == "true")
                {
                    _DebugData = true;
                }
            }
            catch { }

            for (int i = 0; i < _minerInfo.PoolInfos.Length; i++)
            {
                PoolInfo pi = new PoolInfo();
                string section = string.Format("{0}{1}", PoolSectionHead, i + 1);
                string v = _iniWrapper.ReadValue(section, PoolURLKey);
                if (!string.IsNullOrEmpty(v))
                {
                    pi.PoolURL = v;
                }

                v = _iniWrapper.ReadValue(section, PoolPortKey);
                if (!string.IsNullOrEmpty(v))
                {
                    pi.PoolPort = v;
                }

                v = _iniWrapper.ReadValue(section, PoolWorkerNameKey);
                if (!string.IsNullOrEmpty(v))
                {
                    pi.WorkerName = v;
                }

                v = _iniWrapper.ReadValue(section, PoolWorkerPasswordKey);
                if (!string.IsNullOrEmpty(v))
                {
                    pi.WorkerPassword = v;
                }

                _minerInfo.PoolInfos[i] = pi;
            }

            AvalonInfo ai = new AvalonInfo();
            string v2 = _iniWrapper.ReadValue(AvalonSectionName, LogKey);
            if (!string.IsNullOrEmpty(v2))
            {
                ai.Log = v2;
            }

            v2 = _iniWrapper.ReadValue(AvalonSectionName, ParametersKey);
            if (!string.IsNullOrEmpty(v2))
            {
                ai.Parameters = v2;
            }

            _minerInfo.AvalonInfo = ai;
        }


        public void SaveConfig()
        {
            LOG.Info("==enter SaveConfig==");

            for (int i = 0; i < _minerInfo.PoolInfos.Length; i++)
            {
                string section = string.Format("{0}{1}", PoolSectionHead, i + 1);
                _iniWrapper.WriteValue(section, PoolURLKey, _minerInfo.PoolInfos[i].PoolURL);
                _iniWrapper.WriteValue(section, PoolPortKey, _minerInfo.PoolInfos[i].PoolPort);
                _iniWrapper.WriteValue(section, PoolWorkerNameKey, _minerInfo.PoolInfos[i].WorkerName);
                _iniWrapper.WriteValue(section, PoolWorkerPasswordKey, _minerInfo.PoolInfos[i].WorkerPassword);
            }

            _iniWrapper.WriteValue(AvalonSectionName, LogKey, _minerInfo.AvalonInfo.Log);
            _iniWrapper.WriteValue(AvalonSectionName, ParametersKey, _minerInfo.AvalonInfo.Parameters);
        }
    }


    public class MinerInfo
    {
        const string SerialFlag = @"-S";
        const string PortParamHeader = "ICA:\\\\.\\COM";

        private AvalonConfig _avalonConfig;

        public PoolInfo[] PoolInfos;
        public AvalonInfo AvalonInfo;

        public MinerInfo(AvalonConfig config)
        {
            PoolInfos = new PoolInfo[3];
            AvalonInfo = new AvalonInfo();

            _avalonConfig = config;
        }

        public string GetCommandLine2()
        {
            return new AvalonInfo().Parameters;
        }

        public string GetCommandLine()
        {
            string cmdLine = string.Empty;
            for (int i = 0; i < PoolInfos.Length; i++)
            {
                cmdLine += string.Format(" -o {0}:{1} -O {2}:{3} ", PoolInfos[i].PoolURL, PoolInfos[i].PoolPort, PoolInfos[i].WorkerName, PoolInfos[i].WorkerPassword);
            }

            //string[] comPorts = DevicePorts.GetDevicePortArray();
            string[] comPorts = DeviceCounter.GetNanoComs();

            if (comPorts == null || comPorts.Length == 0)
            {
                return string.Empty;
            }

            string comArguments = string.Empty;
            foreach (string port in comPorts)
            {
                comArguments += SerialFlag + " " + PortParamHeader + port.Substring(3) + " ";
            }

            string arguments = string.Empty;
            string[] items = AvalonInfo.Parameters.Split(",".ToCharArray());

            bool comArgumentAdded = false;
            for (int i = 0; i < items.Length; i++)
            {
                string item = items[i].Trim();

                bool serialFlag = item.Equals(SerialFlag, StringComparison.InvariantCultureIgnoreCase);

                if (!comArgumentAdded && serialFlag)
                {
                    arguments += " " + comArguments + " ";
                    comArgumentAdded = true;
                }

                if (serialFlag)
                {
                    i++;
                    continue;
                }

                if (!string.IsNullOrEmpty(item))
                {
                    arguments += " " + item + " ";
                }
            }

            cmdLine += arguments;
            cmdLine += " --log-file " + AvalonInfo.Log;
            cmdLine = cmdLine.Replace("\"", "");

            string timeIntervalParameter = string.Empty;
            switch (_avalonConfig.MHSSeconds)
            {
                case 20:
                    break;
                case 300:
                    timeIntervalParameter = " -l 300";
                    break;
            }

            cmdLine += timeIntervalParameter;

            return cmdLine;
        }
    }

    public class PoolInfo
    {
        public string PoolURL;
        public string PoolPort;
        public string WorkerName;
        public string WorkerPassword;

        public PoolInfo()
        {
            PoolPort = "80";
        }
    }

    public class AvalonInfo
    {
        public string Log;
        public string Parameters;

        public AvalonInfo()
        {
            Log = "avalon.log";
            //Parameters = "-T -S ICA:/dev/ttyACM0 --set-device ICA:baud=115200 --set-device ICA:reopen=timeout --set-device ICA:work_division=1 --set-device ICA:fpga_count=1 --set-device ICA:probe_timeout=100 --set-device ICA:timing=0.22 --api-listen";
            Parameters = @" -S ICA:\\.\COM6 -S ICA:\\.\COM10 -S ICA:\\.\COM11 -S ICA:\\.\COM12 -S ICA:\\.\COM3 -S ICA:\\.\COM47 -o http://p2pool.org:9332 -O 1GG4mvU4E7U8yHEEXQ17QsP5xXM2ZYBBDp:useless --set-device ICA:baud=115200 --set-device ICA:reopen=timeout --set-device ICA:work_division=1 --set-device ICA:fpga_count=1 --set-device ICA:probe_timeout=100 --set-device ICA:timing=0.22 --api-listen -D -T";
        }
    }
}
