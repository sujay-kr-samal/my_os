#ifndef MOUSE_H
#define MOUSE_H
#include <stdint.h>


void  mouse_init(void);
void  mouse_handler(void);
int   mouse_x(void);
int   mouse_y(void);
int   mouse_left(void);
int   mouse_right(void);

#endif
