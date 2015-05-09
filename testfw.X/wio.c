/*
    Copyright 2014-2015 David Fritz, Brian Gordon, Wira Mulia

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */

// Wira's terminal I/O stuff source code

// Include your processor header here
#include <xc.h>
#include <proc/p32mx270f256b.h>
#include "wio.h"
#include "vt.h"

#define UART_TIMEOUT    7200000L

void init_uart1(int pbclock, int baud) {
    U1MODEbits.ON = 1;          // on
    U1MODEbits.UEN = 0b00;      // rx/tx pins only
    U1MODEbits.BRGH = 1;        // brgh mode
    U1MODEbits.PDSEL = 0b00;    // 8-bit data, no parity
    U1MODEbits.STSEL = 0;       // 1 stop bit
    U1STAbits.URXEN = 1;        // receive enable
    U1STAbits.UTXEN = 1;        // transmit enable
    U1BRG = (pbclock/baud)/4 - 1;

    // default terminal options
    WIO_EMULATE_VT = 0;
    WIO_BACKSPACE_SUPPORT = 0;
    VT_HISTORY_SET = 0;
}

void init_uart2(int pbclock, int baud) {
    U2MODEbits.ON = 1;          // on
    U2MODEbits.UEN = 0b00;      // rx/tx pins only
    U2MODEbits.BRGH = 1;        // brgh mode
    U2MODEbits.PDSEL = 0b00;    // 8-bit data, no parity
    U2MODEbits.STSEL = 0;       // 1 stop bit
    U2STAbits.URXEN = 1;        // receive enable
    U2STAbits.UTXEN = 1;        // transmit enable
    U2BRG = (pbclock/baud)/4 - 1;
}

void print(char *buf) {
    int i = 0;
    while(buf[i] != 0) {
        while(!U1STAbits.TRMT);
        U1TXREG = buf[i];
        i++;
    }
}

void psstr(char *buf, int start, int len) {
    int i;
    for(i = start; i < start+len; i++) {
        while(!U1STAbits.TRMT);
        U1TXREG = buf[i];
    }
}

void pchar(char a) {
    while(!U1STAbits.TRMT);
    U1TXREG = a;
}

void pnewl() {
    pchar(0xa);
    pchar(0xd);
}

void print_buf(wio_line_buf *buf) {
    int i;
    for(i = 0; i < buf->size; i++) {
        pchar(buf->data[i]);
    }
}

char blocking_read() {
    if(U1STAbits.OERR) {
        U1STAbits.OERR = 0;
    }
    while(!U1STAbits.URXDA);
    return U1RXREG;
}

int read() {
    if(U1STAbits.OERR) {
        U1STAbits.OERR = 0;
    }
    if(U1STAbits.URXDA)
        return U1RXREG;
    else
        return -1;
}

int readline(char *line_buf, int size) {
    char buf;
    int nread = 0;

    while(1) {
        buf = blocking_read();
        if(nread == size || buf == CHAR_CR) {
            return nread;
        } else if(WIO_BACKSPACE_SUPPORT && buf == CHAR_BSPACE) {
            if(nread == 0) {
                continue;
            }
            pchar(CHAR_BSPACE);
            pchar(' ');
            pchar(CHAR_BSPACE);
            nread--;
        } else if(WIO_EMULATE_VT && buf == CHAR_ESC) {
            // handle vt escape sequences
            vt_buf = &wio_line;
            vt_escape(&nread);
        } else {
            pchar(buf);
            line_buf[nread++] = buf;
        }
    }
}

void wio_readline() {
    wio_line.size = readline((char*)&wio_line.data, 80);
    if(WIO_EMULATE_VT) {
        buf_cpy(&wio_line, &vt_last_line);
        VT_HISTORY_SET = 1;
        VT_LOOKUP_SET = 0;
    }
}

char str_cmp(char* str1, char* str2, int len) {
    int i;
    for(i = 0; i < len; i++) {
        if(str1[i] != str2[i]) {
            return 0;
        }
    }
    return 1;
}

void buf_cpy(wio_line_buf *src, wio_line_buf *dst) {
    int i;
    for(i = 0; i < src->size; i++) {
        dst->data[i] = src->data[i];
    }
    dst->size = src->size;
}

char parse_ascii_hex(char ascii) {
    if(ascii >= 'a' && ascii <= 'f') {
        return ascii - 'a' + 10;
    } else if(ascii >= '0' && ascii <= '9') {
        return ascii - '0';
    } else {
        return 0;
    }
}

// parse 1-byte hex value in ascii to its literal value
char parse_ascii_hex_byte(char* string, int start) {
    char high = parse_ascii_hex(string[start]);
    char low = parse_ascii_hex(string[start+1]);

    return (high<<4) | low;
}

