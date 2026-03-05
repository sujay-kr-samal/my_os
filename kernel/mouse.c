#include "mouse.h"
#include "pic.h"
#include <stdint.h>

#define MOUSE_STATUS  0x64
#define MOUSE_DATA    0x60
#define IRQ_MOUSE     12

static void mouse_wait_write(void) { int t=100000; while(t-- && (inb(MOUSE_STATUS)&2)); }
static void mouse_wait_read(void)  { int t=100000; while(t-- && !(inb(MOUSE_STATUS)&1)); }

static void mouse_write(uint8_t v) {
    mouse_wait_write(); outb(MOUSE_STATUS, 0xD4);
    mouse_wait_write(); outb(MOUSE_DATA, v);
}
static uint8_t mouse_read(void) {
    mouse_wait_read(); return inb(MOUSE_DATA);
}

static int     mx = 160, my = 100;
static int     mbtn = 0;
static uint8_t packet[3];
static int     pkt_idx = 0;

void mouse_init(void) {
    /* Enable aux device */
    mouse_wait_write(); outb(MOUSE_STATUS, 0xA8);

    /* Read + patch compaq status byte to enable IRQ12 */
    mouse_wait_write(); outb(MOUSE_STATUS, 0x20);
    mouse_wait_read();
    uint8_t status = (inb(MOUSE_DATA) | 2) & ~0x20; /* enable IRQ12, enable aux clock */
    mouse_wait_write(); outb(MOUSE_STATUS, 0x60);
    mouse_wait_write(); outb(MOUSE_DATA, status);

    /* Reset mouse to defaults */
    mouse_write(0xFF); mouse_read(); mouse_read(); mouse_read();

    /* Set sample rate to 40 samples/sec (less noise in QEMU) */
    mouse_write(0xF3); mouse_read();
    mouse_write(40);   mouse_read();

    /* Set resolution: 2 counts/mm */
    mouse_write(0xE8); mouse_read();
    mouse_write(0x01); mouse_read();

    /* Enable data reporting */
    mouse_write(0xF4); mouse_read();

    /* Unmask IRQ12 on slave PIC */
    outb(0xA1, inb(0xA1) & ~(1 << 4));
    /* Unmask IRQ2 (cascade) on master PIC */
    outb(0x21, inb(0x21) & ~(1 << 2));

    /* Discard any stale bytes in the buffer */
    for (int i = 0; i < 8; i++) {
        if (inb(MOUSE_STATUS) & 1) inb(MOUSE_DATA);
    }

    pkt_idx = 0;
}

void mouse_handler(void) {
    uint8_t data = inb(MOUSE_DATA);

    /* Packet sync: byte 0 always has bit 3 set.
     * If we're expecting byte 0 but bit 3 is clear, we're out of sync — discard. */
    if (pkt_idx == 0 && !(data & 0x08)) {
        pic_send_eoi(IRQ_MOUSE);
        return;
    }

    /* Overflow bits set = discard this packet */
    if (pkt_idx == 0 && (data & 0xC0)) {
        pic_send_eoi(IRQ_MOUSE);
        return;
    }

    packet[pkt_idx++] = data;

    if (pkt_idx == 3) {
        pkt_idx = 0;

        uint8_t flags = packet[0];
        int dx = (int)(int8_t)packet[1];
        int dy = (int)(int8_t)packet[2];

        /* Sign extension from status flags */
        if (flags & 0x10) dx |= ~0xFF;   /* X sign bit */
        if (flags & 0x20) dy |= ~0xFF;   /* Y sign bit */

        mbtn = flags & 0x07;   /* bits 0=left, 1=right, 2=middle */

        /* Scale down movement a bit for 320x200 */
        mx += dx / 2;
        my -= dy / 2;   /* screen Y is inverted vs mouse Y */

        if (mx < 0)           mx = 0;
        if (my < 0)           my = 0;
        if (mx >= 320) mx = 319;
        if (my >= 200) my = 199;
    }

    pic_send_eoi(IRQ_MOUSE);
}

int mouse_x(void)    { return mx; }
int mouse_y(void)    { return my; }
int mouse_left(void) { return mbtn & 1; }
int mouse_right(void){ return (mbtn >> 1) & 1; }
