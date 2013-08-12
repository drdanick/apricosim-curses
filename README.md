Apricosim
=========

A cycle accurate simulator for the Apricos CPU architectore



Dependencies
------------

This particular distribution of apricosim requires the following binaries:

gcc 4.0.0+
autoconf 2.65


The following libraries are also required:

curses/ncurses


Compiling
---------

Before the makefiles can be generated, a configure script must first be generated.
This can be done by entering `autoconf` in your shell.

Afterwards, compiling the simulator is as simple as entering the following commands:
```no-highlight
./configure
make
make install
```

To run the simulator, enter `aprsim`.
