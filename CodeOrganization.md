# jeruk-fw code organization #

Wiki pages list: https://code.google.com/p/jeruk-fw/w/list

Refer to http://wmulia.org/plp/jeruk for general overview of the board, principles of operations, I/O organization, and program loading routines.

This document describes the organization of the JERUK board default bootloader. The bootloader provides the following functions for users / programmers of the board:
  * A terminal interface through UART1 to support interactive commands
  * Terminal I/O routines (ASCII conversions, UART1 functions)
  * Other I/O interface, built-in and expansion I/O (switches, LEDs, UART2, SPI, I2C, analog inputs, edge interrupt)
  * Program loading (RAM and Flash)
  * Program execution / hand-off routines
  * PLP CPU on-chip emulation

# Firmware Functions and Code Organization #

### main.c ###
`main.c` is the main source code and it contains calls to initialization routines and [the command input processor](TerminalInputProcessor.md). All board-specific initialization should be placed inside the `jeruk_init()` routine. `main.c` will call I/O-specific init routines from an external source code as more I/O functions are added. `main.c` currently only calls UART1 and UART2 initialization routines that reside in `wio.c`. Check out [the input processor page](TerminalInputProcessor.md) to see how to support new terminal commands.

### wio.c ###
`wio.c` contains [terminal I/O routines](TerminalIO.md) such as reading/writing from/to the UART ports, some string routines, and parsing ASCII inputs into their literal values and vice versa. This file also contains [routines for UART2](UART2Routines.md).

### proccfg.h ###
`proccfg.h` header contains processor-specific configuration such as fuse bits (DEVCFGx bits) and other constants pertinent to the board.

### wloader.c ###
`wloader.c` contains the [wload RAM loader](http://wmulia.org/plp/jeruk/#wload) routine.

### plpemu.c ###
`plpemu.c` contains the PLP CPU emulator code and the fload loader routine.

### envy.c ###
`envy.c` contains the non-volatile memory loader ([envy](NVLoader.md)).

### misc.c ###
Miscellaneous functions.

### Adding Functions ###
To add routines to support a new I/O module or expansion board, please create new source code and header and keep `main.c` clean of I/O code. For example, if we ever decide to finally add USB support, we may want to create `jusb.c` or `jusb.h` for this purpose.