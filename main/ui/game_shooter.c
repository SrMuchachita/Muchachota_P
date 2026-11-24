/*
 * ============================================================================
 *  COSMO DEFENSA — juego de naves original para la pantalla 800x480 (LVGL 9)
 * ============================================================================
 *
 * Mismo archivo para la placa (ESP32-P4) y para el simulador de PC (sim/).
 * No usa nada de hardware: solo LVGL. Se abre con game_shooter_open().
 *
 * Como se juega:
 *   - Desliza el dedo (o arrastra el mouse en el simulador) en cualquier
 *     parte de la pantalla: la nave sigue al dedo en horizontal.
 *   - La nave dispara sola.
 *   - Los enemigos estan en formacion y de a uno se lanzan en picada.
 *   - 3 vidas. Al limpiar la formacion empieza la siguiente oleada, mas rapida.
 *
 * ---------------------------------------------------------------------------
 *  >>> PERSONALIZACION: todo lo que se puede tocar sin miedo esta en la
 *      seccion "CONFIGURACION" de abajo: nombre, colores, puntos, velocidades
 *      y los DIBUJOS de cada figura (nave, enemigos, explosion).
 *
 *  Como funcionan los dibujos: cada figura es una lista de filas de texto.
 *  Cada letra es un "pixel" de color segun la PALETA; el punto '.' es
 *  transparente. Cada pixel del dibujo se agranda PIX veces en pantalla.
 *  Las filas pueden tener cualquier largo (se rellenan solas), y se pueden
 *  agregar o quitar filas libremente. Los enemigos tienen 2 cuadros
 *  (_F1 / _F2) que se alternan para que parezcan moverse.
 * ---------------------------------------------------------------------------
 */

#include "game_shooter.h"
#include "lvgl.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 *  CONFIGURACION
 * ========================================================================== */

#define GAME_TITLE      "COSMO DEFENSA"
#define GAME_SUBTITLE   "Defiende la base de los invasores"

/* Tamaño de cada "pixel" de los dibujos, en pixeles reales de pantalla. */
#define PIX             4

/* Paleta: letra -> color (0xRRGGBB). Agrega letras nuevas aca si quieres. */
typedef struct { char key; uint32_t rgb; } pal_entry_t;
static const pal_entry_t PALETTE[] = {
    { 'W', 0xFFFFFF },  /* blanco        */
    { 'S', 0x9AA4B8 },  /* gris metal    */
    { 'K', 0x101018 },  /* casi negro    */
    { 'R', 0xFF3B3B },  /* rojo          */
    { 'O', 0xFF9A1F },  /* naranja       */
    { 'Y', 0xFFE34A },  /* amarillo      */
    { 'G', 0x3EE06A },  /* verde         */
    { 'C', 0x2FE3FF },  /* cian          */
    { 'B', 0x2F6BFF },  /* azul          */
    { 'M', 0xE24AE2 },  /* magenta       */
    { 'P', 0x8F55FF },  /* violeta       */
};

/* --- Nave del jugador --- */
static const char *const ART_SHIP[] = {
    "......C......",
    ".....CWC.....",
    ".....WWW.....",
    "....WWBWW....",
    "....WBBBW....",
    "R..WWBBBWW..R",
    "S.WWWWBWWWW.S",
    "SSWWWWWWWWWSS",
    "SSS.RRWRR.SSS",
    "S....O.O....S",
    ".....Y.Y.....",
};

/* --- Enemigo A: "Medusa" (filas de abajo) --- */
static const char *const ART_EN_A_F1[] = {
    "...MMMMM...",
    "..MMMMMMM..",
    ".MMWKMWKMM.",
    ".MMMMMMMMM.",
    ".M.M.M.M.M.",
    ".M.M.M.M.M.",
    "M..M...M..M",
    "..M.....M..",
};
static const char *const ART_EN_A_F2[] = {
    "...MMMMM...",
    "..MMMMMMM..",
    ".MMWKMWKMM.",
    ".MMMMMMMMM.",
    ".M.M.M.M.M.",
    "M..M.M.M..M",
    ".M...M...M.",
    "...M...M...",
};

/* --- Enemigo B: "Cangrejo" (filas del medio) --- */
static const char *const ART_EN_B_F1[] = {
    "G.........G",
    ".G.......G.",
    "..GGGGGGG..",
    ".GGYGGGYGG.",
    "GGGGGGGGGGG",
    "G.GGGGGGG.G",
    "G.G.....G.G",
    "...GG.GG...",
};
static const char *const ART_EN_B_F2[] = {
    "..G.....G..",
    "G..G...G..G",
    "G.GGGGGGG.G",
    "GGGYGGGYGGG",
    "GGGGGGGGGGG",
    ".GGGGGGGGG.",
    "..G.....G..",
    ".G.......G.",
};

