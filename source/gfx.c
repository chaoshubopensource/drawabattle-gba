/* language: C (GNU C99), file: gfx.c, target: GBA / devkitARM
 * Desenho em Mode 3 (bitmap direto 240x160, 16bpp). Simples de entender e
 * suficiente para a quantidade de objetos deste jogo; para mais performance
 * o caminho normal seria migrar para sprites (OBJ) + tiles. */
#include "gfx.h"

static const u8 FONT3X5[10][5] = {
    {0x7,0x5,0x5,0x5,0x7}, /* 0 */
    {0x2,0x6,0x2,0x2,0x7}, /* 1 */
    {0x7,0x1,0x7,0x4,0x7}, /* 2 */
    {0x7,0x1,0x7,0x1,0x7}, /* 3 */
    {0x5,0x5,0x7,0x1,0x1}, /* 4 */
    {0x7,0x4,0x7,0x1,0x7}, /* 5 */
    {0x7,0x4,0x7,0x5,0x7}, /* 6 */
    {0x7,0x1,0x2,0x2,0x2}, /* 7 */
    {0x7,0x5,0x7,0x5,0x7}, /* 8 */
    {0x7,0x5,0x7,0x1,0x7}, /* 9 */
};

void gfx_init(void) {
    REG_DISPCNT = MODE3 | BG2_ENABLE;
}

void gfx_vsync(void) {
    while (REG_VCOUNT >= 160) {}
    while (REG_VCOUNT < 160) {}
}

void gfx_clear(u16 color) {
    static u16 fill;
    fill = color;
    REG_DMA3SAD = (u32)&fill;
    REG_DMA3DAD = (u32)VRAM;
    REG_DMA3CNT = (u32)(SCREEN_W * SCREEN_H) | ((DMA_SRC_FIXED | DMA_ENABLE16) << 16);
}

void gfx_pixel(int x, int y, u16 color) {
    if ((unsigned)x >= SCREEN_W || (unsigned)y >= SCREEN_H) return;
    VRAM[y * SCREEN_W + x] = color;
}

void gfx_hline(int x0, int x1, int y, u16 color) {
    if ((unsigned)y >= SCREEN_H) return;
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (x0 < 0) x0 = 0;
    if (x1 > SCREEN_W - 1) x1 = SCREEN_W - 1;
    for (int x = x0; x <= x1; x++) VRAM[y * SCREEN_W + x] = color;
}

void gfx_vline(int x, int y0, int y1, u16 color) {
    if ((unsigned)x >= SCREEN_W) return;
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    if (y0 < 0) y0 = 0;
    if (y1 > SCREEN_H - 1) y1 = SCREEN_H - 1;
    for (int y = y0; y <= y1; y++) VRAM[y * SCREEN_W + x] = color;
}

void gfx_rect(int x, int y, int w, int h, u16 color) {
    for (int row = 0; row < h; row++) gfx_hline(x, x + w - 1, y + row, color);
}

void gfx_rect_outline(int x, int y, int w, int h, u16 color) {
    gfx_hline(x, x + w - 1, y, color);
    gfx_hline(x, x + w - 1, y + h - 1, color);
    gfx_vline(x, y, y + h - 1, color);
    gfx_vline(x + w - 1, y, y + h - 1, color);
}

/* círculo preenchido, algoritmo do ponto médio (só inteiros, sem sqrt) */
void gfx_fill_circle(int cx, int cy, int r, u16 color) {
    int x = r, y = 0, err = 0;
    while (x >= y) {
        gfx_hline(cx - x, cx + x, cy + y, color);
        gfx_hline(cx - x, cx + x, cy - y, color);
        gfx_hline(cx - y, cx + y, cy + x, color);
        gfx_hline(cx - y, cx + y, cy - x, color);
        y++;
        err += 2 * y + 1;
        if (err > 0) { x--; err -= 2 * x + 1; }
    }
}

void gfx_circle_outline(int cx, int cy, int r, u16 color) {
    int x = r, y = 0, err = 0;
    while (x >= y) {
        gfx_pixel(cx + x, cy + y, color); gfx_pixel(cx - x, cy + y, color);
        gfx_pixel(cx + x, cy - y, color); gfx_pixel(cx - x, cy - y, color);
        gfx_pixel(cx + y, cy + x, color); gfx_pixel(cx - y, cy + x, color);
        gfx_pixel(cx + y, cy - x, color); gfx_pixel(cx - y, cy - x, color);
        y++;
        err += 2 * y + 1;
        if (err > 0) { x--; err -= 2 * x + 1; }
    }
}

/* linha de Bresenham */
void gfx_line(int x0, int y0, int x1, int y1, u16 color) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        gfx_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

static void gfx_digit(int x, int y, int d, int scale, u16 color) {
    for (int row = 0; row < 5; row++) {
        u8 bits = FONT3X5[d][row];
        for (int col = 0; col < 3; col++) {
            if (bits & (1 << (2 - col)))
                gfx_rect(x + col * scale, y + row * scale, scale, scale, color);
        }
    }
}

void gfx_number(int x, int y, int value, int scale, u16 color) {
    char digits[6];
    int n = 0;
    if (value < 0) value = 0;
    if (value == 0) { digits[n++] = 0; }
    while (value > 0 && n < 6) { digits[n++] = value % 10; value /= 10; }
    int advance = 4 * scale;
    for (int i = n - 1; i >= 0; i--) {
        gfx_digit(x, y, digits[i], scale, color);
        x += advance;
    }
}
