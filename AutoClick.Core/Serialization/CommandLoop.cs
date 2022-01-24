using Newtonsoft.Json;
using System.Collections.Generic;
using System.Linq;

namespace AutoClick.Core.Serialization
{
    public class CommandLoop : CommandSequence
    {
        public override string Type => "command_loop_script";

        [JsonProperty("count")]
        public uint? Count { get; private set; }

        public CommandLoop()
        {
        }

        public CommandLoop(IList<Script> scripts, uint? count) : base(scripts)
        {
            Count = count;
        }

        public override IScript ToCore()
        {
            var coreScripts = Scripts.Select(e => e.ToCore()).ToList();
            var core = new Core.CommandLoop(coreScripts, Count);
            return core;
        }
    }
}
