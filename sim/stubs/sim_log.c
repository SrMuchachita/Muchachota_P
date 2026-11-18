/*
 * Ver sim_log.h. Dos canales independientes ("LOG" general — RS485/HMI/
 * capturas/etc — y "PS3" — conexion y ejes del control fisico), cada uno
 * con su propio buffer circular de texto; una fila de pestañas arriba
 * decide cual de los dos se ve en el label scrollable de abajo. Agregar a
 * un canal que NO esta activo no lo muestra hasta que se cambia de
 * pestaña, pero igual queda guardado en su buffer.
 *
 * El panel queda alineado con el HMI (mismo x=SIM_HMI_X y ancho=SIM_HMI_W),
 * NO con toda la ventana — las columnas MOTOR/SERVOS de los costados usan
 * su propio espacio extra para crecer en alto (ver
 * sim_side_panels_update_layout() en robot_panel.c), no para el log.
 */

#include "lvgl.h"
#include "sim_layout.h"
#include "sim_log.h"
#include "sim_theme.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <windows.h> // GetTickCount, para el timestamp del log

#define SIM_LOG_BUF_SIZE 8192

typedef enum {
    SIM_LOG_CH_MAIN = 0, // RS485/HMI/capturas/simulador
    SIM_LOG_CH_PAD  = 1, // control fisico (PS3/Xbox/etc)
    SIM_LOG_CH_COUNT
} sim_log_channel_t;

static const char *s_ch_names[SIM_LOG_CH_COUNT] = { "LOG", "PS3" };

static lv_obj_t *s_log_panel = NULL;
static lv_obj_t *s_log_cont  = NULL;
static lv_obj_t *s_log_label = NULL;
static lv_obj_t *s_tab_btn[SIM_LOG_CH_COUNT]  = { NULL, NULL };
static lv_obj_t *s_tab_lbl[SIM_LOG_CH_COUNT]  = { NULL, NULL };
static char      s_log_buf[SIM_LOG_CH_COUNT][SIM_LOG_BUF_SIZE] = { { 0 } };
static int       s_active_ch = SIM_LOG_CH_MAIN;
static DWORD     s_log_t0 = 0;

static void refresh_tab_styles(void)
{
    for (int i = 0; i < SIM_LOG_CH_COUNT; i++) {
        if (!s_tab_btn[i]) continue;
        bool active = (i == s_active_ch);
        lv_obj_set_style_bg_color(s_tab_btn[i], lv_color_hex(active ? SIM_PRIMARY : SIM_BTN), 0);
        if (s_tab_lbl[i]) lv_obj_set_style_text_color(s_tab_lbl[i], lv_color_hex(active ? SIM_TEXT : SIM_TEXT_DIM), 0);
    }
}

static void refresh_display(void)
{
    if (!s_log_label) return;
    lv_label_set_text(s_log_label, s_log_buf[s_active_ch]);
    if (s_log_cont) lv_obj_scroll_to_y(s_log_cont, LV_COORD_MAX, LV_ANIM_OFF);
}

static void tab_click_cb(lv_event_t *e)
{
    int ch = (int)(intptr_t)lv_event_get_user_data(e);
    if (ch == s_active_ch) return;
    s_active_ch = ch;
    refresh_tab_styles();
    refresh_display();
}

