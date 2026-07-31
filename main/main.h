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
    HMI_REG_SRV1_LIMITS         = 0x10, // Limites servo1 cabeza: bits[31:16]=max, bits[15:0]=min
    HMI_REG_SRV2_LIMITS         = 0x11, // Limites servo2 cuello: bits[31:16]=max, bits[15:0]=min
    HMI_REG_JOY1                = 0x12, // Joystick 1: bits[31:16]=x, bits[15:0]=y
    HMI_REG_JOY2                = 0x13, // Joystick 2: bits[31:16]=x, bits[15:0]=y
    HMI_REG_BUTTONS             = 0x14, // Botones: bit1=J1, bit0=J2
    HMI_REG_PING                = 0x15, // HMI -> Consola: ping (value=1)
    HMI_REG_PONG                = 0x16, // Consola -> HMI: respuesta al ping (value=1)
    HMI_REG_P1                  = 0x17, // Joystick P1: un solo eje (0-4095), cada 200ms
    // 0x18 = CENTER_SRV3 (usado del lado de la placa de consola, no en el HMI)
    HMI_REG_SRV3_LIMITS         = 0x19, // Limites servo3: bits[31:16]=max, bits[15:0]=min
    HMI_REG_MOTOR               = 0x1A, // Motor: bits[31:16]=cmd, bits[15:0]=vel (0-1000)
    HMI_REG_SRV1_ANGLE          = 0x1B, // Angulo servo1 cabeza (0-270°)
    HMI_REG_SRV2_ANGLE          = 0x1C, // Angulo servo2 cuello (0-270°)
    HMI_REG_SRV3_ANGLE          = 0x1D, // Angulo servo3 (0-270°)
    HMI_REG_OTA_STATUS          = 0x1E, // Consola -> HMI: estado update consola (0-5)
    HMI_REG_FW_VERSION          = 0x1F, // Consola -> HMI: version FW consola: bits[23:16]=major,[15:8]=minor,[7:0]=patch (se manda 1 vez al boot)
    HMI_REG_WIFI_STATUS         = 0x20, // Consola -> HMI: 1 = conectado (tiene IP), 0 = desconectado/apagado
    HMI_REG_VIDEO_TECH          = 0x21, // Tecnologia de video, boton A: 1 = prender LED en la placa de consola, 0 = apagar
    HMI_REG_VIDEO_TECH_B        = 0x22, // Tecnologia de video, boton B: 1 = prender LED en la placa de consola, 0 = apagar
    HMI_REG_WIFI_ENABLE         = 0x23, // HMI -> Consola: prender/apagar wifi de la consola (1/0)
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
void update_confirm_cb(lv_event_t *e);
void console_wifi_toggle_cb(lv_event_t *e);

// Carga la calibracion del encoder desde NVS en los spinboxes de
// Settings>Encoder. Debe llamarse despues de que esos spinboxes existan (ver
// create_panel_settings_encoder() en screens.c, y la construccion en etapas
// en app_main).
void enc_settings_load_from_nvs(void);

// ================================================================
// Preferencias de UI (tema/idioma/modo de bateria) — se guardan en NVS
// cuando el usuario las cambia desde Settings, y se cargan una vez al
// arrancar (ui_init, antes de aplicar tema/idioma) para que sobrevivan un
// reinicio en vez de volver siempre al default de fabrica.
// ================================================================
void hmi_ui_prefs_save_theme(int theme_idx);
void hmi_ui_prefs_save_lang(int lang_id);
void hmi_ui_prefs_save_bat_display(int percent);
int  hmi_ui_prefs_load_theme(void);       // default 1 (Classic)
int  hmi_ui_prefs_load_lang(void);        // default 0 (ES)
int  hmi_ui_prefs_load_bat_display(void); // default 0 (voltaje)

// ================================================================
// Giro Automatico personalizado — "Control por Puntos" / "Config Auto
// Rotation" / Settings > Limites de Servo. Mismo patron que Encoder/Update:
// los widgets se crean en ui/screens.c pero toda la logica (estado en RAM,
// NVS, motor de reproduccion con interpolacion) vive en main.c.
// ================================================================

// Llamar una vez desde app_main, despues de create_panel_modes_auto_rotation()
// y create_panel_settings_srv_limits() — carga NVS a RAM y refresca las UI.
void hmi_auto_rotation_init(void);

// MODES — grilla principal
void modes_btn_control_por_puntos_cb(lv_event_t *e);
void modes_btn_config_auto_rotation_cb(lv_event_t *e);
void modes_btn_giro_automatico_cb(lv_event_t *e);    // Iniciar / Pausar (toggle)
void modes_btn_stop_giro_automatico_cb(lv_event_t *e); // Cancelar

// Control por Puntos — reutiliza los +/- de cabeza/cuello ORIGINALES
// (reubicados dentro de este panel en screens.c), con sus callbacks
// action_angle_*_btn_* de siempre; solo estos dos son nuevos.
void cp_btn_guardar_centrado_cb(lv_event_t *e);
void cp_btn_volver_cb(lv_event_t *e);

// Config Auto Rotation — selector de recorrido
void autorot_btn_recorrido1_cb(lv_event_t *e);
void autorot_btn_recorrido2_cb(lv_event_t *e);
void autorot_picker_btn_volver_cb(lv_event_t *e);

// Config Auto Rotation — editor de puntos
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
// user_data = indice de punto (0..RECORRIDO_MAX_POINTS-1), asignado por fila
// al reconstruir la lista en hmi_auto_rotation_editor_refresh_list().
void ar_point_row_clicked_cb(lv_event_t *e);

// Settings > Limites de Servo
// user_data empaquetado: servo_idx(0-2)*100 + (is_max?10:0) + (increase?1:0)
void sl_limit_btn_cb(lv_event_t *e);
void sl_btn_guardar_cb(lv_event_t *e);
// Refresca los 6 labels de Settings>Limites de Servo desde la copia en RAM
// (cargada al boot desde NVS). Llamar al entrar a ese panel.
void hmi_srv_limits_load_to_ui(void);

#endif