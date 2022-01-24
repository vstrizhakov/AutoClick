using AutoClick.Core.Serialization;
using System;
using System.Windows.Input;

namespace AutoClick
{
    internal class SingleCommandItem : ScriptItem
    {
        private readonly SingleCommand _singleCommand;

        public override Script Script => _singleCommand;

        public override string DisplayName
        {
            get
            {
                if (_singleCommand.Command is KeyboardCommand keyboardCommand)
                {
                    return KeyInterop.KeyFromVirtualKey(keyboardCommand.KeyCode).ToString();
                }
                return "???";
            }
        }

        public SingleCommandItem(SingleCommand singleCommand)
        {
            _singleCommand = singleCommand ?? throw new ArgumentNullException(nameof(singleCommand));
        }
    }
}