/* --- Enemigo C: "Ojo comandante" (fila de arriba, aguanta 2 tiros) --- */
static const char *const ART_EN_C_F1[] = {
    "...OOOOO...",
    ".OOOOOOOOO.",
    "OOOWWWWWOOO",
    "OOWWCCCWWOO",
    "OOWWCKCWWOO",
    "OOOWWWWWOOO",
    ".OO.OOO.OO.",
    "O....O....O",
};
static const char *const ART_EN_C_F2[] = {
    "...OOOOO...",
    ".OOOOOOOOO.",
    "OOOWWWWWOOO",
    "OOWCCCWWWOO",
    "OOWCKCWWWOO",
    "OOOWWWWWOOO",
    ".OO.OOO.OO.",
    ".O...O...O.",
};

/* --- Explosion (2 cuadros) --- */
static const char *const ART_BOOM_F1[] = {
    "....Y....",
    ".Y..O..Y.",
    "..OOYOO..",
    "..OYWYO..",
    "YOYWWWYOY",
    "..OYWYO..",
    "..OOYOO..",
    ".Y..O..Y.",
    "....Y....",
};
static const char *const ART_BOOM_F2[] = {
    "R...O...R",
    "..R...R..",
    "O..O.O..O",
    "...R.R...",
    ".O.....O.",
    "...R.R...",
    "O..O.O..O",
    "..R...R..",
    "R...O...R",
};

/* Puntos por enemigo (A, B, C). En picada valen el doble. */
static const int ENEMY_POINTS[3] = { 50, 80, 150 };
static const int ENEMY_HP[3]     = { 1, 1, 2 };

/* Colores de disparos, estrellas y fondo */
#define COLOR_BG            0x05060F
#define COLOR_SHOT          0x7DF9FF
#define COLOR_ENEMY_SHOT    0xFF5A5A
#define COLOR_HUD           0xFFFFFF
#define COLOR_TITLE         0x2FE3FF
#define COLOR_ACCENT        0xFFE34A

#define START_LIVES         3
#define SHIP_SPEED          700.0f   /* px/s siguiendo al dedo */
#define FIRE_EVERY_MS       260      /* disparo automatico */
#define SHOT_SPEED          560.0f

/* ============================================================================
 *  (de aca para abajo es la logica del juego)
 * ========================================================================== */

#define GW          800
#define GH          480

#define COLS        8
#define ROWS        4
#define N_ENEMIES   (COLS * ROWS)
#define CELL_W      64
#define CELL_H      46
#define FORM_Y      70

#define N_SHOTS     5
#define N_ESHOTS    8
#define N_STARS     36
#define N_BOOMS     10
#define N_LIFE_ICON 6

#define ART_ROWS(a) ((int)(sizeof(a) / sizeof((a)[0])))

#if LV_FONT_MONTSERRAT_32
#define FONT_BIG (&lv_font_montserrat_32)
#else
#define FONT_BIG LV_FONT_DEFAULT
#endif
#if LV_FONT_MONTSERRAT_20
#define FONT_MID (&lv_font_montserrat_20)
#else
#define FONT_MID LV_FONT_DEFAULT
#endif
#if LV_FONT_MONTSERRAT_14
#define FONT_SMALL (&lv_font_montserrat_14)
#else
#define FONT_SMALL LV_FONT_DEFAULT
#endif

/* ---------------------------------------------------------------- sprites */

typedef struct {
    lv_image_dsc_t dsc;
    uint8_t       *buf;
    int            w, h;
} sprite_t;

static sprite_t spr_ship, spr_ship_mini, spr_boom[2], spr_enemy[3][2];
static bool     s_sprites_ready = false;

static bool pal_lookup(char c, uint32_t *rgb)
{
    for (size_t i = 0; i < sizeof(PALETTE) / sizeof(PALETTE[0]); i++) {
        if (PALETTE[i].key == c) { *rgb = PALETTE[i].rgb; return true; }
    }
    return false;
}

static void sprite_build(sprite_t *s, const char *const *art, int rows, int scale)
{
    int cols = 0;
    for (int r = 0; r < rows; r++) {
        int len = (int)strlen(art[r]);
        if (len > cols) cols = len;
    }
    s->w = cols * scale;
    s->h = rows * scale;
    uint32_t stride = (uint32_t)s->w * 4;
    uint32_t size   = stride * (uint32_t)s->h;
    s->buf = lv_malloc(size);
    if (!s->buf) { s->w = s->h = 0; return; }
    memset(s->buf, 0, size);

    for (int r = 0; r < rows; r++) {
        int len = (int)strlen(art[r]);
        for (int c = 0; c < len; c++) {
            uint32_t rgb;
            if (!pal_lookup(art[r][c], &rgb)) continue;
            for (int dy = 0; dy < scale; dy++) {
                uint8_t *p = s->buf + (uint32_t)(r * scale + dy) * stride + (uint32_t)(c * scale) * 4;
                for (int dx = 0; dx < scale; dx++, p += 4) {
                    p[0] = (uint8_t)(rgb & 0xFF);         /* B */
                    p[1] = (uint8_t)((rgb >> 8) & 0xFF);  /* G */
                    p[2] = (uint8_t)((rgb >> 16) & 0xFF); /* R */
                    p[3] = 0xFF;                          /* A */
                }
            }
        }
    }

    memset(&s->dsc, 0, sizeof(s->dsc));
    s->dsc.header.magic  = LV_IMAGE_HEADER_MAGIC;
    s->dsc.header.cf     = LV_COLOR_FORMAT_ARGB8888;
    s->dsc.header.w      = (uint32_t)s->w;
    s->dsc.header.h      = (uint32_t)s->h;
    s->dsc.header.stride = stride;
    s->dsc.data_size     = size;
    s->dsc.data          = s->buf;
}

