/*
 * Panel de LOG del simulador — ocupa el espacio extra que aparece al
 * maximizar/agrandar la ventana (el HMI y las columnas MOTOR/SERVOS quedan
 * siempre fijas en 1440x480, ver sim_layout.h; si la ventana es mas alta,
 * en vez de dejar el sobrante en negro se usa para mostrar un log de
 * eventos — conexion RS485, capturas de pantalla, ping/pong, etc).
 */

#ifndef SIM_LOG_H
#define SIM_LOG_H

// Crea el panel (arranca oculto: solo se muestra cuando hay espacio extra).
void sim_log_panel_create(void);

// Llamar en cada tick: muestra/oculta y redimensiona el panel segun el alto
// actual de la ventana (lv_display_get_vertical_resolution del default).
void sim_log_panel_update_layout(void);

// Agrega una linea a la pestaña "LOG" (con timestamp), estilo printf.
void sim_log(const char *fmt, ...);

// Agrega una linea a la pestaña "PS3" (control fisico) — mismo timestamp,
// buffer separado (no se mezcla con el LOG general).
void sim_log_pad(const char *fmt, ...);

#endif
