using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;

namespace AutoClick.Core.Serialization
{
    public class ScriptJsonConverter : JsonConverter<Script>
    {
        public override bool CanWrite => false;

        public override Script ReadJson(JsonReader reader, Type objectType, Script existingValue, bool hasExistingValue, JsonSerializer serializer)
        {
            var jObject = JObject.Load(reader);

            if (jObject.TryGetValue("type", out var typeToken))
            {
                var type = typeToken.ToString();
                if (type == "single_command_script")
                {
                    var obj = new SingleCommand();
                    serializer.Populate(jObject.CreateReader(), obj);
                    return obj;
                }
                else if (type == "command_sequence_script")
                {
                    var obj = new CommandSequence();
                    serializer.Populate(jObject.CreateReader(), obj);
                    return obj;
                }
                else if (type == "command_loop_script")
                {
                    var obj = new CommandLoop();
                    serializer.Populate(jObject.CreateReader(), obj);
                    return obj;
                }
            }
            return null;
        }

        public override void WriteJson(JsonWriter writer, Script value, JsonSerializer serializer)
        {
            throw new NotImplementedException();
        }
    }
}
