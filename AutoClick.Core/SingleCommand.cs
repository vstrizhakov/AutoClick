using System;
using System.Collections;
using System.Collections.Generic;

namespace AutoClick.Core
{
    public class SingleCommandEnumerator : IEnumerator<ICommand>
    {
        private readonly ICommand _command;
        private ICommand _current;

        public ICommand Current
        {
            get
            {
                if (_current == null)
                {
                    throw new InvalidOperationException();
                }

                return _current;
            }
        }

        object IEnumerator.Current => Current;

        public SingleCommandEnumerator(ICommand command)
        {
            _command = command ?? throw new ArgumentNullException(nameof(command));
        }

        public void Dispose()
        {
        }

        public bool MoveNext()
        {
            if (_current != null)
            {
                return false;
            }

            _current = _command;
            return true;
        }

        public void Reset()
        {
            _current = null;
        }
    }

    public class SingleCommand : IScript
    {
        private readonly ICommand _command;

        public SingleCommand(ICommand command)
        {
            _command = command ?? throw new ArgumentNullException(nameof(command));
        }

        public IEnumerator<ICommand> GetEnumerator()
        {
            return new SingleCommandEnumerator(_command);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
}
