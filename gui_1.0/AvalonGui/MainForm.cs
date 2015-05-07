using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows.Forms;

using AvalonGui.FloatWindow;
using AvalonGui.Model;
using AvalonGui.Utils;
using AvalonGui.Properties;
using NamedPipeWrapper;
using Newtonsoft.Json;
using AvalonDevice;
using Avalon.Utils;

namespace AvalonGui
{
    public partial class MainForm : Form
    {
        private static readonly log4net.ILog LOG
             = log4net.LogManager.GetLogger(System.Reflection.MethodBase.GetCurrentMethod().DeclaringType);

        // 广告相关
        AvalonAd _avalonAd = null;
        bool _formClosing = false;
        int _currentAdImageId = 0;
        object _synAccessAvalonAd = new object();

        // BitCoin挖矿
        const string ConfigFileName = "avalon.ini";
        const int SummaryQueueSize = 10;

        // 和服务通讯
        private readonly NamedPipeServer<string> _namedpipeServer = new NamedPipeServer<string>(Constants.NAMEDPIPE_SERVER_NAME);

        // 路径
        static string _currentDir = AppDomain.CurrentDomain.BaseDirectory;
        string _bfgMinerDir = string.Empty;
        string _bfgMinerPath = string.Empty;
        string _configPath = string.Empty;

        // 配置、数据
        AvalonConfig _appConfig = null;
        RPCThread _rpcThread = null;
        ProcessManager _processManager = null;
        Process _processMiner = null;
        Queue<SummaryResult> _summaryQueue = null;
        SummaryResult _currentSummaryResult = null;
        PoolResult _currentPoolResult = null;
        bool _myFormDragging = false;
        Point _myPointClicked;

        // 显示MHS，和折线区域
        int _usbCount = 0;
        DrawInfo[] _MHSDrawInfos = new DrawInfo[]
        {
            new DrawInfo(new RectangleF(178, 55, 40, 26), 14, Color.FromArgb(255, 0xCE, 0xFE, 0xE5)),
            new DrawInfo(new RectangleF(405, 105, 60, 28), 18, Color.FromArgb(255, 0x2A, 0x6D, 0xF5)),
            new DrawInfo(new RectangleF(405, 210, 60, 28), 18, Color.FromArgb(255, 0xF2, 0xF9, 0x2D)),
        };

        const int MaxGHSValue = 10;

        // 配置区域
        int[] _configTextAreaWidthArray = { 144, 104, 53, 60 };  // width of url,workername, port, last commit
        int _configTextAreaHeight = 20;  // item height
        Point _configTextAreaLeftTopPoint = new Point(60, 57); // left top 
        int _configTextAreaVGap = 50;  // vertical gap
        //int[] _configTextAreaHGaps = { 9, 13, 50 };
        int[] _configTextAreaHGaps = { 9, 13, 45 };

        // { 65, 246, 335, 97 };
        Rectangle _graphRect = new Rectangle(67, 97, 266, 148);
        Color _graphColor = Color.FromArgb(255, 0x33, 0x7E, 0x58);
        string _fontName = "SimSun";

        // 悬浮窗
        AvalonFloatingWindow _avalonFloatWindow = new AvalonFloatingWindow();
        bool _avalonFloatWindowVisible = false;

        // about form
        AboutForm _aboutForm = new AboutForm();

        public MainForm()
        {
            InitializeComponent();

            ButtonUtil.SetButtonImages(buttonQuitApp, Resources.close, Resources.close);
            ButtonUtil.SetButtonImages(buttonSettings, Resources.btn_set_push, Resources.btn_set_normal);

            buttonQuitApp.Visible = false;
        }

