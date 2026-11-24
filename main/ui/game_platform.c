/*
 * ============================================================================
 *  PROYECTO AVENTURA — plataformas + disparos (BASE de prueba)
 * ============================================================================
 *
 * Mismo archivo para la placa (ESP32-P4) y el simulador de PC (sim/).
 * Solo LVGL, nada de hardware. Se abre con game_platform_open().
 *
 * Esta es la BASE: la jugabilidad (correr, saltar, disparar, enemigos,
 * colisiones, camara, vidas). Los graficos son PROVISIONALES: despues se
 * reemplazan por los diseños propios (personaje, tiles, fondo, titulo)
 * sin tocar la logica.
 *
 * Controles:
 *   Tactil:  botones en pantalla  <  >  ^ (apuntar arriba)  SALTO  DISP  CORRER
 *   Simulador (teclado): flechas o A/D mover, W/flecha arriba apuntar arriba,
 *                        Z o ESPACIO saltar, X disparar, C o SHIFT correr.
 *   Corriendo se salta mas alto y mas lejos (como en los plataformas clasicos).
 *
 * ---------------------------------------------------------------------------
 *  >>> EL NIVEL se dibuja con texto en LEVEL_1 (mas abajo). Cada letra es un
 *      bloque de 32x32 px. 15 filas = alto de la pantalla. El largo es libre
 *      (hasta 256 columnas). Leyenda:
 *        .  vacio              #  suelo / roca (solido)
 *        =  plataforma (solida) B  ladrillo (se rompe con la cabeza)
 *        ^  pinchos            o  moneda
 *        w  enemigo caminante  t  torreta (dispara)
 *        P  inicio del jugador C  punto de control (reaparecer)
 *        F  meta (bandera)
 *      Las filas 13 y 14 sin suelo son AGUA TOXICA: caer ahi cuesta una vida.
 *
 *  >>> TEMA ALCANTARILLA: las texturas (pared, piedra, baba, rejilla, ladrillo
 *      oxidado, agua) son dibujos ART_TEX_* que se repiten como azulejos.
 *      Se editan igual que los personajes (tambien desde tools/estudio_juegos.html).
 * ---------------------------------------------------------------------------
 */

#include "game_platform.h"
#include "lvgl.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 *  CONFIGURACION
 * ========================================================================== */

#define GAME_TITLE      "LA ALCANTARILLA"
#define GAME_SUBTITLE   "Acaba con la plaga de cucarachas"

static const char *const LEVEL_1[] = {
    "......................................................................................................................................................",
    "......................................................................................................................................................",
    "......................................................................................................................................................",
    "......................................................................................................................................................",
    "......................................................................................................................................................",
    "...........................................................................oooo.......................................................................",
    "...........................................................................====.......................................................................",
    "................................................................................................oooo..................................................",
    "............................ooooo......................o..............oooo......................BBBB........................oo........................",
    "............................BBBBB.........oo...........#..............====........oooo......................................##........................",
    ".........................................====.........##.....................................t.............................####.......................",
    ".....................................................###...........................==.......###...........................######......................",
    "..P...ooo.....w.......................t.............####......^^^...C...w....w..............###..w............^^^..w..w..########....w...........F....",
    "####################....#################....####################################......##############....#############################################",
    "####################....#################....####################################......##############....#############################################",
};

/* NIVEL 2 - EL TUNEL: tubo de concreto con techo, derrumbes y escombros.
 *   R  escombros (solido; se destruye a tiros, 2 disparos)
 *   r  basura suelta en el piso (decoracion, no molesta) */
static const char *const LEVEL_2[] = {
    "################################################################################################################################################################",
    "################################################################################################################################################################",
    "..............................################..............RRRR....................................################....RRR.....................................",
    "............................................................RRRR........................................................RRR.....................................",
    "............................................................RRRR........................................................RRR.....................................",
    "............................................................RRRR........................................................RRR.....................................",
    "............................................................RRRR........................................................RRR.....................................",
    "............................................................RRRR.....................................ooooo..t...........RRR......ooo............................",
    "................................................ooo.........RRRR........................oo..........===========.................................................",
    "..................oooo..................................................o........................................................===........RR..................",
    "................................................===.....................R...............==..................................................RR..................",
    ".....ooo..................RR...........................................RRR..................................................................RR..................",
    "..P.....r...w.............RRR....r..w...r.w.t...........r.w.......r...RRRRR...w.C.w............rw...........r......w..w................w....RR.....w..r.t...F...",
    "##################....#########################.....##################################......####################################.....###########################",
    "##################....#########################.....##################################......####################################.....###########################",
};

/* Fisica (pixeles y segundos). Ajustar para cambiar como "se siente". */
#define WALK_SPEED       230.0f   /* velocidad maxima caminando */
#define RUN_SPEED        370.0f   /* velocidad maxima con CORRER apretado */
#define RUN_ACCEL       1800.0f   /* que tan rapido arranca */
#define RUN_FRICTION    2200.0f   /* que tan rapido frena en el suelo */
#define AIR_ACCEL       1300.0f   /* control en el aire */
#define GRAVITY         1900.0f
#define JUMP_SPEED       720.0f   /* salto quieto/caminando (~4 bloques) */
#define JUMP_RUN_BONUS   120.0f   /* extra al saltar corriendo a toda velocidad (~5.5 bloques) */
#define JUMP_CUT         260.0f   /* soltar SALTO antes = salto mas bajo */
#define MAX_FALL         900.0f
#define STOMP_BOUNCE     430.0f   /* rebote al pisar un enemigo */
#define COYOTE_MS         90      /* margen para saltar justo al salir del borde */
#define JUMP_BUFFER_MS   110      /* margen si apretas SALTO un poco antes de caer */

#define FIRE_EVERY_MS    170
#define BULLET_SPEED     620.0f
#define WALKER_SPEED      60.0f
#define TURRET_HP          3
#define TURRET_FIRE_MS  1800
#define ENEMY_BULLET_SPD 230.0f

#define START_LIVES        3
#define START_HEARTS       3
#define COINS_FOR_LIFE    50

#define PTS_COIN          10
#define PTS_BRICK         50
#define PTS_WALKER       100
#define PTS_TURRET       300
#define PTS_GOAL        1000

/* Colores del escenario (lo que no es textura) */
#define C_BG_TOP      0x0B100D   /* fondo detras de todo (casi no se ve) */
#define C_BG_BOT      0x18241C
#define C_ARCH        0x070A08   /* tuneles oscuros de la pared del fondo */
#define C_ARCH_EDGE   0x3A4238   /* borde de piedra de los tuneles */
#define C_PIPE        0x5E6A63   /* caños */
#define C_PIPE_DARK   0x343C38
#define C_RUST        0x8A4B22   /* oxido de los caños */
#define C_LIGHT       0xFFD27A   /* luz de las lamparas */
#define C_DRIP        0x8FD18F   /* gotas que caen de los caños */
#define C_STREAM      0x5DBB4C   /* chorro de agua de los tuneles */
#define C_WATER_DEEP  0x0F3316   /* agua toxica (fondo) */
#define C_BULLET      0xFFE34A   /* disparo del jugador */
#define C_EBULLET     0x9CFF3A   /* acido que escupen las cucarachas */
/* nivel 2: tunel */
#define C_RING        0x9FB6CC   /* reflejo de luz en los anillos del tubo */
#define C_TUNNEL_DARK 0x05070A   /* oscuridad arriba/abajo del tubo y en los ramales */
#define C_TUNNEL_LIGHT 0xC8DAFF  /* luz al final de los ramales */
#define C_MURKY_DEEP  0x1E2410   /* agua turbia (fondo) */

/* Paleta de los dibujos (letra -> color) */
typedef struct { char key; uint32_t rgb; } pal_entry_t;
static const pal_entry_t PALETTE[] = {
    { 'W', 0xFFFFFF }, { 'K', 0x15151F }, { 'S', 0xF2C090 }, { 'R', 0xE0302A },
    { 'B', 0x2F6BFF }, { 'N', 0x5A3A1E }, { 'G', 0x8A93A6 }, { 'Y', 0xFFE34A },
    { 'O', 0xFF9A1F }, { 'P', 0x9B4DE0 }, { 'L', 0x3A3F55 }, { 'E', 0x9AA4B8 },
    /* alcantarilla */
    { 'D', 0x2B332E }, { 'd', 0x222925 }, { 'M', 0x121614 },   /* pared: ladrillo, sombra, junta */
    { 'T', 0x66705F }, { 't', 0x4A5246 }, { 'h', 0x838C78 },   /* piedra: base, sombra, brillo */
    { 'V', 0x7BD13A }, { 'v', 0x3F8A22 },                      /* baba verde */
    { 'A', 0x3E9E4A }, { 'a', 0x22662C },                      /* agua toxica */
    { 'X', 0x8A4B22 }, { 'x', 0x5C3216 },                      /* oxido */
    { 'Q', 0x7D8A8F }, { 'q', 0x4B565B },                      /* metal */
    { 'C', 0x7A4420 }, { 'c', 0x4A260F }, { 'H', 0xB57A45 },   /* cucaracha */
    { 'g', 0x9CFF3A },                                         /* acido */
    /* tunel de concreto (nivel 2) */
    { 'J', 0x3A4652 }, { 'j', 0x2A333D }, { 'k', 0x5A6977 },   /* concreto: base, sombra, brillo */
    { 'n', 0x5A4A2E },                                         /* manchas de humedad */
    { 'U', 0x7C8740 }, { 'u', 0x434A22 },                      /* agua turbia */
};

/* Jugador (mirando a la derecha; el espejo se genera solo) 12x16, x2 */
static const char *const ART_P_IDLE[] = {
    "....RRRR....", "...RRRRRRR..", "...SSSSS.RR.", "...SSSKS....",
    "...SSSSS....", "....SSS.....", "..BBBBBB....", ".BBBBBBGGGG.",
    ".SBBBBBGGGGG", "..BBBBBB....", "..NNNNNN....", "..NN..NN....",
    "..NN..NN....", "..NN..NN....", ".KKK..KKK...", "............",
};
static const char *const ART_P_RUN1[] = {
    "....RRRR....", "...RRRRRRR..", "...SSSSS.RR.", "...SSSKS....",
    "...SSSSS....", "....SSS.....", "..BBBBBB....", ".BBBBBBGGGG.",
    ".SBBBBBGGGGG", "..BBBBBB....", "..NNNNNN....", ".NN....NN...",
    "NN......NN..", "N........N..", "KK.......KK.", "............",
};
static const char *const ART_P_RUN2[] = {
    "....RRRR....", "...RRRRRRR..", "...SSSSS.RR.", "...SSSKS....",
    "...SSSSS....", "....SSS.....", "..BBBBBB....", ".BBBBBBGGGG.",
    ".SBBBBBGGGGG", "..BBBBBB....", "..NNNNNN....", "...NNNN.....",
    "...NN.NN....", "..NN...NN...", "..KK...KK...", "............",
};
static const char *const ART_P_JUMP[] = {
    "....RRRR....", "...RRRRRRR..", "...SSSSS.RR.", "...SSSKS....",
    "...SSSSS....", "....SSS.....", "..BBBBBB....", ".BBBBBBGGGG.",
    ".SBBBBBGGGGG", "..BBBBBB....", "..NNNNNN....", ".NNN..NNN...",
    ".KK....KK...", "............", "............", "............",
};
/* apuntando arriba */
static const char *const ART_P_UP[] = {
    "....RRRR.G..", "...RRRRRRG..", "...SSSSSGRR.", "...SSSKSG...",
    "...SSSSSS...", "....SSSS....", "..BBBBBB....", ".BBBBBBB....",
    ".SBBBBBB....", "..BBBBBB....", "..NNNNNN....", "..NN..NN....",
    "..NN..NN....", "..NN..NN....", ".KKK..KKK...", "............",
};

