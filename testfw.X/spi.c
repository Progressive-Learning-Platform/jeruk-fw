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

#include "wio.h"

void spi_tx() {

}

void spi() {
    unsigned int operand;
    char stop = 0;
    char wordbuf[9];

    while(!stop) {
        print("spi> ");
        input_ptr = readline(input_buf, 80);
        pchar('\n');

             if(parse("quit", OPR_NONE))    { stop = 1;                        }
        else if(parse("tx", OPR_HEX32))     { spi_tx();                        }
        else {
            print("Invalid command.");
        }

        pchar('\n');
    }
}