/* language: C (GNU C99), file: input.h, target: GBA / devkitARM */
#ifndef INPUT_H
#define INPUT_H
#include "gba_regs.h"

typedef enum { DIR_NONE, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Dir;

typedef enum { SPELL_STAR, SPELL_LINE, SPELL_CHAIN, SPELL_NOVA, SPELL_NONE } SpellId;

void input_update(void);
u16  key_held(void);
u16  key_pressed(void);   /* apertou neste frame */
u16  key_released(void);  /* soltou neste frame */

/* direção atual do D-pad (ignora diagonais, prioriza o último eixo tocado) */
Dir  input_dpad_dir(void);

/* ao segurar A o D-pad "desenha" um gesto em vez de mover o personagem;
 * soltar A classifica o traço em uma magia. */
void     gesture_begin(void);
void     gesture_feed(Dir d);
SpellId  gesture_finish(void);

#endif
