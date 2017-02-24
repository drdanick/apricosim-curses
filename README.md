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

Compiling a minimal version of the simulator is as simple as entering the following commands:
```
aclocal
automake --add-missing
autoconf
./configure
make
sudo make install
```

For a fully featured version, which is required to run the Apricot OS, enter the following before `make`:
```
./configure --enable-tty-emulation --enable-port-emulation --enable-disk-emulation
```

Running
-------

To run the simulator without any programs, enter `aprsim` in your shell.

Besides running with no input, the simulator supports a number of command line parameters to customise execution:  
`--version`          /  `-v`   Print version information and exit.  
`--help`             /  `-h`   Print program usage information.  
`--fifo_tty_file`    /  `-f`   Set fifo tty file. (Only if compiled with `enable-tty-emulation`)  
`--serial_tty_file`  /  `-u`   Set serial device tty file. (Only if compiled with `enable-tty-emulation`)  
`--symbols`          /  `-s`   Load a symbols file.  

Program binaries are listed after all other arguments, and are separated by spaces.


Using the simulator
-------------------

The simulator can be operated with the following controls:  
`j/k`         Scroll the memory display down/up.  
`J/K`         Scroll the stack display down/up.  
`PGdown/PGup` Scroll the memory display 256 lines down/up.  
`p`           Jump to program counter position in memory display.  
`r`           Toggle between register and CPU status display modes.  
`b`           Toggle a breakpoint.  
`m`           Toggle between state execution and instruction execution modes.  
`space`       Execute one CPU state or instruction.  
`q`           Quit the simulator.  
`F1`          Interrupt the currently running program and quit the simulator.


Licence
-------

See LICENCE for details.
