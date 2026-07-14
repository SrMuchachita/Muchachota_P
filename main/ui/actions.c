#include "vars.h"
#include "screens.h"
#include "actions.h"
#include "main.h"
#include "lang.h"

void action_robot_brightness_changed(lv_event_t *e) {
    // TODO: Implement action robot_brightness_changed here
    lv_obj_t * obj = lv_event_get_target(e);
    int32_t slider_value = lv_slider_get_value(obj);
    int32_t value_pwm = (slider_value * 1023) / 100;

    static uint32_t start_time;

    char buff[10];
    lv_snprintf(buff, sizeof(buff), "%d %%", slider_value);
    set_var_brightness(buff);
    lv_label_set_text_static(objects.brightness_label,get_var_brightness());

    // a 200ms
    if (lv_tick_elaps(start_time) > 100){
        start_time = lv_tick_get();
        // Envio de datos a la consola
        hmi_send_data(HMI_REG_ROBOT_LED_CHANGED, value_pwm);
    }
}

void action_robot_brightness_released(lv_event_t *e) {
    // TODO: Implement action robot_brightness_released here
    lv_obj_t * obj = lv_event_get_target(e);
    int32_t slider_value = lv_slider_get_value(obj);
    int32_t value_pwm = (slider_value * 1023) / 100;

    // Envio de datos a la consola
    hmi_send_data(HMI_REG_ROBOT_LED_CHANGED, value_pwm);
}

void action_encoder_button_reset_clicked(lv_event_t *e) {
    // Button encoder
    hmi_encoder_set_raw(0);

    // Envio de datos a la consola
    hmi_send_data(HMI_REG_ENCODER, 0);
}

void action_angle_neck_btn_decreased(lv_event_t *e) {
    // TODO: Implement action angle_neck_btn_decreased here
    int32_t value = get_var_angle_neck_value();
    value = value > 0 ? value - 1 : 0;
    set_var_angle_neck_value(value);

    char buff[10];
    lv_snprintf(buff, sizeof(buff), "%03d", value);
    set_var_angle_neck_string(buff);

    lv_label_set_text_static(objects.angle_neck_label, get_var_angle_neck_string());
}

void action_angle_neck_btn_increased(lv_event_t *e) {
    // TODO: Implement action angle_neck_btn_increased here
    int32_t value = get_var_angle_neck_value();
    value = value < 180 ? value + 1 : 180;
    set_var_angle_neck_value(value);

    char buff[10];
    lv_snprintf(buff, sizeof(buff), "%03d", value);
    set_var_angle_neck_string(buff);

    lv_label_set_text_static(objects.angle_neck_label, get_var_angle_neck_string());
}

void action_angle_head_btn_decreased(lv_event_t *e) {
    // TODO: Implement action angle_head_btn_decreased here
    int32_t value = get_var_angle_head_value();
    value = value > 0 ? value - 1 : 0;
    set_var_angle_head_value(value);

    char buff[10];
    lv_snprintf(buff, sizeof(buff), "%03d", value);
    set_var_angle_head_string(buff);
    
    lv_label_set_text_static(objects.angle_head_label, get_var_angle_head_string());
}

void action_angle_head_btn_increased(lv_event_t *e) {
    // TODO: Implement action angle_neck_btn_increased here
    int32_t value = get_var_angle_head_value();
    value = value < 180 ? value + 1 : 180;
    set_var_angle_head_value(value);

    char buff[10];
    lv_snprintf(buff, sizeof(buff), "%03d", value);
    set_var_angle_head_string(buff);

    lv_label_set_text_static(objects.angle_head_label, get_var_angle_head_string());
}

void action_angle_neck_btn_decreased_pressing(lv_event_t *e) {
    // TODO: Implement action angle_neck_btn_decreased_pressing here
    int32_t value = get_var_angle_neck_value();
    value = value > 0 ? value - 1 : 0;
    set_var_angle_neck_value(value);

    char buff[10];
    lv_snprintf(buff, sizeof(buff), "%03d", value);
    set_var_angle_neck_string(buff);

    lv_label_set_text_static(objects.angle_neck_label, get_var_angle_neck_string());
}

void action_angle_neck_btn_increased_pressing(lv_event_t *e) {
    // TODO: Implement action angle_neck_btn_increased_pressing here
    int32_t value = get_var_angle_neck_value();
    value = value < 180 ? value + 1 : 180;
    set_var_angle_neck_value(value);

    char buff[10];
    lv_snprintf(buff, sizeof(buff), "%03d", value);
    set_var_angle_neck_string(buff);

    lv_label_set_text_static(objects.angle_neck_label, get_var_angle_neck_string());
}

void action_angle_head_btn_decreased_pressing(lv_event_t *e) {
    // TODO: Implement action angle_head_btn_decreased_pressing here
    int32_t value = get_var_angle_head_value();
    value = value > 0 ? value - 1 : 0;
    set_var_angle_head_value(value);

    char buff[10];
    lv_snprintf(buff, sizeof(buff), "%03d", value);
    set_var_angle_head_string(buff);
    
    lv_label_set_text_static(objects.angle_head_label, get_var_angle_head_string());
}

void action_angle_head_btn_increased_pressing(lv_event_t *e) {
    // TODO: Implement action angle_head_btn_increased_pressing here
    int32_t value = get_var_angle_head_value();
    value = value < 180 ? value + 1 : 180;
    set_var_angle_head_value(value);

    char buff[10];
    lv_snprintf(buff, sizeof(buff), "%03d", value);
    set_var_angle_head_string(buff);

    lv_label_set_text_static(objects.angle_head_label, get_var_angle_head_string());
}

