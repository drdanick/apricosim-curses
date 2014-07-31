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
```
aclocal
automake --add-missing
autoconf
./configure
make
sudo make install
```

To run the simulator, enter `aprsim` in your shell.


Using the simulator
-------------------

The simulator can be operated with the following controls:  
`j/k`   Scroll the memory display down/up.  
`J/K`   Scroll the stack display down/up.  
`r`     Toggle between register and CPU status display modes.  
`b`     Toggle a breakpoint.  
`m`     Toggle between state execution and instruction execution modes.  
`space` Execute one CPU state or instruction.  
`q`     Quit the simulator


Licence
-------

See LICENCE for details.
