using Prism.Commands;
using Prism.Mvvm;
using System.Windows.Input;

namespace AutoClick.ViewModels
{
    internal class SettingsWindowViewModel : BindableBase
    {
        private readonly DelegateCommand _windowLoadedCommand;
        private readonly DelegateCommand _windowClosingCommand;
        private readonly DelegateCommand _selectPauseKeyCommand;
        private readonly DelegateCommand _selectPlayKeyCommand;

        private ushort _pauseKey;
        private ushort _playKey;
        private bool _selectingPauseKey;
        private bool _selectingPlayKey;
        
        public ushort PauseKey
        {
            get => _pauseKey;
            private set
            {
                if (SetProperty(ref _pauseKey, value))
                {
                    App.Settings.PauseKey = _pauseKey;
                }
            }
        }

        public ushort PlayKey
        {
            get => _playKey;
            private set
            {
                if (SetProperty(ref _playKey, value))
                {
                    App.Settings.PlayKey = _playKey;
                }
            }
        }

        public bool SelectingPauseKey
        {
            get => _selectingPauseKey;
            private set
            {
                if (SetProperty(ref _selectingPauseKey, value))
                {
                    SelectingPlayKey = false;
                    _selectPauseKeyCommand.RaiseCanExecuteChanged();
                }
            }
        }

        public bool SelectingPlayKey
        {
            get => _selectingPlayKey;
            private set
            {
                if (SetProperty(ref _selectingPlayKey, value))
                {
                    SelectingPauseKey = false;
                    _selectPlayKeyCommand.RaiseCanExecuteChanged();
                }
            }
        }

        public ICommand WindowLoadedCommand => _windowLoadedCommand;
        public ICommand WindowClosingCommand => _windowClosingCommand;
        public ICommand SelectPauseKeyCommand => _selectPauseKeyCommand;
        public ICommand SelectPlayKeyCommand => _selectPlayKeyCommand;

        public SettingsWindowViewModel()
        {
            _windowLoadedCommand = new DelegateCommand(OnWindowLoaded);
            _windowClosingCommand = new DelegateCommand(OnWindowClosing);
            _selectPauseKeyCommand = new DelegateCommand(() => SelectingPauseKey = true, () => !SelectingPauseKey);
            _selectPlayKeyCommand = new DelegateCommand(() => SelectingPlayKey = true, () => !SelectingPlayKey);

            _playKey = App.Settings.PlayKey;
            _pauseKey = App.Settings.PauseKey;
        }

        private void OnWindowLoaded()
        {
            App.Input.KeyPressed += InputManager_KeyPressed;
        }

        private void OnWindowClosing()
        {
            App.Input.KeyPressed -= InputManager_KeyPressed;
        }

        private void InputManager_KeyPressed(object sender, ushort keyCode)
        {
            if (SelectingPauseKey)
            {
                PauseKey = keyCode;
                SelectingPauseKey = false;
            }
            else if (SelectingPlayKey)
            {
                PlayKey = keyCode;
                SelectingPlayKey = false;
            }
        }
    }
}
