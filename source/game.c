/* language: C (GNU C99), file: game.c, target: GBA / devkitARM
 * DrawaBattle GBA - versão homebrew inspirada no jogo web (desenhar símbolo
 * no D-pad para lançar magia + sobreviver a ondas de inimigos).
 * Sem texto/fonte completa: HUD usa barras, ícones e números (gfx_number). */
#include "game.h"
#include "gba_regs.h"
#include "gfx.h"
#include "input.h"

/* ---------------------------------------------------------- constantes */
#define MAX_ENEMIES   20
#define MAX_EFFECTS   12
#define ARENA_X0 4
#define ARENA_Y0 14
#define ARENA_X1 (SCREEN_W - 4)
#define ARENA_Y1 (SCREEN_H - 4)

#define PLAYER_MAXHP   100
#define PLAYER_SPEED   130   /* fixed-point: px por frame * 100 */
#define PLAYER_HIT_DMG 10
#define PLAYER_INVULN_F 50

#define COL_BG     RGB15(3,3,6)
#define COL_WALL   RGB15(10,9,14)
#define COL_PLAYER RGB15(18,26,31)
#define COL_HPBAR  RGB15(6,28,10)
#define COL_HPBG   RGB15(20,4,4)
#define COL_ENEMY  RGB15(28,10,10)
#define COL_ENEMY_SLOW RGB15(10,18,28)
#define COL_STAR   RGB15(31,24,8)
#define COL_LINE   RGB15(18,27,31)
#define COL_CHAIN  RGB15(25,16,31)
#define COL_NOVA   RGB15(13,22,31)
#define COL_TEXT   RGB15(31,31,31)

typedef enum { ST_TITLE, ST_PLAY, ST_BREAK, ST_OVER } GameState;

typedef struct { int x100, y100; int hp, maxhp, alive; int speed100; int slow_f; } Enemy;
typedef struct { int active, kind; int x0, y0, x1, y1; int r; int timer, life; u16 color; } Effect;

static GameState state;
static int px100, py100;      /* posição do jogador, fixed-point *100 */
static int facing_dx, facing_dy;
static int phealth, pmaxhealth;
static int pinvuln;
static int channeling;        /* 1 enquanto A está segurado (desenhando gesto) */

static Enemy enemies[MAX_ENEMIES];
static Effect fx[MAX_EFFECTS];

static int cd_star, cd_line, cd_chain, cd_nova; /* cooldowns restantes, em frames */
static int wave_n, wave_alive, wave_to_spawn, wave_spawn_timer, break_timer;
static int score, kills;
static u32 rng_state = 98765;
static int blink;

/* --------------------------------------------------------------- util */
static u32 rng_next(void) { rng_state = rng_state * 1664525u + 1013904223u; return rng_state; }
static int rng_range(int lo, int hi) { return lo + (int)((rng_next() >> 16) % (u32)(hi - lo + 1)); }

static int isq(int a, int b) { int d = a - b; return d * d; } /* diferença ao quadrado */

static void fx_spawn(int kind, int x0, int y0, int x1, int y1, int r, int life, u16 color) {
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (!fx[i].active) {
            fx[i] = (Effect){ 1, kind, x0, y0, x1, y1, r, life, life, color };
            return;
        }
    }
}

/* ------------------------------------------------------------ inimigo */
static void enemy_spawn(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].alive) continue;
        int edge = rng_range(0, 3);
        int x, y;
        switch (edge) {
            case 0: x = rng_range(ARENA_X0, ARENA_X1); y = ARENA_Y0; break;
            case 1: x = rng_range(ARENA_X0, ARENA_X1); y = ARENA_Y1; break;
            case 2: x = ARENA_X0; y = rng_range(ARENA_Y0, ARENA_Y1); break;
            default: x = ARENA_X1; y = rng_range(ARENA_Y0, ARENA_Y1); break;
        }
        int hp = 36 + wave_n * 9;
        int spd = 55 + wave_n * 3; if (spd > 150) spd = 150;
        enemies[i] = (Enemy){ x * 100, y * 100, hp, hp, 1, spd, 0 };
        return;
    }
}

static void enemy_kill(Enemy *e) {
    e->alive = 0;
    kills++;
    score += 10 + wave_n * 2;
    wave_alive--;
    fx_spawn(1, e->x100 / 100, e->y100 / 100, 0, 0, 10, 10, COL_ENEMY);
}

static void enemy_damage(Enemy *e, int dmg) {
    if (!e->alive) return;
    e->hp -= dmg;
    if (e->hp <= 0) enemy_kill(e);
}

