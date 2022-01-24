namespace AutoClick.Core
{
    public interface IKeyboardCommand : ICommand
    {
        ushort KeyCode { get; }
    }
}
