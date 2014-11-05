// Wira's terminal I/O stuff source code

#include <xc.h>
#include <proc/p32mx270f256b.h>

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

char blocking_read() {
    if(U1STAbits.OERR) {
        U1STAbits.OERR = 0;
    }
    while(!U1STAbits.URXDA);
    return U1RXREG;
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
// [0] is the high nibble, [1] is the low nibble
char parse_ascii_hex_byte(char ascii_h, char ascii_l) {
    char high = parse_ascii_hex(ascii_h);
    char low = parse_ascii_hex(ascii_l);

    return (high<<4) | low;
}

// parse 32-bit hex value in ascii to its literal value
int parse_ascii_hex_word(char ascii_7, char ascii_6, char ascii_5, char ascii_4,
        char ascii_3, char ascii_2, char ascii_1, char ascii_0) {
    char b7 = parse_ascii_hex(ascii_7);
    char b6 = parse_ascii_hex(ascii_6);
    char b5 = parse_ascii_hex(ascii_5);
    char b4 = parse_ascii_hex(ascii_4);
    char b3 = parse_ascii_hex(ascii_3);
    char b2 = parse_ascii_hex(ascii_2);
    char b1 = parse_ascii_hex(ascii_1);
    char b0 = parse_ascii_hex(ascii_0);

    return (b7 << 28) | (b6 << 24) | (b5 << 20) | (b4 << 16) |
           (b3 << 12) | (b2 << 8) | (b1 << 4) | (b0);
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