/* -------------------------------------------------------------- ondas */
static void wave_start(int n) {
    wave_n = n;
    wave_to_spawn = 4 + n * 2;
    if (wave_to_spawn > MAX_ENEMIES) wave_to_spawn = MAX_ENEMIES;
    wave_alive = wave_to_spawn;
    wave_spawn_timer = 0;
    phealth += pmaxhealth * 10 / 100;
    if (phealth > pmaxhealth) phealth = pmaxhealth;
}

/* --------------------------------------------------------- magias --- */
static void cast_star(void) {
    if (cd_star > 0) return;
    cd_star = 90;
    int r = 42;
    fx_spawn(2, px100 / 100, py100 / 100, 0, 0, r, 14, COL_STAR);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) continue;
        if (isq(enemies[i].x100 / 100, px100 / 100) + isq(enemies[i].y100 / 100, py100 / 100) <= r * r)
            enemy_damage(&enemies[i], 34);
    }
}

static void cast_line(void) {
    if (cd_line > 0) return;
    cd_line = 70;
    int len = 70;
    int x1 = px100 / 100 + facing_dx * len, y1 = py100 / 100 + facing_dy * len;
    fx_spawn(3, px100 / 100, py100 / 100, x1, y1, 0, 12, COL_LINE);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) continue;
        int ex = enemies[i].x100 / 100, ey = enemies[i].y100 / 100;
        /* distância aproximada do inimigo até o segmento, checando alguns pontos da linha */
        for (int t = 0; t <= 10; t++) {
            int sx = px100 / 100 + facing_dx * len * t / 10;
            int sy = py100 / 100 + facing_dy * len * t / 10;
            if (isq(ex, sx) + isq(ey, sy) <= 16 * 16) { enemy_damage(&enemies[i], 20); break; }
        }
    }
}

static void cast_chain(void) {
    if (cd_chain > 0) return;
    cd_chain = 170;
    int hit[MAX_ENEMIES]; for (int i = 0; i < MAX_ENEMIES; i++) hit[i] = 0;
    int cx = px100 / 100, cy = py100 / 100;
    for (int step = 0; step < 4; step++) {
        int best = -1, bestd = 1 << 30;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!enemies[i].alive || hit[i]) continue;
            int d = isq(enemies[i].x100 / 100, cx) + isq(enemies[i].y100 / 100, cy);
            if (d < bestd) { bestd = d; best = i; }
        }
        int range = (step == 0) ? 100 * 100 : 75 * 75;
        if (best < 0 || bestd > range) break;
        hit[best] = 1;
        enemy_damage(&enemies[best], 24);
        fx_spawn(3, cx, cy, enemies[best].x100 / 100, enemies[best].y100 / 100, 0, 10, COL_CHAIN);
        cx = enemies[best].x100 / 100; cy = enemies[best].y100 / 100;
    }
}

static void cast_nova(void) {
    if (cd_nova > 0) return;
    cd_nova = 230;
    int r = 58;
    fx_spawn(4, px100 / 100, py100 / 100, 0, 0, r, 20, COL_NOVA);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) continue;
        if (isq(enemies[i].x100 / 100, px100 / 100) + isq(enemies[i].y100 / 100, py100 / 100) <= r * r) {
            enemy_damage(&enemies[i], 20);
            enemies[i].slow_f = 90;
        }
    }
}

static void cast(SpellId s) {
    switch (s) {
        case SPELL_STAR:  cast_star();  break;
        case SPELL_LINE:  cast_line();  break;
        case SPELL_CHAIN: cast_chain(); break;
        case SPELL_NOVA:  cast_nova();  break;
        default: break;
    }
}

/* ------------------------------------------------------------- setup */
static void reset_run(void) {
    px100 = (SCREEN_W / 2) * 100; py100 = (SCREEN_H / 2) * 100;
    facing_dx = 0; facing_dy = -1;
    pmaxhealth = PLAYER_MAXHP; phealth = pmaxhealth; pinvuln = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].alive = 0;
    for (int i = 0; i < MAX_EFFECTS; i++) fx[i].active = 0;
    cd_star = cd_line = cd_chain = cd_nova = 0;
    score = 0; kills = 0;
    break_timer = 60;
    wave_start(1);
    state = ST_BREAK;
}

void game_init(void) {
    gfx_init();
    state = ST_TITLE;
    reset_run();
    state = ST_TITLE;
}

/* -------------------------------------------------------------update */
static void update_title(void) {
    blink++;
    if (key_pressed() & KEY_START) { reset_run(); }
}

