using AutoClick.Views;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace AutoClick.ViewModels
{
    internal class MainWindowViewModel : BindableBase
    {
        private readonly DelegateCommand _openSettingsCommand;

        public ICommand OpenSettingsCommand => _openSettingsCommand;

        public MainWindowViewModel()
        {
            _openSettingsCommand = new DelegateCommand(OpenSettings);
        }

        private void OpenSettings()
        {
            var window = new SettingsWindow();
            window.ShowDialog();
        }
    }
}
