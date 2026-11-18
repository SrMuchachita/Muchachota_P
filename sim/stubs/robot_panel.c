/*
 * Columnas "MOTOR" (izquierda del HMI) y "SERVOS" (derecha del HMI) del
 * simulador — hablan Modbus RTU DIRECTO con el robot por un adaptador
 * USB-RS485, igual que RD90PRO_V9_CONSOLA_VIRTUAL.py (Python) que el
 * usuario ya usaba. Las tres zonas (motor | HMI 800x480 sin tocar | servos)
 * viven en la MISMA ventana SDL (ver stubs/sim_layout.h para las medidas) —
 * sim_motor_panel_create() y sim_servo_panel_create() arman cada columna.
 *
 * Tambien dibuja los DOS joysticks virtuales (controlados por teclado, ver
 * "Joystick virtual" mas abajo) replicando el mapeo de joy1/joy2 de la placa
 * de consola real (WP_C_V9_UP/main/main.c): motor = comando discreto
 * (WASD), servos = paso incremental (flechas).
 *
 * Es una funcion NUEVA, sin relacion con el resto del simulador — no toca
 * ni depende de la UI del HMI (que solo imita lo que se ve en la pantalla
 * real). Registros Modbus (identicos a los que usa el script Python):
 *   Escritura (FC06): 0x0000=cmd motor(0-4) 0x0001=velocidad(0-1000)
 *                      0x0002=servo1 0x0003=servo2 0x0005=servo3 (0-180)
 *                      0x0004=LED PWM (0-1023)
 *   Lectura   (FC04): 0x0000=pitch 0x0001=roll 0x0002=temp 0x0003=voltage
 *                      0x0004=current 0x0005=power (todos x100 o x1000,
 *                      pitch/roll/temp con signo)
 *                      0x0006..0x000B = nombre del dispositivo (ASCII, 2
 *                      caracteres por registro)
 *
 * El puerto serie SOLO se toca desde un hilo Windows dedicado (nunca desde
 * LVGL) — la UI lee/escribe un "snapshot" compartido protegido por una
 * critical section, y un lv_timer periodico sincroniza ese snapshot con los
 * widgets. Simplificacion respecto al script Python: no hay caja de log
 * (solo una linea de estado) y no hay control por teclado WASD (solo los
 * botones en pantalla).
 */

#include "lvgl.h"
#include "modbus_rtu.h"
#include "sim_layout.h"
#include "sim_log.h"
#include "sim_theme.h"
#include "main.h" // HMI_REG_* y el prototipo real de hmi_send_data() (implementada mas abajo)
#include "vars.h" // get/set_var_brightness, set_var_angle_head/neck_value — para espejar en el HMI real
#include "screens.h" // objects.* (el slider/labels reales del HMI que se espejan)

#include <windows.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// Definida en bigview_port.c — agrega los controles "solo simulador"
// (Simular datos, Capturar pantalla, Modelo robot, Ping/Pong) como hijos de
// `parent`. Se llama al final de sim_motor_panel_create().
extern void sim_extra_controls_create(lv_obj_t *parent);

// ===================== Estado compartido hilo <-> UI =====================

typedef struct {
    CRITICAL_SECTION lock;
    HANDLE thread;
    volatile bool stop_thread;

    // Config / pedidos de conexion (UI -> hilo)
    char     port[16];
    int      baud;
    uint8_t  slave_addr;
    volatile bool connect_requested;
    volatile bool disconnect_requested;

    // Estado de conexion (hilo -> UI)
    sim_modbus_t *modbus;
    volatile bool connected;
    char status_msg[64];
    char device_name[32];
    volatile bool device_name_valid;

    // Comandos (UI -> hilo, se envian cuando cambian + watchdog cada 1500ms
    // mientras el motor esta en movimiento, igual que el script Python)
    volatile int32_t motor_cmd;  // 0=stop 1=fwd 2=rev 3=right 4=left
    volatile int32_t motor_vel;  // 0-1000
    volatile int32_t servo[3];   // 0-180
    volatile int32_t led_pwm;    // 0-1023

    volatile bool auto_read;     // leer sensores cada 1s

    // Sensores (hilo -> UI)
    float pitch, roll, temp, voltage, current, power;
    volatile bool sensors_valid;
} robot_link_t;

static robot_link_t s_link;

static DWORD WINAPI robot_link_thread(LPVOID param)
{
    robot_link_t *link = (robot_link_t *)param;

    int32_t last_vel = -1, last_cmd = -1, last_led = -1;
    int32_t last_servo[3] = { -1, -1, -1 };
    DWORD last_watchdog = GetTickCount();
    DWORD last_sensor_read = 0;

    while (!link->stop_thread) {
        EnterCriticalSection(&link->lock);
        bool want_connect    = link->connect_requested;
        bool want_disconnect = link->disconnect_requested;
        char port[16]; strncpy(port, link->port, sizeof(port));
        int baud = link->baud;
        uint8_t addr = link->slave_addr;
        link->connect_requested = false;
        link->disconnect_requested = false;
        LeaveCriticalSection(&link->lock);

        if (want_connect) {
            sim_modbus_t *mb = sim_modbus_open(port, baud);
            EnterCriticalSection(&link->lock);
            if (mb) {
                link->modbus = mb;
                link->connected = true;
                snprintf(link->status_msg, sizeof(link->status_msg),
                         "Conectado %s Addr:%d %dbps", port, addr, baud);
            } else {
                snprintf(link->status_msg, sizeof(link->status_msg),
                         "Error abriendo %s", port);
            }
            LeaveCriticalSection(&link->lock);

            if (mb) {
                Sleep(200);
                uint16_t regs[6];
                bool ok = sim_modbus_read_input_registers(mb, addr, 0x0006, 6, regs);
                if (!ok) ok = sim_modbus_read_holding_registers(mb, addr, 0x0006, 6, regs);
                if (ok) {
                    char name[16]; int p = 0;
                    for (int i = 0; i < 6 && p < 14; i++) {
                        unsigned char c1 = (unsigned char)(regs[i] >> 8);
                        unsigned char c2 = (unsigned char)(regs[i] & 0xFF);
                        if (isprint(c1)) name[p++] = (char)c1;
                        if (isprint(c2)) name[p++] = (char)c2;
                    }
                    name[p] = '\0';
                    if (p > 0) {
                        EnterCriticalSection(&link->lock);
                        strncpy(link->device_name, name, sizeof(link->device_name) - 1);
                        link->device_name_valid = true;
                        LeaveCriticalSection(&link->lock);
                    }
                }
                last_vel = last_cmd = last_led = -1;
                last_servo[0] = last_servo[1] = last_servo[2] = -1;
            }
        }

        if (want_disconnect) {
            EnterCriticalSection(&link->lock);
            if (link->modbus) { sim_modbus_close(link->modbus); link->modbus = NULL; }
            link->connected = false;
            link->device_name_valid = false;
            strncpy(link->status_msg, "Desconectado", sizeof(link->status_msg));
            LeaveCriticalSection(&link->lock);
        }

        EnterCriticalSection(&link->lock);
        bool connected = link->connected;
        sim_modbus_t *mb = link->modbus;
        int32_t vel = link->motor_vel, cmd = link->motor_cmd, led = link->led_pwm;
        int32_t srv[3] = { link->servo[0], link->servo[1], link->servo[2] };
        bool auto_read = link->auto_read;
        LeaveCriticalSection(&link->lock);

        if (connected && mb) {
            DWORD now = GetTickCount();

            if (vel != last_vel || cmd != last_cmd) {
                sim_modbus_write_register(mb, addr, 0x0001, (uint16_t)vel);
                sim_modbus_write_register(mb, addr, 0x0000, (uint16_t)cmd);
                last_vel = vel; last_cmd = cmd;
                last_watchdog = now;
            } else if (cmd != 0 && (now - last_watchdog) >= 1500) {
                // Mismo "watchdog" que el script Python: mientras se este
                // moviendo, reenvia cmd/vel cada 1.5s aunque no cambien.
                sim_modbus_write_register(mb, addr, 0x0001, (uint16_t)vel);
                sim_modbus_write_register(mb, addr, 0x0000, (uint16_t)cmd);
                last_watchdog = now;
            }

            if (srv[0] != last_servo[0]) { sim_modbus_write_register(mb, addr, 0x0002, (uint16_t)srv[0]); last_servo[0] = srv[0]; }
            if (srv[1] != last_servo[1]) { sim_modbus_write_register(mb, addr, 0x0003, (uint16_t)srv[1]); last_servo[1] = srv[1]; }
            if (srv[2] != last_servo[2]) { sim_modbus_write_register(mb, addr, 0x0005, (uint16_t)srv[2]); last_servo[2] = srv[2]; }
            if (led     != last_led)     { sim_modbus_write_register(mb, addr, 0x0004, (uint16_t)led);    last_led    = led; }

            if (auto_read && (now - last_sensor_read) >= 1000) {
                uint16_t regs[6];
                if (sim_modbus_read_input_registers(mb, addr, 0x0000, 6, regs)) {
                    int16_t p_raw = (int16_t)regs[0];
                    int16_t r_raw = (int16_t)regs[1];
                    int16_t t_raw = (int16_t)regs[2];
                    EnterCriticalSection(&link->lock);
                    link->pitch    = p_raw / 100.0f;
                    link->roll     = r_raw / 100.0f;
                    link->temp     = t_raw / 100.0f;
                    link->voltage  = regs[3] / 1000.0f;
                    link->current  = regs[4] / 1000.0f;
                    link->power    = regs[5] / 1000.0f;
                    link->sensors_valid = true;
                    LeaveCriticalSection(&link->lock);
                }
                last_sensor_read = now;
            }
        }

        Sleep(80);
    }

    if (link->modbus) { sim_modbus_close(link->modbus); link->modbus = NULL; }
    return 0;
}

