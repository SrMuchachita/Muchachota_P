/*
 * Punto de entrada del SIMULADOR DE PC. Reemplaza a main/main.c (que tiene
 * demasiada logica de hardware especifica del ESP32-P4) SOLO para este
 * build de sim/ — el main.c real no se toca ni se usa aca.
 *
 * UNA sola ventana SDL, con el HMI real (800x480, screens.c sin tocar)
 * centrado entre dos columnas propias del simulador (ver stubs/sim_layout.h
 * para las medidas exactas):
 *   IZQUIERDA (340px) "MOTOR"  — conexion RS485 + joystick de motor (WASD)
 *                                 + sensores + controles solo-simulador
 *   CENTRO    (800px)          — pantalla real, sin tocar
 *   DERECHA   (300px) "SERVOS" — joystick de servos (flechas) + sliders
 *
 * Como centrar el HMI: lv_screen_load()/lv_scr_load_anim() SIEMPRE fuerzan
 * la posicion del screen a (0,0) al cargarlo (ver lv_display.c,
 * load_new_screen), asi que no alcanza con mover el screen una sola vez
 * despues de ui_init(). El truco "obvio" (hookear el evento
 * LV_EVENT_SCREEN_LOADED de la display) NO sirve aca: ui.c carga cada
 * screen con lv_scr_load_anim(..., LV_SCR_LOAD_ANIM_FADE_IN, ...) (ver
 * change_screen() en ui.c), y ese camino animado de LVGL dispara
 * LV_EVENT_SCREEN_LOADED solo a nivel OBJETO (lv_obj_send_event), nunca a
 * nivel DISPLAY (lv_display_send_event) — probado, el hook de display
 * nunca se disparaba. La solucion robusta (y que no depende de que tipo de
 * animacion use ui.c) es un timer que cada pocos ms re-centra el screen
 * activo (y el anterior, mientras dura el fade) si no estan ya en
 * SIM_HMI_X — se "autocorrige" solo en el peor caso 1 frame despues de
 * cada cambio de pantalla, imperceptible.
 *
 * La ventana ES redimensionable/maximizable (lv_sdl_window_set_resizeable):
 * el HMI y las columnas MOTOR/SERVOS quedan siempre fijas en 1440x480 (no
 * se agrandan ni reacomodan), pero si el usuario agranda la ventana, el
 * espacio de sobra abajo se usa para un panel de LOG en vez de quedar en
 * negro (ver stubs/sim_log.c).
 *
 * Splash de arranque: en la placa real, app_main() (main.c) muestra el logo
 * "welltepp" a pantalla completa 1500ms + fade-out de 400ms ANTES de llamar
 * ui_init() (ver main/main.c, vTaskDelay + lv_obj_fade_out) — ui_init() por
 * si solo no lo hace. Aca se replica lo mismo pero sin bloquear el hilo (un
 * vTaskDelay real congelaria la ventana SDL): dos lv_timer de un solo tiro
 * (sostener y luego borrar) en vez de un delay bloqueante.
 */

#include "lvgl.h"
#include "drivers/sdl/lv_sdl_window.h"
#include "drivers/sdl/lv_sdl_mouse.h"
#include "drivers/sdl/lv_sdl_mousewheel.h"
#include "drivers/sdl/lv_sdl_keyboard.h"
#include <SDL2/SDL.h> // SDL_INIT_GAMECONTROLLER — joystick/gamepad fisico (ver stubs/robot_panel.c)

#include "ui.h"
#include "images.h" // declara "extern const lv_img_dsc_t welltepp" (asset real del splash)
#include "sim_layout.h"
#include "sim_log.h"

// Definidas en stubs/bigview_port.c
void sim_logo_secret_button_wire(void);      // engancha el logo WELLTEP -> Panel de Manejo
void sim_settings_nav_enable_scroll(void);   // habilita scroll en la columna de Ajustes (10 botones)
void sim_hmi_conn_indicator_create(void);    // crea el LED amarillo de ping/pong (no existe sin esto)
void sim_console_serial_apply(void);         // pone el numero de serie de la consola (sale vacio sin esto)
void sim_version_panel_populate(void);       // llena Sistema > Version (sale vacio sin esto)
void sim_console_battery_seed(void);     // pinta la bateria CONSOLA inicial (100%) en la tarjeta del HMI
void sim_robot_battery_seed(void);       // pinta la bateria ROBOT inicial ("sin datos", arranca en OFF)
void sim_camera_settings_init_hidden(void); // Ajustes > Camara arranca oculto (solo el RD100 la trae)
void sim_camera_rl1_mode_wire(void);        // Ajustes > Camara: botones ON/OFF de RL1

// Definida en stubs/dev_mode_port.c
void sim_dev_mode_wire(void); // MODO DESARROLLADOR: triple-toque en Sistema > Version + PIN
void sim_set_main_display(lv_display_t *disp); // fija la display (para la captura de pantalla)

// Definidas en stubs/robot_panel.c
void sim_motor_panel_create(void); // columna izquierda "MOTOR (RS485)"
void sim_servo_panel_create(void); // columna derecha "SERVOS (RS485)"
void sim_side_panels_update_layout(void); // las agranda al maximizar (para que no necesiten scroll)

// ==================== Splash de arranque (ver comentario de arriba) ====================

static lv_obj_t *s_splash_img = NULL;

