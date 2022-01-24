using System;

namespace AutoClick.Core
{
    public interface ICommand
    {
        TimeSpan Delay { get; }
    }
}
