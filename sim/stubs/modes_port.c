/*
 * Port de MODOS (Control por Puntos / Config Auto Rotation / editor de
 * recorridos / Iniciar-Pausar-Detener Giro Automatico) para el simulador de
 * PC. Copia fiel de la logica real de main/main.c (modes_show_view,
 * auto_rotation_*, ar_*, cp_*, autorot_*) — no una reinvencion simplificada.
 * Las unicas diferencias son:
 *
 *   - No hay NVS real: los recorridos guardados (g_recorridos) y el centro
 *     (g_center_head/neck) arrancan siempre en sus defaults de fabrica en
 *     vez de leerse de flash, y "guardar" no persiste entre corridas del
 *     simulador (dura mientras el programa esta abierto, como cualquier
 *     variable en RAM).
 *   - Los limites de servo (g_srv_limits) quedan fijos en 0-270 (el default)
 *     porque Settings > Limites de Servo no esta portado — no hace falta
 *     para que MODOS navegue y funcione.
 *
 * IMPORTANTE: si el main.c real cambia esta logica, hay que reflejar el
 * cambio aca a mano tambien — no se comparte codigo automaticamente.
 */

#include "lvgl.h"
#include "main.h"
#include "lang.h"
#include "vars.h"
#include "screens.h"
#include "actions.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <SDL2/SDL.h> // SDL_GetMouseState: 2do dedo simulado con clic derecho (multitouch)

// ===================== Datos de recorridos (sin NVS) =====================
#define RECORRIDO_MAX_POINTS 10
#define RECORRIDO_COUNT      2

typedef struct {
    int16_t head;
    int16_t neck;
} recorrido_point_t;

typedef struct {
    uint8_t            count;
    recorrido_point_t  points[RECORRIDO_MAX_POINTS];
    uint16_t           speed;
} recorrido_t;

typedef struct {
    int16_t min;
    int16_t max;
} servo_limit_t;

typedef struct {
    servo_limit_t srv1, srv2, srv3;
} servo_limits_t;

static recorrido_t     g_recorridos[RECORRIDO_COUNT];
static servo_limits_t  g_srv_limits;
static int32_t         g_center_head = 90, g_center_neck = 90;
static uint8_t         g_editing_recorrido = 0;
static int             g_editor_selected_point = -1;
static int             g_selected_recorrido = -1;
static int             g_ar_playing_point_idx = -1;

typedef enum {
    MODES_VIEW_HOME = 0,
    MODES_VIEW_CONTROL_PUNTOS,
    MODES_VIEW_AUTOROT_PICKER,
    MODES_VIEW_AUTOROT_EDITOR,
    MODES_VIEW_CONTROL_DIGITAL,
} modes_view_t;

typedef enum {
    AUTO_PLAY_IDLE = 0,
    AUTO_PLAY_ONE_SHOT_MOVE,
    AUTO_PLAY_MOVING_TO_START,
    AUTO_PLAY_PLAYING,
    AUTO_PLAY_PAUSED,
    AUTO_PLAY_RETURNING_CENTER,
} auto_play_state_t;

static auto_play_state_t g_play_state      = AUTO_PLAY_IDLE;
static auto_play_state_t g_pre_pause_state = AUTO_PLAY_IDLE;
static uint8_t   g_play_recorrido_idx = 0;
static uint8_t   g_play_point_idx     = 0;
static int32_t   g_move_target_head = 90, g_move_target_neck = 90;
static int32_t   g_move_step_deg    = 2;
static lv_timer_t *g_auto_rotation_timer = NULL;
static bool      s_ar_test_running = false;

static bool demo_is_running(void);              // Demo de MODES, ver hmi_demo_start()
static void demo_set_running_style(bool running);
static void auto_rotation_editor_refresh_list(void);
static void auto_rotation_editor_refresh_row_colors(void);
static void ar_test_set_running(bool running);

