using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;

namespace AutoClick.Core.Serialization
{
    public class CommandJsonConverter : JsonConverter<Command>
    {
        public override bool CanWrite => false;

        public override Command ReadJson(JsonReader reader, Type objectType, Command existingValue, bool hasExistingValue, JsonSerializer serializer)
        {
            var jObject = JObject.Load(reader);

            if (jObject.TryGetValue("type", out var typeToken))
            {
                var type = typeToken.ToString();
                if (type == "keyboard_command")
                {
                    var obj = new KeyboardCommand();
                    serializer.Populate(jObject.CreateReader(), obj);
                    return obj;
                }
            }
            return null;
        }

        public override void WriteJson(JsonWriter writer, Command value, JsonSerializer serializer)
        {
            throw new NotImplementedException();
        }
    }
}
