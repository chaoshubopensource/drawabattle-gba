/* language: C (GNU C99), file: input.c, target: GBA / devkitARM
 * O GBA não tem tela de toque, então "desenhar" o símbolo da magia vira uma
 * sequência de direções do D-pad feita com A segurado:
 *   - soltar A sem apertar direção nenhuma         -> Estrela  (explosão no local)
 *   - segurar 1 direção só                         -> Corte    (linha reta)
 *   - ir e voltar (ex: cima, baixo)                -> Raio     (zigue-zague em cadeia)
 *   - passar por 3+ direções diferentes (dar a volta) -> Nova  (círculo congelante)
 */
#include "input.h"

static u16 prev_keys = 0, cur_keys = 0;

void input_update(void) {
    prev_keys = cur_keys;
    cur_keys = key_down_mask();
}

u16 key_held(void)     { return cur_keys; }
u16 key_pressed(void)  { return cur_keys & ~prev_keys; }
u16 key_released(void) { return prev_keys & ~cur_keys; }

Dir input_dpad_dir(void) {
    if (cur_keys & KEY_UP)    return DIR_UP;
    if (cur_keys & KEY_DOWN)  return DIR_DOWN;
    if (cur_keys & KEY_LEFT)  return DIR_LEFT;
    if (cur_keys & KEY_RIGHT) return DIR_RIGHT;
    return DIR_NONE;
}

#define GESTURE_MAX 4
static Dir gbuf[GESTURE_MAX];
static int glen = 0;
static Dir glast = DIR_NONE;

void gesture_begin(void) {
    glen = 0;
    glast = DIR_NONE;
}

void gesture_feed(Dir d) {
    if (d == DIR_NONE || d == glast) return;
    if (glen < GESTURE_MAX) gbuf[glen++] = d;
    glast = d;
}

static int opposite(Dir a, Dir b) {
    return (a == DIR_UP && b == DIR_DOWN) || (a == DIR_DOWN && b == DIR_UP) ||
           (a == DIR_LEFT && b == DIR_RIGHT) || (a == DIR_RIGHT && b == DIR_LEFT);
}

SpellId gesture_finish(void) {
    if (glen == 0) return SPELL_STAR;
    if (glen == 1) return SPELL_LINE;
    if (glen >= 3) return SPELL_NOVA;      /* deu a volta por 3+ direções: círculo */
    if (glen == 2 && opposite(gbuf[0], gbuf[1])) return SPELL_CHAIN; /* ida e volta: zigue-zague */
    return SPELL_LINE; /* fallback razoável para combinações de 2 direções não opostas */
}
