Apricosim
=========

A cycle accurate simulator for the Apricos CPU architecture.



Dependencies
------------

This particular distribution of apricosim requires the following binaries:

- gcc 4.0.0+
- autoconf 2.65+
- automake 1.11+


The following libraries are also required:

- curses/ncurses


Compiling
---------

Compiling the simulator is as simple as entering the following commands:
```no-highlight
aclocal
automake
autoconf
./configure
make
make install
```

To run the simulator, enter `aprsim`.