        public void SetUSBCount(int usbCount)
        {
            _usbCount = usbCount;
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            _avalonFloatWindow.MainForm = this;

            ThreadPool.QueueUserWorkItem(new WaitCallback(GetAdProc), this);
            ThreadPool.QueueUserWorkItem(new WaitCallback(UpdateAdPictureProc), this);

            string temp = _currentDir.TrimEnd("\\/".ToCharArray());
            string parentDir = Path.GetDirectoryName(temp);
            _bfgMinerDir = Path.Combine(parentDir, Constants.BfgMinerDirName);
            _bfgMinerPath = Path.Combine(_bfgMinerDir, Constants.BfgMinerFileName);
            _configPath = Path.Combine(_currentDir, ConfigFileName);

            LOG.Info("config path: " + _configPath);
            _appConfig = new AvalonConfig(_configPath);
            _appConfig.LoadConfig();

            _processManager = new ProcessManager();
            _summaryQueue = new Queue<SummaryResult>();

            //DebugMinerData();

            string windowsRoot = Environment.GetEnvironmentVariable("SystemRoot");
            string systemDir = Environment.SystemDirectory;

            string pathValue = string.Format("{0};{1};{2};", _bfgMinerDir, systemDir, windowsRoot);
            Environment.SetEnvironmentVariable("PATH", pathValue);
            
            int screenWidth = Screen.PrimaryScreen.Bounds.Width;
            int screenHeight = Screen.PrimaryScreen.Bounds.Height;
            //_avalonFloatWindow.Location = new Point(screenWidth - 186 - 50, 50);
            _avalonFloatWindow.Location = new Point(screenWidth - 261 - 50, 50);
            _avalonFloatWindow.USBCount = DeviceCounter.GetNanoCount();

            LOG.Info("Form_load, usb count: " + _avalonFloatWindow.USBCount);

            if (_avalonFloatWindow.USBCount > 0)
            {
                RunMinerAndMonitorData();
                _avalonFloatWindow.Show();
                notifyIcon1.Visible = true;
                _avalonFloatWindowVisible = true;
            }
            else
            {
                //_avalonFloatWindow.Show();
                notifyIcon1.Visible = false;
                _avalonFloatWindowVisible = false;
                this.Visible = this.ShowInTaskbar = false;
            }

            _namedpipeServer.ClientConnected += OnClientConnected;
            _namedpipeServer.ClientDisconnected += OnClientDisconnected;
            _namedpipeServer.ClientMessage += OnClientMessage;
            _namedpipeServer.Start();
        }


        protected override void WndProc(ref Message msg)
        {
            const int WM_SYSCOMMAND = 0x0112;
            const int SC_CLOSE = 0xF060;

            if (msg.Msg == WM_SYSCOMMAND && ((int)msg.WParam == SC_CLOSE))
            {
                // 点击winform右上关闭按钮
                // 加入想要的逻辑处理
                ShowHome(false);

                return;
            }

            int num1 = msg.Msg;
            if ((num1 == (0x400 + 103))&&this.ShowInTaskbar==false)
            {
                num1 -= 1;
            }
            switch (num1)
            {
                case 0x0400 + 100: // show floating window
                    {
                        LOG.Info("MainForm.WndProc: msg=" + num1);
                        _avalonFloatWindow.Show();
                        return;
                    }
                case 0x0400 + 101: // hide floating window
                    {
                        LOG.Info("MainForm.WndProc: msg=" + num1);
                        _avalonFloatWindow.Hide();
                        return;
                    }

                case 0x400 + 102: // show home
                    notifyIcon1.Visible = true;
                    Show();
                    this.ShowInTaskbar = true;
                    this.BringToFront();
                    return;
                case 0x400 + 103: // hide home
                    Hide();
                    this.ShowInTaskbar = false;
                    return;
            }

            base.WndProc(ref msg);
        }

        private void notifyIcon1_DoubleClick(object sender, EventArgs e)
        {
            Show();
            WindowState = FormWindowState.Normal;
        }

        private void Form1_Resize(object sender, EventArgs e)
        {
            if (FormWindowState.Minimized == WindowState)
                Hide();
        }

        // 来自悬浮窗的菜单
        public void OnAction(int actionType)
        {
            switch (actionType)
            {
                case 0:
                    ShowHome(FormWindowState.Minimized == WindowState);
                    break;
                case 1:
                    _avalonFloatWindowVisible = false;
                    ShowFloatWindow(false);
                    break;
                case 2:
                    SafeRemoveDevice();
                    break;
                case 3:
                    ShowAbout(true);
                    break;
            }
        }

        // 打开主页
        private void dIsplayMainUIToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ShowHome(FormWindowState.Minimized == WindowState);
        }

