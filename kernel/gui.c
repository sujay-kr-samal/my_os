#include "gui.h"
#include "mouse.h"
#include "font.h"
#include "keyboard.h"
#include "timer.h"
#include "vga.h"
#include "memory.h"
#include "proc.h"
#include <stdint.h>
#include <stddef.h>

/* Custom palette indices */
#define C_DESKTOP   16
#define C_PANEL     17
#define C_CARD      18
#define C_GOLD      19
#define C_DIMTXT    20
#define C_WINBG     21
#define C_TITLEBAR  22
#define C_BORDER    23
#define C_GREEN     24
#define C_BLUE      25
#define C_WHITE     15
#define C_BLACK     0
#define C_RED       4
#define C_GREY      7
#define C_DGREY     8
#define C_YELLOW    14

static inline void outb_g(uint16_t p, uint8_t v){asm volatile("outb %0,%1"::"a"(v),"Nd"(p));}

static void set_pal(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    outb_g(0x3C8, idx);
    outb_g(0x3C9, r>>2); outb_g(0x3C9, g>>2); outb_g(0x3C9, b>>2);
}

static void init_palette(void) {
    set_pal(C_DESKTOP,  0x1a,0x1f,0x2e);
    set_pal(C_PANEL,    0x22,0x28,0x38);
    set_pal(C_CARD,     0x2a,0x30,0x45);
    set_pal(C_GOLD,     0xc8,0xa8,0x4b);
    set_pal(C_DIMTXT,   0x88,0x92,0xa4);
    set_pal(C_WINBG,    0x1e,0x24,0x36);
    set_pal(C_TITLEBAR, 0x2d,0x35,0x50);
    set_pal(C_BORDER,   0x3d,0x45,0x60);
    set_pal(C_GREEN,    0x4e,0xc9,0x4e);
    set_pal(C_BLUE,     0x5b,0x9b,0xd5);
    set_pal(15,         0xff,0xff,0xff);
    set_pal(8,          0x44,0x4a,0x5a);
    set_pal(4,          0xe0,0x4a,0x4a);
    set_pal(14,         0xf0,0xc0,0x40);
}

/* Back buffer */
static uint8_t backbuf[SCREEN_HEIGHT][SCREEN_WIDTH];

static void blit(void) {
    volatile uint8_t* fb = FRAMEBUF;
    uint8_t* src = (uint8_t*)backbuf;
    for (int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++) fb[i]=src[i];
}

/* Primitives */
void gui_pixel(int x, int y, uint8_t c) {
    if(x<0||y<0||x>=SCREEN_WIDTH||y>=SCREEN_HEIGHT) return;
    backbuf[y][x]=c;
}
void gui_rect(int x, int y, int w, int h, uint8_t c) {
    if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;}
    for(int dy=0;dy<h;dy++) for(int dx=0;dx<w;dx++) gui_pixel(x+dx,y+dy,c);
}
void gui_rect_outline(int x,int y,int w,int h,uint8_t c){
    for(int dx=0;dx<w;dx++){gui_pixel(x+dx,y,c);gui_pixel(x+dx,y+h-1,c);}
    for(int dy=0;dy<h;dy++){gui_pixel(x,y+dy,c);gui_pixel(x+w-1,y+dy,c);}
}
static void gui_rrect(int x,int y,int w,int h,uint8_t c){
    gui_rect(x+2,y,w-4,h,c); gui_rect(x,y+2,w,h-4,c); gui_rect(x+1,y+1,w-2,h-2,c);
}
void gui_char(int x,int y,char ch,uint8_t fg,uint8_t bg){
    if(ch<32||ch>127)ch=' ';
    const uint8_t* g=font8x8[ch-32];
    for(int r=0;r<8;r++){uint8_t b=g[r];for(int c=0;c<8;c++){
        if(b&(0x80>>c))gui_pixel(x+c,y+r,fg);
        else if(bg!=255)gui_pixel(x+c,y+r,bg);
    }}
}
void gui_str(int x,int y,const char* s,uint8_t fg,uint8_t bg){
    while(*s){gui_char(x,y,*s++,fg,bg);x+=8;}
}
static int slen(const char* s){int n=0;while(s[n])n++;return n;}

static void gui_bar(int x,int y,int w,int h,uint32_t pct,uint8_t fill,uint8_t bg){
    gui_rect(x,y,w,h,bg);
    int fw=(int)(w*pct/100); if(fw>0)gui_rect(x,y,fw,h,fill);
}

static void itoa_s(uint64_t n,char* buf,int max){
    if(!n){buf[0]='0';buf[1]=0;return;}
    char t[20];int i=0;
    while(n&&i<19){t[i++]='0'+(n%10);n/=10;}
    int j=0;while(i-->0&&j<max-1)buf[j++]=t[i];buf[j]=0;
}