static void sprites_init(void)
{
    if (s_sprites_ready) return;
    sprite_build(&spr_ship,      ART_SHIP, ART_ROWS(ART_SHIP), PIX);
    sprite_build(&spr_ship_mini, ART_SHIP, ART_ROWS(ART_SHIP), 2);
    sprite_build(&spr_boom[0],   ART_BOOM_F1, ART_ROWS(ART_BOOM_F1), PIX + 1);
    sprite_build(&spr_boom[1],   ART_BOOM_F2, ART_ROWS(ART_BOOM_F2), PIX + 1);
    sprite_build(&spr_enemy[0][0], ART_EN_A_F1, ART_ROWS(ART_EN_A_F1), PIX);
    sprite_build(&spr_enemy[0][1], ART_EN_A_F2, ART_ROWS(ART_EN_A_F2), PIX);
    sprite_build(&spr_enemy[1][0], ART_EN_B_F1, ART_ROWS(ART_EN_B_F1), PIX);
    sprite_build(&spr_enemy[1][1], ART_EN_B_F2, ART_ROWS(ART_EN_B_F2), PIX);
    sprite_build(&spr_enemy[2][0], ART_EN_C_F1, ART_ROWS(ART_EN_C_F1), PIX);
    sprite_build(&spr_enemy[2][1], ART_EN_C_F2, ART_ROWS(ART_EN_C_F2), PIX);
    s_sprites_ready = true;
}

/* --------------------------------------------------------------- entidades */

typedef enum { EN_DEAD, EN_FORM, EN_DIVE, EN_RETURN } en_state_t;

typedef struct {
    lv_obj_t  *obj;
    en_state_t st;
    int        type, hp, col, row, frame;
    float      x, y, t, dive_x0, dive_amp;
} enemy_t;

typedef struct { lv_obj_t *obj; bool on; float x, y; } shot_t;
typedef struct { lv_obj_t *obj; float x, y, v; } star_t;
typedef struct { lv_obj_t *obj; bool on; uint32_t age; int frame; } boom_t;

typedef enum { GS_TITLE, GS_PLAY, GS_WAVE, GS_DEAD, GS_OVER } game_state_t;

/* ------------------------------------------------------------------ estado */

static bool          s_open = false;
static lv_obj_t     *s_prev_scr, *s_scr, *s_root;
static lv_timer_t   *s_timer;

static lv_obj_t     *s_ship;
static lv_obj_t     *s_lbl_score, *s_lbl_wave, *s_lbl_msg;
static lv_obj_t     *s_btn_pause, *s_lbl_pause;
static lv_obj_t     *s_title_box, *s_title_blink, *s_title_hi;
static lv_obj_t     *s_over_box, *s_over_score, *s_over_hi, *s_over_blink;
static lv_obj_t     *s_life_icon[N_LIFE_ICON];

static enemy_t       s_en[N_ENEMIES];
static shot_t        s_shot[N_SHOTS];
static shot_t        s_eshot[N_ESHOTS];
static star_t        s_star[N_STARS];
static boom_t        s_boom[N_BOOMS];

static game_state_t  s_state;
static bool          s_paused;
static uint32_t      s_state_ms;      /* ms desde que se entro al estado actual */
static uint32_t      s_last_tick;
static int           s_score, s_hi_score, s_lives, s_wave;
static int           s_shown_score = -1;
static float         s_ship_x, s_target_x;
static uint32_t      s_fire_ms, s_anim_ms, s_blink_ms, s_invuln_ms;
static int           s_anim_frame;
static float         s_form_x, s_form_dir;
static int           s_dive_ms, s_efire_ms;

#define SHIP_Y      (GH - spr_ship.h - 14)

/* --------------------------------------------------------------- helpers */

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static void show(lv_obj_t *o, bool on)
{
    if (!o) return;
    if (on) lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
    else    lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
}

static lv_obj_t *make_rect(lv_obj_t *parent, int w, int h, uint32_t color, int radius)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

static lv_obj_t *make_sprite(lv_obj_t *parent, const sprite_t *s)
{
    lv_obj_t *o = lv_image_create(parent);
    lv_image_set_src(o, &s->dsc);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

static lv_obj_t *make_label(lv_obj_t *parent, const lv_font_t *font, uint32_t color, const char *txt)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(l, txt);
    lv_obj_remove_flag(l, LV_OBJ_FLAG_CLICKABLE);
    return l;
}