/* Cucaracha que camina (mirando a la derecha) 14x9, x2 — 2 cuadros: patas */
static const char *const ART_WALK1[] = {
    "..........c...",
    "...CCCCC...c..",
    ".CCHHCCCCc.c..",
    "CCHHCCCCCCCcc.",
    "CCCCCCCCCCCKcc",
    ".ccccccccccc..",
    ".c.c.c..c.c...",
    "c..c..c.c..c..",
    "..............",
};
static const char *const ART_WALK2[] = {
    "...........c..",
    "...CCCCC..c...",
    ".CCHHCCCCc.c..",
    "CCHHCCCCCCCcc.",
    "CCCCCCCCCCCKcc",
    ".ccccccccccc..",
    "..c.c.c..c.c..",
    ".c..c..c..c...",
    "..............",
};

/* Cucaracha escupidora (no camina, escupe acido) 16x16, x2 */
static const char *const ART_TURRET[] = {
    "....c......c....",
    ".....c....c.....",
    "......CCCC......",
    ".....CKCCKC.....",
    ".....CCggCC.....",
    "....cCCCCCCc....",
    "...c.CHHCCC.c...",
    "..c.CHHCCCCC.c..",
    ".c..CCCCCCCC..c.",
    "....CCCCCCCC....",
    "...cCCCCCCCCc...",
    "..c.CCCCCCCC.c..",
    ".c..cCCCCCCc..c.",
    ".....cccccc.....",
    "....c..cc..c....",
    "...c..c..c..c...",
};

/* Moneda 8x8, x2 (2 cuadros: gira) */
static const char *const ART_COIN1[] = {
    "..YYYY..", ".YYWYYY.", "YYWYYYYO", "YYWYYYYO", "YYWYYYYO", "YYYYYYYO", ".YYYYYO.", "..OOOO..",
};
static const char *const ART_COIN2[] = {
    "...YY...", "..YWYO..", "..YWYO..", "..YWYO..", "..YWYO..", "..YYYO..", "..YYYO..", "...OO...",
};

/* Pinchos 16x8, x2 (ocupan la mitad de abajo del bloque) */
static const char *const ART_SPIKES[] = {
    "...W.......W....", "...E.......E....", "..EEE.....EEE...", "..EEE.....EEE...",
    ".EEEEE...EEEEE..", ".EEEEE...EEEEE..", "EEEEEEE.EEEEEEE.", "LLLLLLLLLLLLLLLL",
};

/* Corazon 7x6, x3 */
static const char *const ART_HEART[] = {
    ".RR.RR.", "RWRRRRR", "RRRRRRR", ".RRRRR.", "..RRR..", "...R...",
};
static const char *const ART_HEART_EMPTY[] = {
    ".LL.LL.", "LLLLLLL", "LLLLLLL", ".LLLLL.", "..LLL..", "...L...",
};

/* Explosion 9x9, x3 */
static const char *const ART_PUFF1[] = {
    "....Y....", ".Y..O..Y.", "..OOYOO..", "..OYWYO..", "YOYWWWYOY",
    "..OYWYO..", "..OOYOO..", ".Y..O..Y.", "....Y....",
};
static const char *const ART_PUFF2[] = {
    "R...O...R", "..R...R..", "O..O.O..O", "...R.R...", ".O.....O.",
    "...R.R...", "O..O.O..O", "..R...R..", "R...O...R",
};

/* --------------------------- TEXTURAS DE LA ALCANTARILLA (16 px = 32 px en pantalla) */
/* pared de ladrillos del fondo (se repite) */
static const char *const ART_TEX_WALL[] = {
    "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM",
    "MdDddddddDdDDdDDMDDddddDdDDdDdDD",
    "MDDDDDDdDdDDDDDDMDDDDDDDDDDDDDDd",
    "MDDDDDdDDdDDDDDDMDDDDDDdDDDDDDDD",
    "MDDDdDDDdDDDDDDdMDdDDDDDDDDDDDDD",
    "MDDDDDDDDDDDDDDDMddDDDDDDDDDDDDd",
    "MDDDDdDDDDDDDDDdMDDdDDDdDDDDDdDD",
    "MdDdDDDdDDDDDdDDMDDdDdDdDdDDDddd",
    "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM",
    "DDDdDDDDMDddddDDddDDddDDMDDDdddd",
    "DDdDDDDDMDDDDDDDDDDDdDdDMdDdDDdD",
    "dDdDDDDDMDDDDDdDDdDddDDDMDDDDDDD",
    "DddDDDdDMDDDDDdDDDDDDDdDMDdDDDDd",
    "DdDDDDDDMDDDDDDDDDDdDDDDMDdDDDDD",
    "DDdDddDDMDdDDDdDDdDDddDDMDDDDDDD",
    "DDddDdDDMDdDDDdDdDDDDdDdMdddDdDd",
};
/* bloque de piedra del suelo (se repite) */
static const char *const ART_TEX_STONE[] = {
    "hhhhhhhhhhhhhhht",
    "hTtTtTTtTTTTTTTt",
    "hTTTTTTTtTTTTTTt",
    "hTTTTTTTTTtTTTtt",
    "hTTTTTTTTTTtTTTt",
    "hTTTtTtTttTTtTTt",
    "hTTTTTTTTTTtttTt",
    "hTTTTTTTTTTTTTTt",
    "hTTTTtTTtTTTTtTt",
    "hTtTtTTTTTTtTTTt",
    "hTTtTTTtTTtTTTTt",
    "hTTTTTTTTtTTTTtt",
    "hTTTTTTTTTTTTTTt",
    "hTTtTtTTTTTTTTTt",
    "hTTtTTTTTttTTTTt",
    "tttttttttttttttt",
};
/* baba verde encima del suelo (se repite) */
static const char *const ART_TEX_SLIME[] = {
    "VVVVVVVVVVVVVVVV",
    "VVVVVVVVVVVVVVVV",
    "VVvVVVVvVVVVvVVV",
    "vvv..vvvvv.vvv.v",
    ".v....v..v..v...",
    ".V.......V......",
};
/* rejilla metalica (plataforma, se repite) */
static const char *const ART_TEX_GRATE[] = {
    "QQQQQQQQQQQQQQQQ",
    "qqqqqqqqqqqqqqqq",
    "qQ..qQ..qQ..qQ..",
    "qQ..qQ..qQ..qQ..",
    "qQ..qQ..qQ..qQ..",
    "qQ..qQ..qQ..qQ..",
    "qQ..qQ..qQ..qQ..",
    "qQ..qQ..qQ..qQ..",
    "qQ..qQ..qQ..qQ..",
    "qqqqqqqqqqqqqqqq",
    "xXxXxXxXxXxXxXxX",
};
/* ladrillo oxidado que se rompe con la cabeza */
static const char *const ART_TEX_CRATE[] = {
    "XXXXXXXMXXXXXXXM",
    "xxxxxXxMXXxxxxxM",
    "xxxXxXxMxxxxxxXM",
    "MMMMMMMMMMMMMMMM",
    "XXXMXXXXXXXMXXXX",
    "xxxMxxxxxxxMxxxx",
    "xxxMxxxxXxxMxxxx",
    "MMMMMMMMMMMMMMMM",
    "XXXXXXXMXXXXXXXM",
    "xxxxxxxMxxxxxxxM",
    "xxxxxXxMxXxxxxxM",
    "MMMMMMMMMMMMMMMM",
    "XXXMXXXXXXXMXXXX",
    "XxxMxxxxxxXMxxxx",
    "xxxMxxxxxxxMXxxx",
    "MMMMMMMMMMMMMMMM",
};
/* superficie del agua toxica (se repite y se mueve) */
static const char *const ART_TEX_WATER[] = {
    "....AAA.....AAA.",
    "..AAaaaAA.AAaaaA",
    "AAaaaaaaaAaaaaaa",
    "aaaaaaAaaaaaaaaa",
    "aaaaaaAaaAaAaaAa",
    "aaaaaaaaaaaaaaaa",
    "aaaaaaaaaaaaaaAa",
    "aaaaaaaaaaaaaaaa",
};

