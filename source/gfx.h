/* language: C (GNU C99), file: gfx.h, target: GBA / devkitARM */
#ifndef GFX_H
#define GFX_H
#include "gba_regs.h"

void gfx_init(void);
void gfx_clear(u16 color);
void gfx_pixel(int x, int y, u16 color);
void gfx_hline(int x0, int x1, int y, u16 color);
void gfx_vline(int x, int y0, int y1, u16 color);
void gfx_rect(int x, int y, int w, int h, u16 color);
void gfx_rect_outline(int x, int y, int w, int h, u16 color);
void gfx_fill_circle(int cx, int cy, int r, u16 color);
void gfx_circle_outline(int cx, int cy, int r, u16 color);
void gfx_line(int x0, int y0, int x1, int y1, u16 color);
void gfx_number(int x, int y, int value, int scale, u16 color);
void gfx_vsync(void);

#endif
