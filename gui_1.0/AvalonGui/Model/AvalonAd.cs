using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace AvalonGui.Model
{
    public class AvalonAd
    {
        public AdInfo[] lists { get; set; }
        public string version { get; set; }
    }

    public class AdInfo
    {
        public string title { get; set; }
        public string url { get; set; }
        public string img { get; set; }
        public string description { get; set; }
    }
}
