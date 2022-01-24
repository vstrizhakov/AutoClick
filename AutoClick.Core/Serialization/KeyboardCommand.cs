using Newtonsoft.Json;
using System;

namespace AutoClick.Core.Serialization
{
    public class KeyboardCommand : Command
    {
        public override string Type => "keyboard_command";

        [JsonProperty("key_code")]
        public ushort KeyCode { get; private set; }

        public KeyboardCommand()
        {
        }

        public KeyboardCommand(TimeSpan delay, ushort keyCode) : base(delay)
        {
            KeyCode = keyCode;
        }

        public override ICommand ToCore()
        {
            return new Core.KeyboardCommand(Delay, KeyCode);
        }
    }
}
