using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace AvalonGui.Model
{
    [Serializable]
    public class Pool : ModelBase
    {
        private string urlWithoutPort;
        private int port;

        // POOL=0,URL=stratum+tcp://p2pool.org:9332,Status=Alive,Priority=0,Quota=1,Long Poll=N,Getworks=5,Accepted=0,Rejected=0,Works=2,Discarded=8,Stale=0,Get Failures=0,Remote Failures=0,User=mikeqin.avalon,Last Share Time=0,Diff1 Shares=0,Proxy=,Difficulty Accepted=0,Difficulty Rejected=0,Difficulty Stale=0,Last Share Difficulty=0,Has Stratum=true,Stratum Active=true,Stratum URL=p2pool.org,Best Share=0,Pool Rejected%=0.0000,Pool Stale%=0.0000
        public int POOL { get; set; }

        public string URL
        {
            get
            {
                return urlWithoutPort;
            }

            set
            {
                string[] items = value.Split(":".ToCharArray());

                if (items != null && items.Length > 0)
                {
                    urlWithoutPort = items[0];
                    if (items.Length > 1)
                    {
                        Int32.TryParse(items[1], out port);
                    }
                }
            }
        }

        public int Port
        {
            get
            {
                return port;
            }
        }

        public string Status { get; set; } //Alive
        public int Priority { get; set; }
        public int Quota { get; set; }
        public long Last_Share_Time { get; set; }
        //=1,Long Poll=N,Getworks=5,Accepted=0,Rejected=0,Works=2,Discarded=8,Stale=0,Get Failures=0,Remote Failures=0,User=mikeqin.avalon,Last Share Time=0,Diff1 Shares=0,Proxy=,Difficulty Accepted=0,Difficulty Rejected=0,Difficulty Stale=0,Last Share Difficulty=0,Has Stratum=true,Stratum Active=true,Stratum URL=p2pool.org,Best Share=0,Pool Rejected%=0.0000,Pool Stale%=0.0000
    }
}
