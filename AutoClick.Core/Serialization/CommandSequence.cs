using Newtonsoft.Json;
using System.Collections.Generic;
using System.Linq;

namespace AutoClick.Core.Serialization
{
    public class CommandSequence : Script
    {
        public override string Type => "command_sequence_script";

        [JsonProperty("scripts")]
        public IList<Script> Scripts { get; private set; }

        public CommandSequence()
        {
        }

        public CommandSequence(IList<Script> scripts)
        {
            Scripts = scripts;
        }

        public override IScript ToCore()
        {
            var coreScripts = Scripts.Select(e => e.ToCore()).ToList();
            var core = new Core.CommandSequence(coreScripts);
            return core;
        }
    }
}