/* Windows */
static window_t windows[MAX_WINDOWS];
static int nwindows=0, focused=-1;

static void win_scroll(window_t* w){
    for(int i=0;i<15;i++)for(int j=0;j<40;j++)w->lines[i][j]=w->lines[i+1][j];
    for(int j=0;j<40;j++)w->lines[15][j]=0;
    if(w->cur_line>0)w->cur_line--;
}
static void win_putchar(window_t* w,char c){
    if(c=='\n'||w->cur_col>=38){w->cur_col=0;w->cur_line++;if(w->cur_line>=16)win_scroll(w);}
    if(c!='\n'){w->lines[w->cur_line][w->cur_col++]=c;w->lines[w->cur_line][w->cur_col]=0;}
}
static void win_print(window_t* w,const char* s){while(*s)win_putchar(w,*s++);}
static void win_int(window_t* w,uint64_t n){
    char buf[21];int i=20;buf[20]=0;
    if(!n){win_putchar(w,'0');return;}
    while(n){buf[--i]='0'+(n%10);n/=10;}
    win_print(w,buf+i);
}

static void draw_window(window_t* w, int idx){
    if(!w->visible)return;
    int x=w->x,y=w->y,ww=w->w,wh=w->h;
    int focused_win=(idx==focused);

    /* Shadow */
    gui_rect(x+2,y+2,ww,wh,C_BLACK);

    /* Body */
    gui_rect(x,y,ww,wh,C_WINBG);

    /* Titlebar */
    uint8_t tb=focused_win?C_TITLEBAR:C_PANEL;
    gui_rect(x,y,ww,TITLEBAR_H,tb);
    if(focused_win) gui_rect(x,y+TITLEBAR_H-1,ww,1,C_BLUE);

    /* Traffic light dots */
    gui_rrect(x+4, y+3,7,7,C_RED);
    gui_rrect(x+13,y+3,7,7,C_YELLOW);
    gui_rrect(x+22,y+3,7,7,C_GREEN);

    gui_str(x+33,y+2,w->title,C_WHITE,tb);
    gui_rect_outline(x,y,ww,wh,C_BORDER);

    /* Content */
    int body_y=y+TITLEBAR_H+1;
    gui_rect(x+1,body_y,ww-2,wh-TITLEBAR_H-2,C_WINBG);

    int cx=x+4;
    int content_top=body_y+2;
    int content_bot=y+wh-2;
    int input_y=content_bot-10;
    int max_lines=(input_y-content_top-2)/9;
    if(max_lines>16)max_lines=16;
    if(max_lines<1)max_lines=1;

    gui_rect(x+2,input_y-2,ww-4,1,C_BORDER);

    int first=w->cur_line+1-max_lines;
    if(first<0)first=0;
    for(int i=0;i<max_lines;i++){
        int li=first+i;
        if(li<0||li>15)continue;
        if(w->lines[li][0])
            gui_str(cx,content_top+i*9,w->lines[li],C_GREY,C_WINBG);
    }

    /* Prompt */
    gui_str(cx,      input_y,"myos",C_GREEN, C_WINBG);
    gui_str(cx+32,   input_y,"> ",  C_WHITE, C_WINBG);
    gui_str(cx+48,   input_y,w->input_buf,C_WHITE,C_WINBG);
    int cur_x=cx+48+w->input_len*8;
    if((timer_get_ticks()/50)%2==0) gui_rect(cur_x,input_y,2,8,C_WHITE);
}

/* Widgets */
static void draw_widget_card(int x,int y,int w,int h){
    gui_rrect(x,y,w,h,C_CARD);
    gui_rect_outline(x+1,y+1,w-2,h-2,C_BORDER);
}

static void draw_clock_widget(void){
    int x=4,y=14,w=72,h=28;
    draw_widget_card(x,y,w,h);
    uint64_t secs=timer_get_ticks()/100;
    uint32_t mm=(uint32_t)(secs/60)%60,ss=(uint32_t)(secs%60);
    char buf[8];
    buf[0]='0'+mm/10;buf[1]='0'+mm%10;buf[2]=':';
    buf[3]='0'+ss/10;buf[4]='0'+ss%10;buf[5]=0;
    gui_str(x+4,y+4,buf,C_GOLD,C_CARD);
    gui_str(x+4,y+18,"Uptime",C_DIMTXT,C_CARD);
}

