/*
 * STUB de main.h para el simulador de PC (sim/).
 *
 * Esto NO es el main.h real del firmware (ver main/main.h) — es una copia
 * con las MISMAS firmas de funcion, pero sin implementacion real de
 * hardware (GPIO/UART/NVS/buzzer/etc). Existe para que screens.c/actions.c/
 * ui.c (que si son 100% reusables) compilen sin tocarlos.
 *
 * Si agregas una funcion nueva al main.h real que actions.c/screens.c
 * necesiten, hay que agregarla tambien aca (y su cuerpo vacio en
 * main_stub.c) para que el simulador siga compilando.
 */

#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
#include <stdint.h>
#include "lvgl.h"

typedef enum
{
    HMI_REG_ONLINE_INDICATOR    = 0x00,
    HMI_REG_BLUETOOTH_INDICATOR = 0x01,
    HMI_REG_ANGLE_HEAD_CHANGED  = 0x02,
    HMI_REG_ANGLE_NECK_CHANGED  = 0x03,
    HMI_REG_ROBOT_LED_CHANGED   = 0x04,
    HMI_REG_GIRO_AUTOMATICO     = 0x05,
    HMI_REG_START_DEMO          = 0x06,
    HMI_REG_STOP_DEMO           = 0x07,
    HMI_REG_ENCODER             = 0x08,
    HMI_REG_CONSOLE_VOLTAGE     = 0x09,
    HMI_REG_ROBOT_VOLTAGE       = 0x0A,
    HMI_REG_ANGLE_X             = 0x0B,
    HMI_REG_ANGLE_Y             = 0x0C,
    HMI_REG_BLUETOOTH_PASSWORD  = 0x0D,
    HMI_REG_ROBOT_SERIAL        = 0x0E,
    HMI_REG_CENTER              = 0x0F,
    HMI_REG_SRV1_LIMITS         = 0x10,
    HMI_REG_SRV2_LIMITS         = 0x11,
    HMI_REG_JOY1                = 0x12,
    HMI_REG_JOY2                = 0x13,
    HMI_REG_BUTTONS             = 0x14,
    HMI_REG_PING                = 0x15,
    HMI_REG_PONG                = 0x16,
    HMI_REG_P1                  = 0x17,
    HMI_REG_SRV3_LIMITS         = 0x19,
    HMI_REG_MOTOR               = 0x1A,
    HMI_REG_SRV1_ANGLE          = 0x1B,
    HMI_REG_SRV2_ANGLE          = 0x1C,
    HMI_REG_SRV3_ANGLE          = 0x1D,
    HMI_REG_OTA_STATUS          = 0x1E,
    HMI_REG_FW_VERSION          = 0x1F,
    HMI_REG_WIFI_STATUS         = 0x20,
    HMI_REG_VIDEO_TECH          = 0x21,
    HMI_REG_VIDEO_TECH_B        = 0x22,
    HMI_REG_WIFI_ENABLE         = 0x23,
    HMI_REG_RL1                 = 0x24,
    HMI_REG_ROBOT_MODEL         = 0x25,
    HMI_REG_RL2                 = 0x26,
    HMI_REG_J1_BUTTON_MODE      = 0x27,
    HMI_REG_J2_BUTTON_MODE      = 0x28,
    HMI_REG_BLUETOOTH_MAC_HI    = 0x29,
    HMI_REG_BLUETOOTH_MAC_LO    = 0x2A,
    HMI_REG_BLUETOOTH_DISCONNECT = 0x2B,
    HMI_REG_BLUETOOTH_BLOCK      = 0x2C,
    HMI_REG_BLUETOOTH_UNBLOCK_ALL = 0x2D,
    HMI_REG_J1_CAPTURE_EVENT    = 0x2E,
    HMI_REG_J2_CAPTURE_EVENT    = 0x2F,
    HMI_CMD_MAX
} hmi_reg_t;

