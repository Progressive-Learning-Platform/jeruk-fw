These routines reside in `wio.c`

## Globals (`wio.h`) ##

`char input_buf[80]`

> 80-character input buffer used by the `parse` routine.

`char input_ptr`

> End-of-buffer pointer for `input_buf`.

## UART1 Terminal I/O Routines ##

`void init_uart1(int pbclock, int baud)`

> Initialize UART1 given the peripheral clock (in Hz) and baud rate. Called by the bootloader to setup the terminal interface with 57600 baud rate. This routine will configure UART1 with 8 data bits, no parity, and 1 stop bit.

`void print(char* buf)`

> Print the provided string literal to UART1

`void psstr(char* buf, int start, int len)`

> Print the substring of the given string literal to UART1. Warning, there is no length checking!

`void pchar(char a)`

> Print a character to UART1.

`void pnewl()`

> Print a new line (0xa and 0xd or LF+CR).

`char blocking_read()`

> Block until there is new data from UART1. The data will be returned as char type.

`int read()`

> Non-blocking read. This function returns -1 if there is no data.

`int readline(char* buf, int size)`

> Read and echo back characters and block until a line is read. The line is terminated either by the provided size or if a carriage return character has been encountered. The function will populate the provided buffer and return the number of characters that were read.

`void wio_readline()`

> Like readline, but use input\_buf, input\_ptr, and limit to 80 characters.

## String Routines ##

`char str_cmp(char* str1, char* str2, int len)`

> Compare two strings up to the specified length. Returns 1 if it's a match, 0 otherwise. Warning, there is no length checking!

`char parse(char* cmd, char opr_type)`

> Parse the string buffer `input_buf` for `cmd` and its operand. This routine also uses `input_ptr` to detect the end of the buffer. `input_buf`, `input_ptr`, and operand types are defined in `wio.h`. See [the Terminal Input Processor](TerminalInputProcessor.md) page to see how to use this routine to create your own terminal. This routine will return a 1 if the command string and the operand type match the input line. It will return 0 otherwise.

## Parse ASCII String and Return Literal Values ##

`char parse_ascii_hex(char ascii)`

> Parse an ASCII character that is representing a hex digit and return its literal value. If the character is not a valid hex digit, the routine will return 0 (no validation).

`char parse_ascii_hex_byte(char* string, int start)`

> Parse a byte represented as 2 ASCII hex digits and return its literal value as a char. Arguments are the string pointer, and the index to the first hex digit (highest nybble). Warning, there is no length checking!

`int parse_ascii_hex_32(char* string, int start)`

> Parse a 32-bit word represented as 8 ASCII hex digits and return its literal value as an int. Arguments are the string pointer, and the index to the first hex digit (highest nybble). Warning, there is no length checking!

`char parse_ascii_bin(char* string, int index)`

> Parse a 1-bit value represented as ASCII '0' or '1' and return its literal value as a char. Arguments are the string pointer and the index of the character. Warning, there is no length checking!

`char parse_ascii_bin_8(char* string, int start)`

> Parse a 8-bit value represented as ASCII '0' or '1' and return its literal value as a char. Arguments are the string pointer and the index of the first character. Warning, there is no length checking!


`int parse_ascii_decimal(char* string, char start, char len)`

> Parse the specified substring from a character array as a decimal of arbitrary number of digits and return it as an int. Warning, there is no length checking!

## Get ASCII representation of a value ##

`char ascii_byte_h(char val)`

> Return an ASCII representation of the high nybble of an 8-bit value

`char ascii_byte_l(char val)`

> Return an ASCII representation of the low nybble of an 8-bit value

`char ascii_hex_word(char buf[9], int val)`

> Populate the 9-character buffer with the ASCII representation of a 32-bit value. The last character in the buffer will be filled with the nul character (0), so the buffer can be used directly by string routines such as the print function.