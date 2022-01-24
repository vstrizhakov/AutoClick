using Newtonsoft.Json;

namespace AutoClick.Core.Serialization
{
    [JsonConverter(typeof(ScriptJsonConverter))]
    public abstract class Script
    {
        [JsonProperty("type")]
        public abstract string Type { get; }

        public abstract IScript ToCore();
    }
}
