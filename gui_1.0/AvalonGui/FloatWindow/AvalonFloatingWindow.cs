using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using AvalonGui.Properties;

namespace AvalonGui.FloatWindow
{
    public class AvalonFloatingWindow : FloatingWindow
    {
        private MainForm _mainForm;
        
        // 弹出菜单
        private const int MenuWidth = 108;
        private const int MenuHeight = 128;
        private const int MenuTextTopGap = 8;
        private bool _contextMenuVisible = false;
        private Point _rightClickPoint;
        private int _menuItemHeight = 32; // 128 / 4
        const string FontName = "SimSun";
        

        // 显示信息的区域
        private Rectangle _imageRect = new Rectangle(15, 11, 150, 70);
        private Rectangle _circleRect = new Rectangle(15, 11, 68, 70);
        private Rectangle _moveRect = new Rectangle(20, 18, 58, 56);
        private Rectangle _menuRect = new Rectangle(88, 25, 70, 50);

        private static RectangleF _usbCountRect = new Rectangle(48, 36, 25, 25);
        //private static RectangleF _mhsRect = new Rectangle(96, 43, 45, 28);
        private static RectangleF _mhsRect = new Rectangle(90, 43, 57, 28);

        DrawInfo[] _DrawInfos = new DrawInfo[]
        {
            new DrawInfo(_usbCountRect, 18, Color.White),
            new DrawInfo(_mhsRect, 22, Color.FromArgb(255, 0x4C, 0x73, 0xE0)),
        };

        // draw info
        private int _usbCount;
        private double _curMHS;

        public AvalonFloatingWindow()
        {
            base.MoveRect = _moveRect;
            base.ContextableRect = _menuRect;
        }

        public MainForm MainForm
        {
            get
            {
                return _mainForm;
            }

            set
            {
                _mainForm = value;
            }
        }

        public int USBCount
        {
            get
            {
                return _usbCount;
            }

            set
            {
                _usbCount = value;
                Invalidate();
            }
        }

        public double CurMHS
        {
            get
            {
                return _curMHS;
            }

            set
            {
                _curMHS = value;
                Invalidate();
            }
        }


        protected override void PerformPaint(PaintEventArgs e)
        {
            base.PerformPaint(e);
            if (!base.Minimized)
            {
                Rectangle r = base.ClientRectangle;
                r.Y += 20;
                r.Height -= 20;

                Image wndImage = Resources.floatwnd;
                Graphics g = e.Graphics;

                g.DrawImage(wndImage, 0, 0);

                StringFormat align = new StringFormat();
                align.Alignment = StringAlignment.Center;

                // draw usb count
                Font font = new Font(FontName, _DrawInfos[0].FontSize, FontStyle.Bold);
                Brush brush = new SolidBrush(_DrawInfos[0].FontColor);
                g.DrawString(_usbCount.ToString(), font, brush, _DrawInfos[0].Rect, align);

                font = new Font(FontName, _DrawInfos[1].FontSize, FontStyle.Bold);
                brush = new SolidBrush(_DrawInfos[1].FontColor);
                g.DrawString(_curMHS.ToString(), font, brush, _DrawInfos[1].Rect, align);

                if (_contextMenuVisible)
                {
                    g.DrawImage(Resources.popmenu, _rightClickPoint);

                    string menuItemText = _mainForm.GetContextMenuText(0);
                    Font menuItemFont = new Font(FontName, 10);
                    g.DrawString(menuItemText, menuItemFont, Brushes.Black, _rightClickPoint.X + MenuTextTopGap, _rightClickPoint.Y + MenuTextTopGap);
                    g.DrawString("隐藏悬浮窗", menuItemFont, Brushes.Black, _rightClickPoint.X + MenuTextTopGap, _rightClickPoint.Y + _menuItemHeight + MenuTextTopGap);
                    g.DrawString("安全移除设备", menuItemFont, Brushes.Black, _rightClickPoint.X + MenuTextTopGap, _rightClickPoint.Y + _menuItemHeight * 2 + MenuTextTopGap);
                    g.DrawString("关于", menuItemFont, Brushes.Black, _rightClickPoint.X + MenuTextTopGap, _rightClickPoint.Y + _menuItemHeight * 3 + MenuTextTopGap);
                }
            }
        }

        private void ShowMenu(bool show)
        {
            _contextMenuVisible = show;

            this.ContextMenuVisible = show;

            if (show)
            {
                this.ContextMenuRect = new Rectangle(_rightClickPoint.X, _rightClickPoint.Y, MenuWidth, MenuHeight);
            }

            Invalidate();
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="point"></param>
        /// <returns>
        ///     -1 no item clicked
        ///     0 open home
        ///     1 hide float
        ///     2 remove device
        /// </returns>
        private int GetMenuId(Point point)
        {
            if (!_contextMenuVisible)
            {
                return -1;
            }

            Rectangle menuRect = new Rectangle(_rightClickPoint.X, _rightClickPoint.Y, MenuWidth, MenuHeight);
            if (!menuRect.Contains(point))
            {
                return -1;
            }

            for (int i = 0; i < 4; i++)
            {
                if (point.Y > _rightClickPoint.Y + _menuItemHeight * i + 2 && point.Y < _rightClickPoint.Y + _menuItemHeight * (i + 1) - 2)
                {
                    return i;
                }
            }

            return -1;
        }

        protected override void WndProc(ref Message m)
        {
            int num1 = m.Msg;
            Point point = new Point(m.LParam.ToInt32());

            switch (num1)
            {
                case 0x202: // left button down
                    
                    if (_contextMenuVisible)
                    {
                        int menuId = GetMenuId(point);
                        if (menuId != -1)
                        {
                            NotifyMainWindow(menuId);
                        }
                    }

                    ShowMenu(false);
                    User32.ReleaseCapture();

                    return;
                case 0x205: // right button up
                    
                    if (_contextMenuVisible)
                    {
                        int menuId = GetMenuId(point);
                        if (menuId != -1)
                        {
                            NotifyMainWindow(menuId);
                        }

                        ShowMenu(false);
                    }
                    else
                    {
                        _rightClickPoint = point;

                        //if (_menuRect.Contains(point))
                        {
                            ShowMenu(true);
                        }
                    }

                    break;

                default:
                    break;
            }

            base.WndProc(ref m);
        }

        private void NotifyMainWindow(int type)
        {
            _mainForm.OnAction(type);
        }
    }
}
