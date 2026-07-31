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
    const char *btn_reset;
    /* Modes - grilla: Control por Puntos / Config Auto Rotation */
    const char *btn_control_puntos;
    const char *btn_config_auto_rotation;
    const char *btn_stop_auto_rotation;
    const char *btn_pausar_auto_rotation;
    const char *btn_reanudar_auto_rotation;
    const char *title_control_puntos;
    const char *lbl_neck_servo2;
    const char *lbl_head_servo1;
    const char *btn_guardar_centrado;
    const char *btn_volver;
    const char *title_config_auto_rotation;
    const char *sub_elegir_recorrido;
    const char *btn_recorrido1;
    const char *btn_recorrido2;
    const char *btn_guardar_punto;
    const char *lbl_velocidad;
    const char *btn_probar;
    const char *btn_detener_prueba;
    /* Settings - Servo Limits */
    const char *nav_srv_limits;
    const char *title_srv_limits;
    const char *desc_srv_limits;
    const char *lbl_minimo;
    const char *lbl_maximo;
    const char *btn_guardar_limites;
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
    /* Lock screen (login) + its own "new lock PIN" editor */
    const char *title_new_lock_pin;
    const char *sub_lock_pin_digits;
    const char *lbl_lock_greeting_prefix; /* placeholder: %s = nombre del dispositivo */
    const char *lbl_lock_signing_in;
    const char *lbl_lock_access_granted;
    const char *lbl_lock_pin_incorrect;
    const char *lbl_lock_tap_to_enter;
    /* Settings - Encoder panel */
    const char *nav_encoder;
    const char *title_encoder;
    const char *desc_encoder;
    const char *lbl_perimeter;
    const char *lbl_pulses_rev;
    const char *unit_pulses_rev;
    const char *fmt_encoder_footer; /* placeholders: %.3f mm-per-pulse, %.1f pulses-per-m, %.2f perim-mm, %ld ppr */
    /* Settings - Technology panel */
    const char *nav_technology;
    const char *title_technology;
    const char *desc_technology;
    const char *desc_tech_a;
    const char *desc_tech_b;
    /* Sysinfo - Update panel */
    const char *nav_update;
    const char *title_update;
    const char *lbl_wifi_off;
    const char *lbl_connecting;
    const char *lbl_reconnecting;
    const char *lbl_connected_prefix;
    const char *btn_enable_wifi;
    const char *btn_disable_wifi;
    const char *lbl_updating;
    const char *lbl_network_prefix;
    const char *lbl_update_duration_note;
    /* General Controls - Encoder card feet/meters toggle */
    const char *btn_feet;
    const char *btn_meters;
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

// Definido en main.c: refresca textos que dependen de estado que solo main.c
// conoce (boton/estado de WiFi en el panel Update, toggle Pulsos/Metros del
// dashboard, formula del footer de Encoder) cuando cambia el idioma.
// Llamado al final de lang_apply().
void hmi_extra_panels_apply_lang(void);
