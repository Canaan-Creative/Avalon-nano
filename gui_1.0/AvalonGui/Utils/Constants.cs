using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace AvalonGui.Utils
{
    public class Constants
    {
        // 广告相关
        public const string AdUrl = "http://downloads.canaan-creative.com/ad/index.php";

        // BitCoin挖矿
        public const string BfgMinerDirName = "BfgMiner";
        public const string BfgMinerFileName = "BfgMiner.exe";
        public const string ConfigFileName = "avalon.ini";
        public const string BfgMinerServerAddress = "127.0.0.1";
        public const int BfgMinerServerPort = 4028;

        // 和服务通讯
        public const string NamedPipe_Command_Quit = "quit";
        public const string NamedPipe_Command_USBCounting = "usbcounting";
        public const string NamedPipe_Connect_Message = ".yug nolava !emocleW";
        public const string NamedPipe_Disconnect_Message = ".eybeyB";
        public const string NAMEDPIPE_SERVER_NAME = "avalon_np_server_0989";
    }
}
