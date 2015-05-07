using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Management;
using System.Text;

using PortableDeviceApiLib;
using Newtonsoft.Json;

namespace AvalonDevice
{
    public class DeviceCounter
    {
        const string DeviceCaptionHead = "LPC USB VCom Port (COM";

        delegate void EnumProc(ManagementObject manageObj, object data);

        public static string[] GetNanoComs()
        {
            List<string> coms = new List<string>();
            foreach (string item in GetAvalonNanoDeviceNames())
            {
                string com = item.Substring(DeviceCaptionHead.Length - 3).TrimEnd(")".ToCharArray());
                coms.Add(com);
            }

            return coms.ToArray();
        }

        public static int GetNanoCount()
        {
            return GetAvalonNanoDeviceNames().Count;
        }

        public static List<string> GetAvalonNanoDeviceNames()
        {
            List<string> usbNames = new List<string>();

            EnumDevices(AvalonNanoEnumProc, (object)usbNames);

            return usbNames;
        }

        private static void AvalonNanoEnumProc(ManagementObject m, object data) //List<string> nameList)
        {
            string name = m["Caption"] as string;

            List<string> nameList = data as List<string>;

            if (!string.IsNullOrEmpty(name))
            {
                if (name.StartsWith(DeviceCaptionHead, StringComparison.InvariantCultureIgnoreCase))
                {
                    nameList.Add(name);
                }
            }
        }

        private static void EnumDevices(EnumProc proc, object data)
        {
            try
            {
                ManagementObjectSearcher searcher =  new ManagementObjectSearcher("root\\CIMV2", "SELECT * FROM Win32_PnPEntity");

                foreach (ManagementObject queryObj in searcher.Get())
                {
                    if (proc != null)
                    {
                        proc(queryObj, data);
                    }
                }
            }
            catch (ManagementException e)
            {
                //
            }
        }

    }
}
