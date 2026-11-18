/*
 * Layout de la UNICA ventana del simulador: 3 columnas en el MISMO SDL
 * window (no 3 ventanas separadas):
 *
 *   [ MOTOR (RS485) 340px | HMI 800x480 real | SERVOS (RS485) 300px ]
 *
 * El HMI real (pantalla 800x480, screens.c sin tocar) queda centrado: cada
 * screen se corre a x=SIM_HMI_X apenas se carga (ver el hook
 * LV_EVENT_SCREEN_LOADED en sim_main.c) — es la unica forma de recentrar un
 * "screen" de LVGL sin editar screens.c, porque lv_screen_load() siempre
 * fuerza la posicion a (0,0) internamente.
 *
 * Cambiar estos numeros los mueve automaticamente en los 3 archivos que
 * arman la ventana (sim_main.c, bigview_port.c, robot_panel.c) — todos
 * incluyen este header en vez de repetir los numeros.
 */

#ifndef SIM_LAYOUT_H
#define SIM_LAYOUT_H

#include <stdint.h>

#define SIM_MOTOR_COL_X 0
#define SIM_MOTOR_COL_W 340

#define SIM_HMI_X 340
#define SIM_HMI_W 800

#define SIM_SERVO_COL_X (SIM_HMI_X + SIM_HMI_W)
#define SIM_SERVO_COL_W 300

#define SIM_WINDOW_W (SIM_MOTOR_COL_W + SIM_HMI_W + SIM_SERVO_COL_W)
#define SIM_WINDOW_H 480

// x real del HMI en este instante — SIM_HMI_X es el valor de referencia
// (ventana en su ancho normal), pero si la ventana es mas ancha la columna
// MOTOR crece y empuja el HMI hacia la derecha. Definida en robot_panel.c,
// actualizada por sim_side_panels_update_layout() en cada tick.
int32_t sim_get_hmi_x(void);

#endif