static lv_obj_t *make_box(lv_obj_t *parent)
{
    lv_obj_t *b = lv_obj_create(parent);
    lv_obj_remove_style_all(b);
    lv_obj_set_size(b, GW, GH);
    lv_obj_set_pos(b, 0, 0);
    lv_obj_remove_flag(b, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return b;
}

static bool overlap(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh)
{
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

/* ---------------------------------------------------------- dificultad */

static float form_speed(void)     { return clampf(30.0f + 8.0f * (float)s_wave, 0, 90); }
static int   dive_interval(void)  { int v = 2600 - 250 * (s_wave - 1); return v < 600 ? 600 : v; }
static float dive_speed(void)     { return clampf(150.0f + 15.0f * (float)s_wave, 0, 300); }
static int   efire_interval(void) { int v = 1300 - 100 * (s_wave - 1); return v < 350 ? 350 : v; }
static float eshot_speed(void)    { return clampf(240.0f + 15.0f * (float)s_wave, 0, 400); }

static int jitter(int ms) { return ms * (int)lv_rand(70, 130) / 100; }

/* ------------------------------------------------------------- HUD */

static void hud_update(void)
{
    if (s_score != s_shown_score) {
        lv_label_set_text_fmt(s_lbl_score, "PUNTOS %d", s_score);
        s_shown_score = s_score;
    }
    lv_label_set_text_fmt(s_lbl_wave, "OLEADA %d", s_wave);
    for (int i = 0; i < N_LIFE_ICON; i++) show(s_life_icon[i], i < s_lives);
}

static void msg_show(const char *txt)
{
    if (txt) { lv_label_set_text(s_lbl_msg, txt); show(s_lbl_msg, true); }
    else     show(s_lbl_msg, false);
}

static void set_state(game_state_t st)
{
    s_state    = st;
    s_state_ms = 0;
}

/* ------------------------------------------------------------- acciones */

static void boom_at(float cx, float cy)
{
    for (int i = 0; i < N_BOOMS; i++) {
        if (s_boom[i].on) continue;
        s_boom[i].on    = true;
        s_boom[i].age   = 0;
        s_boom[i].frame = 0;
        lv_image_set_src(s_boom[i].obj, &spr_boom[0].dsc);
        lv_obj_set_pos(s_boom[i].obj, (int)(cx - spr_boom[0].w / 2), (int)(cy - spr_boom[0].h / 2));
        show(s_boom[i].obj, true);
        return;
    }
}

static void clear_shots(void)
{
    for (int i = 0; i < N_SHOTS; i++)  { s_shot[i].on = false;  show(s_shot[i].obj, false); }
    for (int i = 0; i < N_ESHOTS; i++) { s_eshot[i].on = false; show(s_eshot[i].obj, false); }
}

static void spawn_wave(void)
{
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            enemy_t *e = &s_en[r * COLS + c];
            e->type  = (r == 0) ? 2 : (r == 1 ? 1 : 0);
            e->hp    = ENEMY_HP[e->type];
            e->col   = c;
            e->row   = r;
            e->st    = EN_FORM;
            e->frame = s_anim_frame;
            lv_image_set_src(e->obj, &spr_enemy[e->type][e->frame].dsc);
            lv_obj_set_style_image_recolor_opa(e->obj, LV_OPA_TRANSP, 0);
            show(e->obj, true);
        }
    }
    s_form_x   = (GW - ((COLS - 1) * CELL_W + spr_enemy[0][0].w)) / 2.0f;
    s_form_dir = 1.0f;
    s_dive_ms  = 2000;
    s_efire_ms = 1200;
}

static void start_game(void)
{
    s_score   = 0;
    s_lives   = START_LIVES;
    s_wave    = 1;
    s_ship_x  = GW / 2.0f;
    s_target_x = s_ship_x;
    s_invuln_ms = 1500;
    clear_shots();
    spawn_wave();
    show(s_title_box, false);
    show(s_over_box, false);
    show(s_ship, true);
    show(s_btn_pause, true);
    show(s_lbl_score, true);
    show(s_lbl_wave, true);
    msg_show(NULL);
    hud_update();
    set_state(GS_PLAY);
}

static void player_hit(void)
{
    boom_at(s_ship_x, SHIP_Y + spr_ship.h / 2.0f);
    show(s_ship, false);
    s_lives--;
    clear_shots();
    hud_update();
    set_state(GS_DEAD);
}

static void kill_enemy(enemy_t *e)
{
    int pts = ENEMY_POINTS[e->type] * (e->st == EN_DIVE ? 2 : 1);
    s_score += pts;
    boom_at(e->x + spr_enemy[0][0].w / 2.0f, e->y + spr_enemy[0][0].h / 2.0f);
    e->st = EN_DEAD;
    show(e->obj, false);
}

static void game_over(void)
{
    bool record = s_score > s_hi_score;
    if (record) s_hi_score = s_score;
    lv_label_set_text_fmt(s_over_score, "PUNTOS: %d", s_score);
    if (record) lv_label_set_text(s_over_hi, "NUEVO RECORD!");
    else        lv_label_set_text_fmt(s_over_hi, "RECORD: %d", s_hi_score);
    show(s_over_box, true);
    show(s_btn_pause, false);
    set_state(GS_OVER);
}

/* ------------------------------------------------------------- update */

