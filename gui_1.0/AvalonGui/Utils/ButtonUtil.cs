using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using System.Linq;
using System.Text;

namespace AvalonGui.Utils
{
    public class ButtonUtil
    {
        public static void SetButtonImages(Button btn, Bitmap pushImage, Bitmap normalImage)
        {
            btn.FlatStyle = FlatStyle.Flat;//样式
            btn.ForeColor = Color.Transparent;//前景
            btn.BackColor = Color.Transparent;//去背景
            btn.FlatAppearance.BorderSize = 0;//去边线
            btn.FlatAppearance.MouseOverBackColor = Color.Transparent;//鼠标经过
            btn.FlatAppearance.MouseDownBackColor = Color.Transparent;//鼠标按下
            btn.BackgroundImage = normalImage;

            btn.MouseDown += new System.Windows.Forms.MouseEventHandler(
                (sender, e) => {
                    Button bn = sender as Button;
                    if (bn != null)
                    {
                        bn.BackgroundImage = pushImage;
                    }
                }
            );

            btn.MouseUp += new MouseEventHandler(
                (sender, e) => {
                    btn.BackgroundImage = normalImage;
                }
            );

            btn.MouseHover += new EventHandler(
                (sender, e) => {
                    btn.FlatAppearance.BorderSize = 1;
                }
            );

            btn.MouseLeave += new EventHandler(
                (sender, e) => {
                    btn.FlatAppearance.BorderSize = 0;
                }
            );

        }
    }
}