// ===================== Widgets =====================

static lv_obj_t *s_status_lbl   = NULL;
static lv_obj_t *s_devname_lbl  = NULL;
static lv_obj_t *s_connect_btn  = NULL;
static lv_obj_t *s_connect_lbl  = NULL;
static lv_obj_t *s_port_dd      = NULL;
static lv_obj_t *s_baud_dd      = NULL;
static lv_obj_t *s_addr_ta      = NULL;
static lv_obj_t *s_dir_lbl      = NULL;
static lv_obj_t *s_vel_val_lbl  = NULL;
static lv_obj_t *s_vel_slider   = NULL;
static lv_obj_t *s_servo_val_lbl[3] = { NULL, NULL, NULL };
static lv_obj_t *s_servo_slider[3]  = { NULL, NULL, NULL };
static lv_obj_t *s_led_val_lbl  = NULL;
static lv_obj_t *s_led_slider   = NULL;
static lv_obj_t *s_sensor_lbl[6] = { NULL, NULL, NULL, NULL, NULL, NULL };
static lv_obj_t *s_auto_sw      = NULL;

static void robot_link_sync_timer_cb(lv_timer_t *t)
{
    (void)t;
    static char s_last_status[64] = "";
    static bool s_last_name_valid = false;

    EnterCriticalSection(&s_link.lock);
    bool connected = s_link.connected;
    char status[64]; strncpy(status, s_link.status_msg, sizeof(status));
    bool name_valid = s_link.device_name_valid;
    char name[32]; strncpy(name, s_link.device_name, sizeof(name));
    bool sensors_valid = s_link.sensors_valid;
    float pitch = s_link.pitch, roll = s_link.roll, temp = s_link.temp;
    float voltage = s_link.voltage, current = s_link.current, power = s_link.power;
    LeaveCriticalSection(&s_link.lock);

    if (strcmp(status, s_last_status) != 0) {
        sim_log("RS485: %s", status);
        strncpy(s_last_status, status, sizeof(s_last_status) - 1);
    }
    if (name_valid && !s_last_name_valid) {
        sim_log("RS485: nombre del dispositivo = \"%s\"", name);
    }
    s_last_name_valid = name_valid;

    if (s_status_lbl) lv_label_set_text(s_status_lbl, status);
    if (s_devname_lbl) {
        if (name_valid) lv_label_set_text(s_devname_lbl, name);
        else lv_label_set_text(s_devname_lbl, "");
    }
    if (s_connect_lbl) lv_label_set_text(s_connect_lbl, connected ? "DESCONECTAR" : "CONECTAR");
    if (s_connect_btn) {
        lv_obj_set_style_bg_color(s_connect_btn,
            lv_color_hex(connected ? SIM_BTN : SIM_PRIMARY), 0);
    }
    if (sensors_valid) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.2f", pitch);   if (s_sensor_lbl[0]) lv_label_set_text(s_sensor_lbl[0], buf);
        snprintf(buf, sizeof(buf), "%.2f", roll);    if (s_sensor_lbl[1]) lv_label_set_text(s_sensor_lbl[1], buf);
        snprintf(buf, sizeof(buf), "%.2f", temp);    if (s_sensor_lbl[2]) lv_label_set_text(s_sensor_lbl[2], buf);
        snprintf(buf, sizeof(buf), "%.3f", voltage); if (s_sensor_lbl[3]) lv_label_set_text(s_sensor_lbl[3], buf);
        snprintf(buf, sizeof(buf), "%.3f", current); if (s_sensor_lbl[4]) lv_label_set_text(s_sensor_lbl[4], buf);
        snprintf(buf, sizeof(buf), "%.3f", power);   if (s_sensor_lbl[5]) lv_label_set_text(s_sensor_lbl[5], buf);
    }
}

// ---- Estilos oscuros para widgets "de entrada" (dropdown/textarea) ----
// Sin esto quedan con el tema claro por defecto de LVGL (caja blanca) — VS
// Code no tiene blancos en ningun lado, ni sus inputs.

// La lista desplegable (el popup) de lv_dropdown se resiste a cualquier
// lv_obj_set_style_* normal — probado exhaustivamente (estilando el
// contenedor, el label hijo, en el momento de crear el dropdown y en
// LV_EVENT_READY al abrirlo): siempre queda blanca. Algo dentro de LVGL la
// reaplica con el tema claro por defecto en algun punto de lv_dropdown_open()
// despues de cualquier estilo que le pongamos. La unica forma que realmente
// funciona es interceptar su dibujo: se apaga su fondo normal (bg_opa
// transparente, asi el dibujo por defecto no pinta nada) y se pinta un
// rectangulo oscuro a mano en LV_EVENT_DRAW_MAIN_BEGIN, antes de que LVGL
// dibuje el texto de las opciones encima.
static void draw_list_bg_cb(lv_event_t *e)
{
    lv_obj_t *list = lv_event_get_target(e);
    lv_layer_t *layer = lv_event_get_layer(e);

    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = lv_color_hex(SIM_PANEL);
    dsc.bg_opa = LV_OPA_COVER;
    dsc.border_color = lv_color_hex(SIM_BORDER);
    dsc.border_width = 1;
    dsc.border_opa = LV_OPA_COVER;

    lv_area_t area;
    lv_obj_get_coords(list, &area);
    lv_draw_rect(layer, &dsc, &area);
}

static void style_dark_dropdown(lv_obj_t *dd)
{
    lv_obj_set_style_bg_color(dd, lv_color_hex(SIM_BTN), 0);
    lv_obj_set_style_text_color(dd, lv_color_hex(SIM_TEXT), 0);
    lv_obj_set_style_border_color(dd, lv_color_hex(SIM_BORDER), 0);
    lv_obj_set_style_border_width(dd, 1, 0);

    lv_obj_t *list = lv_dropdown_get_list(dd);
    if (list) {
        lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);   // apaga el dibujo por defecto (blanco)
        lv_obj_set_style_border_width(list, 0, 0);         // el borde lo dibujamos a mano tambien
        lv_obj_set_style_text_color(list, lv_color_hex(SIM_TEXT), 0);
        lv_obj_set_style_bg_color(list, lv_color_hex(SIM_ACCENT), LV_PART_SELECTED);
        lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SELECTED);
        lv_obj_add_event_cb(list, draw_list_bg_cb, LV_EVENT_DRAW_MAIN_BEGIN, NULL);
    }
}

static void style_dark_textarea(lv_obj_t *ta)
{
    lv_obj_set_style_bg_color(ta, lv_color_hex(SIM_BTN), 0);
    lv_obj_set_style_text_color(ta, lv_color_hex(SIM_TEXT), 0);
    lv_obj_set_style_border_color(ta, lv_color_hex(SIM_BORDER), 0);
    lv_obj_set_style_border_width(ta, 1, 0);
    lv_obj_set_style_bg_color(ta, lv_color_hex(SIM_ACCENT), LV_PART_CURSOR);
}

// ---- Conexion ----

static void refresh_ports_cb(lv_event_t *e)
{
    (void)e;
    char ports[16][16];
    int n = sim_modbus_list_ports(ports, 16);
    if (n == 0) { lv_dropdown_set_options(s_port_dd, "(sin puertos)"); return; }
    char opts[16 * 17] = {0};
    for (int i = 0; i < n; i++) {
        strcat(opts, ports[i]);
        if (i < n - 1) strcat(opts, "\n");
    }
    lv_dropdown_set_options(s_port_dd, opts);
}