/* --------------------------- TEXTURAS DEL TUNEL (nivel 2) */
/* concreto del tunel (pared del fondo, se repite) */
static const char *const ART_TEX_CONCRETE[] = {
    "JJJJJJJJjJJJJjJJjJJJJJJJJJJJkJJk",
    "jkJJJjjJJJJjJJjkJJJJJJJjJjjjJjJJ",
    "JJJjjJkJjJJJJJJJJJJJJkJJJJJJJJJk",
    "JJJjJJJJJJJJJJjJJJJJjJkJkJJnJJJJ",
    "JJJJJkJJJJJJJJjkJJJJJJjJJjJnJJJJ",
    "JJJJJjJkJJJJJJkJJjJJkJJJjJJnnJJJ",
    "JJjkJJJkJJJJkkJJkJJkjJJJJjJnnJJj",
    "JMjjJJkJJjkJJJJjJJJkJJJJjJJnJJkJ",
    "JjjJJJkJJJjjJjJJJJJJJJjJJjknJJkJ",
    "JJjJJJJJkJJJJJJJJjJJjJJJJJJnnJJJ",
    "JJJJjJJJJnnJJkjJJJJJkJJJJJJnnJJJ",
    "JJjJJJjJJnjJJJJJJJJJjJJJJJJnnJJj",
    "JJJJJjJJjnnJkjJJJjjJjJJJJJJnnjJJ",
    "JJJJjJJjJnnJjJJjJkjJJJJJJJjnnJJJ",
    "JJJjJJJJknnJJJjJkJJjJJJJjJJnnJJJ",
    "JJJjkJjJjnnJJJJjJjjjJJJJJJJnnJJj",
    "JJJJJjJJjnnJJJJJJJJJJJJJJkknMjJJ",
    "JJJJJJJjJnnJJJjjJkJJkJJJJjJnjJJJ",
    "JJjjjMjJJnJkJJjJJkJJJJjkJJJnjJjk",
    "JJJJjjJJjnJJJJJJJJJJJJJJJJJnnkJJ",
    "kJJJJjJJknnJJMjJJJkJJJkjJJJnnJjJ",
    "kJjJkkJJJnjJkjJkJJJJJjJJJjJnJJJJ",
    "JJJjjkjJJnJjJJJJjkJjJJjJJJJnnJJj",
    "jJJJJjJkJnJJJJJJJJkJjJJkJJjnkJJJ",
    "kJJjJJkJJnnJjJjJJjJjJkjkJkJnnJJJ",
    "JJJJJJJJJnnJJJJJJJJJJJJjJJjJjjJk",
    "JJjJJJJjknnJjJJJJJJjjkJjJJkJJJjk",
    "kJJJkJJkJnnJJJjJjjJkJjMjJJJJJJJJ",
    "JJJJJjJJJJjJJjJjJJkJjJjJJkJJJJJJ",
    "JjJJjJJkjJJJJJMjJJJkJJJJJjJJJjJJ",
    "JJJJJJJJJJJJJJjJJJjJJJJJJkJJJJJJ",
    "JkJJJJJJJJkkJjJJJJJJJJJJJJJkJkkJ",
};
/* piso / techo de concreto del tunel (se repite) */
static const char *const ART_TEX_CFLOOR[] = {
    "kkkkkkkkkkkkkkkk",
    "kkJJJJkJJJkkJJJJ",
    "JjJjnJJJJJjJJjjJ",
    "JJJjJJJJJJjJJJJJ",
    "JJJJJJjJJJJJjJJJ",
    "JJJJJJJJjJJnJJJJ",
    "JjJJjJJJJJJJjjJJ",
    "JJjjJJJJnJJjJJJj",
    "JJJJjJJJJnJJJJJJ",
    "JJJJJJJJJJJJjJJJ",
    "jjJJJjJJJJJJJJjJ",
    "JJJJJJJJJJjJJJjJ",
    "JjJJjJJjjjJJJJJJ",
    "JjJJJJJJJJJJJJjJ",
    "JJJjJJJJJJJJJJJJ",
    "jjjjjjjjjjjjjjjj",
};
/* escombros (bloque solido; se destruye a tiros) */
static const char *const ART_TEX_RUBBLE[] = {
    "...k..kkkkJJJJJ.",
    ".kkkJJkkkJJnJnk.",
    "kkkJJJkkJJJJkkkJ",
    "kkJJJkkJJJJJkkJJ",
    "kJJJJJJnJJJkkJJJ",
    "JJJJJMkJnJJJJJJJ",
    "JJnJkkkJJJJMJJJM",
    "jJJkkkJJJJMkkkMJ",
    "jjkkkJJJJnkkkJJn",
    "kkkJJJnJnMkknJJJ",
    "kkJJJJJJMkkJJJJJ",
    "kJJnJJJMMkJJJJJM",
    "JJnJJMMkkknJJJMM",
    "JJJJMMMkknnJJMMM",
    "JJJMMMkkJnJJMMjj",
    "JJMMMjjJJJJMjjjj",
};
/* agua turbia del tunel (se repite y se mueve) */
static const char *const ART_TEX_WATER2[] = {
    "...UUU......UUU.",
    ".UUuuuUU..UUuuuU",
    "UUuuuuuuuUuuuuuu",
    "Uuuuuuuuuuuuuuuu",
    "uuuuuuuuuuuuuuuu",
    "uuuuuuuUuuuUuuuu",
    "uuuUuuuuuUuuuuuu",
    "uuuuuuuuuuuUUuuu",
};
/* basura y piedritas sueltas en el piso (decoracion) */
static const char *const ART_DEBRIS[] = {
    "................",
    "......WW........",
    "......WnW...k...",
    "..kk...W....kJ..",
    ".kJJj..G....JJj.",
    "kJJjjj.GG.kkJjjj",
    "jjjjjj.kJJjjjjjj",
    "................",
};

/* ============================================================================
 *  (de aca para abajo es la logica del juego)
 * ========================================================================== */

#define GW        800
#define GH        480
#define T         32
#define MAP_H     15
#define MAP_MAXW  256

#define MAX_ENEMIES  40
#define MAX_COINS   128
#define MAX_BRICKS   64
#define MAX_PB        8
#define MAX_EB       12
#define MAX_PUFF      8

#define ART_ROWS(a) ((int)(sizeof(a) / sizeof((a)[0])))
#define MAX_RUBBLE  128

/* Niveles en orden: dibujo, tema visual y nombre que se muestra al empezar */
typedef enum { THEME_SEWER, THEME_TUNNEL } theme_t;
typedef struct { const char *const *rows; int nrows; theme_t theme; const char *name; } level_def_t;
static const level_def_t LEVELS[] = {
    { LEVEL_1, ART_ROWS(LEVEL_1), THEME_SEWER,  "NIVEL 1 - LAS CLOACAS" },
    { LEVEL_2, ART_ROWS(LEVEL_2), THEME_TUNNEL, "NIVEL 2 - EL TUNEL" },
};
#define N_LEVELS ((int)(sizeof(LEVELS) / sizeof(LEVELS[0])))

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

typedef struct { lv_image_dsc_t dsc; uint8_t *buf; int w, h; } sprite_t;

enum { PF_IDLE, PF_RUN1, PF_RUN2, PF_JUMP, PF_UP, PF_COUNT };
static sprite_t spr_player[PF_COUNT][2]; /* [cuadro][0=der,1=izq] */
static sprite_t spr_walk[2][2];          /* [cuadro][dir] */
static sprite_t spr_turret, spr_coin[2], spr_spikes, spr_heart, spr_heart_empty, spr_puff[2];
static sprite_t spr_tex_wall, spr_tex_stone, spr_tex_slime, spr_tex_grate, spr_tex_crate, spr_tex_water;
static sprite_t spr_tex_concrete, spr_tex_cfloor, spr_tex_rubble, spr_tex_water2, spr_debris;
static bool     s_sprites_ready;

static bool pal_lookup(char c, uint32_t *rgb)
{
    for (size_t i = 0; i < sizeof(PALETTE) / sizeof(PALETTE[0]); i++)
        if (PALETTE[i].key == c) { *rgb = PALETTE[i].rgb; return true; }
    return false;
}

