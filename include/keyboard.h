#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdint.h>
#define KEYBOARD_DATA_PORT   0x60
#define KEYBOARD_BUFFER_SIZE 256
void keyboard_init(void);
void keyboard_handler(void);
char keyboard_getchar(void);
int  keyboard_has_input(void);
char keyboard_read(void);   /* non-blocking, returns 0 if empty */
#endif
