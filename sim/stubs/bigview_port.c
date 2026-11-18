/*
 * Port del "Panel de Manejo" (pantalla completa que abre el logo WELLTEP)
 * para el simulador de PC. Es una COPIA fiel de la logica real que vive en
 * main/main.c (funciones encoder_dashboard_refresh, hmi_encoder_set_raw,
 * hmi_encoder_reset, bigview_*, encoder_bigview_*, logo_*) — no una
 * reinvencion simplificada. Las unicas diferencias son:
 *
 *   - No lee/escribe NVS para la calibracion del encoder (perimetro/PPR):
 *     usa directo los valores default de fabrica (ENC_PERIM_CX100_DEF /
 *     ENC_PPR_DEF), porque en la PC no hay flash NVS real.
 *   - No hay datos de encoder/angulo en vivo llegando por UART (no hay
 *     consola real conectada), asi que el panel abre y se ve identico,
 *     pero la distancia se queda en 0.000m y el trazo queda plano hasta
 *     que algo llame hmi_encoder_set_raw() (podria conectarse a futuro a
 *     un generador de datos falsos para pruebas, si hace falta).
 *
 * IMPORTANTE: si el main.c real cambia esta logica, hay que reflejar el
 * cambio aca a mano tambien — no se comparte codigo automaticamente.
 */

#include "lvgl.h"
#include "main.h"
#include "lang.h"
#include "vars.h"
#include "screens.h"
#include "lock_logos.h"
#include "actions.h" // action_settings_btn_brightness() — usado en sim_apply_robot_model()
#include "drivers/sdl/lv_sdl_window.h"
#include <SDL2/SDL.h>
#include "sim_layout.h"
#include "sim_log.h"
#include "sim_theme.h" // OJO: solo se usa dentro de las secciones "TEMP" (solo simulador) mas abajo — el resto del archivo es un port fiel del HMI real y no debe tocarse.
#include "stb_image_write.h" // guarda las capturas en PNG (implementacion real en stb_image_write_impl.c)

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <direct.h> // _mkdir
#include <io.h> // _findfirst/_findnext — ver capture_count_init_from_disk()
#include <stdlib.h>

LV_FONT_DECLARE(lv_font_distance);

// Display de la (unica) ventana del simulador. Con una sola ventana esto
// coincide siempre con lv_display_get_default(), pero se guarda aparte para
// no depender de eso — sim_main.c la fija una vez con sim_set_main_display()
// apenas la crea.
static lv_display_t *s_main_disp = NULL;

void sim_set_main_display(lv_display_t *disp)
{
    s_main_disp = disp;
}

// ===================== Calibracion del encoder (sin NVS) =====================
#define ENC_PERIM_CX100_DEF 8520 // 85.20 mm
#define ENC_PPR_DEF          600

static bool     s_encoder_show_feet     = false;
static int32_t  s_encoder_last_raw      = 0;
static float    s_encoder_dist_m        = 0.0f;
static float    s_bigview_current_pitch = 0.0f;
static lv_obj_t *s_encoder_bigview_label      = NULL;
static lv_obj_t *s_encoder_bigview_unit_label = NULL;

static void encoder_dashboard_refresh(void)
{
    if (!objects.encoder_value) return;

    float perim_mm = ENC_PERIM_CX100_DEF / 100.0f;
    float dist_m  = ((float)s_encoder_last_raw * perim_mm) / ((float)ENC_PPR_DEF * 1000.0f);
    s_encoder_dist_m = dist_m;

    char buf[24];
    char num_buf[16];
    const char *unit;
    if (s_encoder_show_feet) {
        snprintf(buf, sizeof(buf), "%.2f ft", dist_m * 3.28084f);
        snprintf(num_buf, sizeof(num_buf), "%.2f", dist_m * 3.28084f);
        unit = "ft";
    } else {
        snprintf(buf, sizeof(buf), "%.3f m", dist_m);
        snprintf(num_buf, sizeof(num_buf), "%.3f", dist_m);
        unit = "m";
    }
    lv_label_set_text(objects.encoder_value, buf);
    if (s_encoder_bigview_label) lv_label_set_text(s_encoder_bigview_label, num_buf);
    if (s_encoder_bigview_unit_label) lv_label_set_text(s_encoder_bigview_unit_label, unit);
}

// ===================== Traza de inclinacion (pitch trace) =====================
#define BIGVIEW_TRACE_W          700
#define BIGVIEW_TRACE_H          120
#define BIGVIEW_TRACE_WINDOW_M   2.0f
#define BIGVIEW_TRACE_PX_PER_M   (BIGVIEW_TRACE_W / BIGVIEW_TRACE_WINDOW_M)
#define BIGVIEW_TRACE_GAIN       1.0f
#define BIGVIEW_TRACE_MAX_DEG    75.0f
#define BIGVIEW_TRACE_MAX_PTS    300

static lv_obj_t *s_bigview_angle_line   = NULL;
static lv_obj_t *s_bigview_angle_tip    = NULL;
static lv_obj_t *s_bigview_ruler_left   = NULL;
static lv_obj_t *s_bigview_ruler_mid    = NULL;
static lv_obj_t *s_bigview_ruler_right  = NULL;

static float s_trace_x[BIGVIEW_TRACE_MAX_PTS];
static float s_trace_y[BIGVIEW_TRACE_MAX_PTS];
static int   s_trace_len = 0;
static lv_point_precise_t s_angle_trace_pts[BIGVIEW_TRACE_MAX_PTS];

static float bigview_amplify_for_display(float angle_deg)
{
    float x = angle_deg * BIGVIEW_TRACE_GAIN / BIGVIEW_TRACE_MAX_DEG;
    return BIGVIEW_TRACE_MAX_DEG * tanhf(x);
}

static void bigview_angle_trace_reset(void)
{
    s_trace_len = 1;
    s_trace_x[0] = 0.0f;
    s_trace_y[0] = BIGVIEW_TRACE_H / 2.0f;
}

static void bigview_ruler_update(void)
{
    if (!s_bigview_ruler_right) return;
    float now_m = s_encoder_dist_m;
    float left_m = now_m - BIGVIEW_TRACE_WINDOW_M;
    float mid_m  = now_m - BIGVIEW_TRACE_WINDOW_M / 2.0f;
    if (left_m < 0) left_m = 0;
    if (mid_m  < 0) mid_m  = 0;

    char buf[16];
    snprintf(buf, sizeof(buf), "%.1fm", left_m);
    lv_label_set_text(s_bigview_ruler_left, buf);
    snprintf(buf, sizeof(buf), "%.1fm", mid_m);
    lv_label_set_text(s_bigview_ruler_mid, buf);
    snprintf(buf, sizeof(buf), "%.1fm", now_m);
    lv_label_set_text(s_bigview_ruler_right, buf);
}

static void bigview_angle_trace_redraw(void)
{
    if (!s_bigview_angle_line || s_trace_len == 0) return;

    float right_x   = s_trace_x[s_trace_len - 1];
    float left_edge = right_x - BIGVIEW_TRACE_W;
    if (left_edge < 0.0f) left_edge = 0.0f;

    int n = 0;
    for (int i = 0; i < s_trace_len && n < BIGVIEW_TRACE_MAX_PTS; i++) {
        float sx = s_trace_x[i] - left_edge;
        if (sx < 0.0f) sx = 0.0f;
        s_angle_trace_pts[n].x = (lv_value_precise_t)sx;
        s_angle_trace_pts[n].y = (lv_value_precise_t)s_trace_y[i];
        n++;
    }

    lv_line_set_points_mutable(s_bigview_angle_line, s_angle_trace_pts, n);
    if (s_bigview_angle_tip && n > 0) {
        lv_obj_set_pos(s_bigview_angle_tip,
            (int32_t)s_angle_trace_pts[n - 1].x - 9,
            (int32_t)s_angle_trace_pts[n - 1].y - 9);
    }
}