static void draw_stats_widget(void){
    int x=4,y=46,w=72,h=40;
    draw_widget_card(x,y,w,h);
    extern mem_stats_t mem_stats;
    uint32_t used=mem_stats.total_pages-mem_stats.free_pages;
    uint32_t total=mem_stats.total_pages;
    uint32_t pct=total?(used*100/total):0;

    gui_str(x+4,y+3,"RAM",C_WHITE,C_CARD);
    char pb[8]; itoa_s(pct,pb,8);
    int pl=slen(pb); pb[pl]='%'; pb[pl+1]=0;
    gui_str(x+w-pl*8-12,y+3,pb,C_DIMTXT,C_CARD);
    gui_bar(x+4,y+13,w-8,4,pct,C_BLUE,C_BORDER);

    gui_str(x+4,y+21,"Threads",C_WHITE,C_CARD);
    char tb2[8]; itoa_s((uint64_t)nwindows,tb2,8);
    gui_str(x+w-16,y+21,tb2,C_DIMTXT,C_CARD);
    gui_bar(x+4,y+31,w-8,4,(uint32_t)(nwindows*25),C_GREEN,C_BORDER);
}

static void draw_info_widget(void){
    int x=4,y=90,w=72,h=20;
    draw_widget_card(x,y,w,h);
    gui_str(x+4,y+3, "MyOS v0.6", C_WHITE,  C_CARD);
    gui_str(x+4,y+12,"Phase 6",   C_DIMTXT, C_CARD);
}

/* Taskbar */
#define TASKBAR_H 16
#define TASKBAR_Y (SCREEN_HEIGHT-TASKBAR_H)

static void draw_taskbar(void){
    gui_rect(0,TASKBAR_Y,SCREEN_WIDTH,TASKBAR_H,C_PANEL);
    gui_rect(0,TASKBAR_Y,SCREEN_WIDTH,1,C_BORDER);

    /* Menu button */
    gui_rrect(2,TASKBAR_Y+2,26,12,C_CARD);
    gui_str(5,TASKBAR_Y+4,"Menu",C_DIMTXT,C_CARD);

    /* Terminal icon - centered */
    int ix=SCREEN_WIDTH/2-7;
    uint8_t ibg=(nwindows>0)?C_BLUE:C_CARD;
    gui_rrect(ix,TASKBAR_Y+2,14,12,ibg);
    gui_char(ix+3,TASKBAR_Y+4,'>',C_GREEN,ibg);

    /* Clock */
    uint64_t secs=timer_get_ticks()/100;
    char tbuf[8];
    uint32_t mm=(uint32_t)(secs/60)%60,ss=(uint32_t)(secs%60);
    tbuf[0]='0'+mm/10;tbuf[1]='0'+mm%10;tbuf[2]=':';
    tbuf[3]='0'+ss/10;tbuf[4]='0'+ss%10;tbuf[5]=0;
    gui_str(SCREEN_WIDTH-42,TASKBAR_Y+4,tbuf,C_DIMTXT,C_PANEL);
}

/* Desktop */
static void draw_desktop(void){
    gui_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT-TASKBAR_H,C_DESKTOP);
    draw_clock_widget();
    draw_stats_widget();
    draw_info_widget();
}

/* Cursor */
static void draw_cursor(int mx,int my){
    static const uint8_t cur[9][6]={
        {1,0,0,0,0,0},{1,1,0,0,0,0},{1,2,1,0,0,0},
        {1,2,2,1,0,0},{1,2,2,2,1,0},{1,2,2,1,0,0},
        {1,1,2,1,0,0},{0,0,1,2,1,0},{0,0,0,1,0,0},
    };
    for(int y=0;y<9;y++)for(int x=0;x<6;x++){
        if(cur[y][x]==1)gui_pixel(mx+x,my+y,C_WHITE);
        else if(cur[y][x]==2)gui_pixel(mx+x,my+y,C_BLACK);
    }
}

/* Commands */
static int streq(const char* a,const char* b){while(*a&&*b)if(*a++!=*b++)return 0;return *a==*b;}
static window_t* term_win=0;
static void tw(const char* s){if(term_win)win_print(term_win,s);}
static void twi(uint64_t n){if(term_win)win_int(term_win,n);}

static void process_command(window_t* w,const char* cmd){
    term_win=w; win_print(w,"\n");
    if(streq(cmd,"help")){
        tw("Commands:\n");
        tw(" help  clear  about\n");
        tw(" mem   uptime ps\n");
        tw(" spawn exit\n");
    } else if(streq(cmd,"clear")){
        for(int i=0;i<16;i++)for(int j=0;j<40;j++)w->lines[i][j]=0;
        w->cur_line=0;w->cur_col=0;
    } else if(streq(cmd,"about")){
        tw("MyOS v0.6 64-bit\n");
        tw("Dark theme GUI\n");
        tw("Phase 6 complete\n");
    } else if(streq(cmd,"uptime")){
        tw("Up: ");twi(timer_get_ticks()/100);tw("s\n");
    } else if(streq(cmd,"mem")){
        extern mem_stats_t mem_stats;
        tw("Free: ");twi((uint64_t)mem_stats.free_pages*4);tw("KB\n");
        tw("Used: ");twi((uint64_t)(mem_stats.total_pages-mem_stats.free_pages)*4);tw("KB\n");
    } else if(streq(cmd,"ps")){
        tw("Windows: ");twi(nwindows);tw("\n");
    } else if(streq(cmd,"exit")){
        win_print(w,"Bye!\n");
        w->visible=0; focused=-1;
        for(int j=0;j<nwindows;j++)if(&windows[j]==w){
            for(int k=j;k<nwindows-1;k++)windows[k]=windows[k+1];
            nwindows--;break;
        }
    } else if(cmd[0]){
        tw("cmd not found: ");tw(cmd);tw("\n");
    }
    term_win=0;
}

