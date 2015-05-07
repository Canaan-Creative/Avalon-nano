using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.Win32;



namespace AvalonGui.Utils
{
    public class DevicePorts
    {
        const string RootKeyPath = @"SYSTEM\CurrentControlSet\Enum\USB";
        const string DeviceRegKeyHead = "VID_1FC9";

        // VID_1FC9&PID_0083&MI_00
        public static string[] GetDevicePortArray()
        {
            List<string> ports = new List<string>();

            RegistryKey deviceRootKey = Registry.LocalMachine.OpenSubKey(RootKeyPath);
            String[] instanceKeyNames = deviceRootKey.GetSubKeyNames();
            
            foreach (String instanceKey in instanceKeyNames)
            {
                if (instanceKey.StartsWith(DeviceRegKeyHead, StringComparison.InvariantCultureIgnoreCase))
                {
                    EnumValueRecursively(string.Format("{0}\\{1}", RootKeyPath, instanceKey), "Device Parameters", "PortName", ports);
                }
            }

            return ports.ToArray();
        }


        public static void EnumValueRecursively(string keyPath, string keyName, string valueName, List<string> ports)
        {
            try
            {
                RegistryKey HKLM = Registry.LocalMachine;
                RegistryKey RegKey = HKLM.OpenSubKey(keyPath);
                string[] subKeys = RegKey.GetSubKeyNames();

                foreach (string subKey in subKeys)
                {
                    string fullPath = keyPath + "\\" + subKey;

                    if (subKey.Equals(keyName, StringComparison.InvariantCultureIgnoreCase))
                    {
                        RegistryKey portKey = RegKey.OpenSubKey(keyName);

                        try
                        {
                            Object oValue = portKey.GetValue(valueName);
                            ports.Add(oValue.ToString());
                        }
                        catch
                        {
                        }
                    }

                    EnumValueRecursively(fullPath, keyName, valueName, ports);
                }
            }
            catch { }
        }


    }
}
