/* 
 * File:   procdefs.h
 * Author: wira
 *
 * Created on March 5, 2015, 12:04 PM
 */

#ifndef PROCDEFS_H
#define	PROCDEFS_H

#ifdef	__cplusplus
extern "C" {
#endif

#define IO_0 PORTBbits.RB4
#define IO_1 PORTBbits.RB7
#define IO_2 PORTBbits.RB8
#define IO_3 PORTBbits.RB9
#define IO_4 PORTBbits.RB10
#define IO_5 PORTBbits.RB11
#define IO_6 PORTBbits.RB13
#define IO_7 PORTBbits.RB15

#define BTN0 PORTBbits.RB0
#define BTN1 PORTBbits.RB1

#define SYSTEM_CLOCK    48000000
#define UART1_BAUD      57600
#define UART2_BAUD      9600

#define LEDS PORTA

#ifdef	__cplusplus
}
#endif

#endif	/* PROCDEFS_H */

