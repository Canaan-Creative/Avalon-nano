using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Configuration.Install;
using System.Linq;


namespace Avalon.Service
{
    [RunInstaller(true)]
    public partial class SvcInstaller : System.Configuration.Install.Installer
    {
        public SvcInstaller()
        {
            InitializeComponent();
        }
    }
}