void action_console_brightness_changed(lv_event_t *e) {
    // TODO: Implement action console_brightness_changed here
    lv_obj_t * obj = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(obj);

    static uint32_t start_time = 0;

    /*Si paso mas de 100ms entonces cambiar el valor del brillo*/
    if (lv_tick_elaps(start_time) > 20){
        // Cambiar brillo pantalla
        lcd_set_brightness((int32_t)value < 1 ? 1 : (int32_t)value);
        // Guardar nuevo valor en variable global
        char buff[10]; // Variable local o auxiliar
        lv_snprintf(buff, sizeof(buff), "%3d", value);
        set_var_console_brightness(buff);
        // Setear el nuevo valor
        lv_label_set_text_static(objects.console_brightness_label, get_var_console_brightness());
        // Actualizar tiempo
        start_time = lv_tick_get();
    }
}

void action_console_brightness_released(lv_event_t *e) {
    // TODO: Implement action console_brightness_released here
    lv_obj_t * obj = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(obj);

    // Cambiar brillo pantalla
    lcd_set_brightness((int32_t)value < 1 ? 1 : (int32_t)value);
    // Guardar nuevo valor en variable global
    char buff[10]; // Variable local o auxiliar
    lv_snprintf(buff, sizeof(buff), "%3d", value);
    set_var_console_brightness(buff);
    // Setear el nuevo valor
    lv_label_set_text_static(objects.console_brightness_label, get_var_console_brightness());
}

void action_angle_neck_label_pressed(lv_event_t *e) {
    // TODO: Implement action angle_neck_label_pressed here
    int32_t value = get_var_angle_neck_value();
    hmi_send_data(HMI_REG_ANGLE_NECK_CHANGED, value);
}

void action_angle_head_label_pressed(lv_event_t *e) {
    // TODO: Implement action angle_head_label_pressed here
    int32_t value = get_var_angle_head_value();
    hmi_send_data(HMI_REG_ANGLE_HEAD_CHANGED, value);
}

void action_btn_giro_automatico(lv_event_t *e) {
    // TODO: Implement action btn_giro_automatico here
    int32_t value = 0;

    hmi_send_data(HMI_REG_GIRO_AUTOMATICO, 1);

    /*value = get_var_angle_head_value();
    hmi_send_data(HMI_REG_ANGLE_HEAD_CHANGED, value);

    value = get_var_angle_neck_value();
    hmi_send_data(HMI_REG_ANGLE_NECK_CHANGED, value);*/
}

void action_btn_start_demo(lv_event_t *e) {
    // TODO: Implement action btn_start_demo here
    hmi_send_data(HMI_REG_START_DEMO, 1);
}

void action_btn_stop_demo(lv_event_t *e) {
    // TODO: Implement action btn_stop_demo here
    hmi_send_data(HMI_REG_STOP_DEMO, 1);
}

void action_btn_center(lv_event_t *e) {
    int32_t neck   = get_var_angle_neck_value();
    int32_t head   = get_var_angle_head_value();
    int32_t packed = (neck << 16) | (head & 0xFFFF);
    hmi_send_data(HMI_REG_CENTER, packed);
}

/* ================================================================
 *  THEME SYSTEM  —  0=Dark  1=Classic  2=Light
 * ================================================================ */

typedef struct {
    /* Backgrounds */
    lv_color_t bg_screen;
    lv_color_t bg_topbar;
    lv_color_t bg_indicator;
    lv_color_t bg_card;
    lv_color_t bg_nav;
    lv_color_t bg_tabbar;
    /* Borders */
    lv_color_t bd_card;
    /* Text */
    lv_color_t txt_primary;
    lv_color_t txt_secondary;
    lv_color_t txt_accent;
    /* Interactive buttons */
    lv_color_t bg_btn_active;
    lv_color_t txt_btn_active;
    lv_color_t bd_btn_active;
    lv_color_t bg_btn_inactive;
    lv_color_t txt_btn_inactive;
    lv_color_t bd_btn_inactive;
    /* Controls */
    lv_color_t clr_slider;
    lv_color_t clr_bar;
    lv_color_t clr_bar_bg;
} hmi_theme_t;

static int         g_current_theme      = 1; /* 1=Classic at startup */
static hmi_theme_t g_theme;
static lv_obj_t   *g_settings_active_nav = NULL;
static lv_obj_t   *g_sysinfo_active_nav  = NULL;

/* Single helper — styles any interactive nav/mode button using g_theme.
 * Overrides DEFAULT, PRESSED and CHECKED states to prevent the LVGL basic
 * theme from bleeding gold through lv_btn_create's LV_STATE_CHECKED class. */
