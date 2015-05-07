using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;

namespace AvalonGui
{
    public class RPCThread
    {
        private static readonly log4net.ILog LOG
             = log4net.LogManager.GetLogger(System.Reflection.MethodBase.GetCurrentMethod().DeclaringType);

        public delegate void OnInfoReceived(string infoName, string jsonContent);

        private int _running = 0; // 0 stopped, 1 running, 2 paused
        private string _serverName = "127.0.0.1";
        private int _serverPort = 4028;
        private OnInfoReceived _onInfoReceived;
        private StreamReader _debugReader;

        public RPCThread(string serverName, int serverPort, OnInfoReceived onInfoReceived)
        {
            _serverName = serverName;
            _serverPort = serverPort;
            _onInfoReceived = onInfoReceived;
        }

        public void Start(bool debug)
        {
            _running = 1;

            if (debug)
            {
                try
                {
                    _debugReader = new StreamReader(@"d:\build\avalon\GUI\gui-0825.log");
                }
                catch { }
            }

            Thread thread = new Thread(new ThreadStart(this.ThreadProc));
            thread.Start();
        }

        public void Stop()
        {
            _running = 0;

            if (_debugReader != null)
            {
                _debugReader.Close();
                _debugReader = null;
            }
        }

                

        protected void ThreadProc()
        {
            while (_running == 1)
            {
                if (CallApi("summary"))
                {
                    Thread.Sleep(100);//parseSummary();
                }
                else
                {
                    continue;
                }

                if (_running != 1) break;

                if (CallApi("pools"))
                {
                    Thread.Sleep(100);//parsePools();
                }
                else
                {
                    continue;
                }

                if (_running != 1) break;

                /* sleep 5 seconds (25 * 200) ==> sleep 1 second (5 * 200) */
                //        for (int i = 0; i < 25; i ++) {
                for (int i = 0; i < 25; i++)
                {
                    Thread.Sleep(200);
                    if (_running != 1) break;
                }
            }
        }

        private bool CallApi(string apiName)
        {
            if (_debugReader != null)
            {
                return DebugCallApi(apiName);
            }

            const int Timeout = 300; //1 * 1000;

            IPAddress ip = IPAddress.Parse(_serverName);
            Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

            try
            {
                IAsyncResult result  = socket.BeginConnect(ip, _serverPort, null, null);
                bool success = result.AsyncWaitHandle.WaitOne(Timeout, true);

                if (!success)
                {
                    socket.Close();
                    return false;
                }
            }
            catch (Exception ex)
            {
                LOG.Error(ex);
                socket.Close();
                return false;
            }

            byte[] command = Encoding.ASCII.GetBytes(apiName);

            try
            {
                if (socket.Send(command) != command.Length)
                {
                    socket.Close();
                    return false;
                }
            }
            catch (Exception ex)
            {
                LOG.Error(ex);
                socket.Close();
                return false;
            }

            int wait = 0;
            while (socket.Available < (int)sizeof(Int16))
            {
                Thread.Sleep(10);
                wait += 10;
                if (wait > Timeout)
                {
                    socket.Close();
                    return false;
                }
            }

            byte[] buffer = new byte[1024 * 10]; // 1M

            try
            {
                int len = socket.Receive(buffer);
                string recvString = Encoding.ASCII.GetString(buffer, 0, len - 1);

                if (_onInfoReceived != null)
                {
                    _onInfoReceived(apiName, recvString);
                }
            }
            catch (Exception ex)
            {
                LOG.Error(ex);
            }
            finally
            {
                socket.Close();
            }

            return true;
        }

        // STATUS=S,When=1408775311,Code=11,Msg=Summary,Description=bfgminer 4.4.0|SUMMARY,
        // STATUS=S,When=1408775311,Code=7,Msg=3 Pool(s),Description=bfgminer 4.4.0|POOL=0,
        private bool DebugCallApi(string apiName)
        {
            if (_debugReader == null)
            {
                return true;
            }

            string dnaStartText = "|SUMMARY,";
            if (apiName != "summary")
            {
                dnaStartText = "|POOL=0,";
            }

            int dataStart = 0;
            string line;
            while ((line = _debugReader.ReadLine()) != null)
            {
                if (line.IndexOf(dnaStartText) > 0)
                {
                    dataStart = line.IndexOf("STATUS=S");
                    if (dataStart > 0)
                    {
                        string data = line.Substring(dataStart);
                        if (!string.IsNullOrEmpty(data) && _onInfoReceived != null)
                        {
                            _onInfoReceived(apiName, data);
                        }

                        return true;
                    }
                }
            }

            return false;
        }
    }
}
