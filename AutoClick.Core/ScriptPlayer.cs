using InputSimulator;
using System;
using System.Threading;
using System.Threading.Tasks;

namespace AutoClick.Core
{
    public class ScriptPlayer : IScriptPlayer, IDisposable
    {
        private readonly InputManager _inputManager;

        private SemaphoreSlim _semaphore;
        private CancellationTokenSource _cancellationTokenSource;
        private Task _task;
        private bool _isPlaying;

        public bool IsPlaying => _isPlaying;

        public ScriptPlayer(InputManager inputManager)
        {
            _inputManager = inputManager ?? throw new ArgumentNullException(nameof(inputManager));

            _semaphore = new SemaphoreSlim(1, 1);
        }

        public void Dispose()
        {
            _semaphore.Dispose();
        }

        public async Task PlayAsync(IScript script)
        {
            await _semaphore.WaitAsync()
                .ConfigureAwait(false);
            try
            {
                if (!_isPlaying)
                {
                    _cancellationTokenSource = new CancellationTokenSource();
                    var token = _cancellationTokenSource.Token;
                    _task = Task.Run(async () => await PlayInternalAsync(script, token));

                    _isPlaying = true;
                }
            }
            finally
            {
                _semaphore.Release();
            }
        }

        public async Task StopAsync()
        {
            await _semaphore.WaitAsync()
                .ConfigureAwait(false);
            try
            {
                if (_isPlaying)
                {
                    if (_cancellationTokenSource != null)
                    {
                        _cancellationTokenSource.Cancel();
                        _cancellationTokenSource.Dispose();
                        _cancellationTokenSource = null;
                    }

                    if (_task != null)
                    {
                        try
                        {
                            await _task.ConfigureAwait(false);
                        }
                        catch
                        {
                        }
                    }

                    _isPlaying = false;
                }
            }
            finally
            {
                _semaphore.Release();
            }
        }

        private async Task PlayInternalAsync(IScript script, CancellationToken cancellationToken)
        {
            try
            {
                foreach (var command in script)
                {
                    cancellationToken.ThrowIfCancellationRequested();

                    if (command is IKeyboardCommand keyboardCommand)
                    {
                        var keyCode = keyboardCommand.KeyCode;
                        _inputManager.SendKeyDown(keyCode);
                        _inputManager.SendKeyUp(keyCode);
                    }

                    await Task.Delay(command.Delay, cancellationToken).ConfigureAwait(false);
                }
            }
            catch (OperationCanceledException)
            {
            }
        }
    }
}
