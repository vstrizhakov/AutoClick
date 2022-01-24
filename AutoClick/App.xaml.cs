using System.Windows;

namespace AutoClick
{
    public partial class App : Application
    {
        private void Application_Startup(object sender, StartupEventArgs args)
        {
            var window = new MainWindow();
            window.Show();
        }
    }
}
