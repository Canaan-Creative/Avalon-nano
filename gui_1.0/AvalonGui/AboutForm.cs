using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

using AvalonGui.Properties;

namespace AvalonGui
{
    public partial class AboutForm : Form
    {
        private static Rectangle CloseRect = new Rectangle(468, 20, 12, 12);

        public AboutForm()
        {
            InitializeComponent();
        }

        private void AboutForm_Load(object sender, EventArgs e)
        {

        }

        protected override void WndProc(ref Message msg)
        {
            const int WM_SYSCOMMAND = 0x0112;
            const int SC_CLOSE = 0xF060;

            if (msg.Msg == WM_SYSCOMMAND && ((int)msg.WParam == SC_CLOSE))
            {
                // 点击winform右上关闭按钮
                // 加入想要的逻辑处理
                ShowAbout(false);

                return;
            }

            base.WndProc(ref msg);
        }

        private void ShowAbout(bool show)
        {
            if (show)
            {
                Show();
                WindowState = FormWindowState.Normal;
            }
            else
            {
                Hide();
                WindowState = FormWindowState.Minimized;
            }
        }

        private void AboutForm_MouseDown(object sender, MouseEventArgs e)
        {
            if (CloseRect.Contains(e.X, e.Y))
            {
                Hide();
            }
        }

        private void AboutForm_MouseUp(object sender, MouseEventArgs e)
        {

        }

        private void AboutForm_MouseMove(object sender, MouseEventArgs e)
        {

        }

        private void AboutForm_Paint(object sender, PaintEventArgs e)
        {
            Graphics g = e.Graphics;

            g.DrawImage(Resources.about_close, CloseRect);
        }

        private void linkLabel1_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            OpenLink(sender);
        }

        private void linkLabel2_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            OpenLink(sender);
        }

        private void linkLabel3_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            OpenLink(sender);
        }

        private void OpenLink(object linker)
        {
            LinkLabel link = linker as LinkLabel;

            if (link != null)
            {
                System.Diagnostics.Process.Start(link.Text);
            }
        }
    }
}
