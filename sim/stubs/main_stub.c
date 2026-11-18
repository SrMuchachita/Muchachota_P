/*
 * Implementaciones vacias de las funciones declaradas en stubs/main.h.
 * No hablan con hardware real (no hay GPIO/UART/NVS/buzzer en la PC) —
 * solo existen para que screens.c/actions.c/ui.c (compilados tal cual,
 * sin editar) tengan algo a que enlazar.
 */

#include "main.h"
#include "lang.h"
#include <stdio.h>

// Declarada en ui/lang.h (no en main.h), implementada en el main.c real —
// re-traduce paneles "extra" segun idioma. No-op en el simulador.
void hmi_extra_panels_apply_lang(void) {}

uint8_t g_bat_display_percent = 0;
uint8_t g_battery_alert_enabled = 1; // 1=alerta habilitada (default) — ver apply_robot_battery_mv() en bigview_port.c

// hmi_send_data() SI tiene implementacion real — ver stubs/robot_panel.c
// (reenvia los registros que le corresponden al robot real por Modbus/RS485).
void lcd_set_brightness(int brightness_percent) { (void)brightness_percent; }
void buzzer_nav_beep(void) {}
void buzzer_sound_3(void) {}
void buzzer_sound_17(void) {}
void hmi_deactivate_dynamic_nav(void) {}
void hmi_open_device_name_editor(void) {}
void hmi_open_wifi_editor(void) {}
void hmi_open_lock_pin_editor(void) {}
void hmi_set_lock_enabled(bool enabled) { (void)enabled; }
bool hmi_get_lock_enabled(void) { return false; }

extern void dev_logs_feed(const char *msg); // ver stubs/dev_mode_port.c (panel Logs de DEV MODE)

void hmi_log(log_type_t type, const char *msg)
{
    (void)type;
    printf("[hmi_log] %s\n", msg ? msg : "");
    dev_logs_feed(msg);
}

// hmi_encoder_set_raw / hmi_encoder_reset / hmi_bigview_led_level_refresh:
// implementadas de verdad en stubs/bigview_port.c (parte del port del
// Panel de Manejo), no aca.

void enc_sb_increment_cb(lv_event_t *e) { (void)e; }
void enc_sb_decrement_cb(lv_event_t *e) { (void)e; }
void enc_preset_cb(lv_event_t *e) { (void)e; }
void update_wifi_toggle_cb(lv_event_t *e) { (void)e; }
void update_confirm_cb(lv_event_t *e) { (void)e; }
void console_wifi_toggle_cb(lv_event_t *e) { (void)e; }

void enc_settings_load_from_nvs(void) {}

void hmi_ui_prefs_save_theme(int theme_idx) { (void)theme_idx; }
void hmi_ui_prefs_save_lang(int lang_id) { (void)lang_id; }
void hmi_ui_prefs_save_bat_display(int percent) { (void)percent; }
void hmi_ui_prefs_save_battery_alert(int enabled) { (void)enabled; }
int  hmi_ui_prefs_load_theme(void) { return 1; } // Classic
int  hmi_ui_prefs_load_lang(void) { return 0; }  // ES
int  hmi_ui_prefs_load_bat_display(void) { return 0; }
int  hmi_ui_prefs_load_battery_alert(void) { return 1; } // habilitada por default

// hmi_auto_rotation_init / modes_btn_* / cp_btn_* / autorot_*_cb / ar_*_cb:
// implementadas de verdad en stubs/modes_port.c (port de MODOS), no aca.

void sl_limit_btn_cb(lv_event_t *e) { (void)e; }
void sl_btn_guardar_cb(lv_event_t *e) { (void)e; }
void hmi_srv_limits_load_to_ui(void) {}
