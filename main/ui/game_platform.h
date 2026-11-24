#ifndef GAME_PLATFORM_H
#define GAME_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Juego de plataformas + disparos (BASE de prueba, graficos provisionales).
 * Mismo codigo para la placa y para el simulador de PC.
 */
void game_platform_open(void);
bool game_platform_is_open(void);

/* Entrada externa (teclado del simulador, joystick de la consola, multitouch
 * de la placa...). Se combina (OR) con los botones tactiles de la pantalla.
 * Llamarla cada vez que cambie el estado (o periodicamente). */
#define GP_IN_LEFT   0x01
#define GP_IN_RIGHT  0x02
#define GP_IN_JUMP   0x04
#define GP_IN_FIRE   0x08
#define GP_IN_UP     0x10
#define GP_IN_RUN    0x20
void game_platform_set_ext_input(uint8_t mask);

#ifdef __cplusplus
}
#endif

#endif /* GAME_PLATFORM_H */
