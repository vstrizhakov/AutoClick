using AutoClick.Core.Serialization;

namespace AutoClick
{
    internal abstract class ScriptItem
    {
        public abstract Script Script { get; }

        public abstract string DisplayName { get; }

        public override string ToString()
        {
            return DisplayName;
        }
    }
}
