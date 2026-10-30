/* language: C (GNU C99), file: gba_regs.h, target: Nintendo GBA (ARM7TDMI), toolchain: devkitARM
 * Registradores de hardware usados diretamente (sem depender de libgba/tonc),
 * endereços fixos documentados publicamente pelo hardware do GBA. */
#ifndef GBA_REGS_H
#define GBA_REGS_H

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef volatile u16   vu16;
typedef volatile u32   vu32;

/* --- Display --- */
#define REG_DISPCNT   (*(vu32*)0x04000000)
#define REG_VCOUNT    (*(vu16*)0x04000006)

#define MODE3         0x0003
#define BG2_ENABLE    0x0400

#define SCREEN_W 240
#define SCREEN_H 160
#define VRAM     ((vu16*)0x06000000)

/* --- Teclado --- */
#define REG_KEYINPUT  (*(vu16*)0x04000130)
#define KEY_A       0x0001
#define KEY_B       0x0002
#define KEY_SELECT  0x0004
#define KEY_START   0x0008
#define KEY_RIGHT   0x0010
#define KEY_LEFT    0x0020
#define KEY_UP      0x0040
#define KEY_DOWN    0x0080
#define KEY_R       0x0100
#define KEY_L       0x0200

/* --- DMA 3 (usado para limpar a tela rápido) --- */
#define REG_DMA3SAD   (*(vu32*)0x040000D4)
#define REG_DMA3DAD   (*(vu32*)0x040000D8)
#define REG_DMA3CNT   (*(vu32*)0x040000DC)
#define DMA_SRC_FIXED (2u << 7)   /* controle de origem: endereço fixo (repete o mesmo valor) */
#define DMA_ENABLE16  (1u << 15)

/* Cor 15-bit BGR (modo 3) */
#define RGB15(r,g,b) ((u16)((r) | ((g) << 5) | ((b) << 10)))

static inline u16 key_raw(void) { return REG_KEYINPUT; }
/* teclado é "ativo em zero": inverte e mascara para teclas pressionadas = bit 1 */
static inline u16 key_down_mask(void) { return (~REG_KEYINPUT) & 0x03FF; }

#endif