static void sprite_build(sprite_t *s, const char *const *art, int rows, int scale, bool mirror)
{
    int cols = 0;
    for (int r = 0; r < rows; r++) { int l = (int)strlen(art[r]); if (l > cols) cols = l; }
    s->w = cols * scale;
    s->h = rows * scale;
    uint32_t stride = (uint32_t)s->w * 4, size = stride * (uint32_t)s->h;
    s->buf = lv_malloc(size);
    if (!s->buf) { s->w = s->h = 0; return; }
    memset(s->buf, 0, size);
    for (int r = 0; r < rows; r++) {
        int len = (int)strlen(art[r]);
        for (int c = 0; c < len; c++) {
            uint32_t rgb;
            if (!pal_lookup(art[r][c], &rgb)) continue;
            int dc = mirror ? (cols - 1 - c) : c;
            for (int dy = 0; dy < scale; dy++) {
                uint8_t *p = s->buf + (uint32_t)(r * scale + dy) * stride + (uint32_t)(dc * scale) * 4;
                for (int dx = 0; dx < scale; dx++, p += 4) {
                    p[0] = rgb & 0xFF; p[1] = (rgb >> 8) & 0xFF; p[2] = (rgb >> 16) & 0xFF; p[3] = 0xFF;
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

#define BUILD(s, a, sc, m) sprite_build(&(s), a, ART_ROWS(a), sc, m)

static void sprites_init(void)
{
    if (s_sprites_ready) return;
    for (int m = 0; m < 2; m++) {
        BUILD(spr_player[PF_IDLE][m], ART_P_IDLE, 2, m);
        BUILD(spr_player[PF_RUN1][m], ART_P_RUN1, 2, m);
        BUILD(spr_player[PF_RUN2][m], ART_P_RUN2, 2, m);
        BUILD(spr_player[PF_JUMP][m], ART_P_JUMP, 2, m);
        BUILD(spr_player[PF_UP][m],   ART_P_UP,   2, m);
        BUILD(spr_walk[0][m], ART_WALK1, 2, m);
        BUILD(spr_walk[1][m], ART_WALK2, 2, m);
    }
    BUILD(spr_turret, ART_TURRET, 2, false);
    BUILD(spr_coin[0], ART_COIN1, 2, false);
    BUILD(spr_coin[1], ART_COIN2, 2, false);
    BUILD(spr_spikes, ART_SPIKES, 2, false);
    BUILD(spr_heart, ART_HEART, 3, false);
    BUILD(spr_heart_empty, ART_HEART_EMPTY, 3, false);
    BUILD(spr_puff[0], ART_PUFF1, 3, false);
    BUILD(spr_puff[1], ART_PUFF2, 3, false);
    BUILD(spr_tex_wall,  ART_TEX_WALL,  2, false);
    BUILD(spr_tex_stone, ART_TEX_STONE, 2, false);
    BUILD(spr_tex_slime, ART_TEX_SLIME, 2, false);
    BUILD(spr_tex_grate, ART_TEX_GRATE, 2, false);
    BUILD(spr_tex_crate, ART_TEX_CRATE, 2, false);
    BUILD(spr_tex_water, ART_TEX_WATER, 2, false);
    BUILD(spr_tex_concrete, ART_TEX_CONCRETE, 2, false);
    BUILD(spr_tex_cfloor,   ART_TEX_CFLOOR,   2, false);
    BUILD(spr_tex_rubble,   ART_TEX_RUBBLE,   2, false);
    BUILD(spr_tex_water2,   ART_TEX_WATER2,   2, false);
    BUILD(spr_debris,       ART_DEBRIS,       2, false);
    s_sprites_ready = true;
}

/* --------------------------------------------------------------- entidades */

typedef enum { EN_WALKER, EN_TURRET } en_type_t;

typedef struct {
    lv_obj_t *obj;
    en_type_t type;
    bool      alive, on_ground;
    int       hp, frame, dir;
    float     x, y, vx, vy, w, h;
    int       timer_ms, flash_ms;
} enemy_t;

typedef struct { lv_obj_t *obj; bool on; float x, y, vx, vy; int w, h; } bullet_t;
typedef struct { lv_obj_t *obj; bool on; float x, y; } coin_t;
typedef struct { lv_obj_t *obj; int tx, ty; bool on; } brick_t;
typedef struct { lv_obj_t *obj; int tx, ty, hp; bool on; } rubble_t;
typedef struct { lv_obj_t *obj; bool on; int age, frame; } puff_t;

typedef struct {
    float x, y, vx, vy;
    float w, h;
    bool  on_ground, alive;
    int   facing;          /* 1 derecha, -1 izquierda */
    int   coyote_ms, jbuf_ms, fire_ms, invuln_ms, knock_ms, anim_ms;
    int   hearts;
} player_t;

typedef enum { GS_TITLE, GS_PLAY, GS_DYING, GS_CLEAR, GS_OVER } game_state_t;

/* ------------------------------------------------------------------ estado */

static bool         s_open;
static lv_obj_t    *s_prev_scr, *s_scr, *s_root, *s_far, *s_mid, *s_world;
static lv_timer_t  *s_timer;

static char         s_map[MAP_H][MAP_MAXW + 1];
static int          s_map_w;
static float        s_start_x, s_start_y, s_cp_x, s_cp_y;
static float        s_flag_x;
static lv_obj_t    *s_cp_head;
static bool         s_cp_taken;
static float        s_cp_tile_x;

static player_t     P;
static lv_obj_t    *s_player_obj;
static enemy_t      s_en[MAX_ENEMIES];  static int s_n_en;
static coin_t       s_coin[MAX_COINS];  static int s_n_coin;
static brick_t      s_brick[MAX_BRICKS]; static int s_n_brick;
static rubble_t     s_rubble[MAX_RUBBLE]; static int s_n_rubble;
static int          s_level;            /* indice en LEVELS[] */
static theme_t      s_theme;
static int          s_banner_ms;        /* cuanto tiempo mas se muestra el nombre del nivel */
static lv_obj_t    *s_vig_top, *s_vig_bot; /* sombra arriba/abajo del tubo (nivel tunel) */
static bullet_t     s_pb[MAX_PB];
static bullet_t     s_eb[MAX_EB];
static puff_t       s_puff[MAX_PUFF];

static game_state_t s_state;
static int          s_state_ms;
static bool         s_paused;
static uint32_t     s_last_tick;
static float        s_cam;
static int          s_score, s_coins, s_lives;
static int          s_shown_score = -1, s_shown_coins = -1, s_shown_lives = -1, s_shown_hearts = -1;
static int          s_anim_ms, s_anim_frame;

static uint8_t      s_touch_in, s_ext_in, s_prev_in;

/* HUD / overlays */
static lv_obj_t *s_lbl_score, *s_lbl_coins, *s_lbl_lives, *s_heart[START_HEARTS];
static lv_obj_t *s_lbl_msg, *s_lbl_sub, *s_btn_pause, *s_lbl_pause, *s_title_box, *s_title_blink;
static lv_obj_t *s_ctrl_box, *s_hud_bg;

/* --------------------------------------------------------------- helpers */

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static void show(lv_obj_t *o, bool on)
{
    if (!o) return;
    if (on) lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
}
static bool overlap(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh)
{
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

static lv_obj_t *make_rect(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color, int radius)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
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
static lv_obj_t *make_label(lv_obj_t *parent, const lv_font_t *f, uint32_t color, const char *txt)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(l, txt);
    lv_obj_remove_flag(l, LV_OBJ_FLAG_CLICKABLE);
    return l;
}
static lv_obj_t *make_container(lv_obj_t *parent, int w, int h)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, 0, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

/* --------------------------------------------------------------- mapa */

static bool is_solid_ch(char c) { return c == '#' || c == '=' || c == 'B' || c == 'R'; }

static char map_at(int tx, int ty)
{
    if (ty < 0 || ty >= MAP_H || tx < 0 || tx >= s_map_w) return '.';
    return s_map[ty][tx];
}

static bool solid_at(int tx, int ty)
{
    if (tx < 0 || tx >= s_map_w) return true;   /* paredes invisibles en los extremos */
    if (ty < 0 || ty >= MAP_H) return false;    /* arriba: aire; abajo: vacio (caida) */
    return is_solid_ch(s_map[ty][tx]);
}

static int fl(float v) { return (int)floorf(v / T); }

static bool box_solid(float x, float y, float w, float h)
{
    int tx0 = fl(x), tx1 = fl(x + w - 0.01f), ty0 = fl(y), ty1 = fl(y + h - 0.01f);
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++)
            if (solid_at(tx, ty)) return true;
    return false;
}

/* mueve una caja con colision contra el mapa; devuelve bits: 1=pared X, 2=suelo, 4=techo */
static int move_box(float *x, float *y, float w, float h, float *vx, float *vy, float dt)
{
    int hit = 0;
    float nx = *x + *vx * dt;
    if (box_solid(nx, *y, w, h)) {
        if (*vx > 0) nx = (float)(fl(nx + w) * T) - w - 0.01f;
        else         nx = (float)((fl(nx) + 1) * T);
        *vx = 0;
        hit |= 1;
    }
    *x = nx;
    float ny = *y + *vy * dt;
    if (box_solid(*x, ny, w, h)) {
        if (*vy > 0) { ny = (float)(fl(ny + h) * T) - h - 0.01f; hit |= 2; }
        else         { ny = (float)((fl(ny) + 1) * T); hit |= 4; }
        *vy = 0;
    }
    *y = ny;
    return hit;
}

/* ------------------------------------------------------------- efectos */

static void puff_at(float cx, float cy)
{
    for (int i = 0; i < MAX_PUFF; i++) {
        if (s_puff[i].on) continue;
        s_puff[i].on = true; s_puff[i].age = 0; s_puff[i].frame = 0;
        lv_image_set_src(s_puff[i].obj, &spr_puff[0].dsc);
        lv_obj_set_pos(s_puff[i].obj, (int)(cx - spr_puff[0].w / 2), (int)(cy - spr_puff[0].h / 2));
        lv_obj_move_foreground(s_puff[i].obj);
        show(s_puff[i].obj, true);
        return;
    }
}

static void msg(const char *big, const char *small)
{
    if (big) { lv_label_set_text(s_lbl_msg, big); show(s_lbl_msg, true); } else show(s_lbl_msg, false);
    if (small) { lv_label_set_text(s_lbl_sub, small); show(s_lbl_sub, true); } else show(s_lbl_sub, false);
}

static void set_state(game_state_t st) { s_state = st; s_state_ms = 0; }

/* ------------------------------------------------------------- armado del nivel */

/* imagen que se repite como azulejo hasta llenar w x h */
static lv_obj_t *make_tiled(lv_obj_t *parent, const sprite_t *s, int x, int y, int w, int h)
{
    lv_obj_t *o = lv_image_create(parent);
    lv_image_set_src(o, &s->dsc);
    lv_image_set_inner_align(o, LV_IMAGE_ALIGN_TILE);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

#define N_DRIPS 10
typedef struct { lv_obj_t *obj; float x, y, y0, y1, v; } drip_t;
static drip_t    s_drip[N_DRIPS];
static int       s_n_drip;
static lv_obj_t *s_water;
static float     s_water_off;

static void build_bg_sewer(void)
{
    /* LEJOS (se mueve a 1/4): pared de ladrillos, tuneles con chorros, caño grande y lamparas */
    int far_w = (int)(s_map_w * T * 0.25f) + GW + 200;
    s_far = make_container(s_root, far_w, GH);
    make_tiled(s_far, &spr_tex_wall, 0, 0, far_w, GH);
    for (int x = 120, i = 0; x < far_w; x += 460, i++) {
        int w = 150 + (i % 2) * 40, h = 230;
        lv_obj_t *arch = make_rect(s_far, x, GH - 60 - h, w, h + 80, C_ARCH, 70);
        lv_obj_set_style_border_color(arch, lv_color_hex(C_ARCH_EDGE), 0);
        lv_obj_set_style_border_width(arch, 8, 0);
        lv_obj_t *st = make_rect(s_far, x + w / 2 - 10, GH - 60 - 70, 20, 80, C_STREAM, 4); /* chorro */
        lv_obj_set_style_bg_opa(st, 150, 0);
    }
    lv_obj_t *pipe = make_rect(s_far, 0, 54, far_w, 34, C_PIPE, 0);       /* caño grande */
    lv_obj_set_style_border_color(pipe, lv_color_hex(C_PIPE_DARK), 0);
    lv_obj_set_style_border_width(pipe, 6, 0);
    lv_obj_set_style_border_side(pipe, LV_BORDER_SIDE_BOTTOM, 0);
    for (int x = 60; x < far_w; x += 210) {
        make_rect(s_far, x, 48, 16, 46, C_PIPE_DARK, 2);                  /* uniones */
        make_rect(s_far, x + 30, 88, 26, 10, C_RUST, 3);                  /* oxido */
    }
    for (int x = 300; x < far_w; x += 520) {                              /* lamparas */
        lv_obj_t *g1 = make_rect(s_far, x - 120, 30, 240, 240, C_LIGHT, LV_RADIUS_CIRCLE);
        lv_obj_set_style_bg_opa(g1, 22, 0);
        lv_obj_t *g2 = make_rect(s_far, x - 55, 95, 110, 110, C_LIGHT, LV_RADIUS_CIRCLE);
        lv_obj_set_style_bg_opa(g2, 38, 0);
        make_rect(s_far, x - 3, 94, 6, 46, C_PIPE_DARK, 0);
        make_rect(s_far, x - 14, 138, 28, 14, C_LIGHT, 6);
    }

    /* MEDIO (se mueve a 1/2): caños verticales que gotean */
    int mid_w = (int)(s_map_w * T * 0.5f) + GW + 200;
    s_mid = make_container(s_root, mid_w, GH);
    s_n_drip = 0;
    for (int x = 200, i = 0; x < mid_w; x += 380, i++) {
        int h = 110 + (i * 47) % 120;
        lv_obj_t *p = make_rect(s_mid, x, 0, 40, h, C_PIPE, 0);
        lv_obj_set_style_border_color(p, lv_color_hex(C_PIPE_DARK), 0);
        lv_obj_set_style_border_width(p, 6, 0);
        lv_obj_set_style_border_side(p, LV_BORDER_SIDE_RIGHT, 0);
        make_rect(s_mid, x - 6, h - 16, 52, 16, C_PIPE_DARK, 3);         /* boca */
        make_rect(s_mid, x + 4, h - 44, 14, 18, C_RUST, 3);
        if (s_n_drip < N_DRIPS) {
            drip_t *d = &s_drip[s_n_drip++];
            d->x = (float)(x + 18); d->y0 = (float)h; d->y1 = (float)(GH - 70);
            d->y = d->y0 + (float)lv_rand(0, 200);
            d->v = (float)lv_rand(160, 260);
            d->obj = make_rect(s_mid, (int)d->x, (int)d->y, 4, 9, C_DRIP, 2);
        }
    }
}

/* NIVEL TUNEL: tubo de concreto visto de costado. Pared de concreto con los
 * anillos de las uniones reflejando luz, y cada tanto un ramal que muestra la
 * vista "hacia adentro" del tubo: anillos que se achican hasta una luz. */
static void build_bg_tunnel(void)
{
    int far_w = (int)(s_map_w * T * 0.25f) + GW + 200;
    s_far = make_container(s_root, far_w, GH);
    make_tiled(s_far, &spr_tex_concrete, 0, 0, far_w, GH);

    /* uniones del tubo principal: arcos de luz */
    for (int x = 80; x < far_w; x += 230) {
        lv_obj_t *a = make_rect(s_far, x, -120, 110, GH + 240, 0, LV_RADIUS_CIRCLE);
        lv_obj_set_style_bg_opa(a, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(a, lv_color_hex(C_RING), 0);
        lv_obj_set_style_border_width(a, 4, 0);
        lv_obj_set_style_border_opa(a, 90, 0);
        lv_obj_set_style_border_side(a, LV_BORDER_SIDE_LEFT, 0);
        lv_obj_t *s = make_rect(s_far, x + 7, -120, 110, GH + 240, 0, LV_RADIUS_CIRCLE);
        lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(s, lv_color_hex(C_TUNNEL_DARK), 0);
        lv_obj_set_style_border_width(s, 4, 0);
        lv_obj_set_style_border_opa(s, 140, 0);
        lv_obj_set_style_border_side(s, LV_BORDER_SIDE_LEFT, 0);
    }

    /* ramales con luz al final */
    static const uint32_t fill[] = { 0x080B10, 0x0E141C, 0x162130, 0x223447, 0x34506E, 0x4F729A };
    static const uint8_t  ring_opa[] = { 150, 130, 150, 175, 200, 230 };
    for (int cx = 330; cx < far_w; cx += 880) {
        int cy = 236, size = 300;
        for (int k = 0; k < 6; k++) {
            lv_obj_t *c = make_rect(s_far, cx - size / 2, cy - size / 2 + k * 6, size, size, fill[k], LV_RADIUS_CIRCLE);
            lv_obj_set_style_border_color(c, lv_color_hex(C_RING), 0);
            lv_obj_set_style_border_width(c, k == 0 ? 7 : 3, 0);
            lv_obj_set_style_border_opa(c, ring_opa[k], 0);
            size = size * 7 / 10;
        }
        lv_obj_t *glow = make_rect(s_far, cx - 34, cy - 34 + 36, 68, 68, C_TUNNEL_LIGHT, LV_RADIUS_CIRCLE);
        lv_obj_set_style_bg_opa(glow, 120, 0);
        make_rect(s_far, cx - 12, cy - 12 + 36, 24, 24, 0xFFFFFF, LV_RADIUS_CIRCLE);
        /* reflejo del agua que sale del ramal */
        lv_obj_t *wr = make_rect(s_far, cx - 10, cy + 60, 20, 90, C_TUNNEL_LIGHT, 4);
        lv_obj_set_style_bg_opa(wr, 50, 0);
    }
    /* el fondo un poco mas oscuro para que el piso y los escombros resalten */
    lv_obj_t *dim = make_rect(s_far, 0, 0, far_w, GH, C_TUNNEL_DARK, 0);
    lv_obj_set_style_bg_opa(dim, 80, 0);

    /* MEDIO: manchas de oxido/humedad que bajan del techo y goteras */
    int mid_w = (int)(s_map_w * T * 0.5f) + GW + 200;
    s_mid = make_container(s_root, mid_w, GH);
    s_n_drip = 0;
    for (int x = 150, i = 0; x < mid_w; x += 260, i++) {
        lv_obj_t *st = make_rect(s_mid, x, 64, 10 + (i % 3) * 6, 80 + (i * 37) % 120, C_RUST, 5);
        lv_obj_set_style_bg_opa(st, 70, 0);
        if (s_n_drip < N_DRIPS && i % 2 == 0) {
            drip_t *d = &s_drip[s_n_drip++];
            d->x = (float)(x + 3); d->y0 = 64; d->y1 = (float)(GH - 70);
            d->y = d->y0 + (float)lv_rand(0, 250);
            d->v = (float)lv_rand(180, 280);
            d->obj = make_rect(s_mid, (int)d->x, (int)d->y, 4, 9, C_RING, 2);
        }
    }
}

static void build_background(void)
{
    if (s_theme == THEME_TUNNEL) build_bg_tunnel();
    else                         build_bg_sewer();
}

/* sombra fija arriba y abajo de la pantalla: da la sensacion de estar dentro del tubo */
static void build_vignette(void)
{
    if (s_vig_top) { lv_obj_delete(s_vig_top); s_vig_top = NULL; }
    if (s_vig_bot) { lv_obj_delete(s_vig_bot); s_vig_bot = NULL; }
    if (s_theme != THEME_TUNNEL) return;
    s_vig_top = make_rect(s_root, 0, 0, GW, 170, C_TUNNEL_DARK, 0);
    lv_obj_set_style_bg_grad_color(s_vig_top, lv_color_hex(C_TUNNEL_DARK), 0);
    lv_obj_set_style_bg_grad_dir(s_vig_top, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_main_opa(s_vig_top, 210, 0);
    lv_obj_set_style_bg_grad_opa(s_vig_top, 0, 0);
    s_vig_bot = make_rect(s_root, 0, GH - 70, GW, 70, C_TUNNEL_DARK, 0);
    lv_obj_set_style_bg_grad_color(s_vig_bot, lv_color_hex(C_TUNNEL_DARK), 0);
    lv_obj_set_style_bg_grad_dir(s_vig_bot, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_main_opa(s_vig_bot, 0, 0);
    lv_obj_set_style_bg_grad_opa(s_vig_bot, 90, 0);
    lv_obj_move_to_index(s_vig_top, 3);
    lv_obj_move_to_index(s_vig_bot, 4);
}

static void build_tiles(void)
{
    bool tunnel = s_theme == THEME_TUNNEL;
    const sprite_t *ground = tunnel ? &spr_tex_cfloor : &spr_tex_stone;
    const sprite_t *water  = tunnel ? &spr_tex_water2 : &spr_tex_water;

    /* agua a lo largo de todo el nivel (el suelo la tapa donde hay piso) */
    make_rect(s_world, 0, 13 * T + 12, s_map_w * T, GH - 13 * T - 12, tunnel ? C_MURKY_DEEP : C_WATER_DEEP, 0);
    s_water = make_tiled(s_world, water, 0, 13 * T + 4, s_map_w * T, water->h);
    s_water_off = 0;

    for (int r = 0; r < MAP_H; r++) {
        int c = 0;
        while (c < s_map_w) {
            char ch = s_map[r][c];
            if (ch != '#' && ch != '=') { c++; continue; }
            int e = c;
            while (e < s_map_w && s_map[r][e] == ch) e++;
            if (ch == '#') make_tiled(s_world, ground, c * T, r * T, (e - c) * T, T);
            else           make_tiled(s_world, &spr_tex_grate, c * T, r * T, (e - c) * T, spr_tex_grate.h);
            c = e;
        }
        if (tunnel) { /* en el tunel no hay baba: borde de concreto iluminado */
            c = 0;
            while (c < s_map_w) {
                bool cap = s_map[r][c] == '#' && !is_solid_ch(map_at(c, r - 1));
                if (!cap) { c++; continue; }
                int e = c;
                while (e < s_map_w && s_map[r][e] == '#' && !is_solid_ch(map_at(e, r - 1))) e++;
                lv_obj_t *lip = make_rect(s_world, c * T, r * T, (e - c) * T, 4, C_RING, 1);
                lv_obj_set_style_bg_opa(lip, 200, 0);
                c = e;
            }
            continue;
        }
        /* baba verde sobre el suelo que tiene aire arriba */
        c = 0;
        while (c < s_map_w) {
            bool cap = s_map[r][c] == '#' && !is_solid_ch(map_at(c, r - 1));
            if (!cap) { c++; continue; }
            int e = c;
            while (e < s_map_w && s_map[r][e] == '#' && !is_solid_ch(map_at(e, r - 1))) e++;
            make_tiled(s_world, &spr_tex_slime, c * T, r * T - 2, (e - c) * T, spr_tex_slime.h);
            c = e;
        }
    }
}

static void load_level(int idx)
{
    const char *const *rows = LEVELS[idx].rows;
    int nrows = LEVELS[idx].nrows;
    s_level = idx;
    s_theme = LEVELS[idx].theme;
    memset(s_map, '.', sizeof(s_map));
    s_map_w = 0;
    for (int r = 0; r < MAP_H; r++) {
        int len = (r < nrows) ? (int)strlen(rows[r]) : 0;
        if (len > MAP_MAXW) len = MAP_MAXW;
        if (len > s_map_w) s_map_w = len;
        if (r < nrows) memcpy(s_map[r], rows[r], (size_t)len);
    }
    if (s_map_w < GW / T) s_map_w = GW / T;
    for (int r = 0; r < MAP_H; r++) s_map[r][s_map_w] = 0;

    if (s_world) lv_obj_delete(s_world);
    if (s_far) lv_obj_delete(s_far);
    if (s_mid) lv_obj_delete(s_mid);
    s_world = s_far = s_mid = NULL;

    build_background();
    s_world = make_container(s_root, s_map_w * T, GH);
    /* fondo lejano, colinas y mundo siempre detras del HUD y los botones */
    lv_obj_move_to_index(s_far, 0);
    lv_obj_move_to_index(s_mid, 1);
    lv_obj_move_to_index(s_world, 2);
    build_vignette();
    build_tiles();

    s_n_en = s_n_coin = s_n_brick = s_n_rubble = 0;
    s_start_x = 2 * T; s_start_y = 10 * T;
    s_flag_x = -1000;
    s_cp_head = NULL; s_cp_taken = false;

    for (int r = 0; r < MAP_H; r++) {
        for (int c = 0; c < s_map_w; c++) {
            char ch = s_map[r][c];
            float px = (float)(c * T), py = (float)(r * T);
            switch (ch) {
            case 'P':
                s_start_x = px + 6; s_start_y = py + T - 30 - 0.01f;
                s_map[r][c] = '.';
                break;
            case 'o':
                if (s_n_coin < MAX_COINS) {
                    coin_t *k = &s_coin[s_n_coin++];
                    k->x = px + 8; k->y = py + 8; k->on = true;
                    k->obj = make_sprite(s_world, &spr_coin[0]);
                    lv_obj_set_pos(k->obj, (int)k->x, (int)k->y);
                }
                s_map[r][c] = '.';
                break;
            case 'B':
                if (s_n_brick < MAX_BRICKS) {
                    brick_t *b = &s_brick[s_n_brick++];
                    b->tx = c; b->ty = r; b->on = true;
                    b->obj = make_sprite(s_world, &spr_tex_crate);
                    lv_obj_set_pos(b->obj, c * T, r * T);
                } else {
                    s_map[r][c] = '#';
                }
                break;
            case 'R':
                if (s_n_rubble < MAX_RUBBLE) {
                    rubble_t *rb = &s_rubble[s_n_rubble++];
                    rb->tx = c; rb->ty = r; rb->hp = 2; rb->on = true;
                    rb->obj = make_sprite(s_world, &spr_tex_rubble);
                    lv_obj_set_pos(rb->obj, c * T, r * T);
                } else {
                    s_map[r][c] = '#';
                }
                break;
            case 'r': {
                lv_obj_t *o = make_sprite(s_world, &spr_debris);
                lv_obj_set_pos(o, c * T, r * T + T - spr_debris.h + 2);
                s_map[r][c] = '.';
                break;
            }
            case '^': {
                lv_obj_t *o = make_sprite(s_world, &spr_spikes);
                lv_obj_set_pos(o, c * T, r * T + T - spr_spikes.h);
                break;
            }
            case 'w': case 't':
                if (s_n_en < MAX_ENEMIES) {
                    enemy_t *e = &s_en[s_n_en++];
                    memset(e, 0, sizeof(*e));
                    e->alive = true;
                    if (ch == 'w') {
                        e->type = EN_WALKER; e->hp = 1; e->dir = -1;
                        e->w = 24; e->h = 16;
                        e->x = px + 4; e->y = py + T - e->h - 0.01f;
                        e->obj = make_sprite(s_world, &spr_walk[0][1]);
                    } else {
                        e->type = EN_TURRET; e->hp = TURRET_HP;
                        e->w = 28; e->h = 30;
                        e->x = px + 2; e->y = py + 2;
                        e->timer_ms = TURRET_FIRE_MS;
                        e->obj = make_sprite(s_world, &spr_turret);
                    }
                    lv_obj_set_pos(e->obj, (int)e->x, (int)e->y); /* si no, arranca en 0,0 */
                }
                s_map[r][c] = '.';
                break;
            case 'C': {
                make_rect(s_world, c * T + 14, r * T - 16, 4, T + 16, 0xDDDDDD, 0);
                s_cp_head = make_rect(s_world, c * T + 6, r * T - 30, 20, 20, 0x9AA4B8, LV_RADIUS_CIRCLE);
                s_cp_tile_x = px;
                s_map[r][c] = '.';
                break;
            }
            case 'F': {
                s_flag_x = px;
                make_rect(s_world, c * T + 14, r * T - 4 * T, 5, 5 * T, 0xEEEEEE, 2);
                make_rect(s_world, c * T + 10, r * T - 4 * T - 8, 13, 13, C_BULLET, LV_RADIUS_CIRCLE);
                lv_obj_t *fl_obj = make_rect(s_world, c * T + 19, r * T - 4 * T + 2, 44, 28, 0x30D158, 3);
                make_label(fl_obj, FONT_SMALL, 0xFFFFFF, "META");
                lv_obj_center(lv_obj_get_child(fl_obj, 0));
                s_map[r][c] = '.';
                break;
            }
            default: break;
            }
        }
    }

    /* jugador y proyectiles arriba de todo dentro del mundo */
    s_player_obj = make_sprite(s_world, &spr_player[PF_IDLE][0]);
    for (int i = 0; i < MAX_PB; i++) {
        s_pb[i].obj = make_rect(s_world, 0, 0, 12, 5, C_BULLET, 2);
        lv_obj_set_style_border_color(s_pb[i].obj, lv_color_hex(0xFF9A1F), 0);
        lv_obj_set_style_border_width(s_pb[i].obj, 1, 0);
        s_pb[i].on = false; show(s_pb[i].obj, false);
    }
    for (int i = 0; i < MAX_EB; i++) {
        s_eb[i].obj = make_rect(s_world, 0, 0, 10, 10, C_EBULLET, LV_RADIUS_CIRCLE);
        lv_obj_set_style_border_color(s_eb[i].obj, lv_color_hex(0xFFE0E0), 0);
        lv_obj_set_style_border_width(s_eb[i].obj, 2, 0);
        s_eb[i].on = false; show(s_eb[i].obj, false);
    }
    for (int i = 0; i < MAX_PUFF; i++) {
        s_puff[i].obj = make_sprite(s_world, &spr_puff[0]);
        s_puff[i].on = false; show(s_puff[i].obj, false);
    }
    s_cp_x = s_start_x; s_cp_y = s_start_y;
}

/* ------------------------------------------------------------- jugador */

static void player_spawn(float x, float y)
{
    memset(&P, 0, sizeof(P));
    P.x = x; P.y = y; P.w = 20; P.h = 30;
    P.facing = 1; P.alive = true;
    P.hearts = START_HEARTS;
    P.invuln_ms = 1500;
    show(s_player_obj, true);
    s_cam = clampf(P.x - GW * 0.4f, 0, (float)(s_map_w * T - GW));
}

static void player_die(void)
{
    if (!P.alive) return;
    P.alive = false;
    puff_at(P.x + P.w / 2, P.y + P.h / 2);
    show(s_player_obj, false);
    s_lives--;
    msg(s_lives > 0 ? "OUCH!" : NULL, NULL);
    set_state(GS_DYING);
}

static void player_hurt(void)
{
    if (!P.alive || P.invuln_ms > 0) return;
    P.hearts--;
    if (P.hearts <= 0) { player_die(); return; }
    P.invuln_ms = 1200;
    P.knock_ms  = 220;
    P.vy = -380;
    P.vx = -P.facing * 200.0f;
}

static void fire_bullet(bool up)
{
    for (int i = 0; i < MAX_PB; i++) {
        bullet_t *b = &s_pb[i];
        if (b->on) continue;
        b->on = true;
        if (up) {
            b->w = 5; b->h = 12;
            b->x = P.x + P.w / 2 + (P.facing > 0 ? 2 : -6);
            b->y = P.y - 10;
            b->vx = 0; b->vy = -BULLET_SPEED;
        } else {
            b->w = 12; b->h = 5;
            b->x = P.facing > 0 ? P.x + P.w + 2 : P.x - 14;
            b->y = P.y + 14;
            b->vx = P.facing * BULLET_SPEED; b->vy = 0;
        }
        lv_obj_set_size(b->obj, b->w, b->h);
        show(b->obj, true);
        return;
    }
}

static void break_brick(int tx, int ty)
{
    for (int i = 0; i < s_n_brick; i++) {
        brick_t *b = &s_brick[i];
        if (!b->on || b->tx != tx || b->ty != ty) continue;
        b->on = false;
        show(b->obj, false);
        s_map[ty][tx] = '.';
        s_score += PTS_BRICK;
        puff_at(tx * T + T / 2.0f, ty * T + T / 2.0f);
        return;
    }
}

#define PTS_RUBBLE 20
/* un disparo pega en escombros: 2 tiros y se deshacen */
static void hit_rubble(int tx, int ty)
{
    for (int i = 0; i < s_n_rubble; i++) {
        rubble_t *rb = &s_rubble[i];
        if (!rb->on || rb->tx != tx || rb->ty != ty) continue;
        if (--rb->hp > 0) {
            lv_obj_set_style_image_recolor(rb->obj, lv_color_hex(0x000000), 0);
            lv_obj_set_style_image_recolor_opa(rb->obj, 90, 0); /* se oscurece: esta por romperse */
            return;
        }
        rb->on = false;
        show(rb->obj, false);
        s_map[ty][tx] = '.';
        s_score += PTS_RUBBLE;
        puff_at(tx * T + T / 2.0f, ty * T + T / 2.0f);
        return;
    }
}

static void update_player(float dt, int dt_ms, uint8_t in, uint8_t pressed)
{
    /* horizontal */
    float want = 0;
    if (in & GP_IN_LEFT)  want -= 1;
    if (in & GP_IN_RIGHT) want += 1;
    if (P.knock_ms > 0) { P.knock_ms -= dt_ms; want = 0; }
    else if (want != 0) P.facing = want > 0 ? 1 : -1;

    float accel = P.on_ground ? RUN_ACCEL : AIR_ACCEL;
    /* CORRER solo sube el tope en el suelo; en el aire se conserva el impulso */
    float vmax = (in & GP_IN_RUN) ? RUN_SPEED : WALK_SPEED;
    if (want != 0) {
        P.vx += want * accel * dt;
        if (P.on_ground || fabsf(P.vx) <= WALK_SPEED) {
            if (P.vx > vmax)  P.vx = P.on_ground ? fmaxf(vmax, P.vx - RUN_FRICTION * dt) : vmax;
            if (P.vx < -vmax) P.vx = P.on_ground ? fminf(-vmax, P.vx + RUN_FRICTION * dt) : -vmax;
        } else {
            P.vx = clampf(P.vx, -RUN_SPEED, RUN_SPEED);
        }
    } else if (P.on_ground && P.knock_ms <= 0) {
        float f = RUN_FRICTION * dt;
        if (P.vx > f) P.vx -= f; else if (P.vx < -f) P.vx += f; else P.vx = 0;
    }

    /* salto (con coyote time y buffer) */
    if (pressed & GP_IN_JUMP) P.jbuf_ms = JUMP_BUFFER_MS;
    else if (P.jbuf_ms > 0) P.jbuf_ms -= dt_ms;
    if (P.on_ground) P.coyote_ms = COYOTE_MS; else if (P.coyote_ms > 0) P.coyote_ms -= dt_ms;
    if (P.jbuf_ms > 0 && P.coyote_ms > 0) {
        /* como en Mario: cuanto mas rapido vas, mas alto saltas */
        float speed_k = clampf((fabsf(P.vx) - WALK_SPEED) / (RUN_SPEED - WALK_SPEED), 0, 1);
        P.vy = -(JUMP_SPEED + JUMP_RUN_BONUS * speed_k);
        P.jbuf_ms = 0; P.coyote_ms = 0;
        P.on_ground = false;
    }
    if (!(in & GP_IN_JUMP) && P.vy < -JUMP_CUT) P.vy = -JUMP_CUT;

    P.vy += GRAVITY * dt;
    if (P.vy > MAX_FALL) P.vy = MAX_FALL;

    int hit = move_box(&P.x, &P.y, P.w, P.h, &P.vx, &P.vy, dt);
    P.on_ground = (hit & 2) != 0;
    if (!P.on_ground && P.vy >= 0 && box_solid(P.x, P.y + 1.0f, P.w, P.h)) P.on_ground = true;
    if (hit & 4) { /* cabezazo: romper ladrillos */
        int hty = fl(P.y) - 1;
        int tx0 = fl(P.x + 2), tx1 = fl(P.x + P.w - 2);
        for (int tx = tx0; tx <= tx1; tx++) if (map_at(tx, hty) == 'B') break_brick(tx, hty);
    }

    /* disparo */
    if (P.fire_ms > 0) P.fire_ms -= dt_ms;
    if ((in & GP_IN_FIRE) && P.fire_ms <= 0) { fire_bullet((in & GP_IN_UP) != 0); P.fire_ms = FIRE_EVERY_MS; }

    /* invulnerable: parpadeo */
    if (P.invuln_ms > 0) {
        P.invuln_ms -= dt_ms;
        show(s_player_obj, P.invuln_ms <= 0 || (P.invuln_ms / 90) % 2 == 0);
    }

    /* pinchos (mitad de abajo del bloque) */
    int tx0 = fl(P.x), tx1 = fl(P.x + P.w - 0.01f), ty0 = fl(P.y), ty1 = fl(P.y + P.h - 0.01f);
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++)
            if (map_at(tx, ty) == '^' && overlap(P.x, P.y, P.w, P.h, tx * T + 3, ty * T + 18, T - 6, 14)) {
                player_hurt();
                if (P.alive && P.invuln_ms > 1000) P.vy = -520;
            }

    /* caida al agua toxica (filas 13-14 sin suelo) */
    if (P.y + P.h > 13 * T + 14) { P.hearts = 0; player_die(); return; }

    /* monedas */
    for (int i = 0; i < s_n_coin; i++) {
        coin_t *k = &s_coin[i];
        if (!k->on || !overlap(P.x, P.y, P.w, P.h, k->x, k->y, 16, 16)) continue;
        k->on = false; show(k->obj, false);
        s_score += PTS_COIN;
        if (++s_coins % COINS_FOR_LIFE == 0) s_lives++;
    }

    /* punto de control */
    if (s_cp_head && !s_cp_taken && P.x > s_cp_tile_x) {
        s_cp_taken = true;
        s_cp_x = s_cp_tile_x + 6; s_cp_y = P.y;
        lv_obj_set_style_bg_color(s_cp_head, lv_color_hex(0x30D158), 0);
    }

    /* meta */
    if (P.x + P.w > s_flag_x + 10 && P.x < s_flag_x + T) {
        s_score += PTS_GOAL;
        if (s_level + 1 < N_LEVELS) msg("NIVEL COMPLETADO!", "Toca la pantalla (o SALTO) para el siguiente nivel");
        else                        msg("GANASTE!", "Acabaste con la plaga. Toca para jugar otra vez");
        s_banner_ms = 0;
        set_state(GS_CLEAR);
    }

    /* sprite segun estado */
    int f;
    if (!P.on_ground) f = PF_JUMP;
    else if (in & GP_IN_UP) f = PF_UP;
    else if (fabsf(P.vx) > 20) {
        P.anim_ms += dt_ms;
        int step = fabsf(P.vx) > WALK_SPEED + 20 ? 70 : 110; /* piernas mas rapidas corriendo */
        f = ((P.anim_ms / step) % 2) ? PF_RUN2 : PF_RUN1;
    } else f = PF_IDLE;
    lv_image_set_src(s_player_obj, &spr_player[f][P.facing > 0 ? 0 : 1].dsc);
    lv_obj_set_pos(s_player_obj, (int)(P.x - 2), (int)(P.y - 2));
}

/* ------------------------------------------------------------- enemigos y balas */

static bool on_camera(float x, float w, float margin)
{
    return x + w > s_cam - margin && x < s_cam + GW + margin;
}

static void enemy_fire(enemy_t *e)
{
    float cx = e->x + e->w / 2, cy = e->y + 8;
    float dx = (P.x + P.w / 2) - cx, dy = (P.y + P.h / 2) - cy;
    float d = sqrtf(dx * dx + dy * dy);
    if (d < 1) return;
    for (int i = 0; i < MAX_EB; i++) {
        bullet_t *b = &s_eb[i];
        if (b->on) continue;
        b->on = true; b->w = 10; b->h = 10;
        b->x = cx - 5; b->y = cy - 5;
        b->vx = dx / d * ENEMY_BULLET_SPD; b->vy = dy / d * ENEMY_BULLET_SPD;
        show(b->obj, true);
        return;
    }
}

static void enemy_kill(enemy_t *e, int pts)
{
    e->alive = false;
    show(e->obj, false);
    s_score += pts;
    puff_at(e->x + e->w / 2, e->y + e->h / 2);
}

static void update_enemies(float dt, int dt_ms)
{
    for (int i = 0; i < s_n_en; i++) {
        enemy_t *e = &s_en[i];
        if (!e->alive) continue;
        if (!on_camera(e->x, e->w, 120)) continue; /* fuera de pantalla: quieto */

        if (e->flash_ms > 0 && (e->flash_ms -= dt_ms) <= 0)
            lv_obj_set_style_image_recolor_opa(e->obj, LV_OPA_TRANSP, 0);

        if (e->type == EN_WALKER) {
            e->vx = e->dir * WALKER_SPEED;
            e->vy += GRAVITY * dt;
            if (e->vy > MAX_FALL) e->vy = MAX_FALL;
            int hit = move_box(&e->x, &e->y, e->w, e->h, &e->vx, &e->vy, dt);
            e->on_ground = (hit & 2) || box_solid(e->x, e->y + 1, e->w, e->h);
            if (hit & 1) e->dir = -e->dir;
            else if (e->on_ground) { /* no caerse de los bordes */
                float fx = e->dir > 0 ? e->x + e->w + 1 : e->x - 1;
                if (!solid_at(fl(fx), fl(e->y + e->h + 2))) e->dir = -e->dir;
            }
            if (e->y + e->h > 13 * T + 14) { /* cayo al agua toxica */
                e->alive = false; show(e->obj, false);
                puff_at(e->x + e->w / 2, 13 * T + 8);
                continue;
            }
            e->timer_ms += dt_ms;
            int fr = (e->timer_ms / 120) % 2; /* patitas rapidas */
            lv_image_set_src(e->obj, &spr_walk[fr][e->dir > 0 ? 0 : 1].dsc);
            lv_obj_set_pos(e->obj, (int)(e->x - 2), (int)(e->y - 2));
        } else {
            lv_obj_set_pos(e->obj, (int)(e->x - 2), (int)(e->y - 2));
            float dx = fabsf((P.x + P.w / 2) - (e->x + e->w / 2));
            if (s_state == GS_PLAY && P.alive && dx < 460 && on_camera(e->x, e->w, -10)) {
                e->timer_ms -= dt_ms;
                if (e->timer_ms <= 0) { e->timer_ms = TURRET_FIRE_MS; enemy_fire(e); }
            }
        }

        /* contacto con el jugador */
        if (s_state == GS_PLAY && P.alive && overlap(P.x, P.y, P.w, P.h, e->x, e->y, e->w, e->h)) {
            bool stomp = e->type == EN_WALKER && P.vy > 0 && (P.y + P.h) - e->y < 14;
            if (stomp) {
                enemy_kill(e, PTS_WALKER);
                P.vy = -STOMP_BOUNCE;
            } else {
                player_hurt();
            }
        }
    }
}

static void update_bullets(float dt)
{
    for (int i = 0; i < MAX_PB; i++) {
        bullet_t *b = &s_pb[i];
        if (!b->on) continue;
        b->x += b->vx * dt; b->y += b->vy * dt;
        int btx = fl(b->x + b->w / 2.0f), bty = fl(b->y + b->h / 2.0f);
        bool dead = !on_camera(b->x, (float)b->w, 20) || b->y < -20 || solid_at(btx, bty);
        if (dead && map_at(btx, bty) == 'R') hit_rubble(btx, bty);
        if (!dead) {
            for (int k = 0; k < s_n_en; k++) {
                enemy_t *e = &s_en[k];
                if (!e->alive || !overlap(b->x, b->y, (float)b->w, (float)b->h, e->x, e->y, e->w, e->h)) continue;
                dead = true;
                if (--e->hp <= 0) enemy_kill(e, e->type == EN_TURRET ? PTS_TURRET : PTS_WALKER);
                else {
                    e->flash_ms = 120;
                    lv_obj_set_style_image_recolor(e->obj, lv_color_hex(0xFFFFFF), 0);
                    lv_obj_set_style_image_recolor_opa(e->obj, 200, 0);
                }
                break;
            }
        }
        if (dead) { b->on = false; show(b->obj, false); continue; }
        lv_obj_set_pos(b->obj, (int)b->x, (int)b->y);
    }

    for (int i = 0; i < MAX_EB; i++) {
        bullet_t *b = &s_eb[i];
        if (!b->on) continue;
        b->x += b->vx * dt; b->y += b->vy * dt;
        bool dead = !on_camera(b->x, (float)b->w, 40) || b->y < -40 || b->y > GH + 20 ||
                    solid_at(fl(b->x + 5), fl(b->y + 5));
        if (!dead && s_state == GS_PLAY && P.alive && overlap(b->x, b->y, 10, 10, P.x + 2, P.y + 2, P.w - 4, P.h - 4)) {
            dead = true;
            player_hurt();
        }
        if (dead) { b->on = false; show(b->obj, false); continue; }
        lv_obj_set_pos(b->obj, (int)b->x, (int)b->y);
    }
}

static void update_misc(int dt_ms)
{
    /* el agua corre y los caños gotean */
    float dt = dt_ms / 1000.0f;
    s_water_off += 24.0f * dt;
    if (s_water_off >= spr_tex_water.w) s_water_off -= spr_tex_water.w;
    if (s_water) lv_image_set_offset_x(s_water, (int32_t)s_water_off);
    for (int i = 0; i < s_n_drip; i++) {
        drip_t *d = &s_drip[i];
        d->y += d->v * dt;
        if (d->y > d->y1) d->y = d->y0;
        lv_obj_set_y(d->obj, (int32_t)d->y);
    }

    s_anim_ms += dt_ms;
    if (s_anim_ms >= 160) {
        s_anim_ms = 0;
        s_anim_frame ^= 1;
        for (int i = 0; i < s_n_coin; i++)
            if (s_coin[i].on) lv_image_set_src(s_coin[i].obj, &spr_coin[s_anim_frame].dsc);
    }
    for (int i = 0; i < MAX_PUFF; i++) {
        puff_t *p = &s_puff[i];
        if (!p->on) continue;
        p->age += dt_ms;
        if (p->age > 320) { p->on = false; show(p->obj, false); continue; }
        int f = p->age > 140;
        if (f != p->frame) { p->frame = f; lv_image_set_src(p->obj, &spr_puff[f].dsc); }
    }
}

static void update_camera(float dt)
{
    float target = P.x + P.w / 2 - GW * 0.4f;
    float maxc = (float)(s_map_w * T - GW);
    target = clampf(target, 0, maxc < 0 ? 0 : maxc);
    float k = dt * 8.0f; if (k > 1) k = 1;
    s_cam += (target - s_cam) * k;
    lv_obj_set_x(s_world, -(int32_t)s_cam);
    lv_obj_set_x(s_far,   -(int32_t)(s_cam * 0.25f));
    lv_obj_set_x(s_mid,   -(int32_t)(s_cam * 0.5f));
}

/* ------------------------------------------------------------- HUD */

static void hud_update(void)
{
    if (s_score != s_shown_score) { lv_label_set_text_fmt(s_lbl_score, "PUNTOS %d", s_score); s_shown_score = s_score; }
    if (s_coins != s_shown_coins) { lv_label_set_text_fmt(s_lbl_coins, "MONEDAS %d", s_coins); s_shown_coins = s_coins; }
    if (s_lives != s_shown_lives) { lv_label_set_text_fmt(s_lbl_lives, "VIDAS %d", s_lives < 0 ? 0 : s_lives); s_shown_lives = s_lives; }
    if (P.hearts != s_shown_hearts) {
        for (int i = 0; i < START_HEARTS; i++)
            lv_image_set_src(s_heart[i], i < P.hearts ? &spr_heart.dsc : &spr_heart_empty.dsc);
        s_shown_hearts = P.hearts;
    }
}

static void hud_show(bool on)
{
    show(s_hud_bg, on);
    show(s_lbl_score, on); show(s_lbl_coins, on); show(s_lbl_lives, on);
    for (int i = 0; i < START_HEARTS; i++) show(s_heart[i], on);
    show(s_btn_pause, on);
    show(s_ctrl_box, on);
}

/* ------------------------------------------------------------- flujo */

/* arranca (o reinicia) un nivel conservando puntos y vidas */
static void start_level(int idx)
{
    load_level(idx);
    player_spawn(s_start_x, s_start_y);
    show(s_title_box, false);
    hud_show(true);
    s_shown_hearts = -1;
    msg(LEVELS[idx].name, NULL);   /* cartel con el nombre del nivel */
    s_banner_ms = 1800;
    hud_update();
    update_camera(1.0f);
    set_state(GS_PLAY);
}

/* partida nueva desde el nivel idx */
static void new_game(int idx)
{
    s_score = 0; s_coins = 0; s_lives = START_LIVES;
    s_shown_score = s_shown_coins = s_shown_lives = s_shown_hearts = -1;
    start_level(idx);
}

/* despues de "NIVEL COMPLETADO" / "FIN DEL JUEGO": tocar para seguir */
static void continue_after_end(void)
{
    if (s_state == GS_OVER)              new_game(s_level);          /* reintenta el mismo nivel */
    else if (s_level + 1 < N_LEVELS)     start_level(s_level + 1);   /* siguiente nivel */
    else                                 new_game(0);                /* gano todo: de nuevo */
}

static void game_tick(lv_timer_t *t)
{
    (void)t;
    uint32_t now = lv_tick_get();
    int dt_ms = (int)lv_tick_diff(now, s_last_tick);
    s_last_tick = now;
    if (dt_ms > 33) dt_ms = 33;
    if (s_paused) return;
    float dt = dt_ms / 1000.0f;
    s_state_ms += dt_ms;

    uint8_t in = s_touch_in | s_ext_in;
    uint8_t pressed = in & ~s_prev_in;
    s_prev_in = in;

    switch (s_state) {
    case GS_TITLE:
        show(s_title_blink, (s_state_ms / 500) % 2 == 0);
        if ((pressed & GP_IN_JUMP) && s_state_ms > 600) new_game(0);
        return;
    case GS_PLAY:
        if (s_banner_ms > 0 && (s_banner_ms -= dt_ms) <= 0) msg(NULL, NULL);
        update_player(dt, dt_ms, in, pressed);
        if (s_state == GS_PLAY) update_enemies(dt, dt_ms);
        update_bullets(dt);
        break;
    case GS_DYING:
        update_enemies(dt, dt_ms);
        update_bullets(dt);
        if (s_state_ms > 1500) {
            if (s_lives <= 0) {
                msg("FIN DEL JUEGO", "Toca la pantalla (o SALTO) para reintentar");
                set_state(GS_OVER);
            } else {
                msg(NULL, NULL);
                for (int i = 0; i < MAX_EB; i++) { s_eb[i].on = false; show(s_eb[i].obj, false); }
                player_spawn(s_cp_x, s_cp_y);
                set_state(GS_PLAY);
            }
        }
        break;
    case GS_CLEAR:
    case GS_OVER:
        if ((pressed & GP_IN_JUMP) && s_state_ms > 1000) continue_after_end();
        break;
    }
    update_misc(dt_ms);
    if (P.alive) update_camera(dt);
    hud_update();
}

/* ------------------------------------------------------------- eventos */

static void root_click_cb(lv_event_t *e)
{
    (void)e;
    if (s_paused) return;
    if (s_state == GS_TITLE && s_state_ms > 600) new_game(0);
    else if ((s_state == GS_CLEAR || s_state == GS_OVER) && s_state_ms > 1000) continue_after_end();
}

static void level_btn_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (s_state == GS_TITLE && s_state_ms > 600 && idx >= 0 && idx < N_LEVELS) new_game(idx);
}

static void ctrl_btn_cb(lv_event_t *e)
{
    uint8_t bit = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) s_touch_in |= bit;
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) s_touch_in &= (uint8_t)~bit;
}

static void pause_btn_cb(lv_event_t *e)
{
    (void)e;
    if (s_state != GS_PLAY && s_state != GS_DYING && !s_paused) return;
    s_paused = !s_paused;
    lv_label_set_text(s_lbl_pause, s_paused ? LV_SYMBOL_PLAY : LV_SYMBOL_PAUSE);
    msg(s_paused ? "PAUSA" : NULL, NULL);
    s_last_tick = lv_tick_get();
}

static void game_close(void)
{
    if (!s_open) return;
    if (s_timer) { lv_timer_delete(s_timer); s_timer = NULL; }
    s_open = false;
    lv_obj_t *prev = s_prev_scr;
    s_prev_scr = NULL;
    if (prev && lv_obj_is_valid(prev)) lv_screen_load_anim(prev, LV_SCREEN_LOAD_ANIM_FADE_IN, 200, 0, true);
    else lv_obj_delete_async(s_scr);
    s_scr = s_root = s_world = s_far = s_mid = NULL;
    s_vig_top = s_vig_bot = NULL;
}

static void close_btn_cb(lv_event_t *e) { (void)e; game_close(); }

static lv_obj_t *make_button(lv_obj_t *parent, int x, int y, int w, int h, const char *txt,
                             const lv_font_t *font, lv_event_cb_t cb, uint8_t bit, lv_obj_t **lbl_out)
{
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, w, h);
    lv_obj_set_style_radius(b, 12, 0);
    lv_obj_set_style_shadow_opa(b, 0, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x10142A), 0);
    lv_obj_set_style_bg_opa(b, 110, 0);
    lv_obj_set_style_bg_opa(b, 200, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(b, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_opa(b, 120, 0);
    lv_obj_set_style_border_width(b, 2, 0);
    if (bit) {
        lv_obj_add_event_cb(b, cb, LV_EVENT_PRESSED, (void *)(uintptr_t)bit);
        lv_obj_add_event_cb(b, cb, LV_EVENT_RELEASED, (void *)(uintptr_t)bit);
        lv_obj_add_event_cb(b, cb, LV_EVENT_PRESS_LOST, (void *)(uintptr_t)bit);
    } else {
        lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    }
    lv_obj_t *l = lv_label_create(b);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(l, txt);
    lv_obj_center(l);
    if (lbl_out) *lbl_out = l;
    return b;
}

/* ------------------------------------------------------------- API */

bool game_platform_is_open(void) { return s_open; }
void game_platform_set_ext_input(uint8_t mask) { s_ext_in = mask; }

void game_platform_open(void)
{
    if (s_open) return;
    sprites_init();

    s_paused = false;
    s_touch_in = s_ext_in = 0;
    s_prev_in = 0xFF; /* ignora teclas que ya estaban apretadas al abrir */
    s_world = s_far = s_mid = NULL;
    s_vig_top = s_vig_bot = NULL;
    memset(&P, 0, sizeof(P));

    s_prev_scr = lv_screen_active();
    s_scr = lv_obj_create(NULL);
    lv_obj_set_size(s_scr, GW, GH);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(0x000000), 0);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);

    s_root = lv_obj_create(s_scr);
    lv_obj_remove_style_all(s_root);
    lv_obj_set_size(s_root, GW, GH);
    lv_obj_set_pos(s_root, 0, 0);
    lv_obj_set_style_bg_color(s_root, lv_color_hex(C_BG_TOP), 0);
    lv_obj_set_style_bg_grad_color(s_root, lv_color_hex(C_BG_BOT), 0);
    lv_obj_set_style_bg_grad_dir(s_root, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, 0);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_root, root_click_cb, LV_EVENT_CLICKED, NULL);

    /* nivel (fondo + mundo) — indices 0,1,2 del root */
    load_level(0);
    player_spawn(s_start_x, s_start_y);
    show(s_player_obj, false);
    update_camera(1.0f);

    /* HUD */
    s_hud_bg = make_rect(s_root, 0, 0, GW, 46, 0x000000, 0);
    lv_obj_set_style_bg_opa(s_hud_bg, 90, 0);
    s_lbl_score = make_label(s_root, FONT_MID, 0xFFFFFF, "");
    lv_obj_set_pos(s_lbl_score, 14, 12);
    s_lbl_coins = make_label(s_root, FONT_MID, 0xFFE34A, "");
    lv_obj_set_pos(s_lbl_coins, 210, 12);
    s_lbl_lives = make_label(s_root, FONT_MID, 0xFFFFFF, "");
    lv_obj_set_pos(s_lbl_lives, 400, 12);
    for (int i = 0; i < START_HEARTS; i++) {
        s_heart[i] = make_sprite(s_root, &spr_heart);
        lv_obj_set_pos(s_heart[i], 520 + i * 28, 14);
    }

    /* controles tactiles */
    s_ctrl_box = make_container(s_root, GW, GH);
    make_button(s_ctrl_box, 14,  392, 84, 76, LV_SYMBOL_LEFT,  FONT_BIG, ctrl_btn_cb, GP_IN_LEFT, NULL);
    make_button(s_ctrl_box, 108, 392, 84, 76, LV_SYMBOL_RIGHT, FONT_BIG, ctrl_btn_cb, GP_IN_RIGHT, NULL);
    make_button(s_ctrl_box, 61,  318, 84, 64, LV_SYMBOL_UP,    FONT_MID, ctrl_btn_cb, GP_IN_UP, NULL);
    make_button(s_ctrl_box, GW - 196, 392, 88, 76, "DISP",  FONT_MID, ctrl_btn_cb, GP_IN_FIRE, NULL);
    make_button(s_ctrl_box, GW - 196, 318, 88, 64, "CORRER", FONT_SMALL, ctrl_btn_cb, GP_IN_RUN, NULL);
    make_button(s_ctrl_box, GW - 100, 380, 88, 88, "SALTO", FONT_MID, ctrl_btn_cb, GP_IN_JUMP, NULL);

    s_lbl_msg = make_label(s_root, FONT_BIG, 0xFFFFFF, "");
    lv_obj_align(s_lbl_msg, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_text_opa(s_lbl_msg, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_lbl_msg, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_lbl_msg, 140, 0);
    lv_obj_set_style_pad_hor(s_lbl_msg, 16, 0);
    lv_obj_set_style_pad_ver(s_lbl_msg, 6, 0);
    lv_obj_set_style_radius(s_lbl_msg, 8, 0);
    s_lbl_sub = make_label(s_root, FONT_SMALL, 0xFFFFFF, "");
    lv_obj_align(s_lbl_sub, LV_ALIGN_CENTER, 0, 10);
    msg(NULL, NULL);

    s_btn_pause = make_button(s_root, GW - 122, 3, 52, 40, LV_SYMBOL_PAUSE, FONT_MID, pause_btn_cb, 0, &s_lbl_pause);
    make_button(s_root, GW - 62, 3, 52, 40, LV_SYMBOL_CLOSE, FONT_MID, close_btn_cb, 0, NULL);

    /* portada */
    s_title_box = make_rect(s_root, 0, 0, GW, GH, 0x0B1030, 0);
    lv_obj_set_style_bg_opa(s_title_box, 170, 0);
    lv_obj_move_to_index(s_title_box, -3); /* debajo de los botones pausa/cerrar */
    lv_obj_t *tt = make_label(s_title_box, FONT_BIG, 0xFFE34A, GAME_TITLE);
    lv_obj_align(tt, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_t *st = make_label(s_title_box, FONT_SMALL, 0xC8D0E0, GAME_SUBTITLE);
    lv_obj_align(st, LV_ALIGN_TOP_MID, 0, 124);
    lv_obj_t *hero = make_sprite(s_title_box, &spr_player[PF_IDLE][0]);
    lv_image_set_scale(hero, 512); /* x2, escala desde el centro */
    lv_obj_set_pos(hero, GW / 2 - 12, 190);
    lv_obj_t *help = make_label(s_title_box, FONT_SMALL, 0xFFFFFF,
        "Tactil: botones en pantalla      Teclado (simulador): flechas o A/D mover,\n"
        "Z / ESPACIO saltar,  X disparar,  C / SHIFT correr,  flecha arriba / W apuntar arriba\n"
        "Corriendo saltas mas alto y mas lejos. Pisa a los enemigos o disparales.");
    lv_obj_align(help, LV_ALIGN_TOP_MID, 0, 270);
    s_title_blink = make_label(s_title_box, FONT_MID, 0x7DF9FF, "TOCA LA PANTALLA O PULSA SALTO");
    lv_obj_align(s_title_blink, LV_ALIGN_TOP_MID, 0, 340);

    /* elegir nivel directo (para probar cada nivel) */
    int bw = 150, gap = 16, x0 = (GW - N_LEVELS * bw - (N_LEVELS - 1) * gap) / 2;
    for (int i = 0; i < N_LEVELS; i++) {
        char txt[16];
        snprintf(txt, sizeof(txt), "NIVEL %d", i + 1);
        lv_obj_t *b = make_button(s_title_box, x0 + i * (bw + gap), 390, bw, 48, txt, FONT_MID, level_btn_cb, 0, NULL);
        lv_obj_remove_event_cb(b, level_btn_cb);
        lv_obj_add_event_cb(b, level_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }

    hud_show(false);
    set_state(GS_TITLE);
    s_last_tick = lv_tick_get();
    s_timer = lv_timer_create(game_tick, 16, NULL);
    s_open = true;
    lv_screen_load_anim(s_scr, LV_SCREEN_LOAD_ANIM_FADE_IN, 200, 0, false);
}
