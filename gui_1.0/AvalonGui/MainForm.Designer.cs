namespace AvalonGui
{
    partial class MainForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
            this.notifyIcon1 = new System.Windows.Forms.NotifyIcon(this.components);
            this.contextMenuStrip1 = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.dIsplayMainUIToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.toolStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.safeRemoveUSBsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this.toolStripMenuItemAbout = new System.Windows.Forms.ToolStripMenuItem();
            this.pictureBoxPoolStatus2 = new System.Windows.Forms.PictureBox();
            this.pictureBoxPoolStatus1 = new System.Windows.Forms.PictureBox();
            this.pictureBoxPoolStatus3 = new System.Windows.Forms.PictureBox();
            this.pictureBoxAd = new System.Windows.Forms.PictureBox();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.buttonQuitApp = new System.Windows.Forms.Button();
            this.buttonSettings = new System.Windows.Forms.Button();
            this.pictureBoxGraph = new System.Windows.Forms.PictureBox();
            this.contextMenuStrip1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxPoolStatus2)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxPoolStatus1)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxPoolStatus3)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxAd)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxGraph)).BeginInit();
            this.SuspendLayout();
            // 
            // notifyIcon1
            // 
            this.notifyIcon1.ContextMenuStrip = this.contextMenuStrip1;
            this.notifyIcon1.Icon = ((System.Drawing.Icon)(resources.GetObject("notifyIcon1.Icon")));
            this.notifyIcon1.Text = "Avalon Nano";
            this.notifyIcon1.Visible = true;
            this.notifyIcon1.DoubleClick += new System.EventHandler(this.notifyIcon1_DoubleClick);
            // 
            // contextMenuStrip1
            // 
            this.contextMenuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.dIsplayMainUIToolStripMenuItem,
            this.toolStripSeparator1,
            this.toolStripMenuItem1,
            this.toolStripSeparator2,
            this.safeRemoveUSBsToolStripMenuItem,
            this.toolStripSeparator3,
            this.toolStripMenuItemAbout});
            this.contextMenuStrip1.Name = "contextMenuStrip1";
            this.contextMenuStrip1.ShowImageMargin = false;
            this.contextMenuStrip1.ShowItemToolTips = false;
            this.contextMenuStrip1.Size = new System.Drawing.Size(124, 110);
            this.contextMenuStrip1.Opening += new System.ComponentModel.CancelEventHandler(this.contextMenuStrip1_Opening);
            // 
            // dIsplayMainUIToolStripMenuItem
            // 
            this.dIsplayMainUIToolStripMenuItem.Name = "dIsplayMainUIToolStripMenuItem";
            this.dIsplayMainUIToolStripMenuItem.Size = new System.Drawing.Size(123, 22);
            this.dIsplayMainUIToolStripMenuItem.Text = "打开主页";
            this.dIsplayMainUIToolStripMenuItem.Click += new System.EventHandler(this.dIsplayMainUIToolStripMenuItem_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(120, 6);
            // 
            // toolStripMenuItem1
            // 
            this.toolStripMenuItem1.Name = "toolStripMenuItem1";
            this.toolStripMenuItem1.Size = new System.Drawing.Size(123, 22);
            this.toolStripMenuItem1.Text = "隐藏悬浮窗";
            this.toolStripMenuItem1.Click += new System.EventHandler(this.toolStripMenuItem1_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(120, 6);
            // 
            // safeRemoveUSBsToolStripMenuItem
            // 
            this.safeRemoveUSBsToolStripMenuItem.Name = "safeRemoveUSBsToolStripMenuItem";
            this.safeRemoveUSBsToolStripMenuItem.Size = new System.Drawing.Size(123, 22);
            this.safeRemoveUSBsToolStripMenuItem.Text = "安全移除设备";
            this.safeRemoveUSBsToolStripMenuItem.Click += new System.EventHandler(this.safeRemoveUSBsToolStripMenuItem_Click);
            // 
            // toolStripSeparator3
            // 
            this.toolStripSeparator3.Name = "toolStripSeparator3";
            this.toolStripSeparator3.Size = new System.Drawing.Size(120, 6);
            // 
            // toolStripMenuItemAbout
            // 
            this.toolStripMenuItemAbout.Name = "toolStripMenuItemAbout";
            this.toolStripMenuItemAbout.Size = new System.Drawing.Size(123, 22);
            this.toolStripMenuItemAbout.Text = "关于";
            this.toolStripMenuItemAbout.Click += new System.EventHandler(this.toolStripMenuItemAbout_Click);
            // 
            // pictureBoxPoolStatus2
            // 
            this.pictureBoxPoolStatus2.BackgroundImage = global::AvalonGui.Properties.Resources.offline;
            this.pictureBoxPoolStatus2.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Zoom;
            this.pictureBoxPoolStatus2.Location = new System.Drawing.Point(389, 364);
            this.pictureBoxPoolStatus2.Name = "pictureBoxPoolStatus2";
            this.pictureBoxPoolStatus2.Size = new System.Drawing.Size(22, 20);
            this.pictureBoxPoolStatus2.TabIndex = 28;
            this.pictureBoxPoolStatus2.TabStop = false;
            this.pictureBoxPoolStatus2.Visible = false;
            // 
            // pictureBoxPoolStatus1
            // 
            this.pictureBoxPoolStatus1.BackgroundImage = global::AvalonGui.Properties.Resources.offline;
            this.pictureBoxPoolStatus1.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Zoom;
            this.pictureBoxPoolStatus1.Location = new System.Drawing.Point(389, 314);
            this.pictureBoxPoolStatus1.Name = "pictureBoxPoolStatus1";
            this.pictureBoxPoolStatus1.Size = new System.Drawing.Size(22, 20);
            this.pictureBoxPoolStatus1.TabIndex = 27;
            this.pictureBoxPoolStatus1.TabStop = false;
            this.pictureBoxPoolStatus1.Visible = false;
            // 
            // pictureBoxPoolStatus3
            // 
            this.pictureBoxPoolStatus3.BackgroundImage = global::AvalonGui.Properties.Resources.offline;
            this.pictureBoxPoolStatus3.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Zoom;
            this.pictureBoxPoolStatus3.Location = new System.Drawing.Point(389, 413);
            this.pictureBoxPoolStatus3.Name = "pictureBoxPoolStatus3";
            this.pictureBoxPoolStatus3.Size = new System.Drawing.Size(22, 20);
            this.pictureBoxPoolStatus3.TabIndex = 26;
            this.pictureBoxPoolStatus3.TabStop = false;
            this.pictureBoxPoolStatus3.Visible = false;
            // 
            // pictureBoxAd
            // 
            this.pictureBoxAd.Image = global::AvalonGui.Properties.Resources.ad;
            this.pictureBoxAd.Location = new System.Drawing.Point(19, 482);
            this.pictureBoxAd.Name = "pictureBoxAd";
            this.pictureBoxAd.Size = new System.Drawing.Size(460, 114);
            this.pictureBoxAd.TabIndex = 13;
            this.pictureBoxAd.TabStop = false;
            // 
            // pictureBox1
            // 
            this.pictureBox1.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.pictureBox1.Image = global::AvalonGui.Properties.Resources.table;
            this.pictureBox1.Location = new System.Drawing.Point(0, 257);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(501, 190);
            this.pictureBox1.TabIndex = 12;
            this.pictureBox1.TabStop = false;
            this.pictureBox1.Paint += new System.Windows.Forms.PaintEventHandler(this.pictureBox1_Paint);
            // 
            // buttonQuitApp
            // 
            this.buttonQuitApp.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center;
            this.buttonQuitApp.Image = global::AvalonGui.Properties.Resources.close;
            this.buttonQuitApp.Location = new System.Drawing.Point(474, 9);
            this.buttonQuitApp.Name = "buttonQuitApp";
            this.buttonQuitApp.Size = new System.Drawing.Size(13, 12);
            this.buttonQuitApp.TabIndex = 11;
            this.buttonQuitApp.UseVisualStyleBackColor = true;
            this.buttonQuitApp.Click += new System.EventHandler(this.buttonQuitApp_Click);
            // 
            // buttonSettings
            // 
            this.buttonSettings.BackgroundImage = global::AvalonGui.Properties.Resources.btn_set_normal;
            this.buttonSettings.Location = new System.Drawing.Point(412, 448);
            this.buttonSettings.Name = "buttonSettings";
            this.buttonSettings.Size = new System.Drawing.Size(68, 24);
            this.buttonSettings.TabIndex = 4;
            this.buttonSettings.UseVisualStyleBackColor = true;
            this.buttonSettings.Click += new System.EventHandler(this.buttonSettings_Click);
            // 
            // pictureBoxGraph
            // 
            this.pictureBoxGraph.Image = global::AvalonGui.Properties.Resources.graph;
            this.pictureBoxGraph.Location = new System.Drawing.Point(0, 1);
            this.pictureBoxGraph.Name = "pictureBoxGraph";
            this.pictureBoxGraph.Size = new System.Drawing.Size(501, 261);
            this.pictureBoxGraph.TabIndex = 3;
            this.pictureBoxGraph.TabStop = false;
            this.pictureBoxGraph.Paint += new System.Windows.Forms.PaintEventHandler(this.pictureBoxGraph_Paint);
            this.pictureBoxGraph.MouseDown += new System.Windows.Forms.MouseEventHandler(this.pictureBoxGraph_MouseDown);
            this.pictureBoxGraph.MouseMove += new System.Windows.Forms.MouseEventHandler(this.pictureBoxGraph_MouseMove);
            this.pictureBoxGraph.MouseUp += new System.Windows.Forms.MouseEventHandler(this.pictureBoxGraph_MouseUp);
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(500, 600);
            this.ContextMenuStrip = this.contextMenuStrip1;
            this.Controls.Add(this.pictureBoxPoolStatus2);
            this.Controls.Add(this.pictureBoxPoolStatus1);
            this.Controls.Add(this.pictureBoxPoolStatus3);
            this.Controls.Add(this.pictureBoxAd);
            this.Controls.Add(this.buttonQuitApp);
            this.Controls.Add(this.buttonSettings);
            this.Controls.Add(this.pictureBoxGraph);
            this.Controls.Add(this.pictureBox1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.None;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "MainForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Avalon Nano";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Form1_FormClosing);
            this.Load += new System.EventHandler(this.Form1_Load);
            this.Resize += new System.EventHandler(this.Form1_Resize);
            this.contextMenuStrip1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxPoolStatus2)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxPoolStatus1)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxPoolStatus3)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxAd)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBoxGraph)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.NotifyIcon notifyIcon1;
        private System.Windows.Forms.ContextMenuStrip contextMenuStrip1;
        private System.Windows.Forms.ToolStripMenuItem dIsplayMainUIToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem safeRemoveUSBsToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem toolStripMenuItem1;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.PictureBox pictureBoxGraph;
        private System.Windows.Forms.Button buttonSettings;
        private System.Windows.Forms.Button buttonQuitApp;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.PictureBox pictureBoxAd;
        private System.Windows.Forms.PictureBox pictureBoxPoolStatus3;
        private System.Windows.Forms.PictureBox pictureBoxPoolStatus1;
        private System.Windows.Forms.PictureBox pictureBoxPoolStatus2;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
        private System.Windows.Forms.ToolStripMenuItem toolStripMenuItemAbout;
    }
}

