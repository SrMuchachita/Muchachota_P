#pragma once
#include <lvgl.h>

typedef struct {
    /* Tab bar */
    const char *tab_general;
    const char *tab_modes;
    const char *tab_settings;
    const char *tab_sysinfo;
    /* General Controls - section titles */
    const char *sec_battery;
    const char *sec_tilt;
    const char *sec_led_brightness;
    const char *sec_encoder;
    /* General Controls - field labels */
    const char *lbl_robot_voltage;
    const char *lbl_console_voltage;
    const char *lbl_angle_x;
    const char *lbl_angle_y;
    /* Modes - section titles */
    const char *sec_neck;
    const char *sec_head;
    /* Modes - action buttons */
    const char *btn_auto_rotation;
    const char *btn_start_demo;
    const char *btn_stop_demo;
    const char *btn_center;
    const char *btn_reset;
    /* Settings - nav buttons */
    const char *nav_brightness;
    const char *nav_theme;
    const char *nav_battery;
    const char *nav_language;
    /* Settings - content titles */
    const char *title_brightness;
    const char *title_appearance;
    const char *title_battery_display;
    const char *title_language;
    /* Settings - Brightness */
    const char *lbl_screen;
    /* Settings - Theme buttons */
    const char *btn_dark;
    const char *btn_classic;
    const char *btn_light;
    /* Settings - Battery buttons */
    const char *btn_voltage;
    const char *btn_percent;
    /* Sysinfo - nav buttons */
    const char *nav_device;
    const char *nav_version;
    const char *nav_logs;
    const char *nav_guide;
    /* Sysinfo - content titles */
    const char *title_device_info;
    const char *title_version_info;
    const char *title_system_logs;
    const char *title_quick_guide;
    /* Sysinfo - guide body text */
    const char *guide_body;
    /* Battery alerts */
    const char *alert_battery_critical;
    const char *alert_shutdown;
    /* Settings - User panel */
    const char *nav_user;
    const char *title_user;
    const char *lbl_name;
    const char *btn_change_name;
    const char *btn_change_pin;
    const char *lbl_enable_password;
    /* Device name editor dialog */
    const char *title_device_name_editor;
    const char *btn_save;
    const char *btn_cancel;
    /* PIN dialog (shared: user "change PIN" + dev-mode unlock) */
    const char *title_change_pin;
    const char *title_dev_mode;
    const char *sub_change_pin;
    const char *sub_enter_pin;
} lang_strings_t;

typedef enum {
    LANG_ES = 0,
    LANG_EN = 1,
    LANG_PT = 2,
    LANG_COUNT
} lang_id_t;

extern const lang_strings_t *g_lang;

void      lang_set(lang_id_t id);
lang_id_t lang_get(void);
void      lang_apply(void);
