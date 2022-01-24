using System;

namespace AutoClick.Core
{
    public class KeyboardCommand : IKeyboardCommand
    {
        public TimeSpan Delay { get; }

        public ushort KeyCode { get; }

        public KeyboardCommand(TimeSpan delay, ushort keyCode)
        {
            KeyCode = keyCode;
            Delay = delay;
        }
    }
}
