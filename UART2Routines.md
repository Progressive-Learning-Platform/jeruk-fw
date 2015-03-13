These routines reside in `wio.c`

## UART2 Routines ##

`void init_uart2(int pbclock, int baud)`

> Initialize UART2 given the peripheral clock (in Hz) and baud rate. The bootloader configures this port to run at 9600 by default, but this can be changed using the '`u2bd`' terminal command. This routine will configure UART1 with 8 data bits, no parity, and 1 stop bit.

`void u2_set_baud(int val_int)`

> Set a new baud rate for UART2. The routine will also print out the new baud rate in hex and the updated BRG register value.

`char u2_blocking_read()`

> Block until there is new data from UART2. The data will be returned as char type.

`void u2_read_print()`

> Read from UART2 and print the hex ASCII representation of the value to the terminal. If no data is received, the routine will time out and return.

`void u2_write(char a)`

> Write a character to UART2.