/* Input */
static int prev_btn=0;

static void open_terminal(void){
    if(nwindows>=MAX_WINDOWS)return;
    window_t* w=&windows[nwindows++];
    w->x=50;w->y=10;w->w=210;w->h=145;
    w->visible=1;w->dragging=0;
    const char* t="Terminal";
    for(int i=0;i<32;i++)w->title[i]=0;
    for(int i=0;t[i];i++)w->title[i]=t[i];
    for(int i=0;i<16;i++)for(int j=0;j<40;j++)w->lines[i][j]=0;
    w->cur_line=0;w->cur_col=0;w->input_len=0;w->input_buf[0]=0;
    win_print(w,"MyOS Terminal v0.6\n");
    win_print(w,"Type 'help'\n");
    focused=nwindows-1;
}

static void handle_mouse(void){
    int mx=mouse_x(),my=mouse_y();
    int btn=mouse_left(),clicked=btn&&!prev_btn;

    if(clicked){
        if(my>=TASKBAR_Y){
            int ix=SCREEN_WIDTH/2-7;
            if(mx>=ix&&mx<ix+14){
                if(nwindows==0)open_terminal();
                else{focused=0;windows[0].visible=1;}
            }
            prev_btn=btn;return;
        }
        for(int i=nwindows-1;i>=0;i--){
            window_t* w=&windows[i];
            if(!w->visible)continue;
            if(mx>=w->x+4&&mx<w->x+11&&my>=w->y+3&&my<w->y+10){
                w->visible=0;if(focused==i)focused=-1;
                for(int j=i;j<nwindows-1;j++)windows[j]=windows[j+1];
                nwindows--;break;
            }
            if(mx>=w->x&&mx<w->x+w->w&&my>=w->y&&my<w->y+TITLEBAR_H){
                focused=i;w->dragging=1;w->drag_ox=mx-w->x;w->drag_oy=my-w->y;break;
            }
            if(mx>=w->x&&mx<w->x+w->w&&my>=w->y&&my<w->y+w->h){focused=i;break;}
        }
    }

    if(btn){
        for(int i=0;i<nwindows;i++){
            window_t* w=&windows[i];
            if(!w->dragging)continue;
            w->x=mx-w->drag_ox;w->y=my-w->drag_oy;
            if(w->x<0)w->x=0;if(w->y<0)w->y=0;
            if(w->x+w->w>SCREEN_WIDTH)w->x=SCREEN_WIDTH-w->w;
            if(w->y+w->h>TASKBAR_Y)w->y=TASKBAR_Y-w->h;
            break;
        }
    } else {
        for(int i=0;i<nwindows;i++)windows[i].dragging=0;
    }
    prev_btn=btn;
}

static void handle_keyboard(void){
    if(!keyboard_has_input())return;
    char c=keyboard_read();if(!c)return;
    if(focused<0||focused>=nwindows)return;
    window_t* w=&windows[focused];
    if(!w->visible)return;
    if(c=='\n'){w->input_buf[w->input_len]=0;process_command(w,w->input_buf);w->input_len=0;w->input_buf[0]=0;}
    else if(c=='\b'){if(w->input_len>0){w->input_len--;w->input_buf[w->input_len]=0;}}
    else if(c>=32&&c<127&&w->input_len<38){w->input_buf[w->input_len++]=c;w->input_buf[w->input_len]=0;}
}

void gui_init(void){
    init_palette();
    nwindows=0;focused=-1;
    mouse_init();
}

void gui_run(void){
    for(;;){
        handle_mouse();
        handle_keyboard();
        draw_desktop();
        for(int i=0;i<nwindows;i++)if(i!=focused)draw_window(&windows[i],i);
        if(focused>=0&&focused<nwindows)draw_window(&windows[focused],focused);
        draw_taskbar();
        draw_cursor(mouse_x(),mouse_y());
        blit();
        uint64_t t=timer_get_ticks();
        while(timer_get_ticks()<t+3)asm volatile("hlt");
    }
}
