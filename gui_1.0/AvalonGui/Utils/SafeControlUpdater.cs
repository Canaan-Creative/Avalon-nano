using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using System.Linq;
using System.Text;

namespace AvalonGui.Utils
{
    public class SafeControlUpdater
    {
        public static void SetTextBoxText(TextBox textBox, string text)
        {
            if (textBox.InvokeRequired)
            {
                textBox.BeginInvoke(new Action(() =>
                {
                    textBox.Text = text;
                }));
            }
            else
            {
                textBox.Text = text;
            }
        }

        public static void SetLabelText(Label label, string text)
        {
            if (label.InvokeRequired)
            {
                label.BeginInvoke(new Action(() =>
                {
                    label.Text = text;
                }));
            }
            else
            {
                label.Text = text;
            }
        }

        public static void ShowForm(Form form, bool show)
        {
            if (form.InvokeRequired)
            {
                form.BeginInvoke(new Action(() =>
                {
                    if (show)
                    {
                        form.Show();
                        form.BringToFront();
                        form.WindowState = FormWindowState.Normal;
                    }
                    else
                    {
                        form.Hide();
                        form.WindowState = FormWindowState.Minimized;
                    }
                }));
            }
            else
            {
                if (show)
                {
                    form.Show();
                    form.BringToFront();
                    form.WindowState = FormWindowState.Normal;
                }
                else
                {
                    form.Hide();
                    form.WindowState = FormWindowState.Minimized;
                }
            }
        }

        public static void SetVisible(Control control, bool visible)
        {
            if (control.InvokeRequired)
            {
                control.BeginInvoke(new Action(() =>
                {
                    control.Visible = visible;
                }));
            }
            else
            {
                control.Visible = visible;
            }
        }

        public static void SetbackgroundImage(Control control, Image image)
        {
            if (control.InvokeRequired)
            {
                control.BeginInvoke(new Action(() =>
                {
                    control.BackgroundImage = image;
                }));
            }
            else
            {
                control.BackgroundImage = image;
            }
        }

        public static void Invalidate(Control control)
        {
            if (control.InvokeRequired)
            {
                control.BeginInvoke(new Action(() =>
                {
                    control.Invalidate();
                }));
            }
            else
            {
                control.Invalidate();
            }
        }
    }
}