// ===================== Navegacion entre las 4 vistas de MODES =====================
static void modes_show_view(modes_view_t view)
{
    if (objects.modes_grid_panel) {
        if (view == MODES_VIEW_HOME) lv_obj_remove_flag(objects.modes_grid_panel, LV_OBJ_FLAG_HIDDEN);
        else                          lv_obj_add_flag(objects.modes_grid_panel, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_t *sub_objs[4] = { objects.modes_control_puntos_panel, objects.modes_autorot_picker_panel,
                              objects.modes_autorot_editor_panel, objects.modes_control_digital_panel };
    for (int i = 0; i < 4; i++) {
        if (!sub_objs[i]) continue;
        bool show = (view == (modes_view_t)(i + 1));
        if (show) lv_obj_remove_flag(sub_objs[i], LV_OBJ_FLAG_HIDDEN);
        else      lv_obj_add_flag(sub_objs[i], LV_OBJ_FLAG_HIDDEN);
    }
}

static void auto_rotation_sync_labels(int32_t head, int32_t neck)
{
    char buff[10];
    lv_snprintf(buff, sizeof(buff), "%03d", (int)head);
    if (objects.angle_head_label) lv_label_set_text(objects.angle_head_label, buff);
    lv_snprintf(buff, sizeof(buff), "%03d", (int)neck);
    if (objects.angle_neck_label) lv_label_set_text(objects.angle_neck_label, buff);

    char buff_deg[12];
    lv_snprintf(buff_deg, sizeof(buff_deg), "%03d\xC2\xB0", (int)head);
    if (objects.ar_angle_head_label) lv_label_set_text(objects.ar_angle_head_label, buff_deg);
    lv_snprintf(buff_deg, sizeof(buff_deg), "%03d\xC2\xB0", (int)neck);
    if (objects.ar_angle_neck_label) lv_label_set_text(objects.ar_angle_neck_label, buff_deg);
    // Control Digital: mismos angulos, con el caption traducido.
    char cd_buf[40];
    lv_snprintf(cd_buf, sizeof(cd_buf), "%s: %03d\xC2\xB0", g_lang->cd_lbl_head, (int)head);
    if (objects.cd_head_label) lv_label_set_text(objects.cd_head_label, cd_buf);
    lv_snprintf(cd_buf, sizeof(cd_buf), "%s: %03d\xC2\xB0", g_lang->cd_lbl_neck, (int)neck);
    if (objects.cd_neck_label) lv_label_set_text(objects.cd_neck_label, cd_buf);
}

static void modes_giro_automatico_set_state(const char *txt, bool active)
{
    if (objects.obj23) lv_label_set_text(objects.obj23, txt);
    hmi_style_btn(objects.btn_giro_automatico, active);
}

void hmi_modes_giro_retheme(void)
{
    bool active = (g_play_state == AUTO_PLAY_MOVING_TO_START ||
                   g_play_state == AUTO_PLAY_PLAYING ||
                   g_play_state == AUTO_PLAY_PAUSED);
    hmi_style_btn(objects.btn_giro_automatico, active);
    if (demo_is_running()) demo_set_running_style(true);
}

static void angle_axis_bump(lv_obj_t *label, bool is_head, int32_t delta)
{
    servo_limit_t lim = is_head ? g_srv_limits.srv1 : g_srv_limits.srv2;
    int32_t value = is_head ? get_var_angle_head_value() : get_var_angle_neck_value();
    value += delta;
    if (value < lim.min) value = lim.min;
    if (value > lim.max) value = lim.max;
    if (is_head) set_var_angle_head_value(value); else set_var_angle_neck_value(value);
    if (label) {
        char buff[12];
        lv_snprintf(buff, sizeof(buff), "%03d\xC2\xB0", (int)value);
        lv_label_set_text(label, buff);
    }
    hmi_send_data(is_head ? HMI_REG_ANGLE_HEAD_CHANGED : HMI_REG_ANGLE_NECK_CHANGED, value);
}

// ===================== Motor de reproduccion (timer 100ms) =====================
static void auto_rotation_queue_next_point(void)
{
    recorrido_t *r = &g_recorridos[g_play_recorrido_idx];
    uint8_t next = g_play_point_idx + 1;
    if (next >= r->count) {
        g_play_state = AUTO_PLAY_RETURNING_CENTER;
        g_move_target_head = g_center_head;
        g_move_target_neck = g_center_neck;
        g_move_step_deg = 3;
        lv_timer_set_period(g_auto_rotation_timer, 100);
        if (g_play_recorrido_idx == g_editing_recorrido) {
            g_ar_playing_point_idx = -1;
            auto_rotation_editor_refresh_row_colors();
        }
        return;
    }
    g_play_point_idx = next;
    g_move_target_head = r->points[next].head;
    g_move_target_neck = r->points[next].neck;
    g_move_step_deg = 1;
    lv_timer_set_period(g_auto_rotation_timer, r->speed);
    if (g_play_recorrido_idx == g_editing_recorrido) {
        g_ar_playing_point_idx = (int)next;
        auto_rotation_editor_refresh_row_colors();
    }
}

static void auto_rotation_on_target_reached(void)
{
    switch (g_play_state) {
    case AUTO_PLAY_ONE_SHOT_MOVE:
        g_play_state = AUTO_PLAY_IDLE;
        if (g_auto_rotation_timer) lv_timer_pause(g_auto_rotation_timer);
        ar_test_set_running(false);
        break;
    case AUTO_PLAY_MOVING_TO_START:
        g_play_state = AUTO_PLAY_PLAYING;
        g_play_point_idx = 0;
        auto_rotation_queue_next_point();
        break;
    case AUTO_PLAY_PLAYING:
        auto_rotation_queue_next_point();
        break;
    case AUTO_PLAY_RETURNING_CENTER:
        g_play_state = AUTO_PLAY_IDLE;
        if (g_auto_rotation_timer) lv_timer_pause(g_auto_rotation_timer);
        modes_giro_automatico_set_state(g_lang->btn_auto_rotation, false);
        ar_test_set_running(false);
        break;
    default:
        break;
    }
}

static void auto_rotation_timer_cb(lv_timer_t *t)
{
    (void)t;
    int32_t head = get_var_angle_head_value();
    int32_t neck = get_var_angle_neck_value();
    bool head_done = (head == g_move_target_head);
    bool neck_done = (neck == g_move_target_neck);
    if (!head_done) {
        int32_t d = g_move_target_head - head;
        int32_t step = (d > 0) ? (d < g_move_step_deg ? d : g_move_step_deg)
                                : (d > -g_move_step_deg ? d : -g_move_step_deg);
        head += step;
        set_var_angle_head_value(head);
        hmi_send_data(HMI_REG_ANGLE_HEAD_CHANGED, head);
        head_done = (head == g_move_target_head);
    }
    if (!neck_done) {
        int32_t d = g_move_target_neck - neck;
        int32_t step = (d > 0) ? (d < g_move_step_deg ? d : g_move_step_deg)
                                : (d > -g_move_step_deg ? d : -g_move_step_deg);
        neck += step;
        set_var_angle_neck_value(neck);
        hmi_send_data(HMI_REG_ANGLE_NECK_CHANGED, neck);
        neck_done = (neck == g_move_target_neck);
    }
    auto_rotation_sync_labels(head, neck);
    if (head_done && neck_done) auto_rotation_on_target_reached();
}

static void auto_rotation_ensure_timer(void)
{
    if (!g_auto_rotation_timer) g_auto_rotation_timer = lv_timer_create(auto_rotation_timer_cb, 100, NULL);
}

static void auto_rotation_start_one_shot_move(int32_t target_head, int32_t target_neck)
{
    if (demo_is_running()) return; // la Demo esta moviendo cabeza/cuello
    auto_rotation_ensure_timer();
    g_play_state = AUTO_PLAY_ONE_SHOT_MOVE;
    g_move_target_head = target_head;
    g_move_target_neck = target_neck;
    g_move_step_deg = 2;
    lv_timer_set_period(g_auto_rotation_timer, 100);
    lv_timer_resume(g_auto_rotation_timer);
}

static void hmi_auto_rotation_play_start(void)
{
    if (demo_is_running()) return; // la Demo esta moviendo cabeza/cuello
    if (g_selected_recorrido < 0) return;
    recorrido_t *r = &g_recorridos[g_selected_recorrido];
    if (r->count == 0) return;
    auto_rotation_ensure_timer();
    g_play_recorrido_idx = (uint8_t)g_selected_recorrido;
    g_play_state = AUTO_PLAY_MOVING_TO_START;
    g_move_target_head = r->points[0].head;
    g_move_target_neck = r->points[0].neck;
    g_play_point_idx = 0;
    g_move_step_deg = 1;
    lv_timer_set_period(g_auto_rotation_timer, r->speed);
    lv_timer_resume(g_auto_rotation_timer);
    modes_giro_automatico_set_state(g_lang->btn_pausar_auto_rotation, true);
    if (g_play_recorrido_idx == g_editing_recorrido) {
        g_ar_playing_point_idx = 0;
        auto_rotation_editor_refresh_row_colors();
    }
    ar_test_set_running(true);
}

static void auto_rotation_stop_and_return_center(void)
{
    if (g_play_state == AUTO_PLAY_IDLE || g_play_state == AUTO_PLAY_ONE_SHOT_MOVE) return;
    auto_rotation_ensure_timer();
    g_play_state = AUTO_PLAY_RETURNING_CENTER;
    g_move_target_head = g_center_head;
    g_move_target_neck = g_center_neck;
    g_move_step_deg = 3;
    lv_timer_set_period(g_auto_rotation_timer, 100);
    lv_timer_resume(g_auto_rotation_timer);
    if (g_play_recorrido_idx == g_editing_recorrido) {
        g_ar_playing_point_idx = -1;
        auto_rotation_editor_refresh_row_colors();
    }
    modes_giro_automatico_set_state(g_lang->btn_auto_rotation, false);
    ar_test_set_running(false);
}

// ---- MODES: Demo (INICIAR DEMO / DETENER DEMO) ----
// La secuencia la arma el propio HMI (antes solo mandaba HMI_REG_START_DEMO y
// la consola hacia su demo): cabeza y cuello a 90, LED sube 0->100% y baja
// a 0, cabeza 90->0->180->90, despues cuello 90->0->180->90. Cada
// paso interpola con ease-in-out
// (arranca y frena suave) y solo manda el registro cuando el valor entero
// cambia. Los angulos se recortan a los Limites de Servo. DETENER DEMO corta
// donde este (no vuelve al centro).
typedef enum { DEMO_LED = 0, DEMO_HEAD, DEMO_NECK } demo_axis_t;
typedef struct {
    demo_axis_t axis;
    int32_t     target;      // % para el LED, grados para cabeza/cuello
    uint16_t    ms_per_unit; // duracion = |distancia| * ms_per_unit
} demo_step_t;

static const demo_step_t k_demo_steps[] = {
    { DEMO_HEAD,  90, 10 }, // posicion inicial: cabeza a 90 desde donde este
    { DEMO_NECK,  90, 10 }, // y cuello a 90
    { DEMO_LED,    0,  3 }, // LED a 0 rapido, para arrancar siempre desde apagado
    { DEMO_LED,  100, 15 }, // LED se prende de a poco (0->100% en 1.5 s)
    { DEMO_LED,    0, 15 }, // y baja la intensidad
    { DEMO_HEAD,   0, 17 }, // cabeza 90->0
    { DEMO_HEAD, 180, 17 }, // cabeza 0->180 (~3 s)
    { DEMO_HEAD,  90, 17 }, // cabeza vuelve a 90 antes de mover el cuello
    { DEMO_NECK,   0, 17 }, // cuello 90->0
    { DEMO_NECK, 180, 17 }, // cuello 0->180
    { DEMO_NECK,  90, 17 }, // cuello vuelve a 90
};
#define DEMO_STEP_COUNT ((int)(sizeof(k_demo_steps) / sizeof(k_demo_steps[0])))
#define DEMO_TICK_MS    20

static lv_timer_t *s_demo_timer = NULL;
static int      s_demo_step = -1; // -1 = la demo no esta corriendo
static uint32_t s_demo_t0, s_demo_dur;
static int32_t  s_demo_from, s_demo_to, s_demo_last;

static bool demo_is_running(void) { return s_demo_step >= 0; }

static int32_t demo_current(demo_axis_t axis)
{
    switch (axis) {
    case DEMO_LED:  return objects.obj16 ? lv_slider_get_value(objects.obj16) : 0;
    case DEMO_HEAD: return get_var_angle_head_value();
    default:        return get_var_angle_neck_value();
    }
}

static void demo_apply(demo_axis_t axis, int32_t v)
{
    if (axis == DEMO_LED) {
        if (objects.obj16) lv_slider_set_value(objects.obj16, v, LV_ANIM_OFF);
        char buff[10];
        lv_snprintf(buff, sizeof(buff), "%d %%", (int)v);
        set_var_brightness(buff);
        if (objects.brightness_label) lv_label_set_text_static(objects.brightness_label, get_var_brightness());
        hmi_bigview_led_level_refresh();
        hmi_send_data(HMI_REG_ROBOT_LED_CHANGED, (v * 1023 + 50) / 100);
        return;
    }
    if (axis == DEMO_HEAD) {
        set_var_angle_head_value(v);
        hmi_send_data(HMI_REG_ANGLE_HEAD_CHANGED, v);
    } else {
        set_var_angle_neck_value(v);
        hmi_send_data(HMI_REG_ANGLE_NECK_CHANGED, v);
    }
    auto_rotation_sync_labels(get_var_angle_head_value(), get_var_angle_neck_value());
}

static void demo_set_running_style(bool running)
{
    lv_obj_t *btn = objects.btn_start_demo;
    hmi_style_btn(btn, running);
    if (running || !btn) return;
    // hmi_style_btn(false) lo deja gris "inactivo"; este boton en reposo es
    // una tarjeta de la grilla de MODES (fondo/borde de tarjeta, texto accent).
    lv_obj_set_style_bg_color(btn, hmi_theme_bg_card(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn, hmi_theme_bd_card(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    if (lbl) {
        lv_obj_set_style_text_color(lbl, hmi_theme_accent(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lbl, hmi_theme_accent(), LV_PART_MAIN | LV_STATE_PRESSED);
    }
}

static void demo_finish(void)
{
    s_demo_step = -1;
    if (s_demo_timer) lv_timer_pause(s_demo_timer);
    demo_set_running_style(false);
}

static void demo_begin_step(int idx)
{
    if (idx >= DEMO_STEP_COUNT) { demo_finish(); return; }
    const demo_step_t *s = &k_demo_steps[idx];
    int32_t to = s->target;
    if (s->axis != DEMO_LED) {
        servo_limit_t lim = (s->axis == DEMO_HEAD) ? g_srv_limits.srv1 : g_srv_limits.srv2;
        if (to < lim.min) to = lim.min;
        if (to > lim.max) to = lim.max;
    }
    s_demo_step = idx;
    s_demo_from = s_demo_last = demo_current(s->axis);
    s_demo_to   = to;
    int32_t dist = (to > s_demo_from) ? to - s_demo_from : s_demo_from - to;
    s_demo_dur  = (uint32_t)dist * s->ms_per_unit;
    s_demo_t0   = lv_tick_get();
}

static void demo_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (!demo_is_running()) return;
    demo_axis_t axis = k_demo_steps[s_demo_step].axis;
    uint32_t el = lv_tick_elaps(s_demo_t0);
    int32_t v = s_demo_to;
    if (el < s_demo_dur) {
        float p = (float)el / (float)s_demo_dur;
        float e = p * p * (3.0f - 2.0f * p); // smoothstep: arranca y frena suave
        v = s_demo_from + (int32_t)lroundf((float)(s_demo_to - s_demo_from) * e);
    }
    if (v != s_demo_last) {
        demo_apply(axis, v);
        s_demo_last = v;
    }
    if (el >= s_demo_dur) demo_begin_step(s_demo_step + 1);
}

void hmi_demo_start(void)
{
    // No pisar un Giro Automatico / Test / movimiento al centro en curso.
    if (demo_is_running() || g_play_state != AUTO_PLAY_IDLE) return;
    if (!s_demo_timer) s_demo_timer = lv_timer_create(demo_timer_cb, DEMO_TICK_MS, NULL);
    demo_begin_step(0);
    lv_timer_resume(s_demo_timer);
    demo_set_running_style(true);
}

void hmi_demo_stop(void)
{
    if (demo_is_running()) demo_finish();
}

// ===================== MODES home =====================
void modes_btn_control_por_puntos_cb(lv_event_t *e)
{
    (void)e;
    modes_show_view(MODES_VIEW_CONTROL_PUNTOS);
    auto_rotation_sync_labels(get_var_angle_head_value(), get_var_angle_neck_value());
    if (g_play_state == AUTO_PLAY_IDLE) auto_rotation_start_one_shot_move(g_center_head, g_center_neck);
}

void modes_btn_config_auto_rotation_cb(lv_event_t *e)
{
    (void)e;
    modes_show_view(MODES_VIEW_AUTOROT_PICKER);
}

void modes_btn_giro_automatico_cb(lv_event_t *e)
{
    (void)e;
    switch (g_play_state) {
    case AUTO_PLAY_IDLE:
        hmi_auto_rotation_play_start();
        break;
    case AUTO_PLAY_MOVING_TO_START:
    case AUTO_PLAY_PLAYING:
        g_pre_pause_state = g_play_state;
        g_play_state = AUTO_PLAY_PAUSED;
        if (g_auto_rotation_timer) lv_timer_pause(g_auto_rotation_timer);
        modes_giro_automatico_set_state(g_lang->btn_reanudar_auto_rotation, true);
        break;
    case AUTO_PLAY_PAUSED:
        g_play_state = g_pre_pause_state;
        if (g_auto_rotation_timer) lv_timer_resume(g_auto_rotation_timer);
        modes_giro_automatico_set_state(g_lang->btn_pausar_auto_rotation, true);
        break;
    default:
        break;
    }
}

void modes_btn_stop_giro_automatico_cb(lv_event_t *e) { (void)e; auto_rotation_stop_and_return_center(); }

// ===================== Control por Puntos =====================
void cp_btn_guardar_centrado_cb(lv_event_t *e)
{
    (void)e;
    int32_t head = get_var_angle_head_value();
    int32_t neck = get_var_angle_neck_value();
    int32_t packed = (neck << 16) | (head & 0xFFFF);
    hmi_send_data(HMI_REG_CENTER, packed);
    g_center_head = head;
    g_center_neck = neck;
}

void cp_btn_volver_cb(lv_event_t *e) { (void)e; modes_show_view(MODES_VIEW_HOME); }

// ---- Multitouch SIMULADO (en el HMI lo lee main.c del GT911) ----
// Windows junta todos los mouses en un solo cursor, asi que el 2do dedo se
// simula con el clic DERECHO:
//   clic IZQUIERDO = dedo 1 (el mouse normal, se arrastra);
//   clic DERECHO   = apoya un dedo 2 FIJO en ese lugar (circulo celeste) y
//                    otro clic derecho lo levanta.
// Asi se prueban los dos joysticks de Control Digital a la vez. Ademas se
// imita al GT911: al levantar un dedo, UNA lectura sale sin dedos (lo que en
// el HMI hacia que el otro joystick saltara al centro un instante).
static bool       s_sim_pin_on = false;
static lv_point_t s_sim_pin;
static lv_obj_t  *s_sim_pin_mark = NULL;
static bool       s_sim_rbtn_prev = false;
static uint8_t    s_sim_prev_n = 0;

static lv_indev_t *sim_mouse_indev(void)
{
    for (lv_indev_t *i = lv_indev_get_next(NULL); i; i = lv_indev_get_next(i))
        if (lv_indev_get_type(i) == LV_INDEV_TYPE_POINTER) return i;
    return NULL;
}

static void sim_pin_mark_update(void)
{
    if (!s_sim_pin_mark) {
        s_sim_pin_mark = lv_obj_create(lv_layer_top());
        lv_obj_set_size(s_sim_pin_mark, 34, 34);
        lv_obj_set_style_radius(s_sim_pin_mark, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(s_sim_pin_mark, lv_color_hex(0x29B6F6), 0);
        lv_obj_set_style_bg_opa(s_sim_pin_mark, LV_OPA_60, 0);
        lv_obj_set_style_border_color(s_sim_pin_mark, lv_color_white(), 0);
        lv_obj_set_style_border_width(s_sim_pin_mark, 2, 0);
        lv_obj_remove_flag(s_sim_pin_mark, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    }
    if (s_sim_pin_on) {
        lv_obj_set_pos(s_sim_pin_mark, s_sim_pin.x - 17, s_sim_pin.y - 17);
        lv_obj_remove_flag(s_sim_pin_mark, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_sim_pin_mark, LV_OBJ_FLAG_HIDDEN);
    }
}

bool hmi_touch_multi_supported(void) { return true; }

uint8_t hmi_touch_get_points(lv_point_t *pts, uint8_t max)
{
    if (!pts || max == 0) return 0;
    int mx, my;
    uint32_t b = SDL_GetMouseState(&mx, &my);
    bool r = (b & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    if (r && !s_sim_rbtn_prev) { // flanco: apoya / levanta el dedo 2
        s_sim_pin_on = !s_sim_pin_on;
        s_sim_pin.x = mx;
        s_sim_pin.y = my;
        sim_pin_mark_update();
    }
    s_sim_rbtn_prev = r;

    uint8_t n = 0;
    lv_indev_t *m = sim_mouse_indev();
    if (m && lv_indev_get_state(m) == LV_INDEV_STATE_PRESSED) lv_indev_get_point(m, &pts[n++]);
    if (s_sim_pin_on && n < max) pts[n++] = s_sim_pin;

    uint8_t real = n;
    if (n < s_sim_prev_n) n = 0; // imita al GT911: 1 lectura vacia al levantar un dedo
    s_sim_prev_n = real;
    return n;
}

// ---- MODES: Control Digital (joystick tactil de cabeza/cuello) ----
// Joystick de servos a UNA sola velocidad (CD_SERVO_DEG_PER_S, la misma que
// el joystick fisico de la consola: 1 grado cada 50 ms): empujar hacia un
// lado mueve el servo siempre igual de rapido, sin importar cuanto. Cada eje
// necesita un empuje claro (CD_SERVO_AXIS_MIN del radio) para que un desvio
// chico del dedo no mueva el otro servo. Arriba/abajo = cabeza, izq/der =
// cuello (mismo mapeo que las flechas del simulador). Al soltar, la perilla
// vuelve al centro y el servo queda donde llego. Respeta los Limites de
// Servo (y 0-180) y no hace nada mientras corren la Demo, el Giro
// Automatico o un movimiento al centro, para no pisarse.
#define CD_TICK_MS         30
#define CD_SERVO_DEG_PER_S 20.0f  // velocidad unica de cabeza/cuello
#define CD_SERVO_AXIS_MIN  0.35f  // fraccion del radio para que un eje se mueva
#define CD_DEADZONE        0.12f  // zona muerta del joystick del motor

static lv_timer_t *s_cd_timer = NULL;
static bool  s_cd_active = false;
static float s_cd_x = 0.0f, s_cd_y = 0.0f; // -1..1, y positivo = arriba
static float s_cd_head_f = 90.0f, s_cd_neck_f = 90.0f;

static float cd_axis_speed(float v)
{
    if (fabsf(v) < CD_SERVO_AXIS_MIN) return 0.0f;
    return (v < 0.0f) ? -CD_SERVO_DEG_PER_S : CD_SERVO_DEG_PER_S;
}

static float cd_clamp(float v, servo_limit_t lim)
{
    float lo = (lim.min > 0)   ? (float)lim.min : 0.0f;
    float hi = (lim.max < 180) ? (float)lim.max : 180.0f;
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    return v;
}

static void cd_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_cd_active) return;
    const float dt = CD_TICK_MS / 1000.0f;
    s_cd_head_f = cd_clamp(s_cd_head_f + cd_axis_speed(s_cd_y) * dt, g_srv_limits.srv1);
    s_cd_neck_f = cd_clamp(s_cd_neck_f + cd_axis_speed(s_cd_x) * dt, g_srv_limits.srv2);
    int32_t head = (int32_t)lroundf(s_cd_head_f);
    int32_t neck = (int32_t)lroundf(s_cd_neck_f);
    bool changed = false;
    if (head != get_var_angle_head_value()) {
        set_var_angle_head_value(head);
        hmi_send_data(HMI_REG_ANGLE_HEAD_CHANGED, head);
        changed = true;
    }
    if (neck != get_var_angle_neck_value()) {
        set_var_angle_neck_value(neck);
        hmi_send_data(HMI_REG_ANGLE_NECK_CHANGED, neck);
        changed = true;
    }
    if (changed) auto_rotation_sync_labels(head, neck);
}

static void cd_release(void)
{
    s_cd_active = false;
    s_cd_x = s_cd_y = 0.0f;
    if (s_cd_timer) lv_timer_pause(s_cd_timer);
    if (objects.cd_joy_knob) lv_obj_align(objects.cd_joy_knob, LV_ALIGN_CENTER, 0, 0);
}

// Mueve la perilla bajo el dedo (recortada al circulo) y devuelve la
// posicion normalizada -1..1 (y positivo = arriba). Sirve para los dos
// joysticks (motor y cabeza/cuello).
static void cd_track_at(lv_obj_t *base, lv_obj_t *knob, lv_point_t p, float *out_x, float *out_y)
{
    *out_x = *out_y = 0.0f;
    if (!base || !knob) return;
    lv_area_t a;
    lv_obj_get_coords(base, &a);
    float dx = (float)(p.x - (a.x1 + a.x2) / 2);
    float dy = (float)(p.y - (a.y1 + a.y2) / 2);
    float r = (float)(lv_obj_get_width(base) - lv_obj_get_width(knob)) / 2.0f
              - (float)lv_obj_get_style_border_width(base, LV_PART_MAIN);
    if (r < 1.0f) r = 1.0f;
    float len = sqrtf(dx * dx + dy * dy);
    if (len > r) { dx = dx * r / len; dy = dy * r / len; }
    lv_obj_align(knob, LV_ALIGN_CENTER, (int32_t)dx, (int32_t)dy);
    *out_x = dx / r;
    *out_y = -dy / r;
}

// Igual que cd_track_at() pero con el punto del puntero de LVGL (un dedo).
static void cd_track(lv_obj_t *base, lv_obj_t *knob, float *out_x, float *out_y)
{
    *out_x = *out_y = 0.0f;
    lv_indev_t *indev = lv_indev_active();
    if (!indev) return;
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    cd_track_at(base, knob, p, out_x, out_y);
}

static void cd_track_point(lv_obj_t *base) { cd_track(base, objects.cd_joy_knob, &s_cd_x, &s_cd_y); }

void cd_joystick_event_cb(lv_event_t *e)
{
    // Con multitouch los dos joysticks los maneja cd_mt_timer_cb().
    if (hmi_touch_multi_supported()) return;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *base = lv_event_get_target(e);
    switch (code) {
    case LV_EVENT_PRESSED:
        if (demo_is_running() || g_play_state != AUTO_PLAY_IDLE) return;
        s_cd_head_f = (float)get_var_angle_head_value();
        s_cd_neck_f = (float)get_var_angle_neck_value();
        s_cd_active = true;
        if (!s_cd_timer) s_cd_timer = lv_timer_create(cd_timer_cb, CD_TICK_MS, NULL);
        lv_timer_resume(s_cd_timer);
        cd_track_point(base);
        break;
    case LV_EVENT_PRESSING:
        if (s_cd_active) cd_track_point(base);
        break;
    case LV_EVENT_RELEASED:
    case LV_EVENT_PRESS_LOST:
        cd_release();
        break;
    default:
        break;
    }
}

// Joystick MOTOR: mismo criterio que el joystick 1 de la consola — el eje
// dominante decide la direccion (adelante/atras/derecha/izquierda) y la
// distancia al centro la velocidad (0-1000). OJO: en la consola, un
// HMI_REG_MOTOR con cmd != STOP le quita el control al joystick fisico hasta
// recibir STOP, y NO tiene tiempo limite. Por eso STOP se manda siempre al
// soltar, al perder el toque y al salir de la pantalla, y mientras se
// mantiene apretado el comando se reenvia cada CD_MOTOR_RESEND_MS (sirve si
// mas adelante la consola agrega un tiempo limite).
enum { CD_MOTOR_STOP = 0, CD_MOTOR_FORWARD, CD_MOTOR_REVERSE, CD_MOTOR_RIGHT, CD_MOTOR_LEFT };
#define CD_MOTOR_RESEND_MS 200
#define CD_MOTOR_VEL_STEP  50 // velocidad en pasos de 50: no mandar un mensaje por cada pixel

static lv_timer_t *s_cd_motor_timer = NULL;
static bool    s_cd_motor_engaged = false; // el dedo esta en el joystick del motor
static int32_t s_cd_motor_cmd = CD_MOTOR_STOP, s_cd_motor_vel = 0;

void hmi_control_digital_motor_label_refresh(void)
{
    if (!objects.cd_motor_label) return;
    int c = (s_cd_motor_cmd >= CD_MOTOR_STOP && s_cd_motor_cmd <= CD_MOTOR_LEFT) ? s_cd_motor_cmd : CD_MOTOR_STOP;
    char buf[48];
    if (c == CD_MOTOR_STOP)
        lv_snprintf(buf, sizeof(buf), "%s: %s", g_lang->cd_caption_motor, g_lang->cd_motor_dir[0]);
    else
        lv_snprintf(buf, sizeof(buf), "%s: %s %d%%", g_lang->cd_caption_motor, g_lang->cd_motor_dir[c], (int)(s_cd_motor_vel / 10));
    lv_label_set_text(objects.cd_motor_label, buf);
}

static void cd_motor_send(void)
{
    hmi_send_data(HMI_REG_MOTOR, (s_cd_motor_cmd << 16) | (s_cd_motor_vel & 0xFFFF));
}

static void cd_motor_set(int32_t cmd, int32_t vel)
{
    if (cmd == s_cd_motor_cmd && vel == s_cd_motor_vel) return;
    s_cd_motor_cmd = cmd;
    s_cd_motor_vel = vel;
    cd_motor_send();
    hmi_control_digital_motor_label_refresh();
}

static void cd_motor_resend_cb(lv_timer_t *t)
{
    (void)t;
    if (s_cd_motor_engaged && s_cd_motor_cmd != CD_MOTOR_STOP) cd_motor_send();
}

static void cd_motor_from_xy(float x, float y)
{
    float ax = fabsf(x), ay = fabsf(y);
    float a = (ay >= ax) ? ay : ax;
    if (a < CD_DEADZONE) { cd_motor_set(CD_MOTOR_STOP, 0); return; }
    float frac = (a - CD_DEADZONE) / (1.0f - CD_DEADZONE);
    int32_t vel = (int32_t)lroundf(frac * 1000.0f / CD_MOTOR_VEL_STEP) * CD_MOTOR_VEL_STEP;
    if (vel < CD_MOTOR_VEL_STEP) vel = CD_MOTOR_VEL_STEP;
    if (vel > 1000) vel = 1000;
    int32_t cmd = (ay >= ax) ? (y > 0.0f ? CD_MOTOR_FORWARD : CD_MOTOR_REVERSE)
                             : (x > 0.0f ? CD_MOTOR_RIGHT   : CD_MOTOR_LEFT);
    cd_motor_set(cmd, vel);
}

// Suelta el joystick del motor: perilla al centro y STOP (siempre que se
// haya tocado, aunque el ultimo comando ya fuera STOP — mas vale de mas).
static void cd_motor_release(void)
{
    if (s_cd_motor_timer) lv_timer_pause(s_cd_motor_timer);
    if (objects.cd_motor_knob) lv_obj_align(objects.cd_motor_knob, LV_ALIGN_CENTER, 0, 0);
    if (!s_cd_motor_engaged) return;
    s_cd_motor_engaged = false;
    s_cd_motor_cmd = CD_MOTOR_STOP;
    s_cd_motor_vel = 0;
    cd_motor_send();
    hmi_control_digital_motor_label_refresh();
}

void cd_motor_event_cb(lv_event_t *e)
{
    if (hmi_touch_multi_supported()) return; // ver cd_mt_timer_cb()
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *base = lv_event_get_target(e);
    float x, y;
    switch (code) {
    case LV_EVENT_PRESSED:
        s_cd_motor_engaged = true;
        if (!s_cd_motor_timer) s_cd_motor_timer = lv_timer_create(cd_motor_resend_cb, CD_MOTOR_RESEND_MS, NULL);
        lv_timer_resume(s_cd_motor_timer);
        cd_track(base, objects.cd_motor_knob, &x, &y);
        cd_motor_from_xy(x, y);
        break;
    case LV_EVENT_PRESSING:
        if (!s_cd_motor_engaged) break;
        cd_track(base, objects.cd_motor_knob, &x, &y);
        cd_motor_from_xy(x, y);
        break;
    case LV_EVENT_RELEASED:
    case LV_EVENT_PRESS_LOST:
        cd_motor_release();
        break;
    default:
        break;
    }
}

// ---- Control Digital con MULTITOUCH (los dos joysticks a la vez) ----
// Cada CD_TICK_MS se leen todos los dedos (hmi_touch_get_points) y a cada
// joystick se le asigna el suyo: si ya tenia uno, el dedo mas cercano a su
// ultima posicion (mientras no "salte" mas de CD_MT_MAX_JUMP px); si no, un
// dedo libre que haya caido dentro de su circulo. Un mismo dedo nunca maneja
// los dos. Si el dedo se va, ese joystick se suelta (el motor manda STOP).
// Si la pantalla deja de verse (cambio de pestana) se suelta todo; el timer
// sigue corriendo para que responda al volver. Volver lo pausa.
#define CD_MT_MAX_PTS       5
#define CD_MT_GRAB_MARGIN   10  // px fuera del circulo que todavia toman el dedo
#define CD_MT_MAX_JUMP      150 // px por lectura; mas que eso = otro dedo
// Al levantar UN dedo, el GT911 puede mandar alguna lectura sin dedos (o con
// el que queda corrido): si eso soltara al instante, el OTRO joystick
// volveria al centro un instante (y el motor recibiria STOP + arranque). Por
// eso un joystick recien se suelta cuando su dedo falta CD_MT_RELEASE_TICKS
// lecturas seguidas (~90 ms); mientras tanto se queda donde estaba.
#define CD_MT_RELEASE_TICKS 3

typedef struct { bool on; lv_point_t p; uint8_t miss; } cd_finger_t;
static lv_timer_t *s_cd_mt_timer = NULL;
static cd_finger_t s_cd_mt_motor = { false, { 0, 0 }, 0 };
static cd_finger_t s_cd_mt_servo = { false, { 0, 0 }, 0 };

static bool cd_point_in_base(lv_obj_t *base, lv_point_t p, int32_t margin)
{
    lv_area_t a;
    lv_obj_get_coords(base, &a);
    int32_t dx = p.x - (a.x1 + a.x2) / 2;
    int32_t dy = p.y - (a.y1 + a.y2) / 2;
    int32_t r  = lv_obj_get_width(base) / 2 + margin;
    return dx * dx + dy * dy <= r * r;
}

static bool cd_mt_pick(cd_finger_t *f, lv_obj_t *base, const lv_point_t *pts, uint8_t n, bool *used)
{
    int best = -1;
    int32_t best_d = 0;
    for (uint8_t i = 0; i < n; i++) {
        if (used[i]) continue;
        if (f->on) {
            int32_t dx = pts[i].x - f->p.x, dy = pts[i].y - f->p.y;
            int32_t d = dx * dx + dy * dy;
            if (d <= CD_MT_MAX_JUMP * CD_MT_MAX_JUMP && (best < 0 || d < best_d)) { best = i; best_d = d; }
        } else if (base && cd_point_in_base(base, pts[i], CD_MT_GRAB_MARGIN)) {
            best = i;
            break;
        }
    }
    if (best < 0) return false;
    used[best] = true;
    f->p = pts[best];
    return true;
}

// Joystick que YA tenia dedo: si esta lectura no lo trae, se lo sigue dando
// por apoyado (en su ultima posicion) hasta CD_MT_RELEASE_TICKS faltas.
static bool cd_mt_keep(cd_finger_t *f, bool found)
{
    if (found) { f->miss = 0; return true; }
    if (++f->miss < CD_MT_RELEASE_TICKS) return true;
    f->miss = 0;
    return false;
}

static void cd_mt_release_all(void)
{
    cd_release();
    cd_motor_release();
    s_cd_mt_motor.on = s_cd_mt_servo.on = false;
    s_cd_mt_motor.miss = s_cd_mt_servo.miss = 0;
}

static void cd_mt_timer_cb(lv_timer_t *t)
{
    // Pantalla oculta (otra pestana, otra vista de MODOS): suelta lo que haya
    // quedado apretado pero NO pausa el timer — si se vuelve por la pestana,
    // Control Digital sigue abierto y tiene que responder sin pasar por
    // Volver. Solo Volver lo pausa (y CONTROL DIGITAL lo reanuda).
    (void)t;
    if (!objects.modes_control_digital_panel || !lv_obj_is_visible(objects.modes_control_digital_panel)) {
        if (s_cd_mt_motor.on || s_cd_mt_servo.on || s_cd_active || s_cd_motor_engaged) cd_mt_release_all();
        return;
    }
    lv_point_t pts[CD_MT_MAX_PTS];
    bool used[CD_MT_MAX_PTS] = { false };
    uint8_t n = hmi_touch_get_points(pts, CD_MT_MAX_PTS);

    bool motor_was = s_cd_mt_motor.on, servo_was = s_cd_mt_servo.on;
    bool motor_on = false, servo_on = false;
    // Primero los joysticks que ya tienen dedo, para que uno libre no les robe el suyo.
    if (motor_was)  motor_on = cd_mt_keep(&s_cd_mt_motor, cd_mt_pick(&s_cd_mt_motor, objects.cd_motor_base, pts, n, used));
    if (servo_was)  servo_on = cd_mt_keep(&s_cd_mt_servo, cd_mt_pick(&s_cd_mt_servo, objects.cd_joy_base,   pts, n, used));
    if (!motor_was) motor_on = cd_mt_pick(&s_cd_mt_motor, objects.cd_motor_base, pts, n, used);
    if (!servo_was) servo_on = cd_mt_pick(&s_cd_mt_servo, objects.cd_joy_base,   pts, n, used);

    // Motor
    if (motor_on) {
        if (!motor_was) {
            s_cd_motor_engaged = true;
            if (!s_cd_motor_timer) s_cd_motor_timer = lv_timer_create(cd_motor_resend_cb, CD_MOTOR_RESEND_MS, NULL);
            lv_timer_resume(s_cd_motor_timer);
        }
        float x, y;
        cd_track_at(objects.cd_motor_base, objects.cd_motor_knob, s_cd_mt_motor.p, &x, &y);
        cd_motor_from_xy(x, y);
    } else if (motor_was) {
        cd_motor_release();
    }
    s_cd_mt_motor.on = motor_on;

    // Cabeza/cuello (no arranca mientras corren la Demo, el Giro Automatico
    // o un movimiento al centro; el dedo se vuelve a probar en la proxima lectura).
    if (servo_on && !servo_was) {
        if (demo_is_running() || g_play_state != AUTO_PLAY_IDLE) {
            servo_on = false;
        } else {
            s_cd_head_f = (float)get_var_angle_head_value();
            s_cd_neck_f = (float)get_var_angle_neck_value();
            s_cd_active = true;
            if (!s_cd_timer) s_cd_timer = lv_timer_create(cd_timer_cb, CD_TICK_MS, NULL);
            lv_timer_resume(s_cd_timer);
        }
    }
    if (servo_on) {
        if (s_cd_active) cd_track_at(objects.cd_joy_base, objects.cd_joy_knob, s_cd_mt_servo.p, &s_cd_x, &s_cd_y);
    } else if (servo_was) {
        cd_release();
    }
    s_cd_mt_servo.on = servo_on;
}

void modes_btn_control_digital_cb(lv_event_t *e)
{
    (void)e;
    modes_show_view(MODES_VIEW_CONTROL_DIGITAL);
    auto_rotation_sync_labels(get_var_angle_head_value(), get_var_angle_neck_value());
    hmi_control_digital_motor_label_refresh();
    if (hmi_touch_multi_supported()) {
        s_cd_mt_motor.on = s_cd_mt_servo.on = false;
        s_cd_mt_motor.miss = s_cd_mt_servo.miss = 0;
        if (!s_cd_mt_timer) s_cd_mt_timer = lv_timer_create(cd_mt_timer_cb, CD_TICK_MS, NULL);
        lv_timer_resume(s_cd_mt_timer);
    }
}

void cd_btn_volver_cb(lv_event_t *e)
{
    (void)e;
    cd_mt_release_all();
    if (s_cd_mt_timer) lv_timer_pause(s_cd_mt_timer);
    modes_show_view(MODES_VIEW_HOME);
}

// Centrar: cabeza/cuello al centro guardado (Control por Puntos > Guardar
// centrado, 90/90 por defecto), suave, con el mismo movimiento de un solo
// tiro que usa Control por Puntos al entrar. Mientras dura, el joystick no
// responde (g_play_state != IDLE), asi no se pisan.
void cd_btn_centrar_cb(lv_event_t *e)
{
    (void)e;
    cd_release();
    if (g_play_state == AUTO_PLAY_IDLE) auto_rotation_start_one_shot_move(g_center_head, g_center_neck);
}

// ===================== Config Auto Rotation — selector =====================
static void auto_rotation_editor_open(uint8_t idx)
{
    g_editing_recorrido = idx;
    g_selected_recorrido = idx;
    g_editor_selected_point = -1;
    bool playing_this = (g_play_recorrido_idx == idx) &&
                         (g_play_state == AUTO_PLAY_MOVING_TO_START ||
                          g_play_state == AUTO_PLAY_PLAYING ||
                          g_play_state == AUTO_PLAY_PAUSED);
    g_ar_playing_point_idx = playing_this ? (int)g_play_point_idx : -1;
    ar_test_set_running(playing_this);
    modes_show_view(MODES_VIEW_AUTOROT_EDITOR);
    if (objects.ar_title_label) {
        char title[24];
        lv_snprintf(title, sizeof(title), " RECORRIDO %d", (int)idx + 1);
        lv_label_set_text(objects.ar_title_label, title);
    }
    if (objects.ar_speed_label) {
        char sbuf[8];
        lv_snprintf(sbuf, sizeof(sbuf), "%d ms", (int)g_recorridos[idx].speed);
        lv_label_set_text(objects.ar_speed_label, sbuf);
    }
    auto_rotation_sync_labels(get_var_angle_head_value(), get_var_angle_neck_value());
    auto_rotation_editor_refresh_list();
    if (g_play_state == AUTO_PLAY_IDLE) auto_rotation_start_one_shot_move(g_center_head, g_center_neck);
}

void autorot_btn_recorrido1_cb(lv_event_t *e) { (void)e; auto_rotation_editor_open(0); }
void autorot_btn_recorrido2_cb(lv_event_t *e) { (void)e; auto_rotation_editor_open(1); }
void autorot_picker_btn_volver_cb(lv_event_t *e) { (void)e; modes_show_view(MODES_VIEW_HOME); }

// ===================== Editor de puntos =====================
void ar_angle_neck_dec_cb(lv_event_t *e) { (void)e; angle_axis_bump(objects.ar_angle_neck_label, false, -1); }
void ar_angle_neck_inc_cb(lv_event_t *e) { (void)e; angle_axis_bump(objects.ar_angle_neck_label, false,  1); }
void ar_angle_head_dec_cb(lv_event_t *e) { (void)e; angle_axis_bump(objects.ar_angle_head_label, true,  -1); }
void ar_angle_head_inc_cb(lv_event_t *e) { (void)e; angle_axis_bump(objects.ar_angle_head_label, true,   1); }

static void auto_rotation_editor_refresh_row_colors(void)
{
    if (!objects.ar_points_list) return;
    uint32_t n = lv_obj_get_child_count(objects.ar_points_list);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *row = lv_obj_get_child(objects.ar_points_list, i);
        if (!row) continue;
        if ((int)i == g_ar_playing_point_idx) {
            lv_obj_set_style_bg_color(row, lv_color_hex(0xff1e6b3a), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else if ((int)i == g_editor_selected_point) {
            lv_obj_set_style_bg_color(row, lv_color_hex(0xff4a3f0a), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            lv_obj_set_style_bg_color(row, hmi_theme_bg_indicator(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(row, hmi_theme_bd_card(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_t *lbl = lv_obj_get_child(row, lv_obj_get_child_count(row) - 1);
            if (lbl) lv_obj_set_style_text_color(lbl, hmi_theme_txt_primary(), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

static void ar_test_set_running(bool running)
{
    s_ar_test_running = running;
    if (!objects.ar_btn_probar) return;
    lv_obj_t *lbl = lv_obj_get_child(objects.ar_btn_probar, 0);
    if (lbl) lv_label_set_text(lbl, running ? g_lang->btn_detener_prueba : g_lang->btn_probar);
    lv_color_t border = running ? lv_color_hex(0xffbc0f2d) : hmi_theme_bd_card();
    lv_obj_set_style_border_color(objects.ar_btn_probar, border, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void hmi_autorot_editor_retheme(void)
{
    auto_rotation_editor_refresh_row_colors();
    ar_test_set_running(s_ar_test_running);
}

void ar_point_row_clicked_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    recorrido_t *r = &g_recorridos[g_editing_recorrido];
    if (idx < 0 || idx >= r->count) return;
    g_editor_selected_point = idx;
    recorrido_point_t p = r->points[idx];
    set_var_angle_head_value(p.head);
    set_var_angle_neck_value(p.neck);
    auto_rotation_sync_labels(p.head, p.neck);
    if (g_play_state == AUTO_PLAY_IDLE) auto_rotation_start_one_shot_move(p.head, p.neck);
    auto_rotation_editor_refresh_row_colors();
}

static void auto_rotation_editor_refresh_list(void)
{
    if (!objects.ar_points_list) return;
    lv_obj_clean(objects.ar_points_list);
    recorrido_t *r = &g_recorridos[g_editing_recorrido];
    for (uint8_t i = 0; i < r->count; i++) {
        char buf[24];
        lv_snprintf(buf, sizeof(buf), "%02d      H%d N%d", (int)i + 1, (int)r->points[i].head, (int)r->points[i].neck);
        lv_obj_t *btn = lv_list_add_btn(objects.ar_points_list, NULL, buf);
        lv_obj_add_event_cb(btn, ar_point_row_clicked_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_t *lbl = lv_obj_get_child(btn, lv_obj_get_child_count(btn) - 1);
        if (lbl) lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    }

    lv_obj_set_height(objects.ar_points_list, LV_SIZE_CONTENT);
    lv_obj_update_layout(objects.ar_points_list);
    lv_coord_t natural_h = lv_obj_get_height(objects.ar_points_list);
    const lv_coord_t LIST_MIN_H = 70;
    const lv_coord_t LIST_MAX_H = 210;
    if (natural_h < LIST_MIN_H) natural_h = LIST_MIN_H;
    if (natural_h > LIST_MAX_H) natural_h = LIST_MAX_H;
    lv_obj_set_height(objects.ar_points_list, natural_h);
    auto_rotation_editor_refresh_row_colors();
}

void ar_btn_guardar_punto_cb(lv_event_t *e)
{
    (void)e;
    recorrido_t *r = &g_recorridos[g_editing_recorrido];
    int32_t head = get_var_angle_head_value();
    int32_t neck = get_var_angle_neck_value();
    if (g_editor_selected_point >= 0 && g_editor_selected_point < r->count) {
        r->points[g_editor_selected_point].head = (int16_t)head;
        r->points[g_editor_selected_point].neck = (int16_t)neck;
    } else if (r->count < RECORRIDO_MAX_POINTS) {
        r->points[r->count].head = (int16_t)head;
        r->points[r->count].neck = (int16_t)neck;
        r->count++;
    } else {
        return;
    }
    g_editor_selected_point = -1;
    auto_rotation_editor_refresh_list();
}

void ar_btn_eliminar_punto_cb(lv_event_t *e)
{
    (void)e;
    recorrido_t *r = &g_recorridos[g_editing_recorrido];
    if (g_editor_selected_point < 0 || g_editor_selected_point >= r->count) return;
    for (int i = g_editor_selected_point; i < r->count - 1; i++) {
        r->points[i] = r->points[i + 1];
    }
    r->count--;
    g_editor_selected_point = -1;
    auto_rotation_editor_refresh_list();
}

void ar_speed_dec_cb(lv_event_t *e)
{
    (void)e;
    recorrido_t *r = &g_recorridos[g_editing_recorrido];
    if (r->speed > 5) r->speed -= 5;
    if (objects.ar_speed_label) { char buf[8]; lv_snprintf(buf, sizeof(buf), "%d ms", (int)r->speed); lv_label_set_text(objects.ar_speed_label, buf); }
}

void ar_speed_inc_cb(lv_event_t *e)
{
    (void)e;
    recorrido_t *r = &g_recorridos[g_editing_recorrido];
    if (r->speed < 500) r->speed += 5;
    if (objects.ar_speed_label) { char buf[8]; lv_snprintf(buf, sizeof(buf), "%d ms", (int)r->speed); lv_label_set_text(objects.ar_speed_label, buf); }
}

void ar_btn_probar_cb(lv_event_t *e)
{
    (void)e;
    if (g_play_state == AUTO_PLAY_IDLE) {
        g_selected_recorrido = g_editing_recorrido;
        hmi_auto_rotation_play_start();
    } else if (g_play_state == AUTO_PLAY_MOVING_TO_START ||
               g_play_state == AUTO_PLAY_PLAYING ||
               g_play_state == AUTO_PLAY_PAUSED) {
        auto_rotation_stop_and_return_center();
    }
}

void ar_btn_volver_cb(lv_event_t *e) { (void)e; modes_show_view(MODES_VIEW_AUTOROT_PICKER); }

// ===================== Init (llamada desde main.h ya declarada) =====================
void hmi_auto_rotation_init(void)
{
    for (int i = 0; i < RECORRIDO_COUNT; i++) {
        memset(&g_recorridos[i], 0, sizeof(recorrido_t));
        g_recorridos[i].speed = 150;
    }
    g_srv_limits.srv1.min = g_srv_limits.srv2.min = g_srv_limits.srv3.min = 0;
    g_srv_limits.srv1.max = g_srv_limits.srv2.max = g_srv_limits.srv3.max = 270;
    g_center_head = 90;
    g_center_neck = 90;
    modes_show_view(MODES_VIEW_HOME);
    modes_giro_automatico_set_state(g_lang->btn_auto_rotation, false);
}
