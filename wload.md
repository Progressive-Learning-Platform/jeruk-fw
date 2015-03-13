# wload RAM Loader #

wload is the interactive UART RAM loader. wload can be used to write to RAM locations on the microcontroller. You can see the [wload section on the jeruk website](http://wmulia.org/plp/jeruk/#wload) for the protocol. The protocol allows programming file to be formatted so it is human readable. [wloader.c](https://code.google.com/p/jeruk-fw/source/browse/testfw.X/wloader.c) is where wload resides in the source tree.

# Development Notes #

The jump is definitely broken. You have to use long jump to jump to RAM space (i.e. jr or jalr instead of j or jal).