static void update_player_move(void) {
    int dx = 0, dy = 0;
    if (key_held() & KEY_UP) dy -= 1;
    if (key_held() & KEY_DOWN) dy += 1;
    if (key_held() & KEY_LEFT) dx -= 1;
    if (key_held() & KEY_RIGHT) dx += 1;
    if (dx || dy) {
        facing_dx = dx; facing_dy = dy;
        /* normaliza aproximadamente a diagonal (evita andar mais rápido na diagonal) */
        int spd = (dx && dy) ? (PLAYER_SPEED * 71 / 100) : PLAYER_SPEED;
        px100 += dx * spd; py100 += dy * spd;
    }
    int minx = ARENA_X0 * 100, maxx = ARENA_X1 * 100, miny = ARENA_Y0 * 100, maxy = ARENA_Y1 * 100;
    if (px100 < minx) px100 = minx;
    if (px100 > maxx) px100 = maxx;
    if (py100 < miny) py100 = miny;
    if (py100 > maxy) py100 = maxy;
}

static void update_gesture_input(void) {
    u16 kp = key_pressed(), kh = key_held(), kr = key_released();
    if (kp & KEY_A) { channeling = 1; gesture_begin(); }
    if (channeling) {
        Dir d = input_dpad_dir();
        gesture_feed(d);
        if (kr & KEY_A) {
            channeling = 0;
            cast(gesture_finish());
        }
    }
    (void)kh;
}

static void update_enemies(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &enemies[i];
        if (!e->alive) continue;
        int spd = e->speed100;
        if (e->slow_f > 0) { e->slow_f--; spd /= 2; }
        int ddx = px100 - e->x100, ddy = py100 - e->y100;
        int adx = ddx < 0 ? -ddx : ddx, ady = ddy < 0 ? -ddy : ddy;
        int sum = adx + ady;
        if (sum > 0) {
            /* aproxima um vetor normalizado sem sqrt: pondera pelo peso manhattan */
            int sxn = ddx > 0 ? 1 : (ddx < 0 ? -1 : 0);
            int syn = ddy > 0 ? 1 : (ddy < 0 ? -1 : 0);
            e->x100 += sxn * spd * adx / sum;
            e->y100 += syn * spd * ady / sum;
        }
        if (isq(e->x100 / 100, px100 / 100) + isq(e->y100 / 100, py100 / 100) < 10 * 10) {
            if (pinvuln <= 0) {
                phealth -= (8 + wave_n / 2);
                pinvuln = PLAYER_INVULN_F;
                fx_spawn(1, px100 / 100, py100 / 100, 0, 0, 14, 10, RGB15(31,6,6));
            }
        }
    }
}

static void update_effects(void) {
    for (int i = 0; i < MAX_EFFECTS; i++)
        if (fx[i].active) { fx[i].timer--; if (fx[i].timer <= 0) fx[i].active = 0; }
}

static void update_play(void) {
    if (pinvuln > 0) pinvuln--;
    if (cd_star > 0) cd_star--;
    if (cd_line > 0) cd_line--;
    if (cd_chain > 0) cd_chain--;
    if (cd_nova > 0) cd_nova--;

    update_gesture_input();
    if (!channeling) update_player_move();

    wave_spawn_timer--;
    if (wave_to_spawn > 0 && wave_spawn_timer <= 0) {
        enemy_spawn(); wave_to_spawn--; wave_spawn_timer = 24;
    }

    update_enemies();
    update_effects();

    if (phealth <= 0) { state = ST_OVER; blink = 0; return; }
    if (wave_to_spawn == 0 && wave_alive <= 0) {
        break_timer = 90;
        state = ST_BREAK;
    }
}

static void update_break(void) {
    update_effects();
    break_timer--;
    if (break_timer <= 0) { wave_start(wave_n + 1); state = ST_PLAY; }
}

static void update_over(void) {
    blink++;
    if (key_pressed() & KEY_START) { reset_run(); }
}

void game_update(void) {
    input_update();
    switch (state) {
        case ST_TITLE: update_title(); break;
        case ST_PLAY:  update_play();  break;
        case ST_BREAK: update_break(); break;
        case ST_OVER:  update_over();  break;
    }
}

/* --------------------------------------------------------------- draw */
static void draw_arena(void) {
    gfx_clear(COL_BG);
    gfx_rect_outline(ARENA_X0 - 1, ARENA_Y0 - 1, ARENA_X1 - ARENA_X0 + 2, ARENA_Y1 - ARENA_Y0 + 2, COL_WALL);
}

