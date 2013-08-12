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
sudo make install
```

To run the simulator, enter `aprsim` in your shell.


Using the simulator
-------------------

The simulator can be operated with the following controls:  
`j/k`   Scroll the memory display down/up.  
`J/K`   Scroll the stack display down/up.  
`b`     Toggle a breakpoint.  
`m`     Toggle between state execution and instruction execution modes.
`space` Execute one CPU state or instruction.  

Notes
-----

As a test, the simulator loads the following instructions into memory:   
```
00001011  
01010000  
01010000  
01010000  
01010000  
00001010  
10100000  
10101100
```

This sample program will place the value of 0xBA into the accumulator, 
push the accumulator to the stack, then pop the stack, adding the popped 
value to the acucmulator. The accumulator should hold the value of 2 * (0xBA).