static void connect_btn_cb(lv_event_t *e)
{
    (void)e;
    EnterCriticalSection(&s_link.lock);
    bool connected = s_link.connected;
    LeaveCriticalSection(&s_link.lock);

    if (connected) {
        sim_log("RS485: desconectando...");
        EnterCriticalSection(&s_link.lock);
        s_link.disconnect_requested = true;
        LeaveCriticalSection(&s_link.lock);
        return;
    }

    char port[16];
    lv_dropdown_get_selected_str(s_port_dd, port, sizeof(port));
    const char *baud_opts[] = { "9600", "19200", "38400", "57600", "115200" };
    int baud = baud_opts[lv_dropdown_get_selected(s_baud_dd)] ? atoi(baud_opts[lv_dropdown_get_selected(s_baud_dd)]) : 115200;
    int addr = atoi(lv_textarea_get_text(s_addr_ta));
    if (addr <= 0) addr = 10;

    sim_log("RS485: conectando a %s Addr:%d %dbps...", port, addr, baud);
    EnterCriticalSection(&s_link.lock);
    strncpy(s_link.port, port, sizeof(s_link.port) - 1);
    s_link.baud = baud;
    s_link.slave_addr = (uint8_t)addr;
    s_link.connect_requested = true;
    LeaveCriticalSection(&s_link.lock);
}

// ---- Motor ----

static void set_motor(int32_t cmd, const char *name, uint32_t color)
{
    EnterCriticalSection(&s_link.lock);
    s_link.motor_cmd = cmd;
    LeaveCriticalSection(&s_link.lock);
    if (s_dir_lbl) {
        lv_label_set_text(s_dir_lbl, name);
        lv_obj_set_style_text_color(s_dir_lbl, lv_color_hex(color), 0);
    }
}

static void motor_fwd_cb(lv_event_t *e)   { (void)e; set_motor(1, "FORWARD", SIM_ACCENT); }
static void motor_rev_cb(lv_event_t *e)   { (void)e; set_motor(2, "REVERSE", SIM_ACCENT); }
static void motor_right_cb(lv_event_t *e) { (void)e; set_motor(3, "RIGHT",   SIM_ACCENT); }
static void motor_left_cb(lv_event_t *e)  { (void)e; set_motor(4, "LEFT",    SIM_ACCENT); }
static void motor_stop_cb(lv_event_t *e)  { (void)e; set_motor(0, "STOP",    SIM_ACCENT); }

static void vel_slider_cb(lv_event_t *e)
{
    lv_obj_t *sl = lv_event_get_target(e);
    int32_t v = lv_slider_get_value(sl);
    EnterCriticalSection(&s_link.lock);
    s_link.motor_vel = v;
    LeaveCriticalSection(&s_link.lock);
    char buf[8]; snprintf(buf, sizeof(buf), "%d", (int)v);
    if (s_vel_val_lbl) lv_label_set_text(s_vel_val_lbl, buf);
}

// Espejo Servo1/2 -> HMI real (cabeza/cuello) — llamado desde donde sea que
// se mueva el servo por fuera del propio HMI (slider del panel, flechas del
// teclado): mismo idx/relacion que main.c/modes_port.c (srv1=cabeza,
// srv2=cuello). Solo toca label + variable; lv_slider_set_value ya se
// encarga por separado de mover el slider visual de cada caller.
static void mirror_servo_to_hmi(int idx, int32_t v)
{
    char b[8];
    if (idx == 0 && objects.angle_head_label) {
        set_var_angle_head_value(v);
        snprintf(b, sizeof(b), "%03d", (int)v);
        lv_label_set_text(objects.angle_head_label, b);
    } else if (idx == 1 && objects.angle_neck_label) {
        set_var_angle_neck_value(v);
        snprintf(b, sizeof(b), "%03d", (int)v);
        lv_label_set_text(objects.angle_neck_label, b);
    }
}

// Nucleo compartido por el slider de servo del panel Y por los botones del
// control fisico (ver pad_center_servos() mas abajo) — un solo lugar que
// escribe s_link, mueve el slider visual y espeja al HMI real.
static void apply_servo_value(int idx, int32_t v)
{
    EnterCriticalSection(&s_link.lock);
    s_link.servo[idx] = v;
    LeaveCriticalSection(&s_link.lock);
    char buf[8]; snprintf(buf, sizeof(buf), "%d", (int)v);
    if (s_servo_val_lbl[idx]) lv_label_set_text(s_servo_val_lbl[idx], buf);
    if (s_servo_slider[idx]) lv_slider_set_value(s_servo_slider[idx], v, LV_ANIM_OFF);

    mirror_servo_to_hmi(idx, v);
}

static void servo_slider_cb(lv_event_t *e)
{
    lv_obj_t *sl = lv_event_get_target(e);
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    int32_t v = lv_slider_get_value(sl);
    apply_servo_value(idx, v);
}

// Idem apply_servo_value pero para el LED — usado por el slider del panel Y
// por el boton del control fisico (ver pad_toggle_led() mas abajo).
static void apply_led_value(int32_t v)
{
    if (v < 0) v = 0;
    if (v > 1023) v = 1023;
    EnterCriticalSection(&s_link.lock);
    s_link.led_pwm = v;
    LeaveCriticalSection(&s_link.lock);
    char buf[8]; snprintf(buf, sizeof(buf), "%d", (int)v);
    if (s_led_val_lbl) lv_label_set_text(s_led_val_lbl, buf);
    if (s_led_slider) lv_slider_set_value(s_led_slider, v, LV_ANIM_OFF);

    // Espejo hacia el slider real del HMI (objects.obj16, escala 0-100%) y
    // su label de porcentaje — mismo camino que action_robot_brightness_changed()
    // en actions.c, pero sin volver a llamar hmi_send_data() (el Modbus ya
    // se escribio arriba; llamarlo de nuevo mandaria el mismo dato dos veces).
    int32_t pct = (v * 100 + 511) / 1023;
    if (objects.obj16) lv_slider_set_value(objects.obj16, pct, LV_ANIM_OFF);
    if (objects.brightness_label) {
        char pbuf[10];
        snprintf(pbuf, sizeof(pbuf), "%d %%", (int)pct);
        set_var_brightness(pbuf);
        lv_label_set_text_static(objects.brightness_label, get_var_brightness());
    }
    hmi_bigview_led_level_refresh();
}

static void led_slider_cb(lv_event_t *e)
{
    lv_obj_t *sl = lv_event_get_target(e);
    int32_t v = lv_slider_get_value(sl); // 0-1023
    apply_led_value(v);
}

static void auto_switch_cb(lv_event_t *e)
{
    bool on = lv_obj_has_state(s_auto_sw, LV_STATE_CHECKED);
    (void)e;
    EnterCriticalSection(&s_link.lock);
    s_link.auto_read = on;
    LeaveCriticalSection(&s_link.lock);
}

static void read_once_cb(lv_event_t *e)
{
    (void)e;
    // Un solo pedido: se resuelve solo si ya esta conectado, el hilo lo
    // toma en su proximo ciclo (~80ms) al forzar auto_read por un instante.
    EnterCriticalSection(&s_link.lock);
    s_link.sensors_valid = false;
    LeaveCriticalSection(&s_link.lock);
}

// ===================== Joystick virtual (teclado) =====================
// Dos joysticks dibujados (circulo + perilla), controlados por teclado ya
// que no hay un joystick fisico en la PC: el de MOTOR (ventana derecha) con
// WASD, el de SERVOS (ventana izquierda) con las flechas. La logica de
// mapeo replica main.c de WP_C_V9_UP (placa de consola real):
//   - Motor: comando discreto (STOP/FWD/REV/LEFT/RIGHT), no mezcla de ejes
//     (ver joy1_y/joy1_x en main.c: Y tiene prioridad sobre X). La velocidad
//     la sigue poniendo el slider "Velocidad", igual que si el joystick
//     real estuviera siempre al fondo de su recorrido.
//   - Servos: movimiento incremental (1 paso por tick mientras se mantiene
//     la tecla), igual que el SERVO_STEP de la consola real, no un salto
//     instantaneo al valor maximo.
// Solo el DIBUJO (el circulo con la perilla) es nuevo — el resto (cmd/vel/
// servo[]) reutiliza exactamente los mismos campos/funciones que ya
// escriben por Modbus (set_motor(), s_link.servo[]).

typedef struct {
    lv_obj_t *knob;
    lv_obj_t *lbl_xy;
} sim_joy_ui_t;

static sim_joy_ui_t s_motor_joy;
static sim_joy_ui_t s_servo_joy;
static int32_t      s_kb_last_motor_cmd = -1;

