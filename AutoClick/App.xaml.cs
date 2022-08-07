using InputSimulator;
using System;
using System.Windows;

namespace AutoClick
{
    public partial class App : Application
    {
        public static InputManager Input { get; } = new InputManager();
        public static SettingsManager Settings { get; } = new SettingsManager();

        private void Application_Startup(object sender, StartupEventArgs args)
        {
            try
            {
                Input.Initialize();
            }
            catch (InvalidOperationException ex)
            {
                MessageBox.Show(ex.Message);
            }

            var window = new MainWindow();
            window.Show();
        }
    }
}