static void bigview_angle_trace_push(float angle_deg, float delta_dist_m)
{
    if (!s_bigview_angle_line || s_trace_len == 0 || delta_dist_m <= 0.0f) return;

    float step_px = delta_dist_m * BIGVIEW_TRACE_PX_PER_M;
    float vis = bigview_amplify_for_display(angle_deg);
    float rad = vis * ((float)M_PI / 180.0f);

    float last_x = s_trace_x[s_trace_len - 1];
    float last_y = s_trace_y[s_trace_len - 1];
    float new_x = last_x + step_px * cosf(rad);
    float new_y = last_y + step_px * sinf(rad);
    if (new_y < 4.0f) new_y = 4.0f;
    if (new_y > BIGVIEW_TRACE_H - 4.0f) new_y = BIGVIEW_TRACE_H - 4.0f;

    if (s_trace_len >= BIGVIEW_TRACE_MAX_PTS) {
        memmove(s_trace_x, s_trace_x + 1, (BIGVIEW_TRACE_MAX_PTS - 1) * sizeof(float));
        memmove(s_trace_y, s_trace_y + 1, (BIGVIEW_TRACE_MAX_PTS - 1) * sizeof(float));
        s_trace_len--;
    }
    s_trace_x[s_trace_len] = new_x;
    s_trace_y[s_trace_len] = new_y;
    s_trace_len++;

    float left_edge = new_x - BIGVIEW_TRACE_W;
    int drop = 0;
    while (drop < s_trace_len - 1 && s_trace_x[drop] < left_edge) drop++;
    if (drop > 0) {
        memmove(s_trace_x, s_trace_x + drop, (s_trace_len - drop) * sizeof(float));
        memmove(s_trace_y, s_trace_y + drop, (s_trace_len - drop) * sizeof(float));
        s_trace_len -= drop;
    }

    bigview_angle_trace_redraw();
    bigview_ruler_update();
}

static void bigview_reset_encoder_and_trace(void)
{
    hmi_encoder_reset();
    bigview_angle_trace_reset();
    bigview_angle_trace_redraw();
    bigview_ruler_update();
}

// ===================== Gestos de toque del panel =====================
static int       s_bigview_tap_count = 0;
static lv_timer_t *s_bigview_tap_timer = NULL;

static void bigview_tap_timeout_cb(lv_timer_t *t)
{
    (void)t;
    if (s_bigview_tap_count == 2) {
        bigview_reset_encoder_and_trace();
    }
    s_bigview_tap_count = 0;
    s_bigview_tap_timer = NULL;
}

static lv_obj_t *s_encoder_bigview_panel = NULL;