static void update_stars(float dt)
{
    for (int i = 0; i < N_STARS; i++) {
        star_t *s = &s_star[i];
        s->y += s->v * dt;
        if (s->y > GH) { s->y = -4; s->x = (float)lv_rand(0, GW - 3); }
        lv_obj_set_pos(s->obj, (int)s->x, (int)s->y);
    }
}

static void update_booms(uint32_t dt_ms)
{
    for (int i = 0; i < N_BOOMS; i++) {
        boom_t *b = &s_boom[i];
        if (!b->on) continue;
        b->age += dt_ms;
        if (b->age > 320) { b->on = false; show(b->obj, false); continue; }
        int f = b->age > 140 ? 1 : 0;
        if (f != b->frame) { b->frame = f; lv_image_set_src(b->obj, &spr_boom[f].dsc); }
    }
}

static void update_player(float dt, uint32_t dt_ms)
{
    float half = spr_ship.w / 2.0f;
    float dx   = s_target_x - s_ship_x;
    float step = SHIP_SPEED * dt;
    if (dx > step) dx = step;
    if (dx < -step) dx = -step;
    s_ship_x = clampf(s_ship_x + dx, half + 4, GW - half - 4);
    lv_obj_set_pos(s_ship, (int)(s_ship_x - half), SHIP_Y);

    if (s_invuln_ms > 0) {
        s_invuln_ms = s_invuln_ms > dt_ms ? s_invuln_ms - dt_ms : 0;
        show(s_ship, s_invuln_ms == 0 || ((s_invuln_ms / 100) % 2 == 0));
    }

    s_fire_ms += dt_ms;
    if (s_fire_ms >= FIRE_EVERY_MS) {
        for (int i = 0; i < N_SHOTS; i++) {
            if (s_shot[i].on) continue;
            s_shot[i].on = true;
            s_shot[i].x  = s_ship_x - 2;
            s_shot[i].y  = (float)SHIP_Y - 12;
            show(s_shot[i].obj, true);
            s_fire_ms = 0;
            break;
        }
    }
}

static void update_shots(float dt)
{
    int ew = spr_enemy[0][0].w, eh = spr_enemy[0][0].h;
    for (int i = 0; i < N_SHOTS; i++) {
        shot_t *s = &s_shot[i];
        if (!s->on) continue;
        s->y -= SHOT_SPEED * dt;
        if (s->y < -20) { s->on = false; show(s->obj, false); continue; }
        lv_obj_set_pos(s->obj, (int)s->x, (int)s->y);
        for (int k = 0; k < N_ENEMIES; k++) {
            enemy_t *e = &s_en[k];
            if (e->st == EN_DEAD) continue;
            if (!overlap(s->x, s->y, 4, 16, e->x + 3, e->y + 3, ew - 6, eh - 6)) continue;
            s->on = false;
            show(s->obj, false);
            if (--e->hp <= 0) {
                kill_enemy(e);
            } else {
                lv_obj_set_style_image_recolor(e->obj, lv_color_hex(0xFF2020), 0);
                lv_obj_set_style_image_recolor_opa(e->obj, 140, 0);
            }
            break;
        }
    }

    float half = spr_ship.w / 2.0f;
    for (int i = 0; i < N_ESHOTS; i++) {
        shot_t *s = &s_eshot[i];
        if (!s->on) continue;
        s->y += eshot_speed() * dt;
        if (s->y > GH) { s->on = false; show(s->obj, false); continue; }
        lv_obj_set_pos(s->obj, (int)s->x, (int)s->y);
        if (s_state == GS_PLAY && s_invuln_ms == 0 &&
            overlap(s->x, s->y, 6, 12, s_ship_x - half + 10, SHIP_Y + 10, spr_ship.w - 20, spr_ship.h - 14)) {
            s->on = false;
            show(s->obj, false);
            player_hit();
            return;
        }
    }
}

static void enemy_fire(void)
{
    int alive[N_ENEMIES], n = 0, diving[N_ENEMIES], nd = 0;
    for (int k = 0; k < N_ENEMIES; k++) {
        if (s_en[k].st == EN_DEAD) continue;
        alive[n++] = k;
        if (s_en[k].st == EN_DIVE && s_en[k].y > 0) diving[nd++] = k;
    }
    if (n == 0) return;
    int k = (nd > 0 && lv_rand(0, 1)) ? diving[lv_rand(0, nd - 1)] : alive[lv_rand(0, n - 1)];
    for (int i = 0; i < N_ESHOTS; i++) {
        if (s_eshot[i].on) continue;
        s_eshot[i].on = true;
        s_eshot[i].x  = s_en[k].x + spr_enemy[0][0].w / 2.0f - 3;
        s_eshot[i].y  = s_en[k].y + spr_enemy[0][0].h;
        show(s_eshot[i].obj, true);
        return;
    }
}

