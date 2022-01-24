using System;
using System.Collections;
using System.Collections.Generic;

namespace AutoClick.Core
{
    public class CommandSequenceEnumerator : IEnumerator<ICommand>
    {
        private readonly IEnumerator<IScript> _scripts;
        private IEnumerator<ICommand> _currentScript;

        public virtual ICommand Current
        {
            get
            {
                if (_currentScript == null)
                {
                    throw new InvalidOperationException();
                }

                return _currentScript.Current;
            }
        }

        object IEnumerator.Current => Current;

        public CommandSequenceEnumerator(IEnumerable<IScript> scripts)
        {
            _scripts = scripts?.GetEnumerator() ?? throw new ArgumentNullException(nameof(scripts));
        }

        public virtual void Dispose()
        {
            _scripts.Dispose();
            if (_currentScript != null)
            {
                _currentScript.Reset();
                _currentScript.Dispose();
                _currentScript = null;
            }
        }

        public virtual bool MoveNext()
        {
            while (_currentScript == null || !_currentScript.MoveNext())
            {
                if (!_scripts.MoveNext())
                {
                    return false;
                }

                _currentScript = _scripts.Current.GetEnumerator();
            }

            return true;
        }

        public virtual void Reset()
        {
            _scripts.Reset();
            if (_currentScript != null)
            {
                _currentScript.Dispose();
                _currentScript = null;
            }
        }
    }

    public class CommandSequence : IScript
    {
        private readonly IEnumerable<IScript> _scripts;

        public CommandSequence(IEnumerable<IScript> scripts)
        {
            _scripts = scripts ?? throw new ArgumentNullException(nameof(scripts));
        }

        public IEnumerator<ICommand> GetEnumerator()
        {
            return new CommandSequenceEnumerator(_scripts);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
}
