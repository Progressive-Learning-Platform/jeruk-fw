/*
    Copyright 2014 David Fritz, Brian Gordon, Wira Mulia

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
int readline(char*, int);
char str_cmp(char*, char*, int);
char parse_ascii_hex_byte(char*, int);
int parse_ascii_hex_32(char*, int);
char parse_ascii_bin(char*, int);
char parse_ascii_bin_8(char*, int);
char ascii_byte_h(char);
char ascii_byte_l(char);
void print_ascii_byte(char);
char parse_ascii_hex(char);
char ascii_nybble(char);
void ascii_hex_word(char[], int);
void uart_forward(void);
void u2_set_baud(int);
char u2_blocking_read(void);
void u2_read_print(void);
void u2_write(char);

#ifdef	__cplusplus
}
#endif

#endif	/* WTERMIO_H */

