/* 
 * File:   wio.h
 * Author: Wira
 *
 * Created on October 30, 2014, 12:31 PM
 */

#ifndef WIO_H
#define	WIO_H

#ifdef	__cplusplus
extern "C" {
#endif

void init_uart1(int, int);
void init_uart2(int, int);
void print(char*);
void psstr(char*, int, int);
void pchar(char);
char blocking_read(void);
char str_cmp(char*, char*, int);
char parse_ascii_hex_byte(char*, int);
int parse_ascii_hex_word(char*, int);
int parse_ascii_hex_32(char*, int);
char ascii_byte_h(char);
char ascii_byte_l(char);
char parse_ascii_hex(char);
char ascii_nybble(char);
void ascii_hex_word(char[], int);
void uart_forward(void);
void u2_set_baud(int);
char u2_blocking_read(void);
char u2_read_print(void);
void u2_write(char);

#ifdef	__cplusplus
}
#endif

#endif	/* WTERMIO_H */

