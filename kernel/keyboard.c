#include "keyboard.h"
#include "pic.h"
#include <stdint.h>

static const char sc_normal[] = {
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,
    '*',0,' '
};
static const char sc_shift[] = {
    0,0,'!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,
    '*',0,' '
};

static uint8_t  shift = 0, caps = 0;
static char     kb_buf[KEYBOARD_BUFFER_SIZE];
static uint8_t  kb_read = 0, kb_write = 0;

static void push(char c) {
    uint8_t next = (kb_write + 1) % KEYBOARD_BUFFER_SIZE;
    if (next != kb_read) { kb_buf[kb_write] = c; kb_write = next; }
}

void keyboard_handler(void) {
    uint8_t sc = inb(KEYBOARD_DATA_PORT);
    if (sc & 0x80) {
        uint8_t r = sc & 0x7F;
        if (r == 0x2A || r == 0x36) shift = 0;
    } else {
        if (sc == 0x2A || sc == 0x36) { shift = 1; goto eoi; }
        if (sc == 0x3A) { caps = !caps; goto eoi; }
        if (sc < sizeof(sc_normal)) {
            char c = (shift ^ caps) ? sc_shift[sc] : sc_normal[sc];
            if (c) push(c);
        }
    }
eoi:
    pic_send_eoi(IRQ_KEYBOARD);
}
void keyboard_init(void) { kb_read = 0; kb_write = 0; }
char keyboard_getchar(void) {
    while (kb_read == kb_write) asm volatile("hlt");
    char c = kb_buf[kb_read];
    kb_read = (kb_read + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}
int  keyboard_has_input(void) { return kb_read != kb_write; }
char keyboard_read(void) {
    if (kb_read == kb_write) return 0;
    char c = kb_buf[kb_read];
    kb_read = (kb_read + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}
