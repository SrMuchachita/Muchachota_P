#ifndef GAME_SHOOTER_H
#define GAME_SHOOTER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Juego de naves "COSMO DEFENSA" (original, LVGL puro).
 * Mismo codigo para la placa y para el simulador de PC.
 *
 * game_shooter_open(): abre el juego a pantalla completa (800x480) como un
 * screen propio. Al tocar la X vuelve al screen que estaba activo antes.
 */
void game_shooter_open(void);
bool game_shooter_is_open(void);

#ifdef __cplusplus
}
#endif

#endif /* GAME_SHOOTER_H */