        // 隐藏悬浮窗
        private void toolStripMenuItem1_Click(object sender, EventArgs e)
        {
            _avalonFloatWindowVisible = !_avalonFloatWindowVisible;

            ShowFloatWindow(_avalonFloatWindowVisible);
        }

        private void ShowFloatWindow(bool show)
        {
            int msg = 0x0400 + 100 + (show ? 0 : 1);
            IntPtr hWnd = this.Handle;

            LOG.Info(string.Format("ShowFloatWindow: (show={0}, hwnd={1}, msg={2})", show, hWnd, msg));

            User32.PostMessage(hWnd, msg, 0, 0);
        }

        // 安全移除设备
        private void safeRemoveUSBsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            SafeRemoveDevice();
        }

        private void SafeRemoveDevice()
        {
            try
            {
                notifyIcon1.Visible = false;
                ShowFloatWindow(false);
                ShowHome(false);
                ShowAbout(false);
            }
            catch (Exception ex)
            {
                LOG.Error(ex);
            }

            SafeCloseBfgMiner();
        }

        // 关于
        private void toolStripMenuItemAbout_Click(object sender, EventArgs e)
        {
            ShowAbout(true);
        }

        private void ShowAbout(bool show)
        {
            SafeControlUpdater.ShowForm(_aboutForm, show);

        }


        // start BfgMiner and request data through RPC
        private void RunMinerAndMonitorData()
        {
            string arguments = _appConfig.MinerInfo.GetCommandLine();
            LOG.Info("arguments: " + arguments);

            if (!string.IsNullOrEmpty(arguments))
            {
                _processMiner = _processManager.RunApplication2(Constants.BfgMinerFileName, _bfgMinerPath, arguments);

                Thread.Sleep(5000);
                _rpcThread = new RPCThread(Constants.BfgMinerServerAddress, Constants.BfgMinerServerPort, this.OnDataReceived);
                _rpcThread.Start(_appConfig.DebugData);
            }
        }

        // Miner data received
        private void OnDataReceived(string apiName, string content)
        {
            LOG.Debug(apiName);
            LOG.Info(content);

            try
            {
                if (ParseData(apiName, content))
                {
                    pictureBoxGraph.Invalidate();
                }
            }
            catch (Exception ex)
            {
                LOG.Error(ex);
            }
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            LOG.Info("===enter Form1_FormClosing===");

            _formClosing = true;

            SafeCloseBfgMiner();

            _namedpipeServer.Stop();
        }

        private bool ParseData(string apiName, string content)
        {
            string[] items = content.Split("|".ToCharArray(), StringSplitOptions.RemoveEmptyEntries);

            if (items == null || items.Length < 2)
            {
                return false;
            }

            if (string.Compare(apiName, "summary", true) == 0)
            {
                SummaryResult summary = new SummaryResult();
                summary.Header.Deserialize(items[0]);
                summary.Summary.Deserialize(items[1]);

                if (_summaryQueue.Count > SummaryQueueSize)
                {
                    _summaryQueue.Dequeue();
                }

                _summaryQueue.Enqueue(summary);

                UpdateSummary(summary);

                return true;
            }
            else if (string.Compare(apiName, "pools", true) == 0)
            {
                PoolResult poolResult = new PoolResult();
                poolResult.Header.Deserialize(items[0]);
                int poolSize = items.Length - 1;
                int v = 0;

                if (Int32.TryParse(poolResult.Header.Msg, out v))
                {
                    if (v < poolSize)
                    {
                        poolSize = v;
                    }
                }

                if (poolSize > 0)
                {
                    Pool[] pools = new Pool[poolSize];
                    for (int i = 0; i < poolSize; i++)
                    {
                        Pool pi = new Pool();
                        pi.Deserialize(items[i + 1]);
                        pools[i] = pi;
                    }
                    poolResult.Pools = pools;
                }

                _currentPoolResult = poolResult;

                UpdatePoolInfo(poolResult);

                return true;
            }

            return false;
        }

        private void pictureBoxGraph_Paint(object sender, PaintEventArgs e)
        {
            Graphics g = e.Graphics;

            // close

            try
            {
                g.DrawImage(Resources.close, pictureBoxGraph.Width - 15 * 2, 15);

                DrawSummary(g, _currentSummaryResult);

                DrawTrends(g);
            }
            catch (Exception ex)
            {
                LOG.Error(ex);
            }
        }

        private double SafeGHS(double ghs)
        {
            if (ghs > MaxGHSValue)
            {
                return MaxGHSValue;
            }

            return ghs;
        }

        private void DrawSummary(Graphics g, SummaryResult summary)
        {
            if (g == null || summary == null)
            {
                return;
            }

            StringFormat align = new StringFormat();
            align.Alignment = StringAlignment.Center;

            // draw MHS total
            Font font = new Font(_fontName, _MHSDrawInfos[0].FontSize, FontStyle.Bold);
            Brush brush = new SolidBrush(_MHSDrawInfos[0].FontColor);
            g.DrawString(SafeGHS(summary.Summary.GetCurGHS()).ToString(), font, brush, _MHSDrawInfos[0].Rect, align);

            // draw MHS av
            font = new Font(_fontName, _MHSDrawInfos[1].FontSize, FontStyle.Bold);
            brush = new SolidBrush(_MHSDrawInfos[1].FontColor);
            g.DrawString(SafeGHS(summary.Summary.GetGHSAv()).ToString(), font, brush, _MHSDrawInfos[1].Rect, align);

            // draw MHS 300s
            font = new Font(_fontName, _MHSDrawInfos[2].FontSize, FontStyle.Bold);
            brush = new SolidBrush(_MHSDrawInfos[2].FontColor);
            g.DrawString(SafeGHS(summary.Summary.GetGHSByInterval(_appConfig.MHSSeconds)).ToString(), font, brush, _MHSDrawInfos[2].Rect, align);
        }

        private void DrawTrends(Graphics g)
        {
            //Pen pen = new Pen(_graphColor, 2);

            if (_summaryQueue.Count < 2)
            {
                return;
            }

            SummaryResult[] summarys = _summaryQueue.ToArray();

            double xRatio = _graphRect.Width * 1.0 / (SummaryQueueSize - 1);
            double yRatio = _graphRect.Height * 1.0 / MaxGHSValue;

            PointF[] points = new PointF[summarys.Length];

            float x = _graphRect.Left;
            float y = (float)(_graphRect.Bottom - SafeGHS(summarys[0].Summary.GetCurGHS()) * yRatio);
            points[0] = new PointF(x, y);
            for (int i = 1; i < summarys.Length; i++)
            {
                x = (float)(_graphRect.Left +  xRatio * i);
                y = (float)(_graphRect.Bottom - (SafeGHS(summarys[i].Summary.GetCurGHS()) * yRatio));
                points[i] = new PointF(x, y);
            }

            GraphicsPath path = new GraphicsPath();
            path.AddLines(points);

            Rectangle rect = new Rectangle(_graphRect.X, _graphRect.Y - 1, _graphRect.Width, _graphRect.Height + 2);
            g.Clip = new Region(rect);

            using (Pen pen = new Pen(_graphColor, 2))
            {
                g.DrawPath(pen, path);
            }
        }

        private void pictureBox1_Paint(object sender, PaintEventArgs e)
        {
            Graphics g = e.Graphics;

            try
            {
                DrawConfig(g);
            }
            catch (Exception ex)
            {
                LOG.Error(ex);
            }
        }

        private void DrawConfig(Graphics g)
        {
            Font font = new Font(_fontName, 8);

            StringFormat drawFormat = new StringFormat();
            drawFormat.Trimming = StringTrimming.EllipsisCharacter;
            drawFormat.FormatFlags = StringFormatFlags.NoWrap;

            int i = 0;
            int x = _configTextAreaLeftTopPoint.X;
            int y = _configTextAreaLeftTopPoint.Y;
            for (; i < _appConfig.MinerInfo.PoolInfos.Length && i < 3; i++)
            {
                // url
                RectangleF rect = new RectangleF(x, y, _configTextAreaWidthArray[0], _configTextAreaHeight);
                g.DrawString(_appConfig.MinerInfo.PoolInfos[i].PoolURL, font, Brushes.Black, rect, drawFormat);

                x += _configTextAreaWidthArray[0] + _configTextAreaHGaps[0];
                rect = new RectangleF(x, y, _configTextAreaWidthArray[1], _configTextAreaHeight);
                g.DrawString(_appConfig.MinerInfo.PoolInfos[i].WorkerName, font, Brushes.Black, rect, drawFormat);

                x += _configTextAreaWidthArray[1] + _configTextAreaHGaps[1];
                rect = new RectangleF(x, y, _configTextAreaWidthArray[2], _configTextAreaHeight);
                g.DrawString(_appConfig.MinerInfo.PoolInfos[i].PoolPort, font, Brushes.Black, rect, drawFormat);

                //_currentPoolResult
                bool active = false;
                x += _configTextAreaWidthArray[2] + _configTextAreaHGaps[2];

                string timeString = TimeConvertor.GetTimeString(0);
                if (_currentPoolResult != null && i < _currentPoolResult.Pools.Length)
                {
                    timeString = TimeConvertor.GetTimeString(_currentPoolResult.Pools[i].Last_Share_Time * 1000);

                    if (_currentPoolResult.Pools[i].Status.Equals("Alive", StringComparison.InvariantCultureIgnoreCase))
                    {
                        active = true;
                    }
                }

                rect = new RectangleF(x, y, _configTextAreaWidthArray[2], _configTextAreaHeight);
                g.DrawString(timeString, font, Brushes.Black, rect, drawFormat);

                if (active)
                {
                    g.DrawImage(Resources.online, x - 20 - 20, y - 5);
                }
                else
                {
                    g.DrawImage(Resources.offline, x - 20 - 20, y - 5);
                }

                x = _configTextAreaLeftTopPoint.X;
                y += _configTextAreaVGap;
            }
        }


        private void buttonSettings_Click(object sender, EventArgs e)
        {
            SettingsForm sf = new SettingsForm();

            Rectangle rect = this.ClientRectangle;
            Rectangle gRect = this.RectangleToScreen(rect);
            sf.ParentScreenRect = gRect;

            sf.SetConfing(_appConfig);

            if (sf.ShowDialog(this) == System.Windows.Forms.DialogResult.OK)
            {
                _appConfig.SaveConfig();
                UpdateUIByConfig(_appConfig);

                // restart bfgminer
                RestartBfgMiner();
            }
        }

        private void RestartBfgMiner()
        {
            ThreadPool.QueueUserWorkItem(new WaitCallback(RestartBfgMinerProc), this);
        }

        private void RestartBfgMinerProc(object state)
        {
            SafeCloseBfgMiner();

            _summaryQueue.Clear();
            _currentSummaryResult = null;
            _currentPoolResult = null;

            SafeControlUpdater.Invalidate(pictureBox1);
            SafeControlUpdater.Invalidate(pictureBoxGraph);

            _avalonFloatWindow.CurMHS = 0;

            Thread.Sleep(100);

            RunMinerAndMonitorData();
        }


        private void SafeCloseBfgMiner()
        {
            if (_rpcThread != null)
            {
                _rpcThread.Stop();
                _rpcThread = null;
            }

            LOG.Info("Try to close BfgMiner.");

            if (_processMiner != null)
            {
                try
                {
                    _processMiner.Kill();
                    _processMiner = null;
                }
                catch (Exception ex)
                {
                    LOG.Error(ex);
                }
            }

            TryKillBfgMiner();
        }

        private void UpdateSummary(SummaryResult summary)
        {
            _currentSummaryResult = summary;
            _avalonFloatWindow.CurMHS = SafeGHS(summary.Summary.GetCurGHS());

            SafeControlUpdater.Invalidate(this.pictureBoxGraph);
            _avalonFloatWindow.Invalidate();
        }

        private void UpdatePoolInfo(PoolResult poolResult)
        {
            if (poolResult == null)
            {
                return;
            }

            Control[] poolStatusIcons = { pictureBoxPoolStatus1, pictureBoxPoolStatus2, pictureBoxPoolStatus3 };

            int i = 0;
            for (; i < poolResult.Pools.Length && i < 3; i++)
            {
                if (poolResult.Pools[i].Status == "Alive")
                {
                    SafeControlUpdater.SetbackgroundImage(poolStatusIcons[i], Resources.online);
                }
                else
                {
                    SafeControlUpdater.SetbackgroundImage(poolStatusIcons[i], Resources.offline);
                }
            }

            SafeControlUpdater.Invalidate(this.pictureBox1);
        }

        private void buttonQuitApp_Click(object sender, EventArgs e)
        {
            ShowHome(false);
        }

        private void ShowHome(bool show)
        {
            //SafeControlUpdater.ShowForm(this, show);

            User32.PostMessage(this.Handle, 0x400 + 102 + (show ? 0 : 1), 0, 0);
        }

        private void pictureBoxGraph_MouseDown(object sender, MouseEventArgs e)
        {
            if (new Rectangle(pictureBoxGraph.Width - 15 * 2, 15, 12, 12).Contains(e.X, e.Y))
            {
                ShowHome(false);
                return;
            }

            _myFormDragging = true;
            _myPointClicked = new Point(e.X, e.Y);
        }

        private void pictureBoxGraph_MouseUp(object sender, MouseEventArgs e)
        {
            _myFormDragging = false;
        }

        private void pictureBoxGraph_MouseMove(object sender, MouseEventArgs e)
        {
            if (_myFormDragging)
            {
                Point aMoveToPoint;

                aMoveToPoint = this.PointToScreen(new Point(e.X, e.Y));
                aMoveToPoint.Offset(_myPointClicked.X * -1, (_myPointClicked.Y) * -1);
                Location = aMoveToPoint;
            }
        }

        // 客户端连接上了NamedPipe
        private void OnClientConnected(NamedPipeConnection<string, string> connection)
        {
            connection.PushMessage(Constants.NamedPipe_Connect_Message);
            connection.PushMessage(Constants.NamedPipe_Command_USBCounting);
        }

        // 客户端断开了NamedPipe
        private void OnClientDisconnected(NamedPipeConnection<string, string> connection)
        {
            LOG.Info(string.Format("client {0} disconnected.", connection.Name));
        }

        // 客户端发送的消息
        private void OnClientMessage(NamedPipeConnection<string, string> connection, string message)
        {
            LOG.Info("Message from service: " + message);

            // we only handle quit message
            if (message == Constants.NamedPipe_Command_Quit)
            {
                // quit app
                LOG.Info("client request to quit me.");
                SafeRemoveDevice();
            }
            else if (message.StartsWith(Constants.NamedPipe_Command_USBCounting + " "))
            {
                string cnt = message.Substring(Constants.NamedPipe_Command_USBCounting.Length + 1);

                LOG.Info("usb count=" + cnt);

                int v = 0;
                if (Int32.TryParse(cnt, out v))
                {
                    if (v > 0)
                    {
                        _usbCount = v;
                        _avalonFloatWindow.USBCount = v;

                        //bool postSucceed = User32.PostMessage(_avalonFloatWindow.Handle, 0x0400 + 100, 0, 0);
                        ShowFloatWindow(true);

                        LOG.Info("==ready to show home window");
                        //ShowHome(true);
                        //SafeControlUpdater.Invalidate(pictureBoxGraph);
                        this.notifyIcon1.Visible = true;

                        LOG.Info("==restart miner application");
                        RestartBfgMiner();
                    }
                    else
                    {
                        SafeRemoveDevice();
                    }
                }
            }
        }

        private void UpdateUIByConfig(AvalonConfig config)
        {
            pictureBox1.Invalidate();
        }

        // 获取广告线程处理函数
        private void GetAdProc(object instance)
        {
            while (!_formClosing)
            {
                string adContent = string.Empty;

                try
                {
                    adContent = WebClient.GetResponseString(Constants.AdUrl, Encoding.UTF8);
                }
                catch (Exception ex)
                {
                    LOG.Error(ex);
                }

                if (!string.IsNullOrEmpty(adContent))
                {
                    AvalonAd ad = null;
                    try
                    {
                        ad = JsonConvert.DeserializeObject<AvalonAd>(adContent);

                        if (ad != null)
                        {
                            lock (_synAccessAvalonAd)
                            {
                                _avalonAd = ad;
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        LOG.Error(ex);
                    }
                }

                // update ad each 10 minutes: 10 * 60 * 1000 is 10 minutes
                for (int i = 0; i < 10 * 60 * 5; i++)
                {
                    if (_formClosing) break;
                    Thread.Sleep(200);
                }
            }
        }

        // 更新广告图片处理函数：每5秒更新图片
        private void UpdateAdPictureProc(object instance)
        {
            while (!_formClosing)
            {
                lock (_synAccessAvalonAd)
                {
                    if (_avalonAd != null)
                    {
                        if (_currentAdImageId > _avalonAd.lists.Length - 1)
                        {
                            _currentAdImageId = 0;
                        }

                        pictureBoxAd.ImageLocation = _avalonAd.lists[_currentAdImageId].img;
                        _currentAdImageId++;
                    }
                }

                // update ad each 5 seconds: 5000 is 5 minutes
                for (int i = 0; i < 25; i++)
                {
                    if (_formClosing) break;
                    Thread.Sleep(200);
                }
            }

        }

        private void TryKillBfgMiner()
        {
            ProcessKiller.KillProcessByName(Constants.BfgMinerDirName);

            _processMiner = null;
        }

        public string GetContextMenuText(int menuItemId)
        {
            string text = string.Empty;

            switch (menuItemId)
            {
                case 0: // home
                    //if (WindowState == FormWindowState.Normal)
                    if (this.Visible)
                    {
                        text = "关闭主页";
                    }
                    else
                    {
                        text = "打开主页";
                    }
                    break;
                case 1:
                    //if (_avalonFloatWindowVisible)
                    if (_avalonFloatWindow.Visible)
                    {
                        text = "隐藏悬浮窗";
                    }
                    else
                    {
                        text = "打开悬浮窗";
                    }
                    break;
                case 2:
                    text = "安全移除设备";
                    break;
                case 3:
                    text = "关于";
                    break;
            }

            return text;
        }

        private void contextMenuStrip1_Opening(object sender, CancelEventArgs e)
        {
            this.dIsplayMainUIToolStripMenuItem.Text = GetContextMenuText(0);
            this.toolStripMenuItem1.Text = GetContextMenuText(1);
            this.safeRemoveUSBsToolStripMenuItem.Text = GetContextMenuText(2);
        }

        // STATUS=S,When=1408776027,Code=11,Msg=Summary,Description=bfgminer 4.4.0|SUMMARY,Elapsed=700,MHS av=4504.326,MHS 20s=4499.603,Found Blocks=0,Getworks=25,Accepted=1,Rejected=0,Hardware Errors=40,Utility=0.086,Discarded=102,Stale=0,Get Failures=1,Local Work=1357,Remote Failures=0,Network Blocks=2,Total MH=3153177.9470,Diff1 Work=595,Work Utility=50.998,Difficulty Accepted=999.99999980,Difficulty Rejected=0,Difficulty Stale=0,Best Share=3390.82029769,Device Hardware%=6.2992,Device Rejected%=0.0000,Pool Rejected%=0.0000,Pool Stale%=0.0000,Last getwork=1408776027|
        // STATUS=S,When=1408776027,Code=7,Msg=3 Pool(s),Description=bfgminer 4.4.0|POOL=0,URL=stratum+tcp://p2pool.org:9332,Status=Alive,Priority=0,Quota=1,Long Poll=N,Getworks=25,Accepted=1,Rejected=0,Works=1230,Discarded=102,Stale=0,Get Failures=1,Remote Failures=0,User=mikeqin.avalon,Last Share Time=1408775441,Diff1 Shares=595,Proxy=,Difficulty Accepted=999.99999980,Difficulty Rejected=0,Difficulty Stale=0,Last Share Difficulty=999.99999980,Has Stratum=true,Stratum Active=true,Stratum URL=p2pool.org,Best Share=3390.82029769,Pool Rejected%=0.0000,Pool Stale%=0.0000|POOL=1,URL=stratum+tcp://au.ozco.in:80,Status=Dead,Priority=1,Quota=1,Long Poll=N,Getworks=0,Accepted=0,Rejected=0,Works=0,Discarded=0,Stale=0,Get Failures=0,Remote Failures=0,User=mikeqin.avalon,Last Share Time=0,Diff1 Shares=0,Proxy=,Difficulty Accepted=0,Difficulty Rejected=0,Difficulty Stale=0,Last Share Difficulty=0,Has Stratum=true,Stratum Active=false,Stratum URL=,Best Share=0,Pool Rejected%=0.0000,Pool Stale%=0.0000|POOL=2,URL=stratum+tcp://cn.ozco.in:80,Status=Dead,Priority=2,Quota=1,Long Poll=N,Getworks=0,Accepted=0,Rejected=0,Works=0,Discarded=0,Stale=0,Get Failures=0,Remote Failures=0,User=mikeqin.avalon,Last Share Time=0,Diff1 Shares=0,Proxy=,Difficulty Accepted=0,Difficulty Rejected=0,Difficulty Stale=0,Last Share Difficulty=0,Has Stratum=true,Stratum Active=false,Stratum URL=,Best Share=0,Pool Rejected%=0.0000,Pool Stale%=0.0000|
        private void DebugMinerData()
        {
            string summary = "STATUS=S,When=1408776027,Code=11,Msg=Summary,Description=bfgminer 4.4.0|SUMMARY,Elapsed=700,MHS av=4504.326,MHS 20s=4499.603,Found Blocks=0,Getworks=25,Accepted=1,Rejected=0,Hardware Errors=40,Utility=0.086,Discarded=102,Stale=0,Get Failures=1,Local Work=1357,Remote Failures=0,Network Blocks=2,Total MH=3153177.9470,Diff1 Work=595,Work Utility=50.998,Difficulty Accepted=999.99999980,Difficulty Rejected=0,Difficulty Stale=0,Best Share=3390.82029769,Device Hardware%=6.2992,Device Rejected%=0.0000,Pool Rejected%=0.0000,Pool Stale%=0.0000,Last getwork=1408776027|";
            string pool = "STATUS=S,When=1408776027,Code=7,Msg=3 Pool(s),Description=bfgminer 4.4.0|POOL=0,URL=stratum+tcp://p2pool.org:9332,Status=Alive,Priority=0,Quota=1,Long Poll=N,Getworks=25,Accepted=1,Rejected=0,Works=1230,Discarded=102,Stale=0,Get Failures=1,Remote Failures=0,User=mikeqin.avalon,Last Share Time=1408775441,Diff1 Shares=595,Proxy=,Difficulty Accepted=999.99999980,Difficulty Rejected=0,Difficulty Stale=0,Last Share Difficulty=999.99999980,Has Stratum=true,Stratum Active=true,Stratum URL=p2pool.org,Best Share=3390.82029769,Pool Rejected%=0.0000,Pool Stale%=0.0000|POOL=1,URL=stratum+tcp://au.ozco.in:80,Status=Dead,Priority=1,Quota=1,Long Poll=N,Getworks=0,Accepted=0,Rejected=0,Works=0,Discarded=0,Stale=0,Get Failures=0,Remote Failures=0,User=mikeqin.avalon,Last Share Time=0,Diff1 Shares=0,Proxy=,Difficulty Accepted=0,Difficulty Rejected=0,Difficulty Stale=0,Last Share Difficulty=0,Has Stratum=true,Stratum Active=false,Stratum URL=,Best Share=0,Pool Rejected%=0.0000,Pool Stale%=0.0000|POOL=2,URL=stratum+tcp://cn.ozco.in:80,Status=Dead,Priority=2,Quota=1,Long Poll=N,Getworks=0,Accepted=0,Rejected=0,Works=0,Discarded=0,Stale=0,Get Failures=0,Remote Failures=0,User=mikeqin.avalon,Last Share Time=0,Diff1 Shares=0,Proxy=,Difficulty Accepted=0,Difficulty Rejected=0,Difficulty Stale=0,Last Share Difficulty=0,Has Stratum=true,Stratum Active=false,Stratum URL=,Best Share=0,Pool Rejected%=0.0000,Pool Stale%=0.0000|";

            ParseData("Summary", summary);
            ParseData("Pools", pool);
        }
        
    }

    class DrawInfo
    {
        public RectangleF Rect;
        public int FontSize;
        public Color FontColor;

        public DrawInfo(RectangleF rect, int fontSize, Color fontColor)
        {
            Rect = rect;
            FontSize = fontSize;
            FontColor = fontColor;
        }
    }
}
