/*
    Copyright 2015 David Fritz, Brian Gordon, Wira Mulia

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
#include "vt.h"

#define VT_SCAN     0
#define VT_NUM      1
#define VT_CMD      2
#define VT_DONE     3

void vt_delete(int num) {
    int i;
    for(i = 0; i < num; i++) {
        pchar(CHAR_BSPACE);
        pchar(' ');
        pchar(CHAR_BSPACE);
    }
}

void vt_escape(int* cursor) {
    int state = VT_SCAN;
    char buf;
    while(state != VT_DONE) {
        buf = blocking_read();
        if(state == VT_SCAN && buf == '[') {
            state++;

        // ^A
        } else if(state == VT_NUM && !VT_LOOKUP_SET && VT_HISTORY_SET && buf == 'A') {
            vt_buf->size = *cursor;
            vt_delete(vt_buf->size);
            buf_cpy(vt_buf, &vt_temp_line);
            buf_cpy(&vt_last_line, vt_buf);
            print_buf(vt_buf);
            *cursor = vt_buf->size;
            VT_LOOKUP_SET = 1;
            state = VT_DONE;

        // ^B
        } else if(state == VT_NUM && VT_LOOKUP_SET && buf == 'B') {
            vt_buf->size = *cursor;
            vt_delete(vt_buf->size);
            buf_cpy(&vt_temp_line, vt_buf);
            print_buf(vt_buf);
            *cursor = vt_buf->size;
            VT_LOOKUP_SET = 0;
            state = VT_DONE;
        } else {
            state = VT_DONE;
        }
    }
}