// parse 4-byte hex value in ascii to its literal value
int parse_ascii_hex_32(char* string, int start) {
    char buf;
    int val = 0;
    int i;
    for(i = 0; i < 8; i++) {
        buf = parse_ascii_hex(string[start+i]);
        val |= (buf << 4*(7-i));
    }
    return val;
}

// parse 1-bit hex value in ascii to its literal value
char parse_ascii_bin(char* string, int index) {
    return string[index] == '1' ? 1 : 0;
}

char parse_ascii_bin_8(char* string, int start) {
    char val = 0;
    int i;
    for(i = 0; i < 8; i++) {
        if(string[start+i] == '1') {
            val |= (1<<(7-i));
        }
    }
    return val;
}

char ascii_byte_h(char val) {
    val = (val & 0xf0) >> 4;
    if(val < 10) {
        return val + '0';
    } else {
        return val - 10 + 'a';
    }
}

char ascii_byte_l(char val) {
    val &= 0xf;
    if(val < 10) {
        return val + '0';
    } else {
        return val - 10 + 'a';
    }
}

char ascii_nybble(char val) {
    if(val < 10) {
        return val + '0';
    } else {
        return val - 10 + 'a';
    }
}

void print_ascii_byte(char val) {
    pchar(ascii_byte_h(val));
    pchar(ascii_byte_l(val));
}

void ascii_hex_word(char buf[], int val) {
    buf[0] = ascii_nybble((val & 0xf0000000) >> 28);
    buf[1] = ascii_nybble((val & 0x0f000000) >> 24);
    buf[2] = ascii_nybble((val & 0x00f00000) >> 20);
    buf[3] = ascii_nybble((val & 0x000f0000) >> 16);
    buf[4] = ascii_nybble((val & 0x0000f000) >> 12);
    buf[5] = ascii_nybble((val & 0x00000f00) >> 8);
    buf[6] = ascii_nybble((val & 0x000000f0) >> 4);
    buf[7] = ascii_nybble((val & 0x0000000f));
    buf[8] = 0; // nul
}

void uart_forward(void) {
    while(1) {
        if(U1STAbits.OERR) {
            U1STAbits.OERR = 0;
        }
        if(U2STAbits.OERR) {
            U2STAbits.OERR = 0;
        }
        if(U2STAbits.URXDA) {
            while(!U1STAbits.TRMT);
            U1TXREG = U2RXREG;
        }
        if(U1STAbits.URXDA) {
            while(!U2STAbits.TRMT);
            U2TXREG = U1RXREG;
        }
    }
}

int parse_ascii_decimal(char* buf, char start, char len) {
    // big-endian, lower index digits have greater magnitudes
    unsigned char i = 0, j = 0;
    unsigned int val = 0;
    unsigned int mag;
    for(i = start; i < start+len; i++) {
        mag = 1;
        for(j = (len-1) - (i-start); j > 0; j--) {
            mag *= 10;
        }
        val += ((buf[i] - '0') * mag);
    }

    return val;
}

void u2_set_baud(int val_int) {
    char wordbuf[9];
    U2BRG = (48000000/val_int)/4 - 1;
    print("New baud: ");
    ascii_hex_word(wordbuf, val_int);
    print(wordbuf);
    pnewl();
    print("brg value: ");
    ascii_hex_word(wordbuf, U2BRG);
    print(wordbuf);
    pnewl();
}

char u2_blocking_read() {
    if(U2STAbits.OERR) {
        U2STAbits.OERR = 0;
    }
    while(!U2STAbits.URXDA);
    return U2RXREG;
}

void u2_read_print() {
    int timeout_count = 0;
    char val;
    if(U2STAbits.OERR) {
        U2STAbits.OERR = 0;
    }
    while(timeout_count < UART_TIMEOUT && !U2STAbits.URXDA) {
        timeout_count++;
    }
    if(U2STAbits.URXDA) {
        val = U2RXREG;
        pchar(ascii_byte_h(val));
        pchar(ascii_byte_l(val));
    } else {
        print("UART receive timeout");
        pnewl();
    }
}

void u2_write(char a) {
    while(!U2STAbits.TRMT);
    U2TXREG = a;
}

