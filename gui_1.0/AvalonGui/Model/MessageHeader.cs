using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.Serialization;

namespace AvalonGui.Model
{
    [Serializable]
    public class MessageHeader : ModelBase
    {
        // STATUS=S,When=1407906228,Code=11,Msg=Summary,Description=bfgminer 4.4.0
        public string STATUS { get; set; }
        public Int64 When { get; set; }
        public int Code { get; set; }
        public string Msg { get; set; }
        public string Description { get; set; }
    }
}
