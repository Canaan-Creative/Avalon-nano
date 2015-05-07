using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

using Microsoft.Win32;
using AvalonGui.FloatWindow;

namespace AvalonGui
{
    public class IniWrapper
    {
        private static readonly log4net.ILog LOG
             = log4net.LogManager.GetLogger(System.Reflection.MethodBase.GetCurrentMethod().DeclaringType);

        const int Max_Bytes_Per_Line = 1024;
        const string AppName = "Avalon Nano";
        const string MicrosoftRegPath = "Software";


        private string _iniPath = null;
        public IniWrapper(string path)
        {
            this._iniPath = path;
        }

        public void WriteValue(string section, string key, string value)
        {
            // section=配置节，key=键名，value=键值，path=路径

            WriteRegValue(section, key, value);

            LOG.Error(string.Format("WriteValue: {0}, {1}, {2}, {3}", section, key, value, _iniPath));
            long v = Kernel32.WritePrivateProfileString(section, key, value, _iniPath);
            if (v == 0)
            {
                LOG.Error(string.Format("WriteValue error({0}: {1}", _iniPath, Kernel32.GetLastError()));
            }
        }

        public string ReadValue(string section, string key)
        {
            string value = string.Empty;
            value = ReadRegValue(section, key);

            if (!string.IsNullOrEmpty(value))
            {
                return value;
            }

            // 每次从ini中读取多少字节
            System.Text.StringBuilder temp = new System.Text.StringBuilder(Max_Bytes_Per_Line);

            // section=配置节，key=键名，temp=上面，path=路径
            Kernel32.GetPrivateProfileString(section, key, "", temp, Max_Bytes_Per_Line, _iniPath);

            return temp.ToString();
        }

        private string ReadRegValue(string section, string key)
        {
            string value = string.Empty;

            RegistryKey rootKey = null;
            RegistryKey sectionKey = null;

            try
            {
                rootKey = CreateOrOpenAppRoot();
                if (rootKey != null)
                {
                    sectionKey = rootKey.CreateSubKey(section);
                    if (sectionKey != null)
                    {
                        value = sectionKey.GetValue(key) as string;
                    }
                }
            }
            finally
            {
                if (rootKey != null)
                {
                    rootKey.Dispose();
                }

                if (sectionKey != null)
                {
                    sectionKey.Dispose();
                }
            }

            return value;
        }

        private void WriteRegValue(string section, string key, string value)
        {
            RegistryKey rootKey = null;
            RegistryKey sectionKey = null;

            try
            {
                rootKey = CreateOrOpenAppRoot();
                if (rootKey != null)
                {
                    sectionKey = rootKey.CreateSubKey(section);
                    if (sectionKey != null)
                    {
                        sectionKey.SetValue(key, value);
                    }
                }
            }
            finally
            {
                if (rootKey != null)
                {
                    rootKey.Dispose();
                }

                if (sectionKey != null)
                {
                    sectionKey.Dispose();
                }
            }
        }

        private RegistryKey CreateOrOpenAppRoot()
        {
            RegistryKey key = Registry.CurrentUser;

            key = key.OpenSubKey(MicrosoftRegPath, RegistryKeyPermissionCheck.ReadWriteSubTree);

            RegistryKey resultKey = null;

            try
            {
                resultKey = key.CreateSubKey(AppName, RegistryKeyPermissionCheck.ReadWriteSubTree);
            }
            catch (Exception ex)
            {
                LOG.Error(ex);
            }

            return resultKey;
        }

    }
}