void sim_log_panel_create(void)
{
    if (s_log_panel) return;
    s_log_t0 = GetTickCount();

    lv_obj_t *panel = lv_obj_create(lv_layer_top());
    s_log_panel = panel;
    lv_obj_set_pos(panel, SIM_HMI_X, SIM_WINDOW_H); // mismo ancho/alineado que el HMI, no toda la ventana
    lv_obj_set_size(panel, SIM_HMI_W, 1); // real: sim_log_panel_update_layout()
    lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN); // oculto hasta que haya espacio extra
    lv_obj_set_style_bg_color(panel, lv_color_hex(SIM_BG), 0);
    lv_obj_set_style_bg_opa(panel, 255, 0);
    lv_obj_set_style_border_side(panel, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(SIM_BORDER), 0);
    lv_obj_set_style_radius(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 8, 0);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    // ---- Fila de pestañas: LOG / PS3 ----
    lv_obj_t *tabs = lv_obj_create(panel);
    lv_obj_set_pos(tabs, 0, 0);
    lv_obj_set_size(tabs, LV_PCT(100), 26);
    lv_obj_set_style_bg_opa(tabs, 0, 0);
    lv_obj_set_style_border_width(tabs, 0, 0);
    lv_obj_set_style_pad_all(tabs, 0, 0);
    lv_obj_set_style_pad_column(tabs, 6, 0);
    lv_obj_set_style_layout(tabs, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(tabs, LV_FLEX_FLOW_ROW, 0);
    lv_obj_remove_flag(tabs, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < SIM_LOG_CH_COUNT; i++) {
        lv_obj_t *btn = lv_button_create(tabs);
        s_tab_btn[i] = btn;
        lv_obj_set_size(btn, 70, 24);
        lv_obj_set_style_radius(btn, 4, 0);
        lv_obj_set_style_shadow_opa(btn, 0, 0);
        lv_obj_add_event_cb(btn, tab_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t *lbl = lv_label_create(btn);
        s_tab_lbl[i] = lbl;
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_center(lbl);
        lv_label_set_text(lbl, s_ch_names[i]);
    }
    refresh_tab_styles();

    lv_obj_t *cont = lv_obj_create(panel);
    s_log_cont = cont;
    lv_obj_set_pos(cont, 0, 32);
    lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(cont, 0, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_scroll_dir(cont, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);

    lv_obj_t *label = lv_label_create(cont);
    s_log_label = label;
    lv_obj_set_width(label, LV_PCT(100));
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(SIM_TEXT_DIM), 0);
    lv_label_set_text(label, "");

    sim_log("Simulador iniciado");
}

void sim_log_panel_update_layout(void)
{
    if (!s_log_panel) return;
    lv_display_t *disp = lv_display_get_default();
    int32_t ver_res = lv_display_get_vertical_resolution(disp);
    int32_t extra_h = ver_res - SIM_WINDOW_H;

    if (extra_h < 40) { // muy poco espacio como para mostrar algo util
        lv_obj_add_flag(s_log_panel, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_remove_flag(s_log_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(s_log_panel, sim_get_hmi_x(), SIM_WINDOW_H);
    lv_obj_set_size(s_log_panel, SIM_HMI_W, extra_h);
    if (s_log_cont) lv_obj_set_size(s_log_cont, LV_PCT(100), extra_h - 40);
}

static void sim_log_ch(sim_log_channel_t ch, const char *fmt, va_list args)
{
    if (!s_log_label) return;

    DWORD ms = GetTickCount() - s_log_t0;
    char line[256];
    int p = snprintf(line, sizeof(line), "[%3lu.%03lus] ", (unsigned long)(ms / 1000), (unsigned long)(ms % 1000));
    vsnprintf(line + p, sizeof(line) - p, fmt, args);
    strncat(line, "\n", sizeof(line) - strlen(line) - 1);

    char *buf = s_log_buf[ch];
    size_t cur_len = strlen(buf);
    size_t line_len = strlen(line);
    if (cur_len + line_len >= SIM_LOG_BUF_SIZE) {
        // Sin espacio: tira el primer 25% del buffer (por linea completa).
        size_t drop = SIM_LOG_BUF_SIZE / 4;
        char *nl = strchr(buf + drop, '\n');
        size_t keep_from = nl ? (size_t)(nl - buf) + 1 : drop;
        memmove(buf, buf + keep_from, cur_len - keep_from + 1);
        cur_len = strlen(buf);
    }
    strncat(buf, line, SIM_LOG_BUF_SIZE - cur_len - 1);

    if (ch == s_active_ch) refresh_display();
}

void sim_log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    sim_log_ch(SIM_LOG_CH_MAIN, fmt, args);
    va_end(args);
}

void sim_log_pad(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    sim_log_ch(SIM_LOG_CH_PAD, fmt, args);
    va_end(args);
}