static void draw_hud(void) {
    int barw = 100, barh = 6;
    gfx_rect(4, 2, barw, barh, COL_HPBG);
    int hp = phealth < 0 ? 0 : phealth;
    gfx_rect(4, 2, barw * hp / pmaxhealth, barh, COL_HPBAR);
    gfx_rect_outline(4, 2, barw, barh, COL_WALL);
    gfx_number(4, 2 + barh + 2, hp, 1, COL_TEXT);

    /* estrela = onda, número ao lado */
    gfx_fill_circle(SCREEN_W - 70, 6, 3, COL_STAR);
    gfx_number(SCREEN_W - 62, 2, wave_n, 1, COL_TEXT);
    /* losango = pontuação */
    gfx_fill_circle(SCREEN_W - 32, 6, 3, RGB15(20, 31, 20));
    gfx_number(SCREEN_W - 24, 2, score, 1, COL_TEXT);

    /* ícones de cooldown das 4 magias, canto inferior */
    int iy = SCREEN_H - 10;
    int cds[4] = { cd_star, cd_line, cd_chain, cd_nova };
    u16 cols[4] = { COL_STAR, COL_LINE, COL_CHAIN, COL_NOVA };
    for (int i = 0; i < 4; i++) {
        int ix = 6 + i * 16;
        u16 c = cds[i] > 0 ? RGB15(10,10,10) : cols[i];
        gfx_rect(ix, iy, 10, 6, c);
        gfx_rect_outline(ix, iy, 10, 6, COL_WALL);
    }
}

static void draw_player(void) {
    int x = px100 / 100, y = py100 / 100;
    u16 c = (pinvuln > 0 && (pinvuln / 4) % 2) ? RGB15(31,31,31) : COL_PLAYER;
    gfx_fill_circle(x, y, 6, c);
    gfx_line(x, y, x + facing_dx * 9, y + facing_dy * 9, RGB15(31,31,31));
}

static void draw_enemies(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) continue;
        int x = enemies[i].x100 / 100, y = enemies[i].y100 / 100;
        u16 c = enemies[i].slow_f > 0 ? COL_ENEMY_SLOW : COL_ENEMY;
        gfx_fill_circle(x, y, 5, c);
    }
}

static void draw_effects(void) {
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (!fx[i].active) continue;
        Effect *e = &fx[i];
        int t = e->life > 0 ? (e->timer * 100 / e->life) : 0;
        switch (e->kind) {
            case 1: gfx_circle_outline(e->x0, e->y0, (e->life - e->timer) + 2, e->color); break;
            case 2: { int r = e->r * (100 - t) / 100 + 4; gfx_circle_outline(e->x0, e->y0, r, e->color); } break;
            case 3: gfx_line(e->x0, e->y0, e->x1, e->y1, e->color); break;
            case 4: { int r = e->r * (100 - t) / 100; gfx_circle_outline(e->x0, e->y0, r, e->color); } break;
        }
    }
}

static void draw_title(void) {
    gfx_clear(RGB15(2,2,5));
    int r = 20 + ((blink / 3) % 10);
    gfx_circle_outline(SCREEN_W / 2, SCREEN_H / 2 - 20, r, COL_STAR);
    gfx_fill_circle(SCREEN_W / 2, SCREEN_H / 2 - 20, 8, COL_STAR);
    if ((blink / 20) % 2) {
        /* triângulo "play" piscando = aperte START */
        int cx = SCREEN_W / 2, cy = SCREEN_H - 30;
        for (int i = 0; i < 10; i++) gfx_vline(cx - 5 + i, cy - i / 2, cy + i / 2, COL_TEXT);
    }
}

static void draw_over(void) {
    draw_arena();
    draw_enemies();
    draw_player();
    draw_hud();
    gfx_rect(SCREEN_W / 2 - 40, SCREEN_H / 2 - 16, 80, 32, RGB15(10,2,2));
    gfx_rect_outline(SCREEN_W / 2 - 40, SCREEN_H / 2 - 16, 80, 32, RGB15(28,6,6));
    gfx_number(SCREEN_W / 2 - 30, SCREEN_H / 2 - 12, wave_n, 2, COL_TEXT);
    gfx_number(SCREEN_W / 2 - 30, SCREEN_H / 2 + 2, score, 1, COL_TEXT);
}

void game_draw(void) {
    switch (state) {
        case ST_TITLE:
            draw_title();
            break;
        case ST_PLAY:
        case ST_BREAK:
            draw_arena();
            draw_effects();
            draw_enemies();
            draw_player();
            draw_hud();
            break;
        case ST_OVER:
            draw_over();
            break;
    }
}
