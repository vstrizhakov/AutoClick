using Newtonsoft.Json;
using System;

namespace AutoClick.Core.Serialization
{
    [JsonConverter(typeof(CommandJsonConverter))]
    public abstract class Command
    {
        [JsonProperty("type")]
        public abstract string Type { get; }

        [JsonProperty("delay")]
        public TimeSpan Delay { get; private set; }


        public Command()
        {
        }

        public Command(TimeSpan delay)
        {
            Delay = delay;
        }

        public abstract ICommand ToCore();
    }
}