static void update_enemies(float dt, uint32_t dt_ms)
{
    int ew = spr_enemy[0][0].w, eh = spr_enemy[0][0].h;

    /* animacion de 2 cuadros */
    s_anim_ms += dt_ms;
    if (s_anim_ms >= 400) { s_anim_ms = 0; s_anim_frame ^= 1; }

    /* la formacion se balancea de lado a lado */
    float form_w = (COLS - 1) * CELL_W + ew;
    s_form_x += s_form_dir * form_speed() * dt;
    if (s_form_x < 24)               { s_form_x = 24;               s_form_dir = 1.0f; }
    if (s_form_x > GW - 24 - form_w) { s_form_x = GW - 24 - form_w; s_form_dir = -1.0f; }

    int alive = 0;
    float half = spr_ship.w / 2.0f;

    for (int k = 0; k < N_ENEMIES; k++) {
        enemy_t *e = &s_en[k];
        if (e->st == EN_DEAD) continue;
        alive++;

        float sx = s_form_x + e->col * CELL_W;
        float sy = FORM_Y + e->row * CELL_H;

        switch (e->st) {
        case EN_FORM:
            e->x = sx;
            e->y = sy;
            break;
        case EN_DIVE:
            e->t += dt;
            e->y += dive_speed() * dt;
            /* se va acercando a la nave mientras zigzaguea */
            if (e->dive_x0 < s_ship_x - ew / 2.0f) e->dive_x0 += 70.0f * dt;
            else                                    e->dive_x0 -= 70.0f * dt;
            e->x = e->dive_x0 + sinf(e->t * 2.4f) * e->dive_amp;
            if (e->y > GH + 10) { e->y = -eh - 10; e->x = sx; e->st = EN_RETURN; }
            break;
        case EN_RETURN: {
            float dx = sx - e->x, dy = sy - e->y;
            float d  = sqrtf(dx * dx + dy * dy);
            float st = 240.0f * dt;
            if (d <= st) { e->x = sx; e->y = sy; e->st = EN_FORM; }
            else         { e->x += dx / d * st; e->y += dy / d * st; }
            break;
        }
        default: break;
        }

        if (e->frame != s_anim_frame) {
            e->frame = s_anim_frame;
            lv_image_set_src(e->obj, &spr_enemy[e->type][e->frame].dsc);
        }
        lv_obj_set_pos(e->obj, (int)e->x, (int)e->y);

        /* choque contra la nave */
        if (s_state == GS_PLAY && s_invuln_ms == 0 &&
            overlap(e->x + 4, e->y + 4, ew - 8, eh - 8, s_ship_x - half + 8, SHIP_Y + 8, spr_ship.w - 16, spr_ship.h - 12)) {
            kill_enemy(e);
            player_hit();
        }
    }

    if (s_state != GS_PLAY) return;

    if (alive == 0) {
        s_wave++;
        char buf[32];
        snprintf(buf, sizeof(buf), "OLEADA %d", s_wave);
        msg_show(buf);
        hud_update();
        set_state(GS_WAVE);
        return;
    }

    /* lanzar uno en picada */
    s_dive_ms -= (int)dt_ms;
    if (s_dive_ms <= 0) {
        s_dive_ms = jitter(dive_interval());
        for (int tries = 0; tries < 12; tries++) {
            enemy_t *e = &s_en[lv_rand(0, N_ENEMIES - 1)];
            if (e->st != EN_FORM) continue;
            e->st       = EN_DIVE;
            e->t        = 0;
            e->dive_x0  = e->x;
            e->dive_amp = (float)lv_rand(30, 90);
            break;
        }
    }

    /* disparos enemigos */
    s_efire_ms -= (int)dt_ms;
    if (s_efire_ms <= 0) {
        s_efire_ms = jitter(efire_interval());
        enemy_fire();
    }
}

static void game_tick(lv_timer_t *t)
{
    (void)t;
    uint32_t now   = lv_tick_get();
    uint32_t dt_ms = lv_tick_diff(now, s_last_tick);
    s_last_tick = now;
    if (dt_ms > 50) dt_ms = 50; /* si hubo un tiron, no teletransportar todo */
    if (s_paused) return;
    float dt = dt_ms / 1000.0f;

    s_state_ms += dt_ms;
    update_stars(dt);
    update_booms(dt_ms);

    s_blink_ms += dt_ms;
    bool blink_on = (s_blink_ms / 500) % 2 == 0;

    switch (s_state) {
    case GS_TITLE:
        show(s_title_blink, blink_on);
        return; /* sin HUD en la portada */
    case GS_PLAY:
        update_player(dt, dt_ms);
        if (s_state != GS_PLAY) break;
        update_shots(dt);
        if (s_state != GS_PLAY) break;
        update_enemies(dt, dt_ms);
        break;
    case GS_WAVE:
        update_player(dt, dt_ms);
        update_shots(dt);
        if (s_state_ms > 1500) {
            msg_show(NULL);
            spawn_wave();
            set_state(GS_PLAY);
        }
        break;
    case GS_DEAD:
        update_enemies(dt, dt_ms);
        if (s_state_ms > 1500) {
            if (s_lives <= 0) {
                game_over();
            } else {
                s_ship_x = s_target_x = GW / 2.0f;
                s_invuln_ms = 2000;
                show(s_ship, true);
                set_state(GS_PLAY);
            }
        }
        break;
    case GS_OVER:
        update_shots(dt);
        show(s_over_blink, blink_on && s_state_ms > 800);
        break;
    }

    hud_update();
}

