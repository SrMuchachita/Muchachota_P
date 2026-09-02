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
    lv_obj_t *robot_model_pill;
    lv_obj_t *led_robot_model;
    lv_obj_t *robot_model_label;
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
    lv_obj_t *robot_voltage_caption;
    lv_obj_t *robot_voltage;
    lv_obj_t *console_voltage_caption;
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
    lv_obj_t *modes_neck_panel;
    lv_obj_t *modes_head_panel;
    lv_obj_t *modes_grid_panel;
    lv_obj_t *btn_control_por_puntos;
    lv_obj_t *btn_config_auto_rotation;
    lv_obj_t *btn_stop_giro_automatico;
    // Control por Puntos (pantalla completa, dentro de MODES) — reutiliza
    // los paneles angle_neck/angle_head ORIGINALES via reparent, no crea
    // copias nuevas (ver create_panel_modes_auto_rotation en screens.c).
    // Barra superior fija (titulo + Volver) para que el boton de salir
    // siempre este a la vista, sin depender de scroll.
    lv_obj_t *modes_control_puntos_panel;
    lv_obj_t *cp_title_label;
    lv_obj_t *cp_btn_guardar_centrado;
    lv_obj_t *cp_btn_volver;
    // Config Auto Rotation — selector de recorrido (pantalla completa)
    lv_obj_t *modes_autorot_picker_panel;
    lv_obj_t *autorot_picker_title_label;
    lv_obj_t *autorot_picker_subtitle_label;
    lv_obj_t *autorot_btn_recorrido1;
    lv_obj_t *autorot_btn_recorrido2;
    lv_obj_t *autorot_picker_btn_volver;
    // Config Auto Rotation — editor de puntos (pantalla completa)
    lv_obj_t *modes_autorot_editor_panel;
    lv_obj_t *ar_title_label;
    lv_obj_t *ar_btn_volver;
    lv_obj_t *ar_points_caption_label;
    lv_obj_t *ar_points_list;
    lv_obj_t *ar_telemetry_neck_label;
    lv_obj_t *ar_telemetry_head_label;
    lv_obj_t *ar_speed_card;
    lv_obj_t *ar_speed_caption_label;
    lv_obj_t *ar_speed_btn_decrease;
    lv_obj_t *ar_speed_label;
    lv_obj_t *ar_speed_btn_increase;
    lv_obj_t *ar_head_card;
    lv_obj_t *ar_head_caption_label;
    lv_obj_t *ar_angle_head_btn_decrese;
    lv_obj_t *ar_angle_head_label;
    lv_obj_t *ar_angle_head_btn_increased;
    lv_obj_t *ar_neck_card;
    lv_obj_t *ar_neck_caption_label;
    lv_obj_t *ar_angle_neck_btn_decrese;
    lv_obj_t *ar_angle_neck_label;
    lv_obj_t *ar_angle_neck_btn_increased;
    lv_obj_t *ar_btn_guardar_punto;
    lv_obj_t *ar_btn_eliminar_punto;
    lv_obj_t *ar_btn_probar;
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
    lv_obj_t *update_available_btn;
    lv_obj_t *update_duration_note;
    lv_obj_t *update_network_label;
    lv_obj_t *update_wifi_edit_btn;
    lv_obj_t *update_wifi_network_row;
    // System Info > Update — estado de la consola (recibido por UART, no
    // confundir con update_* de arriba que es el WiFi/OTA de la PANTALLA)
    lv_obj_t *console_ota_led;
    lv_obj_t *console_ota_status_label;
    lv_obj_t *console_wifi_led;
    lv_obj_t *console_wifi_toggle_btn;
    lv_obj_t *sysinfo_robot_serial_value;
    lv_obj_t *sysinfo_console_serial_value;
    lv_obj_t *sysinfo_console_fw_value;
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
    lv_obj_t *settings_btn_tecnologia;
    lv_obj_t *settings_content_tecnologia;
    lv_obj_t *tech_btn_a;
    lv_obj_t *tech_btn_b;
    lv_obj_t *settings_btn_camera;
    lv_obj_t *settings_content_camera;
    lv_obj_t *cam_reverse_led;
    lv_obj_t *cam_reverse_label;
    lv_obj_t *cam_forward_led;
    lv_obj_t *cam_forward_label;
    lv_obj_t *cam_rl1_btn_on;
    lv_obj_t *cam_rl1_btn_off;
    lv_obj_t *cam_rl1_btn_auto;
    lv_obj_t *settings_btn_bluetooth;
    lv_obj_t *settings_content_bluetooth;
    lv_obj_t *bt_panel_led;
    lv_obj_t *bt_panel_status_label;
    lv_obj_t *bt_panel_mac_caption;
    lv_obj_t *bt_panel_mac_label;
    lv_obj_t *bt_panel_password_caption;
    lv_obj_t *bt_panel_password_label;
    lv_obj_t *bt_panel_disconnect_btn;
    lv_obj_t *bt_panel_block_btn;
    lv_obj_t *bt_panel_unblock_all_btn;
    lv_obj_t *settings_btn_joystick;
    lv_obj_t *settings_content_joystick;
    lv_obj_t *j1_mode_btn_servo3;
    lv_obj_t *j1_mode_btn_led;
    lv_obj_t *j1_mode_btn_center;
    lv_obj_t *j1_mode_btn_capture;
    lv_obj_t *j2_mode_btn_servo3;
    lv_obj_t *j2_mode_btn_led;
    lv_obj_t *j2_mode_btn_center;
    lv_obj_t *j2_mode_btn_capture;
    lv_obj_t *settings_wifi_ssid_value;
    lv_obj_t *tech_desc_a;
    lv_obj_t *tech_desc_b;
    lv_obj_t *settings_btn_srv_limits;
    lv_obj_t *settings_content_srv_limits;
    lv_obj_t *sl_srv1_min_btn_decrese;
    lv_obj_t *sl_srv1_min_label;
    lv_obj_t *sl_srv1_min_btn_increased;
    lv_obj_t *sl_srv1_max_btn_decrese;
    lv_obj_t *sl_srv1_max_label;
    lv_obj_t *sl_srv1_max_btn_increased;
    lv_obj_t *sl_srv2_min_btn_decrese;
    lv_obj_t *sl_srv2_min_label;
    lv_obj_t *sl_srv2_min_btn_increased;
    lv_obj_t *sl_srv2_max_btn_decrese;
    lv_obj_t *sl_srv2_max_label;
    lv_obj_t *sl_srv2_max_btn_increased;
    lv_obj_t *sl_srv3_min_btn_decrese;
    lv_obj_t *sl_srv3_min_label;
    lv_obj_t *sl_srv3_min_btn_increased;
    lv_obj_t *sl_srv3_max_btn_decrese;
    lv_obj_t *sl_srv3_max_label;
    lv_obj_t *sl_srv3_max_btn_increased;
    lv_obj_t *sl_btn_guardar;
    lv_obj_t *ota_progress_overlay;
    lv_obj_t *ota_progress_label;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();
// Paneles ocultos por defecto (ninguno se ve hasta tocar su boton de nav):
// se llaman todos seguidos, de un tiron, desde ui_init() (ui.c) — la unica
// arquitectura de arranque confirmada estable (ver boot_deferred_construction
// en la memoria del proyecto: cualquier intento de escalonarlos con locks o
// timers separados termino en pantalla negra trabada).
void create_panel_settings_encoder();
void create_panel_sysinfo_update();
void create_panel_settings_theme_battery_lang_user();
void create_panel_sysinfo_version_guide();
void create_panel_settings_tecnologia();
void create_panel_settings_camera();
void create_panel_settings_bluetooth();
void create_panel_settings_joystick();
void create_panel_modes_auto_rotation();
void create_panel_settings_srv_limits();
// Overlay pantalla completa "Actualizando..." mostrado durante la descarga
// OTA (ver ota_before_download_cb en main.c). Oculto por defecto.
void create_ota_progress_overlay();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/