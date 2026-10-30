/* language: C (GNU C99), file: main.c, target: Nintendo GBA (ARM7TDMI) / devkitARM
 * DrawaBattle GBA - demonstração homebrew.
 * Controles: D-pad move. Segure A e desenhe uma direção no D-pad para
 * carregar a magia, solte A para lançar:
 *   soltar sem direção  = Estrela (explosão no local)
 *   1 direção só        = Corte   (linha reta na direção que olha)
 *   ida-e-volta          = Raio    (corrente entre inimigos próximos)
 *   3+ direções (volta)  = Nova    (círculo que atordoa e some em área) */
#include "gfx.h"
#include "game.h"

int main(void) {
    game_init();
    for (;;) {
        gfx_vsync();
        game_update();
        game_draw();
    }
    return 0;
}
