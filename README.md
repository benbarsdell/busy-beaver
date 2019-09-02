# BB's Busy Beaver

Efficient simulator of [busy
beaver](https://en.wikipedia.org/wiki/Busy_Beaver_game) Turing
machines, capable of evaluating the best known 6-state machine (that
halts after [7.412e36534
steps](http://www.logique.jussieu.fr/~michel/beh.html#tm62h)) in less
than a minute on a single modern CPU core.

This is a C++11 implementation of the techniques described in [Alex
Holkner,
2004](https://www.semanticscholar.org/paper/Acceleration-Techniques-for-Busy-Beaver-Candidates-Holkner/b94fbed213cd7e70635b6c8e102a09fda9201d3b).

An AWK implementation by Heiner Marxen from 2006 can be found
[here](https://web.archive.org/web/20061009141225/http://www.drb.insel.de/~heiner/BB/hmMMsimu.awk).

A history of busy beaver machines can be found
[here](http://www.logique.jussieu.fr/~michel/ha.html).

A discussion of recent progress in evaluating all 5-state machines can
be found [here](https://googology.wikia.org/wiki/Forum:Sigma_project).

# Dependencies

  * [GNU Multiple Precision Arithmetic library](https://gmplib.org/)

# Instructions

    $ make get-deps
    $ make -j8
    $ make test
    $ ./busy_beaver -h

# About

This implementation currently only supports single-tape, 2-symbol
(binary), <=6-state machines. There is also a limitation that
`macro_nbit` <= 60.

The code implements 3 types of simulators, each building on the previous one:

  1. Micro machine: represents the tape as individual bits and
  implements direct step-by-step evaluation of the Turing machine.

  2. Macro machine: represents the tape as a run-length-encoded
  sequence of macro symbols (sequences of `macro_nbit` bits on the
  tape) and implements accelerated simulation capable of updating
  whole runs at a time.

  3. Proof machine: identifies patterns in the history of the
  macro-machine simulation, attempts to prove that they will continue a
  certain number of times, and then applies them to skip the
  simulation ahead.

The macro machine implemented here does not currently support
tail/back-context.

The simulation speed varies widely between different busy beaver
programs. While the best known 6-state machine can be readily
simulated to completion, some other machines simulate very slowly and
completing them is impractical (even though they may be known to halt
in fewer steps). The speed (and tape compression) can also depend
strongly on the choice of `macro_nbit`.
