using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

using AvalonGui.Utils;
using Avalon.Utils;

namespace AvalonGui
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            bool createdNew;
            System.Threading.Mutex m = new System.Threading.Mutex(true, "avalon_gui_ever", out createdNew);

            int usbCount = 0;
            if (args != null && args.Length >= 2)
            {
                if (args[0] == "-n")
                {
                    int v = 0;
                    if (Int32.TryParse(args[1], out v))
                    {
                        if (v < 0)
                        {
                            v = 0;
                        }

                        usbCount = v;
                    }
                }
            }

            try
            {
                if (!createdNew)
                {
                    return;
                }

                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);

                MainForm mainForm = new MainForm();
                mainForm.SetUSBCount(usbCount);
                Application.Run(mainForm);
            }
            finally
            {
                m.Close();
                ProcessKiller.KillProcessByName(Constants.BfgMinerFileName);
            }
        }
    }
}
