using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace AvalonGui.Model
{
    [Serializable]
    public class Summary : ModelBase
    {
        //SUMMARY,Elapsed=20,MHS av=0.000,MHS 20s=0.000,Found Blocks=0,Getworks=5,Accepted=0,Rejected=0,Hardware Errors=0,Utility=0.000,Discarded=8,Stale=0,Get Failures=0,Local Work=15,Remote Failures=0,Network Blocks=2,Total MH=0.0000,Diff1 Work=0,Work Utility=0.000,Difficulty Accepted=0,Difficulty Rejected=0,Difficulty Stale=0,Best Share=0,Device Hardware%=0.0000,Device Rejected%=0.0000,Pool Rejected%=0.0000,Pool Stale%=0.0000,Last getwork=0|

        public int Elapsed { get; set; }
        public double MHS_av { get; set; }
        public double MHS_20s { get; set; }
        public double MHS_300s { get; set; }
        public int Found_Blocks { get; set; }
        public int Getworks { get; set; }
        public int Accepted { get; set; }
        public int Rejected { get; set; }
        public int Hardware_Errors { get; set; }
        public double Utility { get; set; }
        public int Discarded { get; set; }
        public int Stale { get; set; }
        public int Get_Failures { get; set; }
        public int Local_Work { get; set; }
        //=15,Remote Failures=0,Network Blocks=2,
        public double Total_MH { get; set; }
        public int Diff1_Work { get; set; }
        public double Work_Utility { get; set; }
        public int Difficulty_Accepted { get; set; }
        public int Difficulty_Rejected { get; set; }
        public int Difficulty_Stale { get; set; }
        //=0,Best Share=0,Device Hardware%=0.0000,Device Rejected%=0.0000,Pool Rejected%=0.0000,Pool Stale%=0.0000,Last getwork=0|

        //cur value = val_Diff1_Work / (val_Difficulty_Accepted / (val_Difficulty_Accepted + val_Difficulty_Rejected + val_Difficulty_Stale)) * 60 / val_Elapsed * 71582788 / 1000000 / 1000.0;

        public double GetGHSAv()
        {
            return MHS_av / 1000.0;
        }

        public double GetGHS20s()
        {
            return MHS_20s / 1000.0;
        }

        public double GetGHS300s()
        {
            return MHS_300s / 1000.0;
        }

        public double GetGHSByInterval(int seconds)
        {
            double v = GetGHS20s();

            switch (seconds)
            {
                //case 20:
                //    break;
                case 300:
                    v = GetGHS300s();
                    break;
            }

            return v;
        }

        public double GetCurGHS()
        {
            double v = 0;

            if (Elapsed != 0)
            {
                if (Difficulty_Accepted == 0 || (Difficulty_Accepted + Difficulty_Rejected + Difficulty_Stale) == 0)
                {
                    //v = Diff1_Work * 60 / Elapsed * 71582788 / 1000000 / 1000.0;
                    v = Diff1_Work * 0.06 / Elapsed * 71.582788;
                }
                else
                {
                    //v = Diff1_Work / (Difficulty_Accepted / (Difficulty_Accepted + Difficulty_Rejected + Difficulty_Stale)) * 60 / Elapsed * 71582788 / 1000000 / 1000.0;
                    v = Diff1_Work / (Difficulty_Accepted / (Difficulty_Accepted + Difficulty_Rejected + Difficulty_Stale)) * 0.06 / Elapsed * 71.582788;
                }
            }

            return v;
        }
    }
}
