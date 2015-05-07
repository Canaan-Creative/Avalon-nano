using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

using AvalonGui.Properties;
using AvalonGui.Utils;


namespace AvalonGui
{
    public partial class SettingsForm : Form
    {
        AvalonConfig _appConfig = null;

        public Rectangle ParentScreenRect
        {
            get;
            set;
        }

        public SettingsForm()
        {
            InitializeComponent();

            ButtonUtil.SetButtonImages(buttonSave, Resources.btn_save_push, Resources.btn_save_normal);
            buttonSave.BackgroundImage = Resources.btn_save_disable;
            ButtonUtil.SetButtonImages(buttonCancel, Resources.btn_cancel_push, Resources.btn_cancel_normal);
        }

        public void SetConfing(AvalonConfig config)
        {
            _appConfig = config;
        }

        private void buttonSave_Click(object sender, EventArgs e)
        {
            UI2Config();
            buttonSave.BackgroundImage = Resources.btn_save_disable;
            this.Close();
        }

        private void buttonCancel_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void SettingsForm_Load(object sender, EventArgs e)
        {
            int left = ParentScreenRect.Left + (ParentScreenRect.Width - this.Width) / 2;
            int top = ParentScreenRect.Top + 226;

            this.Location = new Point(left, top);

            if (_appConfig == null)
            {
                return;
            }

            Config2UI();

            buttonSave.Enabled = false;
            buttonSave.BackgroundImage = Resources.btn_save_disable;
        }

        private void UI2Config()
        {
            _appConfig.MinerInfo.PoolInfos[0].PoolURL = textBoxUrl1.Text;
            _appConfig.MinerInfo.PoolInfos[0].PoolPort = textBoxPort1.Text;
            _appConfig.MinerInfo.PoolInfos[0].WorkerName = textBoxWorker1.Text;
            _appConfig.MinerInfo.PoolInfos[0].WorkerPassword = textBoxPassword1.Text;

            _appConfig.MinerInfo.PoolInfos[1].PoolURL = textBoxUrl2.Text;
            _appConfig.MinerInfo.PoolInfos[1].PoolPort = textBoxPort2.Text;
            _appConfig.MinerInfo.PoolInfos[1].WorkerName = textBoxWorker2.Text;
            _appConfig.MinerInfo.PoolInfos[1].WorkerPassword = textBoxPassword2.Text;

            _appConfig.MinerInfo.PoolInfos[2].PoolURL = textBoxUrl3.Text;
            _appConfig.MinerInfo.PoolInfos[2].PoolPort = textBoxPort3.Text;
            _appConfig.MinerInfo.PoolInfos[2].WorkerName = textBoxWorker3.Text;
            _appConfig.MinerInfo.PoolInfos[2].WorkerPassword = textBoxPassword3.Text;
        }

        private void Config2UI()
        {
            textBoxUrl1.Text = _appConfig.MinerInfo.PoolInfos[0].PoolURL;
            textBoxPort1.Text = _appConfig.MinerInfo.PoolInfos[0].PoolPort;
            textBoxWorker1.Text = _appConfig.MinerInfo.PoolInfos[0].WorkerName;
            textBoxPassword1.Text = _appConfig.MinerInfo.PoolInfos[0].WorkerPassword;

            textBoxUrl2.Text = _appConfig.MinerInfo.PoolInfos[1].PoolURL;
            textBoxPort2.Text = _appConfig.MinerInfo.PoolInfos[1].PoolPort;
            textBoxWorker2.Text = _appConfig.MinerInfo.PoolInfos[1].WorkerName;
            textBoxPassword2.Text = _appConfig.MinerInfo.PoolInfos[1].WorkerPassword;

            textBoxUrl3.Text = _appConfig.MinerInfo.PoolInfos[2].PoolURL;
            textBoxPort3.Text = _appConfig.MinerInfo.PoolInfos[2].PoolPort;
            textBoxWorker3.Text = _appConfig.MinerInfo.PoolInfos[2].WorkerName;
            textBoxPassword3.Text = _appConfig.MinerInfo.PoolInfos[2].WorkerPassword;
        }

        private void textBoxUrl1_TextChanged(object sender, EventArgs e)
        {
            buttonSave.Enabled = true;
            buttonSave.BackgroundImage = Resources.btn_save_normal;
        }

        private void textBoxPort1_TextChanged(object sender, EventArgs e)
        {
            buttonSave.Enabled = true;
            buttonSave.BackgroundImage = Resources.btn_save_normal;
        }

        private void textBoxWorker1_TextChanged(object sender, EventArgs e)
        {
            buttonSave.Enabled = true;
            buttonSave.BackgroundImage = Resources.btn_save_normal;
        }

        private void textBoxPassword1_TextChanged(object sender, EventArgs e)
        {
            buttonSave.Enabled = true;
            buttonSave.BackgroundImage = Resources.btn_save_normal;
        }

        private void textBoxUrl2_TextChanged(object sender, EventArgs e)
        {
            buttonSave.Enabled = true;
            buttonSave.BackgroundImage = Resources.btn_save_normal;
        }

        private void textBoxPort2_TextChanged(object sender, EventArgs e)
        {
            buttonSave.Enabled = true;
            buttonSave.BackgroundImage = Resources.btn_save_normal;
        }

        private void textBoxWorker2_TextChanged(object sender, EventArgs e)
        {
            buttonSave.Enabled = true;
            buttonSave.BackgroundImage = Resources.btn_save_normal;
        }

        private void textBoxPassword2_TextChanged(object sender, EventArgs e)
        {
            buttonSave.Enabled = true;
            buttonSave.BackgroundImage = Resources.btn_save_normal;
        }

        private void textBoxUrl3_TextChanged(object sender, EventArgs e)
        {
            buttonSave.Enabled = true;
            buttonSave.BackgroundImage = Resources.btn_save_normal;
        }

        private void textBoxPort3_TextChanged(object sender, EventArgs e)
        {
            buttonSave.Enabled = true;
            buttonSave.BackgroundImage = Resources.btn_save_normal;
        }

        private void textBoxWorker3_TextChanged(object sender, EventArgs e)
        {
            buttonSave.Enabled = true;
            buttonSave.BackgroundImage = Resources.btn_save_normal;
        }

        private void textBoxPassword3_TextChanged(object sender, EventArgs e)
        {
            buttonSave.Enabled = true;
            buttonSave.BackgroundImage = Resources.btn_save_normal;
        }
    }
}
