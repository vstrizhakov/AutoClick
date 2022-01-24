using Newtonsoft.Json;

namespace AutoClick.Core.Serialization
{
    public class SingleCommand : Script
    {
        public override string Type => "single_command_script";

        [JsonProperty("command")]
        public Command Command { get; private set; }

        public SingleCommand()
        {
        }

        public SingleCommand(Command command)
        {
            Command = command;
        }

        public override IScript ToCore()
        {
            return new Core.SingleCommand(Command.ToCore());
        }
    }
}
