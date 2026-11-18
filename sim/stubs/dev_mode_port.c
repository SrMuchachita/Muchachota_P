/*
 * Port de "MODO DESARROLLADOR" (DEV MODE) para el simulador de PC — es una
 * REPLICA FUNCIONAL simplificada de la logica real en main/main.c. No hay
 * copia automatica: main.c no se compila para el simulador (tiene codigo
 * especifico de ESP32), asi que esto esta escrito a mano, igual que
 * bigview_port.c (Panel de Manejo) o modes_port.c (Modos).
 *
 * Simplificaciones respecto al original (ver conversacion para el detalle
 * completo de que hace cada parte en main.c):
 *   - Sin NVS real: el PIN vive solo en memoria (se resetea cada vez que se
 *     relanza el simulador, siempre arranca en el default "2402").
 *   - Sin editor de "Serie"/"Nombre de dispositivo" ni botones de heap
 *     libre/reiniciar/test de buzzer — dependen de NVS/ESP-IDF real y no
 *     aportan nada en PC.
 *   - Los 5 paneles auxiliares (Logs/Serial/Joystick diag/Values/Test) se
 *     crean TODOS juntos al desbloquear DEV MODE, en vez del sistema de
 *     "paneles visibles desde el arranque" persistido en NVS del original
 *     (ahi son independientes del desbloqueo por PIN; aca no).
 *   - Joystick diag / Values se alimentan del control fisico/teclado del
 *     simulador (robot_panel_get_joy_debug/get_motor_servo_debug, ver
 *     robot_panel.c) en vez de tramas UART reales — aca no hay consola real
 *     de por medio, asi que no hay nada que leer del UART.
 *   - "Test" (D-pad) SI manda comandos reales: usa el mismo hmi_send_data()
 *     que ya esta enganchado al Modbus/RS485 real en este simulador (ver
 *     hmi_send_data() en robot_panel.c) — sirve para probar el robot
 *     directo, sin pasar por ninguna otra pantalla.
 *
 * Como entrar: triple-toque sobre "Sistema > Version" (dentro de ~2s),
 * despues PIN "2402" (default) en el teclado que aparece.
 */

#include "lvgl.h"
#include "main.h"
#include "screens.h"
#include "sim_theme.h"
#include "sim_log.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// ---- Snapshot en vivo del joystick/motor/servos, expuesto por robot_panel.c ----
extern void robot_panel_get_joy_debug(float *j1x, float *j1y, float *j2x, float *j2y, bool *btn1, bool *btn2);
extern void robot_panel_get_motor_servo_debug(int32_t *cmd, int32_t *vel, int32_t *srv1, int32_t *srv2, int32_t *srv3);

#define DEV_PIN_DEF "2402"
#define DEV_PIN_LEN 4

static char s_dev_pin[DEV_PIN_LEN + 1] = DEV_PIN_DEF;

static void dev_cb_change_pin(lv_event_t *e); // definidas junto al teclado de PIN, mas abajo
static void dev_cb_reset_pin(lv_event_t *e);

// ==================== Indicador de puntos (mientras se cuentan los toques) ====================

static lv_obj_t   *s_dot_label = NULL;
static lv_timer_t *s_dot_timer = NULL;

static void dot_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (s_dot_label) { lv_obj_delete(s_dot_label); s_dot_label = NULL; }
    s_dot_timer = NULL;
}

