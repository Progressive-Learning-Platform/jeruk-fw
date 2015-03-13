The terminal input processor resides in **main.c** and **wio.c** and is comprised of three parts:
  * the line buffering loop
  * the line processor
  * and the command parser

# Line Buffering #

```
while(1) {
    wio_readline();
    process_input();
}
```

The terminal is an infinite while loop that waits for an input from UART1 (blocking read). The `wio_readline` routine from `wio.c` is used. The terminal has an 80 character buffer (`input_buf`) and an input index (`input_ptr`) defined in `wio.h`. Whenever the terminal receives a character other than carriage return (`0x0d`), it will store this character in the buffer, echo the character back, and increment `input_ptr`. The terminal will call the line processor when either the line buffer is full or the carriage return character is encountered (`0x0d`). `input_buf` array and `input_ptr` are **global** in `wio.h` and should not be written to by any other part of the firmware other than the `wio_readline` routine! You can provide your own buffer, pointer index, and a different character limit by using the `readline` function. You will have to write your parser if this is the case.

# Line Processor #

The line processor routine '`process_input()`' in `main.c` is where all supported terminal commands are checked against the latest input line. The routine calls the command parser for every possible terminal command, and if there is a match between the input from user and the supported command, it will then hand off control to the command routine. For example, the following is a snippet of the line processor where it handles either the '`rbyte`' or the '`range`' commands.

```
    else if(parse("rbyte",  OPR_HEX32))  cmd_rbyte();
    else if(parse("range",  OPR_RANGE))  cmd_range();
```

If the input line from the user matches '`rbyte`' and its 32-bit ASCII hex operand, the `cmd_rbyte()` routine will be called and the line processor will return. **To support new commands for the terminal**, add another '`else if`' entry, call the parser and pass it the command token that you want to match and the type of operand that it takes, and call your external routine. The next section describes the operand types that are supported by the parser.

```
    // example for adding a command
    else if(parse("gobble", OPR_HEX32))  gobble();
```

To extract the value of the operand for your routine to use, use the `parse_ascii_` functions from the [terminal I/O library](TerminalIO.md). For example, if your command takes a 32-bit word, you can use:

```
 parse_ascii_hex_32(input_buf, length_of_your_command+1)
```

E.g. if your command is `gobble`, you can use the following to extract the 32-bit operand and get an int value:

```
 int val = parse_ascii_hex_32(input_buf, 7);
```

# Command Parser #

The command parser of the terminal is the '`parse()`' routine in `wio.c`. It takes a command literal to match and the operand it expects. `parse()` uses `input_buf` and `input_ptr` from `wio.h` as inputs. The command and its operand (if there is one) must be separated by a single space. The parser returns 1 if the command literal matches, the length of the input is correct, and the operand is valid, the parser will return 0 otherwise. The following is a table of operand types that the command parser supports:

| OPR\_NONE | No operand, the parser only expects the command literal to match |
|:----------|:-----------------------------------------------------------------|
| OPR\_HEX32 | 32-bit operand in 8-digit hex ASCII, all lowercase (e.g. 'a0004000') |
| OPR\_HEX8 | 8-bit operand in 2-digit hex ASCII, all lowercase (e.g. '7f') |
| OPR\_DEC6 | 6-digit decimal (e.g. '115200' or '009600') |
| OPR\_RANGE | Two 32-bit operands in 8-digit hex ASCII, all lowercase, separated by a space (e.g. 'a0004000 a0004200')|
| OPR\_ADVAL | 32-bit address and 8-bit value pair, 8- and 2-digit hex ASCII, all lowercase, separated by a space (e.g. 'a0004000 7f') |
| OPR\_ADBIT | 32-bit address and 8-bit value pair, 8-digit hex ASCII and 8-bit binary ASCII, separate by a space (e.g. 'a0004020 00110101') |
| OPR\_BIN | 1-bit operand in binary ASCII (e.g. '0' or '1') |
| OPR\_BIN8 | 8-bit operand in binary ASCII (e.g. '00001101') |

# Using `readline()` and `parse()` to create your own terminal #

To create a sub-terminal, you can include `wio.h` and implement a line-oriented terminal like the one in `main.c`. Here's an example code:

```
#include "wio.h"

void myterminal() {
   unsigned int operand;
   char stop = 0;

   while(!stop) {
      print("myterm> ");
      wio_readline();

      if parse("quit", OPR_NONE) {
         stop = 1;                                   // quit this terminal
      } else if parse("work", OPR_HEX32) {
         operand = parse_ascii_hex_32(input_buf, 5);
         work(operand);
      } else {
         print("Invalid command");                   // no match
      }
      pnewl();
   }
}
```