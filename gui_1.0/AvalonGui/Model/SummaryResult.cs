using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace AvalonGui.Model
{
    public class SummaryResult
    {
        public MessageHeader Header { get; set; }
        public Summary Summary { get; set; }

        public SummaryResult()
        {
            this.Header = new MessageHeader();
            this.Summary = new Summary();
        }
    }
}
