#ifndef GUI_H
#define GUI_H
#include <stdint.h>

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 200
#define FRAMEBUF      ((volatile uint8_t*)0xA0000)

/* macOS-inspired color palette indices (VGA 256-color) */
#define COL_DESKTOP     1   /* dark blue-grey desktop */
#define COL_MENUBAR     7   /* light grey menu bar */
#define COL_MENUTEXT    0   /* black */
#define COL_TITLEBAR    4   /* blue title bar active */
#define COL_TITLEBAR_I  8   /* dark grey inactive */
#define COL_TITLETEXT   15  /* white */
#define COL_WINBG       7   /* light grey window body */
#define COL_WINSHADOW   8   /* dark shadow */
#define COL_BORDER      8   /* dark border */
#define COL_CLOSE       4   /* red close button */
#define COL_TERMINAL_BG 0   /* black terminal bg */
#define COL_TERMINAL_FG 2   /* green terminal text */
#define COL_DOCK_BG     8   /* dark dock */
#define COL_DOCK_ITEM   7   /* dock icon bg */
#define COL_WHITE       15
#define COL_BLACK       0
#define COL_CURSOR      15  /* white cursor */

#define MAX_WINDOWS     4
#define TITLEBAR_H      12
#define DOCK_H          24
#define MENUBAR_H       10

typedef struct {
    int      x, y, w, h;
    int      visible;
    int      dragging;
    int      drag_ox, drag_oy;
    char     title[32];
    /* terminal state */
    char     lines[16][40];  /* 16 lines of 40 chars */
    int      cur_line;
    int      cur_col;
    char     input_buf[64];
    int      input_len;
} window_t;

void gui_init(void);
void gui_run(void);   /* never returns - main GUI loop */

/* Drawing primitives */
void gui_pixel(int x, int y, uint8_t c);
void gui_rect(int x, int y, int w, int h, uint8_t c);
void gui_rect_outline(int x, int y, int w, int h, uint8_t c);
void gui_char(int x, int y, char ch, uint8_t fg, uint8_t bg);
void gui_str(int x, int y, const char* s, uint8_t fg, uint8_t bg);

#endif