// Recien aca se arma el HMI real — antes de esto objects.* no existe, por
// eso sim_hmi_conn_indicator_create/sim_console_serial_apply/etc no pueden
// llamarse hasta que termina el splash.
static void sim_boot_finish(void)
{
    ui_init();
    sim_logo_secret_button_wire();
    sim_settings_nav_enable_scroll();
    sim_hmi_conn_indicator_create();
    sim_console_serial_apply();
    sim_version_panel_populate();
    sim_console_battery_seed();
    sim_robot_battery_seed();
    sim_camera_settings_init_hidden();
    sim_camera_rl1_mode_wire();
    sim_dev_mode_wire();
}

static void splash_delete_cb(lv_timer_t *t)
{
    (void)t;
    if (s_splash_img) { lv_obj_delete(s_splash_img); s_splash_img = NULL; }
    sim_boot_finish();
}

static void splash_fadeout_cb(lv_timer_t *t)
{
    (void)t;
    if (s_splash_img) lv_obj_fade_out(s_splash_img, 400, 0);
    lv_timer_t *del_t = lv_timer_create(splash_delete_cb, 400, NULL);
    lv_timer_set_repeat_count(del_t, 1);
}

// Ver comentario de arriba: LVGL fuerza cada screen a (0,0) al cargarlo, asi
// que lo re-centramos por polling en vez de por evento (el evento de
// display no se dispara con las animaciones de fade que usa ui.c).
static void screen_reposition_timer_cb(lv_timer_t *t)
{
    lv_display_t *disp = (lv_display_t *)lv_timer_get_user_data(t);

    // Primero recalcula el layout (ancho/alto de MOTOR/SERVOS segun el
    // tamaño actual de la ventana) — el HMI se centra segun lo que esto deje.
    sim_side_panels_update_layout();
    int32_t hmi_x = sim_get_hmi_x();

    lv_obj_t *act  = lv_display_get_screen_active(disp);
    lv_obj_t *prev = lv_display_get_screen_prev(disp);
    if (act  && (lv_obj_get_x(act)  != hmi_x || lv_obj_get_y(act)  != 0)) lv_obj_set_pos(act,  hmi_x, 0);
    if (prev && (lv_obj_get_x(prev) != hmi_x || lv_obj_get_y(prev) != 0)) lv_obj_set_pos(prev, hmi_x, 0);
    sim_log_panel_update_layout();
}

int main(void)
{
    lv_init();

    lv_display_t *disp = lv_sdl_window_create(SIM_WINDOW_W, SIM_WINDOW_H);
    lv_sdl_window_set_title(disp, "WP_P_V9 - Simulador de UI");
    lv_sdl_window_set_resizeable(disp, true);
    sim_set_main_display(disp);

    lv_sdl_mouse_create();
    lv_sdl_mousewheel_create();
    lv_sdl_keyboard_create();

    // lv_sdl_window_create() solo inicializa SDL_INIT_VIDEO — el soporte de
    // joystick/gamepad (control PS3/Xbox/etc por USB, ver kb_poll_timer_cb
    // en stubs/robot_panel.c) se prende aparte.
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);

    sim_log_panel_create();
    lv_timer_create(screen_reposition_timer_cb, 16, disp);

    // Splash: pantalla negra + logo centrado (se recentra solo, junto con
    // el resto del HMI, via screen_reposition_timer_cb de arriba). Al
    // terminar (~1900ms), sim_boot_finish() recien ahi llama ui_init().
    // OJO: cualquier screen ACTIVO (act_scr) reporta lv_obj_get_width() ==
    // resolucion horizontal de la display (1440), sin importar que tamaño se
    // le pida con lv_obj_set_size() — probado (imprimiendo el valor real).
    // No afecta al HMI real (screens.c) porque sus hijos usan coordenadas
    // absolutas, no porcentajes/centrado relativos al ancho del screen — pero
    // ROMPE lv_obj_center() aca, que sí depende de ese ancho (centraba
    // contra 1440px en vez de 800px, dejando el logo corrido a la derecha).
    // Arreglo: en vez de centrar contra el screen, se aprovecha que la
    // imagen nativa YA es 800x480 (el ancho real de diseño) — posicionandola
    // en (0,0) su propio pivote (centro de sus 800x480, por defecto) cae
    // exactamente en el centro real del HMI, sin depender del ancho (mal
    // reportado) del screen padre.
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_size(scr, SIM_HMI_W, SIM_WINDOW_H);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_screen_load(scr);
    s_splash_img = lv_img_create(scr);
    lv_img_set_src(s_splash_img, &welltepp);
    // El wordmark dentro del asset esta dibujado mucho mas grande que su
    // propio lienzo 800x480 (a 100% no entra ni de cerca) — se achica a 25%
    // para que quede contenido dentro del HMI.
    lv_img_set_zoom(s_splash_img, 115); // 256 = 100% (~45%)
    lv_obj_set_pos(s_splash_img, 0, 0);
    lv_timer_t *hold_t = lv_timer_create(splash_fadeout_cb, 1500, NULL);
    lv_timer_set_repeat_count(hold_t, 1);

    // Columna izquierda: conexion RS485 + joystick de MOTOR + sensores +
    // controles solo-simulador (Simular datos, Capturar, Modelo, Ping/Pong).
    // No dependen de objects.* (eso lo arma ui_init(), recien al terminar el
    // splash), pueden crearse ya.
    sim_motor_panel_create();

    // Columna derecha: joystick de SERVOS + sliders.
    sim_servo_panel_create();

    while (1) {
        uint32_t next_ms = lv_timer_handler();
        if (next_ms == LV_NO_TIMER_READY) next_ms = 50;
        lv_delay_ms(next_ms);
    }

    return 0;
}