void hmi_send_data(uint8_t reg, int32_t value);
void lcd_set_brightness(int brightness_percent);
void buzzer_nav_beep(void);
void buzzer_sound_3(void);
void buzzer_sound_17(void);
void hmi_deactivate_dynamic_nav(void);
void hmi_open_device_name_editor(void);
void hmi_open_wifi_editor(void);
void hmi_open_lock_pin_editor(void);
void hmi_set_lock_enabled(bool enabled);
bool hmi_get_lock_enabled(void);

typedef enum {
    LOG_OK   = 0,
    LOG_ERR  = 1,
    LOG_RX   = 2,
    LOG_TX   = 3,
    LOG_WARN = 4,
} log_type_t;

void hmi_log(log_type_t type, const char *msg);

extern uint8_t g_bat_display_percent;
extern uint8_t g_battery_alert_enabled;

void hmi_encoder_set_raw(int32_t raw_pulses);
void hmi_encoder_reset(void);
void hmi_bigview_led_level_refresh(void);
void hmi_demo_start(void);
void hmi_demo_stop(void);

void enc_sb_increment_cb(lv_event_t *e);
void enc_sb_decrement_cb(lv_event_t *e);
void enc_preset_cb(lv_event_t *e);
void update_wifi_toggle_cb(lv_event_t *e);
void update_confirm_cb(lv_event_t *e);
void console_wifi_toggle_cb(lv_event_t *e);

void enc_settings_load_from_nvs(void);

void hmi_ui_prefs_save_theme(int theme_idx);
void hmi_ui_prefs_save_lang(int lang_id);
void hmi_ui_prefs_save_bat_display(int percent);
void hmi_ui_prefs_save_battery_alert(int enabled);
int  hmi_ui_prefs_load_theme(void);
int  hmi_ui_prefs_load_lang(void);
int  hmi_ui_prefs_load_bat_display(void);
int  hmi_ui_prefs_load_battery_alert(void);

void hmi_auto_rotation_init(void);

void modes_btn_control_por_puntos_cb(lv_event_t *e);
void modes_btn_config_auto_rotation_cb(lv_event_t *e);
void modes_btn_giro_automatico_cb(lv_event_t *e);
void modes_btn_stop_giro_automatico_cb(lv_event_t *e);

void cp_btn_guardar_centrado_cb(lv_event_t *e);
void cp_btn_volver_cb(lv_event_t *e);

// Control Digital — joystick tactil de cabeza/cuello (MODES)
void modes_btn_control_digital_cb(lv_event_t *e);
void cd_btn_volver_cb(lv_event_t *e);
void cd_joystick_event_cb(lv_event_t *e);
void cd_btn_centrar_cb(lv_event_t *e);
// Multitouch (GT911): todos los dedos en coordenadas de pantalla. En el
// simulador no hay multitouch (supported = false, 0 puntos).
bool hmi_touch_multi_supported(void);
uint8_t hmi_touch_get_points(lv_point_t *pts, uint8_t max);
void cd_motor_event_cb(lv_event_t *e);   // joystick del motor (HMI_REG_MOTOR)
void hmi_control_digital_motor_label_refresh(void); // "Motor: <estado>" (lo llama lang.c al cambiar de idioma)

void autorot_btn_recorrido1_cb(lv_event_t *e);
void autorot_btn_recorrido2_cb(lv_event_t *e);
void autorot_picker_btn_volver_cb(lv_event_t *e);

void ar_angle_neck_dec_cb(lv_event_t *e);
void ar_angle_neck_inc_cb(lv_event_t *e);
void ar_angle_head_dec_cb(lv_event_t *e);
void ar_angle_head_inc_cb(lv_event_t *e);
void ar_btn_guardar_punto_cb(lv_event_t *e);
void ar_btn_eliminar_punto_cb(lv_event_t *e);
void ar_speed_dec_cb(lv_event_t *e);
void ar_speed_inc_cb(lv_event_t *e);
void ar_btn_probar_cb(lv_event_t *e);
void ar_btn_volver_cb(lv_event_t *e);
void ar_point_row_clicked_cb(lv_event_t *e);

void sl_limit_btn_cb(lv_event_t *e);
void sl_btn_guardar_cb(lv_event_t *e);
void hmi_srv_limits_load_to_ui(void);

#endif