/* ------------------------------------------------------------- eventos */

static void root_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING) {
        lv_indev_t *indev = lv_indev_active();
        if (!indev) return;
        lv_point_t p;
        lv_indev_get_point(indev, &p);
        lv_area_t a;
        lv_obj_get_coords(s_root, &a);
        s_target_x = (float)(p.x - a.x1);
    } else if (code == LV_EVENT_CLICKED) {
        if (s_paused) return;
        /* 600 ms de gracia: que el mismo toque que abrio el juego no lo arranque */
        if (s_state == GS_TITLE && s_state_ms > 600) start_game();
        else if (s_state == GS_OVER && s_state_ms > 800) start_game();
    }
}

static void pause_btn_cb(lv_event_t *e)
{
    (void)e;
    if (s_state == GS_TITLE || s_state == GS_OVER) return;
    s_paused = !s_paused;
    lv_label_set_text(s_lbl_pause, s_paused ? LV_SYMBOL_PLAY : LV_SYMBOL_PAUSE);
    if (s_paused) {
        msg_show("PAUSA");
    } else if (s_state == GS_WAVE) {
        char buf[32];
        snprintf(buf, sizeof(buf), "OLEADA %d", s_wave);
        msg_show(buf);
    } else {
        msg_show(NULL);
    }
}

static void game_close(void)
{
    if (!s_open) return;
    if (s_timer) { lv_timer_delete(s_timer); s_timer = NULL; }
    s_open = false;
    lv_obj_t *prev = s_prev_scr;
    s_prev_scr = NULL;
    /* el screen del juego se borra solo al terminar el fade (auto_del) */
    if (prev && lv_obj_is_valid(prev)) {
        lv_screen_load_anim(prev, LV_SCREEN_LOAD_ANIM_FADE_IN, 200, 0, true);
    } else {
        lv_obj_delete_async(s_scr);
    }
    s_scr = s_root = NULL;
}

static void close_btn_cb(lv_event_t *e)
{
    (void)e;
    game_close();
}

static lv_obj_t *make_hud_button(lv_obj_t *parent, const char *symbol, int x, lv_event_cb_t cb, lv_obj_t **lbl_out)
{
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_size(b, 52, 40);
    lv_obj_set_pos(b, x, 6);
    lv_obj_set_style_radius(b, 8, 0);
    lv_obj_set_style_shadow_opa(b, 0, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x1B2140), 0);
    lv_obj_set_style_bg_opa(b, 200, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(0x3A4580), 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l = lv_label_create(b);
    lv_obj_set_style_text_font(l, FONT_MID, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(COLOR_HUD), 0);
    lv_label_set_text(l, symbol);
    lv_obj_center(l);
    if (lbl_out) *lbl_out = l;
    return b;
}

/* ------------------------------------------------------------- armado */

static void build_title(void)
{
    s_title_box = make_box(s_root);

    lv_obj_t *t = make_label(s_title_box, FONT_BIG, COLOR_TITLE, GAME_TITLE);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_t *st = make_label(s_title_box, FONT_SMALL, 0x9AA4B8, GAME_SUBTITLE);
    lv_obj_align(st, LV_ALIGN_TOP_MID, 0, 104);

    for (int i = 0; i < 3; i++) {
        int type = 2 - i;
        int y = 150 + i * 56;
        lv_obj_t *img = make_sprite(s_title_box, &spr_enemy[type][0]);
        lv_obj_set_pos(img, 310, y);
        lv_obj_t *l = make_label(s_title_box, FONT_MID, COLOR_HUD, "");
        lv_label_set_text_fmt(l, "=  %d PUNTOS", ENEMY_POINTS[type]);
        lv_obj_set_pos(l, 372, y + 6);
    }

    s_title_blink = make_label(s_title_box, FONT_MID, COLOR_ACCENT, "TOCA PARA EMPEZAR");
    lv_obj_align(s_title_blink, LV_ALIGN_TOP_MID, 0, 330);

    lv_obj_t *help = make_label(s_title_box, FONT_SMALL, 0x9AA4B8,
                                "Desliza el dedo para mover la nave  -  dispara sola");
    lv_obj_align(help, LV_ALIGN_TOP_MID, 0, 372);

    s_title_hi = make_label(s_title_box, FONT_SMALL, COLOR_ACCENT, "");
    lv_label_set_text_fmt(s_title_hi, "RECORD: %d", s_hi_score);
    lv_obj_align(s_title_hi, LV_ALIGN_TOP_MID, 0, 400);
}

