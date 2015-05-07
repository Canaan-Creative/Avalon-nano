using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace AvalonGui.Model
{
    public class PoolResult
    {
        public MessageHeader Header { get; set; }
        public Pool[] Pools { get; set; }

        public PoolResult()
        {
            this.Header = new MessageHeader();
        }
    }
}
