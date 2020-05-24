using InputSimulator;
using ScreenCapture;
using System;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;

namespace Bot
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        #region Private Fields

        private InputManager _inputManager;
        private CaptureManager _captureManager;

        private bool _isCapturing;
        private Int32Rect _selectedRectangle = new Int32Rect(0, 0, 1920, 1080);
        private CancellationTokenSource _cancellationTokenSource;
        private TimeSpan _lastRenderingTime;

        #endregion

        #region Life Cycle

        public MainWindow()
        {
            InitializeComponent();
            _inputManager = new InputManager();
            _captureManager = new CaptureManager(1920, 1080, true, 4);

            Loaded += MainWindow_Loaded;
            Closing += MainWindow_Closing;
        }

        private void MainWindow_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            CompositionTarget.Rendering -= CompositionTarget_Rendering;
            _captureManager?.Dispose();
        }

        private void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            CompositionTarget.Rendering += CompositionTarget_Rendering;
            if (_inputManager != null)
            {
                _inputManager.Initialize();
                _inputManager.KeyPressed += InputManager_KeyPressed;
            }
        }

        #endregion

        #region Public Methods

        #endregion

        #region Private Methods

        private async Task PlayScript()
        {
            StopScript();
            _cancellationTokenSource = new CancellationTokenSource();
            var token = _cancellationTokenSource.Token;
            var keyCodes = CaptureKeyStrokesListBox.Items.OfType<ushort>().ToList();
            try
            {
                await Task.Run(async () =>
                {
                    while (true)
                    {
                        foreach (var keyCode in keyCodes)
                        {
                            token.ThrowIfCancellationRequested();
                            _inputManager.SendKeyDown(keyCode);
                            _inputManager.SendKeyUp(keyCode);
                            await Task.Delay(10);
                        }
                    }
                }, token);
            }
            catch (OperationCanceledException)
            {

            }
        }

        private void StopScript()
        {
            if (_cancellationTokenSource != null)
            {
                _cancellationTokenSource.Cancel();
                _cancellationTokenSource.Dispose();
                _cancellationTokenSource = null;
            }
        }

        #endregion

        #region Callbacks

        private async void Button_Click(object sender, RoutedEventArgs e)
        {
            await Task.Delay(2000);
            _inputManager.SendString(Stroke.Text);
            _inputManager.SendLeftMouseClick(500, 500);
        }

        private void SelectAreaButton_Click(object sender, RoutedEventArgs e)
        {
            Hide();
            SelectionWindow selectionWindow = new SelectionWindow();
            bool? result = selectionWindow.ShowDialog();

            //if (result == true)
            //{
            //    Rect rect = selectionWindow.Rectangle;
            //    int width = _captureManager.Width;
            //    int height = _captureManager.Height;
            //    int x = (int)(rect.X * width);
            //    int y = (int)(rect.Y * height);
            //    int intWidth = (int)(rect.Width * width);
            //    int intHeight = (int)(rect.Height * height);
            //    _selectedRectangle = new Int32Rect(x, y, intWidth, intHeight);
            //}

            Show();
        }

        private void CaptureKeyboardButton_Click(object sender, RoutedEventArgs e)
        {
            _isCapturing = !_isCapturing;
            if (_isCapturing)
            {
                CaptureKeyStrokesListBox.Items.Clear();
            }
        }

        private void InputManager_KeyPressed(object sender, ushort keyCode)
        {
            if (keyCode == 1)
            {
                StopScript();
            }
            else if (_isCapturing)
            {
                Dispatcher.Invoke(() =>
                {
                    CaptureKeyStrokesListBox.Items.Add(keyCode);
                });
            }
        }

        private async void PlayScriptButton_Click(object sender, RoutedEventArgs e)
        {
            await PlayScript();
        }

        private void StopScriptButton_Click(object sender, RoutedEventArgs e)
        {
            StopScript();
        }

        private void CompositionTarget_Rendering(object sender, EventArgs e)
        {
            if (e is RenderingEventArgs renderingEventArgs)
            {
                if (D3DImageSource.IsFrontBufferAvailable && renderingEventArgs.RenderingTime != _lastRenderingTime)
                {
                    _captureManager.Render(D3DImageSource);
                    _lastRenderingTime = renderingEventArgs.RenderingTime;
                }
            }
        }

        #endregion
    }
}
