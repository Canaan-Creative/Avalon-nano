using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace AvalonGui.Utils
{
    public class TimeConvertor
    {
        static DateTime BaseTime = new DateTime(1970, 1, 1, 8, 0, 0);

        public static DateTime FromMillisSeconds(long miseconds)
        {
            return BaseTime.AddMilliseconds(miseconds);
        }

        public static string GetTimeString(DateTime dateTime)
        {
            return dateTime.ToString("HH:mm:ss");
        }

        public static string GetTimeString(long miseconds)
        {
            if (miseconds < 1)
            {
                return "--:--:--";
            }

            return FromMillisSeconds(miseconds).ToString("HH:mm:ss");
        }

    }
}
