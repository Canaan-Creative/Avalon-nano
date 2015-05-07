/////////////////////////////////////////////////////////////////////////////////////////
//
// FloatingWindow
//
// Author      : Iulian Ionescu
// Company     : Olvio IT, Inc.
// Website     : http://www.olvio.com/
// Description : The class exposes a native window that can be displayed with transparency,
//               and can be moved and resized. The window shows a shadow and the painting can 
//               be overridden in an inherited class. 
//
// Disclaimer : THE FloatingWindow source code and binaries (the "PRODUCT") ARE PROVIDED 
//              FREE OF CHARGE, AND, THEREFORE, ON AN "AS IS" BASIS, WITHOUT WARRANTY OF 
//              ANY KIND, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE WARRANTIES 
//              THAT IT IS FREE OF DEFECTS, VIRUS FREE, ABLE TO OPERATE ON AN UNINTERRUPTED 
//              BASIS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE OR NON-INFRINGING. THIS 
//              DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL PART OF THE PRODUCT. NO USE 
//              OF THE PRODUCT IS AUTHORIZED HEREUNDER EXCEPT UNDER THIS DISCLAIMER. 
//
// Credits    : The idea in this code comes from various sources and it was inspired by 
//              a quesion on the vbcity.com forums.
//
// To do      : I couldn't find a way to determine the window under the native window, a 
//              window that is in a different thread, in order to pass the mouse events and 
//              have a complete "invisibility" for the window. 
//
// Products   : Check http://www.olvio.com for more Windows Forms products.
//
// Copyright 2002-2005 (c) Olvio IT, Inc. All Rights Reserved.
//
using System;
using System.Collections;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.ComponentModel;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace AvalonGui.FloatWindow
{
	/// <summary>
	/// Represents a floating native window with shadow, transparency and the ability to 
	/// move and be resized with the mouse.
	/// </summary>
	public class FloatingWindow: NativeWindow, IDisposable
	{
        internal static readonly log4net.ILog LOG
             = log4net.LogManager.GetLogger(System.Reflection.MethodBase.GetCurrentMethod().DeclaringType);

		#region #  Fields  #

		private bool isMouseIn;
		private bool onMouseMove;
		private bool onMouseDown;
		private bool onMouseUp;
		private bool minimizing;
		private Size sizeBeforeMinimize;
		private bool minimized;
		private int deltaX;
		private int deltaY;
		private bool captured;
		private bool resizing;
		private bool disposed;
		private int clientWidth;
		private int clientHeight;
		private bool hasShadow = true;
		private int shadowLength = 4;
		private Point location = new Point(0,0);
		//private Size size = new Size(186, 90);
        private Size size = new Size(266, 203); // for popup menu
		private int alpha = 210;
		private Control parent;
		private bool supportsLayered;
		private Point lastMouseDown = Point.Empty;
		private Rectangle clientRectangle;
		private Rectangle shadowRectangle;
		private string text = "Text";
		private int captionHeight = 20;
		private Rectangle resizeR;
        private Rectangle moveRect;
        private Rectangle contextableRect;
        private bool visible;

        private Rectangle contextMenuRect;
        private bool contextMenuVisible = false;

		#endregion

		#region #  Constructors  #

		/// <summary>
		/// Creates a new instance of the <see cref="FloatingWindow"/> class
		/// </summary>
		public FloatingWindow() : base()
		{
			this.supportsLayered = OSFeature.Feature.GetVersionPresent(OSFeature.LayeredWindows) != null;
		}

		~FloatingWindow()
		{
			this.Dispose(false);
		}

		#endregion

		#region #  Methods  #

		#region == Painting ==

		/// <summary>
		/// Performs the painting of the window.
		/// </summary>
		/// <param name="e">A <see cref="PaintEventArgs"/> containing the event data.</param>
		protected virtual void PerformPaint(PaintEventArgs e)
		{
			if (this.minimized)
			{
				this.resizeR = Rectangle.Empty;
			}
		}
		/// <summary>
		/// Raises the <see cref="Paint"/> event.
		/// </summary>
		/// <param name="e">A <see cref="PaintEventArgs"/> containing the event data.</param>
		public virtual void OnPaint(PaintEventArgs e)
		{
			this.PerformPaint(e);
			if (this.Paint!=null)
			{
				this.Paint(this, e);
			}
		}

		#endregion

		#region == Layout ==
		
		private void Minimize()
		{
			this.sizeBeforeMinimize = this.Size;
			this.minimized = true;
			this.minimizing = true;
			this.Size = new Size(this.Size.Width, captionHeight+this.shadowLength);
			this.Invalidate();
			this.minimizing = false;
		}
		private void Maximize()
		{
			this.minimized = false;
			this.Size = this.sizeBeforeMinimize;
			this.Invalidate();
		}
		private void PreCalculateLayout()
		{
		}
		private void ComputeLayout()
		{
			RECT rct = new RECT();
			POINT pnt = new POINT();
			User32.GetWindowRect(base.Handle, ref rct);
			pnt.x = rct.left;
			pnt.y = rct.top;
			User32.ScreenToClient(base.Handle, ref pnt);
			this.clientRectangle = new Rectangle(pnt.x, pnt.y, rct.right-rct.left, rct.bottom-rct.top);
			if (this.hasShadow)
			{
				this.clientRectangle.Width-=this.shadowLength;
				this.clientRectangle.Height-=this.shadowLength;
				this.shadowRectangle = this.clientRectangle;
				this.shadowRectangle.X+=this.shadowLength;
				this.shadowRectangle.Y+=this.shadowLength;
			}
			else
			{
				this.shadowRectangle = Rectangle.Empty;
			}
		}


		#endregion

		#region == Updating ==

		protected internal void Invalidate()
		{
			this.UpdateLayeredWindow();
		}
		private void UpdateLayeredWindow()
		{
			this.UpdateLayeredWindow(this.location, this.size, (byte)this.alpha);
		}
		private void UpdateLayeredWindowAnimate()
		{
			this.UpdateLayeredWindow(true);
		}
		
		private void UpdateLayeredWindow(bool animate)
		{
			if (animate)
			{
				for (int num1 = 0; num1 < 0xff; num1 += 3)
				{
					if (num1 == 0xfe)
					{
						num1 = 0xff;
					}
					this.UpdateLayeredWindow(this.location, this.size, (byte) num1);
				}
			}
			else
			{
				this.UpdateLayeredWindow(this.location, this.size, 0xff);
			}
		}
		private void UpdateLayeredWindow(byte alpha)
		{
			this.UpdateLayeredWindow(this.location, this.size, alpha);
		}
		private void UpdateLayeredWindow(Point point, Size size, byte alpha)
		{
			Bitmap bitmap1 = new Bitmap(size.Width, size.Height, PixelFormat.Format32bppArgb);
			using (Graphics graphics1 = Graphics.FromImage(bitmap1))
			{
				Rectangle rectangle1;
				SIZE size1;
				POINT point1;
				POINT point2;
				BLENDFUNCTION blendfunction1;
				rectangle1 = new Rectangle(0, 0, size.Width, size.Height);
				this.OnPaint(new PaintEventArgs(graphics1, rectangle1));
				IntPtr ptr1 = User32.GetDC(IntPtr.Zero);
				IntPtr ptr2 = Gdi32.CreateCompatibleDC(ptr1);
				IntPtr ptr3 = bitmap1.GetHbitmap(Color.FromArgb(0));
				IntPtr ptr4 = Gdi32.SelectObject(ptr2, ptr3);
				size1.cx = size.Width;
				size1.cy = size.Height;
				point1.x = point.X;
				point1.y = point.Y;
				point2.x = 0;
				point2.y = 0;
				blendfunction1 = new BLENDFUNCTION();
				blendfunction1.BlendOp = 0;
				blendfunction1.BlendFlags = 0;
				blendfunction1.SourceConstantAlpha = alpha;
				blendfunction1.AlphaFormat = 1;
				User32.UpdateLayeredWindow(base.Handle, ptr1, ref point1, ref size1, ptr2, ref point2, 0, ref blendfunction1, 2);
				Gdi32.SelectObject(ptr2, ptr4);
				User32.ReleaseDC(IntPtr.Zero, ptr1);
				Gdi32.DeleteObject(ptr3);
				Gdi32.DeleteDC(ptr2);
			}
		}

		
		#endregion

		#region == Show / Hide ==

		/// <summary>
		/// Shows the window.
		/// </summary>
		/// <remarks>
		/// Showing is done with animation.
		/// </remarks>
		public virtual void Show()
		{
			this.Show(this.X, this.Y);
		}
		/// <summary>
		/// Shows the window at the specified location.
		/// </summary>
		/// <param name="x">The horizontal coordinate.</param>
		/// <param name="y">The vertical coordinate.</param>
		/// <remarks>
		/// Showing is done with animation.
		/// </remarks>
		public virtual void Show(int x, int y)
		{
			this.Show(x, y, true);
		}
		/// <summary>
		/// Shows the window at the specified location.
		/// </summary>
		/// <param name="x">The horizontal coordinate.</param>
		/// <param name="y">The vertical coordinate.</param>
		/// <param name="animate"><b>true</b> if the showing should be done with animation; otherwise, <b>false</b>.</param>
		public virtual void Show(int x, int y, bool animate)
		{
            LOG.Info("==enter show");

			this.location = new Point(x, y);
			this.PreCalculateLayout();			
			CreateParams params1 = this.CreateParams;
			if (base.Handle == IntPtr.Zero)
			{
				this.CreateHandle(params1);
			}

            LOG.Info("==ready to compute layout");

			this.ComputeLayout();
			if (this.supportsLayered)
			{
				if (animate)
				{
                    LOG.Info("==show with animate");

					User32.ShowWindow(base.Handle, 4);
					System.Threading.Thread thread1 = new System.Threading.Thread(new System.Threading.ThreadStart(this.UpdateLayeredWindowAnimate));
					thread1.IsBackground = true;
					thread1.Start();
				}
				else
				{
                    LOG.Info("==show without animate");

					this.UpdateLayeredWindow();
				}
			}
			User32.ShowWindow(base.Handle, 4);

            this.visible = true;

            LOG.Info("==show finished.");
		}

		private void ShowInvisible(int x, int y)
		{
			CreateParams params1 = new CreateParams();
			params1.Caption = "FloatingNativeWindow";
			int nX = x;
			int nY = y;
			Screen screen1 = Screen.FromHandle(base.Handle);
			if ((nX + this.size.Width) > screen1.Bounds.Width)
			{
				nX = screen1.Bounds.Width - this.size.Width;
			}
			if ((nY + this.size.Height) > screen1.Bounds.Height)
			{
				nY = screen1.Bounds.Height - this.size.Height;
			}
			this.location = new Point(nX, nY);
			Size size1 = this.size;
			Point point1 = this.location;
			params1.X = nX;
			params1.Y = nY;
			params1.Height = size1.Height;
			params1.Width = size1.Width;
			params1.Parent = IntPtr.Zero;
			params1.Style = -2147483648;
			params1.ExStyle = 0x88;
			if (this.supportsLayered)
			{
				params1.ExStyle += 0x80000;
			}
			if (base.Handle == IntPtr.Zero)
			{
				this.CreateHandle(params1);
			}
			this.size = size1;
			this.location = point1;
			this.ComputeLayout();

            this.visible = true;
		}

		/// <summary>
		/// Shows the window with a specific animation.
		/// </summary>
		/// <param name="x">The horizontal coordinate.</param>
		/// <param name="y">The vertical coordinate.</param>
		/// <param name="mode">An <see cref="AnimateMode"/> parameter.</param>
		public virtual void ShowAnimate(int x, int y, AnimateMode mode)
		{
			uint flag = (uint)AnimateWindow.AW_CENTER;
			switch (mode)
			{
				case AnimateMode.Blend:
					this.Show(x, y, true);
					return;
				case AnimateMode.ExpandCollapse:
					flag = (uint)AnimateWindow.AW_CENTER;
					break;
				case AnimateMode.SlideLeftToRight:
					flag = (uint)(AnimateWindow.AW_HOR_POSITIVE | AnimateWindow.AW_SLIDE);
					break;
				case AnimateMode.SlideRightToLeft:
					flag = (uint)(AnimateWindow.AW_HOR_NEGATIVE | AnimateWindow.AW_SLIDE);
					break;
				case AnimateMode.SlideBottmToTop:
					flag = (uint)(AnimateWindow.AW_VER_POSITIVE | AnimateWindow.AW_SLIDE);
					break;
				case AnimateMode.SlideTopToBottom:
					flag = (uint)(AnimateWindow.AW_VER_NEGATIVE | AnimateWindow.AW_SLIDE);
					break;
				case AnimateMode.RollLeftToRight:
					flag = (uint)(AnimateWindow.AW_HOR_POSITIVE);
					break;
				case AnimateMode.RollRightToLeft:
					flag = (uint)(AnimateWindow.AW_HOR_NEGATIVE);
					break;
				case AnimateMode.RollBottmToTop:
					flag = (uint)(AnimateWindow.AW_VER_POSITIVE);
					break;
				case AnimateMode.RollTopToBottom:
					flag = (uint)(AnimateWindow.AW_VER_NEGATIVE);
					break;
			}
			if (this.supportsLayered)
			{
				this.ShowInvisible(x, y);
				this.UpdateLayeredWindow();
				User32.AnimateWindow(base.Handle, 200, flag);
			}
			else
			{
				this.Show(x, y);
			}
		}

		/// <summary>
		/// Hides the window with a specific animation.
		/// </summary>
		/// <param name="mode">An <see cref="AnimateMode"/> parameter.</param>
		public virtual void HideAnimate(AnimateMode mode)
		{
			uint flag = (uint)AnimateWindow.AW_CENTER;
			switch (mode)
			{
				case AnimateMode.Blend:
					this.HideWindowWithAnimation();
					return;
				case AnimateMode.ExpandCollapse:
					flag = (uint)AnimateWindow.AW_CENTER;
					break;
				case AnimateMode.SlideLeftToRight:
					flag = (uint)(AnimateWindow.AW_HOR_POSITIVE | AnimateWindow.AW_SLIDE);
					break;
				case AnimateMode.SlideRightToLeft:
					flag = (uint)(AnimateWindow.AW_HOR_NEGATIVE | AnimateWindow.AW_SLIDE);
					break;
				case AnimateMode.SlideBottmToTop:
					flag = (uint)(AnimateWindow.AW_VER_POSITIVE | AnimateWindow.AW_SLIDE);
					break;
				case AnimateMode.SlideTopToBottom:
					flag = (uint)(AnimateWindow.AW_VER_NEGATIVE | AnimateWindow.AW_SLIDE);
					break;
				case AnimateMode.RollLeftToRight:
					flag = (uint)(AnimateWindow.AW_HOR_POSITIVE);
					break;
				case AnimateMode.RollRightToLeft:
					flag = (uint)(AnimateWindow.AW_HOR_NEGATIVE);
					break;
				case AnimateMode.RollBottmToTop:
					flag = (uint)(AnimateWindow.AW_VER_POSITIVE);
					break;
				case AnimateMode.RollTopToBottom:
					flag = (uint)(AnimateWindow.AW_VER_NEGATIVE);
					break;
			}
			flag = (uint)((uint)flag | (uint)AnimateWindow.AW_HIDE);
			if (this.supportsLayered)
			{
				this.UpdateLayeredWindow();
				User32.AnimateWindow(base.Handle, 200, flag);
			}
			this.Hide();
		}
		/// <summary>
		/// Hides the window.
		/// </summary>
		public virtual void Hide()
		{
			if (this.captured)
			{
				User32.ReleaseCapture();
			}
			User32.ShowWindow(base.Handle, 0);
			this.ReleaseHandle();

            this.visible = false;
		}

		private void HideWindowWithAnimation()
		{
			if (this.supportsLayered)
			{
				for (int num1 = 0xff; num1 > 0; num1 -= 3)
				{
					if (num1 < 0)
					{
						num1 = 0;
					}
					this.UpdateLayeredWindow(this.location, this.size, (byte) num1);
				}
			}
			this.Hide();
		}


		#endregion

		#region == Mouse ==

		private POINT MousePositionToClient(POINT point)
		{
			POINT point1;
			point1.x = point.x;
			point1.y = point.y;
			User32.ScreenToClient(base.Handle, ref point1);
			return point1;
		}
		private POINT MousePositionToScreen(MSG msg)
		{
			POINT point1;
			point1.x = (short) (((int) msg.lParam) & 0xffff);
			point1.y = (short) ((((int) msg.lParam) & -65536) >> 0x10);
			if ((((msg.message != 0xa2) && (msg.message != 0xa8)) && ((msg.message != 0xa5) && (msg.message != 0xac))) && (((msg.message != 0xa1) && (msg.message != 0xa7)) && ((msg.message != 0xa4) && (msg.message != 0xab))))
			{
				User32.ClientToScreen(msg.hwnd, ref point1);
			}
			return point1;
		}
		private POINT MousePositionToScreen(POINT point)
		{
			POINT point1;
			point1.x = point.x;
			point1.y = point.y;
			User32.ClientToScreen(base.Handle, ref point1);
			return point1;
		}
		private POINT MousePositionToScreen(Message msg)
		{
			POINT point1;
			point1.x = (short) (((int) msg.LParam) & 0xffff);
			point1.y = (short) ((((int) msg.LParam) & -65536) >> 0x10);
			if ((((msg.Msg != 0xa2) && (msg.Msg != 0xa8)) && ((msg.Msg != 0xa5) && (msg.Msg != 0xac))) && (((msg.Msg != 0xa1) && (msg.Msg != 0xa7)) && ((msg.Msg != 0xa4) && (msg.Msg != 0xab))))
			{
				User32.ClientToScreen(msg.HWnd, ref point1);
			}
			return point1;
		}

		private void PerformWmMouseDown(ref Message m)
		{
            if (this.resizeR.Contains(this.lastMouseDown))
            {
                this.resizing = true;
            }
            this.captured = true;
            User32.SetCapture(base.Handle);
		}

		private void PerformWmMouseMove(ref Message m)
		{
			Point p = Control.MousePosition;
			POINT point1 = new POINT();
			point1.x = p.X;
			point1.y = p.Y;
			point1 = this.MousePositionToClient(point1);

			if (this.resizing || this.resizeR.Contains(point1.x, point1.y))
			{
				Cursor.Current = Cursors.SizeNWSE;
			}
			else
				Cursor.Current = Cursors.Arrow;

			if (this.captured)
			{
				if (this.resizing)
				{
					int w = System.Math.Max(50, (p.X+deltaX)-this.Location.X);
					int h = System.Math.Max(50, (p.Y+deltaY)-this.Location.Y);
					this.Size = new Size(w, h);
				}
				else
				{
					this.Location = new Point(p.X-deltaX, p.Y-deltaY);
				}
			}
		}
		private void PerformWmMouseUp(ref Message m)
		{
			this.resizing = false;
			if (this.captured)
			{
				this.captured = false;
				User32.ReleaseCapture();
			}
		}
		private void PerformWmMouseActivate(ref Message m)
		{
			m.Result = (IntPtr) 3;
		}


		protected virtual void OnMouseMove(MouseEventArgs e)
		{
			if (this.MouseMove!=null)
			{
				this.MouseMove(this, e);
			}
			this.onMouseMove = true;
		}
		protected virtual void OnMouseDown(MouseEventArgs e)
		{
			if (this.MouseDown!=null)
			{
				this.MouseDown(this, e);
			}
			this.onMouseDown = true;
		}
		protected virtual void OnMouseUp(MouseEventArgs e)
		{
			if (this.MouseUp!=null)
			{
				this.MouseUp(this, e);
			}
			this.onMouseUp = true;
		}

		protected virtual void OnMouseEnter()
		{
			if (this.MouseEnter!=null)
			{
				this.MouseEnter(this, EventArgs.Empty);
			}
		}
		protected virtual void OnMouseLeave()
		{
			if (this.MouseLeave!=null)
			{
				this.MouseLeave(this, EventArgs.Empty);
			}
		}

		#endregion

		#region == Other messages ==

		private bool PerformWmNcHitTest(ref Message m)
		{
			POINT point1;
			Point p = Control.MousePosition;
			point1.x = p.X;
			point1.y = p.Y;
			point1 = this.MousePositionToClient(point1);

			Rectangle rect = new Rectangle(0, 0, this.Width, this.captionHeight);

			if (this.resizeR.Contains(point1.x, point1.y))
			{
				return false;
			}
            //if (rect.Contains(point1.x, point1.y))
            //{
            //    return false;
            //}

            if (moveRect.Contains(point1.x, point1.y))
            {
                return false;
            }

            if (contextableRect.Contains(point1.x, point1.y))
            {
                return false;
            }

            if (contextMenuVisible && contextMenuRect.Contains(point1.x, point1.y))
            {
                return false;
            }

			m.Result = (IntPtr) (-1);
			return true;
		}
		
		private void PerformWmSetCursor(ref Message m)
		{
		}
		private void PerformWmPaint(ref Message m)
		{
			PAINTSTRUCT paintstruct1;
			RECT rect1;
			Rectangle rectangle1;
			paintstruct1 = new PAINTSTRUCT();
			IntPtr ptr1 = User32.BeginPaint(m.HWnd, ref paintstruct1);
			rect1 = new RECT();
			User32.GetWindowRect(base.Handle, ref rect1);
			rectangle1 = new Rectangle(0, 0, rect1.right - rect1.left, rect1.bottom - rect1.top);
			using (Graphics graphics1 = Graphics.FromHdc(ptr1))
			{
				Bitmap bitmap1 = new Bitmap(rectangle1.Width, rectangle1.Height);
				using (Graphics graphics2 = Graphics.FromImage(bitmap1))
				{
					this.OnPaint(new PaintEventArgs(graphics2, rectangle1));
				}
				graphics1.DrawImageUnscaled(bitmap1, 0, 0);
			}
			User32.EndPaint(m.HWnd, ref paintstruct1);
		}

		protected override void WndProc(ref Message m)
		{
			int num1 = m.Msg;
			if (num1 <= 0x1c) // WM_PAINT
			{
				if (num1 == 15)
				{
					this.PerformWmPaint(ref m);
					return;
				}
			}
			else
			{
				switch (num1)
				{
					case 0x20: // WM_SETCURSOR
					{
						this.PerformWmSetCursor(ref m);
						return;
					}
					case 0x21: // WM_MOUSEACTIVATE
					{
						this.PerformWmMouseActivate(ref m);
						return;
					}
					case 0x84: // WM_NCHITTEST
					{
						if (!this.PerformWmNcHitTest(ref m))
						{
							base.WndProc(ref m);
						}
						return;
					}
					case 0x200: // WM_MOUSEMOVE
                        if (m.WParam.ToInt32() != 0x0001)
                        {
                            break;
                        }

						if (!this.isMouseIn)
						{
							this.OnMouseEnter();
							this.isMouseIn = true;
						}
						Point p6 = new Point(m.LParam.ToInt32());
						this.OnMouseMove(new MouseEventArgs(Control.MouseButtons, 1, p6.X, p6.X, 0));
						if (this.onMouseMove)
						{
							this.PerformWmMouseMove(ref m);
							this.onMouseMove = false;
						}
						break;
					case 0x201: // WM_MOUSEDOWN
					{
						POINT point1;
						this.lastMouseDown = new Point(m.LParam.ToInt32());
						point1 = new POINT();
						point1.x = this.lastMouseDown.X;
						point1.y = this.lastMouseDown.Y;
						point1 = this.MousePositionToScreen(point1);
						deltaX = point1.x - this.Location.X;
						deltaY = point1.y - this.Location.Y;
						this.OnMouseDown(new MouseEventArgs(Control.MouseButtons, 1, lastMouseDown.X, lastMouseDown.Y, 0));
						if (this.onMouseDown)
						{
							this.PerformWmMouseDown(ref m);
							this.onMouseDown = false;
						}
						if (resizing)
						{
							deltaX = this.Bounds.Right - point1.x;
							deltaY = this.Bounds.Bottom - point1.y;
						}

						return;
					}
					case 0x202: // WM_LBUTTONUP
					{
						Point p = new Point(m.LParam.ToInt32());
						this.OnMouseUp(new MouseEventArgs(Control.MouseButtons, 1, p.X, p.Y, 0));
						if (this.onMouseUp)
						{
							this.PerformWmMouseUp(ref m);
							this.onMouseUp = false;
						}
						return;
					}
					case 0x02A3: // WM_MOUSELEAVE
					{
						if (this.isMouseIn)
						{
							this.OnMouseLeave();
							this.isMouseIn = false;
						}
						break;
					}
				}
			}
			base.WndProc(ref m);
		}


		#endregion

		#region == Event Methods ==

		protected virtual void OnLocationChanged(EventArgs e)
		{
			this.OnMove(EventArgs.Empty);
			if (this.LocationChanged!=null)
			{
				this.LocationChanged(this, e);
			}
		}
		protected virtual void OnSizeChanged(EventArgs e)
		{
			this.OnResize(EventArgs.Empty);
			if (this.SizeChanged!=null)
			{
				this.SizeChanged(this, e);
			}
		}
		protected virtual void OnMove(EventArgs e)
		{
			if (this.Move!=null)
			{
				this.Move(this, e);
			}
		}
		protected virtual void OnResize(EventArgs e)
		{
			if (this.Resize!=null)
			{
				this.Resize(this, e);
			}
		}


		#endregion

		#region == Size and Location ==

		protected virtual void SetBoundsCore(int x, int y, int width, int height)
		{
			if (width < (11+11+4+4)) width = 11+11+4+4;
			if (height < captionHeight*2 && !minimizing) height = captionHeight*2;

			if (((this.X != x) || (this.Y != y)) || ((this.Width != width) || (this.Height != height)))
			{
				if (base.Handle != IntPtr.Zero)
				{
					int num1 = 20;
					if ((this.X == x) && (this.Y == y))
					{
						num1 |= 2;
					}
					if ((this.Width == width) && (this.Height == height))
					{
						num1 |= 1;
					}
					User32.SetWindowPos(base.Handle, IntPtr.Zero, x, y, width, height, (uint)num1);
				}
				else
				{
					this.UpdateBounds(x, y, width, height);
				}
			}
		}
		private void UpdateBounds()
		{
			RECT rect1;
			rect1 = new RECT();
			User32.GetClientRect(base.Handle, ref rect1);
			int num1 = rect1.right;
			int num2 = rect1.bottom;
			User32.GetWindowRect(base.Handle, ref rect1);
			if (User32.GetParent(base.Handle)!=IntPtr.Zero)
			{
				User32.MapWindowPoints(IntPtr.Zero, User32.GetParent(base.Handle), ref rect1, 2);
			}
			this.UpdateBounds(rect1.left, rect1.top, rect1.right - rect1.left, rect1.bottom - rect1.top, num1, num2);
		}
		private void UpdateBounds(int x, int y, int width, int height)
		{
			RECT rect1;
			int num3;
			rect1 = new RECT();
			rect1.bottom = num3 = 0;
			rect1.top = num3;
			rect1.right = num3;
			rect1.left = num3;
			CreateParams params1 = this.CreateParams;
			User32.AdjustWindowRectEx(ref rect1, params1.Style, false, params1.ExStyle);
			int num1 = width - (rect1.right - rect1.left);
			int num2 = height - (rect1.bottom - rect1.top);
			this.UpdateBounds(x, y, width, height, num1, num2);
		}
		private void UpdateBounds(int x, int y, int width, int height, int clientWidth, int clientHeight)
		{
			bool flag1 = (this.X != x) || (this.Y != y);
			bool flag2 = (((this.Width != width) || (this.Height != height)) || (this.clientWidth != clientWidth)) || (this.clientHeight != clientHeight);
			this.size = new Size(width, height);
			this.location = new Point(x, y);
			this.clientWidth = clientWidth;
			this.clientHeight = clientHeight;
			if (flag1)
			{
				this.OnLocationChanged(EventArgs.Empty);
			}
			if (flag2)
			{
				this.OnSizeChanged(EventArgs.Empty);
			}
		}
 

		#endregion

		#region == Various ==

		public void RecreateHandle()
		{
			CreateParams params1 = this.CreateParams;
			this.CreateHandle(params1);
			this.Invalidate();
		}
 
		private void value_HandleDestroyed(object sender, EventArgs e)
		{
			this.parent.HandleDestroyed-=new EventHandler(value_HandleDestroyed);
			this.Hide();
		}

		public void Destroy()
		{
			this.Hide();
			this.Dispose();
		}

		
		#endregion

		#endregion

		#region #  Properties  #

		public bool Minimized
		{
			get {return this.minimized;}
			set
			{
				if (value)
					this.Minimize();
				else
					this.Maximize();
			}
		}
		protected virtual CreateParams CreateParams
		{
			get
			{
				CreateParams params1 = new CreateParams();
				params1.Caption = "FloatingNativeWindow";
				int nX = location.X;
				int nY = location.Y;
				Screen screen1 = Screen.FromHandle(base.Handle);
				if ((nX + this.size.Width) > screen1.Bounds.Width)
				{
					nX = screen1.Bounds.Width - this.size.Width;
				}
				if ((nY + this.size.Height) > screen1.Bounds.Height)
				{
					nY = screen1.Bounds.Height - this.size.Height;
				}
				this.location = new Point(nX, nY);
				Size size1 = this.size;
				Point point1 = this.location;
				params1.X = nX;
				params1.Y = nY;
				params1.Height = size1.Height;
				params1.Width = size1.Width;
				params1.Parent = IntPtr.Zero;
				params1.Style = -2147483648;
				params1.ExStyle = 0x88;
				if (this.supportsLayered)
				{
					params1.ExStyle += 0x80000;
				}
				this.size = size1;
				this.location = point1;
				return params1;
			}
		}
		public Control Parent
		{
			get {return this.parent;}
			set
			{
				if (value == this.parent) return;
				if (this.parent!=null)
				{
					value.HandleDestroyed-=new EventHandler(value_HandleDestroyed);
				}
				this.parent = value;
				if (value != null)
				{
					value.HandleDestroyed+=new EventHandler(value_HandleDestroyed);
				}
			}
		}

		public virtual Size Size
		{
			get {return this.size;}
			set
			{
				if (base.Handle!=IntPtr.Zero)
				{
					this.SetBoundsCore(this.location.X, this.location.Y, value.Width, value.Height);
					RECT rect = new RECT();
					User32.GetWindowRect(base.Handle, ref rect);
					this.size = new Size(rect.right-rect.left, rect.bottom-rect.top);
					this.UpdateLayeredWindow();
				}
				else
				{
					this.size = value;
				}
			}
		}
		public virtual Point Location
		{
			get {return this.location;}
			set
			{
				if (base.Handle!=IntPtr.Zero)
				{
					this.SetBoundsCore(value.X, value.Y, this.size.Width, this.size.Height);
					RECT rect = new RECT();
					User32.GetWindowRect(base.Handle, ref rect);
					this.location = new Point(rect.left, rect.top);
					this.UpdateLayeredWindow();
				}
				else
				{
					this.location = value;
				}
			}
		}
		public int Height
		{
			get {return this.size.Height;}
			set
			{
				this.Size = new Size(this.size.Width, value);
			}
		}
		public int Width
		{
			get {return this.size.Width;}
			set
			{
				this.Size = new Size(value, this.size.Height);
			}
		}
		public int X
		{
			get {return this.location.X;}
			set
			{
				this.Location = new Point(value, this.location.Y);
			}
		}
		public int Y
		{
			get {return this.location.Y;}
			set
			{
				this.Location = new Point(this.location.X, value);
			}
		}
		public Rectangle ClientRectangle
		{
			get
			{
				RECT rct = new RECT();
				POINT pnt = new POINT();
				User32.GetWindowRect(base.Handle, ref rct);
				pnt.x = rct.left;
				pnt.y = rct.top;
				User32.ScreenToClient(base.Handle, ref pnt);
				Rectangle rect = new Rectangle(pnt.x, pnt.y, rct.right-rct.left, rct.bottom-rct.top);
				if (this.hasShadow)
				{
					rect.Width -= this.shadowLength;
					rect.Height -= this.shadowLength;
				}
				return rect;
			}
		}
		protected Rectangle ShadowRectangle
		{
			get 
			{
				Rectangle rect = this.ClientRectangle;
				rect.X += this.shadowLength;
				rect.Y += this.shadowLength;
				return rect;
			}
		}
		public Rectangle Bounds
		{
			get
			{
				return new Rectangle(this.location, this.size);
			}
			set
			{
				this.location = value.Location;
				this.size = value.Size;
				this.SetBoundsCore(this.location.X, this.location.Y, this.size.Width, this.size.Height);
			}
		}
		public bool HasShadow
		{
			get {return this.hasShadow;}
			set
			{
				this.hasShadow = value;
				this.ComputeLayout();
				this.Invalidate();
			}
		}
		public string Text
		{
			get {return this.text;}
			set
			{
				this.text = value;
				this.ComputeLayout();
				this.Invalidate();
			}
		}
		public int CaptionHeight
		{
			get {return this.captionHeight;}
			set
			{
				this.captionHeight = value;
				this.ComputeLayout();
				this.Invalidate();
			}
		}

        public Rectangle MoveRect
        {
            get { return this.moveRect; }
            set
            {
                this.moveRect = value;
            }
        }

        public Rectangle ContextableRect
        {
            get { return this.contextableRect; }
            set
            {
                this.contextableRect = value;
            }
        }

        public bool Visible
        {
            get
            {
                return visible;
            }
        }

        public Rectangle ContextMenuRect
        {
            get
            {
                return this.contextMenuRect;
            }

            set
            {
                this.contextMenuRect = value;
            }
        }

        public bool ContextMenuVisible
        {
            get
            {
                return this.contextMenuVisible;
            }

            set
            {
                this.contextMenuVisible = value;
            }
        }


		public int Alpha
		{
			get {return this.alpha;}
			set
			{
				if (this.alpha == value) return;
				if (value < 0 || value > 255)
				{
					throw new ArgumentException("Alpha must be between 0 and 255");
				}
				this.alpha = value;
				this.UpdateLayeredWindow((byte)this.alpha);
			}
		}

		#endregion

		#region #  Events  #

		public event PaintEventHandler Paint;
		public event EventHandler SizeChanged;
		public event EventHandler LocationChanged;
		public event EventHandler Move;
		public event EventHandler Resize;
		public event MouseEventHandler MouseDown;
		public event MouseEventHandler MouseUp;
		public event MouseEventHandler MouseMove;
		public event EventHandler MouseEnter;
		public event EventHandler MouseLeave;

		#endregion

		#region IDisposable Members

		public void Dispose()
		{
			this.Dispose(true);
			GC.SuppressFinalize(this);
		}
		private void Dispose(bool disposing)
		{
			if (!this.disposed)
			{
				if (disposing)
				{
					if (this.parent!=null)
					{
						this.parent.HandleDestroyed-=new EventHandler(value_HandleDestroyed);
					}
				}
				this.DestroyHandle();
				this.disposed = true;
			}
		}

		#endregion
	}

	#region #  Win32  #

	internal struct PAINTSTRUCT
	{
		public IntPtr hdc;
		public int fErase;
		public Rectangle rcPaint;
		public int fRestore;
		public int fIncUpdate;
		public int Reserved1;
		public int Reserved2;
		public int Reserved3;
		public int Reserved4;
		public int Reserved5;
		public int Reserved6;
		public int Reserved7;
		public int Reserved8;
	}
	[StructLayout(LayoutKind.Sequential)]
	internal struct POINT
	{
		public int x;
		public int y;
	}
	[StructLayout(LayoutKind.Sequential)]
	internal struct RECT
	{
		public int left;
		public int top;
		public int right;
		public int bottom;
	}
	[StructLayout(LayoutKind.Sequential)]
	internal struct SIZE
	{
		public int cx;
		public int cy;
	}
	[StructLayout(LayoutKind.Sequential)][CLSCompliant(false)]
	internal struct TRACKMOUSEEVENTS
	{
		public uint cbSize;
		public uint dwFlags;
		public IntPtr hWnd;
		public uint dwHoverTime;
	}
	[StructLayout(LayoutKind.Sequential)]
	internal struct MSG
	{
		public IntPtr hwnd;
		public int message;
		public IntPtr wParam;
		public IntPtr lParam;
		public int time;
		public int pt_x;
		public int pt_y;
	}
	[StructLayout(LayoutKind.Sequential, Pack=1)]
	internal struct BLENDFUNCTION
	{
		public byte BlendOp;
		public byte BlendFlags;
		public byte SourceConstantAlpha;
		public byte AlphaFormat;
	}

	internal class AnimateWindow
	{
		private AnimateWindow() 
		{
		}
		public static int AW_HOR_POSITIVE = 0x1;
		public static int AW_HOR_NEGATIVE = 0x2;
		public static int AW_VER_POSITIVE = 0x4;
		public static int AW_VER_NEGATIVE = 0x8;
		public static int AW_CENTER = 0x10;
		public static int AW_HIDE = 0x10000;
		public static int AW_ACTIVATE = 0x20000;
		public static int AW_SLIDE = 0x40000;
		public static int AW_BLEND = 0x80000;
	}

	public enum AnimateMode
	{
		SlideRightToLeft,
		SlideLeftToRight,
		SlideTopToBottom,
		SlideBottmToTop,
		RollRightToLeft,
		RollLeftToRight,
		RollTopToBottom,
		RollBottmToTop,
		Blend,
		ExpandCollapse
	}

	internal class User32
	{
		// Methods
		private User32()
		{
		}
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool AnimateWindow(IntPtr hWnd, uint dwTime, uint dwFlags);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern IntPtr BeginPaint(IntPtr hWnd, ref PAINTSTRUCT ps);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool ClientToScreen(IntPtr hWnd, ref POINT pt);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool DispatchMessage(ref MSG msg);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool DrawFocusRect(IntPtr hWnd, ref RECT rect);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool EndPaint(IntPtr hWnd, ref PAINTSTRUCT ps);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern IntPtr GetDC(IntPtr hWnd);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern IntPtr GetFocus();
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern ushort GetKeyState(int virtKey);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool GetMessage(ref MSG msg, int hWnd, uint wFilterMin, uint wFilterMax);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern IntPtr GetParent(IntPtr hWnd);
		[DllImport("user32.dll", CharSet=CharSet.Auto, ExactSpelling=true)]
		public static extern bool GetClientRect(IntPtr hWnd, [In, Out] ref RECT rect);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern int GetWindowLong(IntPtr hWnd, int nIndex);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern IntPtr GetWindow(IntPtr hWnd, int cmd);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool GetWindowRect(IntPtr hWnd, ref RECT rect);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool HideCaret(IntPtr hWnd);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool InvalidateRect(IntPtr hWnd, ref RECT rect, bool erase);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern IntPtr LoadCursor(IntPtr hInstance, uint cursor);
		[DllImport("user32.dll", CharSet=CharSet.Auto, ExactSpelling=true)]
		public static extern int MapWindowPoints(IntPtr hWndFrom, IntPtr hWndTo, [In, Out] ref RECT rect, int cPoints);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool MoveWindow(IntPtr hWnd, int x, int y, int width, int height, bool repaint);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool PeekMessage(ref MSG msg, int hWnd, uint wFilterMin, uint wFilterMax, uint wFlag);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool PostMessage(IntPtr hWnd, int Msg, uint wParam, uint lParam);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool ReleaseCapture();
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern int ReleaseDC(IntPtr hWnd, IntPtr hDC);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool ScreenToClient(IntPtr hWnd, ref POINT pt);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern uint SendMessage(IntPtr hWnd, int Msg, uint wParam, uint lParam);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern IntPtr SetCursor(IntPtr hCursor);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern IntPtr SetFocus(IntPtr hWnd);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern int SetWindowLong(IntPtr hWnd, int nIndex, int newLong);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern int SetWindowPos(IntPtr hWnd, IntPtr hWndAfter, int X, int Y, int Width, int Height, uint flags);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool SetWindowRgn(IntPtr hWnd, IntPtr hRgn, bool redraw);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool ShowCaret(IntPtr hWnd);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool SetCapture(IntPtr hWnd);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern int ShowWindow(IntPtr hWnd, short cmdShow);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool SystemParametersInfo(uint uiAction, uint uiParam, ref int bRetValue, uint fWinINI);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool TrackMouseEvent(ref TRACKMOUSEEVENTS tme);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool TranslateMessage(ref MSG msg);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool UpdateLayeredWindow(IntPtr hwnd, IntPtr hdcDst, ref POINT pptDst, ref SIZE psize, IntPtr hdcSrc, ref POINT pprSrc, int crKey, ref BLENDFUNCTION pblend, int dwFlags);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool UpdateWindow(IntPtr hwnd);
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		internal static extern bool WaitMessage();
		[DllImport("user32.dll", CharSet=CharSet.Auto, ExactSpelling=true)]
		public static extern bool AdjustWindowRectEx(ref RECT lpRect, int dwStyle, bool bMenu, int dwExStyle);
	}

	internal class Gdi32
	{
		// Methods
		private Gdi32()
		{
		}
		[DllImport("gdi32.dll", CharSet=CharSet.Auto)]
		internal static extern int CombineRgn(IntPtr dest, IntPtr src1, IntPtr src2, int flags);
		[DllImport("gdi32.dll", CharSet=CharSet.Auto)]
		internal static extern IntPtr CreateBrushIndirect(ref LOGBRUSH brush);
		[DllImport("gdi32.dll", CharSet=CharSet.Auto)]
		internal static extern IntPtr CreateCompatibleDC(IntPtr hDC);
		[DllImport("gdi32.dll", CharSet=CharSet.Auto)]
		internal static extern IntPtr CreateRectRgnIndirect(ref RECT rect);
		[DllImport("gdi32.dll", CharSet=CharSet.Auto)]
		internal static extern bool DeleteDC(IntPtr hDC);
		[DllImport("gdi32.dll", CharSet=CharSet.Auto)]
		internal static extern IntPtr DeleteObject(IntPtr hObject);
		[DllImport("gdi32.dll", CharSet=CharSet.Auto)]
		internal static extern int GetClipBox(IntPtr hDC, ref RECT rectBox);
		[DllImport("gdi32.dll", CharSet=CharSet.Auto)]
		internal static extern bool PatBlt(IntPtr hDC, int x, int y, int width, int height, uint flags);
		[DllImport("gdi32.dll", CharSet=CharSet.Auto)]
		internal static extern int SelectClipRgn(IntPtr hDC, IntPtr hRgn);
		[DllImport("gdi32.dll", CharSet=CharSet.Auto)]
		internal static extern IntPtr SelectObject(IntPtr hDC, IntPtr hObject);
	}

    internal class Kernel32
    {
        [DllImport("kernel32")]
        internal static extern long WritePrivateProfileString(string section, string key, string val, string filePath);

        // 声明INI文件的读操作函数 GetPrivateProfileString()
        [DllImport("kernel32")]
        internal static extern int GetPrivateProfileString(string section, string key, string def, System.Text.StringBuilder retVal, int size, string filePath);

        [DllImport("kernel32")]
        internal static extern long GetLastError();
    }

	[StructLayout(LayoutKind.Sequential)][CLSCompliant(false)]
	public struct LOGBRUSH
	{
		public uint lbStyle;
		public uint lbColor;
		public uint lbHatch;
	}

	#endregion
}
