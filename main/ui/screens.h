#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    _SCREEN_ID_LAST = 1
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *led_online;
    lv_obj_t *obj3;
    lv_obj_t *console_voltage_percent_label;
    lv_obj_t *console_voltage_percent_bar;
    lv_obj_t *obj4;
    lv_obj_t *led_bluetooth;
    lv_obj_t *obj5;
    lv_obj_t *tabview;
    lv_obj_t *general;
    lv_obj_t *obj6;
    lv_obj_t *obj7;
    lv_obj_t *robot_voltage_percent_bar;
    lv_obj_t *robot_battery_alert;
    lv_obj_t *obj8;
    lv_obj_t *robot_voltage;
    lv_obj_t *obj9;
    lv_obj_t *console_voltage;
    lv_obj_t *obj10;
    lv_obj_t *obj11;
    lv_obj_t *obj12;
    lv_obj_t *obj13;
    lv_obj_t *angle_x;
    lv_obj_t *angle_y;
    lv_obj_t *obj14;
    lv_obj_t *obj15;
    lv_obj_t *obj16;
    lv_obj_t *brightness_label;
    lv_obj_t *obj17;
    lv_obj_t *obj18;
    lv_obj_t *encoder_value;
    lv_obj_t *btn_encoder_reset;
    lv_obj_t *paneo;
    lv_obj_t *obj19;
    lv_obj_t *obj20;
    lv_obj_t *angle_neck_btn_decrese;
    lv_obj_t *angle_neck_label;
    lv_obj_t *angle_neck_btn_increased;
    lv_obj_t *obj21;
    lv_obj_t *obj22;
    lv_obj_t *angle_head_btn_decrese;
    lv_obj_t *angle_head_label;
    lv_obj_t *angle_head_btn_increased;
    lv_obj_t *btn_giro_automatico;
    lv_obj_t *obj23;
    lv_obj_t *btn_start_demo;
    lv_obj_t *obj24;
    lv_obj_t *btn_stop_demo;
    lv_obj_t *obj25;
    lv_obj_t *btn_center;
    lv_obj_t *configuracion;
    lv_obj_t *obj26;
    lv_obj_t *bluetooth_password_label;
    lv_obj_t *obj27;
    lv_obj_t *console_brightness_label;
    lv_obj_t *console_brightness_slider;
    lv_obj_t *obj28;
    lv_obj_t *serial_number;
    lv_obj_t *robot_serial_number;
    lv_obj_t *sysinfo;
    lv_obj_t *sysinfo_btn_device;
    lv_obj_t *sysinfo_btn_version;
    lv_obj_t *sysinfo_btn_logs;
    lv_obj_t *sysinfo_btn_guide;
    lv_obj_t *sysinfo_content_device;
    lv_obj_t *sysinfo_content_version;
    lv_obj_t *sysinfo_content_logs;
    lv_obj_t *sysinfo_logs_body;
    lv_obj_t *sysinfo_content_guide;
    lv_obj_t *sysinfo_btn_update;
    lv_obj_t *sysinfo_content_update;
    lv_obj_t *update_led;
    lv_obj_t *update_status_label;
    lv_obj_t *update_toggle_btn;
    lv_obj_t *sysinfo_robot_serial_value;
    lv_obj_t *sysinfo_console_serial_value;
    lv_obj_t *sysinfo_device_name_value;
    lv_obj_t *settings_btn_brightness;
    lv_obj_t *settings_btn_theme;
    lv_obj_t *settings_btn_battery;
    lv_obj_t *settings_content_brightness;
    lv_obj_t *settings_content_theme;
    lv_obj_t *settings_content_battery;
    lv_obj_t *settings_btn_theme_dark;
    lv_obj_t *settings_btn_theme_classic;
    lv_obj_t *settings_btn_theme_light;
    lv_obj_t *settings_btn_bat_voltage;
    lv_obj_t *settings_btn_bat_percent;
    lv_obj_t *settings_btn_language;
    lv_obj_t *settings_content_language;
    lv_obj_t *lang_btn_es;
    lv_obj_t *lang_btn_en;
    lv_obj_t *lang_btn_pt;
    lv_obj_t *settings_btn_user;
    lv_obj_t *settings_content_user;
    lv_obj_t *settings_user_name_value;
    lv_obj_t *settings_user_pin_switch;
    lv_obj_t *settings_btn_encoder;
    lv_obj_t *settings_content_encoder;
    lv_obj_t *enc_spinbox_perim;
    lv_obj_t *enc_spinbox_ppr;
    lv_obj_t *enc_footer_label;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();
// Paneles ocultos por defecto (Encoder/Update/Theme/Battery/Language/User/
// Version/Guide): se llaman en 4 pasos separados desde app_main (main.c),
// cada uno en su propio ciclo corto de esp_lv_adapter_lock()/unlock() +
// vTaskDelay() real entre uno y otro, para no bloquear la tarea de render de
// LVGL (ni disparar el watchdog de IDLE0) con un solo bloque de ~9s.
void create_panel_settings_encoder();
void create_panel_sysinfo_update();
void create_panel_settings_theme_battery_lang_user();
void create_panel_sysinfo_version_guide();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/