char parse(char* cmd, char opr_type) {
    int i  = 0;
    char input_ptr = wio_line.size;
    char buf;
    int cmd_len = 0;
    while(cmd[cmd_len] != 0) {
        cmd_len++;
    }

    while((buf = cmd[i]) != 0 && i < input_ptr) {
        if(wio_line.data[i] != buf) {
            return 0;
        }
        i++;
    }

    if(opr_type == OPR_NONE && cmd_len != input_ptr) {
        return 0;
    }

    else if((opr_type == OPR_HEX32 || opr_type == OPR_BIN8) && (cmd_len+1+8) != input_ptr) {
        return 0;
    }

    else if(opr_type == OPR_HEX8 && (cmd_len+1+2) != input_ptr) {
        return 0;
    }

    else if(opr_type == OPR_BIN && (cmd_len+1+1) != input_ptr) {
        return 0;
    }

    else if(opr_type == OPR_DEC6 && (cmd_len+1+6) != input_ptr) {
        return 0;
    }

    else if((opr_type == OPR_RANGE || opr_type == OPR_ADBIT) && (cmd_len+1+8+1+8) != input_ptr) {
        return 0;
    }

    else if(opr_type == OPR_ADVAL && (cmd_len+1+8+1+2) != input_ptr) {
        return 0;
    }

    if(opr_type == OPR_HEX32 || opr_type == OPR_HEX8) {
        if(wio_line.data[i] != ' ') {
            return 0;
        }
        for(i=i+1; i < input_ptr; i++) {
            if(!((wio_line.data[i] >= '0' && wio_line.data[i] <= '9') ||
               (wio_line.data[i] >= 'a' && wio_line.data[i] <= 'f'))) {
                return 0;
            }
        }

        if(opr_type == OPR_HEX32) {
            wio_opr_int1 = parse_ascii_hex_32(wio_line.data, cmd_len+1);
        } else {
            wio_opr_char = parse_ascii_hex_byte(wio_line.data, cmd_len+1);
        }
    }

    else if(opr_type == OPR_RANGE) {
        if(wio_line.data[i] != ' ' || wio_line.data[input_ptr-9] != ' ') {
            return 0;
        }
        for(i=i+1; i < input_ptr-9; i++) {
            if(!((wio_line.data[i] >= '0' && wio_line.data[i] <= '9') ||
               (wio_line.data[i] >= 'a' && wio_line.data[i] <= 'f'))) {
                return 0;
            }
        }

        for(i=input_ptr-8; i < input_ptr; i++) {
            if(!((wio_line.data[i] >= '0' && wio_line.data[i] <= '9') ||
               (wio_line.data[i] >= 'a' && wio_line.data[i] <= 'f'))) {
                return 0;
            }
        }

        wio_opr_int1 = parse_ascii_hex_32(wio_line.data, cmd_len+1);
        wio_opr_int2 = parse_ascii_hex_32(wio_line.data, cmd_len+1+9);
    }

    else if(opr_type == OPR_ADVAL) {
        if(wio_line.data[i] != ' ' || wio_line.data[input_ptr-3] != ' ') {
            return 0;
        }
        for(i=i+1; i < input_ptr-3; i++) {
            if(!((wio_line.data[i] >= '0' && wio_line.data[i] <= '9') ||
               (wio_line.data[i] >= 'a' && wio_line.data[i] <= 'f'))) {
                return 0;
            }
        }

        for(i=input_ptr-2; i < input_ptr; i++) {
            if(!((wio_line.data[i] >= '0' && wio_line.data[i] <= '9') ||
               (wio_line.data[i] >= 'a' && wio_line.data[i] <= 'f'))) {
                return 0;
            }
        }

        wio_opr_int1 = parse_ascii_hex_32(wio_line.data, cmd_len+1);
        wio_opr_char = parse_ascii_hex_byte(wio_line.data, cmd_len+1+9);
    }

    else if(opr_type == OPR_ADBIT) {
        if(wio_line.data[i] != ' ' || wio_line.data[input_ptr-9] != ' ') {
            return 0;
        }
        for(i=i+1; i < input_ptr-9; i++) {
            if(!((wio_line.data[i] >= '0' && wio_line.data[i] <= '9') ||
               (wio_line.data[i] >= 'a' && wio_line.data[i] <= 'f'))) {
                return 0;
            }
        }

        for(i=input_ptr-8; i < input_ptr; i++) {
            if(wio_line.data[i] != '0' && wio_line.data[i] != '1') {
                return 0;
            }
        }

        wio_opr_int1 = parse_ascii_hex_32(wio_line.data, cmd_len+1);
        wio_opr_char = parse_ascii_bin_8(wio_line.data, cmd_len+1+9);
    }

    else if(opr_type == OPR_DEC6) {
        if(wio_line.data[i] != ' ') {
            return 0;
        }
        for(i=i+1; i < input_ptr; i++) {
            if(!(wio_line.data[i] >= '0' && wio_line.data[i] <= '9')) {
                return 0;
            }
        }

        wio_opr_int1 = parse_ascii_decimal(wio_line.data, cmd_len+1, 6);
    }

    else if(opr_type == OPR_BIN || opr_type == OPR_BIN8) {
        if(wio_line.data[i] != ' ') {
            return 0;
        }
        for(i=i+1; i < input_ptr; i++) {
            if(wio_line.data[i] != '0' && wio_line.data[i] != '1') {
                return 0;
            }
        }

        if(opr_type == OPR_BIN) {
            wio_opr_char = parse_ascii_bin(wio_line.data, cmd_len+1);
        } else {
            wio_opr_char = parse_ascii_bin_8(wio_line.data, cmd_len+1);
        }
    }

    return 1;
}