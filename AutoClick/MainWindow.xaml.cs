using AutoClick.Core;
using Microsoft.Win32;
using Newtonsoft.Json;
using ScreenCapture;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;

namespace AutoClick
{
    public partial class MainWindow : Window
    {
        public static readonly TimeSpan DefaultDelay = TimeSpan.FromMilliseconds(10);

        #region Private Fields

        private readonly InputSimulator.InputManager _inputManager;
        private readonly CaptureManager _captureManager;
        private readonly ScriptPlayer _scriptPlayer;

        private bool _isCapturing;
        private Int32Rect _selectedRectangle = new Int32Rect(0, 0, 1920, 1080);
        private TimeSpan _lastRenderingTime;
        private List<RenderOutput> _renderOutputs;

        private RenderOutput _desktopRenderOutput;

        private bool _isInProcess;

        #endregion

        #region Life Cycle

        public MainWindow()
        {
            InitializeComponent();
            _inputManager = new InputSimulator.InputManager();
            //_captureManager = new CaptureManager();
            //_renderOutputs = new List<RenderOutput>();
            _scriptPlayer = new ScriptPlayer(_inputManager);

            Topmost = true;
            DelayTextBox.Text = DefaultDelay.TotalMilliseconds.ToString();

            Loaded += MainWindow_Loaded;
            Closing += MainWindow_Closing;
        }

        private void MainWindow_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            //CompositionTarget.Rendering -= CompositionTarget_Rendering;
            //_captureManager?.Dispose();
        }

        private void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            //_desktopRenderOutput = _captureManager.AddOutput(D3DImageSource, new System.Windows.Rect(0, 0, 1, 1));

            //CompositionTarget.Rendering += CompositionTarget_Rendering;

            _inputManager.KeyPressed += InputManager_KeyPressed;

            try
            {
                _inputManager.Initialize();
            }
            catch (InvalidOperationException ex)
            {
                MessageBox.Show(ex.Message);
            }

            UpdateButtons();
        }

        #endregion

        #region Private Methods

        private async Task PlayScriptAsync()
        {
            var scripts = CaptureKeyStrokesListBox.Items.OfType<ScriptItem>().Select(e => e.Script.ToCore()).ToList();
            var commandLoop = new Core.CommandLoop(scripts, null);
            await _scriptPlayer.PlayAsync(commandLoop);

            UpdateButtons();
        }

        private async Task StopScriptAsync()
        {
            await _scriptPlayer.StopAsync();

            UpdateButtons();
        }

        private void UpdateButtons()
        {
            PlayScriptButton.IsEnabled = !_isCapturing && !_isInProcess && !_scriptPlayer.IsPlaying;
            StopScriptButton.IsEnabled = !_isCapturing && !_isInProcess && _scriptPlayer.IsPlaying;
            CaptureKeyboardButton.IsEnabled = !_scriptPlayer.IsPlaying && !_isInProcess;
            SaveButton.IsEnabled = !_isCapturing && !_isInProcess && !_scriptPlayer.IsPlaying;
            OpenButton.IsEnabled = !_isCapturing && !_isInProcess && !_scriptPlayer.IsPlaying;
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
            if (_isInProcess)
            {
                return;
            }

            _isCapturing = !_isCapturing;
            if (_isCapturing)
            {
                CaptureKeyStrokesListBox.Items.Clear();
            }
            UpdateButtons();
        }

        private void InputManager_KeyPressed(object sender, ushort keyCode)
        {
            Dispatcher.Invoke(async () =>
            {
                if (KeyInterop.KeyFromVirtualKey(keyCode) == Key.Escape)
                {
                    await StopScriptAsync();
                }
                else if (_isCapturing)
                {
                    var script = new Core.Serialization.SingleCommand(new Core.Serialization.KeyboardCommand(DefaultDelay, keyCode));
                    CaptureKeyStrokesListBox.Items.Add(new SingleCommandItem(script));
                }
            });
        }

        private async void PlayScriptButton_Click(object sender, RoutedEventArgs e)
        {
            
            await PlayScriptAsync();
        }

        private async void StopScriptButton_Click(object sender, RoutedEventArgs e)
        {
            await StopScriptAsync();
        }

        private async void SaveButton_Click(object sender, RoutedEventArgs args)
        {
            if (_isInProcess || _isCapturing)
            {
                return;
            }

            _isInProcess = true;
            UpdateButtons();

            try
            {
                var fileSaveDialog = new SaveFileDialog()
                {
                    Filter = "Macros Files|*.macros",
                    DefaultExt = ".macros",
                    AddExtension = true,
                };
                if (fileSaveDialog.ShowDialog() == true)
                {
                    var scripts = CaptureKeyStrokesListBox.Items.OfType<ScriptItem>().Select(e => e.Script).ToList();
                    var commandLoop = new Core.Serialization.CommandLoop(scripts, null);
                    var content = JsonConvert.SerializeObject(commandLoop);

                    using (var stream = fileSaveDialog.OpenFile())
                    {
                        using (var streamWriter = new StreamWriter(stream))
                        {
                            await streamWriter.WriteAsync(content);
                        }
                    }
                }
            }
            finally
            {
                _isInProcess = false;
                UpdateButtons();
            }
        }

        private async void OpenButton_Click(object sender, RoutedEventArgs e)
        {
            if (_isInProcess || _isCapturing)
            {
                return;
            }

            _isInProcess = true;
            UpdateButtons();

            try
            {
                var openFileDialog = new OpenFileDialog()
                {
                    Filter = "Macros Files|*.macros",
                };
                if (openFileDialog.ShowDialog() == true)
                {
                    using (var stream = openFileDialog.OpenFile())
                    {
                        using (var streamReader = new StreamReader(stream))
                        {
                            var content = await streamReader.ReadToEndAsync();
                            var commandLoop = JsonConvert.DeserializeObject<Core.Serialization.CommandLoop>(content);

                            CaptureKeyStrokesListBox.Items.Clear();
                            foreach (var singleCommand in commandLoop.Scripts.OfType<Core.Serialization.SingleCommand>())
                            {
                                CaptureKeyStrokesListBox.Items.Add(new SingleCommandItem(singleCommand));
                            }
                        }
                    }
                }
            }
            finally
            {
                _isInProcess = false;
                UpdateButtons();
            }
        }

        private void CompositionTarget_Rendering(object sender, EventArgs e)
        {
            if (e is RenderingEventArgs renderingEventArgs)
            {
                if (D3DImageSource.IsFrontBufferAvailable && renderingEventArgs.RenderingTime != _lastRenderingTime)
                {
                    _captureManager.Render();
                    _lastRenderingTime = renderingEventArgs.RenderingTime;
                }
            }
        }
        private void DelayTextBox_TextChanged(object sender, System.Windows.Controls.TextChangedEventArgs e)
        {

        }

        #endregion
    }
}