static void build_game_over(void)
{
    s_over_box = make_box(s_root);
    lv_obj_t *t = make_label(s_over_box, FONT_BIG, 0xFF3B3B, "FIN DEL JUEGO");
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 140);
    s_over_score = make_label(s_over_box, FONT_MID, COLOR_HUD, "");
    lv_obj_align(s_over_score, LV_ALIGN_TOP_MID, 0, 200);
    s_over_hi = make_label(s_over_box, FONT_MID, COLOR_ACCENT, "");
    lv_obj_align(s_over_hi, LV_ALIGN_TOP_MID, 0, 234);
    s_over_blink = make_label(s_over_box, FONT_MID, COLOR_TITLE, "TOCA PARA JUGAR OTRA VEZ");
    lv_obj_align(s_over_blink, LV_ALIGN_TOP_MID, 0, 300);
    show(s_over_box, false);
}

bool game_shooter_is_open(void) { return s_open; }

void game_shooter_open(void)
{
    if (s_open) return;
    sprites_init();

    memset(s_en, 0, sizeof(s_en));
    memset(s_shot, 0, sizeof(s_shot));
    memset(s_eshot, 0, sizeof(s_eshot));
    memset(s_star, 0, sizeof(s_star));
    memset(s_boom, 0, sizeof(s_boom));
    s_paused = false;
    s_shown_score = -1;
    s_score = 0;
    s_lives = START_LIVES;
    s_wave = 1;

    s_prev_scr = lv_screen_active();

    s_scr = lv_obj_create(NULL);
    lv_obj_set_size(s_scr, GW, GH);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(s_scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);

    s_root = lv_obj_create(s_scr);
    lv_obj_remove_style_all(s_root);
    lv_obj_set_size(s_root, GW, GH);
    lv_obj_set_pos(s_root, 0, 0);
    lv_obj_set_style_bg_color(s_root, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, 0);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_root, root_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_root, root_event_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(s_root, root_event_cb, LV_EVENT_CLICKED, NULL);

    /* estrellas (fondo) */
    for (int i = 0; i < N_STARS; i++) {
        int big = (i % 6 == 0);
        uint32_t col = big ? 0xFFFFFF : (i % 3 ? 0x5A6080 : 0x9AA4B8);
        s_star[i].obj = make_rect(s_root, big ? 3 : 2, big ? 3 : 2, col, 0);
        s_star[i].x = (float)lv_rand(0, GW - 3);
        s_star[i].y = (float)lv_rand(0, GH);
        s_star[i].v = big ? (float)lv_rand(90, 140) : (float)lv_rand(25, 80);
        lv_obj_set_pos(s_star[i].obj, (int)s_star[i].x, (int)s_star[i].y);
    }

    for (int k = 0; k < N_ENEMIES; k++) {
        s_en[k].obj = make_sprite(s_root, &spr_enemy[0][0]);
        s_en[k].st  = EN_DEAD;
        show(s_en[k].obj, false);
    }
    for (int i = 0; i < N_SHOTS; i++) {
        s_shot[i].obj = make_rect(s_root, 4, 16, COLOR_SHOT, 2);
        show(s_shot[i].obj, false);
    }
    for (int i = 0; i < N_ESHOTS; i++) {
        s_eshot[i].obj = make_rect(s_root, 6, 12, COLOR_ENEMY_SHOT, 3);
        show(s_eshot[i].obj, false);
    }

    s_ship = make_sprite(s_root, &spr_ship);
    show(s_ship, false);

    for (int i = 0; i < N_BOOMS; i++) {
        s_boom[i].obj = make_sprite(s_root, &spr_boom[0]);
        show(s_boom[i].obj, false);
    }

    /* HUD */
    s_lbl_score = make_label(s_root, FONT_MID, COLOR_HUD, "PUNTOS 0");
    lv_obj_set_pos(s_lbl_score, 14, 14);
    s_lbl_wave = make_label(s_root, FONT_SMALL, 0x9AA4B8, "OLEADA 1");
    lv_obj_align(s_lbl_wave, LV_ALIGN_TOP_MID, 0, 18);
    for (int i = 0; i < N_LIFE_ICON; i++) {
        s_life_icon[i] = make_sprite(s_root, &spr_ship_mini);
        lv_obj_set_pos(s_life_icon[i], GW - 134 - (i + 1) * (spr_ship_mini.w + 6), 14);
        show(s_life_icon[i], false);
    }
    show(s_lbl_score, false); /* el HUD aparece al empezar a jugar */
    show(s_lbl_wave, false);
    s_lbl_msg = make_label(s_root, FONT_BIG, COLOR_ACCENT, "");
    lv_obj_align(s_lbl_msg, LV_ALIGN_CENTER, 0, 20);
    show(s_lbl_msg, false);

    build_title();
    build_game_over();

    s_btn_pause = make_hud_button(s_root, LV_SYMBOL_PAUSE, GW - 122, pause_btn_cb, &s_lbl_pause);
    show(s_btn_pause, false);
    make_hud_button(s_root, LV_SYMBOL_CLOSE, GW - 62, close_btn_cb, NULL);

    set_state(GS_TITLE);
    s_last_tick = lv_tick_get();
    s_timer = lv_timer_create(game_tick, 20, NULL);
    s_open = true;

    lv_screen_load_anim(s_scr, LV_SCREEN_LOAD_ANIM_FADE_IN, 200, 0, false);
}