static void apply_btn_style(lv_obj_t *btn, bool active) {
    if (!btn) return;
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    if (active) {
        lv_obj_set_style_bg_color(btn,     g_theme.bg_btn_active, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btn,     g_theme.bg_btn_active, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(btn,     g_theme.bg_btn_active, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_border_color(btn, g_theme.bd_btn_active, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(btn, g_theme.bd_btn_active, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn, g_theme.bd_btn_active, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_border_width(btn, 3,                      LV_PART_MAIN | LV_STATE_DEFAULT);
        if (lbl) lv_obj_set_style_text_color(lbl, g_theme.txt_btn_active, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (lbl) lv_obj_set_style_text_color(lbl, g_theme.txt_btn_active, LV_PART_MAIN | LV_STATE_PRESSED);
        if (lbl) lv_obj_set_style_text_color(lbl, g_theme.txt_btn_active, LV_PART_MAIN | LV_STATE_CHECKED);
    } else {
        lv_obj_set_style_bg_color(btn,     g_theme.bg_btn_inactive, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btn,     g_theme.bg_btn_inactive, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(btn,     g_theme.bg_btn_inactive, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_border_color(btn, g_theme.bd_btn_inactive, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(btn, g_theme.bd_btn_inactive, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn, g_theme.bd_btn_inactive, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_border_width(btn, 2,                        LV_PART_MAIN | LV_STATE_DEFAULT);
        if (lbl) lv_obj_set_style_text_color(lbl, g_theme.txt_btn_inactive, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (lbl) lv_obj_set_style_text_color(lbl, g_theme.txt_btn_inactive, LV_PART_MAIN | LV_STATE_PRESSED);
        if (lbl) lv_obj_set_style_text_color(lbl, g_theme.txt_btn_inactive, LV_PART_MAIN | LV_STATE_CHECKED);
    }
}

/* Themes the title label inside a settings content panel.
 * Each panel has: child(0)=title label with left-border accent.   */
static void theme_panel_title(lv_obj_t *panel) {
    if (!panel) return;
    lv_obj_t *title = lv_obj_get_child(panel, 0);
    if (!title) return;
    lv_obj_set_style_text_color(title,   g_theme.txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(title, g_theme.txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
}

/* Applies theme colors to a sysinfo content panel.
 * Panel layout: child(0)=title label, child(1+)=info rows or plain body labels.
 * Info rows have key/val label children; body labels (logs, guide) do not.    */
static void theme_info_panel(lv_obj_t *panel) {
    if (!panel) return;
    /* Title label (child 0) — accent text + border */
    lv_obj_t *title = lv_obj_get_child(panel, 0);
    if (title) {
        lv_obj_set_style_text_color(title,   g_theme.txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(title, g_theme.txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    /* child(1+): info rows or plain body labels */
    uint32_t n = lv_obj_get_child_count(panel);
    for (uint32_t i = 1; i < n; i++) {
        lv_obj_t *row = lv_obj_get_child(panel, i);
        if (!row) continue;
        lv_obj_t *key = lv_obj_get_child(row, 0);
        lv_obj_t *val = lv_obj_get_child(row, 1);
        if (key || val) {
            /* info row: key = secondary, value = primary */
            if (key) lv_obj_set_style_text_color(key, g_theme.txt_secondary, LV_PART_MAIN | LV_STATE_DEFAULT);
            if (val) lv_obj_set_style_text_color(val, g_theme.txt_primary,   LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            /* plain body label (logs, guide body text) */
            lv_obj_set_style_text_color(row, g_theme.txt_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

/* Themes the "User" settings panel — mismo patron plano usado en Brightness/Battery.
 * Layout: child(0)=title,
 *         child(1)=device name row (key label, value label),
 *         child(2)=action buttons row (160x70 "Cambiar Nombre" + "Cambiar PIN"),
 *         child(3)=enable-password row (key label + switch). */
static void theme_user_panel(lv_obj_t *panel) {
    if (!panel) return;
    theme_panel_title(panel);

    lv_obj_t *name_row = lv_obj_get_child(panel, 1);
    if (name_row) {
        lv_obj_t *key = lv_obj_get_child(name_row, 0);
        lv_obj_t *val = lv_obj_get_child(name_row, 1);
        if (key) lv_obj_set_style_text_color(key, g_theme.txt_secondary, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (val) lv_obj_set_style_text_color(val, g_theme.txt_primary,   LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    lv_obj_t *actions_row = lv_obj_get_child(panel, 2);
    if (actions_row) {
        apply_btn_style(lv_obj_get_child(actions_row, 0), false);
        apply_btn_style(lv_obj_get_child(actions_row, 1), false);
    }

    lv_obj_t *sw_row = lv_obj_get_child(panel, 3);
    if (sw_row) {
        lv_obj_t *lbl = lv_obj_get_child(sw_row, 0);
        lv_obj_t *sw  = lv_obj_get_child(sw_row, 1);
        if (lbl) lv_obj_set_style_text_color(lbl, g_theme.txt_secondary, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (sw) {
            lv_obj_set_style_bg_color(sw, g_theme.clr_bar_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(sw, g_theme.clr_slider, LV_PART_INDICATOR | LV_STATE_CHECKED);
            lv_obj_set_style_bg_color(sw, lv_color_hex(0xffffffff), LV_PART_KNOB | LV_STATE_DEFAULT);
        }
    }
}

/* Themes the "Encoder" settings panel — creado dinamicamente en main.c.
 * Layout fijo: child(0)=title, child(1)=subtitulo,
 *              child(2)=fila perimetro, child(3)=fila presets,
 *              child(4)=fila pulsos/vuelta, child(5)=footer formula. */
static void theme_encoder_panel(lv_obj_t *panel) {
    if (!panel) return;
    theme_panel_title(panel);

    lv_obj_t *subtitle = lv_obj_get_child(panel, 1);
    if (subtitle) lv_obj_set_style_text_color(subtitle, g_theme.txt_secondary, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *perim_row = lv_obj_get_child(panel, 2);
    if (perim_row) {
        lv_obj_t *key  = lv_obj_get_child(perim_row, 0);
        lv_obj_t *unit = lv_obj_get_child(perim_row, 2);
        if (key)  lv_obj_set_style_text_color(key,  g_theme.txt_secondary, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (unit) lv_obj_set_style_text_color(unit, g_theme.txt_secondary, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    lv_obj_t *ppr_row = lv_obj_get_child(panel, 4);
    if (ppr_row) {
        lv_obj_t *key  = lv_obj_get_child(ppr_row, 0);
        lv_obj_t *unit = lv_obj_get_child(ppr_row, 2);
        if (key)  lv_obj_set_style_text_color(key,  g_theme.txt_secondary, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (unit) lv_obj_set_style_text_color(unit, g_theme.txt_secondary, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    lv_obj_t *footer = lv_obj_get_child(panel, 5);
    if (footer) lv_obj_set_style_text_color(footer, g_theme.txt_secondary, LV_PART_MAIN | LV_STATE_DEFAULT);
}

/* Weak: reaplicado en main.c para recolorear el boton toggle de WiFi (su
 * color activo/inactivo depende de estado logico que solo main.c conoce). */
void __attribute__((weak)) hmi_update_panel_retheme(void) {}

/* Themes the "Update" (WiFi/OTA) panel de System Info — creado dinamicamente
 * en main.c. No se reusa theme_info_panel() generico: su heuristica por
 * cantidad de hijos (2 hijos = fila key/val) no encaja con la fila
 * LED+label ni con el boton toggle, y terminaria pisando el color del boton.
 * Layout fijo: child(0)=title, child(1)=fila LED+status label,
 *              child(2)=boton toggle (recoloreado via hmi_update_panel_retheme),
 *              child(3)=label info secundario (red configurada). */
static void theme_update_panel(lv_obj_t *panel) {
    if (!panel) return;
    theme_panel_title(panel);

    lv_obj_t *status_row = lv_obj_get_child(panel, 1);
    if (status_row) {
        lv_obj_t *status_lbl = lv_obj_get_child(status_row, 1);
        if (status_lbl) lv_obj_set_style_text_color(status_lbl, g_theme.txt_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    lv_obj_t *info_lbl = lv_obj_get_child(panel, 3);
    if (info_lbl) lv_obj_set_style_text_color(info_lbl, g_theme.txt_secondary, LV_PART_MAIN | LV_STATE_DEFAULT);

    hmi_update_panel_retheme();
}

/* ---- System Info navigation ---- */

static void sysinfo_set_nav_active(lv_obj_t *active_btn) {
    hmi_deactivate_dynamic_nav();
    lv_obj_t *btns[4] = {
        objects.sysinfo_btn_device,
        objects.sysinfo_btn_version,
        objects.sysinfo_btn_guide,
        objects.sysinfo_btn_update
    };
    for (int i = 0; i < 4; i++)
        apply_btn_style(btns[i], btns[i] == active_btn);
}

static void sysinfo_show_panel(lv_obj_t *panel) {
    lv_obj_t *panels[4] = {
        objects.sysinfo_content_device,
        objects.sysinfo_content_version,
        objects.sysinfo_content_guide,
        objects.sysinfo_content_update
    };
    for (int i = 0; i < 4; i++) {
        if (!panels[i]) continue;
        if (panels[i] == panel)
            lv_obj_remove_flag(panels[i], LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(panels[i], LV_OBJ_FLAG_HIDDEN);
    }
}

void action_sysinfo_btn_device(lv_event_t *e) {
    g_sysinfo_active_nav = objects.sysinfo_btn_device;
    sysinfo_set_nav_active(objects.sysinfo_btn_device);
    sysinfo_show_panel(objects.sysinfo_content_device);
}

void action_sysinfo_btn_version(lv_event_t *e) {
    g_sysinfo_active_nav = objects.sysinfo_btn_version;
    sysinfo_set_nav_active(objects.sysinfo_btn_version);
    sysinfo_show_panel(objects.sysinfo_content_version);
}

void action_sysinfo_btn_logs(lv_event_t *e) { /* movido a DEV mode */ }

void action_sysinfo_btn_guide(lv_event_t *e) {
    g_sysinfo_active_nav = objects.sysinfo_btn_guide;
    sysinfo_set_nav_active(objects.sysinfo_btn_guide);
    sysinfo_show_panel(objects.sysinfo_content_guide);
}

void action_sysinfo_btn_update(lv_event_t *e) {
    g_sysinfo_active_nav = objects.sysinfo_btn_update;
    sysinfo_set_nav_active(objects.sysinfo_btn_update);
    sysinfo_show_panel(objects.sysinfo_content_update);
}

/* ---- Settings navigation ---- */

static void settings_set_nav_active(lv_obj_t *active_btn) {
    lv_obj_t *btns[6] = {
        objects.settings_btn_brightness,
        objects.settings_btn_theme,
        objects.settings_btn_battery,
        objects.settings_btn_language,
        objects.settings_btn_user,
        objects.settings_btn_encoder
    };
    for (int i = 0; i < 6; i++)
        apply_btn_style(btns[i], btns[i] == active_btn);
}

static void settings_show_panel(lv_obj_t *panel) {
    lv_obj_t *panels[6] = {
        objects.settings_content_brightness,
        objects.settings_content_theme,
        objects.settings_content_battery,
        objects.settings_content_language,
        objects.settings_content_user,
        objects.settings_content_encoder
    };
    for (int i = 0; i < 6; i++) {
        if (!panels[i]) continue;
        if (panels[i] == panel)
            lv_obj_remove_flag(panels[i], LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(panels[i], LV_OBJ_FLAG_HIDDEN);
    }
}

static void lang_set_btn_active(lv_obj_t *active_btn) {
    lv_obj_t *btns[3] = { objects.lang_btn_es, objects.lang_btn_en, objects.lang_btn_pt };
    for (int i = 0; i < 3; i++)
        apply_btn_style(btns[i], btns[i] == active_btn);
}

void action_settings_btn_brightness(lv_event_t *e) {
    g_settings_active_nav = objects.settings_btn_brightness;
    settings_set_nav_active(objects.settings_btn_brightness);
    settings_show_panel(objects.settings_content_brightness);
}

void action_settings_btn_theme(lv_event_t *e) {
    g_settings_active_nav = objects.settings_btn_theme;
    settings_set_nav_active(objects.settings_btn_theme);
    settings_show_panel(objects.settings_content_theme);
}

void action_settings_btn_battery(lv_event_t *e) {
    g_settings_active_nav = objects.settings_btn_battery;
    settings_set_nav_active(objects.settings_btn_battery);
    settings_show_panel(objects.settings_content_battery);
}

void action_settings_btn_language(lv_event_t *e) {
    g_settings_active_nav = objects.settings_btn_language;
    settings_set_nav_active(objects.settings_btn_language);
    settings_show_panel(objects.settings_content_language);
}

void action_settings_btn_user(lv_event_t *e) {
    g_settings_active_nav = objects.settings_btn_user;
    settings_set_nav_active(objects.settings_btn_user);
    settings_show_panel(objects.settings_content_user);
}

void action_settings_btn_encoder(lv_event_t *e) {
    g_settings_active_nav = objects.settings_btn_encoder;
    settings_set_nav_active(objects.settings_btn_encoder);
    settings_show_panel(objects.settings_content_encoder);
}

void action_settings_user_edit_name(lv_event_t *e) {
    hmi_open_device_name_editor();
}

void action_settings_change_lock_pin(lv_event_t *e) {
    hmi_open_lock_pin_editor();
}

void action_settings_toggle_lock_pin(lv_event_t *e) {
    lv_obj_t *sw = lv_event_get_target(e);
    bool checked = lv_obj_has_state(sw, LV_STATE_CHECKED);
    /* Switch marcado = "habilitar contrasena" -> lock_enabled = checked */
    hmi_set_lock_enabled(checked);
}

/* ---- Language selector ---- */

void action_lang_es(lv_event_t *e) {
    lang_set(LANG_ES);
    lang_apply();
    lang_set_btn_active(objects.lang_btn_es);
}

void action_lang_en(lv_event_t *e) {
    lang_set(LANG_EN);
    lang_apply();
    lang_set_btn_active(objects.lang_btn_en);
}

void action_lang_pt(lv_event_t *e) {
    lang_set(LANG_PT);
    lang_apply();
    lang_set_btn_active(objects.lang_btn_pt);
}

/* ---- Theme selector ---- */

static void theme_set_active(lv_obj_t *active_btn) {
    lv_obj_t *btns[3] = {
        objects.settings_btn_theme_dark,
        objects.settings_btn_theme_classic,
        objects.settings_btn_theme_light
    };
    for (int i = 0; i < 3; i++)
        apply_btn_style(btns[i], btns[i] == active_btn);
}

/* ================================================================
 *  apply_theme  — builds palette then applies to every widget
 * ================================================================ */
static void apply_theme(int t) {

    /* ---- 1. Build palette ---- */
    if (t == 0) { /* Dark  (iOS dark) */
        g_theme.bg_screen       = lv_color_hex(0xff121212);
        g_theme.bg_topbar       = lv_color_hex(0xff1c1c1e);
        g_theme.bg_indicator    = lv_color_hex(0xff2c2c2e);
        g_theme.bg_card         = lv_color_hex(0xff1c1c1e);
        g_theme.bg_nav          = lv_color_hex(0xff161618);
        g_theme.bg_tabbar       = lv_color_hex(0xff1c1c1e);
        g_theme.bd_card         = lv_color_hex(0xff3a3a3c);
        g_theme.txt_primary     = lv_color_hex(0xfff2f2f7);
        g_theme.txt_secondary   = lv_color_hex(0xff636366);
        g_theme.txt_accent      = lv_color_hex(0xff0a84ff);
        g_theme.bg_btn_active   = lv_color_hex(0xff0a84ff);
        g_theme.txt_btn_active  = lv_color_hex(0xffffffff);
        g_theme.bd_btn_active   = lv_color_hex(0xff0a84ff);
        g_theme.bg_btn_inactive = lv_color_hex(0xff2c2c2e);
        g_theme.txt_btn_inactive= lv_color_hex(0xff8e8e93);
        g_theme.bd_btn_inactive = lv_color_hex(0xff48484a);
        g_theme.clr_slider      = lv_color_hex(0xff0a84ff);
        g_theme.clr_bar         = lv_color_hex(0xff30d158);
        g_theme.clr_bar_bg      = lv_color_hex(0xff3a3a3c);
    } else if (t == 1) { /* Classic — exact original screens.c values */
        g_theme.bg_screen       = lv_color_hex(0xff000000); /* tabview/screen bg = black (original) */
        g_theme.bg_topbar       = lv_color_hex(0xff393939); /* top bar original #393939             */
        g_theme.bg_indicator    = lv_color_hex(0xff1a1a1a); /* pill containers original              */
        g_theme.bg_card         = lv_color_hex(0xff1a1a1a); /* all cards original                   */
        g_theme.bg_nav          = lv_color_hex(0xff141414); /* nav columns slightly darker           */
        g_theme.bg_tabbar       = lv_color_hex(0xff1a1a1a); /* tab bar original                     */
        g_theme.bd_card         = lv_color_hex(0xff252525); /* all borders original                 */
        g_theme.txt_primary     = lv_color_hex(0xffffffff); /* white text original                  */
        g_theme.txt_secondary   = lv_color_hex(0xffaaaaaa); /* gray secondary original              */
        g_theme.txt_accent      = lv_color_hex(0xfff5c518); /* gold accent original                 */
        g_theme.bg_btn_active   = lv_color_hex(0xfff5c518); /* active button gold                   */
        g_theme.txt_btn_active  = lv_color_hex(0xff1a1a1a); /* dark text on gold                    */
        g_theme.bd_btn_active   = lv_color_hex(0xfff5c518);
        g_theme.bg_btn_inactive = lv_color_hex(0xff2a2a2a);
        g_theme.txt_btn_inactive= lv_color_hex(0xffaaaaaa);
        g_theme.bd_btn_inactive = lv_color_hex(0xff3c3c3c);
        g_theme.clr_slider      = lv_color_hex(0xfff5c518); /* brightness slider indicator gold      */
        g_theme.clr_bar         = lv_color_hex(0xff27ae60); /* robot battery bar green (original)   */
        g_theme.clr_bar_bg      = lv_color_hex(0xff2d2d2d); /* robot battery bar track (original)   */
    } else { /* Light  (iOS light) */
        g_theme.bg_screen       = lv_color_hex(0xfff2f2f7);
        g_theme.bg_topbar       = lv_color_hex(0xffe8e8ed);
        g_theme.bg_indicator    = lv_color_hex(0xffe5e5ea);
        g_theme.bg_card         = lv_color_hex(0xffffffff);
        g_theme.bg_nav          = lv_color_hex(0xffe8e8ed);
        g_theme.bg_tabbar       = lv_color_hex(0xffe8e8ed);
        g_theme.bd_card         = lv_color_hex(0xffc8c8cc);
        g_theme.txt_primary     = lv_color_hex(0xff1c1c1e);
        g_theme.txt_secondary   = lv_color_hex(0xff8e8e93);
        g_theme.txt_accent      = lv_color_hex(0xff007aff);
        g_theme.bg_btn_active   = lv_color_hex(0xff007aff);
        g_theme.txt_btn_active  = lv_color_hex(0xffffffff);
        g_theme.bd_btn_active   = lv_color_hex(0xff007aff);
        g_theme.bg_btn_inactive = lv_color_hex(0xffe5e5ea);
        g_theme.txt_btn_inactive= lv_color_hex(0xff636366);
        g_theme.bd_btn_inactive = lv_color_hex(0xffc8c8cc);
        g_theme.clr_slider      = lv_color_hex(0xff007aff);
        g_theme.clr_bar         = lv_color_hex(0xff34c759);
        g_theme.clr_bar_bg      = lv_color_hex(0xffe5e5ea);
    }
    g_current_theme = t;
    const hmi_theme_t *th = &g_theme;

    /* Shorthand macros */
    #define SET_CARD_PTR(p) do { \
        lv_obj_set_style_bg_color((p),     th->bg_card, LV_PART_MAIN | LV_STATE_DEFAULT); \
        lv_obj_set_style_border_color((p), th->bd_card, LV_PART_MAIN | LV_STATE_DEFAULT); \
    } while(0)
    #define SET_CARD(field)  SET_CARD_PTR(objects.field)
    #define TXT(field, clr)  lv_obj_set_style_text_color(objects.field, (clr), LV_PART_MAIN | LV_STATE_DEFAULT)

    /* ---- 2. Screen root ---- */
    lv_obj_set_style_bg_color(objects.main, th->bg_screen, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ---- 3. Tab containers ---- */
    lv_obj_set_style_bg_color(objects.tabview,      th->bg_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.general,      th->bg_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.paneo,        th->bg_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.configuracion,th->bg_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.sysinfo,      th->bg_screen, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ---- 4. Top bar ---- */
    lv_obj_set_style_bg_color(objects.obj0, th->bg_topbar, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(objects.obj2,     th->bg_indicator, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.obj2, th->bd_card,       LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.obj4,     th->bg_indicator,  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.obj4, th->bd_card,       LV_PART_MAIN | LV_STATE_DEFAULT);

    /* obj1 = logotipo Welltepp (imagen), ya no es texto — no necesita retheme */
    TXT(obj3,  th->txt_primary);  /* "Online"    */
    TXT(obj5,  th->txt_primary);  /* "Bluetooth" */
    TXT(console_voltage_percent_label, th->txt_primary);

    /* Console battery bar in top bar — gold in Classic (original), accent in other themes */
    {
        lv_color_t cbar_main = (t == 1) ? lv_color_hex(0xfff5c518) : th->bg_indicator;
        lv_color_t cbar_ind  = (t == 1) ? lv_color_hex(0xfff5c518) : th->clr_bar;
        lv_obj_set_style_bg_color(objects.console_voltage_percent_bar, cbar_main, LV_PART_MAIN      | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(objects.console_voltage_percent_bar, cbar_ind,  LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }

    /* ---- 5. Tab bar ---- */
    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(objects.tabview);
    lv_obj_set_style_bg_color(tab_bar,   th->bg_tabbar, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(tab_bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    uint32_t n_tabs = lv_obj_get_child_count(tab_bar);
    for (uint32_t i = 0; i < n_tabs; i++) {
        lv_obj_t *tb = lv_obj_get_child(tab_bar, i);
        /* Default (unselected) state */
        lv_obj_set_style_bg_color(tb,    th->bg_tabbar,  LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(tb, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(tb,  th->txt_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
        /* Checked (selected) state: soft accent tint + accent text + accent border
         * bg_opa controla la intensidad del tinte: 30=muy sutil, 50=suave, 80=moderado, 255=solido */
        lv_obj_set_style_bg_color(tb,    th->txt_accent,  LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(tb,  50,                  LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(tb,  th->txt_accent,  LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_border_color(tb, th->txt_accent, LV_PART_MAIN | LV_STATE_CHECKED);
        /* Pressed state: same soft tint to avoid gold flash */
        lv_obj_set_style_bg_color(tb,    th->txt_accent,  LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(tb,  50,                  LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_text_color(tb,  th->txt_accent,  LV_PART_MAIN | LV_STATE_PRESSED);
    }

    /* ---- 6. GENERAL tab ---- */
    /* Battery card */
    SET_CARD(obj6);
    TXT(obj7,  th->txt_accent);
    lv_obj_set_style_border_color(objects.obj7,  th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.robot_voltage_percent_bar,
                               th->clr_bar_bg, LV_PART_MAIN      | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.robot_voltage_percent_bar,
                               th->clr_bar,    LV_PART_INDICATOR  | LV_STATE_DEFAULT);
    TXT(obj8,           th->txt_primary);
    TXT(robot_voltage,  th->txt_primary);
    TXT(obj9,           th->txt_primary);
    TXT(console_voltage,th->txt_primary);

    /* Tilt card */
    SET_CARD(obj10);
    TXT(obj11, th->txt_accent);
    lv_obj_set_style_border_color(objects.obj11, th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    TXT(obj12, th->txt_secondary);  /* "Angle X" key label */
    TXT(obj13, th->txt_secondary);  /* "Angle Y" key label */
    /* angle_x / angle_y: accent-coloured display box (semi-transparent bg + border) */
    lv_obj_set_style_text_color(objects.angle_x,   th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.angle_x,     th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.angle_x, th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.angle_y,   th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.angle_y,     th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.angle_y, th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* LED Brightness card */
    SET_CARD(obj14);
    TXT(obj15, th->txt_accent);
    lv_obj_set_style_border_color(objects.obj15, th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (t == 1) { /* Classic: restore exact original slider colors */
        lv_obj_set_style_bg_color(objects.obj16,      lv_color_hex(0xff3c3c3c), LV_PART_MAIN      | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(objects.obj16,      lv_color_hex(0xfff5c518), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_color(objects.obj16, lv_color_hex(0xfff5c518), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(objects.obj16,      lv_color_hex(0xff3c3c3c), LV_PART_KNOB      | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_bg_color(objects.obj16,      th->clr_bar_bg, LV_PART_MAIN      | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(objects.obj16,      th->clr_slider, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_color(objects.obj16, th->clr_slider, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(objects.obj16,      th->txt_primary, LV_PART_KNOB     | LV_STATE_DEFAULT);
    }
    TXT(brightness_label, th->txt_accent); /* accent: gold in Classic, blue in Dark/Light */

    /* Encoder card */
    SET_CARD(obj17);
    TXT(obj18, th->txt_accent);
    lv_obj_set_style_border_color(objects.obj18, th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    /* encoder_value: same accent display box style as angle displays */
    lv_obj_set_style_text_color(objects.encoder_value,   th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.encoder_value,     th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.encoder_value, th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    /* RESET button: always "active" accent style (gold in Classic, blue in others) */
    apply_btn_style(objects.btn_encoder_reset, true);

    /* ---- 7. PANEO tab ---- */
    /* Neck card */
    SET_CARD(obj19);
    TXT(obj20, th->txt_accent);
    lv_obj_set_style_border_color(objects.obj20, th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    /* ± buttons: active accent style (gold bg in Classic, blue in Dark/Light) */
    apply_btn_style(objects.angle_neck_btn_decrese,  true);
    apply_btn_style(objects.angle_neck_btn_increased, true);
    /* angle_neck_label: accent display box */
    lv_obj_set_style_text_color(objects.angle_neck_label,   th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.angle_neck_label,     th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.angle_neck_label, th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Head card */
    SET_CARD(obj21);
    TXT(obj22, th->txt_accent);
    lv_obj_set_style_border_color(objects.obj22, th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    apply_btn_style(objects.angle_head_btn_decrese,  true);
    apply_btn_style(objects.angle_head_btn_increased, true);
    /* angle_head_label: accent display box */
    lv_obj_set_style_text_color(objects.angle_head_label,   th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.angle_head_label,     th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.angle_head_label, th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Action buttons */
    SET_CARD(btn_giro_automatico); SET_CARD(btn_start_demo);
    SET_CARD(btn_stop_demo);       SET_CARD(btn_center);
    TXT(obj23, th->txt_accent); TXT(obj24, th->txt_accent); TXT(obj25, th->txt_accent);
    { lv_obj_t *l = lv_obj_get_child(objects.btn_center, 0);
      if (l) lv_obj_set_style_text_color(l, th->txt_accent, LV_PART_MAIN | LV_STATE_DEFAULT); }

    /* ---- 8. SETTINGS tab ---- */
    /* Nav column */
    lv_obj_set_style_bg_color(objects.obj26,     th->bg_nav,  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.obj26, th->bd_card, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Settings nav buttons — re-apply active state for current panel */
    settings_set_nav_active(
        g_settings_active_nav ? g_settings_active_nav : objects.settings_btn_brightness
    );

    /* Content area */
    lv_obj_set_style_bg_color(objects.obj27,     th->bg_card, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.obj27, th->bd_card, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Brightness panel */
    theme_panel_title(objects.settings_content_brightness);  /* "BRIGHTNESS" title */
    /* Slider row container (obj28 has bg_opa=0, transparent — theme children) */
    { lv_obj_t *screen_lbl = lv_obj_get_child(objects.obj28, 0);
      if (screen_lbl) lv_obj_set_style_text_color(screen_lbl, th->txt_secondary, LV_PART_MAIN | LV_STATE_DEFAULT); }
    if (t == 1) { /* Classic: original console slider — all parts #3c3c3c */
        lv_obj_set_style_bg_color(objects.console_brightness_slider, lv_color_hex(0xff3c3c3c), LV_PART_MAIN      | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(objects.console_brightness_slider, lv_color_hex(0xff3c3c3c), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(objects.console_brightness_slider, lv_color_hex(0xff3c3c3c), LV_PART_KNOB      | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_bg_color(objects.console_brightness_slider, th->clr_bar_bg,  LV_PART_MAIN      | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(objects.console_brightness_slider, th->clr_slider,  LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(objects.console_brightness_slider, th->txt_primary, LV_PART_KNOB      | LV_STATE_DEFAULT);
    }
    /* console_brightness_label is accent (gold in Classic, blue in others) */
    TXT(console_brightness_label, th->txt_accent);

    /* Theme panel */
    theme_panel_title(objects.settings_content_theme);  /* "APPEARANCE" title */
    theme_set_active(
        t == 0 ? objects.settings_btn_theme_dark :
        t == 1 ? objects.settings_btn_theme_classic :
                 objects.settings_btn_theme_light
    );

    /* Battery panel */
    theme_panel_title(objects.settings_content_battery);  /* "BATTERY DISPLAY" title */
    apply_btn_style(objects.settings_btn_bat_voltage, !g_bat_display_percent);
    apply_btn_style(objects.settings_btn_bat_percent,  g_bat_display_percent);

    /* Language panel */
    theme_panel_title(objects.settings_content_language);  /* "LANGUAGE" title */
    {
        lv_obj_t *lang_btns[3] = { objects.lang_btn_es, objects.lang_btn_en, objects.lang_btn_pt };
        lang_set_btn_active(lang_btns[lang_get()]);
    }

    /* User panel */
    theme_user_panel(objects.settings_content_user);

    /* Encoder panel */
    theme_encoder_panel(objects.settings_content_encoder);

    /* ---- 9. SYSTEM INFO tab ---- */
    /* Nav column container (parent of the 4 sysinfo nav buttons) */
    lv_obj_t *si_nav_col = lv_obj_get_parent(objects.sysinfo_btn_device);
    if (si_nav_col) {
        lv_obj_set_style_bg_color(si_nav_col,     th->bg_nav,  LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(si_nav_col, th->bd_card, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    /* Re-apply nav button active state */
    sysinfo_set_nav_active(
        g_sysinfo_active_nav ? g_sysinfo_active_nav : objects.sysinfo_btn_device
    );

    /* Right content area — hard-coded dark bg in screens.c, must be overridden here */
    lv_obj_t *si_content_area = lv_obj_get_parent(objects.sysinfo_content_device);
    if (si_content_area) {
        lv_obj_set_style_bg_color(si_content_area,     th->bg_card, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(si_content_area, th->bd_card, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    /* Content panels — title accent + info rows + body labels */
    theme_info_panel(objects.sysinfo_content_device);
    theme_info_panel(objects.sysinfo_content_version);
    theme_info_panel(objects.sysinfo_content_guide);

    /* Update panel */
    theme_update_panel(objects.sysinfo_content_update);

    lang_apply();

    #undef SET_CARD_PTR
    #undef SET_CARD
    #undef TXT

    // Retheme the DEV panel if it is currently open (weak: no-op when DEV_MODE is off)
    hmi_dev_retheme();
}

void action_settings_theme_dark(lv_event_t *e) {
    apply_theme(0);
}

void action_settings_theme_classic(lv_event_t *e) {
    apply_theme(1);
}

void action_settings_theme_light(lv_event_t *e) {
    apply_theme(2);
}

/* ---- Battery display mode ---- */

void action_settings_bat_voltage(lv_event_t *e) {
    g_bat_display_percent = 0;
    apply_btn_style(objects.settings_btn_bat_voltage, true);
    apply_btn_style(objects.settings_btn_bat_percent,  false);
    if (objects.robot_voltage)
        lv_label_set_text(objects.robot_voltage, get_var_robot_voltage());
    if (objects.console_voltage)
        lv_label_set_text(objects.console_voltage, get_var_console_voltage());
}

void action_settings_bat_percent(lv_event_t *e) {
    g_bat_display_percent = 1;
    apply_btn_style(objects.settings_btn_bat_voltage, false);
    apply_btn_style(objects.settings_btn_bat_percent,  true);
    if (objects.robot_voltage)
        lv_label_set_text(objects.robot_voltage, get_var_robot_voltage_percent());
    if (objects.console_voltage)
        lv_label_set_text(objects.console_voltage, get_var_console_voltage_percent());
}

/* Public entry point — call once after create_screens() to boot the theme */
void hmi_theme_apply(int t) {
    apply_theme(t);
}

/* Tema activo actual (0=Dark, 1=Classic, 2=Light). Usado por el timer de
 * construccion diferida en ui.c para re-themear despues de crear los
 * paneles ocultos, sin asumir que sigue siendo el default de boot. */
int hmi_theme_current(void) {
    return g_current_theme;
}

/* Theme helpers — used by main.c DEV panel */
void       hmi_style_btn(lv_obj_t *btn, bool active) { apply_btn_style(btn, active); }
lv_color_t hmi_theme_accent(void)        { return g_theme.txt_accent; }
lv_color_t hmi_theme_txt_primary(void)   { return g_theme.txt_primary; }
lv_color_t hmi_theme_txt_secondary(void) { return g_theme.txt_secondary; }
lv_color_t hmi_theme_bg_btn_inactive(void)  { return g_theme.bg_btn_inactive; }
lv_color_t hmi_theme_bd_btn_inactive(void)  { return g_theme.bd_btn_inactive; }
lv_color_t hmi_theme_txt_btn_inactive(void) { return g_theme.txt_btn_inactive; }
lv_color_t hmi_theme_bg_btn_active(void)    { return g_theme.bg_btn_active; }
lv_color_t hmi_theme_bd_btn_active(void)    { return g_theme.bd_btn_active; }
lv_color_t hmi_theme_txt_btn_active(void)   { return g_theme.txt_btn_active; }

/* Weak stub: main.c overrides this when DEV_MODE is defined */
void __attribute__((weak)) hmi_dev_retheme(void) {}