static void encoder_bigview_tap_cb(lv_event_t *e)
{
    (void)e;
    s_bigview_tap_count++;

    if (s_bigview_tap_count >= 3) {
        if (s_bigview_tap_timer) {
            lv_timer_delete(s_bigview_tap_timer);
            s_bigview_tap_timer = NULL;
        }
        s_bigview_tap_count = 0;
        if (s_encoder_bigview_panel) lv_obj_add_flag(s_encoder_bigview_panel, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (s_bigview_tap_timer) {
        lv_timer_reset(s_bigview_tap_timer);
    } else {
        s_bigview_tap_timer = lv_timer_create(bigview_tap_timeout_cb, 400, NULL);
        lv_timer_set_repeat_count(s_bigview_tap_timer, 1);
    }
}

// ===================== Bateria (celdas ROBOT/CONSOLA) =====================
static lv_obj_t *s_bigview_robot_batt_bar     = NULL;
static lv_obj_t *s_bigview_robot_batt_label   = NULL;
static lv_obj_t *s_bigview_console_batt_bar   = NULL;
static lv_obj_t *s_bigview_console_batt_label = NULL;
static lv_obj_t *s_bigview_robot_batt_caption   = NULL;
static lv_obj_t *s_bigview_console_batt_caption = NULL;
static lv_obj_t *s_bigview_distance_caption = NULL;
static lv_obj_t *s_bigview_trace_caption    = NULL;
static lv_obj_t *s_bigview_robot_model_label  = NULL;
static lv_obj_t *s_bigview_robot_model_label_bold = NULL; // copia +1px encima, ver "falso bold" mas abajo
static lv_obj_t *s_bigview_led_bt             = NULL;
static lv_obj_t *s_bigview_led_online         = NULL;
static lv_obj_t *s_bigview_led_level_seg[4] = { NULL, NULL, NULL, NULL };

void hmi_bigview_led_level_refresh(void)
{
    int pct = atoi(get_var_brightness());
    static const int thresholds[4] = { 25, 50, 75, 100 };
    for (int i = 0; i < 4; i++) {
        if (!s_bigview_led_level_seg[i]) continue;
        bool on = pct >= thresholds[i];
        lv_obj_set_style_bg_color(s_bigview_led_level_seg[i], lv_color_hex(0xfff5c518), 0);
        lv_obj_set_style_bg_opa(s_bigview_led_level_seg[i], on ? 255 : 60, 0);
    }
}

static lv_color_t battery_bar_color(uint8_t percent)
{
    if (percent >= 67) return lv_color_hex(0x27AE60);
    if (percent >= 50) return lv_color_hex(0xF5C518);
    if (percent >= 42) return lv_color_hex(0xE67E22);
    return lv_color_hex(0xE74C3C);
}

// Copia exacta de battery_percent() (main.c) — usada solo por la bateria
// del ROBOT (ver apply_robot_battery_mv() mas abajo) para calcular el mismo
// porcentaje que calcularia la placa real a partir del voltaje en mV.
static inline uint8_t battery_percent(uint16_t mv)
{
    if (mv <= 12790) return 0;
    if (mv >= 16720) return 100;
    return (uint8_t)(((mv - 12790) * 100) / (16720 - 12790));
}

static void battery_cell_create(lv_obj_t *parent, const char *caption,
                                 lv_obj_t **out_bar, lv_obj_t **out_label,
                                 lv_obj_t **out_caption)
{
    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_set_size(col, 72, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(col, 0, 0);
    lv_obj_set_style_border_width(col, 0, 0);
    lv_obj_set_style_pad_all(col, 0, 0);
    lv_obj_set_style_pad_row(col, 4, 0);
    lv_obj_set_style_layout(col, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(col, LV_FLEX_FLOW_COLUMN, 0);
    lv_obj_set_style_flex_cross_place(col, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_remove_flag(col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(col, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *cap = lv_label_create(col);
    lv_obj_set_style_text_font(cap, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(cap, lv_color_hex(0xffaaaaaa), 0);
    lv_label_set_text(cap, caption);

    lv_obj_t *bar = lv_bar_create(col);
    lv_obj_set_size(bar, 32, 64);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bar, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bar, 8, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xff27ae60), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);

    lv_obj_t *val = lv_label_create(col);
    lv_obj_set_style_text_font(val, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(val, lv_color_hex(0xffffffff), 0);
    lv_label_set_text(val, "--%");

    *out_bar     = bar;
    *out_label   = val;
    *out_caption = cap;
}

static void bigview_battery_refresh(void)
{
    if (s_bigview_robot_batt_bar) {
        uint8_t p = (uint8_t)atoi(get_var_robot_voltage_percent());
        lv_bar_set_value(s_bigview_robot_batt_bar, p, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_bigview_robot_batt_bar, battery_bar_color(p), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }
    if (s_bigview_robot_batt_label) {
        lv_label_set_text(s_bigview_robot_batt_label, get_var_robot_voltage_percent());
    }
    if (s_bigview_console_batt_bar) {
        uint8_t p = (uint8_t)atoi(get_var_console_voltage_percent());
        lv_bar_set_value(s_bigview_console_batt_bar, p, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_bigview_console_batt_bar, battery_bar_color(p), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }
    if (s_bigview_console_batt_label) {
        lv_label_set_text(s_bigview_console_batt_label, get_var_console_voltage_percent());
    }
}

// ===================== Construccion del panel =====================
static void encoder_bigview_create(void)
{
    if (!objects.main) return;

    lv_obj_t *panel = lv_obj_create(objects.main);
    s_encoder_bigview_panel = panel;
    lv_obj_set_pos(panel, 0, 0);
    lv_obj_set_size(panel, 800, 480);
    lv_obj_set_style_radius(panel, 0, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0xff000000), 0);
    lv_obj_set_style_bg_opa(panel, 255, 0);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(panel, encoder_bigview_tap_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *dist_col = lv_obj_create(panel);
    lv_obj_set_size(dist_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(dist_col, 0, 0);
    lv_obj_set_style_border_width(dist_col, 0, 0);
    lv_obj_set_style_pad_all(dist_col, 0, 0);
    lv_obj_set_style_pad_row(dist_col, 6, 0);
    lv_obj_set_style_layout(dist_col, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(dist_col, LV_FLEX_FLOW_COLUMN, 0);
    lv_obj_set_style_flex_cross_place(dist_col, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_remove_flag(dist_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(dist_col, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(dist_col, LV_ALIGN_CENTER, 0, -100);

    lv_obj_t *led_level_row = lv_obj_create(dist_col);
    lv_obj_set_size(led_level_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(led_level_row, 0, 0);
    lv_obj_set_style_border_width(led_level_row, 0, 0);
    lv_obj_set_style_pad_all(led_level_row, 0, 0);
    lv_obj_set_style_pad_column(led_level_row, 6, 0);
    lv_obj_set_style_layout(led_level_row, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(led_level_row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_remove_flag(led_level_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(led_level_row, LV_OBJ_FLAG_CLICKABLE);

    for (int i = 0; i < 4; i++) {
        lv_obj_t *seg = lv_obj_create(led_level_row);
        s_bigview_led_level_seg[i] = seg;
        lv_obj_set_size(seg, 40, 8);
        lv_obj_set_style_radius(seg, 2, 0);
        lv_obj_set_style_border_width(seg, 0, 0);
        lv_obj_set_style_bg_color(seg, lv_color_hex(0xfff5c518), 0);
        lv_obj_set_style_bg_opa(seg, 60, 0);
        lv_obj_remove_flag(seg, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(seg, LV_OBJ_FLAG_CLICKABLE);
    }
    hmi_bigview_led_level_refresh();

    lv_obj_t *caption = lv_label_create(dist_col);
    s_bigview_distance_caption = caption;
    lv_obj_set_style_text_font(caption, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(caption, lv_color_hex(0xffaaaaaa), 0);
    lv_label_set_text(caption, g_lang->panel_drive_distance_caption);

    lv_obj_t *num_row = lv_obj_create(dist_col);
    lv_obj_set_size(num_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(num_row, 0, 0);
    lv_obj_set_style_border_width(num_row, 0, 0);
    lv_obj_set_style_pad_all(num_row, 0, 0);
    lv_obj_set_style_pad_column(num_row, 8, 0);
    lv_obj_set_style_layout(num_row, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(num_row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_cross_place(num_row, LV_FLEX_ALIGN_END, 0);
    lv_obj_remove_flag(num_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(num_row, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *label = lv_label_create(num_row);
    s_encoder_bigview_label = label;
    lv_obj_set_style_text_font(label, &lv_font_distance, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffffff), 0);
    lv_label_set_text(label, "0.000");

    lv_obj_t *unit = lv_label_create(num_row);
    s_encoder_bigview_unit_label = unit;
    lv_obj_set_style_text_font(unit, &lv_font_montserrat_42, 0);
    lv_obj_set_style_text_color(unit, lv_color_hex(0xffaaaaaa), 0);
    lv_obj_set_style_pad_bottom(unit, 12, 0);
    lv_label_set_text(unit, "m");

    lv_obj_t *angle_caption = lv_label_create(panel);
    s_bigview_trace_caption = angle_caption;
    lv_obj_set_style_text_font(angle_caption, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(angle_caption, lv_color_hex(0xffaaaaaa), 0);
    lv_label_set_text(angle_caption, g_lang->panel_drive_trace_caption);
    lv_obj_align(angle_caption, LV_ALIGN_TOP_MID, 0, 268);

    lv_obj_t *trace_box = lv_obj_create(panel);
    lv_obj_set_size(trace_box, BIGVIEW_TRACE_W + 24, BIGVIEW_TRACE_H + 24);
    lv_obj_align(trace_box, LV_ALIGN_TOP_MID, 0, 296);
    lv_obj_set_style_pad_all(trace_box, 12, 0);
    lv_obj_set_style_bg_color(trace_box, lv_color_hex(0xff1a1a1a), 0);
    lv_obj_set_style_bg_opa(trace_box, 40, 0);
    lv_obj_set_style_border_color(trace_box, lv_color_hex(0xff2d2d2d), 0);
    lv_obj_set_style_border_width(trace_box, 2, 0);
    lv_obj_set_style_border_opa(trace_box, 150, 0);
    lv_obj_set_style_radius(trace_box, 10, 0);
    lv_obj_remove_flag(trace_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(trace_box, LV_OBJ_FLAG_CLICKABLE);

    static lv_point_precise_t level_ref_pts[2];
    level_ref_pts[0].x = 0;
    level_ref_pts[0].y = BIGVIEW_TRACE_H / 2;
    level_ref_pts[1].x = BIGVIEW_TRACE_W;
    level_ref_pts[1].y = BIGVIEW_TRACE_H / 2;
    lv_obj_t *level_ref = lv_line_create(trace_box);
    lv_obj_set_size(level_ref, BIGVIEW_TRACE_W, BIGVIEW_TRACE_H);
    lv_line_set_points(level_ref, level_ref_pts, 2);
    lv_obj_set_style_line_color(level_ref, lv_color_hex(0xff888888), 0);
    lv_obj_set_style_line_width(level_ref, 1, 0);
    lv_obj_set_style_line_dash_width(level_ref, 4, 0);
    lv_obj_set_style_line_dash_gap(level_ref, 4, 0);
    lv_obj_remove_flag(level_ref, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *line = lv_line_create(trace_box);
    s_bigview_angle_line = line;
    lv_obj_set_size(line, BIGVIEW_TRACE_W, BIGVIEW_TRACE_H);
    lv_obj_set_style_line_color(line, lv_color_hex(0xfff5c518), 0);
    lv_obj_set_style_line_width(line, 2, 0);
    lv_obj_set_style_line_rounded(line, true, 0);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *tip = lv_obj_create(trace_box);
    s_bigview_angle_tip = tip;
    lv_obj_set_size(tip, 18, 18);
    lv_obj_set_style_radius(tip, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(tip, lv_color_hex(0xff39ff6a), 0);
    lv_obj_set_style_bg_opa(tip, 255, 0);
    lv_obj_set_style_border_color(tip, lv_color_hex(0xffffffff), 0);
    lv_obj_set_style_border_width(tip, 2, 0);
    lv_obj_remove_flag(tip, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(tip, LV_OBJ_FLAG_CLICKABLE);

    bigview_angle_trace_reset();
    bigview_angle_trace_redraw();

    int32_t ruler_y = 296 + BIGVIEW_TRACE_H + 24 + 6;

    lv_obj_t *ruler_left = lv_label_create(panel);
    s_bigview_ruler_left = ruler_left;
    lv_obj_set_style_text_font(ruler_left, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ruler_left, lv_color_hex(0xff888888), 0);
    lv_label_set_text(ruler_left, "0.0m");
    lv_obj_align(ruler_left, LV_ALIGN_TOP_MID, -(BIGVIEW_TRACE_W / 2), ruler_y);

    lv_obj_t *ruler_mid = lv_label_create(panel);
    s_bigview_ruler_mid = ruler_mid;
    lv_obj_set_style_text_font(ruler_mid, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ruler_mid, lv_color_hex(0xff888888), 0);
    lv_label_set_text(ruler_mid, "0.0m");
    lv_obj_align(ruler_mid, LV_ALIGN_TOP_MID, 0, ruler_y);

    lv_obj_t *ruler_right = lv_label_create(panel);
    s_bigview_ruler_right = ruler_right;
    lv_obj_set_style_text_font(ruler_right, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ruler_right, lv_color_hex(0xff888888), 0);
    lv_label_set_text(ruler_right, "0.0m");
    lv_obj_align(ruler_right, LV_ALIGN_TOP_MID, (BIGVIEW_TRACE_W / 2), ruler_y);

    bigview_ruler_update();

    lv_obj_t *logo = lv_img_create(panel);
    lv_img_set_src(logo, &lock_logo_wordmark);
    lv_image_set_scale(logo, 205);
    lv_obj_set_pos(logo, 16, 16);
    lv_obj_remove_flag(logo, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *model_label = lv_label_create(panel);
    s_bigview_robot_model_label = model_label;
    lv_obj_set_style_text_font(model_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(model_label, lv_color_hex(0xffffffff), 0);
    lv_obj_set_style_text_align(model_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(model_label, "RD--");
    lv_obj_set_width(model_label, lv_obj_get_width(logo));
    lv_obj_align_to(model_label, logo, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);
    lv_obj_add_flag(model_label, LV_OBJ_FLAG_HIDDEN);

    // "Falso bold" (ver comentario igual en main.c): copia identica 1px a
    // la derecha, encima de la primera.
    lv_obj_t *model_label_bold = lv_label_create(panel);
    s_bigview_robot_model_label_bold = model_label_bold;
    lv_obj_set_style_text_font(model_label_bold, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(model_label_bold, lv_color_hex(0xffffffff), 0);
    lv_obj_set_style_text_align(model_label_bold, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(model_label_bold, "RD--");
    lv_obj_set_width(model_label_bold, lv_obj_get_width(logo));
    lv_obj_align_to(model_label_bold, model_label, LV_ALIGN_TOP_LEFT, 1, 0);
    lv_obj_add_flag(model_label_bold, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *batt_row = lv_obj_create(panel);
    lv_obj_set_pos(batt_row, 514, 16);
    lv_obj_set_size(batt_row, 270, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(batt_row, 0, 0);
    lv_obj_set_style_border_width(batt_row, 0, 0);
    lv_obj_set_style_pad_all(batt_row, 0, 0);
    lv_obj_set_style_pad_column(batt_row, 24, 0);
    lv_obj_set_style_layout(batt_row, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(batt_row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_main_place(batt_row, LV_FLEX_ALIGN_END, 0);
    lv_obj_set_style_flex_cross_place(batt_row, LV_FLEX_ALIGN_START, 0);
    lv_obj_remove_flag(batt_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(batt_row, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *led_col = lv_obj_create(batt_row);
    lv_obj_set_size(led_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(led_col, 0, 0);
    lv_obj_set_style_border_width(led_col, 0, 0);
    lv_obj_set_style_pad_top(led_col, 3, 0);
    lv_obj_set_style_pad_bottom(led_col, 3, 0);
    lv_obj_set_style_pad_left(led_col, 10, 0);
    lv_obj_set_style_pad_right(led_col, 10, 0);
    lv_obj_set_style_pad_column(led_col, 20, 0);
    lv_obj_set_style_layout(led_col, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(led_col, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_cross_place(led_col, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_remove_flag(led_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(led_col, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *led_bt = lv_led_create(led_col);
    s_bigview_led_bt = led_bt;
    lv_obj_set_size(led_bt, 12, 12);
    lv_led_set_color(led_bt, lv_color_hex(0xff0034ff));
    lv_led_set_brightness(led_bt, objects.led_bluetooth ? lv_led_get_brightness(objects.led_bluetooth) : 0);

    lv_obj_t *led_on = lv_led_create(led_col);
    s_bigview_led_online = led_on;
    lv_obj_set_size(led_on, 12, 12);
    lv_led_set_color(led_on, lv_color_hex(0xff00971c));
    lv_led_set_brightness(led_on, objects.led_online ? lv_led_get_brightness(objects.led_online) : 0);

    battery_cell_create(batt_row, g_lang->panel_drive_robot_batt,
        &s_bigview_robot_batt_bar, &s_bigview_robot_batt_label, &s_bigview_robot_batt_caption);
    battery_cell_create(batt_row, g_lang->panel_drive_console_batt,
        &s_bigview_console_batt_bar, &s_bigview_console_batt_label, &s_bigview_console_batt_caption);

    bigview_battery_refresh();
}

static void encoder_bigview_open(void)
{
    if (!s_encoder_bigview_panel) encoder_bigview_create();
    if (!s_encoder_bigview_panel) return;
    encoder_dashboard_refresh();
    bigview_battery_refresh();
    lv_obj_move_foreground(s_encoder_bigview_panel);
    lv_obj_remove_flag(s_encoder_bigview_panel, LV_OBJ_FLAG_HIDDEN);
    buzzer_sound_17();
}

static void logo_bigview_open_cb(lv_event_t *e)
{
    (void)e;
    encoder_bigview_open();
}

// Expuesta para que sim_main.c la llame una vez despues de ui_init(),
// igual que hace app_main() con el logo_secret_button_wire() real.
void sim_logo_secret_button_wire(void)
{
    if (!objects.obj1) return;
    lv_obj_add_flag(objects.obj1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(objects.obj1, logo_bigview_open_cb, LV_EVENT_CLICKED, NULL);
}

// screens.c deja la columna de nav de Settings sin LV_OBJ_FLAG_SCROLLABLE
// (pensada para 5 botones); con los 10 botones reales el contenido no entra
// y sin esto no hay forma de bajar a ver Tecnologia/Camara/Bluetooth/Limites
// de Servo. Copia de settings_nav_enable_scroll() (main.c), llamada una vez
// desde sim_main.c despues de ui_init(), igual que el real desde app_main().
void sim_settings_nav_enable_scroll(void)
{
    if (!objects.settings_btn_user) return;
    lv_obj_t *nav_col = lv_obj_get_parent(objects.settings_btn_user);
    lv_obj_add_flag(nav_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(nav_col, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(nav_col, LV_SCROLLBAR_MODE_AUTO);
}

// ==================== TEMP: modelo de robot + LED "ping/pong" (solo simulador) ====================
// En la placa real, el LED amarillo (led_hmi_conn) se crea en
// hmi_conn_indicator_create() (main.c) y se prende al recibir HMI_REG_PONG
// de la consola. Como en el simulador no hay consola real, ese LED ni
// siquiera existe hasta que lo creamos aca — y el modelo de robot
// (apply_robot_model) tampoco se aplica nunca sin HMI_REG_ROBOT_MODEL real.

static lv_obj_t *s_led_hmi_conn = NULL;

// Copia de hmi_conn_indicator_create() (main.c), sin el timer de timeout
// (en el simulador no hace falta apagarlo solo — el boton de abajo lo
// prende/apaga a mano). Llamar una vez desde sim_main.c despues de ui_init().
void sim_hmi_conn_indicator_create(void)
{
    if (s_led_hmi_conn || !objects.led_online) return;
    lv_obj_t *pill = lv_obj_get_parent(objects.led_online);
    lv_obj_set_width(pill, 130);
    lv_obj_set_x(pill, 427);

    s_led_hmi_conn = lv_led_create(pill);
    lv_obj_set_size(s_led_hmi_conn, 12, 12);
    lv_led_set_color(s_led_hmi_conn, lv_color_hex(0xFFBF00));
    lv_led_set_brightness(s_led_hmi_conn, 0);
    lv_obj_move_to_index(s_led_hmi_conn, 1);
}

// Copia de dev_serial_labels_update() (main.c) — el numero de serie de la
// CONSOLA (no del robot): en la placa real sale siempre desde el arranque
// (default de fabrica "260007", editable en DEV_MODE), no depende de
// conectar nada. Aca no se lee de NVS, se usa directo el default. Llamar
// una vez desde sim_main.c despues de ui_init().
#define SIM_SERIAL_NUMBER "260007"

void sim_console_serial_apply(void)
{
    char sn_buff[30];
    snprintf(sn_buff, sizeof(sn_buff), "Console S/N :  RD90C-%s", SIM_SERIAL_NUMBER);
    if (objects.serial_number) lv_label_set_text(objects.serial_number, sn_buff);

    char sn_short[20];
    snprintf(sn_short, sizeof(sn_short), "RD90C-%s", SIM_SERIAL_NUMBER);
    if (objects.sysinfo_console_serial_value) {
        lv_label_set_text(objects.sysinfo_console_serial_value, sn_short);
    }
}

// Copia del bloque "SET VERSION INFO" de main.c (dentro de vHardwareInit) —
// llena el panel Sistema > Version por INDICE DE HIJO directo (no son
// objects.* con nombre: child(0)=titulo, child(1..6)=filas, cada fila
// child(1)=valor), asi que si screens.c cambia el orden de esas filas hay
// que actualizar esto tambien. En la placa real usa esp_app_get_description(),
// ESPIDF_VERSION, HW_REVISION (todo especifico de ESP-IDF/el hardware real)
// y el boot count guardado en NVS — aca se reemplazan por equivalentes de PC
// donde no aplican. LVGL_VERSION_STR SI es el real (misma libreria).
void sim_version_panel_populate(void)
{
    lv_obj_t *panel = objects.sysinfo_content_version;
    if (!panel) return;
    lv_obj_t *row;
    row = lv_obj_get_child(panel, 1); if (row) lv_label_set_text(lv_obj_get_child(row, 1), "SIM-PC (" __DATE__ ")");
    row = lv_obj_get_child(panel, 2); if (row) lv_label_set_text(lv_obj_get_child(row, 1), "N/A (simulador)");
    row = lv_obj_get_child(panel, 3);
    if (row) {
        char lv_ver[16];
        snprintf(lv_ver, sizeof(lv_ver), "%d.%d.%d", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
        lv_label_set_text(lv_obj_get_child(row, 1), lv_ver);
    }
    row = lv_obj_get_child(panel, 4); if (row) lv_label_set_text(lv_obj_get_child(row, 1), "N/A (PC, no ESP-IDF)");
    row = lv_obj_get_child(panel, 5); if (row) lv_label_set_text(lv_obj_get_child(row, 1), __DATE__);
    row = lv_obj_get_child(panel, 6); if (row) lv_label_set_text(lv_obj_get_child(row, 1), "1");
}

// Copia de apply_robot_model() (main.c) — actualiza el pill superior y el
// label del Panel de Manejo (s_bigview_robot_model_label, ya declarado mas
// arriba en este mismo archivo).
// Llamar una vez desde sim_main.c justo despues de ui_init() — igual que el
// hide inicial en app_main() (main.c real): Camara arranca oculto hasta que
// se "conecte" un modelo RD100 (ver sim_model_rd100_cb() mas abajo).
void sim_camera_settings_init_hidden(void)
{
    if (objects.settings_btn_camera) lv_obj_add_flag(objects.settings_btn_camera, LV_OBJ_FLAG_HIDDEN);
}

// Copia de camera_rl1_mode_wire() (main.c): interruptor ON/OFF de RL1 en
// Ajustes > Camara, arranca en OFF.
static bool s_sim_rl1_on = false;

static void sim_camera_rl1_style_update(void)
{
    hmi_style_btn(objects.cam_rl1_btn_on,  s_sim_rl1_on);
    hmi_style_btn(objects.cam_rl1_btn_off, !s_sim_rl1_on);
    if (objects.cam_front_led)
        lv_led_set_brightness(objects.cam_front_led, s_sim_rl1_on ? 0 : 255);
    if (objects.cam_reverse_led)
        lv_led_set_brightness(objects.cam_reverse_led, s_sim_rl1_on ? 255 : 0);
}

static void sim_camera_rl1_cb(lv_event_t *e)
{
    s_sim_rl1_on = (bool)(intptr_t)lv_event_get_user_data(e);
    hmi_send_data(HMI_REG_RL1, s_sim_rl1_on ? 1 : 0);
    sim_camera_rl1_style_update();
}

void sim_camera_rl1_mode_wire(void)
{
    if (objects.cam_rl1_btn_on)
        lv_obj_add_event_cb(objects.cam_rl1_btn_on, sim_camera_rl1_cb, LV_EVENT_CLICKED, (void *)(intptr_t)1);
    if (objects.cam_rl1_btn_off)
        lv_obj_add_event_cb(objects.cam_rl1_btn_off, sim_camera_rl1_cb, LV_EVENT_CLICKED, (void *)(intptr_t)0);
    sim_camera_rl1_style_update();
}

static void sim_apply_robot_model(int32_t value)
{
    static const char *model_names[] = { "RD80", "RD90", "RD100" };
    const char *model = (value >= 0 && value < 3) ? model_names[value] : "RD??";
    if (objects.robot_model_label) lv_label_set_text(objects.robot_model_label, model);
    if (objects.robot_model_pill) lv_obj_remove_flag(objects.robot_model_pill, LV_OBJ_FLAG_HIDDEN);
    if (s_bigview_robot_model_label) {
        lv_label_set_text(s_bigview_robot_model_label, model);
        lv_obj_remove_flag(s_bigview_robot_model_label, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_bigview_robot_model_label_bold) {
        lv_label_set_text(s_bigview_robot_model_label_bold, model);
        lv_obj_remove_flag(s_bigview_robot_model_label_bold, LV_OBJ_FLAG_HIDDEN);
    }
    // Ajustes > Camara: solo el RD100 trae camara de reversa (mismo criterio
    // que apply_robot_model() en main.c/HMI_REG_ROBOT_MODEL real).
    if (objects.settings_btn_camera) {
        bool is_rd100 = (value == 2);
        if (is_rd100) {
            lv_obj_remove_flag(objects.settings_btn_camera, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(objects.settings_btn_camera, LV_OBJ_FLAG_HIDDEN);
            if (objects.settings_content_camera &&
                !lv_obj_has_flag(objects.settings_content_camera, LV_OBJ_FLAG_HIDDEN)) {
                action_settings_btn_brightness(NULL);
            }
        }
    }
}

// Copia de apply_robot_serial() (main.c) — arma "Robot S/N: RD90R-NNNNNN" en
// los mismos dos lugares que el registro real (dashboard + Sistema >
// Dispositivo). En el firmware llega por HMI_REG_ROBOT_SERIAL; aca, como no
// hay consola real, se simula un numero cada vez que "conectas" un modelo.
static void sim_apply_robot_serial(int32_t value)
{
    char buff[30];
    snprintf(buff, sizeof(buff), "Robot S/N   :  RD90R-%06ld", (long)value);
    set_var_robot_serial(buff);
    if (objects.robot_serial_number) {
        lv_label_set_text_static(objects.robot_serial_number, get_var_robot_serial());
    }

    char sn_buff[20];
    snprintf(sn_buff, sizeof(sn_buff), "RD90R-%06ld", (long)value);
    if (objects.sysinfo_robot_serial_value) {
        lv_label_set_text(objects.sysinfo_robot_serial_value, sn_buff);
    }
}

// Cada "conexion" simulada suma un numero distinto, para que se note que es
// un dato nuevo llegando (no un valor fijo pintado una sola vez).
static int32_t s_fake_serial_next = 400123;

static void sim_model_rd80_cb(lv_event_t *e)  { (void)e; sim_log("Modelo robot: RD80 (simulado)");  sim_apply_robot_model(0); sim_apply_robot_serial(s_fake_serial_next++); }
static void sim_model_rd90_cb(lv_event_t *e)  { (void)e; sim_log("Modelo robot: RD90 (simulado)");  sim_apply_robot_model(1); sim_apply_robot_serial(s_fake_serial_next++); }
static void sim_model_rd100_cb(lv_event_t *e) { (void)e; sim_log("Modelo robot: RD100 (simulado)"); sim_apply_robot_model(2); sim_apply_robot_serial(s_fake_serial_next++); }

static bool s_ping_pong_on = false;
static lv_obj_t *s_ping_pong_lbl = NULL;

static void sim_ping_pong_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    s_ping_pong_on = !s_ping_pong_on;
    sim_log("Ping/Pong: %s", s_ping_pong_on ? "ON" : "OFF");
    if (s_led_hmi_conn) lv_led_set_brightness(s_led_hmi_conn, s_ping_pong_on ? 255 : 0);
    lv_label_set_text(s_ping_pong_lbl, s_ping_pong_on ? "PING/PONG: ON" : "PING/PONG: OFF");
    lv_obj_set_style_bg_color(btn, lv_color_hex(s_ping_pong_on ? SIM_PRIMARY : SIM_BTN), 0);
}

// Agrega la seccion "Modelo Robot" + "Ping/Pong" a la columna del simulador
// (parent = panel creado en sim_fake_data_button_create()).
static void sim_conn_controls_create(lv_obj_t *parent)
{
    lv_obj_t *sep = lv_label_create(parent);
    lv_obj_set_style_text_font(sep, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sep, lv_color_hex(SIM_TEXT_DIM), 0);
    lv_label_set_text(sep, "Modelo robot (conectar):");

    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_column(row, 6, 0);
    lv_obj_set_style_layout(row, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    const char *names[3] = { "RD80", "RD90", "RD100" };
    lv_event_cb_t cbs[3] = { sim_model_rd80_cb, sim_model_rd90_cb, sim_model_rd100_cb };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_button_create(row);
        lv_obj_set_flex_grow(btn, 1);
        lv_obj_set_height(btn, 40);
        lv_obj_set_style_radius(btn, 4, 0);
        lv_obj_set_style_shadow_opa(btn, 0, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(SIM_BTN), 0);
        lv_obj_add_event_cb(btn, cbs[i], LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_center(lbl);
        lv_label_set_text(lbl, names[i]);
    }

    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, LV_PCT(100), 40);
    lv_obj_set_style_radius(btn, 4, 0);
    lv_obj_set_style_shadow_opa(btn, 0, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(SIM_BTN), 0);
    lv_obj_add_event_cb(btn, sim_ping_pong_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl = lv_label_create(btn);
    s_ping_pong_lbl = lbl;
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl);
    lv_label_set_text(lbl, "PING/PONG: OFF");
}
// ==================== FIN TEMP: modelo de robot + LED ping/pong ====================

// ===================== Reemplaza las versiones no-op de main_stub.c =====================
// (Estas SI tienen implementacion real aca, a diferencia del resto de
// stubs/main_stub.c que quedan vacios.)

void hmi_encoder_set_raw(int32_t raw_pulses)
{
    if (raw_pulses < 0) {
        hmi_encoder_reset();
        return;
    }
    int32_t new_raw = raw_pulses;
    bool moved = new_raw > s_encoder_last_raw;
    float prev_dist_m = s_encoder_dist_m;
    s_encoder_last_raw = new_raw;
    encoder_dashboard_refresh();

    if (moved) {
        float delta_dist_m = s_encoder_dist_m - prev_dist_m;
        bigview_angle_trace_push(s_bigview_current_pitch, delta_dist_m);
    }
}

void hmi_encoder_reset(void)
{
    s_encoder_last_raw = 0;
    encoder_dashboard_refresh();
    hmi_send_data(HMI_REG_ENCODER, 0);
}

// ==================== TEMP: boton "SIMULAR DATOS" (solo simulador) ====================
// No existe en el firmware real — en la placa estos valores vienen de la
// consola por UART. Aca no hay consola conectada, asi que sin esto todo se
// queda en 0/gris. Prende un timer que empuja datos falsos (encoder
// avanzando, angulo oscilando, baterias oscilando, LEDs de estado prendidos)
// a los mismos widgets/variables que actualizaria el HMI real.

static bool         s_fake_data_on    = false;
static lv_timer_t   *s_fake_data_timer = NULL;
static lv_obj_t     *s_fake_data_btn   = NULL;
static lv_obj_t     *s_fake_data_lbl   = NULL;
static int32_t       s_fake_raw        = 0;
static float         s_fake_phase      = 0.0f;

static void fake_data_timer_cb(lv_timer_t *t)
{
    (void)t;
    s_fake_phase += 0.12f;

    // Encoder + traza de inclinacion: avanza el robot y hace oscilar el
    // angulo, mismo camino que datos reales llegando por UART.
    s_fake_raw += 40;
    s_bigview_current_pitch = 25.0f * sinf(s_fake_phase);
    hmi_encoder_set_raw(s_fake_raw);

    // Bateria ROBOT: ya NO se toca aca — tiene su propio toggle+slider
    // manual en "Simulador (extras)" (ver apply_robot_battery() mas abajo),
    // igual que la de la CONSOLA (console_batt_slider_cb), asi el "Simular
    // datos" no le pisa el valor que el usuario puso a mano.

    bigview_battery_refresh();
    hmi_bigview_led_level_refresh();

    // LEDs de estado (Bluetooth/Online) "conectados" mientras dura la sim.
    if (objects.led_online)    lv_led_set_brightness(objects.led_online, 255);
    if (objects.led_bluetooth) lv_led_set_brightness(objects.led_bluetooth, 255);
    if (s_bigview_led_online)  lv_led_set_brightness(s_bigview_led_online, 255);
    if (s_bigview_led_bt)      lv_led_set_brightness(s_bigview_led_bt, 255);
}

static void fake_data_btn_cb(lv_event_t *e)
{
    (void)e;
    s_fake_data_on = !s_fake_data_on;
    sim_log("Simular datos: %s", s_fake_data_on ? "ON" : "OFF");

    if (s_fake_data_on) {
        lv_label_set_text(s_fake_data_lbl, "SIMULAR: ON");
        lv_obj_set_style_bg_color(s_fake_data_btn, lv_color_hex(SIM_PRIMARY), 0);
        if (!s_fake_data_timer) {
            s_fake_data_timer = lv_timer_create(fake_data_timer_cb, 200, NULL);
        }
    } else {
        lv_label_set_text(s_fake_data_lbl, "SIMULAR: OFF");
        lv_obj_set_style_bg_color(s_fake_data_btn, lv_color_hex(SIM_BTN), 0);
        if (s_fake_data_timer) {
            lv_timer_delete(s_fake_data_timer);
            s_fake_data_timer = NULL;
        }
        if (objects.led_online)    lv_led_set_brightness(objects.led_online, 0);
        if (objects.led_bluetooth) lv_led_set_brightness(objects.led_bluetooth, 0);
        if (s_bigview_led_online)  lv_led_set_brightness(s_bigview_led_online, 0);
        if (s_bigview_led_bt)      lv_led_set_brightness(s_bigview_led_bt, 0);
    }
}

// Bateria del ROBOT — toggle ON/OFF + slider manual (13.20V-16.80V), igual
// idea que la de la CONSOLA pero CON on/off: apagada, la tarjeta BATERIA
// SISTEMA muestra "0.0 V"/barra vacia (como si el robot nunca mando el
// dato, igual que arranca la placa real antes de la primera trama). Prendida,
// aplica el valor del slider y lo mantiene mientras quede en ese valor
// (moviendo el slider actualiza en vivo, sin falta tocar el boton de nuevo).
static bool       s_robot_batt_on     = false;
static lv_obj_t  *s_robot_batt_btn    = NULL;
static lv_obj_t  *s_robot_batt_lbl    = NULL;
static lv_obj_t  *s_robot_batt_sl     = NULL;
static lv_obj_t  *s_robot_batt_val_lbl = NULL;

// Copia EXACTA de la logica de alerta del case HMI_REG_ROBOT_VOLTAGE en
// main.c (hmi_handle_reg): mismos umbrales en mV, mismo texto de alerta
// (g_lang->alert_shutdown / alert_battery_critical), mismo parpadeo de la
// barra (fade infinito) y del marco de la tarjeta (objects.obj6) + barra
// superior (objects.obj0) cada 500ms. Nada de esto se comparte
// automaticamente con el simulador (ver [[wp-p-v9-2-hmi-sim-architecture]]),
// asi que esta copiado a mano igual que el resto de bigview_port.c.
static bool         s_bat_alert_active      = false;
static lv_timer_t  *s_bat_blink_border_timer = NULL;
static bool         s_blink_border_state    = false;

static void bat_blink_anim_cb(void *obj, int32_t opa)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)opa, 0);
}

static void bat_blink_border_cb(lv_timer_t *timer)
{
    (void)timer;
    s_blink_border_state = !s_blink_border_state;
    if (objects.obj6) {
        lv_obj_set_style_border_color(objects.obj6,
            s_blink_border_state ? lv_color_hex(0xE74C3C) : lv_color_hex(0xff252525), 0);
        lv_obj_set_style_border_width(objects.obj6, s_blink_border_state ? 4 : 2, 0);
    }
    if (objects.obj0) {
        lv_obj_set_style_bg_color(objects.obj0,
            s_blink_border_state ? lv_color_hex(0xE74C3C) : lv_color_hex(0xff393939), 0);
    }
}

// mv=0 => "sin datos" (gris, sin alerta) — mismo camino que value<=0 en
// main.c. mv>0 aplica los umbrales reales: >=15200 verde, >=14400 amarillo,
// >=14000 naranja, si no rojo+alerta (shutdown si <=13200).
static void apply_robot_battery_mv(int32_t mv)
{
    uint8_t pct = battery_percent(mv < 0 ? 0 : (uint16_t)mv); // 12790mV=0% .. 16720mV=100%, igual que battery_percent() real
    float robot_v = mv / 1000.0f;

    char pct_buf[10], volt_buf[10];
    snprintf(pct_buf, sizeof(pct_buf), "%3u %%", pct);
    snprintf(volt_buf, sizeof(volt_buf), "%2.2f V", robot_v);
    set_var_robot_voltage_percent(pct_buf);
    set_var_robot_voltage(volt_buf);

    lv_color_t bat_color;
    bool show_alert = false, extreme_crit = false;
    if (mv <= 0)          { bat_color = lv_color_hex(0x555555); }
    else if (mv >= 15200) { bat_color = lv_color_hex(0x27AE60); }
    else if (mv >= 14400) { bat_color = lv_color_hex(0xF5C518); }
    else if (mv >= 14000) { bat_color = lv_color_hex(0xE67E22); }
    else {
        bat_color = lv_color_hex(0xE74C3C);
        show_alert = true;
        extreme_crit = (mv <= 13200);
    }
    // Mismo mirror que HMI_REG_ROBOT_VOLTAGE en main.c: alerta desactivable
    // desde Settings > Battery > Battery Alert (switch compartido via
    // screens.c/actions.c, solo la bandera+persistencia esta duplicada aca).
    if (!g_battery_alert_enabled) show_alert = false;

    if (objects.robot_voltage) {
        lv_label_set_text(objects.robot_voltage, g_bat_display_percent ? pct_buf : volt_buf);
    }
    if (objects.robot_voltage_percent_bar) {
        lv_bar_set_value(objects.robot_voltage_percent_bar, pct, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(objects.robot_voltage_percent_bar, bat_color, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_invalidate(objects.robot_voltage_percent_bar);
    }
    if (objects.robot_battery_alert) {
        if (show_alert) {
            lv_label_set_text(objects.robot_battery_alert,
                extreme_crit ? g_lang->alert_shutdown : g_lang->alert_battery_critical);
            lv_obj_remove_flag(objects.robot_battery_alert, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(objects.robot_battery_alert, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (show_alert && !s_bat_alert_active) {
        s_bat_alert_active = true;
        sim_log(extreme_crit ? "Bateria robot: CRITICA (shutdown)" : "Bateria robot: baja");
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, objects.robot_voltage_percent_bar);
        lv_anim_set_exec_cb(&a, bat_blink_anim_cb);
        lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
        lv_anim_set_duration(&a, 400);
        lv_anim_set_playback_duration(&a, 400);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&a);
        s_blink_border_state = true;
        if (objects.obj6) {
            lv_obj_set_style_border_color(objects.obj6, lv_color_hex(0xE74C3C), 0);
            lv_obj_set_style_border_width(objects.obj6, 4, 0);
        }
        if (objects.obj0) lv_obj_set_style_bg_color(objects.obj0, lv_color_hex(0xE74C3C), 0);
        if (!s_bat_blink_border_timer) {
            s_bat_blink_border_timer = lv_timer_create(bat_blink_border_cb, 500, NULL);
        }
    } else if (!show_alert && s_bat_alert_active) {
        s_bat_alert_active = false;
        lv_anim_delete(objects.robot_voltage_percent_bar, bat_blink_anim_cb);
        if (objects.robot_voltage_percent_bar) lv_obj_set_style_opa(objects.robot_voltage_percent_bar, LV_OPA_COVER, 0);
        if (s_bat_blink_border_timer) { lv_timer_delete(s_bat_blink_border_timer); s_bat_blink_border_timer = NULL; }
        if (objects.obj6) {
            lv_obj_set_style_border_color(objects.obj6, hmi_theme_bd_card(), 0);
            lv_obj_set_style_border_width(objects.obj6, 2, 0);
        }
        if (objects.obj0) lv_obj_set_style_bg_color(objects.obj0, hmi_theme_bg_topbar(), 0);
    }

    bigview_battery_refresh();

    if (s_robot_batt_val_lbl) {
        char lbl_buf[16];
        if (mv <= 0) lv_label_set_text(s_robot_batt_val_lbl, "sin datos");
        else { snprintf(lbl_buf, sizeof(lbl_buf), "%.2fV (%u%%)", robot_v, pct); lv_label_set_text(s_robot_batt_val_lbl, lbl_buf); }
    }
}

static void apply_robot_battery(float robot_v)
{
    apply_robot_battery_mv((int32_t)(robot_v * 1000.0f + 0.5f));
}

// "Sin datos": mismo aspecto que antes de que llegue la primera trama real
// del robot — 0.0V, barra vacia, sin alerta.
static void apply_robot_battery_off(void)
{
    apply_robot_battery_mv(0);
}

static void robot_batt_slider_cb(lv_event_t *e)
{
    lv_obj_t *sl = lv_event_get_target(e);
    float robot_v = lv_slider_get_value(sl) / 100.0f; // el slider guarda voltios*100 (entero)
    if (!s_robot_batt_on) return; // se mueve el slider pero no se aplica hasta prender el toggle
    apply_robot_battery(robot_v);
    sim_log("Bateria robot: %.2fV", robot_v);
}

static void robot_batt_btn_cb(lv_event_t *e)
{
    (void)e;
    s_robot_batt_on = !s_robot_batt_on;
    sim_log("Simular bateria robot: %s", s_robot_batt_on ? "ON" : "OFF");

    if (s_robot_batt_on) {
        lv_label_set_text(s_robot_batt_lbl, "BATERIA ROBOT: ON");
        lv_obj_set_style_bg_color(s_robot_batt_btn, lv_color_hex(SIM_PRIMARY), 0);
        if (s_robot_batt_sl) apply_robot_battery(lv_slider_get_value(s_robot_batt_sl) / 100.0f);
    } else {
        lv_label_set_text(s_robot_batt_lbl, "BATERIA ROBOT: OFF");
        lv_obj_set_style_bg_color(s_robot_batt_btn, lv_color_hex(SIM_BTN), 0);
        apply_robot_battery_off();
    }
}

// Bateria de la CONSOLA — 100% manual (no depende de "Simular datos"), asi
// se puede ajustar el voltaje a mano en cualquier momento, incluso con el
// resto de la simulacion apagada. Aplica el mismo camino que usaria un dato
// real llegando por UART (set_var_console_voltage*, objects.console_*,
// battery_bar_color, bigview_battery_refresh) — ver fake_data_timer_cb.
static lv_obj_t *s_console_batt_val_lbl = NULL;

static void apply_console_battery(float console_v)
{
    int console_pct = (int)((console_v - 13.2f) / (16.8f - 13.2f) * 100.0f + 0.5f);
    if (console_pct < 0) console_pct = 0;
    if (console_pct > 100) console_pct = 100;

    char pct_buf[10], volt_buf[10];
    snprintf(pct_buf, sizeof(pct_buf), "%3d %%", console_pct);
    snprintf(volt_buf, sizeof(volt_buf), "%2.2f V", console_v);
    set_var_console_voltage_percent(pct_buf);
    set_var_console_voltage(volt_buf);
    if (objects.console_voltage) {
        lv_label_set_text(objects.console_voltage, g_bat_display_percent ? pct_buf : volt_buf);
    }
    if (objects.console_voltage_percent_label) {
        lv_label_set_text(objects.console_voltage_percent_label, pct_buf);
    }
    if (objects.console_voltage_percent_bar) {
        lv_bar_set_value(objects.console_voltage_percent_bar, console_pct, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(objects.console_voltage_percent_bar, battery_bar_color((uint8_t)console_pct), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }
    bigview_battery_refresh();

    if (s_console_batt_val_lbl) {
        char lbl_buf[16];
        snprintf(lbl_buf, sizeof(lbl_buf), "%.2fV (%d%%)", console_v, console_pct);
        lv_label_set_text(s_console_batt_val_lbl, lbl_buf);
    }
}

static void console_batt_slider_cb(lv_event_t *e)
{
    lv_obj_t *sl = lv_event_get_target(e);
    float console_v = lv_slider_get_value(sl) / 100.0f; // el slider guarda voltios*100 (entero)
    apply_console_battery(console_v);
    sim_log("Bateria consola: %.2fV", console_v);
}

// El panel (con el slider al 100% por defecto) se arma ANTES de que exista
// el HMI real (objects.* recien lo crea ui_init(), que corre al terminar el
// splash — ver sim_boot_finish() en sim_main.c) — asi que el valor inicial
// no llega a pintarse en la tarjeta BATERIA SISTEMA hasta que el usuario
// toca el slider. sim_main.c llama esto de nuevo justo despues de ui_init()
// para que arranque ya poblado, sin necesidad de tocar nada.
void sim_console_battery_seed(void)
{
    apply_console_battery(16.8f);
}

// Mismo motivo que sim_console_battery_seed() (objects.* no existe todavia
// cuando se crea el panel de controles) — pinta el estado inicial "sin
// datos" de la bateria ROBOT (arranca en OFF).
void sim_robot_battery_seed(void)
{
    apply_robot_battery_off();
}

// Controles del simulador (Simular datos, Bateria consola, Capturar
// pantalla, Modelo robot, Ping/Pong) — con el layout de 3 ventanas ya no
// arman su propia columna flotante: se agregan como hijos del `parent` que
// le pase la ventana derecha (sim_right_panel_create(), en robot_panel.c),
// debajo del resto de controles de esa ventana. Llamar una vez desde ahi.
static void sim_capture_button_create(lv_obj_t *parent); // definida mas abajo

void sim_extra_controls_create(lv_obj_t *parent)
{
    if (s_fake_data_btn) return;

    lv_obj_t *title = lv_label_create(parent);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(SIM_TEXT_DIM), 0);
    lv_label_set_text(title, "SIMULADOR (extras)");

    lv_obj_t *btn = lv_button_create(parent);
    s_fake_data_btn = btn;
    lv_obj_set_size(btn, LV_PCT(100), 44);
    lv_obj_set_style_radius(btn, 4, 0);
    lv_obj_set_style_shadow_opa(btn, 0, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(SIM_BTN), 0);
    lv_obj_add_event_cb(btn, fake_data_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    s_fake_data_lbl = lbl;
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl);
    lv_label_set_text(lbl, "SIMULAR: OFF");

    // ---- Bateria ROBOT (toggle ON/OFF + manual, 13.20V-16.80V) ----
    lv_obj_t *rbatt_btn = lv_button_create(parent);
    s_robot_batt_btn = rbatt_btn;
    lv_obj_set_size(rbatt_btn, LV_PCT(100), 40);
    lv_obj_set_style_radius(rbatt_btn, 4, 0);
    lv_obj_set_style_shadow_opa(rbatt_btn, 0, 0);
    lv_obj_set_style_bg_color(rbatt_btn, lv_color_hex(SIM_BTN), 0);
    lv_obj_add_event_cb(rbatt_btn, robot_batt_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *rbatt_lbl = lv_label_create(rbatt_btn);
    s_robot_batt_lbl = rbatt_lbl;
    lv_obj_set_style_text_font(rbatt_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(rbatt_lbl);
    lv_label_set_text(rbatt_lbl, "BATERIA ROBOT: OFF");

    lv_obj_t *rbatt_sl = lv_slider_create(parent);
    s_robot_batt_sl = rbatt_sl;
    lv_obj_set_size(rbatt_sl, LV_PCT(100), 14);
    lv_slider_set_range(rbatt_sl, 1320, 1680); // voltios*100: 13.20V..16.80V
    lv_slider_set_value(rbatt_sl, 1680, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(rbatt_sl, lv_color_hex(SIM_ACCENT), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(rbatt_sl, lv_color_hex(SIM_ACCENT), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(rbatt_sl, robot_batt_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *rbatt_val = lv_label_create(parent);
    s_robot_batt_val_lbl = rbatt_val;
    lv_obj_set_style_text_font(rbatt_val, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(rbatt_val, lv_color_hex(SIM_ACCENT), 0);
    lv_label_set_text(rbatt_val, "sin datos");
    // El estado inicial ("sin datos", 0.0V) en la tarjeta BATERIA SISTEMA lo
    // pinta sim_robot_battery_seed() (llamada desde sim_main.c despues de
    // ui_init()) — aca objects.* todavia no existe.

    // ---- Bateria CONSOLA (manual, 13.20V-16.80V) ----
    lv_obj_t *batt_name = lv_label_create(parent);
    lv_obj_set_style_text_font(batt_name, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(batt_name, lv_color_hex(SIM_TEXT_DIM), 0);
    lv_label_set_text(batt_name, "Bateria CONSOLA (manual)");

    lv_obj_t *batt_sl = lv_slider_create(parent);
    lv_obj_set_size(batt_sl, LV_PCT(100), 14);
    lv_slider_set_range(batt_sl, 1320, 1680); // voltios*100: 13.20V..16.80V
    lv_slider_set_value(batt_sl, 1680, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(batt_sl, lv_color_hex(SIM_ACCENT), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(batt_sl, lv_color_hex(SIM_ACCENT), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(batt_sl, console_batt_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *batt_val = lv_label_create(parent);
    s_console_batt_val_lbl = batt_val;
    lv_obj_set_style_text_font(batt_val, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(batt_val, lv_color_hex(SIM_ACCENT), 0);
    lv_label_set_text(batt_val, "16.80V (100%)");
    // El valor inicial en la tarjeta BATERIA SISTEMA del HMI lo pinta
    // sim_console_battery_seed() (llamada desde sim_main.c despues de
    // ui_init()) — aca objects.* todavia no existe.

    sim_capture_button_create(parent);
    sim_conn_controls_create(parent);
}
// ==================== FIN TEMP: boton "SIMULAR DATOS" ====================

// ==================== TEMP: boton "CAPTURAR PANTALLA" (solo simulador) ====================
// Guarda un .bmp con SOLO los 800x480 del area del dispositivo (nunca la
// columna del simulador, que vive a partir de x=SIM_PANEL_X=800) — lee el
// framebuffer directo del renderer de SDL, asi que no importa que pantalla/
// panel este activo, siempre captura exactamente lo que se veria en la
// placa real.

static lv_obj_t   *s_capture_lbl   = NULL;
static lv_timer_t *s_capture_flash_timer = NULL;
static int         s_capture_count = 0;

static void capture_flash_revert_cb(lv_timer_t *t)
{
    (void)t;
    if (s_capture_lbl) lv_label_set_text(s_capture_lbl, "CAPTURAR PANTALLA");
    s_capture_flash_timer = NULL;
}

// s_capture_count arranca en 0 cada vez que se relanza el simulador (es una
// variable en memoria, no algo persistido en disco) — sin esto, cada reinicio
// volvia a nombrar "captura_001.png" y PISABA capturas de una corrida
// anterior con el mismo nombre. Se llama una sola vez, antes de la primera
// captura de la sesion, y arranca el contador despues del numero mas alto
// que ya exista en la carpeta.
static void capture_count_init_from_disk(void)
{
    static bool done = false;
    if (done) return;
    done = true;

    struct _finddata_t fd;
    intptr_t h = _findfirst("capturas/captura_*.png", &fd);
    if (h == -1) return;
    do {
        int n = 0;
        if (sscanf(fd.name, "captura_%d.png", &n) == 1 && n > s_capture_count) {
            s_capture_count = n;
        }
    } while (_findnext(h, &fd) == 0);
    _findclose(h);
}

static void capture_btn_cb(lv_event_t *e)
{
    (void)e;
    capture_count_init_from_disk();

    lv_display_t *disp = s_main_disp ? s_main_disp : lv_display_get_default();
    SDL_Renderer *renderer = (SDL_Renderer *)lv_sdl_window_get_renderer(disp);
    if (!renderer) return;

    // SOLO el area del dispositivo real. OJO: no usar SIM_HMI_X (fijo) —
    // si la ventana esta maximizada/agrandada, la columna MOTOR crece y
    // empuja el HMI a la derecha (ver sim_side_panels_update_layout() en
    // robot_panel.c), asi que hay que capturar desde su x REAL actual.
    SDL_Rect rect = { sim_get_hmi_x(), 0, SIM_HMI_W, SIM_WINDOW_H };
    // RGB24 (sin alfa, no hace falta para una captura de pantalla) — el
    // orden de bytes en memoria coincide exacto con lo que espera
    // stbi_write_png (R,G,B por pixel), asi que no hace falta reordenar nada.
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, SIM_HMI_W, SIM_WINDOW_H, 24, SDL_PIXELFORMAT_RGB24);
    if (!surf) return;

    bool ok = (SDL_RenderReadPixels(renderer, &rect, SDL_PIXELFORMAT_RGB24, surf->pixels, surf->pitch) == 0);

    char filename[80] = "";
    if (ok) {
        _mkdir("capturas"); // ya existe -> devuelve error, se ignora a proposito
        s_capture_count++;
        snprintf(filename, sizeof(filename), "capturas/captura_%03d.png", s_capture_count);
        ok = (stbi_write_png(filename, SIM_HMI_W, SIM_WINDOW_H, 3, surf->pixels, surf->pitch) != 0);
    }
    SDL_FreeSurface(surf);

    if (ok) sim_log("Captura guardada: %s", filename);
    else    sim_log("Error al guardar la captura");

    if (s_capture_lbl) {
        lv_label_set_text(s_capture_lbl, ok ? "GUARDADA!" : "ERROR AL GUARDAR");
    }
    if (s_capture_flash_timer) lv_timer_reset(s_capture_flash_timer);
    else {
        s_capture_flash_timer = lv_timer_create(capture_flash_revert_cb, 1200, NULL);
        lv_timer_set_repeat_count(s_capture_flash_timer, 1);
    }
}

// Crea el boton de captura dentro de la columna del simulador (parent =
// panel creado en sim_fake_data_button_create()).
static void sim_capture_button_create(lv_obj_t *parent)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, LV_PCT(100), 44);
    lv_obj_set_style_radius(btn, 4, 0);
    lv_obj_set_style_shadow_opa(btn, 0, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(SIM_PRIMARY), 0);
    lv_obj_add_event_cb(btn, capture_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    s_capture_lbl = lbl;
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl);
    lv_label_set_text(lbl, "CAPTURAR PANTALLA");

    lv_obj_t *hint = lv_label_create(parent);
    lv_obj_set_width(hint, LV_PCT(100));
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(SIM_TEXT_DIM), 0);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
    lv_label_set_text(hint, "Guarda captura_NNN.png en la carpeta del .exe (solo el area 800x480 del HMI, sin las columnas del simulador)");
}
// ==================== FIN TEMP: boton "CAPTURAR PANTALLA" ====================
