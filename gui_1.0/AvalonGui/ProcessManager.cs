using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;

namespace AvalonGui
{
    public class ProcessManager
    {
        private static readonly log4net.ILog LOG
             = log4net.LogManager.GetLogger(System.Reflection.MethodBase.GetCurrentMethod().DeclaringType);

        public void RunApplication(string fileName, MinerInfo config)
        {
            BfgMinerRunner runner = new BfgMinerRunner(fileName, config.GetCommandLine2());
            ThreadPool.QueueUserWorkItem(new WaitCallback(runner.Run), this);
        }

        public Process RunApplication2(string fileName, string path, string arguments)
        {
            Process p = null;

            LOG.Info(path);
            LOG.Info(arguments);

            try
            {
                ProcessStartInfo startInfo = new ProcessStartInfo();
                startInfo.FileName = fileName;
                startInfo.Arguments = arguments;
                startInfo.WindowStyle = ProcessWindowStyle.Hidden;
                startInfo.RedirectStandardOutput = true;
                startInfo.RedirectStandardError = true;

                startInfo.UseShellExecute = false;
                startInfo.CreateNoWindow = true; //让窗体不显示
                startInfo.WorkingDirectory = Path.GetDirectoryName(path);
                p = Process.Start(startInfo);
                string err = p.StandardError.ReadToEnd();
                //string output = p.StandardOutput.ReadToEnd();

                //LOG.Debug(output);
                LOG.Error(err);
                //p.WaitForExit();
            }
            catch (Exception ex)
            {
                LOG.Error(ex);
            }

            return p;
        }
    }

    class BfgMinerRunner
    {
        private static readonly log4net.ILog LOG
             = log4net.LogManager.GetLogger(System.Reflection.MethodBase.GetCurrentMethod().DeclaringType);

        public string FileName;
        public string Arguments;

        public BfgMinerRunner(string fileName, string arguments)
        {
            this.FileName = fileName;
            this.Arguments = arguments;
        }

        public void Run(object instance)
        {
            BfgMinerRunner runner = instance as BfgMinerRunner;
            if (runner == null)
            {
                return;
            }

            Process p = null;

            try
            {
                p = new Process();
                p.StartInfo = new ProcessStartInfo();
                p.StartInfo.FileName = runner.FileName;
                p.StartInfo.Arguments = runner.Arguments;
                p.StartInfo.WindowStyle = ProcessWindowStyle.Hidden;
                p.StartInfo.RedirectStandardOutput = true;
                p.StartInfo.UseShellExecute = false;
                p.StartInfo.CreateNoWindow = true;//让窗体不显示
                p.Start();
                //p.WaitForExit();
            }
            catch (Exception ex)
            {
                LOG.Error(ex);
            }
        }
    }
}