static void dot_show(int count)
{
    static const char *texts[] = { "", ".", "..", "..." };
    if (!s_dot_label) {
        s_dot_label = lv_label_create(lv_layer_top());
        lv_obj_set_style_bg_color(s_dot_label, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(s_dot_label, 140, 0);
        lv_obj_set_style_text_color(s_dot_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_pad_all(s_dot_label, 6, 0);
        lv_obj_set_style_radius(s_dot_label, 4, 0);
        lv_obj_align(s_dot_label, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    }
    lv_label_set_text(s_dot_label, texts[count > 3 ? 3 : count]);
    if (s_dot_timer) lv_timer_reset(s_dot_timer);
    else {
        s_dot_timer = lv_timer_create(dot_timer_cb, 1500, NULL);
        lv_timer_set_repeat_count(s_dot_timer, 1);
    }
}

static void dot_hide(void)
{
    if (s_dot_timer) { lv_timer_delete(s_dot_timer); s_dot_timer = NULL; }
    if (s_dot_label) { lv_obj_delete(s_dot_label); s_dot_label = NULL; }
}

// ==================== Paneles DEV: estado compartido ====================

enum { DEVP_DEV = 0, DEVP_LOGS, DEVP_SERIAL, DEVP_JOY, DEVP_VAL, DEVP_TEST, DEVP_COUNT };
static lv_obj_t *s_panel[DEVP_COUNT]  = { 0 };
static lv_obj_t *s_navbtn[DEVP_COUNT] = { 0 };
static const char *s_panel_labels[DEVP_COUNT] = { "DEV", "Logs", "Serial", "Joystick", "Values", "Test" };

static void dev_style_btn(lv_obj_t *btn, bool active)
{
    if (!btn) return;
    lv_obj_set_style_bg_color(btn, lv_color_hex(active ? 0xfff5c518 : 0xff2a2a2a), 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(active ? 0xfff5c518 : 0xff3c3c3c), 0);
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    if (lbl) lv_obj_set_style_text_color(lbl, lv_color_hex(active ? 0xff000000 : 0xffffffff), 0);
}

// Oculta las 4 pantallas normales de Sistema, muestra SOLO el panel DEV
// pedido (idx), y pinta su boton como activo. No toca los paneles DEV entre
// si mas que para ocultar los demas.
static void dev_show_only(int idx)
{
    if (objects.sysinfo_content_device) lv_obj_add_flag(objects.sysinfo_content_device, LV_OBJ_FLAG_HIDDEN);
    if (objects.sysinfo_content_version) lv_obj_add_flag(objects.sysinfo_content_version, LV_OBJ_FLAG_HIDDEN);
    if (objects.sysinfo_content_guide)  lv_obj_add_flag(objects.sysinfo_content_guide, LV_OBJ_FLAG_HIDDEN);
    if (objects.sysinfo_content_update) lv_obj_add_flag(objects.sysinfo_content_update, LV_OBJ_FLAG_HIDDEN);
    for (int i = 0; i < DEVP_COUNT; i++) {
        if (s_panel[i]) {
            if (i == idx) lv_obj_remove_flag(s_panel[i], LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(s_panel[i], LV_OBJ_FLAG_HIDDEN);
        }
        dev_style_btn(s_navbtn[i], i == idx);
    }
}

// Oculta SOLO los paneles DEV (deja los 4 normales de Sistema tal cual estan
// — no los toca, para no pisar lo que haya hecho el handler original del
// boton que se clickeo). Enganchado ADEMAS de su accion normal sobre
// Dispositivo/Version/Guia/Actualizar (ver sim_dev_mode_wire), igual que
// dev_nav_close_cb() en el main.c real.
static void dev_nav_close_cb(lv_event_t *e)
{
    (void)e;
    for (int i = 0; i < DEVP_COUNT; i++) {
        if (s_panel[i]) lv_obj_add_flag(s_panel[i], LV_OBJ_FLAG_HIDDEN);
        dev_style_btn(s_navbtn[i], false);
    }
}

static void dev_panel_show_cb(lv_event_t *e)   { (void)e; dev_show_only(DEVP_DEV); }
static void logs_panel_show_cb(lv_event_t *e)  { (void)e; dev_show_only(DEVP_LOGS); }
static void serial_panel_show_cb(lv_event_t *e){ (void)e; dev_show_only(DEVP_SERIAL); }
static void joy_panel_show_cb(lv_event_t *e)   { (void)e; dev_show_only(DEVP_JOY); }
static void val_panel_show_cb(lv_event_t *e)   { (void)e; dev_show_only(DEVP_VAL); }
static void test_panel_show_cb(lv_event_t *e)  { (void)e; dev_show_only(DEVP_TEST); }

static lv_event_cb_t s_panel_show_cbs[DEVP_COUNT] = {
    dev_panel_show_cb, logs_panel_show_cb, serial_panel_show_cb,
    joy_panel_show_cb, val_panel_show_cb, test_panel_show_cb
};

// ==================== Logs (alimentado por dev_logs_feed(), ver hmi_log() en main_stub.c) ====================

#define LOGS_BUF_SIZE 4096
static char      s_logs_buf[LOGS_BUF_SIZE] = { 0 };
static lv_obj_t *s_logs_body = NULL;
static DWORD     s_logs_t0 = 0;

void dev_logs_feed(const char *msg)
{
    if (!s_logs_t0) s_logs_t0 = GetTickCount();
    DWORD ms = GetTickCount() - s_logs_t0;
    char line[160];
    snprintf(line, sizeof(line), "[%3lu.%03lus] %s\n", (unsigned long)(ms / 1000), (unsigned long)(ms % 1000), msg ? msg : "");

    size_t cur = strlen(s_logs_buf), add = strlen(line);
    if (cur + add >= LOGS_BUF_SIZE) {
        size_t drop = LOGS_BUF_SIZE / 4;
        char *nl = strchr(s_logs_buf + drop, '\n');
        size_t from = nl ? (size_t)(nl - s_logs_buf) + 1 : drop;
        memmove(s_logs_buf, s_logs_buf + from, cur - from + 1);
        cur = strlen(s_logs_buf);
    }
    strncat(s_logs_buf, line, LOGS_BUF_SIZE - cur - 1);
    if (s_logs_body) lv_label_set_text(s_logs_body, s_logs_buf);
}

static void logs_clear_cb(lv_event_t *e)
{
    (void)e;
    s_logs_buf[0] = '\0';
    if (s_logs_body) lv_label_set_text(s_logs_body, "");
}

// ==================== Serial (alimentado por dev_serial_add(), ver el D-pad de Test) ====================

#define SERIAL_BUF_SIZE 4096
static char      s_serial_buf[SERIAL_BUF_SIZE] = { 0 };
static lv_obj_t *s_serial_body = NULL;

static void dev_serial_add(const char *dir, const char *msg)
{
    char line[160];
    snprintf(line, sizeof(line), "%s: %s\n", dir, msg ? msg : "");
    size_t cur = strlen(s_serial_buf), add = strlen(line);
    if (cur + add >= SERIAL_BUF_SIZE) {
        size_t drop = SERIAL_BUF_SIZE / 4;
        char *nl = strchr(s_serial_buf + drop, '\n');
        size_t from = nl ? (size_t)(nl - s_serial_buf) + 1 : drop;
        memmove(s_serial_buf, s_serial_buf + from, cur - from + 1);
        cur = strlen(s_serial_buf);
    }
    strncat(s_serial_buf, line, SERIAL_BUF_SIZE - cur - 1);
    if (s_serial_body) lv_label_set_text(s_serial_body, s_serial_buf);
}

static void serial_clear_cb(lv_event_t *e)
{
    (void)e;
    s_serial_buf[0] = '\0';
    if (s_serial_body) lv_label_set_text(s_serial_body, "");
}

// ==================== Joystick diag + Values (refrescan cada 150ms) ====================

static lv_obj_t *s_joy_body = NULL;
static lv_obj_t *s_val_body = NULL;

static const char *s_motor_names[5] = { "STOP", "FORWARD", "REVERSE", "RIGHT", "LEFT" };

static void dev_refresh_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_joy_body && !s_val_body) return;

    float j1x, j1y, j2x, j2y; bool b1, b2;
    robot_panel_get_joy_debug(&j1x, &j1y, &j2x, &j2y, &b1, &b2);

    if (s_joy_body) {
        char buf[128];
        snprintf(buf, sizeof(buf), "J1X=%+.2f J1Y=%+.2f\nJ2X=%+.2f J2Y=%+.2f\nB1(L3)=%d B2(R3)=%d",
                 (double)j1x, (double)j1y, (double)j2x, (double)j2y, b1 ? 1 : 0, b2 ? 1 : 0);
        lv_label_set_text(s_joy_body, buf);
    }
    if (s_val_body) {
        int32_t cmd = 0, vel = 0, srv1 = 0, srv2 = 0, srv3 = 0;
        robot_panel_get_motor_servo_debug(&cmd, &vel, &srv1, &srv2, &srv3);
        const char *name = (cmd >= 0 && cmd < 5) ? s_motor_names[cmd] : "?";
        char buf[160];
        snprintf(buf, sizeof(buf),
                 "JOY CONV:\nJ1X=%+.2f J1Y=%+.2f\n\nMOTOR:\ncmd=%d (%s) vel=%d\n\nANGULOS SERVO:\nsrv1=%d srv2=%d srv3=%d",
                 (double)j1x, (double)j1y, (int)cmd, name, (int)vel, (int)srv1, (int)srv2, (int)srv3);
        lv_label_set_text(s_val_body, buf);
    }
}

// ==================== Test (D-pad, manda comandos reales por hmi_send_data) ====================

static lv_obj_t *s_test_status = NULL;
static int32_t   s_test_vel = 500;
static lv_obj_t *s_test_vel_lbl = NULL;

static void test_send_motor(int32_t cmd, const char *name)
{
    int32_t packed = (cmd << 16) | (s_test_vel & 0xFFFF);
    hmi_send_data(HMI_REG_MOTOR, packed);
    if (s_test_status) {
        char buf[48];
        snprintf(buf, sizeof(buf), "CMD: %s   VEL: %d", name, (int)s_test_vel);
        lv_label_set_text(s_test_status, buf);
    }
    char line[48];
    snprintf(line, sizeof(line), "MOTOR cmd=%d vel=%d (%s)", (int)cmd, (int)s_test_vel, name);
    dev_serial_add("TX", line);
}

static void test_cb_forward(lv_event_t *e) { (void)e; test_send_motor(1, "FORWARD"); }
static void test_cb_reverse(lv_event_t *e) { (void)e; test_send_motor(2, "REVERSE"); }
static void test_cb_right(lv_event_t *e)   { (void)e; test_send_motor(3, "RIGHT"); }
static void test_cb_left(lv_event_t *e)    { (void)e; test_send_motor(4, "LEFT"); }
static void test_cb_stop(lv_event_t *e)    { (void)e; test_send_motor(0, "STOP"); }

static void test_vel_slider_cb(lv_event_t *e)
{
    lv_obj_t *sl = lv_event_get_target(e);
    s_test_vel = lv_slider_get_value(sl);
    if (s_test_vel_lbl) {
        char buf[8]; snprintf(buf, sizeof(buf), "%d", (int)s_test_vel);
        lv_label_set_text(s_test_vel_lbl, buf);
    }
}

// ==================== Helpers de layout comunes a los paneles ====================

static lv_obj_t *dev_panel_base_create(lv_obj_t *parent, const char *title)
{
    lv_obj_t *p = lv_obj_create(parent);
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_size(p, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(p, 0, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_set_style_radius(p, 0, 0);
    lv_obj_set_style_layout(p, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(p, LV_FLEX_FLOW_COLUMN, 0);
    lv_obj_set_style_pad_row(p, 10, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(p, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *t = lv_label_create(p);
    lv_obj_set_style_text_color(t, lv_color_hex(0xfff5c518), 0);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_20, 0);
    lv_obj_set_style_border_side(t, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_width(t, 4, 0);
    lv_obj_set_style_border_color(t, lv_color_hex(0xfff5c518), 0);
    lv_obj_set_style_pad_left(t, 8, 0);
    lv_label_set_text(t, title);
    return p;
}

static lv_obj_t *dev_terminal_box_create(lv_obj_t *parent, lv_obj_t **out_body)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, LV_PCT(100), 260);
    lv_obj_set_style_bg_color(box, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(box, 255, 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(0xff3c3c3c), 0);
    lv_obj_set_style_pad_all(box, 8, 0);
    lv_obj_set_style_radius(box, 4, 0);
    lv_obj_set_scroll_dir(box, LV_DIR_VER);

    lv_obj_t *body = lv_label_create(box);
    lv_obj_set_width(body, LV_PCT(100));
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(body, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(body, lv_color_hex(0xffd0d0d0), 0);
    lv_label_set_text(body, "");
    *out_body = body;
    return box;
}

static lv_obj_t *dev_small_btn_create_w(lv_obj_t *parent, const char *text, lv_event_cb_t cb, int32_t width)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, width, 40);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xff2a2a2a), 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xff3c3c3c), 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_style_shadow_opa(btn, 0, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_obj_center(lbl);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_label_set_text(lbl, text);
    return btn;
}

static lv_obj_t *dev_small_btn_create(lv_obj_t *parent, const char *text, lv_event_cb_t cb)
{
    return dev_small_btn_create_w(parent, text, cb, 160);
}

// ==================== Creacion de los 6 paneles + botones de nav ====================

static void dev_create_all_panels(lv_obj_t *content_area, lv_obj_t *nav_col)
{
    if (s_panel[DEVP_DEV]) return; // ya creados (re-desbloqueo en la misma sesion)

    // ---- DEV ----
    {
        lv_obj_t *p = dev_panel_base_create(content_area, " MODO DESARROLLADOR");
        s_panel[DEVP_DEV] = p;
        lv_obj_remove_flag(p, LV_OBJ_FLAG_HIDDEN); // el DEV se muestra apenas se desbloquea

        lv_obj_t *note = lv_label_create(p);
        lv_obj_set_width(note, LV_PCT(100));
        lv_label_set_long_mode(note, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(note, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(note, lv_color_hex(0xff9a9a9a), 0);
        lv_label_set_text(note, "Version simulador (PC) del modo desarrollador real - ver Logs/Serial/Joystick/Values/Test en la lista de la izquierda.");

        lv_obj_t *row = lv_obj_create(p);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, 0, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_style_layout(row, LV_LAYOUT_FLEX, 0);
        lv_obj_set_style_flex_flow(row, LV_FLEX_FLOW_ROW_WRAP, 0);
        lv_obj_set_style_pad_column(row, 10, 0);
        lv_obj_set_style_pad_row(row, 10, 0);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        dev_small_btn_create_w(row, "Cambiar PIN", dev_cb_change_pin, 150);
        dev_small_btn_create_w(row, "Resetear PIN a 2402", dev_cb_reset_pin, 190);
        dev_small_btn_create_w(row, "Cerrar modo desarrollador", dev_nav_close_cb, 220);
    }

    // ---- Logs ----
    {
        lv_obj_t *p = dev_panel_base_create(content_area, " SYSTEM LOGS");
        s_panel[DEVP_LOGS] = p;
        dev_terminal_box_create(p, &s_logs_body);
        lv_label_set_text(s_logs_body, s_logs_buf);
        dev_small_btn_create(p, "Limpiar logs", logs_clear_cb);
    }

    // ---- Serial ----
    {
        lv_obj_t *p = dev_panel_base_create(content_area, " SERIAL MONITOR");
        s_panel[DEVP_SERIAL] = p;
        lv_obj_t *info = lv_label_create(p);
        lv_obj_set_style_text_font(info, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(info, lv_color_hex(0xff9a9a9a), 0);
        lv_label_set_text(info, "UART: 115200 8N1 (aca: comandos generados por el D-pad de Test)");
        dev_terminal_box_create(p, &s_serial_body);
        lv_label_set_text(s_serial_body, s_serial_buf);
        dev_small_btn_create(p, "Limpiar serial", serial_clear_cb);
    }

    // ---- Joystick diag ----
    {
        lv_obj_t *p = dev_panel_base_create(content_area, " JOYSTICK MONITOR");
        s_panel[DEVP_JOY] = p;
        lv_obj_t *info = lv_label_create(p);
        lv_obj_set_style_text_font(info, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(info, lv_color_hex(0xff9a9a9a), 0);
        lv_label_set_text(info, "J1=stick MOTOR (WASD/izq.) J2=stick SERVOS (flechas/der.) - vivo, 150ms");
        dev_terminal_box_create(p, &s_joy_body);
    }

    // ---- Values ----
    {
        lv_obj_t *p = dev_panel_base_create(content_area, " VALORES MONITOR");
        s_panel[DEVP_VAL] = p;
        lv_obj_t *info = lv_label_create(p);
        lv_obj_set_style_text_font(info, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(info, lv_color_hex(0xff9a9a9a), 0);
        lv_label_set_text(info, "Modo: JOY -> SERVO/MOTOR (cada 150ms)");
        dev_terminal_box_create(p, &s_val_body);
    }

    // ---- Test (D-pad) ----
    {
        lv_obj_t *p = dev_panel_base_create(content_area, " ROBOT TEST");
        s_panel[DEVP_TEST] = p;

        lv_obj_t *note = lv_label_create(p);
        lv_obj_set_style_text_font(note, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(note, lv_color_hex(0xff9a9a9a), 0);
        lv_label_set_text(note, "Manda HMI_REG_MOTOR real por RS485 (mismo camino que el resto del simulador).");

        s_test_status = lv_label_create(p);
        lv_obj_set_style_text_color(s_test_status, lv_color_hex(0xffffffff), 0);
        lv_label_set_text(s_test_status, "CMD: --   VEL: 500");

        // D-pad 3x3 (solo 5 celdas usadas)
        lv_obj_t *grid = lv_obj_create(p);
        lv_obj_set_size(grid, 3 * 90 + 2 * 8, 3 * 50 + 2 * 8);
        lv_obj_set_style_bg_opa(grid, 0, 0);
        lv_obj_set_style_border_width(grid, 0, 0);
        lv_obj_set_style_pad_all(grid, 0, 0);
        lv_obj_set_style_pad_row(grid, 8, 0);
        lv_obj_set_style_pad_column(grid, 8, 0);
        static int32_t col_dsc[] = { 90, 90, 90, LV_GRID_TEMPLATE_LAST };
        static int32_t row_dsc[] = { 50, 50, 50, LV_GRID_TEMPLATE_LAST };
        lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
        lv_obj_set_style_layout(grid, LV_LAYOUT_GRID, 0);
        lv_obj_remove_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *b;
        b = dev_small_btn_create(grid, "^ FWD", test_cb_forward);
        lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
        b = dev_small_btn_create(grid, "< LEFT", test_cb_left);
        lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
        b = dev_small_btn_create(grid, "STOP", test_cb_stop);
        lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
        b = dev_small_btn_create(grid, "RIGHT >", test_cb_right);
        lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
        b = dev_small_btn_create(grid, "v REV", test_cb_reverse);
        lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 2, 1);

        lv_obj_t *velrow = lv_obj_create(p);
        lv_obj_set_size(velrow, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(velrow, 0, 0);
        lv_obj_set_style_border_width(velrow, 0, 0);
        lv_obj_set_style_pad_all(velrow, 0, 0);
        lv_obj_remove_flag(velrow, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t *vlbl = lv_label_create(velrow);
        lv_obj_set_style_text_color(vlbl, lv_color_hex(0xffffffff), 0);
        lv_label_set_text(vlbl, "Velocidad:");
        lv_obj_align(vlbl, LV_ALIGN_TOP_LEFT, 0, 6);
        lv_obj_t *sl = lv_slider_create(velrow);
        lv_obj_set_size(sl, 260, 10);
        lv_obj_align(sl, LV_ALIGN_TOP_LEFT, 90, 8);
        lv_slider_set_range(sl, 0, 1000);
        lv_slider_set_value(sl, s_test_vel, LV_ANIM_OFF);
        lv_obj_add_event_cb(sl, test_vel_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
        s_test_vel_lbl = lv_label_create(velrow);
        lv_obj_set_style_text_color(s_test_vel_lbl, lv_color_hex(0xfff5c518), 0);
        lv_obj_align(s_test_vel_lbl, LV_ALIGN_TOP_LEFT, 360, 6);
        char vbuf[8]; snprintf(vbuf, sizeof(vbuf), "%d", (int)s_test_vel);
        lv_label_set_text(s_test_vel_lbl, vbuf);
    }

    // ---- Botones de navegacion (columna izquierda de Sistema) ----
    lv_obj_add_flag(nav_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(nav_col, LV_SCROLLBAR_MODE_AUTO);
    for (int i = 0; i < DEVP_COUNT; i++) {
        lv_obj_t *btn = lv_button_create(nav_col);
        s_navbtn[i] = btn;
        lv_obj_set_size(btn, LV_PCT(100), 60);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_style_shadow_opa(btn, 0, 0);
        lv_obj_add_event_cb(btn, s_panel_show_cbs[i], LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
        lv_label_set_text(lbl, s_panel_labels[i]);
        lv_obj_center(lbl);
        dev_style_btn(btn, i == DEVP_DEV);
    }

    static lv_timer_t *s_refresh_timer = NULL;
    if (!s_refresh_timer) s_refresh_timer = lv_timer_create(dev_refresh_timer_cb, 150, NULL);
}

// ==================== PIN keypad ====================

static lv_obj_t *s_pin_panel = NULL;
static lv_obj_t *s_pin_display = NULL;
static char      s_pin_buf[DEV_PIN_LEN + 1] = "";
static bool      s_pin_changing = false;
static lv_timer_t *s_pin_err_timer = NULL;

static void pin_close(void)
{
    if (s_pin_panel) { lv_obj_delete(s_pin_panel); s_pin_panel = NULL; }
    s_pin_display = NULL;
    if (s_pin_err_timer) { lv_timer_delete(s_pin_err_timer); s_pin_err_timer = NULL; }
}

static void pin_update_display(void)
{
    if (!s_pin_display) return;
    char buf[8] = "";
    int n = (int)strlen(s_pin_buf);
    for (int i = 0; i < DEV_PIN_LEN; i++) strcat(buf, i < n ? "*" : "_");
    lv_label_set_text(s_pin_display, buf);
}

static void pin_err_timer_cb(lv_timer_t *t)
{
    (void)t;
    s_pin_buf[0] = '\0';
    pin_update_display();
    s_pin_err_timer = NULL;
}

static void pin_cancel_cb(lv_event_t *e) { (void)e; pin_close(); }

static void pin_key_cb(lv_event_t *e)
{
    const char *k = (const char *)lv_event_get_user_data(e);
    int n = (int)strlen(s_pin_buf);
    if (strcmp(k, "X") == 0) {
        if (n > 0) s_pin_buf[n - 1] = '\0';
        pin_update_display();
        return;
    }
    if (strcmp(k, "K") == 0) {
        if (s_pin_changing) {
            strncpy(s_dev_pin, s_pin_buf, DEV_PIN_LEN);
            s_dev_pin[DEV_PIN_LEN] = '\0';
            sim_log("DEV MODE: PIN cambiado");
            pin_close();
        } else if (strcmp(s_pin_buf, s_dev_pin) == 0) {
            pin_close();
            lv_obj_t *content_area = lv_obj_get_parent(objects.sysinfo_content_device);
            lv_obj_t *nav_col      = lv_obj_get_parent(objects.sysinfo_btn_device);
            dev_create_all_panels(content_area, nav_col);
            dev_show_only(DEVP_DEV);
            sim_log("DEV MODE: desbloqueado");
        } else {
            lv_label_set_text(s_pin_display, "INCORRECTO");
            if (s_pin_err_timer) lv_timer_reset(s_pin_err_timer);
            else { s_pin_err_timer = lv_timer_create(pin_err_timer_cb, 1200, NULL); lv_timer_set_repeat_count(s_pin_err_timer, 1); }
        }
        return;
    }
    if (n < DEV_PIN_LEN) { s_pin_buf[n] = k[0]; s_pin_buf[n + 1] = '\0'; }
    pin_update_display();
}

static void pin_make_key(lv_obj_t *grid, const char *text, const char *code, int col, int row)
{
    lv_obj_t *btn = lv_button_create(grid);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xff2a2a2a), 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xff3c3c3c), 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_style_shadow_opa(btn, 0, 0);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_add_event_cb(btn, pin_key_cb, LV_EVENT_CLICKED, (void *)code);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
    lv_label_set_text(lbl, text);
    lv_obj_center(lbl);
}

static void pin_open(bool for_change)
{
    if (s_pin_panel) return;
    s_pin_changing = for_change;
    s_pin_buf[0] = '\0';

    lv_obj_t *ov = lv_obj_create(lv_layer_top());
    s_pin_panel = ov;
    lv_obj_set_size(ov, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(ov, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(ov, 180, 0);
    lv_obj_set_style_border_width(ov, 0, 0);
    lv_obj_set_style_radius(ov, 0, 0);
    lv_obj_set_style_pad_all(ov, 0, 0);
    lv_obj_remove_flag(ov, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(ov);
    lv_obj_set_size(card, 300, 380);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xfff5c518), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_pad_all(card, 14, 0);
    lv_obj_set_style_layout(card, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(card, LV_FLEX_FLOW_COLUMN, 0);
    lv_obj_set_style_flex_main_place(card, LV_FLEX_ALIGN_START, 0);
    lv_obj_set_style_flex_cross_place(card, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_row(card, 10, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(card);
    lv_obj_set_style_text_color(title, lv_color_hex(0xfff5c518), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_label_set_text(title, for_change ? "CAMBIAR PIN" : "MODO DESARROLLADOR");

    s_pin_display = lv_label_create(card);
    lv_obj_set_style_text_color(s_pin_display, lv_color_hex(0xffffffff), 0);
    lv_obj_set_style_text_font(s_pin_display, &lv_font_montserrat_20, 0);
    pin_update_display();

    lv_obj_t *grid = lv_obj_create(card);
    lv_obj_set_size(grid, 3 * 70 + 2 * 6, 4 * 46 + 3 * 6);
    lv_obj_set_style_bg_opa(grid, 0, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_style_pad_row(grid, 6, 0);
    lv_obj_set_style_pad_column(grid, 6, 0);
    static int32_t col_dsc[] = { 70, 70, 70, LV_GRID_TEMPLATE_LAST };
    static int32_t row_dsc[] = { 46, 46, 46, 46, LV_GRID_TEMPLATE_LAST };
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_layout(grid, LV_LAYOUT_GRID, 0);
    lv_obj_remove_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    pin_make_key(grid, "1", "1", 0, 0); pin_make_key(grid, "2", "2", 1, 0); pin_make_key(grid, "3", "3", 2, 0);
    pin_make_key(grid, "4", "4", 0, 1); pin_make_key(grid, "5", "5", 1, 1); pin_make_key(grid, "6", "6", 2, 1);
    pin_make_key(grid, "7", "7", 0, 2); pin_make_key(grid, "8", "8", 1, 2); pin_make_key(grid, "9", "9", 2, 2);
    pin_make_key(grid, LV_SYMBOL_BACKSPACE, "X", 0, 3); pin_make_key(grid, "0", "0", 1, 3); pin_make_key(grid, "OK", "K", 2, 3);

    lv_obj_t *cancel = lv_button_create(card);
    lv_obj_set_size(cancel, 120, 36);
    lv_obj_set_style_bg_color(cancel, lv_color_hex(0xff2a2a2a), 0);
    lv_obj_set_style_border_color(cancel, lv_color_hex(0xff3c3c3c), 0);
    lv_obj_set_style_border_width(cancel, 2, 0);
    lv_obj_set_style_radius(cancel, 6, 0);
    lv_obj_add_event_cb(cancel, pin_cancel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *clbl = lv_label_create(cancel);
    lv_obj_center(clbl);
    lv_obj_set_style_text_color(clbl, lv_color_hex(0xffffffff), 0);
    lv_label_set_text(clbl, "Cancelar");
}

static void dev_cb_change_pin(lv_event_t *e) { (void)e; pin_open(true); }
static void dev_cb_reset_pin(lv_event_t *e)
{
    (void)e;
    strncpy(s_dev_pin, DEV_PIN_DEF, DEV_PIN_LEN);
    s_dev_pin[DEV_PIN_LEN] = '\0';
    sim_log("DEV MODE: PIN reseteado a %s", DEV_PIN_DEF);
}

// ==================== Triple-toque en Sistema > Version ====================

static int  s_taps = 0;
static DWORD s_taps_t0 = 0;

static void dev_tap_event_cb(lv_event_t *e)
{
    (void)e;
    DWORD now = GetTickCount();
    if (s_taps > 0 && (now - s_taps_t0) > 2000) s_taps = 0;
    if (s_taps == 0) s_taps_t0 = now;
    s_taps++;
    sim_log("DEV MODE: toque %d/3", s_taps);
    dot_show(s_taps);

    if (s_taps >= 3) {
        s_taps = 0;
        dot_hide();
        if (s_panel[DEVP_DEV]) {
            dev_nav_close_cb(NULL); // ya estaba desbloqueado: 3 toques de nuevo = cerrar
        } else {
            pin_open(false);
        }
    }
}

// ==================== Init publico (llamar una vez desde sim_main.c) ====================

void sim_dev_mode_wire(void)
{
    if (!objects.sysinfo_content_version) return;

    // Fiel al HMI real: el gesto va sobre el PANEL DE CONTENIDO de Version
    // (no sobre el boton de nav), igual que dev_tap_event_cb() en main.c
    // enganchado a objects.sysinfo_content_version. El problema al hacerlo
    // directo sobre el panel es que sus filas de info (Firmware/Hardware/
    // etc, cada una su propio lv_obj) son clickable por default en LVGL y
    // no burbujean el evento al padre, asi que tapan el toque. Se arregla
    // agregando una capa transparente del tamaño del panel, creada AL
    // FINAL (queda arriba de todo en z-order) para que sea ella la que
    // reciba el click sin importar donde caiga dentro del panel.
    lv_obj_t *tap_layer = lv_obj_create(objects.sysinfo_content_version);
    lv_obj_add_flag(tap_layer, LV_OBJ_FLAG_IGNORE_LAYOUT); // no lo mueva el flex del panel (filas apiladas)
    lv_obj_set_pos(tap_layer, 0, 0);
    lv_obj_set_size(tap_layer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(tap_layer, 0, 0);
    lv_obj_set_style_border_width(tap_layer, 0, 0);
    lv_obj_set_style_pad_all(tap_layer, 0, 0);
    lv_obj_set_style_radius(tap_layer, 0, 0);
    lv_obj_remove_flag(tap_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(tap_layer, dev_tap_event_cb, LV_EVENT_CLICKED, NULL);

    // Salir de cualquier panel DEV al volver a las pantallas normales de
    // Sistema — sumado (no reemplaza) a su accion de siempre.
    if (objects.sysinfo_btn_device)  lv_obj_add_event_cb(objects.sysinfo_btn_device,  dev_nav_close_cb, LV_EVENT_CLICKED, NULL);
    if (objects.sysinfo_btn_version) lv_obj_add_event_cb(objects.sysinfo_btn_version, dev_nav_close_cb, LV_EVENT_CLICKED, NULL);
    if (objects.sysinfo_btn_guide)   lv_obj_add_event_cb(objects.sysinfo_btn_guide,   dev_nav_close_cb, LV_EVENT_CLICKED, NULL);
    if (objects.sysinfo_btn_update)  lv_obj_add_event_cb(objects.sysinfo_btn_update,  dev_nav_close_cb, LV_EVENT_CLICKED, NULL);
}