static lv_obj_t *joystick_create(lv_obj_t *parent, const char *subtitle, uint32_t accent, sim_joy_ui_t *ui_out)
{
    lv_obj_t *wrap = lv_obj_create(parent);
    lv_obj_set_size(wrap, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(wrap, 0, 0);
    lv_obj_set_style_border_width(wrap, 0, 0);
    lv_obj_set_style_pad_all(wrap, 4, 0);
    lv_obj_set_style_pad_row(wrap, 6, 0);
    lv_obj_set_style_layout(wrap, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(wrap, LV_FLEX_FLOW_COLUMN, 0);
    lv_obj_set_style_flex_main_place(wrap, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_cross_place(wrap, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_remove_flag(wrap, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *sub = lv_label_create(wrap);
    lv_obj_set_width(sub, LV_PCT(100));
    lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(SIM_TEXT_DIM), 0);
    lv_label_set_long_mode(sub, LV_LABEL_LONG_WRAP);
    lv_label_set_text(sub, subtitle);

    lv_obj_t *base = lv_obj_create(wrap);
    lv_obj_set_size(base, 170, 170);
    lv_obj_set_style_radius(base, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(base, lv_color_hex(SIM_BG), 0);
    lv_obj_set_style_border_width(base, 3, 0);
    lv_obj_set_style_border_color(base, lv_color_hex(accent), 0);
    lv_obj_set_style_pad_all(base, 0, 0);
    lv_obj_remove_flag(base, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *knob = lv_obj_create(base);
    lv_obj_set_size(knob, 16, 16);
    lv_obj_set_style_radius(knob, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(knob, lv_color_hex(accent), 0);
    lv_obj_set_style_border_width(knob, 0, 0);
    lv_obj_set_style_shadow_width(knob, 0, 0);
    lv_obj_remove_flag(knob, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(knob, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *xy = lv_label_create(wrap);
    lv_obj_set_style_text_font(xy, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(xy, lv_color_hex(SIM_TEXT), 0);
    lv_label_set_text(xy, "X: 0.00  Y: 0.00");

    ui_out->knob = knob;
    ui_out->lbl_xy = xy;
    return wrap;
}

static void joystick_set(sim_joy_ui_t *ui, float nx, float ny)
{
    float mag = sqrtf(nx * nx + ny * ny);
    if (mag > 1.0f) { nx /= mag; ny /= mag; }
    const int max_travel = 60; // px de recorrido del centro al borde
    lv_obj_align(ui->knob, LV_ALIGN_CENTER, (int)(nx * max_travel), (int)(ny * max_travel));
    char buf[24];
    snprintf(buf, sizeof(buf), "X: %.2f  Y: %.2f", (double)nx, (double)ny);
    lv_label_set_text(ui->lbl_xy, buf);
}

// Sondea el teclado fisico (SDL_GetKeyboardState, no eventos LVGL) cada
// ~30ms — necesitamos saber si una tecla esta SOSTENIDA, no solo el instante
// en que se presiona/suelta, igual que un joystick analogico real.
// ===================== Control fisico (PS3/Xbox/generico por USB) =====================
// Mismo mapeo que joy1/joy2 en la consola real (WP_C_V9_UP/main.c): stick
// IZQUIERDO = motor (equivalente a WASD), stick DERECHO = servos
// (equivalente a las flechas) — se leen como si fueran esas mismas teclas
// "sostenidas" y se le suman (OR) a lo que ya lee el teclado, asi que ambos
// funcionan en paralelo sin duplicar logica (ver kb_poll_timer_cb).
static SDL_GameController *s_pad = NULL;
static lv_obj_t           *s_pad_status_lbl = NULL; // label fijo en la columna MOTOR (siempre visible, sin depender del LOG)
#define PAD_DEADZONE 10000 // eje de -32768 a 32767; ignora el ruido/drift del centro

static void ensure_gamepad_open(void)
{
    if (s_pad && SDL_GameControllerGetAttached(s_pad)) return;

    if (s_pad) { // se desconecto el que estaba abierto
        SDL_GameControllerClose(s_pad);
        s_pad = NULL;
        sim_log_pad("Control desconectado");
        if (s_pad_status_lbl) lv_label_set_text(s_pad_status_lbl, "Control fisico: no detectado");
    }

    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            s_pad = SDL_GameControllerOpen(i);
            if (s_pad) {
                const char *name = SDL_GameControllerName(s_pad);
                sim_log_pad("Control conectado: %s", name ? name : "?");
                if (s_pad_status_lbl) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "Control fisico: %s", name ? name : "conectado");
                    lv_label_set_text(s_pad_status_lbl, buf);
                }
                break;
            }
        }
    }
}

// ---- Snapshot para los paneles de DEV MODE (dev_mode_port.c) ----
static float s_dbg_j1x = 0, s_dbg_j1y = 0, s_dbg_j2x = 0, s_dbg_j2y = 0;
static bool  s_dbg_btn_l3 = false, s_dbg_btn_r3 = false;

static void kb_poll_timer_cb(lv_timer_t *t)
{
    (void)t;
    const Uint8 *ks = SDL_GetKeyboardState(NULL);
    bool w = ks[SDL_SCANCODE_W], s = ks[SDL_SCANCODE_S];
    bool a = ks[SDL_SCANCODE_A], d = ks[SDL_SCANCODE_D];
    bool up = ks[SDL_SCANCODE_UP], down = ks[SDL_SCANCODE_DOWN];
    bool left = ks[SDL_SCANCODE_LEFT], right = ks[SDL_SCANCODE_RIGHT];

    // Posicion ANALOGICA cruda de los sticks (-1..1), para que el dibujo del
    // joystick siga el movimiento real y proporcional del control fisico en
    // vez de solo saltar a las 8 posiciones "digitales" que usa el teclado.
    // La logica de comando (cmd/vel/paso de servo) sigue siendo digital
    // (igual que joy1_y/joy1_x en la consola real), esto es solo el dibujo.
    float pad_lx = 0.0f, pad_ly = 0.0f, pad_rx = 0.0f, pad_ry = 0.0f;
    bool pad_left_active = false, pad_right_active = false;
    bool btn_l3 = false, btn_r3 = false; // para dev_mode_port.c (panel Joystick diag), ver robot_panel_get_joy_debug()

    ensure_gamepad_open();
    if (s_pad) {
        int16_t lx = SDL_GameControllerGetAxis(s_pad, SDL_CONTROLLER_AXIS_LEFTX);
        int16_t ly = SDL_GameControllerGetAxis(s_pad, SDL_CONTROLLER_AXIS_LEFTY);
        int16_t rx = SDL_GameControllerGetAxis(s_pad, SDL_CONTROLLER_AXIS_RIGHTX);
        int16_t ry = SDL_GameControllerGetAxis(s_pad, SDL_CONTROLLER_AXIS_RIGHTY);

        // Diagnostico: valores CRUDOS de ejes y botones cada ~500ms en la
        // pestaña PS3 — sirve para saber si SDL esta leyendo algo de verdad
        // (muchos DS3 sin driver especial reportan botones pero NO ejes
        // analogicos por USB en Windows sin DsHidMini/SCP, ver conversacion).
        static DWORD s_pad_diag_t0 = 0;
        DWORD now = GetTickCount();
        if (now - s_pad_diag_t0 >= 500) {
            s_pad_diag_t0 = now;
            int nbtn = 0;
            for (int b = 0; b <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT; b++) {
                if (SDL_GameControllerGetButton(s_pad, (SDL_GameControllerButton)b)) nbtn++;
            }
            sim_log_pad("ejes LX=%d LY=%d RX=%d RY=%d | botones activos=%d", lx, ly, rx, ry, nbtn);
        }

        // ---- L3/R3 (botones de los propios sticks, clickeandolos hacia
        // abajo) — replica EXACTA de J1_DIN1/J2_DIN0 en main.c de la consola
        // real (WP_C_V9_UP):
        //
        // J1 (boton del joystick de MOTOR = L3 aca) es de FLANCO (solo al
        // presionar, no repite sosteniendo) y NO es on/off: cicla brillo en
        // 5 pasos 0/25/50/75/100% (pwm = paso*1023/4), igual que
        // "j1_light_step = (j1_light_step+1) % 5" en el firmware real.
        //
        // J2 (boton del joystick de SERVOS = R3 aca) es de NIVEL (mientras
        // se sostiene, cada tick) y centra cabeza/cuello a 90° — igual que
        // "if (gpio_get_level(J2_DIN0)==0) target_srv1/2 = center" alla (sin
        // giro automatico que cancelar aca, eso no existe en el simulador).
        static bool    s_btn_l3_prev = false;
        static uint8_t s_led_step    = 0; // 0..4, igual que j1_light_step
        btn_l3 = SDL_GameControllerGetButton(s_pad, SDL_CONTROLLER_BUTTON_LEFTSTICK);
        btn_r3 = SDL_GameControllerGetButton(s_pad, SDL_CONTROLLER_BUTTON_RIGHTSTICK);

        if (btn_l3 && !s_btn_l3_prev) {
            s_led_step = (s_led_step + 1) % 5;
            int32_t pwm = (int32_t)((uint32_t)s_led_step * 1023 / 4);
            apply_led_value(pwm);
            sim_log_pad("L3: intensidad LED -> %d%% (pwm=%d)", s_led_step * 25, (int)pwm);
        }
        s_btn_l3_prev = btn_l3;

        if (btn_r3) {
            apply_servo_value(0, 90);
            apply_servo_value(1, 90);
        }

        if (ly < -PAD_DEADZONE) w = true;
        if (ly >  PAD_DEADZONE) s = true;
        if (lx < -PAD_DEADZONE) a = true;
        if (lx >  PAD_DEADZONE) d = true;
        if (ry < -PAD_DEADZONE) up = true;
        if (ry >  PAD_DEADZONE) down = true;
        if (rx < -PAD_DEADZONE) left = true;
        if (rx >  PAD_DEADZONE) right = true;

        if (lx < -PAD_DEADZONE || lx > PAD_DEADZONE || ly < -PAD_DEADZONE || ly > PAD_DEADZONE) {
            pad_lx = lx / 32768.0f;
            pad_ly = ly / 32768.0f;
            pad_left_active = true;
        }
        if (rx < -PAD_DEADZONE || rx > PAD_DEADZONE || ry < -PAD_DEADZONE || ry > PAD_DEADZONE) {
            pad_rx = rx / 32768.0f;
            pad_ry = ry / 32768.0f;
            pad_right_active = true;
        }
    }

    // ---- Joystick MOTOR (WASD o stick IZQUIERDO del control) — el comando
    // sigue siendo discreto (Y antes que X, igual que joy1_y/joy1_x en el
    // main.c de la consola real), pero el DIBUJO sigue la posicion real del
    // stick cuando hay control conectado y en uso (si no, cae al digital de
    // WASD como antes). ----
    if (pad_left_active) {
        joystick_set(&s_motor_joy, pad_lx, pad_ly);
    } else {
        joystick_set(&s_motor_joy, (d ? 1.0f : 0.0f) - (a ? 1.0f : 0.0f),
                                    (s ? 1.0f : 0.0f) - (w ? 1.0f : 0.0f));
    }

    int32_t cmd; const char *name; uint32_t color;
    if (w)      { cmd = 1; name = "FORWARD"; color = SIM_ACCENT; }
    else if (s) { cmd = 2; name = "REVERSE"; color = SIM_ACCENT; }
    else if (d) { cmd = 3; name = "RIGHT";   color = SIM_ACCENT; }
    else if (a) { cmd = 4; name = "LEFT";    color = SIM_ACCENT; }
    else        { cmd = 0; name = "STOP";    color = SIM_ACCENT; }

    if (cmd != s_kb_last_motor_cmd) {
        s_kb_last_motor_cmd = cmd;
        set_motor(cmd, name, color);
    }

    // ---- Velocidad ANALOGICA con el stick IZQUIERDO del control (replica
    // exacta de la formula de joy1_x/joy1_y en main.c de la consola real:
    // rampa lineal desde el borde de la zona muerta hasta el extremo del
    // eje, escalada 0-1000) — con teclado (WASD) la velocidad sigue siendo
    // la fija del slider "Velocidad", como siempre. Usa el MISMO eje que
    // decidio la direccion arriba (Y para fwd/rev, X para right/left),
    // igual que alla. ----
    static bool s_pad_vel_was_active = false; // para poder volver a 0 al soltar el stick (ver abajo)
    if (pad_left_active && cmd != 0) {
        float dz = (float)PAD_DEADZONE / 32768.0f;
        float mag = (cmd == 1 || cmd == 2) ? fabsf(pad_ly) : fabsf(pad_lx);
        float vraw = (mag - dz) / (1.0f - dz);
        if (vraw < 0.0f) vraw = 0.0f;
        if (vraw > 1.0f) vraw = 1.0f;
        int32_t vel = (int32_t)(vraw * 1000.0f + 0.5f);

        EnterCriticalSection(&s_link.lock);
        s_link.motor_vel = vel;
        LeaveCriticalSection(&s_link.lock);
        if (s_vel_slider) lv_slider_set_value(s_vel_slider, vel, LV_ANIM_OFF);
        char vbuf[8]; snprintf(vbuf, sizeof(vbuf), "%d", (int)vel);
        if (s_vel_val_lbl) lv_label_set_text(s_vel_val_lbl, vbuf);
        s_pad_vel_was_active = true;
    } else if (s_pad_vel_was_active) {
        // El stick volvio al centro (o el control se desconecto): la
        // rampa analogica lo devuelve a 0, igual que vel=0 en el STOP del
        // main.c real — si no, el slider quedaba pegado en el ultimo valor.
        EnterCriticalSection(&s_link.lock);
        s_link.motor_vel = 0;
        LeaveCriticalSection(&s_link.lock);
        if (s_vel_slider) lv_slider_set_value(s_vel_slider, 0, LV_ANIM_OFF);
        if (s_vel_val_lbl) lv_label_set_text(s_vel_val_lbl, "0");
        s_pad_vel_was_active = false;
    }

    // ---- Joystick SERVOS (flechas) — paso incremental (1°/tick) mientras
    // se sostiene, igual que SERVO_STEP en la consola real: Arriba/Abajo =
    // Servo1, Izquierda/Derecha = Servo2. ----
    if (pad_right_active) {
        joystick_set(&s_servo_joy, pad_rx, pad_ry);
    } else {
        joystick_set(&s_servo_joy, (right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f),
                                    (down ? 1.0f : 0.0f) - (up ? 1.0f : 0.0f));
    }

    if (up || down || left || right) {
        EnterCriticalSection(&s_link.lock);
        int32_t srv1 = s_link.servo[0];
        int32_t srv2 = s_link.servo[1];
        if (up)    srv1 = srv1 < 180 ? srv1 + 1 : 180;
        if (down)  srv1 = srv1 > 0   ? srv1 - 1 : 0;
        if (right) srv2 = srv2 < 180 ? srv2 + 1 : 180;
        if (left)  srv2 = srv2 > 0   ? srv2 - 1 : 0;
        s_link.servo[0] = srv1;
        s_link.servo[1] = srv2;
        LeaveCriticalSection(&s_link.lock);

        char buf[8];
        if (s_servo_slider[0]) lv_slider_set_value(s_servo_slider[0], srv1, LV_ANIM_OFF);
        if (s_servo_slider[1]) lv_slider_set_value(s_servo_slider[1], srv2, LV_ANIM_OFF);
        snprintf(buf, sizeof(buf), "%d", (int)srv1); if (s_servo_val_lbl[0]) lv_label_set_text(s_servo_val_lbl[0], buf);
        snprintf(buf, sizeof(buf), "%d", (int)srv2); if (s_servo_val_lbl[1]) lv_label_set_text(s_servo_val_lbl[1], buf);
        mirror_servo_to_hmi(0, srv1);
        mirror_servo_to_hmi(1, srv2);
    }

    // Snapshot para el panel "Joystick diag" de DEV MODE (dev_mode_port.c) —
    // mismo J1/J2/botones que loguea dev_joy_log_update() en el main.c real,
    // pero alimentado por el control fisico/teclado del simulador en vez de
    // tramas UART reales (aca no hay consola de por medio).
    s_dbg_j1x = pad_left_active  ? pad_lx : (d ? 1.0f : 0.0f) - (a ? 1.0f : 0.0f);
    s_dbg_j1y = pad_left_active  ? pad_ly : (s ? 1.0f : 0.0f) - (w ? 1.0f : 0.0f);
    s_dbg_j2x = pad_right_active ? pad_rx : (right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f);
    s_dbg_j2y = pad_right_active ? pad_ry : (down ? 1.0f : 0.0f) - (up ? 1.0f : 0.0f);
    s_dbg_btn_l3 = btn_l3;
    s_dbg_btn_r3 = btn_r3;
}

void robot_panel_get_joy_debug(float *j1x, float *j1y, float *j2x, float *j2y, bool *btn1, bool *btn2)
{
    if (j1x) *j1x = s_dbg_j1x;
    if (j1y) *j1y = s_dbg_j1y;
    if (j2x) *j2x = s_dbg_j2x;
    if (j2y) *j2y = s_dbg_j2y;
    if (btn1) *btn1 = s_dbg_btn_l3;
    if (btn2) *btn2 = s_dbg_btn_r3;
}

void robot_panel_get_motor_servo_debug(int32_t *cmd, int32_t *vel, int32_t *srv1, int32_t *srv2, int32_t *srv3)
{
    EnterCriticalSection(&s_link.lock);
    if (cmd)  *cmd  = s_link.motor_cmd;
    if (vel)  *vel  = s_link.motor_vel;
    if (srv1) *srv1 = s_link.servo[0];
    if (srv2) *srv2 = s_link.servo[1];
    if (srv3) *srv3 = s_link.servo[2];
    LeaveCriticalSection(&s_link.lock);
}

// ===================== HMI real -> robot real por RS485 =====================
// En la placa real, el HMI le manda estos comandos a la CONSOLA por UART
// (hmi_send_data(), ver main/main.c), y es la CONSOLA la que despues habla
// Modbus con el robot. Aca no hay consola de por medio: cualquier control
// del HMI que en la placa real termina afectando al robot (LED, servos,
// motor) se reenvia DIRECTO por la misma conexion Modbus que ya usa la
// columna MOTOR — mismo mecanismo que los sliders/joystick del simulador:
// solo se toca el estado compartido (s_link), nunca el puerto serie desde
// este hilo (eso lo hace robot_link_thread).
//
// Mapa HMI_REG_* -> registro Modbus (igual al de main/main.h):
//   ANGLE_HEAD_CHANGED (servo1/cabeza) -> 0x0002   ANGLE_NECK_CHANGED (servo2/cuello) -> 0x0003
//   ROBOT_LED_CHANGED (PWM 0-1023)     -> 0x0004
//   CENTER (bits[31:16]=cuello,[15:0]=cabeza)      -> 0x0002 + 0x0003 juntos
//   MOTOR  (bits[31:16]=cmd,[15:0]=vel)            -> 0x0000 + 0x0001 juntos
// El resto de los registros (BLE, wifi, idioma, limites, OTA, etc.) son
// exclusivamente HMI<->consola, sin equivalente en el robot — se ignoran.
void hmi_send_data(uint8_t reg, int32_t value)
{
    char buf[8];

    switch (reg) {
        case HMI_REG_ROBOT_LED_CHANGED: { // PWM 0-1023
            int32_t pwm = value;
            if (pwm < 0) pwm = 0;
            if (pwm > 1023) pwm = 1023;
            EnterCriticalSection(&s_link.lock);
            s_link.led_pwm = pwm;
            LeaveCriticalSection(&s_link.lock);
            if (s_led_slider) lv_slider_set_value(s_led_slider, pwm, LV_ANIM_OFF);
            snprintf(buf, sizeof(buf), "%d", (int)pwm);
            if (s_led_val_lbl) lv_label_set_text(s_led_val_lbl, buf);
            sim_log("HMI: LED -> %d", (int)pwm);
            break;
        }
        case HMI_REG_ANGLE_HEAD_CHANGED: // servo1 (cabeza)
            EnterCriticalSection(&s_link.lock);
            s_link.servo[0] = value;
            LeaveCriticalSection(&s_link.lock);
            if (s_servo_slider[0]) lv_slider_set_value(s_servo_slider[0], value, LV_ANIM_OFF);
            snprintf(buf, sizeof(buf), "%d", (int)value);
            if (s_servo_val_lbl[0]) lv_label_set_text(s_servo_val_lbl[0], buf);
            sim_log("HMI: Servo1 (cabeza) -> %d", (int)value);
            break;
        case HMI_REG_ANGLE_NECK_CHANGED: // servo2 (cuello)
            EnterCriticalSection(&s_link.lock);
            s_link.servo[1] = value;
            LeaveCriticalSection(&s_link.lock);
            if (s_servo_slider[1]) lv_slider_set_value(s_servo_slider[1], value, LV_ANIM_OFF);
            snprintf(buf, sizeof(buf), "%d", (int)value);
            if (s_servo_val_lbl[1]) lv_label_set_text(s_servo_val_lbl[1], buf);
            sim_log("HMI: Servo2 (cuello) -> %d", (int)value);
            break;
        case HMI_REG_CENTER: { // bits[31:16]=cuello(servo2), bits[15:0]=cabeza(servo1)
            int32_t neck = (value >> 16) & 0xFFFF;
            int32_t head = value & 0xFFFF;
            EnterCriticalSection(&s_link.lock);
            s_link.servo[0] = head;
            s_link.servo[1] = neck;
            LeaveCriticalSection(&s_link.lock);
            if (s_servo_slider[0]) lv_slider_set_value(s_servo_slider[0], head, LV_ANIM_OFF);
            if (s_servo_slider[1]) lv_slider_set_value(s_servo_slider[1], neck, LV_ANIM_OFF);
            snprintf(buf, sizeof(buf), "%d", (int)head); if (s_servo_val_lbl[0]) lv_label_set_text(s_servo_val_lbl[0], buf);
            snprintf(buf, sizeof(buf), "%d", (int)neck); if (s_servo_val_lbl[1]) lv_label_set_text(s_servo_val_lbl[1], buf);
            sim_log("HMI: Centrar -> cabeza=%d cuello=%d", (int)head, (int)neck);
            break;
        }
        case HMI_REG_MOTOR: { // bits[31:16]=cmd(0-4), bits[15:0]=vel(0-1000)
            int32_t cmd = (value >> 16) & 0xFFFF;
            int32_t vel = value & 0xFFFF;
            EnterCriticalSection(&s_link.lock);
            s_link.motor_cmd = cmd;
            s_link.motor_vel = vel;
            LeaveCriticalSection(&s_link.lock);
            static const char *dir_names[5] = { "STOP", "FORWARD", "REVERSE", "RIGHT", "LEFT" };
            if (cmd >= 0 && cmd <= 4) set_motor(cmd, dir_names[cmd], SIM_ACCENT);
            sim_log("HMI: Motor cmd=%d vel=%d", (int)cmd, (int)vel);
            break;
        }
        default:
            // Resto de registros (BLE, wifi, idioma, limites de servo, OTA,
            // ping/pong con la consola, etc.) no tienen equivalente en el
            // robot — no se reenvian.
            break;
    }
}

// ===================== Construccion de los paneles =====================
// Con el HMI centrado en la MISMA ventana (800x480, sin tocar, corrido a
// x=SIM_HMI_X), este archivo arma las dos columnas a los lados: izquierda
// (conexion + motor + sensores + extras del simulador) y derecha (servos).
// Viven en lv_layer_top() (por eso siguen visibles sin importar que screen
// del HMI este activo), en las posiciones/anchos que fija sim_layout.h.

// Estilo "tira de acento a la izquierda" (como las notificaciones/paneles de
// VS Code) en vez de un recuadro entero de color: fondo plano, sin borde
// completo, radio chico, solo una franja de 3px del lado izquierdo con el
// color de acento, y el titulo en texto neutro (no pintado).
static lv_obj_t *card(lv_obj_t *parent, const char *title)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(box, lv_color_hex(SIM_PANEL), 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(SIM_BORDER), 0);
    lv_obj_set_style_radius(box, 3, 0);
    lv_obj_set_style_pad_all(box, 8, 0);
    lv_obj_set_style_pad_row(box, 6, 0);
    lv_obj_set_style_layout(box, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(box, LV_FLEX_FLOW_COLUMN, 0);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(box);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(SIM_TEXT), 0);
    lv_label_set_text(lbl, title);
    return box;
}

static lv_obj_t *labeled_slider(lv_obj_t *parent, const char *name, int32_t min, int32_t max,
                                 int32_t start, lv_event_cb_t cb, void *user_data, lv_obj_t **val_lbl_out)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_layout(row, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(row);
    lv_obj_set_flex_grow(lbl, 1);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(SIM_TEXT_DIM), 0);
    lv_label_set_text(lbl, name);

    lv_obj_t *val = lv_label_create(row);
    lv_obj_set_style_text_font(val, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(val, lv_color_hex(SIM_ACCENT), 0);
    char buf[8]; snprintf(buf, sizeof(buf), "%d", (int)start);
    lv_label_set_text(val, buf);
    if (val_lbl_out) *val_lbl_out = val;

    lv_obj_t *sl = lv_slider_create(parent);
    lv_obj_set_size(sl, LV_PCT(100), 14);
    lv_slider_set_range(sl, min, max);
    lv_slider_set_value(sl, start, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sl, lv_color_hex(SIM_ACCENT), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(sl, lv_color_hex(SIM_ACCENT), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(sl, cb, LV_EVENT_VALUE_CHANGED, user_data);

    return sl;
}

// Contenedor comun a ambas columnas — viven en lv_layer_top() (encima del
// HMI, que ocupa el centro de la MISMA ventana, ver stubs/sim_layout.h).
// Quedan SCROLLABLE por si la ventana esta en su tamaño normal (480px) y el
// contenido no entra — un contenedor flex desbordado sin scroll cuelga el
// programa, bug real que ya se encontro y arreglo antes en este mismo
// simulador. Al maximizar, sim_side_panels_update_layout() les da mas alto
// (el mismo que la ventana) para que dejen de necesitar scroll.
static lv_obj_t *s_motor_panel = NULL;
static lv_obj_t *s_servo_panel = NULL;

static lv_obj_t *side_panel_create(int32_t x, int32_t w, lv_border_side_t border_side)
{
    lv_obj_t *panel = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(panel, x, 0);
    lv_obj_set_size(panel, w, SIM_WINDOW_H);
    lv_obj_set_style_bg_color(panel, lv_color_hex(SIM_BG), 0);
    lv_obj_set_style_bg_opa(panel, 255, 0);
    // Divisor neutro y fino entre columnas (como entre grupos de editor en
    // VS Code) — la identidad de cada columna la da la barra de acento
    // debajo del titulo (panel_title_create), no un marco entero de color.
    lv_obj_set_style_border_side(panel, border_side, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(SIM_BORDER), 0);
    lv_obj_set_style_radius(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 10, 0);
    lv_obj_set_style_pad_row(panel, 8, 0);
    lv_obj_set_style_layout(panel, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(panel, LV_FLEX_FLOW_COLUMN, 0);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(panel, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_AUTO);
    return panel;
}

// Titulo de columna estilo VS Code: texto neutro + una barrita de acento
// debajo (como el subrayado de la pestaña activa), en vez de texto pintado
// del color entero de la columna.
static void panel_title_create(lv_obj_t *panel, const char *text, uint32_t accent)
{
    lv_obj_t *title = lv_label_create(panel);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(SIM_TEXT), 0);
    lv_label_set_text(title, text);

    lv_obj_t *underline = lv_obj_create(panel);
    lv_obj_set_size(underline, 36, 3);
    lv_obj_set_style_bg_color(underline, lv_color_hex(accent), 0);
    lv_obj_set_style_bg_opa(underline, 255, 0);
    lv_obj_set_style_border_width(underline, 0, 0);
    lv_obj_set_style_radius(underline, 0, 0);
    lv_obj_remove_flag(underline, LV_OBJ_FLAG_SCROLLABLE);
}

// Columna DERECHA (junto al HMI): joystick de SERVOS (flechas del teclado)
// + sliders de Servo1/2/3 (sincronizados con el joystick) + LED PWM.
void sim_servo_panel_create(void)
{
    lv_obj_t *panel = side_panel_create(SIM_SERVO_COL_X, SIM_SERVO_COL_W, LV_BORDER_SIDE_LEFT);
    s_servo_panel = panel;

    panel_title_create(panel, "SERVOS (RS485)", SIM_ACCENT);

    joystick_create(panel, "Flechas: Cabeza (arriba/abajo) / Cuello (izq/der)", SIM_ACCENT, &s_servo_joy);

    lv_obj_t *servos = card(panel, "SERVOS (0-180)");
    s_servo_slider[0] = labeled_slider(servos, "Servo 1 (cabeza)", 0, 180, 90, servo_slider_cb, (void *)(intptr_t)0, &s_servo_val_lbl[0]);
    s_servo_slider[1] = labeled_slider(servos, "Servo 2 (cuello)", 0, 180, 90, servo_slider_cb, (void *)(intptr_t)1, &s_servo_val_lbl[1]);
    // Servo 3 arranca en 0 (s_link.servo[2] ya arranca en 0; el HMI no lo mueve).
    s_servo_slider[2] = labeled_slider(servos, "Servo 3 (manual)", 0, 180, 0, servo_slider_cb, (void *)(intptr_t)2, &s_servo_val_lbl[2]);

    lv_obj_t *led = card(panel, "LED PWM (0-1023)");
    s_led_slider = labeled_slider(led, "Brillo", 0, 1023, 0, led_slider_cb, NULL, &s_led_val_lbl);

    // Los controles "solo simulador" van aca (no en Motor) para repartir el
    // contenido entre las dos columnas — Motor ya tiene Conexion+Motor+
    // Sensores, que sin esto la dejaba mucho mas alta que Servos.
    sim_extra_controls_create(panel);
}

// Columna IZQUIERDA (junto al HMI): conexion RS485 + joystick de MOTOR
// (WASD) + sensores + controles "solo simulador" (Simular datos, Capturar,
// Modelo, Ping/Pong).
void sim_motor_panel_create(void)
{
    InitializeCriticalSection(&s_link.lock);
    // Mismo arranque que muestran los sliders (y que angle_head/neck_value del
    // HMI): cabeza/cuello en 90, servo 3 en 0. Si quedaban en 0 (static), la
    // primera flecha partia de 0 y los servos saltaban de golpe de 90 a 0.
    s_link.servo[0] = 90;
    s_link.servo[1] = 90;
    s_link.servo[2] = 0;
    strncpy(s_link.port, "COM12", sizeof(s_link.port) - 1);
    s_link.baud = 115200;
    s_link.slave_addr = 10;
    strncpy(s_link.status_msg, "Sin conexion", sizeof(s_link.status_msg) - 1);
    s_link.thread = CreateThread(NULL, 0, robot_link_thread, &s_link, 0, NULL);

    lv_obj_t *panel = side_panel_create(SIM_MOTOR_COL_X, SIM_MOTOR_COL_W, LV_BORDER_SIDE_RIGHT);
    s_motor_panel = panel;

    panel_title_create(panel, "MOTOR (RS485)", SIM_ACCENT);

    // ---- Conexion ----
    lv_obj_t *conn = card(panel, "CONEXION");

    lv_obj_t *row1 = lv_obj_create(conn);
    lv_obj_set_size(row1, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row1, 0, 0);
    lv_obj_set_style_border_width(row1, 0, 0);
    lv_obj_set_style_pad_all(row1, 0, 0);
    lv_obj_set_style_pad_column(row1, 4, 0);
    lv_obj_set_style_layout(row1, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(row1, LV_FLEX_FLOW_ROW, 0);
    lv_obj_remove_flag(row1, LV_OBJ_FLAG_SCROLLABLE);

    s_port_dd = lv_dropdown_create(row1);
    lv_obj_set_flex_grow(s_port_dd, 2);
    lv_dropdown_set_options(s_port_dd, "COM12");
    lv_obj_set_style_text_font(s_port_dd, &lv_font_montserrat_12, 0);
    style_dark_dropdown(s_port_dd);

    lv_obj_t *refresh_btn = lv_button_create(row1);
    lv_obj_set_size(refresh_btn, 28, 28);
    lv_obj_add_event_cb(refresh_btn, refresh_ports_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *refresh_lbl = lv_label_create(refresh_btn);
    lv_label_set_text(refresh_lbl, LV_SYMBOL_REFRESH);
    lv_obj_center(refresh_lbl);

    lv_obj_t *row2 = lv_obj_create(conn);
    lv_obj_set_size(row2, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row2, 0, 0);
    lv_obj_set_style_border_width(row2, 0, 0);
    lv_obj_set_style_pad_all(row2, 0, 0);
    lv_obj_set_style_pad_column(row2, 4, 0);
    lv_obj_set_style_layout(row2, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(row2, LV_FLEX_FLOW_ROW, 0);
    lv_obj_remove_flag(row2, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *addr_lbl = lv_label_create(row2);
    lv_obj_set_style_text_font(addr_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(addr_lbl, lv_color_hex(SIM_TEXT_DIM), 0);
    lv_label_set_text(addr_lbl, "Addr:");

    s_addr_ta = lv_textarea_create(row2);
    lv_obj_set_width(s_addr_ta, 40);
    lv_textarea_set_one_line(s_addr_ta, true);
    lv_textarea_set_text(s_addr_ta, "10");
    lv_obj_set_style_text_font(s_addr_ta, &lv_font_montserrat_12, 0);
    style_dark_textarea(s_addr_ta);

    s_baud_dd = lv_dropdown_create(row2);
    lv_obj_set_flex_grow(s_baud_dd, 1);
    lv_dropdown_set_options(s_baud_dd, "9600\n19200\n38400\n57600\n115200");
    lv_dropdown_set_selected(s_baud_dd, 4); // 115200 default
    lv_obj_set_style_text_font(s_baud_dd, &lv_font_montserrat_12, 0);
    style_dark_dropdown(s_baud_dd);

    s_connect_btn = lv_button_create(conn);
    lv_obj_set_size(s_connect_btn, LV_PCT(100), 36);
    lv_obj_set_style_radius(s_connect_btn, 4, 0);
    lv_obj_set_style_shadow_opa(s_connect_btn, 0, 0);
    lv_obj_set_style_bg_color(s_connect_btn, lv_color_hex(SIM_PRIMARY), 0);
    lv_obj_add_event_cb(s_connect_btn, connect_btn_cb, LV_EVENT_CLICKED, NULL);
    s_connect_lbl = lv_label_create(s_connect_btn);
    lv_obj_set_style_text_font(s_connect_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(s_connect_lbl);
    lv_label_set_text(s_connect_lbl, "CONECTAR");

    s_status_lbl = lv_label_create(conn);
    lv_obj_set_width(s_status_lbl, LV_PCT(100));
    lv_obj_set_style_text_font(s_status_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_status_lbl, lv_color_hex(SIM_TEXT_DIM), 0);
    lv_label_set_long_mode(s_status_lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(s_status_lbl, "Sin conexion");

    s_devname_lbl = lv_label_create(conn);
    lv_obj_set_width(s_devname_lbl, LV_PCT(100));
    lv_obj_set_style_text_font(s_devname_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_devname_lbl, lv_color_hex(SIM_ACCENT), 0);
    lv_label_set_text(s_devname_lbl, "");

    refresh_ports_cb(NULL);

    joystick_create(panel, "WASD: FWD/REV/LEFT/RIGHT (vel. del slider)", SIM_ACCENT, &s_motor_joy);

    s_pad_status_lbl = lv_label_create(panel);
    lv_obj_set_width(s_pad_status_lbl, LV_PCT(100));
    lv_obj_set_style_text_align(s_pad_status_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(s_pad_status_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_pad_status_lbl, lv_color_hex(SIM_TEXT_DIM), 0);
    lv_label_set_long_mode(s_pad_status_lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(s_pad_status_lbl, "Control fisico: no detectado");

    // ---- Motor ----
    lv_obj_t *motor = card(panel, "MOTOR");

    s_vel_slider = labeled_slider(motor, "Velocidad (0-1000)", 0, 1000, 500, vel_slider_cb, NULL, &s_vel_val_lbl);

    s_dir_lbl = lv_label_create(motor);
    lv_obj_set_width(s_dir_lbl, LV_PCT(100));
    lv_obj_set_style_text_align(s_dir_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(s_dir_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_dir_lbl, lv_color_hex(SIM_ACCENT), 0);
    lv_label_set_text(s_dir_lbl, "STOP");

    // D-pad: fila FWD, fila LEFT/STOP/RIGHT, fila REV
    lv_obj_t *fwd_row = lv_obj_create(motor);
    lv_obj_set_size(fwd_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(fwd_row, 0, 0);
    lv_obj_set_style_border_width(fwd_row, 0, 0);
    lv_obj_set_style_pad_all(fwd_row, 0, 0);
    lv_obj_set_style_layout(fwd_row, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(fwd_row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_main_place(fwd_row, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_remove_flag(fwd_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *fwd_btn = lv_button_create(fwd_row);
    lv_obj_set_size(fwd_btn, 90, 36);
    lv_obj_set_style_bg_color(fwd_btn, lv_color_hex(SIM_BTN), 0);
    lv_obj_add_event_cb(fwd_btn, motor_fwd_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *fwd_lbl = lv_label_create(fwd_btn); lv_obj_center(fwd_lbl);
    lv_obj_set_style_text_font(fwd_lbl, &lv_font_montserrat_12, 0);
    lv_label_set_text(fwd_lbl, LV_SYMBOL_UP " FWD");

    lv_obj_t *mid_row = lv_obj_create(motor);
    lv_obj_set_size(mid_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(mid_row, 0, 0);
    lv_obj_set_style_border_width(mid_row, 0, 0);
    lv_obj_set_style_pad_all(mid_row, 0, 0);
    lv_obj_set_style_pad_column(mid_row, 4, 0);
    lv_obj_set_style_layout(mid_row, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(mid_row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_main_place(mid_row, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_remove_flag(mid_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *left_btn = lv_button_create(mid_row);
    lv_obj_set_size(left_btn, 80, 36);
    lv_obj_set_style_bg_color(left_btn, lv_color_hex(SIM_BTN), 0);
    lv_obj_add_event_cb(left_btn, motor_left_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *left_lbl = lv_label_create(left_btn); lv_obj_center(left_lbl);
    lv_obj_set_style_text_font(left_lbl, &lv_font_montserrat_12, 0);
    lv_label_set_text(left_lbl, LV_SYMBOL_LEFT " LEFT");

    lv_obj_t *stop_btn = lv_button_create(mid_row);
    lv_obj_set_size(stop_btn, 70, 36);
    lv_obj_set_style_bg_color(stop_btn, lv_color_hex(SIM_PRIMARY), 0);
    lv_obj_add_event_cb(stop_btn, motor_stop_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *stop_lbl = lv_label_create(stop_btn); lv_obj_center(stop_lbl);
    lv_obj_set_style_text_font(stop_lbl, &lv_font_montserrat_12, 0);
    lv_label_set_text(stop_lbl, LV_SYMBOL_STOP " STOP");

    lv_obj_t *right_btn = lv_button_create(mid_row);
    lv_obj_set_size(right_btn, 80, 36);
    lv_obj_set_style_bg_color(right_btn, lv_color_hex(SIM_BTN), 0);
    lv_obj_add_event_cb(right_btn, motor_right_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *right_lbl = lv_label_create(right_btn); lv_obj_center(right_lbl);
    lv_obj_set_style_text_font(right_lbl, &lv_font_montserrat_12, 0);
    lv_label_set_text(right_lbl, "RIGHT " LV_SYMBOL_RIGHT);

    lv_obj_t *rev_row = lv_obj_create(motor);
    lv_obj_set_size(rev_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(rev_row, 0, 0);
    lv_obj_set_style_border_width(rev_row, 0, 0);
    lv_obj_set_style_pad_all(rev_row, 0, 0);
    lv_obj_set_style_layout(rev_row, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(rev_row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_main_place(rev_row, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_remove_flag(rev_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *rev_btn = lv_button_create(rev_row);
    lv_obj_set_size(rev_btn, 90, 36);
    lv_obj_set_style_bg_color(rev_btn, lv_color_hex(SIM_BTN), 0);
    lv_obj_add_event_cb(rev_btn, motor_rev_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *rev_lbl = lv_label_create(rev_btn); lv_obj_center(rev_lbl);
    lv_obj_set_style_text_font(rev_lbl, &lv_font_montserrat_12, 0);
    lv_label_set_text(rev_lbl, LV_SYMBOL_DOWN " REV");

    // ---- Sensores ----
    lv_obj_t *sensors = card(panel, "SENSORES");
    const char *snames[6] = { "Pitch (deg)", "Roll (deg)", "Temp (C)", "Volt (V)", "Curr (A)", "Pot (W)" };
    for (int i = 0; i < 6; i++) {
        lv_obj_t *row = lv_obj_create(sensors);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, 0, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_style_layout(row, LV_LAYOUT_FLEX, 0);
        lv_obj_set_style_flex_flow(row, LV_FLEX_FLOW_ROW, 0);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *nlbl = lv_label_create(row);
        lv_obj_set_flex_grow(nlbl, 1);
        lv_obj_set_style_text_font(nlbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(nlbl, lv_color_hex(SIM_TEXT_DIM), 0);
        lv_label_set_text(nlbl, snames[i]);

        lv_obj_t *vlbl = lv_label_create(row);
        lv_obj_set_style_text_font(vlbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(vlbl, lv_color_hex(SIM_ACCENT), 0);
        lv_label_set_text(vlbl, "--");
        s_sensor_lbl[i] = vlbl;
    }

    lv_obj_t *btn_row = lv_obj_create(sensors);
    lv_obj_set_size(btn_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(btn_row, 0, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_set_style_pad_column(btn_row, 8, 0);
    lv_obj_set_style_layout(btn_row, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(btn_row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_cross_place(btn_row, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_remove_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *read_btn = lv_button_create(btn_row);
    lv_obj_set_size(read_btn, 70, 30);
    lv_obj_add_event_cb(read_btn, read_once_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *read_lbl = lv_label_create(read_btn); lv_obj_center(read_lbl);
    lv_obj_set_style_text_font(read_lbl, &lv_font_montserrat_12, 0);
    lv_label_set_text(read_lbl, "Leer");

    lv_obj_t *auto_lbl = lv_label_create(btn_row);
    lv_obj_set_style_text_font(auto_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(auto_lbl, lv_color_hex(SIM_TEXT_DIM), 0);
    lv_label_set_text(auto_lbl, "Auto 1s");

    s_auto_sw = lv_switch_create(btn_row);
    lv_obj_set_style_bg_color(s_auto_sw, lv_color_hex(SIM_BTN), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_auto_sw, lv_color_hex(SIM_ACCENT), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(s_auto_sw, auto_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_timer_create(robot_link_sync_timer_cb, 200, NULL);
    lv_timer_create(kb_poll_timer_cb, 30, NULL);
    // sim_extra_controls_create() va en la columna de SERVOS (ver
    // sim_servo_panel_create), para repartir mejor el alto entre columnas.
}

// Llamar en cada tick (ver sim_main.c): si la ventana esta mas alta que los
// 480px normales, las columnas MOTOR/SERVOS crecen para ocupar todo el alto
// disponible — con mas espacio, su contenido entra entero y dejan de
// necesitar scroll (en vez de quedar fijas en 480px con una franja negra
// debajo, que era como se veia antes de este ajuste).
// x actual del HMI (cambia si la ventana es mas ancha que SIM_WINDOW_W: la
// columna MOTOR crece y empuja el HMI/columna SERVOS hacia la derecha).
static int32_t s_hmi_x = SIM_HMI_X;

int32_t sim_get_hmi_x(void)
{
    return s_hmi_x;
}

void sim_side_panels_update_layout(void)
{
    lv_display_t *disp = lv_display_get_default();
    int32_t hor_res = lv_display_get_horizontal_resolution(disp);
    int32_t ver_res = lv_display_get_vertical_resolution(disp);

    // Si la ventana es mas ancha que el layout normal (1440px), reparte el
    // sobrante entre MOTOR y SERVOS en vez de dejarlo en negro a la derecha.
    int32_t extra_w = hor_res - SIM_WINDOW_W;
    if (extra_w < 0) extra_w = 0;
    int32_t motor_w = SIM_MOTOR_COL_W + extra_w / 2;
    int32_t servo_w = SIM_SERVO_COL_W + (extra_w - extra_w / 2);
    s_hmi_x = motor_w;

    if (s_motor_panel) {
        lv_obj_set_size(s_motor_panel, motor_w, ver_res);
    }
    if (s_servo_panel) {
        lv_obj_set_pos(s_servo_panel, s_hmi_x + SIM_HMI_W, 0);
        lv_obj_set_size(s_servo_panel, servo_w, ver_res);
    }
}
