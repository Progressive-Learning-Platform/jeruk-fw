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
 * File:   envy.h
 * Author: Wira
 *
 * Created on November 9, 2014, 8:08 PM
 */

#ifndef ENVY_H
#define	ENVY_H

#ifdef	__cplusplus
extern "C" {
#endif

unsigned int NVMUnlock (unsigned int);
unsigned int envy_write_word (void*, unsigned int);
unsigned int envy_write_row(void*, void*);
unsigned int envy_erase_page(void*);
void envy_write_stream(char, unsigned int);
void envy(void);
void envy_clear(void);

#ifdef	__cplusplus
}
#endif

#endif	/* ENVY_H */

