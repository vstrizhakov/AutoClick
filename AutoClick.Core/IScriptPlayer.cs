using System.Threading.Tasks;

namespace AutoClick.Core
{
    public interface IScriptPlayer
    {
        Task PlayAsync(IScript script);
        Task StopAsync();
    }
}
