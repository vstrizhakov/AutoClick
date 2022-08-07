using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AutoClick
{
    public class SettingsManager
    {
        public event EventHandler SettingsChanged;

        public ushort PauseKey
        {
            get => Settings.Default.PauseKey;
            set
            {
                Settings.Default.PauseKey = value;
                Settings.Default.Save();
                RaiseSettingsChanged();
            }
        }

        public ushort PlayKey
        {
            get => Settings.Default.PlayKey;
            set
            {
                Settings.Default.PlayKey = value;
                Settings.Default.Save();
                RaiseSettingsChanged();
            }
        }

        private void RaiseSettingsChanged()
        {
            SettingsChanged?.Invoke(this, EventArgs.Empty);
        }
    }
}
