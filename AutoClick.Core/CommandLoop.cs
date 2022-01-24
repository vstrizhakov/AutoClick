using System;
using System.Collections;
using System.Collections.Generic;

namespace AutoClick.Core
{
    public class CommandLoopEnumerator : CommandSequenceEnumerator
    {
        private const int InitialIteration = 0;

        private readonly uint? _count;
        private int _currentIteration = InitialIteration;

        public CommandLoopEnumerator(IEnumerable<IScript> scripts, uint? count) : base(scripts)
        {
            _count = count;
        }

        public override bool MoveNext()
        {
            while (!base.MoveNext())
            {
                if (_count == null || _currentIteration < _count)
                {
                    base.Reset();
                    _currentIteration++;
                }
                else
                {
                    return false;
                }
            }

            return true;
        }

        public override void Reset()
        {
            base.Reset();
            _currentIteration = InitialIteration;
        }
    }

    public class CommandLoop : IScript
    {
        private readonly uint? _count;
        private readonly IEnumerable<IScript> _scripts;

        public CommandLoop(IEnumerable<IScript> scripts, uint? count)
        {
            _scripts = scripts ?? throw new ArgumentNullException(nameof(scripts));
            _count = count;
        }

        public IEnumerator<ICommand> GetEnumerator()
        {
            return new CommandLoopEnumerator(_scripts, _count);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
}
