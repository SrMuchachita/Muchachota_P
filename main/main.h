#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
#include "lvgl.h"

/*Exponer tipo de dato a otros archivos*/
typedef enum
{
    HMI_REG_ONLINE_INDICATOR    = 0x00, // En linea
    HMI_REG_BLUETOOTH_INDICATOR = 0x01, // Ble activo
    HMI_REG_ANGLE_HEAD_CHANGED  = 0x02, // Angle servo1 cabeza
    HMI_REG_ANGLE_NECK_CHANGED  = 0x03, // Angle servo2 cuello
    HMI_REG_ROBOT_LED_CHANGED   = 0x04, // PWM LED robot
    HMI_REG_GIRO_AUTOMATICO     = 0x05, // Giro Automatico
    HMI_REG_START_DEMO          = 0x06, // Modo Demo
    HMI_REG_STOP_DEMO           = 0x07, // Stop Demo
    HMI_REG_ENCODER             = 0x08, // Encoder Value
    HMI_REG_CONSOLE_VOLTAGE     = 0x09, // Voltage Console
    HMI_REG_ROBOT_VOLTAGE       = 0x0A, // Voltage Robot
    HMI_REG_ANGLE_X             = 0x0B, // Angle x Robot
    HMI_REG_ANGLE_Y             = 0x0C, // Angle y Robot
    HMI_REG_BLUETOOTH_PASSWORD  = 0x0D, // Password BLE
    HMI_REG_ROBOT_SERIAL        = 0x0E, // Numero de serie del robot
    HMI_REG_CENTER              = 0x0F, // Centrado: bits[31:16]=cuello, bits[15:0]=cabeza
    // 0x10, 0x11 reservados
    HMI_REG_JOY1                = 0x12, // Joystick 1: bits[31:16]=x, bits[15:0]=y
    HMI_REG_JOY2                = 0x13, // Joystick 2: bits[31:16]=x, bits[15:0]=y
    HMI_REG_BUTTONS             = 0x14, // Botones: bit1=J1, bit0=J2
    HMI_REG_PING                = 0x15, // HMI -> Consola: ping (value=1)
    HMI_REG_PONG                = 0x16, // Consola -> HMI: respuesta al ping (value=1)
    HMI_REG_P1                  = 0x17, // Joystick P1: un solo eje (0-4095), cada 200ms
    // 0x18, 0x19 reservados
    HMI_REG_MOTOR               = 0x1A, // Motor: bits[31:16]=cmd, bits[15:0]=vel (0-1000)
    HMI_REG_SRV1_ANGLE          = 0x1B, // Angulo servo1 cabeza (0-180°)
    HMI_REG_SRV2_ANGLE          = 0x1C, // Angulo servo2 cuello (0-180°)
    HMI_REG_SRV3_ANGLE          = 0x1D, // Angulo servo3 (0-270°)
    HMI_CMD_MAX
} hmi_reg_t;

/*Exponer funciones de main.c
  Para que otros archivos tengan acceso*/
void hmi_send_data(uint8_t reg, int32_t value);
void lcd_set_brightness(int brightness_percent);
void hmi_deactivate_dynamic_nav(void);
void hmi_open_device_name_editor(void);
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

// 0 = mostrar voltaje (V), 1 = mostrar porcentaje (%)
extern uint8_t g_bat_display_percent;

// Actualiza el valor crudo de pulsos del encoder y refresca su label en
// GENERAL CONTROLS respetando el modo de visualizacion actual (pulsos/metros).
void hmi_encoder_set_raw(int32_t raw_pulses);

// Callbacks de los widgets de Settings>Encoder y SystemInfo>Update — los
// widgets ahora se crean en ui/screens.c (junto con los demas submenus),
// pero la logica vive en main.c, asi que screens.c los referencia por nombre.
void enc_sb_increment_cb(lv_event_t *e);
void enc_sb_decrement_cb(lv_event_t *e);
void enc_preset_cb(lv_event_t *e);
void update_wifi_toggle_cb(lv_event_t *e);

// Carga la calibracion del encoder desde NVS en los spinboxes de
// Settings>Encoder. Debe llamarse despues de que esos spinboxes existan (ver
// create_panel_settings_encoder() en screens.c, y la construccion en etapas
// en app_main).
void enc_settings_load_from_nvs(void);

#endif