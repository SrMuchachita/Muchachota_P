// ANSI C
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

// ESP-IDF Drivers
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/ledc.h"
#include "driver/i2c_master.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_st7701.h"
#include "esp_lcd_panel_ops.h" // Operacion de la pantalla
#include "driver/i2c_master.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lcd_touch.h"
#include "esp_heap_caps.h"
#include "esp_lv_adapter.h"
#include "lvgl.h"
#include "ui/ui.h"
#include "ui/screens.h"
#include "ui/actions.h"
#include "ui/lang.h"
#include "ui/vars.h"
#include "ui/images.h" // <-- agregado

// Fuente propia para el numero grande del panel de manejo (main/ui/font_distance.c).
// Nombre de archivo/simbolo FIJO a proposito: para cambiar el tamano solo se
// regenera el contenido de ese mismo archivo (no hace falta Full Clean).
LV_FONT_DECLARE(lv_font_distance);

// WiFi / OTA
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_hosted.h"
#include "esp_app_desc.h"
#include "ota_http.h"
#include "ui/lock_logos.h" // logotipo/isotipo Welltepp para pantalla de bloqueo
#include "main.h"


//*****************************************RECURSOS DE MODULO MAIN************************************************/
#define TASK_SIZE (6 * 1024) // KB
#define TAG "RD90Hmio"


//******************************************** SERIAL NUMBER ****************************************************************** */
#define SERIAL_NUMBER     "260007" // 6CARACTERES

//******************************************** VERSION INFO ****************************************************************** */
// FW_VERSION ya NO es un define fijo — se lee en runtime de
// esp_app_get_description()->version (viene de CMakeLists.txt
// project(... VERSION x.x.x), el mismo valor que usa el OTA para comparar),
// asi que la fila "Firmware :" en System Info siempre refleja el build
// realmente corriendo, incluso despues de una actualizacion OTA.
#define HW_REVISION       "Rev B"   // Revision de hardware
#define LVGL_VERSION_STR  "v9.2"    // Version de LVGL
#define ESPIDF_VERSION    "v5.4"    // Version de ESP-IDF
#define BUILD_DATE        __DATE__  // Fecha de compilacion (automatica del compilador)


// Descomentar la siguiente linea para activar la simulacion visual del indicador de bateria.
// La simulacion cicla entre los voltajes de prueba y NO afecta la recepcion UART real.
// Para volver al funcionamiento normal: volver a comentar esta linea.
//#define TEST_BATTERY_VOLTAGE
//#define TEST_UART_TX
#define TEST_UART_RX_DISPLAY   // Overlay en pantalla con tramas UART recibidas
#define DEV_MODE               // Panel desarrollador: 5 taps en VERSION de SysInfo
#define LOCK_SCREEN_ENABLE     // Pantalla de bloqueo al arrancar — comentar para desactivar
//#define TEST_UART_TEXT         // Prueba texto: envia 'U' cada 1s y muestra lo que recibe


//*****************************************PANTALLA******************************************************/
#define LCD_H_RES (480)
#define LCD_V_RES (800)
#define LV_BUFFER_SIZE (LCD_H_RES * LCD_V_RES * 2)
#define MIPI_LCD_BIT_PER_PIXEL (16)
#define MIPI_DSI_PHY_PWR_LDO_CHAN (3)
#define MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV (2500)
#define MIPI_DSI_LANE_NUM (2)
#define MIPI_PIN_NUM_LCD_RST (5)
// FORMATO PIXELS
#define MIPI_DPI_PX_FORMAT (LCD_COLOR_PIXEL_FORMAT_RGB565)


//*****************************************PINOUT PWM BK LCD******************************************************/
#define BK_PIN        23
#define LCD_LEDC_CH   1


//*****************************************DEFINICIONES I2C*********************************************************/
// PinOut I2C0 TOUCH
#define I2C0_I2C_PORT    I2C_NUM_0
#define I2C0_SDA_PIN     (7)
#define I2C0_SCL_PIN     (8)
// Paremetros del device GT911
// #define I2C0_DEV_ADDR     		        (0b1000000) // definida por HW
#define I2C0_DEV_FREQ_Hz (400 * 1000) // 100Khz (Modo Estandar)


//*****************************************DEFINICIONES UART*****************************************************/
// Pinout UART-HMI
/*#define HMI_UART_TXD      (51) // TXD 
#define HMI_UART_RXD      (52) // RXD */
#define HMI_UART_PORT     UART_NUM_1
#define UART_BUFFER_SIZE  (512)
#define HMI_UART_TXD      (51) // TXD via MAX485 -> J4 RS-485
#define HMI_UART_RXD      (52) // RXD via MAX485 -> J4 RS-485
#define HMI_UART_BAUD     (115200)
#define HMI_HEADER_1      0x5A
#define HMI_HEADER_2      0xA5


//*****************************************ESTRUCTURAS DE DATOS*****************************************************/
typedef struct
{
    uint8_t reg;
    int32_t value;
} hmi_tx_frame_t;

typedef struct
{
    uint8_t buffer[64];
    uint8_t len;
} hmi_rx_frame_t;

//****************************************PROTOTIPOS DE FUNCIONES APP*********************************************/
// Prototipos de funciones
void i2c0_master_init(void);
void app_lcd_init(void);
void app_touch_init(void);
void lcd_brightness_init(void);
void lcd_set_brightness(int duty_cycle);
void vUartInit(void);
void vHardwareInit(void);


//**************************************PROTOTIPOS DE FUNCIONES DE UTIL*********************************************/
void hmi_send_data(uint8_t reg, int32_t value);
void hmi_process_buffer(uint8_t *buffer, uint16_t len);
void hmi_handle_reg(uint8_t reg, int32_t value);
void hmi_reapply_cached_boot_regs(void);
static inline uint8_t battery_percent(uint16_t mv);
static uint32_t hmi_get_boot_count(void);
static void     hmi_log_refresh(void);
static void     hmi_conn_indicator_create(void);
static void     panels_startup_init(void);


//**************************************PROTOTIPOS DE FUNCIONES TAREAS*******************************************/
void vTaskUartHmiEvents(void *pvParameters);
void vTaskHmiRxProcess(void *pvParameters);
void vTaskHmiTransmit(void *pvParameters);
#ifdef TEST_BATTERY_VOLTAGE
void vTaskBatterySimTest(void *pvParameters);
#endif
#ifdef TEST_UART_TX
void vTaskUartTxTest(void *pvParameters);
#endif
#ifdef TEST_UART_TEXT
void vTaskUartTextTest(void *pvParameters);
#endif
#ifdef TEST_UART_RX_DISPLAY
static void rx_disp_log_frame(uint8_t reg, int32_t value);
#endif
#ifdef DEV_MODE
static void dev_mode_init(void);
static int  dev_uart_tx_pin(void);
static int  dev_uart_rx_pin(void);
static void dev_serial_add(const char *dir, const char *msg);
static void dev_joy_log_update(void);
static void encoder_display_toggle_create(void);
static void encoder_bigview_create(void);
static void bigview_angle_trace_push(float angle_deg, float delta_dist_m);
static void logo_secret_button_wire(void);
static void camera_rl1_mode_wire(void);
static void encoder_toggle_retheme(void);
static void console_wifi_ui_refresh(void);
static void settings_nav_enable_scroll(void);
static void hmi_wifi_set_enabled(bool enable);
static void dev_nvs_read_wifi_ssid(char *out, size_t max);
static void dev_nvs_write_wifi_ssid(const char *ssid);
static void dev_nvs_read_wifi_pass(char *out, size_t max);
static void dev_nvs_write_wifi_pass(const char *pass);
static void wifi_settings_ui_init(void);
#ifdef LOCK_SCREEN_ENABLE
static void lock_screen_create(void);
static SemaphoreHandle_t s_ls_sem        = NULL;
static lv_obj_t         *s_ls_panel      = NULL; // raiz pantalla completa
static lv_obj_t         *s_ls_card       = NULL; // panel glass central
static lv_obj_t         *s_ls_overlay    = NULL; // captura toques fuera para cerrar teclado
static lv_obj_t         *s_ls_field      = NULL; // fila "toca para ingresar"
static lv_obj_t         *s_ls_hint_lbl   = NULL; // placeholder antes de abrir teclado
static lv_obj_t         *s_ls_dotcon     = NULL;
static lv_obj_t         *s_ls_dots[4]    = {NULL};
static lv_obj_t         *s_ls_status_lbl = NULL;
static lv_obj_t         *s_ls_keypad_wrap= NULL; // contenedor deslizante del teclado
static char              s_ls_buf[5];
static int               s_ls_len       = 0;
static bool              s_ls_busy      = false;
#endif
#endif
#define HMI_LV_LOCKED(expr) do { if (esp_lv_adapter_lock(-1) == ESP_OK) { expr; esp_lv_adapter_unlock(); } } while (0)
#define HMI_LV_SAFE_OBJ(obj, expr) do { if ((obj) != NULL) { HMI_LV_LOCKED(expr); } } while (0)
//************************************** RECURSOS DE FREERTOS ****************************************************/
static QueueHandle_t xQueueHmiTx = NULL;
static QueueHandle_t xQueueHmiRx = NULL;
static QueueHandle_t xQueueUartEvent = NULL;


//************************************** VARIABLES GLOBALES ****************************************************/
uint8_t g_bat_display_percent = 0; // 0=voltaje, 1=porcentaje
#define SERIAL_NUM_LEN  6
static char s_serial_num[SERIAL_NUM_LEN + 1] = SERIAL_NUMBER;

// Indicador de conexion HMI ↔ Consola
static lv_obj_t          *led_hmi_conn       = NULL;
static volatile uint32_t  s_last_pong_ms     = 0;
static volatile bool      s_pong_received    = false;  // evita falso positivo al boot
#define HMI_CONN_TIMEOUT_MS  3000  // apagar LED si no hay PONG en 3s
#if defined(DEV_MODE) && defined(TEST_UART_RX_DISPLAY)
static bool s_rx_log_enabled = true;
#endif

// Variables para I2C
i2c_master_bus_handle_t i2c0_bus_handle = NULL;
/*Pantalla (display y tactil)*/
// MIPI
esp_ldo_channel_handle_t ldo_mipi_phy = NULL;
esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL;
esp_lcd_panel_io_handle_t mipi_dbi_io = NULL;
// TACTIL
esp_lcd_touch_handle_t tp_touch_handle = NULL;
// Numero de buffers que te retorna la interfaz
uint8_t num_fbs;

// Comandos de inicializacion de LCD ST7701
static const st7701_lcd_init_cmd_t lcd_cmd[] = {
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xEF, (uint8_t[]){0x08}, 1, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, (uint8_t[]){0x63, 0x00}, 2, 0},
    {0xC1, (uint8_t[]){0x0D, 0x02}, 2, 0},
    {0xC2, (uint8_t[]){0x10, 0x08}, 2, 0},
    {0xCC, (uint8_t[]){0x10}, 1, 0},

    {0xB0, (uint8_t[]){0x80, 0x09, 0x53, 0x0C, 0xD0, 0x07, 0x0C, 0x09, 0x09, 0x28, 0x06, 0xD4, 0x13, 0x69, 0x2B, 0x71}, 16, 0},
    {0xB1, (uint8_t[]){0x80, 0x94, 0x5A, 0x10, 0xD3, 0x06, 0x0A, 0x08, 0x08, 0x25, 0x03, 0xD3, 0x12, 0x66, 0x6A, 0x0D}, 16, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},

    {0xB0, (uint8_t[]){0x5D}, 1, 0},
    {0xB1, (uint8_t[]){0x58}, 1, 0},
    {0xB2, (uint8_t[]){0x87}, 1, 0},
    {0xB3, (uint8_t[]){0x80}, 1, 0},
    {0xB5, (uint8_t[]){0x4E}, 1, 0},
    {0xB7, (uint8_t[]){0x85}, 1, 0},
    {0xB8, (uint8_t[]){0x21}, 1, 0},
    {0xB9, (uint8_t[]){0x10, 0x1F}, 2, 0},
    {0xBB, (uint8_t[]){0x03}, 1, 0},
    {0xBC, (uint8_t[]){0x00}, 1, 0},

    {0xC1, (uint8_t[]){0x78}, 1, 0},
    {0xC2, (uint8_t[]){0x78}, 1, 0},
    {0xD0, (uint8_t[]){0x88}, 1, 0},

    {0xE0, (uint8_t[]){0x00, 0x3A, 0x02}, 3, 0},
    {0xE1, (uint8_t[]){0x04, 0xA0, 0x00, 0xA0, 0x05, 0xA0, 0x00, 0xA0, 0x00, 0x40, 0x40}, 11, 0},
    {0xE2, (uint8_t[]){0x30, 0x00, 0x40, 0x40, 0x32, 0xA0, 0x00, 0xA0, 0x00, 0xA0, 0x00, 0xA0, 0x00}, 13, 0},
    {0xE3, (uint8_t[]){0x00, 0x00, 0x33, 0x33}, 4, 0},
    {0xE4, (uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE5, (uint8_t[]){0x09, 0x2E, 0xA0, 0xA0, 0x0B, 0x30, 0xA0, 0xA0, 0x05, 0x2A, 0xA0, 0xA0, 0x07, 0x2C, 0xA0, 0xA0}, 16, 0},
    {0xE6, (uint8_t[]){0x00, 0x00, 0x33, 0x33}, 4, 0},
    {0xE7, (uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE8, (uint8_t[]){0x08, 0x2D, 0xA0, 0xA0, 0x0A, 0x2F, 0xA0, 0xA0, 0x04, 0x29, 0xA0, 0xA0, 0x06, 0x2B, 0xA0, 0xA0}, 16, 0},

    {0xEB, (uint8_t[]){0x00, 0x00, 0x4E, 0x4E, 0x00, 0x00, 0x00}, 7, 0},
    {0xEC, (uint8_t[]){0x08, 0x01}, 2, 0},

    {0xED, (uint8_t[]){0xB0, 0x2B, 0x98, 0xA4, 0x56, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0x65, 0x4A, 0x89, 0xB2, 0x0B}, 16, 0},
    {0xEF, (uint8_t[]){0x08, 0x08, 0x08, 0x45, 0x3F, 0x54}, 6, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},

    // {0x3A, (uint8_t []){0x66}, 1, 0},
    {0x11, (uint8_t[]){0x00}, 1, 120},
    {0x29, (uint8_t[]){0x00}, 1, 20},
};


/*****************************************************************************************************/
//**************************************WIFI / OTA****************************************************/
/*****************************************************************************************************/
#define WIFI_SSID "WTP TALLER"
#define WIFI_PASS "24012024"
#define WIFI_SSID_MAX 32
#define WIFI_PASS_MAX 64

#define OTA_VERSION_URL  "https://raw.githubusercontent.com/SrMuchachita/Muchachota_P/main/version.json"
#define OTA_FIRMWARE_URL "https://github.com/SrMuchachita/Muchachota_P/releases/latest/download/WP_P_V9_UP.bin"
#define OTA_CHECK 20 // segundos entre chequeos de version

// Estado del panel "Update" (System Info): WiFi apagado por defecto al
// arrancar, se prende/apaga a mano desde la UI. No se persiste en NVS.
// Los widgets (objects.update_led/update_status_label/update_toggle_btn) se
// crean en ui/screens.c junto con el resto de System Info.
static bool s_wifi_initialized = false; // wifi_init() ya corrio alguna vez
static bool s_wifi_enabled     = false; // estado logico actual (on/off)
static bool s_ota_configured   = false; // ota_http_start() ya corrio alguna vez — separado de
                                         // s_wifi_initialized porque el escaneo/verificacion del
                                         // editor de WiFi puede inicializar el driver ANTES de que
                                         // el usuario toque "Activar WiFi" por primera vez.

// true si fue el editor de WiFi (no el boton "Activar WiFi" de Update) quien
// prendio el radio — para: 1) que wifi_event_handler NO auto-conecte a la
// red vieja guardada en STA_START (chocaria con esp_wifi_scan_start(), que
// falla con "still connecting"), y 2) saber si hay que apagar el radio de
// nuevo (y sincronizar el boton "Activar WiFi") al terminar de escanear.
// Ver wifi_editor_ensure_radio_on()/wifi_editor_close().
static bool s_wifi_enabled_by_editor = false;

// true mientras el editor de WiFi "tiene prestado" el radio para escanear:
// el WiFi de actualizacion, una vez activado, reintenta conectarse solo a la
// red guardada en el fondo (STA_DISCONNECTED -> esp_wifi_connect()) — si
// esa red no esta al alcance, el radio queda "conectando" casi todo el
// tiempo, y esp_wifi_scan_start() falla justo en ese estado ("still
// connecting"). Mientras esto este en true, wifi_event_handler() NO
// reintenta conectar solo; wifi_editor_scan_done() lo apaga y retoma el
// intento normal cuando el escaneo termina.
static bool s_wifi_scan_active = false;

// LED de "Activar WiFi" (System Info > Update): apagado si el WiFi esta
// apagado, PARPADEANDO si esta prendido pero todavia no consiguio conectarse
// a ninguna red, SOLIDO si ya esta conectado.
typedef enum {
    UPDATE_LED_OFF = 0,
    UPDATE_LED_BLINK,
    UPDATE_LED_SOLID,
} update_led_mode_t;

static lv_timer_t *s_update_led_blink_timer = NULL;
static bool        s_update_led_blink_on    = false;

static void update_led_blink_cb(lv_timer_t *t)
{
    (void)t;
    s_update_led_blink_on = !s_update_led_blink_on;
    if (objects.update_led) lv_led_set_brightness(objects.update_led, s_update_led_blink_on ? 255 : 0);
}

static void update_led_set_mode(update_led_mode_t mode)
{
    if (mode == UPDATE_LED_BLINK) {
        if (!s_update_led_blink_timer) {
            s_update_led_blink_on = false;
            s_update_led_blink_timer = lv_timer_create(update_led_blink_cb, 500, NULL);
        }
        return; // el timer se encarga del brillo mientras dura este modo
    }
    if (s_update_led_blink_timer) { lv_timer_delete(s_update_led_blink_timer); s_update_led_blink_timer = NULL; }
    if (objects.update_led) lv_led_set_brightness(objects.update_led, mode == UPDATE_LED_SOLID ? 255 : 0);
}

// Muestra/oculta la fila "SSID guardado + Buscar redes" — solo tiene sentido
// tocarla cuando el WiFi esta activado (ver hmi_wifi_set_enabled()).
static void update_wifi_network_row_set_visible(bool visible)
{
    if (!objects.update_wifi_network_row) return;
    if (visible) lv_obj_remove_flag(objects.update_wifi_network_row, LV_OBJ_FLAG_HIDDEN);
    else         lv_obj_add_flag(objects.update_wifi_network_row, LV_OBJ_FLAG_HIDDEN);
}

// Sin lock: se llama tanto desde contexto LVGL (click del boton toggle) como,
// via HMI_LV_LOCKED, desde wifi_event_handler (tarea del event loop de ESP-IDF).
static void update_panel_set_status(const char *text, update_led_mode_t led_mode)
{
    if (objects.update_status_label) lv_label_set_text(objects.update_status_label, text);
    update_led_set_mode(led_mode);
}

// "Network: <SSID>" solo tiene sentido mostrarlo cuando el WiFi esta
// realmente conectado (con IP) — antes de eso (apagado, conectando,
// reconectando) queda oculto. Sin lock: mismo patron que
// update_panel_set_status(). El texto se arma con el SSID GUARDADO en NVS
// (el que se uso para conectar), no un nombre fijo — antes decia siempre
// "WTP TALLER" aunque el usuario hubiera cambiado de red desde el editor.
static void update_network_label_set_visible(bool visible)
{
    if (!objects.update_network_label) return;
    if (visible) {
        char cur_ssid[WIFI_SSID_MAX];
        dev_nvs_read_wifi_ssid(cur_ssid, sizeof(cur_ssid));
        char buf[48];
        snprintf(buf, sizeof(buf), "%s%s", g_lang->lbl_network_prefix, cur_ssid[0] ? cur_ssid : WIFI_SSID);
        lv_label_set_text(objects.update_network_label, buf);
        lv_obj_remove_flag(objects.update_network_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(objects.update_network_label, LV_OBJ_FLAG_HIDDEN);
    }
}

// Pone el boton "Actualizar" en amarillo con la version nueva, y lo muestra.
// Sin lock: se llama tanto desde contexto LVGL (al reintentar tras un fallo,
// ver mas abajo) como, via HMI_LV_LOCKED, desde ota_update_available_cb()
// (tarea de OTA, fuera de la tarea de render de LVGL).
static void update_available_btn_show(const char *version)
{
    if (!objects.update_available_btn) return;

    char buf[40];
    snprintf(buf, sizeof(buf), "Update %s", version);

    lv_obj_remove_flag(objects.update_available_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_state(objects.update_available_btn, LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(objects.update_available_btn, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.update_available_btn, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *lbl = lv_obj_get_child(objects.update_available_btn, 0);
    if (lbl) {
        lv_label_set_text(lbl, buf);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

// Callback de ota_http (components/ota_http): corre en la tarea de OTA, NO en
// la tarea de render de LVGL — hay que tomar el lock del adaptador antes de
// tocar el arbol de widgets.
static void ota_update_available_cb(const char *new_version)
{
    HMI_LV_LOCKED(update_available_btn_show(new_version));
}

// Llamado por ota_http justo antes de esp_restart() tras un OTA exitoso. El
// backlight es un LED por PWM totalmente aparte del controlador del panel
// MIPI-DSI: al reiniciar, el panel pierde su configuracion y muestra video
// sin inicializar (tipicamente un color solido, ej. celeste) hasta que el
// firmware nuevo lo reprograma, varios segundos despues. Apagando el
// backlight antes del reset, esa ventana queda oculta (pantalla negra) en
// vez de visible — no toca lv_obj nada, es directo sobre el LEDC del
// backlight, asi que no hace falta esp_lv_adapter_lock().
static void ota_before_restart_cb(void)
{
    lcd_set_brightness(0);
}

// Muestra/oculta el overlay "Actualizando..." — pantalla fija, sin numeros
// que cambien, para no reintroducir redibujados dinamicos durante la
// descarga. Sin lock: llamar siempre envuelto en HMI_LV_LOCKED desde fuera
// de la tarea de render.
static void ota_progress_overlay_show(void)
{
    if (!objects.ota_progress_overlay) return;
    lv_obj_move_foreground(objects.ota_progress_overlay);
    lv_obj_remove_flag(objects.ota_progress_overlay, LV_OBJ_FLAG_HIDDEN);
}
static void ota_progress_overlay_hide(void)
{
    if (objects.ota_progress_overlay) lv_obj_add_flag(objects.ota_progress_overlay, LV_OBJ_FLAG_HIDDEN);
}

// Llamado por ota_http justo antes de arrancar la descarga real. Escribir
// cada pagina del firmware a la flash SPI pausa brevemente AMBOS nucleos
// (proteccion de bajo nivel de ESP-IDF, no evitable por scheduling/prioridad
// de tareas), asi que la tarea de render de LVGL no puede mantener el panel
// al dia durante esas pausas — el celeste disimulado ayuda pero no lo tapa
// del todo. Mostramos "Actualizando..." ~2s (tiempo para que el usuario lo
// lea) y despues apagamos el backlight para el resto de la descarga —
// mismo mecanismo que ya funciona para el reinicio. Corre en la tarea de
// OTA, asi que este vTaskDelay solo atrasa el inicio de la descarga 2s, no
// bloquea la UI.
static void ota_before_download_cb(void)
{
    HMI_LV_LOCKED(ota_progress_overlay_show());
    lcd_set_brightness(100);
    vTaskDelay(pdMS_TO_TICKS(2000));
    lcd_set_brightness(0);
}

// Llamado por ota_http si la descarga fallo (red caida, etc.). A diferencia
// del caso exitoso, aca el equipo NO reinicia — sigue mostrando la UI
// normal, asi que hay que ocultar el overlay y volver a prender el
// backlight (ota_before_download_cb lo dejo apagado durante la descarga).
static void ota_download_failed_cb(void)
{
    HMI_LV_LOCKED(ota_progress_overlay_hide());
    lcd_set_brightness(100);
}

// Click en "Actualizar": autoriza la descarga a la tarea de OTA (bloqueada
// esperando ota_http_confirm_update()) y muestra feedback inmediato. Corre en
// contexto LVGL (el propio click), asi que NO hay que tomar el lock aca.
void update_confirm_cb(lv_event_t *e)
{
    (void)e;
    if (!objects.update_available_btn) return;

    lv_obj_t *lbl = lv_obj_get_child(objects.update_available_btn, 0);
    if (lbl) lv_label_set_text(lbl, g_lang->lbl_updating);
    lv_obj_add_state(objects.update_available_btn, LV_STATE_DISABLED);

    ota_http_confirm_update();
}

// Definidas junto con el editor de WiFi (Settings/Update > Editar), mas
// abajo en este archivo — declaradas aca porque wifi_event_handler() (el
// unico event handler de WIFI_EVENT/IP_EVENT que existe) necesita llamarlas.
static void wifi_editor_scan_done(void);

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // El editor de WiFi (escaneo/verificacion) prende el radio por el
        // mismo camino que el boton "Activar WiFi" (hmi_wifi_set_enabled,
        // ver wifi_editor_ensure_radio_on()), para que el boton quede
        // sincronizado. Pero si fue el editor quien lo prendio
        // (s_wifi_enabled_by_editor), NO auto-conectar a la red vieja
        // guardada aca — esp_wifi_scan_start() falla con "still connecting"
        // si hay un intento de conexion en curso, y el editor conecta a
        // mano, explicitamente, recien cuando el usuario elige una red.
        if (s_wifi_enabled && !s_wifi_enabled_by_editor) {
            esp_wifi_connect();
            HMI_LV_LOCKED({
                update_panel_set_status(g_lang->lbl_connecting, UPDATE_LED_BLINK);
                update_network_label_set_visible(false);
            });
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        HMI_LV_LOCKED(wifi_editor_scan_done());
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_wifi_scan_active) {
            // Desconexion a proposito, pedida por el editor para poder
            // escanear (esp_wifi_scan_start() falla si hay un intento de
            // conexion en curso) — no reintentar aca, wifi_editor_scan_done()
            // retoma el intento normal cuando el escaneo termina.
        } else if (s_wifi_enabled) {
            // Si el usuario apago el WiFi a proposito (hmi_wifi_set_enabled(false)
            // ya hizo esp_wifi_disconnect()), no reintentar ni pisar el estado "apagado".
            ESP_LOGW(TAG, "WiFi desconectado, reintentando...");
            esp_wifi_connect();
            HMI_LV_LOCKED({
                update_panel_set_status(g_lang->lbl_reconnecting, UPDATE_LED_BLINK);
                update_network_label_set_visible(false);
            });
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi conectado, IP: " IPSTR, IP2STR(&event->ip_info.ip));
        ota_http_notify_connected();
        char buf[48];
        snprintf(buf, sizeof(buf), "%s" IPSTR, g_lang->lbl_connected_prefix, IP2STR(&event->ip_info.ip));
        HMI_LV_LOCKED({
            update_panel_set_status(buf, UPDATE_LED_SOLID);
            update_network_label_set_visible(true);
        });
    }
}

// Esta funcion debe correr una sola vez en toda la vida del programa — la
// llama hmi_wifi_set_enabled() y esta guardada por s_wifi_initialized. Nunca
// llamarla directamente desde otro lado: esp_netif_create_default_wifi_sta()
// no tolera un segundo llamado ("duplicate key" -> assert -> reinicio).
static void wifi_init(void)
{
    // esp_hosted_init() (transporte SDIO hacia el coprocesador ESP32-C6) ya
    // NO se auto-inicializa en el boot (ver el fork local en
    // components/esp_hosted/host/port/esp/freertos/src/port_esp_hosted_host_init.c
    // — antes corria via constructor de C, sin importar si el WiFi se
    // llegaba a usar, sumando ~3s fijos de handshake SDIO al arranque).
    // Se llama aca, la primera vez que el usuario activa el WiFi. Es
    // idempotente (guardada por esp_hosted_init_done adentro del propio
    // componente), asi que es seguro si wifi_init() llegara a correr de
    // nuevo.
    ESP_ERROR_CHECK(esp_hosted_init());

    // El coprocesador WiFi/BT externo (ESP32-C6 por SDIO, esp_wifi_remote)
    // puede haber quedado con su propio netif/event-loop de una corrida
    // anterior; toleramos "ya inicializado" en vez de tratarlo como fatal.
    esp_err_t err;

    err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }

    // Defensa extra: si por algun motivo ya existe el netif WIFI_STA_DEF
    // (p.ej. quedo vivo de un caso raro), no volver a crearlo — eso es lo
    // que dispara el assert de esp_netif_create_default_wifi_sta().
    if (!esp_netif_get_handle_from_ifkey("WIFI_STA_DEF")) {
        esp_netif_create_default_wifi_sta();
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    // SSID/contrasena guardados a mano en Settings > WiFi tienen prioridad;
    // si nunca se guardo nada, usa los fijos del codigo como antes.
    char saved_ssid[WIFI_SSID_MAX] = "";
    char saved_pass[WIFI_PASS_MAX] = "";
    dev_nvs_read_wifi_ssid(saved_ssid, sizeof(saved_ssid));
    dev_nvs_read_wifi_pass(saved_pass, sizeof(saved_pass));

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, saved_ssid[0] ? saved_ssid : WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, saved_pass[0] ? saved_pass : WIFI_PASS, sizeof(wifi_config.sta.password) - 1);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

// Inicializa el driver WiFi la primera vez que hace falta (guardado por
// s_wifi_initialized, wifi_init() sigue corriendo una sola vez en toda la
// vida del programa) y se asegura de que este prendido. La usan tanto el
// WiFi "de actualizacion" (hmi_wifi_set_enabled) como el escaneo/verificacion
// del editor de WiFi — separada de hmi_wifi_set_enabled() porque esa tambien
// cambia s_wifi_enabled y arranca OTA, cosas que el escaneo no debe tocar.
static void wifi_driver_ensure_ready(void)
{
    if (!s_wifi_initialized) {
        wifi_init();
        s_wifi_initialized = true;
    } else {
        esp_wifi_start(); // no-op (error ignorado) si ya estaba prendido
    }
}

/*****************************************************************************************************/
//**************************************APP MAIN******************************************************/
/*****************************************************************************************************/
// Instrumentacion temporal de arranque — para medir donde se va el tiempo
// entre el saludo y que aparece la interfaz. Sacar despues de diagnosticar.
#define BOOT_MARK(label) ESP_LOGW(TAG, "BOOT_MARK %-28s t=%lld ms", label, (long long)(esp_timer_get_time() / 1000))

void app_main(void)
{
    BOOT_MARK("app_main start");
    // NVS init global (requerido antes de vUartInit cuando DEV_MODE usa NVS)
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Consultar a la interfaz cuantos buffers requiere para mi configuracion
    num_fbs = esp_lv_adapter_get_required_frame_buffer_count(
        ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL,
        ESP_LV_ADAPTER_ROTATE_90);
    ESP_LOGI(TAG, "FRAMEBUFFERS REQUERIDOS PARA 90/270 : %d", num_fbs);

    BOOT_MARK("before vHardwareInit");
    // Hardware init
    vHardwareInit();
    BOOT_MARK("after vHardwareInit");

    // Paso 0: Crear un esp_lcd_panel y opcionalmente un panel_io con esp_lcd APIs
    // Ya esta creado la pantalla arriba
    // Paso 1: Inicializar esp_lv_adapter
    esp_lv_adapter_config_t cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(esp_lv_adapter_init(&cfg));

    // Paso 2: Registrar una pantalla (escoger macro dependiendo de interfaz(I2C/SPI/INTEL8080/RGB/MIPI-DSI))
    esp_lv_adapter_display_config_t disp_cfg = ESP_LV_ADAPTER_DISPLAY_MIPI_DEFAULT_CONFIG(
        panel_handle,
        mipi_dbi_io,
        LCD_H_RES,
        LCD_V_RES,
        ESP_LV_ADAPTER_ROTATE_90);

    disp_cfg.tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL;
    disp_cfg.profile.buffer_height = LCD_V_RES / 10; // Precauicion, si es mucho satura el PPA y se congela
    disp_cfg.profile.use_psram = true;
    disp_cfg.profile.enable_ppa_accel = true; // No usar PPA, inestable por el momento
    lv_display_t *disp = esp_lv_adapter_register_display(&disp_cfg);
    assert(disp != NULL);
    BOOT_MARK("after register_display");

    // Paso 3: (Opcional) Registrar dispositivo de entrada
    // Crear un touch_handle empleando esp_lcd_touch API
    esp_lv_adapter_touch_config_t touch_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(disp, tp_touch_handle);
    lv_indev_t *touch = esp_lv_adapter_register_touch(&touch_cfg);
    assert(touch != NULL);
    // Fondo negro antes de arrancar el task de render: el primer frame pintado ya es negro
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Paso 4: Empezar esp_lv_adapter
    ESP_ERROR_CHECK(esp_lv_adapter_start());

    // Esperar a que inicialice ESP LV ADAPTER antes de mostrar el splash
    ESP_LOGI(TAG, "Waiting for LVGL adapter...");
    while (!esp_lv_adapter_is_initialized())
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    ESP_LOGI(TAG, "ESP LV ADAPTER initialized");
    BOOT_MARK("lv adapter initialized");

    // Mantener pantalla apagada hasta que el contenido esté listo
    lcd_set_brightness(0);

    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        esp_lv_adapter_unlock();
    }
    esp_lv_adapter_refresh_now(disp);
    vTaskDelay(pdMS_TO_TICKS(50));

    // Mostrar splash screen
    lv_obj_t *img = NULL;
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        img = lv_img_create(lv_scr_act());
        lv_img_set_src(img, &welltepp);
        lv_obj_center(img);
        esp_lv_adapter_unlock();
    }

    esp_lv_adapter_refresh_now(disp);
    BOOT_MARK("splash shown");

    // Encender panel y backlight una vez que el splash está listo
    esp_lcd_panel_disp_on_off(panel_handle, true);
    lcd_set_brightness(100);
    BOOT_MARK("backlight on");

    // WiFi + OTA ya NO arranca aca: ahora es manual, ver hmi_wifi_set_enabled()
    // (boton "Update" en System Info). Arrancaba antes solo al boot; se saco
    // porque duplicaba la llamada a wifi_init() (la de hmi_wifi_set_enabled
    // corria ademas de esta) y esp_netif_create_default_wifi_sta() no tolera
    // crearse dos veces: "duplicate key" -> assert -> reinicio.

    vTaskDelay(pdMS_TO_TICKS(1500));

    // Fundido de salida del splash (en vez de un corte abrupto)
    if (img != NULL && esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_obj_fade_out(img, 400, 0);
        esp_lv_adapter_unlock();
    }
    vTaskDelay(pdMS_TO_TICKS(400));
    if (img != NULL && esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_obj_del(img);
        esp_lv_adapter_unlock();
    }
    BOOT_MARK("splash faded out");

#ifdef LOCK_SCREEN_ENABLE
    if (esp_lv_adapter_lock(-1) == ESP_OK) {
        lock_screen_create();  // hace fade-in propio al final de su construccion
        esp_lv_adapter_unlock();
    }
    BOOT_MARK("lock_screen_create done");
    xSemaphoreTake(s_ls_sem, portMAX_DELAY);
    BOOT_MARK("lock screen unlocked (semaphore)");
    // Fundido de salida de la pantalla de bloqueo antes de revelar la UI principal
    if (esp_lv_adapter_lock(-1) == ESP_OK) {
        if (s_ls_panel) lv_obj_fade_out(s_ls_panel, 350, 0);
        esp_lv_adapter_unlock();
    }
    vTaskDelay(pdMS_TO_TICKS(350));
    if (esp_lv_adapter_lock(-1) == ESP_OK) {
        if (s_ls_panel) { lv_obj_del(s_ls_panel); s_ls_panel = NULL; }
        esp_lv_adapter_unlock();
    }
    BOOT_MARK("lock screen removed");
#endif

    // Apagar backlight mientras se arma la interfaz base (create_screens(),
    // invocado desde ui_init()). Sin esto, la pantalla queda prendida
    // mostrando el ultimo frame valido mientras el lock de LVGL esta tomado,
    // y el panel termina mostrando un frame a medio escribir (flash negro/
    // celeste) en vez de mantenerse quieto. Mismo patron que el splash.
    lcd_set_brightness(0);

    // Colas + tareas UART — REVERTIDO a arrancar aca (justo antes de
    // ui_init(), como era originalmente) despues de confirmar que
    // arrancarlas mas temprano (justo tras inicializar el adapter LVGL)
    // causaba pantalla celeste/reinicio cuando el robot ya estaba
    // conectado y mandando datos reales desde el arranque. Con el robot
    // desconectado bootea normal, lo que confirma que era una condicion de
    // carrera con trafico UART real muy temprano en el boot, no un bug
    // general. Los valores "unicos" que puedan llegar tarde igual se
    // cachean y reaplican (ver hmi_reapply_cached_boot_regs).
    xQueueHmiTx = xQueueCreate(50, sizeof(hmi_tx_frame_t));
    if (xQueueHmiTx == NULL)
    {
        ESP_LOGE(TAG, "Error al crear xQueueHmiTx");
        return;
    }

    xQueueHmiRx = xQueueCreate(50, sizeof(hmi_rx_frame_t));
    if (xQueueHmiRx == NULL)
    {
        ESP_LOGE(TAG, "Error al crear xQueueHmiRx");
        return;
    }

    xTaskCreate(vTaskHmiTransmit,   "HMI Tx",    TASK_SIZE, NULL, 5, NULL);
    xTaskCreate(vTaskUartHmiEvents, "HMI Event", TASK_SIZE, NULL, 5, NULL);
    xTaskCreate(vTaskHmiRxProcess,  "HMI Rx",   TASK_SIZE, NULL, 6, NULL);

    // ui_init() construye toda la interfaz (base + paneles ocultos) en un
    // solo pase, dentro de este unico lock. Ver el comentario en ui_init()
    // (ui.c) — cuatro intentos distintos de escalonar esta construccion
    // terminaron en pantalla negra trabada/congelada; esta es la unica
    // arquitectura confirmada estable.
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        ui_init();
        hmi_conn_indicator_create();
        settings_nav_enable_scroll();
        encoder_display_toggle_create();
        encoder_bigview_create();
        logo_secret_button_wire();
        camera_rl1_mode_wire();
        wifi_settings_ui_init();
        esp_lv_adapter_unlock();
    }
    esp_lv_adapter_refresh_now(disp);
    lcd_set_brightness(100);
    BOOT_MARK("ui_init done");

    panels_startup_init();
    BOOT_MARK("panels_startup_init done");
    hmi_log(LOG_OK, "Hardware initialized");
    hmi_log(LOG_OK, "Display ready");
    hmi_log(LOG_OK, "Touch ready");
    hmi_log(LOG_OK, "UART HMI started");
    hmi_log(LOG_OK, "UI loaded");
    hmi_log(LOG_WARN, "Waiting for robot...");

    // Por si algun registro "unico" (S/N, modelo, version de firmware) llego
    // por UART mientras la interfaz todavia se armaba: los widgets ya
    // existen, se vuelven a aplicar los valores cacheados.
    hmi_reapply_cached_boot_regs();

#ifdef TEST_BATTERY_VOLTAGE
    xTaskCreate(vTaskBatterySimTest, "Bat Sim",  TASK_SIZE, NULL, 3, NULL);
    ESP_LOGW(TAG, "*** TEST_BATTERY_VOLTAGE ACTIVO: simulacion de bateria habilitada ***");
#endif
#ifdef TEST_UART_TX
    xTaskCreate(vTaskUartTxTest, "UART Test", TASK_SIZE, NULL, 3, NULL);
    ESP_LOGW(TAG, "*** TEST_UART_TX ACTIVO: enviando 'Hola Mundo' por UART ***");
#endif
#ifdef TEST_UART_RX_DISPLAY
    ESP_LOGW(TAG, "*** TEST_UART_RX_DISPLAY: bytes UART crudos -> panel LOGS ***");
#endif
#ifdef TEST_UART_TEXT
    xTaskCreate(vTaskUartTextTest, "UART Txt", TASK_SIZE, NULL, 3, NULL);
    ESP_LOGW(TAG, "*** TEST_UART_TEXT ACTIVO: TX='U' cada 1s, RX->LOGS ***");
#endif
#ifdef DEV_MODE
    dev_mode_init();
#endif

    vTaskDelay(pdMS_TO_TICKS(100));

    /*CAMBIAR COLOR DE TABVIEW*/
    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        /*SLIDER PARA EL BRILLO, SETEAR*/
        lcd_set_brightness(100);
        lv_label_set_text(objects.console_brightness_label, "100");
        lv_slider_set_value(objects.console_brightness_slider, 100, LV_ANIM_OFF);

        /*TABVIEW ESTILO - MANUAL*/
        lv_obj_t *tabview = objects.tabview;
        lv_obj_t *tab_bar = lv_tabview_get_tab_bar(tabview);

        for (int i = 0; i < 3; i++)
        {
            lv_obj_t *btn = lv_obj_get_child(tab_bar, i);
            lv_obj_set_style_bg_opa(btn, LV_OPA_20, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xf5c518), LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_border_width(btn, 4, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_border_color(btn, lv_color_hex(0xf5c518), LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_text_color(btn, lv_color_hex(0xf5c518), LV_PART_MAIN | LV_STATE_CHECKED);
        }

        /*SET DEVICE NUMBER*/
        {
            char sn_buff[30];
            snprintf(sn_buff, sizeof(sn_buff), "Console S/N :  RD90C-%s", s_serial_num);
            lv_label_set_text(objects.serial_number, sn_buff);

            char sn_short[20];
            snprintf(sn_short, sizeof(sn_short), "RD90C-%s", s_serial_num);
            if (objects.sysinfo_console_serial_value)
                lv_label_set_text(objects.sysinfo_console_serial_value, sn_short);
        }

        /*SET VERSION INFO — panel: child(0)=titulo, child(1..6)=filas, cada fila child(1)=valor*/
        {
            lv_obj_t *panel = objects.sysinfo_content_version;
            if (panel) {
                lv_obj_t *row;
                row = lv_obj_get_child(panel, 1); if (row) lv_label_set_text(lv_obj_get_child(row, 1), esp_app_get_description()->version);
                row = lv_obj_get_child(panel, 2); if (row) lv_label_set_text(lv_obj_get_child(row, 1), HW_REVISION);
                row = lv_obj_get_child(panel, 3); if (row) lv_label_set_text(lv_obj_get_child(row, 1), LVGL_VERSION_STR);
                row = lv_obj_get_child(panel, 4); if (row) lv_label_set_text(lv_obj_get_child(row, 1), ESPIDF_VERSION);
                row = lv_obj_get_child(panel, 5); if (row) lv_label_set_text(lv_obj_get_child(row, 1), BUILD_DATE);
                row = lv_obj_get_child(panel, 6);
                if (row) {
                    char bc_buff[12];
                    snprintf(bc_buff, sizeof(bc_buff), "%lu", hmi_get_boot_count());
                    lv_label_set_text(lv_obj_get_child(row, 1), bc_buff);
                }
            }
        }
        esp_lv_adapter_unlock();
    }

    ESP_LOGI(TAG, "UI INICIALIZADO");
}

/* ============================================================
 * TAREAS FREERTOS
 * ============================================================ */

void vTaskUartHmiEvents(void *pvParameters)
{
    // Datos locales
    uart_event_t event;
    hmi_rx_frame_t rx_frame;

    while (1)
    {
        // Waiting for UART event.
        if (pdTRUE == xQueueReceive(xQueueUartEvent, (void *)&event, pdMS_TO_TICKS(1000)))
        {
            // Limpiar buffer
            bzero(rx_frame.buffer, sizeof(rx_frame.buffer));

            switch (event.type)
            {
            // Evento de recepción de datos UART
            case UART_DATA:
                //ESP_LOGI(TAG, "UART_DATA");
                // Leer informacion del serial
                rx_frame.len = event.size;

                if (rx_frame.len > sizeof(rx_frame.buffer))
                {
                    rx_frame.len = sizeof(rx_frame.buffer);
                }

                uart_read_bytes(HMI_UART_PORT, rx_frame.buffer, rx_frame.len, pdMS_TO_TICKS(10));
                ESP_LOGW(TAG, "[RAW RX] %d bytes: %02X %02X %02X %02X %02X %02X %02X",
                         rx_frame.len,
                         rx_frame.len > 0 ? rx_frame.buffer[0] : 0,
                         rx_frame.len > 1 ? rx_frame.buffer[1] : 0,
                         rx_frame.len > 2 ? rx_frame.buffer[2] : 0,
                         rx_frame.len > 3 ? rx_frame.buffer[3] : 0,
                         rx_frame.len > 4 ? rx_frame.buffer[4] : 0,
                         rx_frame.len > 5 ? rx_frame.buffer[5] : 0,
                         rx_frame.len > 6 ? rx_frame.buffer[6] : 0);

                // Enviar los datos a la cola
                if (pdFALSE == xQueueSendToBack(xQueueHmiRx, &rx_frame, pdMS_TO_TICKS(10)))
                {
                    // Cola llena
                    ESP_LOGE(TAG, "UART_DATA, Cola Llena");
                }
                break;

            // Event of HW FIFO overflow detected
            case UART_FIFO_OVF:
                ESP_LOGE(TAG, "UART_HMI_PORT: UART_FIFO_OVF");
                hmi_log(LOG_ERR, "UART FIFO overflow");
                // Si se produce un desbordamiento de FIFO, debería considerar agregar un control de flujo para su aplicación.
                // El ISR ya ha restablecido el FIFO de recepción.
                uart_flush_input(HMI_UART_PORT);
                xQueueReset(xQueueUartEvent);
                break;

            // Event of UART ring buffer full
            case UART_BUFFER_FULL:
                ESP_LOGE(TAG, "UART_HMI_PORT: UART_BUFFER_FULL");
                // Si el búfer se llena, debería considerar aumentar el tamaño del búfer
                uart_flush_input(HMI_UART_PORT);
                xQueueReset(xQueueUartEvent);
                break;

            // Event of UART RX break detected
            case UART_BREAK:
                
                ESP_LOGE(TAG, "UART_HMI_PORT: UART_BREAK");
                break;

            // Event of UART parity check error
            case UART_PARITY_ERR:
                ESP_LOGE(TAG, "UART_HMI_PORT: UART_PARITY_ERR");
                break;

            // Event of UART frame error
            case UART_FRAME_ERR:
                ESP_LOGE(TAG, "UART_HMI_PORT: UART_FRAME_ERR");
                break;

            // Others
            default:
                ESP_LOGE(TAG, "UART_HMI_PORT: Uart event type: %d \n\n", event.type);
                break;
            }
        }
    }
}


void vTaskHmiRxProcess(void *pvParameters)
{
    // HEADER | REG |  DATA (4 bytes)
    // 5A A5  | 03  |  D3 D2 D1 D0
    hmi_rx_frame_t rx_frame;

    while (1)
    {
#ifdef TEST_UART_RX_DISPLAY
        if (xQueueReceive(xQueueHmiRx, &rx_frame, pdMS_TO_TICKS(500)))
#else
        if (xQueueReceive(xQueueHmiRx, &rx_frame, portMAX_DELAY))
#endif
        {
            hmi_process_buffer(rx_frame.buffer, rx_frame.len);
        }

#ifdef TEST_UART_RX_DISPLAY
        {
            static uint32_t last_ping_ms = 0;
            uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
            if (now_ms < 5000) goto skip_ping;  // esperar 5s al boot para que la consola arranque
            if ((now_ms - last_ping_ms) >= 1000) {
                last_ping_ms = now_ms;
                hmi_send_data(HMI_REG_PING, 1);
                hmi_log(LOG_TX, ">> PING");
#ifdef DEV_MODE
                dev_serial_add("TX", "PING");
#endif
            }
            skip_ping:;
        }
#endif

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void vTaskHmiTransmit(void *pvParameters)
{
    hmi_tx_frame_t txFrame;
    uint8_t frame[7];

    while (1)
    {
        if (xQueueReceive(xQueueHmiTx, &txFrame, portMAX_DELAY))
        {
            frame[0] = HMI_HEADER_1;
            frame[1] = HMI_HEADER_2;

            frame[2] = txFrame.reg;

            frame[3] = (txFrame.value >> 24);
            frame[4] = (txFrame.value >> 16);
            frame[5] = (txFrame.value >> 8);
            frame[6] = (txFrame.value);

            uart_write_bytes(HMI_UART_PORT, frame, sizeof(frame));
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


//*************************************************************************************
//************************** TAREA DE SIMULACION (solo pruebas) **********************
//*************************************************************************************
#ifdef TEST_UART_TX
void vTaskUartTxTest(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(500));

    // Saludo inicial
    const char *bienvenida = "=== TEST UART ACTIVO ===\r\nEscribe una letra y te respondo.\r\n";
    uart_write_bytes(HMI_UART_PORT, bienvenida, strlen(bienvenida));

    uint8_t rx_buf[32];
    while (1)
    {
        int nbytes = uart_read_bytes(HMI_UART_PORT, rx_buf, sizeof(rx_buf) - 1, pdMS_TO_TICKS(100));
        if (nbytes > 0)
        {
            for (int i = 0; i < nbytes; i++)
            {
                char resp[32];
                int len = snprintf(resp, sizeof(resp), "Enviaste: %c (0x%02X)\r\n", rx_buf[i], rx_buf[i]);
                uart_write_bytes(HMI_UART_PORT, resp, len);
                ESP_LOGW(TAG, "[TEST UART RX] Enviaste: %c (0x%02X)", rx_buf[i], rx_buf[i]);
            }
        }
    }
}
#endif

#ifdef TEST_UART_TEXT
//*************************************************************************************
// TEST_UART_TEXT — envia 'U' cada 1s, muestra lo que recibe en log y en pantalla
//*************************************************************************************
void vTaskUartTextTest(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGW(TAG, "[TEXT TEST] Iniciado — enviando 'U' cada 1s");
    hmi_log(LOG_WARN, "[TEXT TEST] Activo");

    uint8_t rx_buf[32];
    uint32_t last_tx = 0;

    while (1)
    {
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
        if ((now - last_tx) >= 1000) {
            last_tx = now;
            uart_write_bytes(HMI_UART_PORT, "U", 1);
            ESP_LOGW(TAG, "[TEXT TEST] TX >> 'U'");
        }

        int n = uart_read_bytes(HMI_UART_PORT, rx_buf, sizeof(rx_buf) - 1, pdMS_TO_TICKS(50));
        if (n > 0) {
            rx_buf[n] = '\0';
            char logbuf[80];
            snprintf(logbuf, sizeof(logbuf), "[TEXT RX] %d bytes: '%s'", n, rx_buf);
            ESP_LOGW(TAG, "%s", logbuf);
            hmi_log(LOG_RX, logbuf);
        }
    }
}
#endif

#ifdef TEST_UART_RX_DISPLAY
//*************************************************************************************
// TEST_UART_RX_DISPLAY — formato: "REG XXXXXXXX  TRADUCCION"
//*************************************************************************************
static void rx_disp_log_frame(uint8_t reg, int32_t value)
{
#if defined(DEV_MODE) && defined(TEST_UART_RX_DISPLAY)
    if (!s_rx_log_enabled) return;
#endif
    static uint32_t last_joy_ms = 0;
    char meaning[34] = "";

    switch (reg) {
    case HMI_REG_ONLINE_INDICATOR:
        snprintf(meaning, sizeof(meaning), "ONLINE=%ld", (long)value);
        break;
    case HMI_REG_BLUETOOTH_INDICATOR:
        snprintf(meaning, sizeof(meaning), "BT=%ld", (long)value);
        break;
    case HMI_REG_CONSOLE_VOLTAGE:
        snprintf(meaning, sizeof(meaning), "CON_V %ldmV %d%%",
                 (long)value, battery_percent((uint16_t)value));
        break;
    case HMI_REG_ROBOT_VOLTAGE:
        snprintf(meaning, sizeof(meaning), "ROB_V %ldmV %d%%",
                 (long)value, battery_percent((uint16_t)value));
        break;
    case HMI_REG_ANGLE_X:
        snprintf(meaning, sizeof(meaning), "ANG_X=%ld", (long)value);
        break;
    case HMI_REG_ANGLE_Y:
        snprintf(meaning, sizeof(meaning), "ANG_Y=%ld", (long)value);
        break;
    case HMI_REG_ANGLE_HEAD_CHANGED:
        snprintf(meaning, sizeof(meaning), "HEAD=%ld", (long)value);
        break;
    case HMI_REG_ANGLE_NECK_CHANGED:
        snprintf(meaning, sizeof(meaning), "NECK=%ld", (long)value);
        break;
    case HMI_REG_ENCODER:
        snprintf(meaning, sizeof(meaning), "ENC=%ld", (long)value);
        break;
    case HMI_REG_CENTER: {
        int16_t neck = (int16_t)((value >> 16) & 0xFFFF);
        int16_t head = (int16_t)(value & 0xFFFF);
        snprintf(meaning, sizeof(meaning), "CENTER N:%d H:%d", neck, head);
        break;
    }
    case HMI_REG_JOY1:
    case HMI_REG_JOY2: {
        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
        if ((now_ms - last_joy_ms) < 1000) return;  // 1 log/s para JOY
        last_joy_ms = now_ms;
        int16_t x = (int16_t)((value >> 16) & 0xFFFF);
        int16_t y = (int16_t)(value & 0xFFFF);
        snprintf(meaning, sizeof(meaning), "%s X:%d Y:%d",
                 reg == HMI_REG_JOY1 ? "JOY1" : "JOY2", x, y);
        break;
    }
    case HMI_REG_BUTTONS: {
        bool j1 = (value >> 1) & 1;
        bool j2 = value & 1;
        snprintf(meaning, sizeof(meaning), "BTN J1=%d J2=%d", j1, j2);
        break;
    }
    case HMI_REG_PONG:
        snprintf(meaning, sizeof(meaning), "<<< PONG OK");
        break;
    case HMI_REG_P1:
        snprintf(meaning, sizeof(meaning), "P1=%d", (int)(value & 0xFFFF));
        break;
    case HMI_REG_MOTOR:
        snprintf(meaning, sizeof(meaning), "MOTOR cmd=%d vel=%d",
                 (int)((value >> 16) & 0xFFFF), (int)(value & 0xFFFF));
        break;
    case HMI_REG_SRV1_ANGLE:
        snprintf(meaning, sizeof(meaning), "SRV1_ANGLE=%d°", (int)value);
        break;
    case HMI_REG_SRV2_ANGLE:
        snprintf(meaning, sizeof(meaning), "SRV2_ANGLE=%d°", (int)value);
        break;
    case HMI_REG_SRV3_ANGLE:
        snprintf(meaning, sizeof(meaning), "SRV3_ANGLE=%d°", (int)value);
        break;
    case HMI_REG_OTA_STATUS:
        snprintf(meaning, sizeof(meaning), "OTA_STATUS=%ld", (long)value);
        break;
    case HMI_REG_FW_VERSION:
        snprintf(meaning, sizeof(meaning), "FW_VER=%u.%u.%u",
                 (unsigned)((value >> 16) & 0xFF), (unsigned)((value >> 8) & 0xFF), (unsigned)(value & 0xFF));
        break;
    case HMI_REG_WIFI_STATUS:
        snprintf(meaning, sizeof(meaning), "CONSOLE_WIFI=%ld", (long)value);
        break;
    default:
        snprintf(meaning, sizeof(meaning), "REG:%02X ?", reg);
        break;
    }

    hmi_log(LOG_RX, meaning);
#ifdef DEV_MODE
    dev_serial_add("RX", meaning);
#endif
}
#endif

#ifdef DEV_MODE
//*************************************************************************************
// DEV_MODE — panel de desarrollador, 3 taps en panel VERSION de System Info
//*************************************************************************************
#define DEV_UNLOCK_TAPS  3
#define DEV_UNLOCK_MS    2000
#define NVS_DEV_NS       "dev_cfg"
#define NVS_KEY_UART_TX  "uart_tx"
#define NVS_KEY_UART_RX  "uart_rx"
#define NVS_KEY_PIN        "dev_pin"
#define NVS_KEY_SERIAL     "serial_num"
#define NVS_KEY_VIS_PANELS "vis_panels"
#define NVS_KEY_DEV_NAME   "dev_name"
#define DEV_NAME_DEF       "WELLTEP Console"
#define DEV_NAME_MAX       32
#define NVS_KEY_WIFI_SSID  "wifi_ssid"
#define NVS_KEY_WIFI_PASS  "wifi_pass"
#define NVS_KEY_ENC_PERIM_CX100 "enc_perim"   // perimetro del rodillo, centesimas de mm
#define NVS_KEY_ENC_PPR         "enc_ppr"     // pulsos por vuelta del encoder

//*************************************************************************************
// GIRO AUTOMATICO PERSONALIZADO — "Control por Puntos" / "Config Auto Rotation"
//*************************************************************************************
#define RECORRIDO_MAX_POINTS 10
#define RECORRIDO_COUNT      2
#define NVS_KEY_RECORRIDO1   "auto_rec1"
#define NVS_KEY_RECORRIDO2   "auto_rec2"
#define NVS_KEY_SRV_LIMITS   "srv_limits"

typedef struct {
    int16_t head;
    int16_t neck;
} recorrido_point_t;

typedef struct {
    uint8_t            count;                        // puntos guardados (0-RECORRIDO_MAX_POINTS)
    recorrido_point_t  points[RECORRIDO_MAX_POINTS];
    // speed va AL FINAL a proposito: asi los recorridos ya guardados en NVS
    // (blob binario tal cual, ver recorrido_nvs_read/write) siguen leyendo
    // "points[]" en el mismo offset de siempre. Si "speed" fuera antes de
    // "points[]" con este tipo mas grande (uint8_t -> uint16_t), el blob
    // viejo se desalinearia y corromperia los puntos guardados de recorridos
    // existentes en equipos ya actualizados — con "speed" al final, lo unico
    // que pasa con datos viejos es que ese campo no se lee (se resetea al
    // default via el clamp de abajo), sin tocar los puntos.
    uint16_t           speed;                         // ms por grado (5-500, paso 5), valido solo durante reproduccion
} recorrido_t;

typedef struct {
    int16_t min;
    int16_t max;
} servo_limit_t;

typedef struct {
    servo_limit_t srv1, srv2, srv3;
} servo_limits_t;

static void recorrido_nvs_read(uint8_t idx, recorrido_t *out)
{
    memset(out, 0, sizeof(*out));
    out->speed = 150;
    nvs_handle_t h;
    const char *key = (idx == 0) ? NVS_KEY_RECORRIDO1 : NVS_KEY_RECORRIDO2;
    if (nvs_open(NVS_DEV_NS, NVS_READONLY, &h) == ESP_OK) {
        size_t sz = sizeof(*out);
        nvs_get_blob(h, key, out, &sz);
        nvs_close(h);
    }
    if (out->count > RECORRIDO_MAX_POINTS) out->count = RECORRIDO_MAX_POINTS;
    if (out->speed < 5 || out->speed > 500) out->speed = 150;
}

static void recorrido_nvs_write(uint8_t idx, const recorrido_t *in)
{
    nvs_handle_t h;
    const char *key = (idx == 0) ? NVS_KEY_RECORRIDO1 : NVS_KEY_RECORRIDO2;
    if (nvs_open(NVS_DEV_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_blob(h, key, in, sizeof(*in));
        nvs_commit(h);
        nvs_close(h);
    }
}

static void srv_limits_nvs_read(servo_limits_t *out)
{
    out->srv1.min = out->srv2.min = out->srv3.min = 0;
    out->srv1.max = out->srv2.max = out->srv3.max = 270;
    nvs_handle_t h;
    if (nvs_open(NVS_DEV_NS, NVS_READONLY, &h) == ESP_OK) {
        size_t sz = sizeof(*out);
        nvs_get_blob(h, NVS_KEY_SRV_LIMITS, out, &sz);
        nvs_close(h);
    }
}

static void srv_limits_nvs_write(const servo_limits_t *in)
{
    nvs_handle_t h;
    if (nvs_open(NVS_DEV_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_blob(h, NVS_KEY_SRV_LIMITS, in, sizeof(*in));
        nvs_commit(h);
        nvs_close(h);
    }
}
// ================================================================
// Giro Automatico personalizado — estado en RAM + motor de reproduccion.
// Los +/- de cabeza/cuello de estas 2 pantallas nuevas (a diferencia de los
// originales de MODES, que solo mandan el registro cuando se toca el label)
// mandan HMI_REG_ANGLE_*_CHANGED en cada toque: el objetivo aca es ver al
// robot moverse en vivo mientras se posiciona un centro/punto a grabar.
// ================================================================
#define NVS_KEY_CENTER_HEAD "auto_c_head"
#define NVS_KEY_CENTER_NECK "auto_c_neck"

// Forward declarations — dev_nvs_read_pin/dev_nvs_write_pin se definen mas
// abajo, en la seccion DEV_MODE, pero ya existian antes de este bloque.
static int  dev_nvs_read_pin(const char *key, int def);
static void dev_nvs_write_pin(const char *key, int val);

static recorrido_t     g_recorridos[RECORRIDO_COUNT];
static servo_limits_t  g_srv_limits;
static int32_t         g_center_head = 90, g_center_neck = 90;
static uint8_t         g_editing_recorrido = 0;
static int             g_editor_selected_point = -1; // -1 = el proximo "Guardar punto" agrega nuevo
static int             g_selected_recorrido = -1;    // -1 = ninguno elegido para Iniciar Giro Automatico
static int             g_ar_playing_point_idx = -1;  // indice del punto que el Test esta ejecutando ahora (-1 = ninguno)

typedef enum {
    MODES_VIEW_HOME = 0,
    MODES_VIEW_CONTROL_PUNTOS,
    MODES_VIEW_AUTOROT_PICKER,
    MODES_VIEW_AUTOROT_EDITOR,
} modes_view_t;

typedef enum {
    AUTO_PLAY_IDLE = 0,
    AUTO_PLAY_ONE_SHOT_MOVE,   // movimiento simple (ej. al entrar a una pantalla, o probar un punto)
    AUTO_PLAY_MOVING_TO_START,
    AUTO_PLAY_PLAYING,
    AUTO_PLAY_PAUSED,
    AUTO_PLAY_RETURNING_CENTER,
} auto_play_state_t;

static auto_play_state_t g_play_state      = AUTO_PLAY_IDLE;
static auto_play_state_t g_pre_pause_state = AUTO_PLAY_IDLE;
static uint8_t   g_play_recorrido_idx = 0;
static uint8_t   g_play_point_idx     = 0;
static int32_t   g_move_target_head = 90, g_move_target_neck = 90;
static int32_t   g_move_step_deg    = 2;
static lv_timer_t *g_auto_rotation_timer = NULL;

static void auto_rotation_editor_refresh_list(void);
static void auto_rotation_editor_refresh_row_colors(void);
static void ar_test_set_running(bool running);

// Muestra exactamente una de las 4 "vistas" de MODES: la grilla+paneles de
// angulo originales (home), o una de las 3 pantallas completas nuevas.
static void modes_show_view(modes_view_t view)
{
    // Los paneles de angulo cabeza/cuello YA NO viven en "home": screens.c
    // los reubico dentro de modes_control_puntos_panel, asi que ocultarse/
    // mostrarse con ese panel les llega automatico (LVGL oculta subarboles
    // enteros). Home ahora es solo la grilla de 6 botones.
    if (objects.modes_grid_panel) {
        if (view == MODES_VIEW_HOME) lv_obj_remove_flag(objects.modes_grid_panel, LV_OBJ_FLAG_HIDDEN);
        else                          lv_obj_add_flag(objects.modes_grid_panel, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_t *sub_objs[3] = { objects.modes_control_puntos_panel, objects.modes_autorot_picker_panel, objects.modes_autorot_editor_panel };
    for (int i = 0; i < 3; i++) {
        if (!sub_objs[i]) continue;
        bool show = (view == (modes_view_t)(i + 1));
        if (show) lv_obj_remove_flag(sub_objs[i], LV_OBJ_FLAG_HIDDEN);
        else      lv_obj_add_flag(sub_objs[i], LV_OBJ_FLAG_HIDDEN);
    }
}

// Refleja cabeza/cuello en los 2 lugares que pueden mostrarlo (Control por
// Puntos vive en objects.angle_head_label/angle_neck_label, reubicados ahi;
// el editor de puntos tiene su propia copia ar_angle_*_label) — a lo sumo
// uno esta visible, pero es mas simple mantener ambos sincronizados que
// averiguar cual esta activo.
static void auto_rotation_sync_labels(int32_t head, int32_t neck)
{
    char buff[10];
    lv_snprintf(buff, sizeof(buff), "%03d", (int)head);
    if (objects.angle_head_label) lv_label_set_text(objects.angle_head_label, buff);
    lv_snprintf(buff, sizeof(buff), "%03d", (int)neck);
    if (objects.angle_neck_label) lv_label_set_text(objects.angle_neck_label, buff);

    // Las tarjetas del editor muestran el grado con el simbolo "°" (mockup
    // del usuario) — buffer separado para no afectar el display de
    // Control por Puntos/home, que sigue sin simbolo.
    char buff_deg[12];
    lv_snprintf(buff_deg, sizeof(buff_deg), "%03d\xC2\xB0", (int)head);
    if (objects.ar_angle_head_label) lv_label_set_text(objects.ar_angle_head_label, buff_deg);
    lv_snprintf(buff_deg, sizeof(buff_deg), "%03d\xC2\xB0", (int)neck);
    if (objects.ar_angle_neck_label) lv_label_set_text(objects.ar_angle_neck_label, buff_deg);
}

// Cambia el texto del boton "Iniciar Giro Automatico"/"Pausar"/"Reanudar" Y
// su resaltado visual (amarillo/accent mientras hay una reproduccion en
// curso — moviendo, reproduciendo o pausada; color neutro en idle). Antes
// solo cambiaba el texto, y sin el resaltado no habia forma de ver a simple
// vista que el giro automatico estaba corriendo.
static void modes_giro_automatico_set_state(const char *txt, bool active)
{
    if (objects.obj23) lv_label_set_text(objects.obj23, txt);
    hmi_style_btn(objects.btn_giro_automatico, active);
}

// Reaplica el resaltado activo/inactivo tras un cambio de tema (SET_CARD lo
// pisaria con el color neutro). Override fuerte del weak-hook de actions.c.
void hmi_modes_giro_retheme(void)
{
    bool active = (g_play_state == AUTO_PLAY_MOVING_TO_START ||
                   g_play_state == AUTO_PLAY_PLAYING ||
                   g_play_state == AUTO_PLAY_PAUSED);
    hmi_style_btn(objects.btn_giro_automatico, active);
}

// Usada solo por los callbacks ar_angle_*_dec/inc_cb del editor de puntos
// (Control por Puntos/home reusan los botones ORIGINALES con sus propios
// callbacks action_angle_*_btn_* en actions.c) — por eso el simbolo "°" fijo
// en el label aca no afecta a esas otras pantallas.
static void angle_axis_bump(lv_obj_t *label, bool is_head, int32_t delta)
{
    servo_limit_t lim = is_head ? g_srv_limits.srv1 : g_srv_limits.srv2;
    int32_t value = is_head ? get_var_angle_head_value() : get_var_angle_neck_value();
    value += delta;
    if (value < lim.min) value = lim.min;
    if (value > lim.max) value = lim.max;
    if (is_head) set_var_angle_head_value(value); else set_var_angle_neck_value(value);
    if (label) {
        char buff[12];
        lv_snprintf(buff, sizeof(buff), "%03d\xC2\xB0", (int)value);
        lv_label_set_text(label, buff);
    }
    hmi_send_data(is_head ? HMI_REG_ANGLE_HEAD_CHANGED : HMI_REG_ANGLE_NECK_CHANGED, value);
}

static void auto_rotation_queue_next_point(void)
{
    recorrido_t *r = &g_recorridos[g_play_recorrido_idx];
    uint8_t next = g_play_point_idx + 1;
    if (next >= r->count) {
        g_play_state = AUTO_PLAY_RETURNING_CENTER;
        g_move_target_head = g_center_head;
        g_move_target_neck = g_center_neck;
        g_move_step_deg = 3;
        lv_timer_set_period(g_auto_rotation_timer, 100);
        if (g_play_recorrido_idx == g_editing_recorrido) {
            g_ar_playing_point_idx = -1;
            auto_rotation_editor_refresh_row_colors();
        }
        return;
    }
    g_play_point_idx = next;
    g_move_target_head = r->points[next].head;
    g_move_target_neck = r->points[next].neck;
    // r->speed ya es directamente el periodo en ms por grado (5-500, paso
    // 5 desde los botones +/-, mantener presionado repite): menos ms = mas
    // rapido. Paso fijo de 1 grado por tick, y el periodo del timer (no el
    // paso) es lo que varia.
    g_move_step_deg = 1;
    lv_timer_set_period(g_auto_rotation_timer, r->speed);
    // Resalta en la lista el punto que se esta ejecutando ahora (solo si el
    // editor tiene abierto el mismo recorrido que se esta reproduciendo).
    if (g_play_recorrido_idx == g_editing_recorrido) {
        g_ar_playing_point_idx = (int)next;
        auto_rotation_editor_refresh_row_colors();
    }
}

static void auto_rotation_on_target_reached(void)
{
    switch (g_play_state) {
    case AUTO_PLAY_ONE_SHOT_MOVE:
        g_play_state = AUTO_PLAY_IDLE;
        if (g_auto_rotation_timer) lv_timer_pause(g_auto_rotation_timer);
        ar_test_set_running(false);
        break;
    case AUTO_PLAY_MOVING_TO_START:
        g_play_state = AUTO_PLAY_PLAYING;
        g_play_point_idx = 0;
        auto_rotation_queue_next_point();
        break;
    case AUTO_PLAY_PLAYING:
        auto_rotation_queue_next_point();
        break;
    case AUTO_PLAY_RETURNING_CENTER:
        g_play_state = AUTO_PLAY_IDLE;
        if (g_auto_rotation_timer) lv_timer_pause(g_auto_rotation_timer);
        modes_giro_automatico_set_state(g_lang->btn_auto_rotation, false);
        ar_test_set_running(false);
        break;
    default:
        break;
    }
}

static void auto_rotation_timer_cb(lv_timer_t *t)
{
    (void)t;
    int32_t head = get_var_angle_head_value();
    int32_t neck = get_var_angle_neck_value();
    bool head_done = (head == g_move_target_head);
    bool neck_done = (neck == g_move_target_neck);
    if (!head_done) {
        int32_t d = g_move_target_head - head;
        int32_t step = (d > 0) ? (d < g_move_step_deg ? d : g_move_step_deg)
                                : (d > -g_move_step_deg ? d : -g_move_step_deg);
        head += step;
        set_var_angle_head_value(head);
        hmi_send_data(HMI_REG_ANGLE_HEAD_CHANGED, head);
        head_done = (head == g_move_target_head);
    }
    if (!neck_done) {
        int32_t d = g_move_target_neck - neck;
        int32_t step = (d > 0) ? (d < g_move_step_deg ? d : g_move_step_deg)
                                : (d > -g_move_step_deg ? d : -g_move_step_deg);
        neck += step;
        set_var_angle_neck_value(neck);
        hmi_send_data(HMI_REG_ANGLE_NECK_CHANGED, neck);
        neck_done = (neck == g_move_target_neck);
    }
    auto_rotation_sync_labels(head, neck);
    if (head_done && neck_done) auto_rotation_on_target_reached();
}

static void auto_rotation_ensure_timer(void)
{
    // 100ms/tick: con g_move_step_deg = velocidad(grados), 1 grado dura
    // exactamente 100ms en SPEED=1 (10°/s), 50ms en SPEED=2 (20°/s), etc.
    // Comparte timer con "Test" (paso=2) y el regreso al centro (paso=3),
    // que tambien quedan mas lentos que antes (era de 40ms).
    if (!g_auto_rotation_timer) g_auto_rotation_timer = lv_timer_create(auto_rotation_timer_cb, 100, NULL);
}

static void auto_rotation_start_one_shot_move(int32_t target_head, int32_t target_neck)
{
    auto_rotation_ensure_timer();
    g_play_state = AUTO_PLAY_ONE_SHOT_MOVE;
    g_move_target_head = target_head;
    g_move_target_neck = target_neck;
    g_move_step_deg = 2;
    lv_timer_set_period(g_auto_rotation_timer, 100);
    lv_timer_resume(g_auto_rotation_timer);
}

static void hmi_auto_rotation_play_start(void)
{
    if (g_selected_recorrido < 0) return;
    recorrido_t *r = &g_recorridos[g_selected_recorrido];
    if (r->count == 0) return;
    auto_rotation_ensure_timer();
    g_play_recorrido_idx = (uint8_t)g_selected_recorrido;
    g_play_state = AUTO_PLAY_MOVING_TO_START;
    g_move_target_head = r->points[0].head;
    g_move_target_neck = r->points[0].neck;
    g_play_point_idx = 0;
    // r->speed ya es directamente el periodo en ms, ver auto_rotation_queue_next_point().
    g_move_step_deg = 1;
    lv_timer_set_period(g_auto_rotation_timer, r->speed);
    lv_timer_resume(g_auto_rotation_timer);
    modes_giro_automatico_set_state(g_lang->btn_pausar_auto_rotation, true);
    if (g_play_recorrido_idx == g_editing_recorrido) {
        g_ar_playing_point_idx = 0;
        auto_rotation_editor_refresh_row_colors();
    }
    ar_test_set_running(true);
}

// ---- MODES: grilla principal ----

void modes_btn_control_por_puntos_cb(lv_event_t *e)
{
    (void)e;
    modes_show_view(MODES_VIEW_CONTROL_PUNTOS);
    auto_rotation_sync_labels(get_var_angle_head_value(), get_var_angle_neck_value());
    if (g_play_state == AUTO_PLAY_IDLE) auto_rotation_start_one_shot_move(g_center_head, g_center_neck);
}

void modes_btn_config_auto_rotation_cb(lv_event_t *e)
{
    (void)e;
    modes_show_view(MODES_VIEW_AUTOROT_PICKER);
}

void modes_btn_giro_automatico_cb(lv_event_t *e)
{
    (void)e;
    switch (g_play_state) {
    case AUTO_PLAY_IDLE:
        hmi_auto_rotation_play_start();
        break;
    case AUTO_PLAY_MOVING_TO_START:
    case AUTO_PLAY_PLAYING:
        g_pre_pause_state = g_play_state;
        g_play_state = AUTO_PLAY_PAUSED;
        if (g_auto_rotation_timer) lv_timer_pause(g_auto_rotation_timer);
        modes_giro_automatico_set_state(g_lang->btn_reanudar_auto_rotation, true);
        break;
    case AUTO_PLAY_PAUSED:
        g_play_state = g_pre_pause_state;
        if (g_auto_rotation_timer) lv_timer_resume(g_auto_rotation_timer);
        modes_giro_automatico_set_state(g_lang->btn_pausar_auto_rotation, true);
        break;
    default:
        break; // RETURNING_CENTER / ONE_SHOT_MOVE: este boton no responde
    }
}

// Detiene cualquier reproduccion en curso (Iniciar Giro Automatico de MODES
// home, o Test del editor) y la manda de vuelta al centro — mismo
// comportamiento sea cual sea el boton que la disparo, asi los dos quedan
// consistentes entre si.
static void auto_rotation_stop_and_return_center(void)
{
    if (g_play_state == AUTO_PLAY_IDLE || g_play_state == AUTO_PLAY_ONE_SHOT_MOVE) return;
    auto_rotation_ensure_timer();
    g_play_state = AUTO_PLAY_RETURNING_CENTER;
    g_move_target_head = g_center_head;
    g_move_target_neck = g_center_neck;
    g_move_step_deg = 3;
    lv_timer_set_period(g_auto_rotation_timer, 100);
    lv_timer_resume(g_auto_rotation_timer);
    if (g_play_recorrido_idx == g_editing_recorrido) {
        g_ar_playing_point_idx = -1;
        auto_rotation_editor_refresh_row_colors();
    }
    modes_giro_automatico_set_state(g_lang->btn_auto_rotation, false);
    ar_test_set_running(false);
}

void modes_btn_stop_giro_automatico_cb(lv_event_t *e) { (void)e; auto_rotation_stop_and_return_center(); }

// ---- Control por Puntos ----
// Los +/- de cabeza/cuello son los originales de MODES, reubicados por
// screens.c dentro de este panel — conservan sus callbacks de siempre
// (action_angle_*_btn_* en actions.c), no hace falta nada nuevo aca.

void cp_btn_guardar_centrado_cb(lv_event_t *e)
{
    (void)e;
    int32_t head = get_var_angle_head_value();
    int32_t neck = get_var_angle_neck_value();
    int32_t packed = (neck << 16) | (head & 0xFFFF);
    hmi_send_data(HMI_REG_CENTER, packed);
    dev_nvs_write_pin(NVS_KEY_CENTER_HEAD, (int)head);
    dev_nvs_write_pin(NVS_KEY_CENTER_NECK, (int)neck);
    g_center_head = head;
    g_center_neck = neck;
}

void cp_btn_volver_cb(lv_event_t *e) { (void)e; modes_show_view(MODES_VIEW_HOME); }

// ---- Config Auto Rotation — selector de recorrido ----

static void auto_rotation_editor_open(uint8_t idx)
{
    g_editing_recorrido = idx;
    g_selected_recorrido = idx;
    g_editor_selected_point = -1;
    // Si ya hay una reproduccion en curso de este mismo recorrido (ej. se
    // inicio desde MODES home y se entro al editor despues), sincroniza el
    // resaltado de punto y el boton Test/Detener con el estado real en vez
    // de asumir que no hay nada corriendo.
    bool playing_this = (g_play_recorrido_idx == idx) &&
                         (g_play_state == AUTO_PLAY_MOVING_TO_START ||
                          g_play_state == AUTO_PLAY_PLAYING ||
                          g_play_state == AUTO_PLAY_PAUSED);
    g_ar_playing_point_idx = playing_this ? (int)g_play_point_idx : -1;
    ar_test_set_running(playing_this);
    modes_show_view(MODES_VIEW_AUTOROT_EDITOR);
    if (objects.ar_title_label) {
        char title[24];
        lv_snprintf(title, sizeof(title), " RECORRIDO %d", (int)idx + 1);
        lv_label_set_text(objects.ar_title_label, title);
    }
    if (objects.ar_speed_label) {
        char sbuf[8];
        lv_snprintf(sbuf, sizeof(sbuf), "%d ms", (int)g_recorridos[idx].speed);
        lv_label_set_text(objects.ar_speed_label, sbuf);
    }
    auto_rotation_sync_labels(get_var_angle_head_value(), get_var_angle_neck_value());
    auto_rotation_editor_refresh_list();
    if (g_play_state == AUTO_PLAY_IDLE) auto_rotation_start_one_shot_move(g_center_head, g_center_neck);
}

void autorot_btn_recorrido1_cb(lv_event_t *e) { (void)e; auto_rotation_editor_open(0); }
void autorot_btn_recorrido2_cb(lv_event_t *e) { (void)e; auto_rotation_editor_open(1); }
void autorot_picker_btn_volver_cb(lv_event_t *e) { (void)e; modes_show_view(MODES_VIEW_HOME); }

// ---- Config Auto Rotation — editor de puntos ----

void ar_angle_neck_dec_cb(lv_event_t *e) { (void)e; angle_axis_bump(objects.ar_angle_neck_label, false, -1); }
void ar_angle_neck_inc_cb(lv_event_t *e) { (void)e; angle_axis_bump(objects.ar_angle_neck_label, false,  1); }
void ar_angle_head_dec_cb(lv_event_t *e) { (void)e; angle_axis_bump(objects.ar_angle_head_label, true,  -1); }
void ar_angle_head_inc_cb(lv_event_t *e) { (void)e; angle_axis_bump(objects.ar_angle_head_label, true,   1); }

// Repinta cada fila de ar_points_list segun su rol actual: verde si es el
// punto que el Test esta ejecutando ahora mismo (g_ar_playing_point_idx),
// amarillo oscuro si es el punto tocado/seleccionado para editar
// (g_editor_selected_point), gris oscuro por defecto. Recalcula todo por
// indice en vez de guardar punteros a filas — asi sigue siendo correcto
// despues de que auto_rotation_editor_refresh_list() destruye y recrea las
// filas (ej. al guardar o borrar un punto).
static void auto_rotation_editor_refresh_row_colors(void)
{
    if (!objects.ar_points_list) return;
    uint32_t n = lv_obj_get_child_count(objects.ar_points_list);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *row = lv_obj_get_child(objects.ar_points_list, i);
        if (!row) continue;
        // Playing/selected quedan con su color semantico fijo (verde/oliva) en
        // cualquier tema; el resto de las filas sigue al tema activo.
        if ((int)i == g_ar_playing_point_idx) {
            lv_obj_set_style_bg_color(row, lv_color_hex(0xff1e6b3a), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else if ((int)i == g_editor_selected_point) {
            lv_obj_set_style_bg_color(row, lv_color_hex(0xff4a3f0a), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            lv_obj_set_style_bg_color(row, hmi_theme_bg_indicator(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(row, hmi_theme_bd_card(), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_t *lbl = lv_obj_get_child(row, lv_obj_get_child_count(row) - 1);
            if (lbl) lv_obj_set_style_text_color(lbl, hmi_theme_txt_primary(), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

// Cambia el boton "Test" a "Detener" (y su color) mientras el Test esta
// reproduciendo un recorrido completo, y de vuelta a "Test" cuando termina o
// se lo detiene manualmente — mismo patron que modes_giro_automatico_set_state
// para el boton de MODES home, pero aplicado al boton del editor.
static bool s_ar_test_running = false; // ultimo estado, para poder reaplicarlo al cambiar de tema

static void ar_test_set_running(bool running)
{
    s_ar_test_running = running;
    if (!objects.ar_btn_probar) return;
    lv_obj_t *lbl = lv_obj_get_child(objects.ar_btn_probar, 0);
    if (lbl) lv_label_set_text(lbl, running ? g_lang->btn_detener_prueba : g_lang->btn_probar);
    // "Corriendo" (rojo) es un color semantico fijo; en reposo, el borde
    // sigue al tema activo.
    lv_color_t border = running ? lv_color_hex(0xffbc0f2d) : hmi_theme_bd_card();
    lv_obj_set_style_border_color(objects.ar_btn_probar, border, LV_PART_MAIN | LV_STATE_DEFAULT);
}

// Reaplica los colores de esta pantalla (filas de la lista + borde del boton
// Test) cuando el usuario cambia de tema — llamado desde apply_theme() via
// theme_autorot_editor_panel() (actions.c). Los elementos que SI viven en
// screens.c (tarjetas, botones +/-, etc.) ya los recolorea esa funcion
// directamente; esto cubre solo lo que se crea/actualiza en runtime aca.
void hmi_autorot_editor_retheme(void)
{
    auto_rotation_editor_refresh_row_colors();
    ar_test_set_running(s_ar_test_running);
}

void ar_point_row_clicked_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    recorrido_t *r = &g_recorridos[g_editing_recorrido];
    if (idx < 0 || idx >= r->count) return;
    g_editor_selected_point = idx;
    recorrido_point_t p = r->points[idx];
    set_var_angle_head_value(p.head);
    set_var_angle_neck_value(p.neck);
    auto_rotation_sync_labels(p.head, p.neck);
    if (g_play_state == AUTO_PLAY_IDLE) auto_rotation_start_one_shot_move(p.head, p.neck);

    // Resalta la fila tocada — asi queda claro cual punto se va a
    // sobreescribir con "Guardar punto" o borrar con "Eliminar".
    auto_rotation_editor_refresh_row_colors();
}

static void auto_rotation_editor_refresh_list(void)
{
    if (!objects.ar_points_list) return;
    lv_obj_clean(objects.ar_points_list); // destruye las filas de abajo
    recorrido_t *r = &g_recorridos[g_editing_recorrido];
    for (uint8_t i = 0; i < r->count; i++) {
        // Formato corto en una sola linea, indice + neck + head (orden y
        // formato del mockup del usuario: "01      N96 H13").
        char buf[24];
        lv_snprintf(buf, sizeof(buf), "%02d      H%d N%d", (int)i + 1, (int)r->points[i].head, (int)r->points[i].neck);
        lv_obj_t *btn = lv_list_add_btn(objects.ar_points_list, NULL, buf);
        lv_obj_add_event_cb(btn, ar_point_row_clicked_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        // Colores (bg/borde/texto) los pone auto_rotation_editor_refresh_row_colors()
        // mas abajo, que ya sigue al tema activo — aca solo el modo de recorte.
        lv_obj_t *lbl = lv_obj_get_child(btn, lv_obj_get_child_count(btn) - 1);
        if (lbl) lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    }

    // Alto dinamico: con pocos puntos guardados, un alto fijo dejaba un
    // marco negro enorme y vacio debajo de la ultima fila. Se mide el alto
    // real del contenido (LV_SIZE_CONTENT + update_layout forzado) y se
    // usa eso, con un piso minimo (para que la lista vacia no desaparezca)
    // y un techo (mas alla de eso, scrollea adentro en vez de seguir
    // creciendo y empujar el resto de la pantalla). Confirmado que este
    // comportamiento es el que se queria — no volver a tocar sin pedido
    // explicito.
    lv_obj_set_height(objects.ar_points_list, LV_SIZE_CONTENT);
    lv_obj_update_layout(objects.ar_points_list);
    lv_coord_t natural_h = lv_obj_get_height(objects.ar_points_list);
    const lv_coord_t LIST_MIN_H = 70;
    const lv_coord_t LIST_MAX_H = 210;
    if (natural_h < LIST_MIN_H) natural_h = LIST_MIN_H;
    if (natural_h > LIST_MAX_H) natural_h = LIST_MAX_H;
    lv_obj_set_height(objects.ar_points_list, natural_h);
    auto_rotation_editor_refresh_row_colors();
}

void ar_btn_guardar_punto_cb(lv_event_t *e)
{
    (void)e;
    recorrido_t *r = &g_recorridos[g_editing_recorrido];
    int32_t head = get_var_angle_head_value();
    int32_t neck = get_var_angle_neck_value();
    if (g_editor_selected_point >= 0 && g_editor_selected_point < r->count) {
        r->points[g_editor_selected_point].head = (int16_t)head;
        r->points[g_editor_selected_point].neck = (int16_t)neck;
    } else if (r->count < RECORRIDO_MAX_POINTS) {
        r->points[r->count].head = (int16_t)head;
        r->points[r->count].neck = (int16_t)neck;
        r->count++;
    } else {
        return; // recorrido lleno (10 puntos), no hace nada
    }
    g_editor_selected_point = -1;
    recorrido_nvs_write(g_editing_recorrido, r);
    auto_rotation_editor_refresh_list();
}

// Borra el punto actualmente resaltado en la lista (el que se toco para
// cargarlo). Si no hay ninguno tocado, no hace nada — evita un borrado por
// error de "el ultimo punto" sin que el usuario haya elegido cual.
void ar_btn_eliminar_punto_cb(lv_event_t *e)
{
    (void)e;
    recorrido_t *r = &g_recorridos[g_editing_recorrido];
    if (g_editor_selected_point < 0 || g_editor_selected_point >= r->count) return;
    for (int i = g_editor_selected_point; i < r->count - 1; i++) {
        r->points[i] = r->points[i + 1];
    }
    r->count--;
    g_editor_selected_point = -1;
    recorrido_nvs_write(g_editing_recorrido, r);
    auto_rotation_editor_refresh_list();
}

void ar_speed_dec_cb(lv_event_t *e)
{
    (void)e;
    recorrido_t *r = &g_recorridos[g_editing_recorrido];
    if (r->speed > 5) r->speed -= 5;
    recorrido_nvs_write(g_editing_recorrido, r);
    if (objects.ar_speed_label) { char buf[8]; lv_snprintf(buf, sizeof(buf), "%d ms", (int)r->speed); lv_label_set_text(objects.ar_speed_label, buf); }
}

void ar_speed_inc_cb(lv_event_t *e)
{
    (void)e;
    recorrido_t *r = &g_recorridos[g_editing_recorrido];
    if (r->speed < 500) r->speed += 5;
    recorrido_nvs_write(g_editing_recorrido, r);
    if (objects.ar_speed_label) { char buf[8]; lv_snprintf(buf, sizeof(buf), "%d ms", (int)r->speed); lv_label_set_text(objects.ar_speed_label, buf); }
}

void ar_btn_probar_cb(lv_event_t *e)
{
    (void)e;
    if (g_play_state == AUTO_PLAY_IDLE) {
        g_selected_recorrido = g_editing_recorrido;
        hmi_auto_rotation_play_start();
    } else if (g_play_state == AUTO_PLAY_MOVING_TO_START ||
               g_play_state == AUTO_PLAY_PLAYING ||
               g_play_state == AUTO_PLAY_PAUSED) {
        // Vuelve a tocar "Test" (ahora "Detener") mientras esta corriendo:
        // lo frena y lo manda de vuelta al centro, igual que "Cancelar" en
        // MODES home.
        auto_rotation_stop_and_return_center();
    }
    // ONE_SHOT_MOVE / RETURNING_CENTER: sin efecto, ya se esta deteniendo o
    // es un movimiento de otro origen ajeno al Test.
}

void ar_btn_volver_cb(lv_event_t *e) { (void)e; modes_show_view(MODES_VIEW_AUTOROT_PICKER); }

// ---- Settings > Limites de Servo ----

void hmi_srv_limits_load_to_ui(void)
{
    lv_obj_t *lbls[6] = {
        objects.sl_srv1_min_label, objects.sl_srv1_max_label,
        objects.sl_srv2_min_label, objects.sl_srv2_max_label,
        objects.sl_srv3_min_label, objects.sl_srv3_max_label,
    };
    int16_t vals[6] = {
        g_srv_limits.srv1.min, g_srv_limits.srv1.max,
        g_srv_limits.srv2.min, g_srv_limits.srv2.max,
        g_srv_limits.srv3.min, g_srv_limits.srv3.max,
    };
    for (int i = 0; i < 6; i++) {
        if (!lbls[i]) continue;
        char buf[6];
        lv_snprintf(buf, sizeof(buf), "%03d", (int)vals[i]);
        lv_label_set_text(lbls[i], buf);
    }
}

void sl_limit_btn_cb(lv_event_t *e)
{
    int code = (int)(intptr_t)lv_event_get_user_data(e);
    int servo_idx = code / 100;      // 0,1,2
    bool is_max   = (code % 100) >= 10;
    bool increase = (code % 10) != 0;
    servo_limit_t *lim = (servo_idx == 0) ? &g_srv_limits.srv1 : (servo_idx == 1) ? &g_srv_limits.srv2 : &g_srv_limits.srv3;
    int16_t *val = is_max ? &lim->max : &lim->min;
    int16_t delta = increase ? 1 : -1;
    int32_t nv = (int32_t)*val + delta;
    if (nv < 0) nv = 0;
    if (nv > 270) nv = 270;
    // El minimo no puede superar al maximo, ni el maximo bajar del minimo
    if (is_max && nv < lim->min) nv = lim->min;
    if (!is_max && nv > lim->max) nv = lim->max;
    *val = (int16_t)nv;
    hmi_srv_limits_load_to_ui();
}

void sl_btn_guardar_cb(lv_event_t *e)
{
    (void)e;
    srv_limits_nvs_write(&g_srv_limits);
    int32_t p1 = ((int32_t)g_srv_limits.srv1.max << 16) | (g_srv_limits.srv1.min & 0xFFFF);
    int32_t p2 = ((int32_t)g_srv_limits.srv2.max << 16) | (g_srv_limits.srv2.min & 0xFFFF);
    int32_t p3 = ((int32_t)g_srv_limits.srv3.max << 16) | (g_srv_limits.srv3.min & 0xFFFF);
    hmi_send_data(HMI_REG_SRV1_LIMITS, p1);
    hmi_send_data(HMI_REG_SRV2_LIMITS, p2);
    hmi_send_data(HMI_REG_SRV3_LIMITS, p3);
}

// Llamar una vez desde app_main, despues de que los paneles de MODES/Settings
// existan (create_panel_modes_auto_rotation / create_panel_settings_srv_limits).
void hmi_auto_rotation_init(void)
{
    for (int i = 0; i < RECORRIDO_COUNT; i++) recorrido_nvs_read((uint8_t)i, &g_recorridos[i]);
    srv_limits_nvs_read(&g_srv_limits);
    g_center_head = dev_nvs_read_pin(NVS_KEY_CENTER_HEAD, 90);
    g_center_neck = dev_nvs_read_pin(NVS_KEY_CENTER_NECK, 90);
    modes_show_view(MODES_VIEW_HOME);
    modes_giro_automatico_set_state(g_lang->btn_auto_rotation, false);
}

#define NVS_KEY_ENC_MODE_FEET   "enc_mode_ft"  // 0=Metros, 1=Pies (toggle de GENERAL CONTROLS)
#define ENC_PERIM_CX100_DEF     8520          // 85.20 mm
#define ENC_PPR_DEF              600
#define VIS_LOG  (1 << 0)
#define VIS_VAL  (1 << 1)
#define VIS_JOY  (1 << 2)
#define VIS_SER  (1 << 3)
#define VIS_TEST (1 << 4)
static uint8_t s_vis_panels = 0;
#define DEV_PIN_LEN      4
#define DEV_PIN_DEF      "1234"   // contraseña por defecto (editable aqui)
#define SERIAL_LOG_MAX   30       // entradas en el buffer del monitor serial
#define SERIAL_LOG_LEN   54       // longitud maxima por entrada
#define JOY_LOG_MAX      30       // entradas en el buffer del monitor joystick
#define JOY_LOG_LEN      88       // longitud maxima por entrada

static lv_obj_t   *s_dev_panel      = NULL;
static lv_obj_t   *s_dev_btn        = NULL;  // boton DEV en barra de navegacion
static lv_obj_t   *s_dev_heap_label = NULL;
static lv_timer_t *s_dev_heap_timer = NULL;
static lv_obj_t   *s_dot_label      = NULL;
static lv_timer_t *s_dot_timer      = NULL;

// PIN keypad
static lv_obj_t   *s_pin_panel     = NULL;
static lv_obj_t   *s_pin_display   = NULL;
static lv_timer_t *s_pin_err_timer = NULL;
static char        s_pin_buf[DEV_PIN_LEN + 1];
static int         s_pin_len       = 0;
static bool        s_pin_changing  = false;


// Serial number editor
static lv_obj_t   *s_sn_panel   = NULL;
static lv_obj_t   *s_sn_display = NULL;
static char        s_sn_buf[SERIAL_NUM_LEN + 1];
static int         s_sn_len     = 0;

// Monitor Logs (DEV mode)
static lv_obj_t      *s_logs_panel = NULL;
static lv_obj_t      *s_logs_btn   = NULL;
static lv_obj_t      *s_logs_tbox  = NULL;

// Monitor Serial
static lv_obj_t      *s_serial_panel = NULL;
static lv_obj_t      *s_serial_btn   = NULL;
static lv_obj_t      *s_serial_body  = NULL;
static lv_timer_t    *s_serial_timer = NULL;
static char           s_serial_lines[SERIAL_LOG_MAX][SERIAL_LOG_LEN];
static int            s_serial_head  = 0;
static int            s_serial_count = 0;
static volatile bool  s_serial_dirty = false;

// Robot Test (D-pad)
static lv_obj_t      *s_test_panel   = NULL;
static lv_obj_t      *s_test_btn     = NULL;
static lv_obj_t      *s_test_status  = NULL;
static lv_obj_t      *s_test_vel_btn = NULL; // boton que muestra "Velocidad: N"
static int32_t         s_test_vel    = 500;  // velocidad actual (0-1000), editable desde el teclado numerico

// Monitor Joystick
static lv_obj_t      *s_joy_panel = NULL;
static lv_obj_t      *s_joy_btn   = NULL;
static lv_obj_t      *s_joy_body  = NULL;
static lv_timer_t    *s_joy_timer = NULL;
static char           s_joy_lines[JOY_LOG_MAX][JOY_LOG_LEN];
static int            s_joy_head  = 0;
static int            s_joy_count = 0;
static volatile bool  s_joy_dirty = false;
// Ultimos valores de joystick (actualizados desde hmi_handle_reg)
static volatile int16_t s_joy_j1x = 0, s_joy_j1y = 0;
static volatile int16_t s_joy_j2x = 0, s_joy_j2y = 0;
static volatile bool    s_joy_btn1 = false, s_joy_btn2 = false;
static volatile int16_t s_p1_value = 0;
static volatile int16_t s_motor_cmd = 0, s_motor_vel = 0;
static volatile int16_t s_srv1_angle = 0, s_srv2_angle = 0, s_srv3_angle = 0;

// Monitor Valores (conversiones)
static lv_obj_t      *s_val_panel = NULL;
static lv_obj_t      *s_val_btn   = NULL;
static lv_obj_t      *s_val_body  = NULL;
static lv_timer_t    *s_val_timer = NULL;
#define JOY_CENTER    2048
#define JOY_DEADZONE  200
#define JOY_ADC_MAX   4095

extern void action_sysinfo_btn_device(lv_event_t *e);
#include "ui/actions.h"

// Activa/desactiva el WiFi+OTA a pedido del usuario (boton "Update" en System
// Info). wifi_init() solo corre una vez en la vida del programa; despues,
// apagar/prender es solo esp_wifi_stop()/esp_wifi_start().
static void hmi_wifi_set_enabled(bool enable)
{
    s_wifi_enabled = enable;

    if (enable) {
        wifi_driver_ensure_ready();

        // Separado de la inicializacion del driver: si el editor de WiFi ya
        // lo inicializo antes (para escanear/verificar una red), esto igual
        // tiene que correr la primera vez que el usuario activa el WiFi de
        // actualizacion.
        if (!s_ota_configured) {
            s_ota_configured = true;
            ota_http_config_t ota_cfg = {
                .version_url         = OTA_VERSION_URL,
                .firmware_url        = OTA_FIRMWARE_URL,
                .check_interval_sec  = OTA_CHECK,
                .on_update_available = ota_update_available_cb,
                .before_restart      = ota_before_restart_cb,
                .before_download     = ota_before_download_cb,
                .on_download_failed  = ota_download_failed_cb,
            };
            ota_http_start(&ota_cfg);
        }
        update_panel_set_status(g_lang->lbl_connecting, UPDATE_LED_BLINK);
        update_network_label_set_visible(false);
        update_wifi_network_row_set_visible(true);
    } else {
        if (s_wifi_initialized) {
            esp_wifi_disconnect();
            esp_wifi_stop();
        }
        update_panel_set_status(g_lang->lbl_wifi_off, UPDATE_LED_OFF);
        update_network_label_set_visible(false);
        update_wifi_network_row_set_visible(false);
    }

    if (objects.update_toggle_btn) {
        lv_obj_t *lbl = lv_obj_get_child(objects.update_toggle_btn, 0);
        if (lbl) lv_label_set_text(lbl, s_wifi_enabled ? g_lang->btn_disable_wifi : g_lang->btn_enable_wifi);
        hmi_style_btn(objects.update_toggle_btn, s_wifi_enabled);
    }
}

void hmi_update_panel_retheme(void)
{
    if (objects.update_toggle_btn) hmi_style_btn(objects.update_toggle_btn, s_wifi_enabled);
    // Botones que no viven en objects.* (son estaticos de este archivo) y cuyo
    // color depende de un estado on/off en tiempo de ejecucion, no solo del
    // tema — apply_theme() no los puede tocar directo, por eso este hook.
    encoder_toggle_retheme();
    console_wifi_ui_refresh();
}

void update_wifi_toggle_cb(lv_event_t *e)
{
    (void)e;
    hmi_wifi_set_enabled(!s_wifi_enabled);
}

//*************************************************************************************
// System Info > Update > CONSOLA — WiFi/OTA de la placa de consola (distinto
// del WiFi de la propia pantalla, de mas arriba). Todo llega/sale por UART:
// 0x1E OTA_STATUS, 0x1F FW_VERSION, 0x20 WIFI_STATUS (RX) y 0x23 WIFI_ENABLE
// (TX). El boton no asume el estado: manda el pedido y muestra "Conectando..."
// hasta que la consola confirma con WIFI_STATUS=1 (recomendacion del spec).
//*************************************************************************************
static bool s_console_wifi_connected    = false; // ultimo WIFI_STATUS recibido
static bool s_console_wifi_requested_on = false; // ultimo pedido mandado por el boton

static void console_wifi_ui_refresh(void)
{
    if (objects.console_wifi_led) {
        lv_led_set_brightness(objects.console_wifi_led, s_console_wifi_connected ? 255 : 0);
    }
    if (objects.console_wifi_toggle_btn) {
        lv_obj_t *lbl = lv_obj_get_child(objects.console_wifi_toggle_btn, 0);
        if (lbl) {
            const char *text;
            if (s_console_wifi_connected) {
                text = "Disable Console WiFi";
            } else if (s_console_wifi_requested_on) {
                text = "Connecting...";
            } else {
                text = "Enable Console WiFi";
            }
            lv_label_set_text(lbl, text);
        }
        hmi_style_btn(objects.console_wifi_toggle_btn, s_console_wifi_connected);
    }
}

void console_wifi_toggle_cb(lv_event_t *e)
{
    (void)e;
    // Si ya esta conectada, tocar el boton pide apagarla; si no, pide prenderla.
    bool turn_on = !s_console_wifi_connected;
    s_console_wifi_requested_on = turn_on;
    hmi_send_data(HMI_REG_WIFI_ENABLE, turn_on ? 1 : 0);
    console_wifi_ui_refresh();
}

// Desactiva todos los botones de nav dinamicos (logs/val/joy/serial/dev)
// Llamado desde actions.c cuando se activa un boton fijo (Device/Version/Guide)
void hmi_deactivate_dynamic_nav(void)
{
    if (s_logs_btn)   hmi_style_btn(s_logs_btn,   false);
    if (s_val_btn)    hmi_style_btn(s_val_btn,     false);
    if (s_joy_btn)    hmi_style_btn(s_joy_btn,     false);
    if (s_serial_btn) hmi_style_btn(s_serial_btn,  false);
    if (s_test_btn)   hmi_style_btn(s_test_btn,    false);
    if (s_dev_btn)    hmi_style_btn(s_dev_btn,     false);

    // Ocultar tambien los paneles (no solo despintar los botones) — sin esto,
    // si un panel de modo desarrollador (Logs/Val/Joystick/Serial/Test/Dev)
    // estaba abierto y el usuario toca Device/Version/Guide/Update, el panel
    // dinamico se queda "colado" por detras del panel fijo que se muestra.
    if (s_logs_panel)   lv_obj_add_flag(s_logs_panel,   LV_OBJ_FLAG_HIDDEN);
    if (s_val_panel)    lv_obj_add_flag(s_val_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_joy_panel)    lv_obj_add_flag(s_joy_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_serial_panel) lv_obj_add_flag(s_serial_panel, LV_OBJ_FLAG_HIDDEN);
    if (s_test_panel)   lv_obj_add_flag(s_test_panel,   LV_OBJ_FLAG_HIDDEN);
    if (s_dev_panel)    lv_obj_add_flag(s_dev_panel,    LV_OBJ_FLAG_HIDDEN);
}

// NVS helpers
static uint8_t vis_panels_nvs_read(void)
{
    nvs_handle_t h; int32_t v = 0;
    if (nvs_open(NVS_DEV_NS, NVS_READONLY, &h) == ESP_OK) {
        nvs_get_i32(h, NVS_KEY_VIS_PANELS, &v); nvs_close(h);
    }
    return (uint8_t)v;
}
static void vis_panels_nvs_write(uint8_t mask)
{
    nvs_handle_t h;
    if (nvs_open(NVS_DEV_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_i32(h, NVS_KEY_VIS_PANELS, (int32_t)mask);
        nvs_commit(h); nvs_close(h);
    }
}

static int dev_nvs_read_pin(const char *key, int def)
{
    nvs_handle_t h; int32_t v = (int32_t)def;
    if (nvs_open(NVS_DEV_NS, NVS_READONLY, &h) == ESP_OK) {
        nvs_get_i32(h, key, &v); nvs_close(h);
    }
    return (int)v;
}
static void dev_nvs_write_pin(const char *key, int val)
{
    nvs_handle_t h;
    if (nvs_open(NVS_DEV_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_i32(h, key, (int32_t)val); nvs_commit(h); nvs_close(h);
    }
}
static int dev_uart_tx_pin(void) { return dev_nvs_read_pin(NVS_KEY_UART_TX, HMI_UART_TXD); }
static int dev_uart_rx_pin(void) { return dev_nvs_read_pin(NVS_KEY_UART_RX, HMI_UART_RXD); }

#define NVS_KEY_UI_THEME "ui_theme"
#define NVS_KEY_UI_LANG  "ui_lang"
#define NVS_KEY_UI_BATD  "ui_batd"

void hmi_ui_prefs_save_theme(int theme_idx)      { dev_nvs_write_pin(NVS_KEY_UI_THEME, theme_idx); }
void hmi_ui_prefs_save_lang(int lang_id)         { dev_nvs_write_pin(NVS_KEY_UI_LANG, lang_id); }
void hmi_ui_prefs_save_bat_display(int percent)  { dev_nvs_write_pin(NVS_KEY_UI_BATD, percent); }
int  hmi_ui_prefs_load_theme(void)               { return dev_nvs_read_pin(NVS_KEY_UI_THEME, 1); }
int  hmi_ui_prefs_load_lang(void)                { return dev_nvs_read_pin(NVS_KEY_UI_LANG, 0); }
int  hmi_ui_prefs_load_bat_display(void)         { return dev_nvs_read_pin(NVS_KEY_UI_BATD, 0); }

static void dev_nvs_read_pin_str(char *out, size_t max)
{
    nvs_handle_t h;
    strncpy(out, DEV_PIN_DEF, max); out[max-1] = '\0';
    if (nvs_open(NVS_DEV_NS, NVS_READONLY, &h) == ESP_OK) {
        size_t sz = max;
        nvs_get_str(h, NVS_KEY_PIN, out, &sz);
        nvs_close(h);
    }
}
static void dev_nvs_write_pin_str(const char *pin)
{
    nvs_handle_t h;
    if (nvs_open(NVS_DEV_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, NVS_KEY_PIN, pin); nvs_commit(h); nvs_close(h);
    }
}

static void dev_nvs_read_serial(char *out, size_t max)
{
    strncpy(out, SERIAL_NUMBER, max); out[max-1] = '\0';
    nvs_handle_t h;
    if (nvs_open(NVS_DEV_NS, NVS_READONLY, &h) == ESP_OK) {
        size_t sz = max;
        nvs_get_str(h, NVS_KEY_SERIAL, out, &sz);
        nvs_close(h);
    }
}
static void dev_nvs_write_serial(const char *sn)
{
    nvs_handle_t h;
    if (nvs_open(NVS_DEV_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, NVS_KEY_SERIAL, sn); nvs_commit(h); nvs_close(h);
    }
}
static void dev_nvs_read_device_name(char *out, size_t max)
{
    strncpy(out, DEV_NAME_DEF, max); out[max-1] = '\0';
    nvs_handle_t h;
    if (nvs_open(NVS_DEV_NS, NVS_READONLY, &h) == ESP_OK) {
        size_t sz = max;
        nvs_get_str(h, NVS_KEY_DEV_NAME, out, &sz);
        nvs_close(h);
    }
}
static void dev_nvs_write_device_name(const char *name)
{
    nvs_handle_t h;
    if (nvs_open(NVS_DEV_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, NVS_KEY_DEV_NAME, name); nvs_commit(h); nvs_close(h);
    }
}
// Settings > WiFi — SSID/contrasena guardados a mano por el usuario, en vez
// de depender solo de los #define WIFI_SSID/WIFI_PASS fijos en el codigo.
// Si nunca se guardo nada, "out" queda vacio ("") — quien llama decide el
// fallback (ver wifi_init()).
static void dev_nvs_read_wifi_ssid(char *out, size_t max)
{
    out[0] = '\0';
    nvs_handle_t h;
    if (nvs_open(NVS_DEV_NS, NVS_READONLY, &h) == ESP_OK) {
        size_t sz = max;
        nvs_get_str(h, NVS_KEY_WIFI_SSID, out, &sz);
        nvs_close(h);
    }
}
static void dev_nvs_write_wifi_ssid(const char *ssid)
{
    nvs_handle_t h;
    if (nvs_open(NVS_DEV_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, NVS_KEY_WIFI_SSID, ssid); nvs_commit(h); nvs_close(h);
    }
}
static void dev_nvs_read_wifi_pass(char *out, size_t max)
{
    out[0] = '\0';
    nvs_handle_t h;
    if (nvs_open(NVS_DEV_NS, NVS_READONLY, &h) == ESP_OK) {
        size_t sz = max;
        nvs_get_str(h, NVS_KEY_WIFI_PASS, out, &sz);
        nvs_close(h);
    }
}
static void dev_nvs_write_wifi_pass(const char *pass)
{
    nvs_handle_t h;
    if (nvs_open(NVS_DEV_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, NVS_KEY_WIFI_PASS, pass); nvs_commit(h); nvs_close(h);
    }
}

// Muestra en Settings > WiFi el SSID guardado (o el fijo del codigo si
// todavia no se guardo nada) — se llama una vez al arrancar, junto con el
// resto del cableado post ui_init() en app_main().
static void wifi_settings_ui_init(void)
{
    if (!objects.settings_wifi_ssid_value) return;
    char cur[WIFI_SSID_MAX];
    dev_nvs_read_wifi_ssid(cur, sizeof(cur));
    lv_label_set_text(objects.settings_wifi_ssid_value, cur[0] ? cur : WIFI_SSID);
}

static void dev_serial_labels_update(void)
{
    char sn_buff[30];
    snprintf(sn_buff, sizeof(sn_buff), "Console S/N :  RD90C-%s", s_serial_num);
    lv_label_set_text(objects.serial_number, sn_buff);
    char sn_short[20];
    snprintf(sn_short, sizeof(sn_short), "RD90C-%s", s_serial_num);
    if (objects.sysinfo_console_serial_value)
        lv_label_set_text(objects.sysinfo_console_serial_value, sn_short);
}

//*************************************************************************************
// Settings > Encoder — calibracion de perimetro del rodillo y pulsos/vuelta
// Los widgets (spinboxes, footer) se crean en ui/screens.c junto con el resto
// de Settings; esta logica solo lee/escribe sus valores via objects.*.
//*************************************************************************************
static void enc_recalc_footer(void)
{
    if (!objects.enc_footer_label || !objects.enc_spinbox_perim || !objects.enc_spinbox_ppr) return;
    int32_t perim_cx100 = lv_spinbox_get_value(objects.enc_spinbox_perim);
    int32_t ppr         = lv_spinbox_get_value(objects.enc_spinbox_ppr);
    float perim_mm      = perim_cx100 / 100.0f;
    float mm_per_pulse   = (ppr > 0) ? (perim_mm / (float)ppr) : 0.0f;
    float pulses_per_m   = (perim_mm > 0.0f) ? ((float)ppr * 1000.0f / perim_mm) : 0.0f;

    char buf[160];
    snprintf(buf, sizeof(buf), g_lang->fmt_encoder_footer,
             mm_per_pulse, pulses_per_m, perim_mm, (long)ppr);
    lv_label_set_text(objects.enc_footer_label, buf);
}

// LVGL 9.5 lv_spinbox_increment()/decrement()/set_value() NO emiten LV_EVENT_VALUE_CHANGED
// (ver lv_spinbox.c) — por eso el commit a NVS + recalculo de footer se hace explicitamente
// aca, justo despues de mover el valor, en vez de colgarse de ese evento.
static void enc_commit_spinbox(lv_obj_t *sb)
{
    if (!sb) return;
    int32_t v = lv_spinbox_get_value(sb);
    if (sb == objects.enc_spinbox_perim) {
        dev_nvs_write_pin(NVS_KEY_ENC_PERIM_CX100, (int)v);
    } else if (sb == objects.enc_spinbox_ppr) {
        dev_nvs_write_pin(NVS_KEY_ENC_PPR, (int)v);
    }
    enc_recalc_footer();
}

void enc_sb_increment_cb(lv_event_t *e)
{
    lv_obj_t *sb = (lv_obj_t *)lv_event_get_user_data(e);
    if (!sb) return;
    lv_spinbox_increment(sb);
    enc_commit_spinbox(sb);
}
void enc_sb_decrement_cb(lv_event_t *e)
{
    lv_obj_t *sb = (lv_obj_t *)lv_event_get_user_data(e);
    if (!sb) return;
    lv_spinbox_decrement(sb);
    enc_commit_spinbox(sb);
}

void enc_preset_cb(lv_event_t *e)
{
    intptr_t cx100 = (intptr_t)lv_event_get_user_data(e);
    if (!objects.enc_spinbox_perim) return;
    lv_spinbox_set_value(objects.enc_spinbox_perim, (int32_t)cx100);
    dev_nvs_write_pin(NVS_KEY_ENC_PERIM_CX100, (int)cx100);
    enc_recalc_footer();
}

// Carga los valores guardados en NVS en los spinboxes (screens.c los crea con
// los defaults de fabrica: 85.20mm / 600 pulsos) y calcula el footer. Llamado
// una vez desde app_main, al final de la construccion en etapas de los
// paneles ocultos (necesita que los spinboxes ya existan).
void enc_settings_load_from_nvs(void)
{
    int32_t perim_cx100 = dev_nvs_read_pin(NVS_KEY_ENC_PERIM_CX100, ENC_PERIM_CX100_DEF);
    int32_t ppr         = dev_nvs_read_pin(NVS_KEY_ENC_PPR, ENC_PPR_DEF);
    if (objects.enc_spinbox_perim) lv_spinbox_set_value(objects.enc_spinbox_perim, perim_cx100);
    if (objects.enc_spinbox_ppr)   lv_spinbox_set_value(objects.enc_spinbox_ppr, ppr);
    enc_recalc_footer();
}

// screens.c deja la columna de nav de Settings con LV_OBJ_FLAG_SCROLLABLE
// removido (pensada para 5 botones). Con el 6to boton (Encoder) el contenido
// ya no entra en los ~400px de alto disponibles y queda cortado — se
// rehabilita el scroll vertical aca, una vez despues de crear la UI.
static void settings_nav_enable_scroll(void)
{
    lv_obj_t *nav_col = lv_obj_get_parent(objects.settings_btn_user);
    lv_obj_add_flag(nav_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(nav_col, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(nav_col, LV_SCROLLBAR_MODE_AUTO);
}

//*************************************************************************************
// GENERAL CONTROLS > tarjeta ENCODER — toggle Pies/Metros usando la calibracion
// guardada en Settings > Encoder (mismo patron que el toggle Voltaje/Porcentaje
// de bateria: g_bat_display_percent + apply_btn_style).
//*************************************************************************************
static bool     s_encoder_show_feet     = false;
static int32_t  s_encoder_last_raw      = 0;
static lv_obj_t *s_encoder_btn_feet     = NULL;
static lv_obj_t *s_encoder_btn_meters   = NULL;
// Distancia total recorrida en metros (siempre en metros, sin importar el
// toggle Pies/Metros de la tarjeta ENCODER) — usada para saber cuanto avanzo
// el robot entre un empuje del trazo y el siguiente, y para la regla de
// medicion del panel de manejo.
static float s_encoder_dist_m = 0.0f;
// Confirmado con la prueba del RAW: la consola SI resetea su propio
// contador a 0 al recibir HMI_REG_ENCODER=0 (ver hmi_encoder_reset()), asi
// que no hace falta ningun offset local — se confia directo en el valor
// crudo que manda.
// Ultimo valor de inclinacion (Y) conocido — el trazo del panel de manejo
// NO avanza solo con el tiempo, avanza cuando el ENCODER avanza (ver
// hmi_encoder_set_raw). Esta variable solo cachea el angulo mas reciente
// para usarlo en el momento en que el encoder efectivamente se mueve.
static float s_bigview_current_pitch = 0.0f;
// Labels del panel de pantalla completa (boton oculto del logo, ver mas
// abajo) — declarados aca para que encoder_dashboard_refresh() los pueda
// actualizar tambien cuando el panel esta abierto. El numero va en fuente
// grande y la unidad (m/ft) en fuente normal, por separado.
static lv_obj_t *s_encoder_bigview_label      = NULL;
static lv_obj_t *s_encoder_bigview_unit_label = NULL;

static void encoder_dashboard_refresh(void)
{
    if (!objects.encoder_value) return;

    int32_t perim_cx100 = dev_nvs_read_pin(NVS_KEY_ENC_PERIM_CX100, ENC_PERIM_CX100_DEF);
    int32_t ppr         = dev_nvs_read_pin(NVS_KEY_ENC_PPR, ENC_PPR_DEF);
    float perim_mm = perim_cx100 / 100.0f;
    float dist_m  = (ppr > 0) ? ((float)s_encoder_last_raw * perim_mm) / ((float)ppr * 1000.0f) : 0.0f;
    s_encoder_dist_m = dist_m;

    char buf[24];
    char num_buf[16];
    const char *unit;
    if (s_encoder_show_feet) {
        snprintf(buf, sizeof(buf), "%.2f ft", dist_m * 3.28084f);
        snprintf(num_buf, sizeof(num_buf), "%.2f", dist_m * 3.28084f);
        unit = "ft";
    } else {
        snprintf(buf, sizeof(buf), "%.3f m", dist_m);
        snprintf(num_buf, sizeof(num_buf), "%.3f", dist_m);
        unit = "m";
    }
    lv_label_set_text(objects.encoder_value, buf);
    if (s_encoder_bigview_label) lv_label_set_text(s_encoder_bigview_label, num_buf);
    if (s_encoder_bigview_unit_label) lv_label_set_text(s_encoder_bigview_unit_label, unit);
}

void hmi_encoder_set_raw(int32_t raw_pulses)
{
    // Solo interesa la distancia positiva (recorrido hacia adelante) — si
    // el conteo se va a negativo, se hace un reset completo (en vez de
    // solo recortar a cero) para que quede consistente con la consola.
    if (raw_pulses < 0) {
        hmi_encoder_reset();
        return;
    }
    int32_t new_raw = raw_pulses;
    bool moved = new_raw > s_encoder_last_raw;
    float prev_dist_m = s_encoder_dist_m;
    s_encoder_last_raw = new_raw;
    encoder_dashboard_refresh(); // recalcula y guarda s_encoder_dist_m

    // El trazo de inclinacion del panel de manejo avanza solo cuando el
    // robot realmente se mueve (el encoder avanza), no con un timer fijo
    // — asi el eje horizontal del trazo representa distancia recorrida
    // real, no tiempo ni cantidad de muestras.
    if (moved) {
        float delta_dist_m = s_encoder_dist_m - prev_dist_m;
        bigview_angle_trace_push(s_bigview_current_pitch, delta_dist_m);
    }
}

// Resetea la distancia mostrada a 0 y le avisa a la consola (HMI_REG_ENCODER
// = 0) para que resetee su propio contador — confirmado que la consola SI
// lo hace, asi que no hace falta ningun offset local, el proximo dato real
// ya viene desde 0.
void hmi_encoder_reset(void)
{
    s_encoder_last_raw = 0;
    encoder_dashboard_refresh();
    hmi_send_data(HMI_REG_ENCODER, 0);
}

// Aplica el modo (UI + estado) sin tocar NVS — usado para restaurar el valor
// guardado al arrancar, sin pisarlo con el default.
static void encoder_mode_apply(bool feet)
{
    s_encoder_show_feet = feet;
    hmi_style_btn(s_encoder_btn_feet,   feet);
    hmi_style_btn(s_encoder_btn_meters, !feet);
    encoder_dashboard_refresh();
}

// Re-colorea los botones FT/METERS con los colores del tema actual sin tocar
// cual esta activo — llamado desde hmi_update_panel_retheme() cuando el
// usuario cambia de tema (Dark/Classic/Light) en Settings.
static void encoder_toggle_retheme(void)
{
    hmi_style_btn(s_encoder_btn_feet,   s_encoder_show_feet);
    hmi_style_btn(s_encoder_btn_meters, !s_encoder_show_feet);
}

// Elegido por el usuario (toque en PIES/METROS): persiste en NVS y aplica.
static void encoder_mode_set(bool feet)
{
    dev_nvs_write_pin(NVS_KEY_ENC_MODE_FEET, feet ? 1 : 0);
    encoder_mode_apply(feet);
}
static void encoder_mode_feet_cb(lv_event_t *e)   { (void)e; encoder_mode_set(true); }
static void encoder_mode_meters_cb(lv_event_t *e) { (void)e; encoder_mode_set(false); }
// Tocar el numero grande de GENERAL CONTROLS tambien alterna la unidad —
// forma mas directa que ir hasta Settings > Encoder para el mismo toggle.
static void encoder_value_toggle_cb(lv_event_t *e) { (void)e; encoder_mode_set(!s_encoder_show_feet); }

// Se engancha al panel Settings > Encoder ya creado por screens.c
// (objects.settings_content_encoder), como una fila mas debajo de
// Pulses/rev — junto con el resto de la calibracion del encoder, no en la
// tarjeta chica de GENERAL CONTROLS (que ahora usa todo el espacio para
// mostrar el numero mas grande).
static void encoder_display_toggle_create(void)
{
    if (!objects.settings_content_encoder) return;

    lv_obj_t *row = lv_obj_create(objects.settings_content_encoder);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_layout(row, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_cross_place(row, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_column(row, 12, 0);
    lv_obj_set_style_pad_ver(row, 6, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *key = lv_label_create(row);
    lv_obj_set_size(key, 130, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(key, lv_color_hex(0xffaaaaaa), 0);
    lv_obj_set_style_text_font(key, &lv_font_montserrat_18, 0);
    lv_label_set_text(key, "Display unit:");

    lv_obj_t *btn_f = lv_button_create(row);
    s_encoder_btn_feet = btn_f;
    lv_obj_set_size(btn_f, 110, 40);
    lv_obj_set_style_radius(btn_f, 8, 0);
    lv_obj_set_style_shadow_opa(btn_f, 0, 0);
    lv_obj_t *lbl_f = lv_label_create(btn_f);
    lv_obj_set_style_align(lbl_f, LV_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl_f, &lv_font_montserrat_18, 0);
    lv_label_set_text(lbl_f, g_lang->btn_feet);
    lv_obj_add_event_cb(btn_f, encoder_mode_feet_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_m = lv_button_create(row);
    s_encoder_btn_meters = btn_m;
    lv_obj_set_size(btn_m, 110, 40);
    lv_obj_set_style_radius(btn_m, 8, 0);
    lv_obj_set_style_shadow_opa(btn_m, 0, 0);
    lv_obj_t *lbl_m = lv_label_create(btn_m);
    lv_obj_set_style_align(lbl_m, LV_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl_m, &lv_font_montserrat_18, 0);
    lv_label_set_text(lbl_m, g_lang->btn_meters);
    lv_obj_add_event_cb(btn_m, encoder_mode_meters_cb, LV_EVENT_CLICKED, NULL);

    if (objects.encoder_value) {
        lv_obj_add_flag(objects.encoder_value, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(objects.encoder_value, encoder_value_toggle_cb, LV_EVENT_CLICKED, NULL);
    }

    // Restaurar el modo guardado en NVS (default: metros) sin volver a
    // escribirlo — encoder_mode_apply() no toca NVS, a diferencia de
    // encoder_mode_set() (que es solo para cuando el usuario toca un boton).
    bool saved_feet = dev_nvs_read_pin(NVS_KEY_ENC_MODE_FEET, 0) != 0;
    encoder_mode_apply(saved_feet);
}

//*************************************************************************************
// "Panel de manejo": boton oculto en el logo "WELLTEP" de la esquina
// superior izquierda que abre un panel de pantalla completa con el valor
// del encoder gigante y centrado (mismo texto/formato que la tarjeta
// ENCODER de GENERAL CONTROLS), mas la bateria de robot y consola (con
// porcentaje) en la esquina superior derecha. Se cierra tocando en
// cualquier parte del panel.
//*************************************************************************************
static lv_obj_t *s_encoder_bigview_panel = NULL;
static lv_obj_t *s_bigview_robot_batt_bar     = NULL;
static lv_obj_t *s_bigview_robot_batt_label   = NULL;
static lv_obj_t *s_bigview_console_batt_bar   = NULL;
static lv_obj_t *s_bigview_console_batt_label = NULL;
static lv_obj_t *s_bigview_robot_batt_caption   = NULL;
static lv_obj_t *s_bigview_console_batt_caption = NULL;
static lv_obj_t *s_bigview_distance_caption = NULL; // "DISTANCIA RECORRIDA"
static lv_obj_t *s_bigview_trace_caption    = NULL; // "TRAZA DE RECORRIDO"
static lv_obj_t *s_bigview_robot_model_label  = NULL; // debajo del logo Welltepp
static lv_obj_t *s_bigview_led_bt             = NULL; // al lado de la bateria ROBOT
static lv_obj_t *s_bigview_led_online         = NULL;
// Barra de niveles (4 escalones: 25/50/75/100%) del brillo del LED del
// robot — arriba de "DISTANCIA RECORRIDA", horizontal, en amarillo.
static lv_obj_t *s_bigview_led_level_seg[4] = { NULL, NULL, NULL, NULL };

void hmi_bigview_led_level_refresh(void)
{
    int pct = atoi(get_var_brightness());
    static const int thresholds[4] = { 25, 50, 75, 100 };
    for (int i = 0; i < 4; i++) {
        if (!s_bigview_led_level_seg[i]) continue;
        bool on = pct >= thresholds[i];
        lv_obj_set_style_bg_color(s_bigview_led_level_seg[i],
            lv_color_hex(0xfff5c518), 0);
        lv_obj_set_style_bg_opa(s_bigview_led_level_seg[i], on ? 255 : 60, 0);
    }
}
// Traza de pitch (ANGLE_Y) tipo "grabadora de vuelo" — basada en la logica
// de C:\Users\USUARIO\Music\PITCH\pitch_trace_gui.py (la PENDIENTE del
// trazo, no su altura, representa el angulo), pero con el eje horizontal
// atado a DISTANCIA REAL recorrida (encoder) en vez de a "una muestra = un
// paso fijo": el cuadro representa BIGVIEW_TRACE_WINDOW_M metros de tuberia
// recorrida, y se desliza a medida que el robot avanza.
#define BIGVIEW_TRACE_W          700    // ancho visible en px
#define BIGVIEW_TRACE_H          120
#define BIGVIEW_TRACE_WINDOW_M   2.0f   // el cuadro representa 2m de recorrido
#define BIGVIEW_TRACE_PX_PER_M   (BIGVIEW_TRACE_W / BIGVIEW_TRACE_WINDOW_M)
#define BIGVIEW_TRACE_GAIN       1.0f   // == TRACE_GAIN de Python
#define BIGVIEW_TRACE_MAX_DEG    75.0f  // == TRACE_MAX_DEG de Python
#define BIGVIEW_TRACE_MAX_PTS    300    // buffer de seguridad (no todos visibles a la vez)

static lv_obj_t *s_bigview_angle_line   = NULL;
static lv_obj_t *s_bigview_angle_tip    = NULL;
static lv_obj_t *s_bigview_ruler_left   = NULL; // distancia en el borde izquierdo (mas vieja)
static lv_obj_t *s_bigview_ruler_mid    = NULL;
static lv_obj_t *s_bigview_ruler_right  = NULL; // distancia actual (punta del trazo)

// Puntos "en el mundo" (x acumulado en px == distancia recorrida * escala,
// nunca se resetea); se recortan por la izquierda cuando quedan fuera de
// la ventana visible de BIGVIEW_TRACE_WINDOW_M metros.
static float s_trace_x[BIGVIEW_TRACE_MAX_PTS];
static float s_trace_y[BIGVIEW_TRACE_MAX_PTS];
static int   s_trace_len = 0;
static lv_point_precise_t s_angle_trace_pts[BIGVIEW_TRACE_MAX_PTS];

// == _amplify_for_display() en Python: empuja un poco las inclinaciones
// casi nulas para que se note en el trazo, saturando suave (tanh) sin
// distorsionar giros grandes reales.
static float bigview_amplify_for_display(float angle_deg)
{
    float x = angle_deg * BIGVIEW_TRACE_GAIN / BIGVIEW_TRACE_MAX_DEG;
    return BIGVIEW_TRACE_MAX_DEG * tanhf(x);
}

// Punto de partida del trazo — se llama una vez al crear el panel.
static void bigview_angle_trace_reset(void)
{
    s_trace_len = 1;
    s_trace_x[0] = 0.0f;
    s_trace_y[0] = BIGVIEW_TRACE_H / 2.0f;
}

// Actualiza los 3 numeros de la regla debajo del cuadro (distancia real
// acumulada — izquierda = borde mas viejo visible, derecha = ahora mismo).
static void bigview_ruler_update(void)
{
    if (!s_bigview_ruler_right) return;
    float now_m = s_encoder_dist_m;
    float left_m = now_m - BIGVIEW_TRACE_WINDOW_M;
    float mid_m  = now_m - BIGVIEW_TRACE_WINDOW_M / 2.0f;
    if (left_m < 0) left_m = 0;
    if (mid_m  < 0) mid_m  = 0;

    char buf[16];
    snprintf(buf, sizeof(buf), "%.1fm", left_m);
    lv_label_set_text(s_bigview_ruler_left, buf);
    snprintf(buf, sizeof(buf), "%.1fm", mid_m);
    lv_label_set_text(s_bigview_ruler_mid, buf);
    snprintf(buf, sizeof(buf), "%.1fm", now_m);
    lv_label_set_text(s_bigview_ruler_right, buf);
}

// Redibuja la linea mapeando los puntos "del mundo" a la ventana visible
// (el mas nuevo siempre queda pegado al borde derecho).
static void bigview_angle_trace_redraw(void)
{
    if (!s_bigview_angle_line || s_trace_len == 0) return;

    float right_x   = s_trace_x[s_trace_len - 1];
    // Mientras no se recorrieron los BIGVIEW_TRACE_WINDOW_M completos, el
    // borde izquierdo se queda fijo en 0 (el trazo crece desde la
    // izquierda, como antes) — recien empieza a deslizarse de verdad una
    // vez que hay mas de una ventana llena de datos.
    float left_edge = right_x - BIGVIEW_TRACE_W;
    if (left_edge < 0.0f) left_edge = 0.0f;

    int n = 0;
    for (int i = 0; i < s_trace_len && n < BIGVIEW_TRACE_MAX_PTS; i++) {
        float sx = s_trace_x[i] - left_edge;
        if (sx < 0.0f) sx = 0.0f;
        s_angle_trace_pts[n].x = (lv_value_precise_t)sx;
        s_angle_trace_pts[n].y = (lv_value_precise_t)s_trace_y[i];
        n++;
    }

    lv_line_set_points_mutable(s_bigview_angle_line, s_angle_trace_pts, n);
    if (s_bigview_angle_tip && n > 0) {
        lv_obj_set_pos(s_bigview_angle_tip,
            (int32_t)s_angle_trace_pts[n - 1].x - 9,
            (int32_t)s_angle_trace_pts[n - 1].y - 9);
    }
}

// == self.pitch_trace.append(pitch) en _handle_line() de Python, pero el
// paso en X es proporcional a la distancia REAL recorrida (delta_dist_m),
// no un paso fijo por muestra.
static void bigview_angle_trace_push(float angle_deg, float delta_dist_m)
{
    if (!s_bigview_angle_line || s_trace_len == 0 || delta_dist_m <= 0.0f) return;

    float step_px = delta_dist_m * BIGVIEW_TRACE_PX_PER_M;
    float vis = bigview_amplify_for_display(angle_deg);
    float rad = vis * ((float)M_PI / 180.0f);

    float last_x = s_trace_x[s_trace_len - 1];
    float last_y = s_trace_y[s_trace_len - 1];
    float new_x = last_x + step_px * cosf(rad);
    float new_y = last_y + step_px * sinf(rad); // positivo = sube (igual que Python)
    if (new_y < 4.0f) new_y = 4.0f;
    if (new_y > BIGVIEW_TRACE_H - 4.0f) new_y = BIGVIEW_TRACE_H - 4.0f;

    if (s_trace_len >= BIGVIEW_TRACE_MAX_PTS) {
        memmove(s_trace_x, s_trace_x + 1, (BIGVIEW_TRACE_MAX_PTS - 1) * sizeof(float));
        memmove(s_trace_y, s_trace_y + 1, (BIGVIEW_TRACE_MAX_PTS - 1) * sizeof(float));
        s_trace_len--;
    }
    s_trace_x[s_trace_len] = new_x;
    s_trace_y[s_trace_len] = new_y;
    s_trace_len++;

    // Recorta puntos que ya quedaron fuera de la ventana visible.
    float left_edge = new_x - BIGVIEW_TRACE_W;
    int drop = 0;
    while (drop < s_trace_len - 1 && s_trace_x[drop] < left_edge) drop++;
    if (drop > 0) {
        memmove(s_trace_x, s_trace_x + drop, (s_trace_len - drop) * sizeof(float));
        memmove(s_trace_y, s_trace_y + drop, (s_trace_len - drop) * sizeof(float));
        s_trace_len -= drop;
    }

    bigview_angle_trace_redraw();
    bigview_ruler_update();
}

// Vuelve la distancia del encoder a cero y limpia el trazo/regla del panel
// de manejo — gesto de 2 toques.
static void bigview_reset_encoder_and_trace(void)
{
    hmi_encoder_reset();
    bigview_angle_trace_reset();
    bigview_angle_trace_redraw();
    bigview_ruler_update();
}

// Al vencer la ventana de 400ms sin toques nuevos: si quedaron exactamente
// 2 toques contados, dispara el reset. 1 toque solo no hace nada. 3 toques
// se resuelven antes, en encoder_bigview_tap_cb() (no esperan a este timer).
static int       s_bigview_tap_count = 0;
static lv_timer_t *s_bigview_tap_timer = NULL;
static void bigview_tap_timeout_cb(lv_timer_t *t)
{
    (void)t;
    if (s_bigview_tap_count == 2) {
        bigview_reset_encoder_and_trace();
    }
    s_bigview_tap_count = 0;
    s_bigview_tap_timer = NULL; // repeat_count=1: LVGL la borra sola despues de correr
}

// Gesto del panel de manejo: 1 toque no hace nada, 2 toques resetean
// encoder+trazo, 3 toques cierran el panel.
static void encoder_bigview_tap_cb(lv_event_t *e)
{
    (void)e;
    s_bigview_tap_count++;

    if (s_bigview_tap_count >= 3) {
        if (s_bigview_tap_timer) {
            lv_timer_delete(s_bigview_tap_timer);
            s_bigview_tap_timer = NULL;
        }
        s_bigview_tap_count = 0;
        if (s_encoder_bigview_panel) lv_obj_add_flag(s_encoder_bigview_panel, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (s_bigview_tap_timer) {
        lv_timer_reset(s_bigview_tap_timer);
    } else {
        s_bigview_tap_timer = lv_timer_create(bigview_tap_timeout_cb, 400, NULL);
        lv_timer_set_repeat_count(s_bigview_tap_timer, 1);
    }
}

// Mismos colores/umbrales que la barra de bateria del robot en Control
// General (ver case HMI_REG_ROBOT_VOLTAGE), pero por porcentaje en vez de
// mV crudos, para poder reusarla igual con la bateria de la consola.
static lv_color_t battery_bar_color(uint8_t percent)
{
    if (percent >= 67) return lv_color_hex(0x27AE60); // verde
    if (percent >= 50) return lv_color_hex(0xF5C518); // amarillo
    if (percent >= 42) return lv_color_hex(0xE67E22); // naranja
    return lv_color_hex(0xE74C3C);                     // rojo
}

// Crea una "celda" de bateria compacta (barra vertical + caption + valor),
// clon chico de la tarjeta SYSTEM BATTERY de Control General.
static void battery_cell_create(lv_obj_t *parent, const char *caption,
                                 lv_obj_t **out_bar, lv_obj_t **out_label,
                                 lv_obj_t **out_caption)
{
    // 72px (antes 64) — "CONSOLA" no entraba en 64px y se veia recortado
    // ("CONSOL").
    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_set_size(col, 72, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(col, 0, 0);
    lv_obj_set_style_border_width(col, 0, 0);
    lv_obj_set_style_pad_all(col, 0, 0);
    lv_obj_set_style_pad_row(col, 4, 0);
    lv_obj_set_style_layout(col, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(col, LV_FLEX_FLOW_COLUMN, 0);
    lv_obj_set_style_flex_cross_place(col, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_remove_flag(col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(col, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *cap = lv_label_create(col);
    lv_obj_set_style_text_font(cap, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(cap, lv_color_hex(0xffaaaaaa), 0);
    lv_label_set_text(cap, caption);

    lv_obj_t *bar = lv_bar_create(col);
    lv_obj_set_size(bar, 32, 64);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bar, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bar, 8, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xff27ae60), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);

    lv_obj_t *val = lv_label_create(col);
    lv_obj_set_style_text_font(val, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(val, lv_color_hex(0xffffffff), 0);
    lv_label_set_text(val, "--%");

    *out_bar     = bar;
    *out_label   = val;
    *out_caption = cap;
}

// Refresca la bateria mostrada en la esquina del panel de manejo — llamado
// al abrir el panel y tambien desde HMI_REG_ROBOT_VOLTAGE/CONSOLE_VOLTAGE
// para que se actualice en vivo mientras esta abierto.
static void bigview_battery_refresh(void)
{
    if (s_bigview_robot_batt_bar) {
        uint8_t p = (uint8_t)atoi(get_var_robot_voltage_percent());
        lv_bar_set_value(s_bigview_robot_batt_bar, p, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_bigview_robot_batt_bar, battery_bar_color(p), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }
    if (s_bigview_robot_batt_label) {
        lv_label_set_text(s_bigview_robot_batt_label, get_var_robot_voltage_percent());
    }
    if (s_bigview_console_batt_bar) {
        uint8_t p = (uint8_t)atoi(get_var_console_voltage_percent());
        lv_bar_set_value(s_bigview_console_batt_bar, p, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_bigview_console_batt_bar, battery_bar_color(p), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }
    if (s_bigview_console_batt_label) {
        lv_label_set_text(s_bigview_console_batt_label, get_var_console_voltage_percent());
    }
}

static void encoder_bigview_create(void)
{
    if (!objects.main) return;

    lv_obj_t *panel = lv_obj_create(objects.main);
    s_encoder_bigview_panel = panel;
    lv_obj_set_pos(panel, 0, 0);
    lv_obj_set_size(panel, 800, 480);
    lv_obj_set_style_radius(panel, 0, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0xff000000), 0);
    lv_obj_set_style_bg_opa(panel, 255, 0);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(panel, encoder_bigview_tap_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *dist_col = lv_obj_create(panel);
    lv_obj_set_size(dist_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(dist_col, 0, 0);
    lv_obj_set_style_border_width(dist_col, 0, 0);
    lv_obj_set_style_pad_all(dist_col, 0, 0);
    lv_obj_set_style_pad_row(dist_col, 6, 0);
    lv_obj_set_style_layout(dist_col, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(dist_col, LV_FLEX_FLOW_COLUMN, 0);
    lv_obj_set_style_flex_cross_place(dist_col, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_remove_flag(dist_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(dist_col, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(dist_col, LV_ALIGN_CENTER, 0, -100);

    // Barra de niveles del brillo del LED del robot — 4 escalones
    // (25/50/75/100%) horizontales en amarillo, arriba de "DISTANCIA
    // RECORRIDA". Se actualiza via hmi_bigview_led_level_refresh().
    lv_obj_t *led_level_row = lv_obj_create(dist_col);
    lv_obj_set_size(led_level_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(led_level_row, 0, 0);
    lv_obj_set_style_border_width(led_level_row, 0, 0);
    lv_obj_set_style_pad_all(led_level_row, 0, 0);
    lv_obj_set_style_pad_column(led_level_row, 6, 0);
    lv_obj_set_style_layout(led_level_row, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(led_level_row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_remove_flag(led_level_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(led_level_row, LV_OBJ_FLAG_CLICKABLE);

    for (int i = 0; i < 4; i++) {
        lv_obj_t *seg = lv_obj_create(led_level_row);
        s_bigview_led_level_seg[i] = seg;
        lv_obj_set_size(seg, 40, 8);
        lv_obj_set_style_radius(seg, 2, 0);
        lv_obj_set_style_border_width(seg, 0, 0);
        lv_obj_set_style_bg_color(seg, lv_color_hex(0xfff5c518), 0);
        lv_obj_set_style_bg_opa(seg, 60, 0);
        lv_obj_remove_flag(seg, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(seg, LV_OBJ_FLAG_CLICKABLE);
    }
    hmi_bigview_led_level_refresh();

    lv_obj_t *caption = lv_label_create(dist_col);
    s_bigview_distance_caption = caption;
    lv_obj_set_style_text_font(caption, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(caption, lv_color_hex(0xffaaaaaa), 0);
    lv_label_set_text(caption, g_lang->panel_drive_distance_caption);

    // Fila numero+unidad: el numero usa la fuente propia grande (ver
    // main/ui/font_distance.c), la unidad (m/ft) queda en el mismo
    // tamano que el titulo "DISTANCIA RECORRIDA", alineados por abajo.
    lv_obj_t *num_row = lv_obj_create(dist_col);
    lv_obj_set_size(num_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(num_row, 0, 0);
    lv_obj_set_style_border_width(num_row, 0, 0);
    lv_obj_set_style_pad_all(num_row, 0, 0);
    lv_obj_set_style_pad_column(num_row, 8, 0);
    lv_obj_set_style_layout(num_row, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(num_row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_cross_place(num_row, LV_FLEX_ALIGN_END, 0);
    lv_obj_remove_flag(num_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(num_row, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *label = lv_label_create(num_row);
    s_encoder_bigview_label = label;
    lv_obj_set_style_text_font(label, &lv_font_distance, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffffff), 0);
    lv_label_set_text(label, "0.000");

    lv_obj_t *unit = lv_label_create(num_row);
    s_encoder_bigview_unit_label = unit;
    lv_obj_set_style_text_font(unit, &lv_font_montserrat_42, 0);
    lv_obj_set_style_text_color(unit, lv_color_hex(0xffaaaaaa), 0);
    lv_obj_set_style_pad_bottom(unit, 12, 0);
    lv_label_set_text(unit, "m");

    // Traza de pitch (ANGLE_Y) — debajo del numero del encoder. Replica de
    // pitch_trace_gui.py: la PENDIENTE de la linea representa el angulo,
    // no su altura (ver bigview_angle_trace_redraw()).
    lv_obj_t *angle_caption = lv_label_create(panel);
    s_bigview_trace_caption = angle_caption;
    lv_obj_set_style_text_font(angle_caption, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(angle_caption, lv_color_hex(0xffaaaaaa), 0);
    lv_label_set_text(angle_caption, g_lang->panel_drive_trace_caption);
    lv_obj_align(angle_caption, LV_ALIGN_TOP_MID, 0, 268);

    // +24 de margen (12px por lado, via padding) para que el circulo de la
    // punta (18px) no quede tapado por el marco de la caja.
    lv_obj_t *trace_box = lv_obj_create(panel);
    lv_obj_set_size(trace_box, BIGVIEW_TRACE_W + 24, BIGVIEW_TRACE_H + 24);
    lv_obj_align(trace_box, LV_ALIGN_TOP_MID, 0, 296);
    lv_obj_set_style_pad_all(trace_box, 12, 0);
    lv_obj_set_style_bg_color(trace_box, lv_color_hex(0xff1a1a1a), 0);
    lv_obj_set_style_bg_opa(trace_box, 40, 0);   // traslucido
    lv_obj_set_style_border_color(trace_box, lv_color_hex(0xff2d2d2d), 0);
    lv_obj_set_style_border_width(trace_box, 2, 0);
    lv_obj_set_style_border_opa(trace_box, 150, 0);
    lv_obj_set_style_radius(trace_box, 10, 0);
    lv_obj_remove_flag(trace_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(trace_box, LV_OBJ_FLAG_CLICKABLE);

    // Linea de "nivel" (referencia horizontal en la mitad), PUNTEADA — ==
    // la linea punteada gris de _draw_pitch_trace() en Python.
    static lv_point_precise_t level_ref_pts[2];
    level_ref_pts[0].x = 0;
    level_ref_pts[0].y = BIGVIEW_TRACE_H / 2;
    level_ref_pts[1].x = BIGVIEW_TRACE_W;
    level_ref_pts[1].y = BIGVIEW_TRACE_H / 2;
    lv_obj_t *level_ref = lv_line_create(trace_box);
    lv_obj_set_size(level_ref, BIGVIEW_TRACE_W, BIGVIEW_TRACE_H);
    lv_line_set_points(level_ref, level_ref_pts, 2);
    lv_obj_set_style_line_color(level_ref, lv_color_hex(0xff888888), 0);
    lv_obj_set_style_line_width(level_ref, 1, 0);
    lv_obj_set_style_line_dash_width(level_ref, 4, 0);
    lv_obj_set_style_line_dash_gap(level_ref, 4, 0);
    lv_obj_remove_flag(level_ref, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *line = lv_line_create(trace_box);
    s_bigview_angle_line = line;
    lv_obj_set_size(line, BIGVIEW_TRACE_W, BIGVIEW_TRACE_H);
    lv_obj_set_style_line_color(line, lv_color_hex(0xfff5c518), 0);
    lv_obj_set_style_line_width(line, 2, 0);
    lv_obj_set_style_line_rounded(line, true, 0);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_CLICKABLE);

    // Punto "actual" — circulo solido en la punta de la traza, == el
    // cv.create_oval(...) verde con borde blanco del script Python.
    lv_obj_t *tip = lv_obj_create(trace_box);
    s_bigview_angle_tip = tip;
    lv_obj_set_size(tip, 18, 18);
    lv_obj_set_style_radius(tip, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(tip, lv_color_hex(0xff39ff6a), 0);
    lv_obj_set_style_bg_opa(tip, 255, 0);
    lv_obj_set_style_border_color(tip, lv_color_hex(0xffffffff), 0);
    lv_obj_set_style_border_width(tip, 2, 0);
    lv_obj_remove_flag(tip, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(tip, LV_OBJ_FLAG_CLICKABLE);

    bigview_angle_trace_reset();
    bigview_angle_trace_redraw();

    // Regla de medicion debajo del cuadro: 3 marcas cada 1m dentro de la
    // ventana de BIGVIEW_TRACE_WINDOW_M (2m) — izquierda = borde mas viejo
    // visible, centro, derecha = distancia actual (punta del trazo).
    int32_t ruler_y = 296 + BIGVIEW_TRACE_H + 24 + 6;

    lv_obj_t *ruler_left = lv_label_create(panel);
    s_bigview_ruler_left = ruler_left;
    lv_obj_set_style_text_font(ruler_left, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ruler_left, lv_color_hex(0xff888888), 0);
    lv_label_set_text(ruler_left, "0.0m");
    lv_obj_align(ruler_left, LV_ALIGN_TOP_MID, -(BIGVIEW_TRACE_W / 2), ruler_y);

    lv_obj_t *ruler_mid = lv_label_create(panel);
    s_bigview_ruler_mid = ruler_mid;
    lv_obj_set_style_text_font(ruler_mid, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ruler_mid, lv_color_hex(0xff888888), 0);
    lv_label_set_text(ruler_mid, "0.0m");
    lv_obj_align(ruler_mid, LV_ALIGN_TOP_MID, 0, ruler_y);

    lv_obj_t *ruler_right = lv_label_create(panel);
    s_bigview_ruler_right = ruler_right;
    lv_obj_set_style_text_font(ruler_right, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ruler_right, lv_color_hex(0xff888888), 0);
    lv_label_set_text(ruler_right, "0.0m");
    lv_obj_align(ruler_right, LV_ALIGN_TOP_MID, (BIGVIEW_TRACE_W / 2), ruler_y);

    bigview_ruler_update();

    // Logo Welltepp — esquina superior izquierda, opuesta a la bateria.
    lv_obj_t *logo = lv_img_create(panel);
    lv_img_set_src(logo, &lock_logo_wordmark);
    lv_image_set_scale(logo, 205); // 205/256 ~ 80%, igual que la barra superior
    lv_obj_set_pos(logo, 16, 16);
    lv_obj_remove_flag(logo, LV_OBJ_FLAG_CLICKABLE);

    // Modelo del robot (RD80/RD90/RD100) — debajo del logo Welltepp, en
    // blanco. Arranca oculto (como el pill de la barra superior) y solo
    // aparece cuando llega HMI_REG_ROBOT_MODEL real — ver apply_robot_model().
    lv_obj_t *model_label = lv_label_create(panel);
    s_bigview_robot_model_label = model_label;
    lv_obj_set_style_text_font(model_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(model_label, lv_color_hex(0xffffffff), 0);
    lv_label_set_text(model_label, "RD--");
    lv_obj_set_pos(model_label, 16, 42);
    lv_obj_add_flag(model_label, LV_OBJ_FLAG_HIDDEN);

    // Bateria robot/consola — esquina superior derecha, mismo estilo de
    // barra que la tarjeta SYSTEM BATTERY de Control General. Ancho
    // ampliado para la columna de LEDs (Bluetooth/Online) al lado de ROBOT
    // y para que "CONSOLA" entre completo.
    lv_obj_t *batt_row = lv_obj_create(panel);
    lv_obj_set_pos(batt_row, 514, 16);
    lv_obj_set_size(batt_row, 270, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(batt_row, 0, 0);
    lv_obj_set_style_border_width(batt_row, 0, 0);
    lv_obj_set_style_pad_all(batt_row, 0, 0);
    lv_obj_set_style_pad_column(batt_row, 24, 0);
    lv_obj_set_style_layout(batt_row, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(batt_row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_main_place(batt_row, LV_FLEX_ALIGN_END, 0);
    // START (arriba) en vez de CENTER: asi los LEDs quedan a la altura del
    // texto "ROBOT"/"CONSOLA" (primera fila de cada celda), no centrados
    // contra toda la altura de la barra+valor.
    lv_obj_set_style_flex_cross_place(batt_row, LV_FLEX_ALIGN_START, 0);
    lv_obj_remove_flag(batt_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(batt_row, LV_OBJ_FLAG_CLICKABLE);

    // LEDs de estado — solo LEDs, sin letrero, al lado de la bateria ROBOT,
    // en fila horizontal a la altura del texto "ROBOT". Azul = Bluetooth
    // conectado, Verde = robot online. Se mantienen en espejo con
    // objects.led_bluetooth/led_online (ver HMI_REG_*_INDICATOR).
    // Margen alrededor de los LEDs para que el halo/brillo (que crece con
    // lv_led segun el brillo) no quede recortado contra el borde del
    // contenedor — mas ancho a los costados que arriba/abajo para no
    // desalinear la fila con la altura del texto "ROBOT".
    lv_obj_t *led_col = lv_obj_create(batt_row);
    lv_obj_set_size(led_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(led_col, 0, 0);
    lv_obj_set_style_border_width(led_col, 0, 0);
    lv_obj_set_style_pad_top(led_col, 3, 0);
    lv_obj_set_style_pad_bottom(led_col, 3, 0);
    lv_obj_set_style_pad_left(led_col, 10, 0);
    lv_obj_set_style_pad_right(led_col, 10, 0);
    lv_obj_set_style_pad_column(led_col, 20, 0);
    lv_obj_set_style_layout(led_col, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(led_col, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_cross_place(led_col, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_remove_flag(led_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(led_col, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *led_bt = lv_led_create(led_col);
    s_bigview_led_bt = led_bt;
    lv_obj_set_size(led_bt, 12, 12);
    lv_led_set_color(led_bt, lv_color_hex(0xff0034ff));
    lv_led_set_brightness(led_bt, objects.led_bluetooth ? lv_led_get_brightness(objects.led_bluetooth) : 0);

    lv_obj_t *led_on = lv_led_create(led_col);
    s_bigview_led_online = led_on;
    lv_obj_set_size(led_on, 12, 12);
    lv_led_set_color(led_on, lv_color_hex(0xff00971c));
    lv_led_set_brightness(led_on, objects.led_online ? lv_led_get_brightness(objects.led_online) : 0);

    battery_cell_create(batt_row, g_lang->panel_drive_robot_batt,
        &s_bigview_robot_batt_bar, &s_bigview_robot_batt_label, &s_bigview_robot_batt_caption);
    battery_cell_create(batt_row, g_lang->panel_drive_console_batt,
        &s_bigview_console_batt_bar, &s_bigview_console_batt_label, &s_bigview_console_batt_caption);

    bigview_battery_refresh();
}

// Logica de apertura compartida por el boton del logo (ya corre con el
// lock de LVGL tomado, por venir de un evento) y por el combo de joystick
// adelante->atras detectado en HMI_REG_MOTOR (background, requiere que el
// llamador tome HMI_LV_LOCKED el mismo).
static void encoder_bigview_open(void)
{
    if (!s_encoder_bigview_panel) encoder_bigview_create();
    if (!s_encoder_bigview_panel) return;
    encoder_dashboard_refresh();
    bigview_battery_refresh();
    lv_obj_move_foreground(s_encoder_bigview_panel);
    lv_obj_remove_flag(s_encoder_bigview_panel, LV_OBJ_FLAG_HIDDEN);
}

static void logo_bigview_open_cb(lv_event_t *e)
{
    (void)e;
    encoder_bigview_open();
}

// Engancha el logo como boton invisible — se llama una vez desde app_main()
// junto con el resto del cableado dinamico posterior a ui_init().
static void logo_secret_button_wire(void)
{
    if (!objects.obj1) return;
    lv_obj_add_flag(objects.obj1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(objects.obj1, logo_bigview_open_cb, LV_EVENT_CLICKED, NULL);
}

// Estado de conexion BLE (Settings > Bluetooth) — separado para poder
// re-traducir el label de estado al cambiar de idioma (hmi_extra_panels_apply_lang)
// sin depender de que vuelva a llegar HMI_REG_BLUETOOTH_INDICATOR.
static bool s_bt_connected = false;
static uint16_t s_bt_mac_hi = 0; // cache de HMI_REG_BLUETOOTH_MAC_HI hasta que llegue el LO
static const char *bt_status_text(bool connected)
{
    return connected ? g_lang->lbl_bluetooth_connected : g_lang->lbl_bluetooth_disconnected;
}

// Texto traducido del estado de OTA_STATUS (0x1E) de la consola — separado
// para poder reusarlo tanto al recibir el registro como al recalcar el
// idioma (hmi_extra_panels_apply_lang) mientras un estado ya esta mostrado.
static int32_t s_console_ota_last_value = -1; // -1 = todavia no llego ninguno
static const char *console_ota_status_text(int32_t value)
{
    switch (value) {
        case 0: return g_lang->ota_status_checking;
        case 1: return g_lang->ota_status_no_conn;
        case 2: return g_lang->ota_status_up_to_date;
        case 3: return g_lang->ota_status_updating;
        case 4: return g_lang->ota_status_success;
        case 5: return g_lang->ota_status_failed;
        default: return g_lang->ota_status_unknown;
    }
}

// Llamado desde lang_apply() (ui/lang.c) al final, para refrescar textos que
// dependen de estado que solo main.c conoce (boton/estado de WiFi, toggle
// Pies/Metros, formula del footer de Encoder) cuando cambia el idioma.
void hmi_extra_panels_apply_lang(void)
{
    // Boton toggle de WiFi: el texto depende del estado actual (activado/desactivado)
    if (objects.update_toggle_btn) {
        lv_obj_t *lbl = lv_obj_get_child(objects.update_toggle_btn, 0);
        if (lbl) lv_label_set_text(lbl, s_wifi_enabled ? g_lang->btn_disable_wifi : g_lang->btn_enable_wifi);
    }
    // Estado de conexion: solo corregimos el caso "apagado" en reposo; los
    // textos de Conectando/Reconectando/Conectado los pisa el proximo evento
    // de WiFi si esta prendido.
    if (!s_wifi_enabled) {
        update_panel_set_status(g_lang->lbl_wifi_off, UPDATE_LED_OFF);
    }
    // "Network: <SSID>" — solo si ya esta visible (WiFi realmente conectado),
    // re-arma el texto con el idioma nuevo y el SSID actual.
    if (objects.update_network_label && !lv_obj_has_flag(objects.update_network_label, LV_OBJ_FLAG_HIDDEN)) {
        update_network_label_set_visible(true);
    }

    // Toggle Pies/Metros del dashboard
    if (s_encoder_btn_feet) {
        lv_obj_t *lbl = lv_obj_get_child(s_encoder_btn_feet, 0);
        if (lbl) lv_label_set_text(lbl, g_lang->btn_feet);
    }
    if (s_encoder_btn_meters) {
        lv_obj_t *lbl = lv_obj_get_child(s_encoder_btn_meters, 0);
        if (lbl) lv_label_set_text(lbl, g_lang->btn_meters);
    }

    // Caption Robot/Console Voltage|Percent: lang_apply() ya puso la variante
    // "Voltage" via L->lbl_robot_voltage/lbl_console_voltage; si el usuario
    // esta en modo porcentaje hay que pisarla con la variante correcta en el
    // idioma nuevo.
    if (g_bat_display_percent) {
        if (objects.robot_voltage_caption)   lv_label_set_text(objects.robot_voltage_caption,   g_lang->lbl_robot_percent);
        if (objects.console_voltage_caption) lv_label_set_text(objects.console_voltage_caption, g_lang->lbl_console_percent);
    }

    // Estado de conexion BLE (Settings > Bluetooth): Conectado/Sin conexion
    // depende de s_bt_connected, no hay que esperar a que llegue de nuevo
    // HMI_REG_BLUETOOTH_INDICATOR.
    if (objects.bt_panel_status_label) {
        lv_label_set_text(objects.bt_panel_status_label, bt_status_text(s_bt_connected));
    }

    // Estado de OTA_STATUS de la consola (System Info > Update > Console):
    // si ya llego algun valor, re-traducirlo en el idioma nuevo.
    if (s_console_ota_last_value >= 0 && objects.console_ota_status_label) {
        lv_label_set_text(objects.console_ota_status_label, console_ota_status_text(s_console_ota_last_value));
    }

    // Formula del footer de Encoder (numeros + texto en el idioma actual)
    enc_recalc_footer();

    // Boton Iniciar/Pausar/Reanudar de Giro Automatico: lang_apply() ya puso
    // el texto default (idle) via L->btn_auto_rotation; si hay una
    // reproduccion en curso hay que pisarlo con el texto de estado correcto.
    if (g_play_state == AUTO_PLAY_MOVING_TO_START || g_play_state == AUTO_PLAY_PLAYING) {
        modes_giro_automatico_set_state(g_lang->btn_pausar_auto_rotation, true);
    } else if (g_play_state == AUTO_PLAY_PAUSED) {
        modes_giro_automatico_set_state(g_lang->btn_reanudar_auto_rotation, true);
    }

    // Panel de manejo (logo/combo joystick) — textos fijos que no dependen
    // de ningun estado, solo re-traducirlos.
    if (s_bigview_distance_caption) lv_label_set_text(s_bigview_distance_caption, g_lang->panel_drive_distance_caption);
    if (s_bigview_trace_caption)    lv_label_set_text(s_bigview_trace_caption, g_lang->panel_drive_trace_caption);
    if (s_bigview_robot_batt_caption)   lv_label_set_text(s_bigview_robot_batt_caption, g_lang->panel_drive_robot_batt);
    if (s_bigview_console_batt_caption) lv_label_set_text(s_bigview_console_batt_caption, g_lang->panel_drive_console_batt);
}

// Forward declarations
static void dev_panel_close(void);
static void dev_panel_create(void);
static void pin_open(bool for_change);
static void pin_close(void);
static void sn_open(void);
static void sn_close(void);
static void dn_open(void);
static void logs_show_cb(lv_event_t *e);
static void logs_panel_create(lv_obj_t *content_area, lv_obj_t *nav_col);
static void val_panel_create(lv_obj_t *content_area, lv_obj_t *nav_col);
static void joy_panel_create(lv_obj_t *content_area, lv_obj_t *nav_col);
static void serial_panel_create(lv_obj_t *content_area, lv_obj_t *nav_col);
static void test_panel_create(lv_obj_t *content_area, lv_obj_t *nav_col);
static void test_show_cb(lv_event_t *e);
static void panel_destroy(uint8_t bit);
static void panel_create_one(uint8_t bit);

// Callbacks — llamados desde contexto LVGL, NO usar esp_lv_adapter_lock
static void dev_cb_close(lv_event_t *e)   { dev_panel_close(); }
static void dev_cb_restart(lv_event_t *e) { esp_restart(); }

static void dev_cb_uart_j5(lv_event_t *e)
{
    dev_nvs_write_pin(NVS_KEY_UART_TX, 26);
    dev_nvs_write_pin(NVS_KEY_UART_RX, 27);
    ESP_LOGW(TAG, "[DEV] UART GPIO26/27 guardado, reiniciando...");
    esp_restart();
}
static void dev_cb_uart_header(lv_event_t *e)
{
    dev_nvs_write_pin(NVS_KEY_UART_TX, 51);
    dev_nvs_write_pin(NVS_KEY_UART_RX, 52);
    ESP_LOGW(TAG, "[DEV] UART GPIO51/52 guardado, reiniciando...");
    esp_restart();
}
static void dev_cb_toggle_log(lv_event_t *e)
{
    s_rx_log_enabled = !s_rx_log_enabled;
    lv_obj_t *lbl = lv_obj_get_child(lv_event_get_target(e), 0);
    if (lbl) lv_label_set_text(lbl, s_rx_log_enabled ? "UART LOG: ON" : "UART LOG: OFF");
}
static void dev_cb_edit_serial(lv_event_t *e) { sn_open(); }
static void dev_cb_edit_device_name(lv_event_t *e) { (void)e; dn_open(); }
static void dev_cb_change_pin(lv_event_t *e) { pin_open(true); }
static void dev_cb_reset_pin(lv_event_t *e)
{
    dev_nvs_write_pin_str(DEV_PIN_DEF);
    hmi_log(LOG_OK, "PIN reseteado a 1234");
    ESP_LOGW(TAG, "[DEV] PIN reseteado a " DEV_PIN_DEF);
}
static void dev_cb_reset_boot_count(lv_event_t *e)
{
    nvs_handle_t h;
    if (nvs_open("storage", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u32(h, "boot_count", 0);
        nvs_commit(h);
        nvs_close(h);
        hmi_log(LOG_OK, "Boot count reseteado a 0");
        ESP_LOGW(TAG, "[DEV] Boot count reseteado a 0");
    }
}
static void dev_heap_timer_cb(lv_timer_t *t)
{
    if (!s_dev_heap_label) return;
    char buf[36];
    snprintf(buf, sizeof(buf), "Heap libre: %lu B", (unsigned long)esp_get_free_heap_size());
    lv_label_set_text(s_dev_heap_label, buf);
}

static void dev_serial_add(const char *dir, const char *msg)
{
    snprintf(s_serial_lines[s_serial_head], SERIAL_LOG_LEN, "[%s] %s", dir, msg);
    s_serial_head = (s_serial_head + 1) % SERIAL_LOG_MAX;
    if (s_serial_count < SERIAL_LOG_MAX) s_serial_count++;
    s_serial_dirty = true;
}

static void serial_refresh_timer_cb(lv_timer_t *t)
{
    if (!s_serial_dirty || !s_serial_body) return;
    s_serial_dirty = false;
    static char buf[2048];
    buf[0] = '\0';
    int start = (s_serial_count < SERIAL_LOG_MAX) ? 0 : s_serial_head;
    for (int i = 0; i < s_serial_count; i++) {
        int idx = (start + i) % SERIAL_LOG_MAX;
        if (i > 0) {
            size_t l = strlen(buf);
            if (l + 1 < sizeof(buf)) { buf[l] = '\n'; buf[l + 1] = '\0'; }
        }
        strncat(buf, s_serial_lines[idx], sizeof(buf) - strlen(buf) - 1);
    }
    lv_label_set_text(s_serial_body, s_serial_count > 0 ? buf : "Esperando datos...");
    lv_obj_scroll_to_y(lv_obj_get_parent(s_serial_body), LV_COORD_MAX, LV_ANIM_OFF);
}

static void dev_cb_serial_clear(lv_event_t *e)
{
    s_serial_head = 0; s_serial_count = 0;
    memset(s_serial_lines, 0, sizeof(s_serial_lines));
    s_serial_dirty = false;
    if (s_serial_body) lv_label_set_text(s_serial_body, "Limpiado");
}

static void dev_joy_log_update(void)
{
    static uint32_t last_joy_log_ms = 0;
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
    if ((now_ms - last_joy_log_ms) < 200) return;
    last_joy_log_ms = now_ms;
    snprintf(s_joy_lines[s_joy_head], JOY_LOG_LEN,
             "J1X=%-5d J1Y=%-5d J2X=%-5d J2Y=%-5d P1=%-5d B1=%d B2=%d",
             s_joy_j1x, s_joy_j1y, s_joy_j2x, s_joy_j2y, s_p1_value, s_joy_btn1, s_joy_btn2);
    s_joy_head = (s_joy_head + 1) % JOY_LOG_MAX;
    if (s_joy_count < JOY_LOG_MAX) s_joy_count++;
    s_joy_dirty = true;
}

static void joy_refresh_timer_cb(lv_timer_t *t)
{
    if (!s_joy_dirty || !s_joy_body) return;
    s_joy_dirty = false;
    static char buf[2048];
    buf[0] = '\0';
    int start = (s_joy_count < JOY_LOG_MAX) ? 0 : s_joy_head;
    for (int i = 0; i < s_joy_count; i++) {
        int idx = (start + i) % JOY_LOG_MAX;
        if (i > 0) {
            size_t l = strlen(buf);
            if (l + 1 < sizeof(buf)) { buf[l] = '\n'; buf[l + 1] = '\0'; }
        }
        strncat(buf, s_joy_lines[idx], sizeof(buf) - strlen(buf) - 1);
    }
    lv_label_set_text(s_joy_body, s_joy_count > 0 ? buf : "Esperando datos joystick...");
    lv_obj_scroll_to_y(lv_obj_get_parent(s_joy_body), LV_COORD_MAX, LV_ANIM_OFF);
}

static void dev_cb_joy_clear(lv_event_t *e)
{
    s_joy_head = 0; s_joy_count = 0;
    memset(s_joy_lines, 0, sizeof(s_joy_lines));
    s_joy_dirty = false;
    if (s_joy_body) lv_label_set_text(s_joy_body, "Limpiado");
}

// Helpers UI — styled igual que los otros paneles de System Info
static lv_obj_t *dev_make_btn(lv_obj_t *parent, const char *text, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_width(btn, LV_PCT(100));
    lv_obj_set_height(btn, 44);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 8, 0);
    hmi_style_btn(btn, false);
    return btn;
}
static void dev_make_sep(lv_obj_t *parent, const char *text)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, hmi_theme_txt_secondary(), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_width(lbl, LV_PCT(100));
}
static void dev_make_info_row(lv_obj_t *parent, const char *key, const char *val)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_radius(row, 0, 0);
    lv_obj_set_style_layout(row, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(row, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_cross_place(row, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *k = lv_label_create(row);
    lv_obj_set_width(k, 155);
    lv_obj_set_height(k, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(k, hmi_theme_txt_secondary(), 0);
    lv_obj_set_style_text_font(k, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(k, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_long_mode(k, LV_LABEL_LONG_CLIP);
    lv_label_set_text(k, key);
    lv_obj_t *v = lv_label_create(row);
    lv_obj_set_flex_grow(v, 1);
    lv_obj_set_height(v, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(v, hmi_theme_txt_primary(), 0);
    lv_obj_set_style_text_font(v, &lv_font_montserrat_18, 0);
    lv_obj_set_style_pad_left(v, 8, 0);
    lv_label_set_text(v, val);
}

// ---- Serial Number editor ----

static void sn_close(void)
{
    if (s_sn_panel) { lv_obj_del(s_sn_panel); s_sn_panel = NULL; s_sn_display = NULL; }
    s_sn_len = 0; s_sn_buf[0] = '\0';
}

static void sn_key_cb(lv_event_t *e)
{
    if (!s_sn_panel) return;
    intptr_t k = (intptr_t)lv_event_get_user_data(e);

    if (k == 'X') {
        if (s_sn_len > 0) { s_sn_len--; s_sn_buf[s_sn_len] = '\0'; }
    } else if (k == 'K') {
        if (s_sn_len == 0) { sn_close(); return; }
        strncpy(s_serial_num, s_sn_buf, sizeof(s_serial_num));
        s_serial_num[sizeof(s_serial_num) - 1] = '\0';
        dev_nvs_write_serial(s_serial_num);
        dev_serial_labels_update();
        hmi_log(LOG_OK, "N/S actualizado");
        ESP_LOGW(TAG, "[DEV] Serial guardado: %s", s_serial_num);
        sn_close();
        return;
    } else if (k == 'C') {
        s_sn_len = 0; s_sn_buf[0] = '\0';
    } else if (s_sn_len < SERIAL_NUM_LEN) {
        s_sn_buf[s_sn_len++] = (char)k;
        s_sn_buf[s_sn_len]   = '\0';
    }
    if (s_sn_display) {
        char disp[SERIAL_NUM_LEN + 8];
        snprintf(disp, sizeof(disp), "RD90C-%s", s_sn_len > 0 ? s_sn_buf : "______");
        lv_label_set_text(s_sn_display, disp);
    }
}

static void sn_open(void)
{
    if (s_sn_panel) return;
    s_sn_len = 0;
    strncpy(s_sn_buf, s_serial_num, sizeof(s_sn_buf));
    s_sn_len = (int)strlen(s_sn_buf);

    lv_obj_t *p = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_size(p, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_70, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_radius(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    s_sn_panel = p;

    lv_obj_t *card = lv_obj_create(p);
    lv_obj_set_size(card, 310, LV_SIZE_CONTENT);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_color(card, hmi_theme_accent(), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 14, 0);
    lv_obj_set_style_pad_row(card, 10, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, "EDITAR N\xC2\xB0 DE SERIE");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, hmi_theme_accent(), 0);
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

    s_sn_display = lv_label_create(card);
    char disp[SERIAL_NUM_LEN + 8];
    snprintf(disp, sizeof(disp), "RD90C-%s", s_sn_buf);
    lv_label_set_text(s_sn_display, disp);
    lv_obj_set_style_text_font(s_sn_display, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_sn_display, hmi_theme_txt_primary(), 0);
    lv_obj_set_style_text_align(s_sn_display, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_sn_display, LV_PCT(100));

    lv_obj_t *hint = lv_label_create(card);
    lv_label_set_text(hint, "Max 6 digitos");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hint, hmi_theme_txt_secondary(), 0);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(hint, LV_PCT(100));

    // Teclado numérico
    lv_obj_t *grid = lv_obj_create(card);
    lv_obj_set_width(grid, LV_PCT(100));
    lv_obj_set_height(grid, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(grid, 0, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_style_pad_gap(grid, 6, 0);
    lv_obj_remove_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    const char *keys[] = {"1","2","3","4","5","6","7","8","9","<","0","OK"};
    const intptr_t ids[] = {'1','2','3','4','5','6','7','8','9','X','0','K'};
    for (int i = 0; i < 12; i++) {
        lv_obj_t *btn = lv_button_create(grid);
        lv_obj_set_size(btn, 72, 52);
        lv_obj_add_event_cb(btn, sn_key_cb, LV_EVENT_CLICKED, (void *)ids[i]);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_shadow_opa(btn, 0, 0);
        if (ids[i] == 'K')
            lv_obj_set_style_bg_color(btn, hmi_theme_accent(), 0);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, keys[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
        lv_obj_center(lbl);
    }

    lv_obj_t *cancel = lv_button_create(card);
    lv_obj_set_width(cancel, LV_PCT(100));
    lv_obj_set_style_bg_color(cancel, lv_color_hex(0x333333), 0);
    lv_obj_set_style_shadow_opa(cancel, 0, 0);
    lv_obj_set_style_radius(cancel, 8, 0);
    lv_obj_add_event_cb(cancel, (lv_event_cb_t)sn_close, LV_EVENT_CLICKED, NULL);
    lv_obj_t *clbl = lv_label_create(cancel);
    lv_label_set_text(clbl, "Cancelar");
    lv_obj_set_style_text_font(clbl, &lv_font_montserrat_14, 0);
    lv_obj_center(clbl);
}

// ---- Device Name editor (texto libre, teclado LVGL) ----
static lv_obj_t *s_dn_panel = NULL;
static lv_obj_t *s_dn_ta    = NULL;

static void dn_close(void)
{
    if (s_dn_panel) { lv_obj_del(s_dn_panel); s_dn_panel = NULL; s_dn_ta = NULL; }
}

static void dn_save_cb(lv_event_t *e)
{
    (void)e;
    if (!s_dn_ta) return;
    const char *txt = lv_textarea_get_text(s_dn_ta);
    if (txt[0] == '\0') { dn_close(); return; }
    dev_nvs_write_device_name(txt);
    if (objects.sysinfo_device_name_value) lv_label_set_text(objects.sysinfo_device_name_value, txt);
    if (objects.settings_user_name_value)  lv_label_set_text(objects.settings_user_name_value, txt);
    hmi_log(LOG_OK, "Nombre del dispositivo actualizado");
    dn_close();
}

static void dn_cancel_cb(lv_event_t *e) { (void)e; dn_close(); }

static void dn_open(void)
{
    if (s_dn_panel) return;

    lv_obj_t *p = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_size(p, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_70, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_radius(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    s_dn_panel = p;

    lv_obj_t *title = lv_label_create(p);
    lv_label_set_text(title, g_lang->title_device_name_editor);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, hmi_theme_accent(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

    lv_obj_t *ta = lv_textarea_create(p);
    s_dn_ta = ta;
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, DEV_NAME_MAX - 1);
    char cur[DEV_NAME_MAX];
    dev_nvs_read_device_name(cur, sizeof(cur));
    lv_textarea_set_text(ta, cur);
    lv_obj_set_size(ta, 460, 44);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_text_font(ta, &lv_font_montserrat_18, 0);

    lv_obj_t *save = lv_button_create(p);
    lv_obj_set_size(save, 140, 40);
    lv_obj_align(save, LV_ALIGN_TOP_MID, -76, 104);
    lv_obj_set_style_bg_color(save, hmi_theme_accent(), 0);
    lv_obj_set_style_radius(save, 8, 0);
    lv_obj_set_style_shadow_opa(save, 0, 0);
    lv_obj_add_event_cb(save, dn_save_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *save_lbl = lv_label_create(save);
    lv_label_set_text(save_lbl, g_lang->btn_save);
    lv_obj_set_style_text_color(save_lbl, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_text_font(save_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(save_lbl);

    lv_obj_t *cancel = lv_button_create(p);
    lv_obj_set_size(cancel, 140, 40);
    lv_obj_align(cancel, LV_ALIGN_TOP_MID, 76, 104);
    lv_obj_set_style_bg_color(cancel, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(cancel, 8, 0);
    lv_obj_set_style_shadow_opa(cancel, 0, 0);
    lv_obj_add_event_cb(cancel, dn_cancel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cancel_lbl = lv_label_create(cancel);
    lv_label_set_text(cancel_lbl, g_lang->btn_cancel);
    lv_obj_set_style_text_font(cancel_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(cancel_lbl);

    lv_obj_t *kb = lv_keyboard_create(p);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_set_size(kb, LV_HOR_RES, 220);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
}

// ---- Settings/Update > WiFi editor (SSID + contrasena, texto libre, con
// escaneo opcional + verificacion por conexion de prueba antes de guardar)
// ----
static lv_obj_t *s_wifi_panel      = NULL;
static lv_obj_t *s_wifi_ssid_ta    = NULL;
static lv_obj_t *s_wifi_pass_ta    = NULL;
static lv_obj_t *s_wifi_kb         = NULL;
static lv_obj_t *s_wifi_scan_btn   = NULL;
static lv_obj_t *s_wifi_save_btn   = NULL;
static lv_obj_t *s_wifi_status_lbl = NULL;
static lv_obj_t *s_wifi_scan_list  = NULL;

// Margen antes de escanear: hay que desconectar cualquier intento de
// conexion en curso antes de esp_wifi_scan_start() (ver s_wifi_scan_active
// mas arriba), y darle un instante a que "asiente".
static lv_timer_t *s_wifi_scan_delay_timer = NULL;
#define WIFI_SCAN_DELAY_MS 300

static void wifi_editor_set_busy(bool busy)
{
    if (s_wifi_scan_btn) { if (busy) lv_obj_add_state(s_wifi_scan_btn, LV_STATE_DISABLED); else lv_obj_remove_state(s_wifi_scan_btn, LV_STATE_DISABLED); }
    if (s_wifi_save_btn) { if (busy) lv_obj_add_state(s_wifi_save_btn, LV_STATE_DISABLED); else lv_obj_remove_state(s_wifi_save_btn, LV_STATE_DISABLED); }
}

// Prende el WiFi por el MISMO camino que el boton "Activar WiFi" de Update
// (hmi_wifi_set_enabled), asi el boton queda sincronizado — en vez de
// prender el driver "por atras" con wifi_driver_ensure_ready(). Si ya estaba
// prendido de antes (uso real del WiFi de actualizacion), no marca
// s_wifi_enabled_by_editor: no es este dialogo quien decide cuando apagarlo.
static void wifi_editor_ensure_radio_on(void)
{
    if (!s_wifi_enabled) {
        // Se arma ANTES de prender, no despues: WIFI_EVENT_STA_START se
        // procesa en la tarea del event loop de ESP-IDF, que podria correr
        // antes de que esta funcion termine — hmi_wifi_set_enabled(true)
        // tiene que encontrar el flag ya en true para no auto-conectar.
        s_wifi_enabled_by_editor = true;
        hmi_wifi_set_enabled(true);
    }
}

// Definida junto con el resto del escaneo, mas abajo — declarada aca porque
// wifi_editor_close() (arriba de esa seccion en el archivo) la necesita.
static void wifi_editor_scan_session_end(void);

static void wifi_editor_close(void)
{
    if (s_wifi_scan_delay_timer) { lv_timer_delete(s_wifi_scan_delay_timer); s_wifi_scan_delay_timer = NULL; }
    if (s_wifi_scan_active) wifi_editor_scan_session_end();
    if (s_wifi_enabled_by_editor) {
        s_wifi_enabled_by_editor = false;
        hmi_wifi_set_enabled(false);
    }
    if (s_wifi_panel) {
        lv_obj_del(s_wifi_panel);
        s_wifi_panel = NULL; s_wifi_ssid_ta = NULL; s_wifi_pass_ta = NULL; s_wifi_kb = NULL;
        s_wifi_scan_btn = NULL; s_wifi_save_btn = NULL; s_wifi_status_lbl = NULL; s_wifi_scan_list = NULL;
    }
}

// Guarda directo — sin conectar de prueba primero: si la red que se esta
// guardando no esta al alcance en este momento (router apagado, se esta
// configurando de antemano, era para pruebas y ya no existe, etc.) igual
// queda guardada, reemplazando la anterior. Si el WiFi de actualizacion ya
// esta activado, ademas aplica la red nueva de una (reconecta con las
// credenciales recien guardadas), pero eso no condiciona el guardado.
static void wifi_editor_save_cb(lv_event_t *e)
{
    (void)e;
    if (!s_wifi_ssid_ta || !s_wifi_pass_ta) return;
    const char *ssid = lv_textarea_get_text(s_wifi_ssid_ta);
    const char *pass = lv_textarea_get_text(s_wifi_pass_ta);
    if (ssid[0] == '\0') { wifi_editor_close(); return; }

    dev_nvs_write_wifi_ssid(ssid);
    dev_nvs_write_wifi_pass(pass);
    if (objects.settings_wifi_ssid_value) lv_label_set_text(objects.settings_wifi_ssid_value, ssid);

    if (s_wifi_enabled) {
        wifi_config_t wifi_config = {0};
        strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
        strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password) - 1);
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        esp_wifi_disconnect();
        esp_wifi_connect();
    }

    hmi_log(LOG_OK, "WiFi actualizado");
    wifi_editor_close();
}

static void wifi_editor_cancel_cb(lv_event_t *e) { (void)e; wifi_editor_close(); }

// Un solo teclado compartido entre los 2 campos: se re-liga al campo que
// tiene el foco en ese momento.
static void wifi_editor_field_focus_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_target(e);
    if (s_wifi_kb) lv_keyboard_set_textarea(s_wifi_kb, ta);
}

// Fila superior de la lista de resultados: vuelve a carga manual sin elegir
// ninguna red (redes ocultas, o el usuario prefiere escribir el nombre).
static void wifi_editor_scan_cancel_cb(lv_event_t *e)
{
    (void)e;
    if (s_wifi_scan_list) lv_obj_add_flag(s_wifi_scan_list, LV_OBJ_FLAG_HIDDEN);
    if (s_wifi_kb) lv_obj_remove_flag(s_wifi_kb, LV_OBJ_FLAG_HIDDEN);
    if (s_wifi_status_lbl) lv_label_set_text(s_wifi_status_lbl, "");
    wifi_editor_set_busy(false);
}

static void wifi_editor_scan_row_clicked_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    if (!s_wifi_scan_list || !s_wifi_ssid_ta) return;
    const char *ssid = lv_list_get_button_text(s_wifi_scan_list, btn);
    lv_textarea_set_text(s_wifi_ssid_ta, ssid);
    lv_obj_add_flag(s_wifi_scan_list, LV_OBJ_FLAG_HIDDEN);
    if (s_wifi_kb && s_wifi_pass_ta) {
        lv_obj_remove_flag(s_wifi_kb, LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(s_wifi_kb, s_wifi_pass_ta);
    }
    if (s_wifi_status_lbl) lv_label_set_text(s_wifi_status_lbl, "");
    wifi_editor_set_busy(false);
}

// Cierra la "sesion" de escaneo: deja de suprimir el auto-reconectar del
// WiFi de actualizacion y, si sigue activado (y no hay una verificacion en
// curso, que maneja su propia conexion), retoma el intento normal a la red
// guardada — lo mismo que hacia solo antes de que el editor pidiera escanear.
static void wifi_editor_scan_session_end(void)
{
    s_wifi_scan_active = false;
    if (s_wifi_enabled) {
        esp_wifi_connect();
    }
}

static void wifi_editor_scan_done(void)
{
    if (!s_wifi_panel || !s_wifi_scan_list) { wifi_editor_scan_session_end(); return; } // el dialogo se cerro mientras escaneaba

    uint16_t num = 0;
    esp_wifi_scan_get_ap_num(&num);

    lv_obj_clean(s_wifi_scan_list);
    lv_obj_t *manual_row = lv_list_add_button(s_wifi_scan_list, LV_SYMBOL_EDIT, g_lang->btn_wifi_scan_manual);
    lv_obj_add_event_cb(manual_row, wifi_editor_scan_cancel_cb, LV_EVENT_CLICKED, NULL);

    uint16_t shown = 0;
    if (num > 0) {
        if (num > 20) num = 20; // limite razonable de lista/memoria
        wifi_ap_record_t *recs = malloc(sizeof(wifi_ap_record_t) * num);
        if (recs) {
            esp_wifi_scan_get_ap_records(&num, recs);
            for (int i = 0; i < num; i++) {
                if (recs[i].ssid[0] == '\0') continue; // red oculta, sin nombre que listar
                lv_obj_t *row = lv_list_add_button(s_wifi_scan_list, LV_SYMBOL_WIFI, (const char *)recs[i].ssid);
                lv_obj_add_event_cb(row, wifi_editor_scan_row_clicked_cb, LV_EVENT_CLICKED, NULL);
                shown++;
            }
            free(recs);
        }
    }

    if (s_wifi_status_lbl) {
        if (shown > 0) {
            char buf[48];
            snprintf(buf, sizeof(buf), g_lang->lbl_wifi_scan_found_fmt, (int)shown);
            lv_label_set_text(s_wifi_status_lbl, buf);
        } else {
            lv_label_set_text(s_wifi_status_lbl, g_lang->lbl_wifi_scan_empty);
        }
    }
    wifi_editor_set_busy(false);
    wifi_editor_scan_session_end();
}

static void wifi_editor_do_scan_start(void)
{
    if (!s_wifi_panel) { wifi_editor_scan_session_end(); return; } // el dialogo se cerro mientras esperaba

    esp_err_t err = esp_wifi_scan_start(NULL, false);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_wifi_scan_start fallo: %s", esp_err_to_name(err));
        if (s_wifi_status_lbl) lv_label_set_text(s_wifi_status_lbl, g_lang->lbl_wifi_scan_busy);
        if (s_wifi_scan_list) lv_obj_add_flag(s_wifi_scan_list, LV_OBJ_FLAG_HIDDEN);
        if (s_wifi_kb) lv_obj_remove_flag(s_wifi_kb, LV_OBJ_FLAG_HIDDEN);
        wifi_editor_set_busy(false);
        wifi_editor_scan_session_end();
    }
    // si arranco bien, s_wifi_scan_active sigue en true hasta wifi_editor_scan_done()
}

static void wifi_editor_scan_delay_cb(lv_timer_t *t)
{
    lv_timer_delete(t);
    s_wifi_scan_delay_timer = NULL;
    wifi_editor_do_scan_start();
}

static void wifi_editor_scan_start_cb(lv_event_t *e)
{
    (void)e;
    wifi_editor_ensure_radio_on();

    if (s_wifi_kb) lv_obj_add_flag(s_wifi_kb, LV_OBJ_FLAG_HIDDEN);
    if (s_wifi_scan_list) {
        lv_obj_clean(s_wifi_scan_list);
        lv_obj_remove_flag(s_wifi_scan_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_wifi_status_lbl) lv_label_set_text(s_wifi_status_lbl, g_lang->lbl_wifi_scanning);
    wifi_editor_set_busy(true);

    // El WiFi de actualizacion, una vez activado, reintenta solo conectarse
    // a la red guardada de fondo — si esa red no esta al alcance, el radio
    // puede estar "conectando" casi todo el tiempo, y esp_wifi_scan_start()
    // falla justo en ese estado. Cortar ese intento y esperar un margen
    // antes de escanear evita el "escaneo no disponible" (tambien cubre el
    // caso mas raro de que el radio, remoto por SDIO a un coprocesador
    // esp_hosted, todavia no hubiera terminado de arrancar).
    s_wifi_scan_active = true;
    esp_wifi_disconnect(); // no-op si no habia nada conectando/conectado

    if (s_wifi_scan_delay_timer) lv_timer_delete(s_wifi_scan_delay_timer);
    s_wifi_scan_delay_timer = lv_timer_create(wifi_editor_scan_delay_cb, WIFI_SCAN_DELAY_MS, NULL);
}

void hmi_open_wifi_editor(void)
{
    if (s_wifi_panel) return;

    s_wifi_enabled_by_editor = false; // defensivo: cada apertura arranca sin arrastrar estado
    s_wifi_scan_active       = false;

    lv_obj_t *p = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_size(p, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_70, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_radius(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    s_wifi_panel = p;

    lv_obj_t *title = lv_label_create(p);
    lv_label_set_text(title, g_lang->title_wifi_editor);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, hmi_theme_accent(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *ssid_lbl = lv_label_create(p);
    lv_label_set_text(ssid_lbl, g_lang->lbl_wifi_ssid);
    lv_obj_set_style_text_font(ssid_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ssid_lbl, lv_color_hex(0xffaaaaaa), 0);
    lv_obj_align(ssid_lbl, LV_ALIGN_TOP_MID, -220, 38);

    lv_obj_t *ssid_ta = lv_textarea_create(p);
    s_wifi_ssid_ta = ssid_ta;
    lv_textarea_set_one_line(ssid_ta, true);
    lv_textarea_set_max_length(ssid_ta, WIFI_SSID_MAX - 1);
    char cur_ssid[WIFI_SSID_MAX];
    dev_nvs_read_wifi_ssid(cur_ssid, sizeof(cur_ssid));
    lv_textarea_set_text(ssid_ta, cur_ssid[0] ? cur_ssid : WIFI_SSID);
    lv_obj_set_size(ssid_ta, 300, 40);
    lv_obj_align(ssid_ta, LV_ALIGN_TOP_MID, -80, 56);
    lv_obj_set_style_text_font(ssid_ta, &lv_font_montserrat_18, 0);
    lv_obj_remove_flag(ssid_ta, LV_OBJ_FLAG_SCROLLABLE); // sin esto, LVGL intenta hacer scroll al campo al escribir
    lv_obj_add_event_cb(ssid_ta, wifi_editor_field_focus_cb, LV_EVENT_FOCUSED, NULL);

    lv_obj_t *scan_btn = lv_button_create(p);
    s_wifi_scan_btn = scan_btn;
    lv_obj_set_size(scan_btn, 150, 40);
    lv_obj_align(scan_btn, LV_ALIGN_TOP_MID, 155, 56);
    lv_obj_set_style_bg_color(scan_btn, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(scan_btn, 8, 0);
    lv_obj_set_style_shadow_opa(scan_btn, 0, 0);
    lv_obj_add_event_cb(scan_btn, wifi_editor_scan_start_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *scan_lbl = lv_label_create(scan_btn);
    lv_label_set_text(scan_lbl, g_lang->btn_wifi_scan);
    lv_obj_set_style_text_font(scan_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(scan_lbl);

    lv_obj_t *pass_lbl = lv_label_create(p);
    lv_label_set_text(pass_lbl, g_lang->lbl_wifi_pass);
    lv_obj_set_style_text_font(pass_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(pass_lbl, lv_color_hex(0xffaaaaaa), 0);
    lv_obj_align(pass_lbl, LV_ALIGN_TOP_MID, -220, 106);

    lv_obj_t *pass_ta = lv_textarea_create(p);
    s_wifi_pass_ta = pass_ta;
    lv_textarea_set_one_line(pass_ta, true);
    lv_textarea_set_max_length(pass_ta, WIFI_PASS_MAX - 1);
    char cur_pass[WIFI_PASS_MAX];
    dev_nvs_read_wifi_pass(cur_pass, sizeof(cur_pass));
    lv_textarea_set_text(pass_ta, cur_pass[0] ? cur_pass : WIFI_PASS);
    lv_obj_set_size(pass_ta, 460, 40);
    lv_obj_align(pass_ta, LV_ALIGN_TOP_MID, 0, 124);
    lv_obj_set_style_text_font(pass_ta, &lv_font_montserrat_18, 0);
    lv_obj_remove_flag(pass_ta, LV_OBJ_FLAG_SCROLLABLE); // sin esto, LVGL intenta hacer scroll al campo al escribir
    lv_obj_add_event_cb(pass_ta, wifi_editor_field_focus_cb, LV_EVENT_FOCUSED, NULL);

    lv_obj_t *save = lv_button_create(p);
    s_wifi_save_btn = save;
    lv_obj_set_size(save, 140, 40);
    lv_obj_align(save, LV_ALIGN_TOP_MID, -76, 174);
    lv_obj_set_style_bg_color(save, hmi_theme_accent(), 0);
    lv_obj_set_style_radius(save, 8, 0);
    lv_obj_set_style_shadow_opa(save, 0, 0);
    lv_obj_add_event_cb(save, wifi_editor_save_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *save_lbl = lv_label_create(save);
    lv_label_set_text(save_lbl, g_lang->btn_save);
    lv_obj_set_style_text_color(save_lbl, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_text_font(save_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(save_lbl);

    lv_obj_t *cancel = lv_button_create(p);
    lv_obj_set_size(cancel, 140, 40);
    lv_obj_align(cancel, LV_ALIGN_TOP_MID, 76, 174);
    lv_obj_set_style_bg_color(cancel, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(cancel, 8, 0);
    lv_obj_set_style_shadow_opa(cancel, 0, 0);
    lv_obj_add_event_cb(cancel, wifi_editor_cancel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cancel_lbl = lv_label_create(cancel);
    lv_label_set_text(cancel_lbl, g_lang->btn_cancel);
    lv_obj_set_style_text_font(cancel_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(cancel_lbl);

    lv_obj_t *status_lbl = lv_label_create(p);
    s_wifi_status_lbl = status_lbl;
    lv_obj_set_size(status_lbl, LV_HOR_RES - 40, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(status_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(status_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status_lbl, lv_color_hex(0xffaaaaaa), 0);
    lv_obj_align(status_lbl, LV_ALIGN_TOP_MID, 0, 210);
    lv_label_set_text(status_lbl, "");

    lv_obj_t *kb = lv_keyboard_create(p);
    s_wifi_kb = kb;
    lv_keyboard_set_textarea(kb, ssid_ta);
    lv_obj_set_size(kb, LV_HOR_RES, 220);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Lista de resultados de escaneo — ocupa el mismo lugar que el teclado
    // (no hace falta escribir mientras se elige de la lista), oculta hasta
    // que se toca "Buscar redes".
    lv_obj_t *list = lv_list_create(p);
    s_wifi_scan_list = list;
    lv_obj_set_size(list, LV_HOR_RES, 220);
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);
}

// ---- PIN keypad ----

static void pin_update_display(void)
{
    if (!s_pin_display) return;
    char disp[DEV_PIN_LEN * 2 + 1] = "";
    for (int i = 0; i < DEV_PIN_LEN; i++) {
        if (i > 0) strcat(disp, " ");
        strcat(disp, i < s_pin_len ? "*" : "_");
    }
    lv_label_set_text(s_pin_display, disp);
}

static void pin_err_timer_cb(lv_timer_t *t)
{
    lv_timer_delete(t); s_pin_err_timer = NULL;
    s_pin_len = 0; s_pin_buf[0] = '\0';
    pin_update_display();
}

static void pin_key_cb(lv_event_t *e)
{
    if (!s_pin_panel) return;
    intptr_t k = (intptr_t)lv_event_get_user_data(e);

    if (k == 'X') {                          // borrar
        if (s_pin_len > 0) { s_pin_len--; s_pin_buf[s_pin_len] = '\0'; }
        pin_update_display();
    } else if (k == 'K') {                   // confirmar
        if (s_pin_len < DEV_PIN_LEN) return;
        if (s_pin_changing) {
            dev_nvs_write_pin_str(s_pin_buf);
            pin_close();
            hmi_log(LOG_OK, "PIN actualizado");
        } else {
            char stored[DEV_PIN_LEN + 1];
            dev_nvs_read_pin_str(stored, sizeof(stored));
            if (strcmp(s_pin_buf, stored) == 0) {
                pin_close();
                dev_panel_create();
            } else {
                if (s_pin_display) lv_label_set_text(s_pin_display, "INCORRECTO");
                if (!s_pin_err_timer)
                    s_pin_err_timer = lv_timer_create(pin_err_timer_cb, 1200, NULL);
            }
        }
    } else if (s_pin_len < DEV_PIN_LEN) {    // digito
        s_pin_buf[s_pin_len++] = (char)k;
        s_pin_buf[s_pin_len]   = '\0';
        pin_update_display();
    }
}

static void pin_cancel_cb(lv_event_t *e) { pin_close(); }

static lv_obj_t *pin_make_key(lv_obj_t *parent, const char *label, intptr_t key_id)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 75, 54);
    lv_obj_add_event_cb(btn, pin_key_cb, LV_EVENT_CLICKED, (void *)key_id);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_shadow_opa(btn, 0, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x2e2e2e), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x3a3a3a), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl);
    return btn;
}

static void pin_close(void)
{
    if (s_pin_err_timer) { lv_timer_delete(s_pin_err_timer); s_pin_err_timer = NULL; }
    if (s_pin_panel)     { lv_obj_del(s_pin_panel); s_pin_panel = NULL; s_pin_display = NULL; }
    s_pin_len = 0; s_pin_buf[0] = '\0';
}

static void pin_open(bool for_change)
{
    if (s_pin_panel) return;
    s_pin_changing = for_change;
    s_pin_len = 0; s_pin_buf[0] = '\0';

    // Fondo semitransparente
    lv_obj_t *p = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_size(p, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_70, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_radius(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    s_pin_panel = p;

    // Tarjeta central
    lv_obj_t *card = lv_obj_create(p);
    lv_obj_set_size(card, 290, LV_SIZE_CONTENT);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_color(card, hmi_theme_accent(), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 14, 0);
    lv_obj_set_style_pad_row(card, 10, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, for_change ? g_lang->title_change_pin : g_lang->title_dev_mode);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, hmi_theme_accent(), 0);
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *sub = lv_label_create(card);
    lv_label_set_text(sub, for_change ? g_lang->sub_change_pin
                                      : g_lang->sub_enter_pin);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(0x888888), 0);
    lv_obj_set_width(sub, LV_PCT(100));
    lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_CENTER, 0);

    // Display de digitos
    s_pin_display = lv_label_create(card);
    lv_label_set_text(s_pin_display, "_ _ _ _");
    lv_obj_set_style_text_font(s_pin_display, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_pin_display, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_color(s_pin_display, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_bg_opa(s_pin_display, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(s_pin_display, 10, 0);
    lv_obj_set_style_radius(s_pin_display, 8, 0);
    lv_obj_set_width(s_pin_display, LV_PCT(100));
    lv_obj_set_style_text_align(s_pin_display, LV_TEXT_ALIGN_CENTER, 0);

    // Grid del teclado numerico
    lv_obj_t *grid = lv_obj_create(card);
    lv_obj_set_width(grid, LV_PCT(100));
    lv_obj_set_height(grid, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(grid, 0, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_style_pad_row(grid, 6, 0);
    lv_obj_set_style_pad_column(grid, 6, 0);
    lv_obj_remove_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    pin_make_key(grid, "1", '1'); pin_make_key(grid, "2", '2'); pin_make_key(grid, "3", '3');
    pin_make_key(grid, "4", '4'); pin_make_key(grid, "5", '5'); pin_make_key(grid, "6", '6');
    pin_make_key(grid, "7", '7'); pin_make_key(grid, "8", '8'); pin_make_key(grid, "9", '9');
    pin_make_key(grid, "<",  'X'); pin_make_key(grid, "0", '0');
    lv_obj_t *ok_btn = pin_make_key(grid, "OK", 'K');
    lv_obj_set_style_bg_color(ok_btn, hmi_theme_accent(), 0);
    lv_obj_set_style_bg_color(ok_btn, hmi_theme_accent(), LV_PART_MAIN | LV_STATE_PRESSED);

    // Boton cancelar
    lv_obj_t *cancel = lv_button_create(card);
    lv_obj_set_size(cancel, LV_PCT(100), 40);
    lv_obj_add_event_cb(cancel, pin_cancel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(cancel, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(cancel, 8, 0);
    lv_obj_set_style_shadow_opa(cancel, 0, 0);
    lv_obj_t *cancel_lbl = lv_label_create(cancel);
    lv_label_set_text(cancel_lbl, g_lang->btn_cancel);
    lv_obj_center(cancel_lbl);
    lv_obj_set_style_text_color(cancel_lbl, lv_color_hex(0xaaaaaa), 0);
}

static void dev_panel_close(void)
{
    if (s_dev_heap_timer) { lv_timer_delete(s_dev_heap_timer); s_dev_heap_timer = NULL; }
    // Siempre eliminar el panel DEV y su boton
    if (s_dev_panel) { lv_obj_del(s_dev_panel); s_dev_panel = NULL; s_dev_heap_label = NULL; }
    if (s_dev_btn)   { lv_obj_del(s_dev_btn);   s_dev_btn   = NULL; }
    // Eliminar paneles que NO sean visibles permanentemente
    if (!(s_vis_panels & VIS_LOG)) {
        if (s_logs_panel) { lv_obj_del(s_logs_panel); s_logs_panel = NULL;
                            s_logs_tbox = NULL; objects.sysinfo_logs_body = NULL; }
        if (s_logs_btn)   { lv_obj_del(s_logs_btn);   s_logs_btn   = NULL; }
    }
    if (!(s_vis_panels & VIS_VAL)) {
        if (s_val_timer)  { lv_timer_delete(s_val_timer); s_val_timer = NULL; }
        if (s_val_panel)  { lv_obj_del(s_val_panel); s_val_panel = NULL; s_val_body = NULL; }
        if (s_val_btn)    { lv_obj_del(s_val_btn);   s_val_btn   = NULL; }
    }
    if (!(s_vis_panels & VIS_JOY)) {
        if (s_joy_timer)  { lv_timer_delete(s_joy_timer); s_joy_timer = NULL; }
        if (s_joy_panel)  { lv_obj_del(s_joy_panel); s_joy_panel = NULL; s_joy_body = NULL; }
        if (s_joy_btn)    { lv_obj_del(s_joy_btn);   s_joy_btn   = NULL; }
    }
    if (!(s_vis_panels & VIS_SER)) {
        if (s_serial_timer) { lv_timer_delete(s_serial_timer); s_serial_timer = NULL; }
        if (s_serial_panel) { lv_obj_del(s_serial_panel); s_serial_panel = NULL; s_serial_body = NULL; }
        if (s_serial_btn)   { lv_obj_del(s_serial_btn);   s_serial_btn   = NULL; }
    }
    if (!(s_vis_panels & VIS_TEST)) {
        if (s_test_panel) { lv_obj_del(s_test_panel); s_test_panel = NULL; s_test_status = NULL; s_test_vel_btn = NULL; }
        if (s_test_btn)   { lv_obj_del(s_test_btn);   s_test_btn   = NULL; }
    }
    // Quitar scroll si no quedan paneles visibles
    if (!s_vis_panels) {
        lv_obj_t *nav_col = lv_obj_get_parent(objects.sysinfo_btn_device);
        if (nav_col) lv_obj_remove_flag(nav_col, LV_OBJ_FLAG_SCROLLABLE);
    }
    action_sysinfo_btn_device(NULL);
    ESP_LOGW(TAG, "[DEV] Panel cerrado");
}

// Cuando el usuario pulsa el boton Serial en la barra de nav
static void serial_show_cb(lv_event_t *e)
{
    if (!s_serial_panel) return;
    lv_obj_add_flag(objects.sysinfo_content_device,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.sysinfo_content_version, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.sysinfo_content_guide,   LV_OBJ_FLAG_HIDDEN);
    if (objects.sysinfo_content_update) lv_obj_add_flag(objects.sysinfo_content_update, LV_OBJ_FLAG_HIDDEN);
    if (s_logs_panel)   lv_obj_add_flag(s_logs_panel,   LV_OBJ_FLAG_HIDDEN);
    if (s_dev_panel)    lv_obj_add_flag(s_dev_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_joy_panel)    lv_obj_add_flag(s_joy_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_val_panel)    lv_obj_add_flag(s_val_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_test_panel)   lv_obj_add_flag(s_test_panel,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_serial_panel, LV_OBJ_FLAG_HIDDEN);
    hmi_style_btn(objects.sysinfo_btn_device,  false);
    hmi_style_btn(objects.sysinfo_btn_version, false);
    hmi_style_btn(objects.sysinfo_btn_guide,   false);
    hmi_style_btn(s_serial_btn, true);
    if (s_logs_btn) hmi_style_btn(s_logs_btn, false);
    if (s_dev_btn)  hmi_style_btn(s_dev_btn,  false);
    if (s_joy_btn)  hmi_style_btn(s_joy_btn,  false);
    if (s_val_btn)  hmi_style_btn(s_val_btn,  false);
    if (s_test_btn) hmi_style_btn(s_test_btn, false);
}

// Cuando el usuario pulsa el boton Joystick en la barra de nav
static void joy_show_cb(lv_event_t *e)
{
    if (!s_joy_panel) return;
    lv_obj_add_flag(objects.sysinfo_content_device,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.sysinfo_content_version, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.sysinfo_content_guide,   LV_OBJ_FLAG_HIDDEN);
    if (objects.sysinfo_content_update) lv_obj_add_flag(objects.sysinfo_content_update, LV_OBJ_FLAG_HIDDEN);
    if (s_logs_panel)   lv_obj_add_flag(s_logs_panel,   LV_OBJ_FLAG_HIDDEN);
    if (s_dev_panel)    lv_obj_add_flag(s_dev_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_serial_panel) lv_obj_add_flag(s_serial_panel, LV_OBJ_FLAG_HIDDEN);
    if (s_val_panel)    lv_obj_add_flag(s_val_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_test_panel)   lv_obj_add_flag(s_test_panel,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_joy_panel, LV_OBJ_FLAG_HIDDEN);
    hmi_style_btn(objects.sysinfo_btn_device,  false);
    hmi_style_btn(objects.sysinfo_btn_version, false);
    hmi_style_btn(objects.sysinfo_btn_guide,   false);
    hmi_style_btn(s_joy_btn, true);
    if (s_logs_btn)   hmi_style_btn(s_logs_btn,   false);
    if (s_dev_btn)    hmi_style_btn(s_dev_btn,    false);
    if (s_serial_btn) hmi_style_btn(s_serial_btn, false);
    if (s_val_btn)    hmi_style_btn(s_val_btn,    false);
    if (s_test_btn)   hmi_style_btn(s_test_btn,   false);
}

// Cuando el usuario cambia a un tab normal: oculta paneles DEV, Serial y Joystick
static void dev_nav_close_cb(lv_event_t *e)
{
    if (s_logs_panel)   lv_obj_add_flag(s_logs_panel,   LV_OBJ_FLAG_HIDDEN);
    if (s_dev_panel)    lv_obj_add_flag(s_dev_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_serial_panel) lv_obj_add_flag(s_serial_panel, LV_OBJ_FLAG_HIDDEN);
    if (s_joy_panel)    lv_obj_add_flag(s_joy_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_val_panel)    lv_obj_add_flag(s_val_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_test_panel)   lv_obj_add_flag(s_test_panel,   LV_OBJ_FLAG_HIDDEN);
}

// Cuando el usuario pulsa el boton DEV en la barra de nav
static void dev_show_cb(lv_event_t *e)
{
    if (!s_dev_panel) return;
    lv_obj_add_flag(objects.sysinfo_content_device,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.sysinfo_content_version, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.sysinfo_content_guide,   LV_OBJ_FLAG_HIDDEN);
    if (objects.sysinfo_content_update) lv_obj_add_flag(objects.sysinfo_content_update, LV_OBJ_FLAG_HIDDEN);
    if (s_logs_panel)   lv_obj_add_flag(s_logs_panel,   LV_OBJ_FLAG_HIDDEN);
    if (s_serial_panel) lv_obj_add_flag(s_serial_panel, LV_OBJ_FLAG_HIDDEN);
    if (s_joy_panel)    lv_obj_add_flag(s_joy_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_val_panel)    lv_obj_add_flag(s_val_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_test_panel)   lv_obj_add_flag(s_test_panel,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_dev_panel, LV_OBJ_FLAG_HIDDEN);
    hmi_style_btn(objects.sysinfo_btn_device,  false);
    hmi_style_btn(objects.sysinfo_btn_version, false);
    hmi_style_btn(objects.sysinfo_btn_guide,   false);
    hmi_style_btn(s_dev_btn, true);
    if (s_logs_btn)   hmi_style_btn(s_logs_btn,   false);
    if (s_serial_btn) hmi_style_btn(s_serial_btn, false);
    if (s_joy_btn)    hmi_style_btn(s_joy_btn,    false);
    if (s_val_btn)    hmi_style_btn(s_val_btn,    false);
    if (s_test_btn)   hmi_style_btn(s_test_btn,   false);
}

static void val_show_cb(lv_event_t *e)
{
    if (!s_val_panel) return;
    lv_obj_add_flag(objects.sysinfo_content_device,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.sysinfo_content_version, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.sysinfo_content_guide,   LV_OBJ_FLAG_HIDDEN);
    if (objects.sysinfo_content_update) lv_obj_add_flag(objects.sysinfo_content_update, LV_OBJ_FLAG_HIDDEN);
    if (s_logs_panel)   lv_obj_add_flag(s_logs_panel,   LV_OBJ_FLAG_HIDDEN);
    if (s_dev_panel)    lv_obj_add_flag(s_dev_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_serial_panel) lv_obj_add_flag(s_serial_panel, LV_OBJ_FLAG_HIDDEN);
    if (s_joy_panel)    lv_obj_add_flag(s_joy_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_test_panel)   lv_obj_add_flag(s_test_panel,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_val_panel, LV_OBJ_FLAG_HIDDEN);
    hmi_style_btn(objects.sysinfo_btn_device,  false);
    hmi_style_btn(objects.sysinfo_btn_version, false);
    hmi_style_btn(objects.sysinfo_btn_guide,   false);
    hmi_style_btn(s_val_btn, true);
    if (s_logs_btn)   hmi_style_btn(s_logs_btn,   false);
    if (s_dev_btn)    hmi_style_btn(s_dev_btn,    false);
    if (s_serial_btn) hmi_style_btn(s_serial_btn, false);
    if (s_joy_btn)    hmi_style_btn(s_joy_btn,    false);
    if (s_test_btn)   hmi_style_btn(s_test_btn,   false);
}

static void val_refresh_timer_cb(lv_timer_t *t)
{
    if (!s_val_body) return;

    int j1x = s_joy_j1x, j1y = s_joy_j1y;
    int j2x = s_joy_j2x, j2y = s_joy_j2y;
    int p1  = s_p1_value;

    static const char *motor_names[] = {
        "MOTOR_STOP", "MOTOR_FORWARD", "MOTOR_REVERSE", "MOTOR_RIGHT", "MOTOR_LEFT"
    };
    int mcmd = s_motor_cmd;
    const char *mname = (mcmd >= 0 && mcmd <= 4) ? motor_names[mcmd] : "MOTOR_?";

    char buf[512];
    snprintf(buf, sizeof(buf),
        "--- JOY CONV ---\n"
        "  J1[%d,%d]     J2[%d,%d]     P1[%d]\n"
        "\n"
        "--- MOTOR ---\n"
        "  cmd=%d  vel=%d  motor=%s\n"
        "\n"
        "--- ANGULOS SERVO ---\n"
        "  SRV1(cabeza)=%d\xC2\xB0\n"
        "  SRV2(cuello)=%d\xC2\xB0\n"
        "  SRV3       =%d\xC2\xB0",
        j1x, j1y, j2x, j2y, p1,
        mcmd, s_motor_vel, mname,
        s_srv1_angle, s_srv2_angle, s_srv3_angle);

    lv_label_set_text(s_val_body, buf);
}

static void dev_cb_val_clear(lv_event_t *e)
{
    if (s_val_body) lv_label_set_text(s_val_body, "Limpiado");
}

static void val_panel_create(lv_obj_t *content_area, lv_obj_t *nav_col)
{
    if (s_val_panel) return;
    lv_obj_t *p = lv_obj_create(content_area);
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_size(p, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(p, 0, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_set_style_radius(p, 0, 0);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(p, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(p, 6, 0);
    lv_obj_add_flag(p, LV_OBJ_FLAG_HIDDEN);
    s_val_panel = p;

    lv_obj_t *title = lv_label_create(p);
    lv_label_set_text(title, " VALORES MONITOR");
    lv_obj_set_size(title, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, hmi_theme_accent(), 0);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_width(title, 4, 0);
    lv_obj_set_style_border_color(title, hmi_theme_accent(), 0);
    lv_obj_set_style_border_opa(title, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(title, 8, 0);

    dev_make_info_row(p, "Modo :", "JOY -> SERVO  (cada 100ms)");

    // Caja tipo terminal — igual que Serial Monitor
    lv_obj_t *tbox = lv_obj_create(p);
    lv_obj_set_width(tbox, LV_PCT(100));
    lv_obj_set_flex_grow(tbox, 1);
    lv_obj_set_style_bg_color(tbox, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(tbox, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tbox, 1, 0);
    lv_obj_set_style_border_color(tbox, hmi_theme_accent(), 0);
    lv_obj_set_style_border_opa(tbox, LV_OPA_50, 0);
    lv_obj_set_style_radius(tbox, 6, 0);
    lv_obj_set_style_pad_all(tbox, 6, 0);

    s_val_body = lv_label_create(tbox);
    lv_label_set_text(s_val_body, "Esperando datos...");
    lv_obj_set_width(s_val_body, LV_PCT(100));
    lv_obj_set_style_text_font(s_val_body, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_val_body, lv_color_hex(0x7EC8A0), 0);
    lv_label_set_long_mode(s_val_body, LV_LABEL_LONG_WRAP);

    s_val_timer = lv_timer_create(val_refresh_timer_cb, 100, NULL);

    dev_make_btn(p, "Limpiar valores", dev_cb_val_clear);

    // Boton Valores en columna de navegacion
    lv_obj_t *val_btn = lv_button_create(nav_col);
    s_val_btn = val_btn;
    lv_obj_set_size(val_btn, LV_PCT(100), 60);
    lv_obj_set_style_shadow_opa(val_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(val_btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *val_btn_lbl = lv_label_create(val_btn);
    lv_obj_set_style_align(val_btn_lbl, LV_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(val_btn_lbl, &lv_font_montserrat_18, 0);
    lv_label_set_text(val_btn_lbl, "Valores");
    hmi_style_btn(val_btn, false);
    lv_obj_add_event_cb(val_btn, val_show_cb, LV_EVENT_CLICKED, NULL);
}

static void panel_destroy(uint8_t bit)
{
    switch (bit) {
    case VIS_LOG:
        if (s_logs_panel) { lv_obj_del(s_logs_panel); s_logs_panel = NULL;
                            s_logs_tbox = NULL; objects.sysinfo_logs_body = NULL; }
        if (s_logs_btn)   { lv_obj_del(s_logs_btn);   s_logs_btn   = NULL; }
        break;
    case VIS_VAL:
        if (s_val_timer)  { lv_timer_delete(s_val_timer); s_val_timer = NULL; }
        if (s_val_panel)  { lv_obj_del(s_val_panel); s_val_panel = NULL; s_val_body = NULL; }
        if (s_val_btn)    { lv_obj_del(s_val_btn);   s_val_btn   = NULL; }
        break;
    case VIS_JOY:
        if (s_joy_timer)  { lv_timer_delete(s_joy_timer); s_joy_timer = NULL; }
        if (s_joy_panel)  { lv_obj_del(s_joy_panel); s_joy_panel = NULL; s_joy_body = NULL; }
        if (s_joy_btn)    { lv_obj_del(s_joy_btn);   s_joy_btn   = NULL; }
        break;
    case VIS_SER:
        if (s_serial_timer) { lv_timer_delete(s_serial_timer); s_serial_timer = NULL; }
        if (s_serial_panel) { lv_obj_del(s_serial_panel); s_serial_panel = NULL; s_serial_body = NULL; }
        if (s_serial_btn)   { lv_obj_del(s_serial_btn);   s_serial_btn   = NULL; }
        break;
    case VIS_TEST:
        if (s_test_panel) { lv_obj_del(s_test_panel); s_test_panel = NULL; s_test_status = NULL; s_test_vel_btn = NULL; }
        if (s_test_btn)   { lv_obj_del(s_test_btn);   s_test_btn   = NULL; }
        break;
    }
}

static void panel_create_one(uint8_t bit)
{
    lv_obj_t *content_area = lv_obj_get_parent(objects.sysinfo_content_device);
    lv_obj_t *nav_col      = lv_obj_get_parent(objects.sysinfo_btn_device);
    lv_obj_add_flag(nav_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(nav_col, LV_SCROLLBAR_MODE_AUTO);
    switch (bit) {
    case VIS_LOG:  logs_panel_create(content_area, nav_col);   break;
    case VIS_VAL:  val_panel_create(content_area, nav_col);    break;
    case VIS_JOY:  joy_panel_create(content_area, nav_col);    break;
    case VIS_SER:  serial_panel_create(content_area, nav_col); break;
    case VIS_TEST: test_panel_create(content_area, nav_col);   break;
    }
}

static void dev_toggle_panel_cb(lv_event_t *e)
{
    uint8_t bit = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    bool now_on = (s_vis_panels & bit) != 0;
    if (now_on) {
        s_vis_panels &= ~bit;
        panel_destroy(bit);
    } else {
        s_vis_panels |= bit;
        panel_create_one(bit);
    }
    vis_panels_nvs_write(s_vis_panels);
    now_on = !now_on;
    // Actualizar texto y estilo del boton
    const char *names[] = { "Logs", "Valores", "Joystick", "Serial", "Test" };
    int idx = 0;
    for (int i = 0; i < 5; i++) if ((1 << i) == bit) { idx = i; break; }
    char buf[24];
    snprintf(buf, sizeof(buf), "%s: %s", names[idx], now_on ? "ON" : "OFF");
    lv_label_set_text(lbl, buf);
    hmi_style_btn(btn, now_on);
}

static void panels_startup_init(void)
{
    s_vis_panels = vis_panels_nvs_read();
    if (!s_vis_panels) return;
    if (esp_lv_adapter_lock(-1) != ESP_OK) return;
    lv_obj_t *content_area = lv_obj_get_parent(objects.sysinfo_content_device);
    lv_obj_t *nav_col      = lv_obj_get_parent(objects.sysinfo_btn_device);
    lv_obj_add_flag(nav_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(nav_col, LV_SCROLLBAR_MODE_AUTO);
    if (s_vis_panels & VIS_LOG)  logs_panel_create(content_area, nav_col);
    if (s_vis_panels & VIS_VAL)  val_panel_create(content_area, nav_col);
    if (s_vis_panels & VIS_JOY)  joy_panel_create(content_area, nav_col);
    if (s_vis_panels & VIS_SER)  serial_panel_create(content_area, nav_col);
    if (s_vis_panels & VIS_TEST) test_panel_create(content_area, nav_col);
    esp_lv_adapter_unlock();
}

static void serial_panel_create(lv_obj_t *content_area, lv_obj_t *nav_col)
{
    if (s_serial_panel) return;
    lv_obj_t *p = lv_obj_create(content_area);
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_size(p, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(p, 0, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_set_style_radius(p, 0, 0);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(p, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(p, 6, 0);
    lv_obj_add_flag(p, LV_OBJ_FLAG_HIDDEN);
    s_serial_panel = p;

    lv_obj_t *title = lv_label_create(p);
    lv_label_set_text(title, " SERIAL MONITOR");
    lv_obj_set_size(title, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, hmi_theme_accent(), 0);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_width(title, 4, 0);
    lv_obj_set_style_border_color(title, hmi_theme_accent(), 0);
    lv_obj_set_style_border_opa(title, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(title, 8, 0);

    dev_make_info_row(p, "UART :", "115200  8N1");

    // Caja tipo terminal — ocupa el espacio restante
    lv_obj_t *tbox = lv_obj_create(p);
    lv_obj_set_width(tbox, LV_PCT(100));
    lv_obj_set_flex_grow(tbox, 1);
    lv_obj_set_style_bg_color(tbox, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(tbox, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tbox, 1, 0);
    lv_obj_set_style_border_color(tbox, hmi_theme_accent(), 0);
    lv_obj_set_style_border_opa(tbox, LV_OPA_50, 0);
    lv_obj_set_style_radius(tbox, 6, 0);
    lv_obj_set_style_pad_all(tbox, 6, 0);

    s_serial_body = lv_label_create(tbox);
    lv_label_set_text(s_serial_body, "Esperando datos...");
    lv_obj_set_width(s_serial_body, LV_PCT(100));
    lv_obj_set_style_text_font(s_serial_body, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_serial_body, lv_color_hex(0x7EC8A0), 0);
    lv_label_set_long_mode(s_serial_body, LV_LABEL_LONG_WRAP);

    s_serial_timer = lv_timer_create(serial_refresh_timer_cb, 100, NULL);

    dev_make_btn(p, "Limpiar serial", dev_cb_serial_clear);

    // Boton Serial en columna de navegacion
    lv_obj_t *serial_btn = lv_button_create(nav_col);
    s_serial_btn = serial_btn;
    lv_obj_set_size(serial_btn, LV_PCT(100), 60);
    lv_obj_set_style_shadow_opa(serial_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(serial_btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *serial_btn_lbl = lv_label_create(serial_btn);
    lv_obj_set_style_align(serial_btn_lbl, LV_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(serial_btn_lbl, &lv_font_montserrat_18, 0);
    lv_label_set_text(serial_btn_lbl, "Serial");
    hmi_style_btn(serial_btn, false);
    lv_obj_add_event_cb(serial_btn, serial_show_cb, LV_EVENT_CLICKED, NULL);
}

// ---- Robot Test (D-pad) — envia el comando de inmediato, igual que el LED ----
#define MOTOR_CMD_STOP     0
#define MOTOR_CMD_FORWARD  1
#define MOTOR_CMD_REVERSE  2
#define MOTOR_CMD_RIGHT    3
#define MOTOR_CMD_LEFT     4
static void test_send_motor(int32_t cmd, int32_t vel, const char *label)
{
    int32_t packed = (cmd << 16) | (vel & 0xFFFF);
    hmi_send_data(HMI_REG_MOTOR, packed);

    char logbuf[48];
    snprintf(logbuf, sizeof(logbuf), "MOTOR cmd=%d vel=%d (%s)", (int)cmd, (int)vel, label);
    hmi_log(LOG_TX, logbuf);
    ESP_LOGW(TAG, ">> %s", logbuf);
    dev_serial_add("TX", logbuf);

    if (s_test_status) {
        char buf[40];
        snprintf(buf, sizeof(buf), "CMD: %d MOTOR: %s", (int)cmd, label);
        lv_label_set_text(s_test_status, buf);
    }
}

static void test_cb_forward(lv_event_t *e) { (void)e; test_send_motor(MOTOR_CMD_FORWARD, s_test_vel, "ADELANTE");  }
static void test_cb_back(lv_event_t *e)    { (void)e; test_send_motor(MOTOR_CMD_REVERSE, s_test_vel, "ATRAS");     }
static void test_cb_left(lv_event_t *e)    { (void)e; test_send_motor(MOTOR_CMD_LEFT,    s_test_vel, "IZQUIERDA"); }
static void test_cb_right(lv_event_t *e)   { (void)e; test_send_motor(MOTOR_CMD_RIGHT,   s_test_vel, "DERECHA");   }
static void test_cb_stop(lv_event_t *e)    { (void)e; test_send_motor(MOTOR_CMD_STOP,    0,          "STOP");     }

// ---- Editor de velocidad de prueba (0-1000), teclado numerico ----
static lv_obj_t *s_tv_panel     = NULL;
static lv_obj_t *s_tv_display   = NULL;
static char      s_tv_buf[5];
static int       s_tv_len       = 0;

static void tv_update_vel_label(void)
{
    if (!s_test_vel_btn) return;
    lv_obj_t *lbl = lv_obj_get_child(s_test_vel_btn, 0);
    if (!lbl) return;
    char buf[32];
    snprintf(buf, sizeof(buf), "Velocidad: %d", (int)s_test_vel);
    lv_label_set_text(lbl, buf);
}

static void tv_close(void)
{
    if (s_tv_panel) { lv_obj_del(s_tv_panel); s_tv_panel = NULL; s_tv_display = NULL; }
    s_tv_len = 0; s_tv_buf[0] = '\0';
}

static void tv_update_display(void)
{
    if (!s_tv_display) return;
    lv_label_set_text(s_tv_display, s_tv_len > 0 ? s_tv_buf : "0");
}

static void tv_key_cb(lv_event_t *e)
{
    if (!s_tv_panel) return;
    intptr_t k = (intptr_t)lv_event_get_user_data(e);

    if (k == 'X') {
        if (s_tv_len > 0) { s_tv_len--; s_tv_buf[s_tv_len] = '\0'; }
        tv_update_display();
        return;
    }
    if (k == 'K') {
        int v = 0;
        for (int i = 0; i < s_tv_len; i++) v = v * 10 + (s_tv_buf[i] - '0');
        if (v > 1000) v = 1000;
        if (v < 0)    v = 0;
        s_test_vel = v;
        tv_update_vel_label();
        tv_close();
        return;
    }
    if (s_tv_len < 4) {
        s_tv_buf[s_tv_len++] = (char)k;
        s_tv_buf[s_tv_len]   = '\0';
    }
    tv_update_display();
}

static void tv_open(lv_event_t *e)
{
    (void)e;
    if (s_tv_panel) return;
    s_tv_len = 0; s_tv_buf[0] = '\0';

    lv_obj_t *p = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_size(p, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_70, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_radius(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    s_tv_panel = p;

    lv_obj_t *card = lv_obj_create(p);
    lv_obj_set_size(card, 310, LV_SIZE_CONTENT);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_color(card, hmi_theme_accent(), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 14, 0);
    lv_obj_set_style_pad_row(card, 10, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, "VELOCIDAD DE PRUEBA");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, hmi_theme_accent(), 0);
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

    s_tv_display = lv_label_create(card);
    lv_label_set_text(s_tv_display, "0");
    lv_obj_set_style_text_font(s_tv_display, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_tv_display, hmi_theme_txt_primary(), 0);
    lv_obj_set_style_text_align(s_tv_display, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_tv_display, LV_PCT(100));

    lv_obj_t *hint = lv_label_create(card);
    lv_label_set_text(hint, "0 a 1000");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hint, hmi_theme_txt_secondary(), 0);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(hint, LV_PCT(100));

    lv_obj_t *grid = lv_obj_create(card);
    lv_obj_set_width(grid, LV_PCT(100));
    lv_obj_set_height(grid, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(grid, 0, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_style_pad_gap(grid, 6, 0);
    lv_obj_remove_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    const char    *keys[] = {"1","2","3","4","5","6","7","8","9","<","0","OK"};
    const intptr_t ids[]  = {'1','2','3','4','5','6','7','8','9','X','0','K'};
    for (int i = 0; i < 12; i++) {
        lv_obj_t *btn = lv_button_create(grid);
        lv_obj_set_size(btn, 72, 52);
        lv_obj_add_event_cb(btn, tv_key_cb, LV_EVENT_CLICKED, (void *)ids[i]);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_shadow_opa(btn, 0, 0);
        if (ids[i] == 'K')
            lv_obj_set_style_bg_color(btn, hmi_theme_accent(), 0);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, keys[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
        lv_obj_center(lbl);
    }

    lv_obj_t *cancel = lv_button_create(card);
    lv_obj_set_width(cancel, LV_PCT(100));
    lv_obj_set_style_bg_color(cancel, lv_color_hex(0x333333), 0);
    lv_obj_set_style_shadow_opa(cancel, 0, 0);
    lv_obj_set_style_radius(cancel, 8, 0);
    lv_obj_add_event_cb(cancel, (lv_event_cb_t)tv_close, LV_EVENT_CLICKED, NULL);
    lv_obj_t *clbl = lv_label_create(cancel);
    lv_label_set_text(clbl, "Cancelar");
    lv_obj_set_style_text_font(clbl, &lv_font_montserrat_14, 0);
    lv_obj_center(clbl);
}

// Transicion suave de color + "hundido" al presionar, compartida por los 5 botones del D-pad
static void style_dpad_button(lv_obj_t *btn)
{
    static bool inited = false;
    static lv_style_t trans_style;
    if (!inited) {
        lv_style_init(&trans_style);
        static const lv_style_prop_t props[] = {
            LV_STYLE_BG_COLOR, LV_STYLE_TRANSFORM_WIDTH, LV_STYLE_TRANSFORM_HEIGHT, 0
        };
        static lv_style_transition_dsc_t trans;
        lv_style_transition_dsc_init(&trans, props, lv_anim_path_ease_out, 140, 0, NULL);
        lv_style_set_transition(&trans_style, &trans);
        inited = true;
    }
    lv_obj_add_style(btn, &trans_style, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_transform_width(btn, -6, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_transform_height(btn, -6, LV_PART_MAIN | LV_STATE_PRESSED);
}

// Cuando el usuario pulsa el boton Test en la barra de nav
static void test_show_cb(lv_event_t *e)
{
    (void)e;
    if (!s_test_panel) return;
    lv_obj_add_flag(objects.sysinfo_content_device,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.sysinfo_content_version, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.sysinfo_content_guide,   LV_OBJ_FLAG_HIDDEN);
    if (objects.sysinfo_content_update) lv_obj_add_flag(objects.sysinfo_content_update, LV_OBJ_FLAG_HIDDEN);
    if (s_logs_panel)   lv_obj_add_flag(s_logs_panel,   LV_OBJ_FLAG_HIDDEN);
    if (s_dev_panel)    lv_obj_add_flag(s_dev_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_serial_panel) lv_obj_add_flag(s_serial_panel, LV_OBJ_FLAG_HIDDEN);
    if (s_joy_panel)    lv_obj_add_flag(s_joy_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_val_panel)    lv_obj_add_flag(s_val_panel,    LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_test_panel, LV_OBJ_FLAG_HIDDEN);
    hmi_style_btn(objects.sysinfo_btn_device,  false);
    hmi_style_btn(objects.sysinfo_btn_version, false);
    hmi_style_btn(objects.sysinfo_btn_guide,   false);
    hmi_style_btn(s_test_btn, true);
    if (s_logs_btn)   hmi_style_btn(s_logs_btn,   false);
    if (s_dev_btn)    hmi_style_btn(s_dev_btn,    false);
    if (s_serial_btn) hmi_style_btn(s_serial_btn, false);
    if (s_joy_btn)    hmi_style_btn(s_joy_btn,    false);
    if (s_val_btn)    hmi_style_btn(s_val_btn,    false);
}

static void test_panel_create(lv_obj_t *content_area, lv_obj_t *nav_col)
{
    if (s_test_panel) return;
    lv_obj_t *p = lv_obj_create(content_area);
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_size(p, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(p, 0, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_set_style_radius(p, 0, 0);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(p, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(p, 10, 0);
    lv_obj_add_flag(p, LV_OBJ_FLAG_HIDDEN);
    s_test_panel = p;

    lv_obj_t *title = lv_label_create(p);
    lv_label_set_text(title, " ROBOT TEST");
    lv_obj_set_size(title, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, hmi_theme_accent(), 0);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_width(title, 4, 0);
    lv_obj_set_style_border_color(title, hmi_theme_accent(), 0);
    lv_obj_set_style_border_opa(title, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(title, 8, 0);

    lv_obj_t *status = lv_label_create(p);
    s_test_status = status;
    lv_obj_set_size(status, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(status, hmi_theme_txt_secondary(), 0);
    lv_obj_set_style_text_font(status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(status, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(status, LV_LABEL_LONG_WRAP);
    lv_label_set_text(status, "CMD: -- MOTOR: ---");

    // Contenedor centrador: cruceta + boton de velocidad, uno al lado del otro
    lv_obj_t *wrap = lv_obj_create(p);
    lv_obj_set_flex_grow(wrap, 1);
    lv_obj_set_size(wrap, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(wrap, 0, 0);
    lv_obj_set_style_border_width(wrap, 0, 0);
    lv_obj_set_style_pad_all(wrap, 0, 0);
    lv_obj_set_style_pad_column(wrap, 24, 0);
    lv_obj_set_flex_flow(wrap, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wrap, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *grid = lv_obj_create(wrap);
    lv_obj_set_style_bg_opa(grid, 0, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    static const int32_t dpad_col_dsc[] = { 120, 120, 120, LV_GRID_TEMPLATE_LAST };
    static const int32_t dpad_row_dsc[] = { 80, 80, 80, LV_GRID_TEMPLATE_LAST };
    lv_obj_set_grid_dsc_array(grid, dpad_col_dsc, dpad_row_dsc);
    lv_obj_set_size(grid, 3 * 120 + 2 * 14, 3 * 80 + 2 * 14);
    lv_obj_set_style_pad_column(grid, 14, 0);
    lv_obj_set_style_pad_row(grid, 14, 0);

    struct { const char *label; int col; int row; lv_event_cb_t cb; } dpad_defs[5] = {
        { "^",    1, 0, test_cb_forward },
        { "<",    0, 1, test_cb_left    },
        { "STOP", 1, 1, test_cb_stop    },
        { ">",    2, 1, test_cb_right   },
        { "v",    1, 2, test_cb_back    },
    };
    for (int i = 0; i < 5; i++) {
        lv_obj_t *btn = lv_button_create(grid);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, dpad_defs[i].col, 1,
                                   LV_GRID_ALIGN_STRETCH, dpad_defs[i].row, 1);
        lv_obj_set_style_bg_color(btn,     hmi_theme_bg_btn_inactive(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(btn, hmi_theme_bd_btn_inactive(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(btn, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btn,     hmi_theme_bg_btn_active(), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn, hmi_theme_bd_btn_active(), LV_PART_MAIN | LV_STATE_PRESSED);
        style_dpad_button(btn);
        lv_obj_add_event_cb(btn, dpad_defs[i].cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lbl, hmi_theme_txt_btn_inactive(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lbl, hmi_theme_txt_btn_active(),   LV_PART_MAIN | LV_STATE_PRESSED);
        lv_label_set_text(lbl, dpad_defs[i].label);
        lv_obj_center(lbl);
    }

    // Boton "Velocidad: N" — al lado de la cruceta, abre el teclado numerico (0-1000)
    lv_obj_t *velbtn = lv_button_create(wrap);
    s_test_vel_btn = velbtn;
    lv_obj_set_size(velbtn, 160, 70);
    lv_obj_set_style_radius(velbtn, 12, 0);
    lv_obj_set_style_shadow_opa(velbtn, 0, 0);
    lv_obj_add_event_cb(velbtn, tv_open, LV_EVENT_CLICKED, NULL);
    hmi_style_btn(velbtn, false);
    lv_obj_t *vellbl = lv_label_create(velbtn);
    lv_obj_set_style_text_font(vellbl, &lv_font_montserrat_16, 0);
    lv_obj_center(vellbl);
    tv_update_vel_label();

    // Boton Test en columna de navegacion
    lv_obj_t *test_btn = lv_button_create(nav_col);
    s_test_btn = test_btn;
    lv_obj_set_size(test_btn, LV_PCT(100), 60);
    lv_obj_set_style_shadow_opa(test_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(test_btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *test_btn_lbl = lv_label_create(test_btn);
    lv_obj_set_style_align(test_btn_lbl, LV_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(test_btn_lbl, &lv_font_montserrat_18, 0);
    lv_label_set_text(test_btn_lbl, "Test");
    hmi_style_btn(test_btn, false);
    lv_obj_add_event_cb(test_btn, test_show_cb, LV_EVENT_CLICKED, NULL);
}

static void joy_panel_create(lv_obj_t *content_area, lv_obj_t *nav_col)
{
    if (s_joy_panel) return;
    lv_obj_t *p = lv_obj_create(content_area);
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_size(p, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(p, 0, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_set_style_radius(p, 0, 0);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(p, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(p, 6, 0);
    lv_obj_add_flag(p, LV_OBJ_FLAG_HIDDEN);
    s_joy_panel = p;

    lv_obj_t *title = lv_label_create(p);
    lv_label_set_text(title, " JOYSTICK MONITOR");
    lv_obj_set_size(title, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, hmi_theme_accent(), 0);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_width(title, 4, 0);
    lv_obj_set_style_border_color(title, hmi_theme_accent(), 0);
    lv_obj_set_style_border_opa(title, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(title, 8, 0);

    dev_make_info_row(p, "Tramas :", "JOY1  JOY2  BTN");

    lv_obj_t *tbox = lv_obj_create(p);
    lv_obj_set_width(tbox, LV_PCT(100));
    lv_obj_set_flex_grow(tbox, 1);
    lv_obj_set_style_bg_color(tbox, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(tbox, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tbox, 1, 0);
    lv_obj_set_style_border_color(tbox, hmi_theme_accent(), 0);
    lv_obj_set_style_border_opa(tbox, LV_OPA_50, 0);
    lv_obj_set_style_radius(tbox, 6, 0);
    lv_obj_set_style_pad_all(tbox, 6, 0);

    s_joy_body = lv_label_create(tbox);
    lv_label_set_text(s_joy_body, "Esperando datos joystick...");
    lv_obj_set_width(s_joy_body, LV_PCT(100));
    lv_obj_set_style_text_font(s_joy_body, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_joy_body, lv_color_hex(0x7EC8A0), 0);
    lv_label_set_long_mode(s_joy_body, LV_LABEL_LONG_WRAP);

    s_joy_timer = lv_timer_create(joy_refresh_timer_cb, 100, NULL);

    dev_make_btn(p, "Limpiar joystick", dev_cb_joy_clear);

    lv_obj_t *joy_btn = lv_button_create(nav_col);
    s_joy_btn = joy_btn;
    lv_obj_set_size(joy_btn, LV_PCT(100), 60);
    lv_obj_set_style_shadow_opa(joy_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(joy_btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *joy_btn_lbl = lv_label_create(joy_btn);
    lv_obj_set_style_align(joy_btn_lbl, LV_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(joy_btn_lbl, &lv_font_montserrat_18, 0);
    lv_label_set_text(joy_btn_lbl, "Joystick");
    hmi_style_btn(joy_btn, false);
    lv_obj_add_event_cb(joy_btn, joy_show_cb, LV_EVENT_CLICKED, NULL);
}

static void dev_panel_create(void)
{
    if (s_dev_panel) return;

    int cur_tx = dev_uart_tx_pin();
    int cur_rx = dev_uart_rx_pin();

    // Panel DEV como hijo del mismo contenedor que los otros panels de sysinfo
    lv_obj_t *content_area = lv_obj_get_parent(objects.sysinfo_content_device);
    lv_obj_t *p = lv_obj_create(content_area);
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_size(p, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(p, 0, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_set_style_radius(p, 0, 0);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(p, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(p, 6, 0);
    s_dev_panel = p;

    // Ocultar todos los panels existentes
    lv_obj_add_flag(objects.sysinfo_content_device,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.sysinfo_content_version, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.sysinfo_content_guide,   LV_OBJ_FLAG_HIDDEN);
    if (objects.sysinfo_content_update) lv_obj_add_flag(objects.sysinfo_content_update, LV_OBJ_FLAG_HIDDEN);

    // Habilitar scroll en la columna de nav para los botones extra DEV/Serial/Joystick
    lv_obj_t *nav_col = lv_obj_get_parent(objects.sysinfo_btn_device);
    lv_obj_add_flag(nav_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(nav_col, LV_SCROLLBAR_MODE_AUTO);

    // Logs es el primer boton DEV (posicion 4)
    logs_panel_create(content_area, nav_col);

    lv_obj_t *dev_btn = lv_button_create(nav_col);
    s_dev_btn = dev_btn;
    lv_obj_set_size(dev_btn, LV_PCT(100), 60);
    lv_obj_set_style_shadow_opa(dev_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(dev_btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *dev_btn_lbl = lv_label_create(dev_btn);
    lv_obj_set_style_align(dev_btn_lbl, LV_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(dev_btn_lbl, &lv_font_montserrat_18, 0);
    lv_label_set_text(dev_btn_lbl, "DEV");
    hmi_style_btn(dev_btn, true);   // estilo activo del tema actual
    // Desactivar botones fijos para que solo DEV quede resaltado
    hmi_style_btn(objects.sysinfo_btn_device,  false);
    hmi_style_btn(objects.sysinfo_btn_version, false);
    hmi_style_btn(objects.sysinfo_btn_guide,   false);
    lv_obj_add_event_cb(dev_btn, dev_show_cb, LV_EVENT_CLICKED, NULL);

    // Titulo — igual que los otros paneles: font_20 + color acento + borde izquierdo
    lv_obj_t *title = lv_label_create(p);
    lv_label_set_text(title, " MODO DESARROLLADOR");
    lv_obj_set_size(title, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, hmi_theme_accent(), 0);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_width(title, 4, 0);
    lv_obj_set_style_border_color(title, hmi_theme_accent(), 0);
    lv_obj_set_style_border_opa(title, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(title, 8, 0);

    // Info rows — identicas a create_info_row de screens.c
    char tx_str[12], rx_str[12];
    snprintf(tx_str, sizeof(tx_str), "GPIO %d", cur_tx);
    snprintf(rx_str, sizeof(rx_str), "GPIO %d", cur_rx);
    dev_make_info_row(p, "UART TX :", tx_str);
    dev_make_info_row(p, "UART RX :", rx_str);

    dev_make_sep(p, "Pines UART — guarda y reinicia");
    lv_obj_t *bj5  = dev_make_btn(p, "GPIO 26/27   Conector J5 (MX 1.25)", dev_cb_uart_j5);
    lv_obj_t *bhdr = dev_make_btn(p, "GPIO 51/52   Pin Header",             dev_cb_uart_header);
    // Resaltar el pin activo con el estilo activo del tema
    hmi_style_btn(cur_tx == 26 ? bj5 : bhdr, true);

    dev_make_sep(p, "Herramientas");
#ifdef TEST_UART_RX_DISPLAY
    dev_make_btn(p, s_rx_log_enabled ? "UART LOG: ON" : "UART LOG: OFF", dev_cb_toggle_log);
#endif
    dev_make_btn(p, "Editar N\xC2\xB0 Serie",  dev_cb_edit_serial);
    dev_make_btn(p, "Editar Nombre Dispositivo", dev_cb_edit_device_name);
    dev_make_btn(p, "Cambiar PIN",         dev_cb_change_pin);
    dev_make_btn(p, "Resetear PIN a 1234", dev_cb_reset_pin);
    dev_make_btn(p, "Resetear Boot Count", dev_cb_reset_boot_count);

    s_dev_heap_label = lv_label_create(p);
    char hbuf[36];
    snprintf(hbuf, sizeof(hbuf), "Heap libre: %lu B", (unsigned long)esp_get_free_heap_size());
    lv_label_set_text(s_dev_heap_label, hbuf);
    lv_obj_set_style_text_color(s_dev_heap_label, hmi_theme_txt_secondary(), 0);
    lv_obj_set_style_text_font(s_dev_heap_label, &lv_font_montserrat_16, 0);
    lv_obj_set_width(s_dev_heap_label, LV_PCT(100));
    s_dev_heap_timer = lv_timer_create(dev_heap_timer_cb, 2000, NULL);

    // Seccion toggles de visibilidad permanente
    dev_make_sep(p, "Paneles visibles");
    const char *pnames[] = { "Logs", "Valores", "Joystick", "Serial", "Test" };
    uint8_t pbits[]      = { VIS_LOG, VIS_VAL, VIS_JOY, VIS_SER, VIS_TEST };
    for (int i = 0; i < 5; i++) {
        bool on = (s_vis_panels & pbits[i]) != 0;
        char lbl_txt[24];
        snprintf(lbl_txt, sizeof(lbl_txt), "%s: %s", pnames[i], on ? "ON" : "OFF");
        lv_obj_t *tbtn = lv_button_create(p);
        lv_obj_set_size(tbtn, LV_PCT(100), 44);
        lv_obj_set_style_shadow_opa(tbtn, 0, 0);
        lv_obj_set_style_radius(tbtn, 8, 0);
        lv_obj_t *tl = lv_label_create(tbtn);
        lv_label_set_text(tl, lbl_txt);
        lv_obj_set_style_text_font(tl, &lv_font_montserrat_16, 0);
        lv_obj_center(tl);
        hmi_style_btn(tbtn, on);
        lv_obj_add_event_cb(tbtn, dev_toggle_panel_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)pbits[i]);
    }

    dev_make_sep(p, "");
    lv_obj_t *bcerrar = dev_make_btn(p, "Cerrar modo desarrollador", dev_cb_close);
    hmi_style_btn(bcerrar, true);
    dev_make_btn(p, "Reiniciar", dev_cb_restart);

    // Crear paneles Valores, Joystick, Serial y Test (hermanos de este panel, botones en nav_col)
    val_panel_create(content_area, nav_col);
    joy_panel_create(content_area, nav_col);
    serial_panel_create(content_area, nav_col);
    test_panel_create(content_area, nav_col);

    ESP_LOGW(TAG, "[DEV] Panel abierto, UART TX=%d RX=%d", cur_tx, cur_rx);
}

static void dot_timer_cb(lv_timer_t *t)
{
    if (s_dot_label) { lv_obj_del(s_dot_label); s_dot_label = NULL; }
    lv_timer_delete(t);
    s_dot_timer = NULL;
}

static void dot_show(int count)
{
    static const char *texts[] = { "", ".", "..", "..." };
    if (!s_dot_label) {
        s_dot_label = lv_label_create(lv_layer_top());
        lv_obj_set_style_text_font(s_dot_label, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(s_dot_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_bg_color(s_dot_label, lv_color_hex(0x222222), 0);
        lv_obj_set_style_bg_opa(s_dot_label, LV_OPA_70, 0);
        lv_obj_set_style_pad_all(s_dot_label, 4, 0);
        lv_obj_set_style_radius(s_dot_label, 6, 0);
        lv_obj_align(s_dot_label, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    }
    lv_label_set_text(s_dot_label, count <= 3 ? texts[count] : "...");
    lv_obj_align(s_dot_label, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

    if (s_dot_timer) lv_timer_reset(s_dot_timer);
    else             s_dot_timer = lv_timer_create(dot_timer_cb, 1500, NULL);
}

static void dot_hide(void)
{
    if (s_dot_timer) { lv_timer_delete(s_dot_timer); s_dot_timer = NULL; }
    if (s_dot_label) { lv_obj_del(s_dot_label);      s_dot_label = NULL; }
}

static void dev_tap_event_cb(lv_event_t *e)
{
    static int      taps = 0;
    static uint32_t t0   = 0;
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);

    if (taps > 0 && (now - t0) > DEV_UNLOCK_MS) taps = 0;
    if (taps == 0) t0 = now;
    ++taps;
    dot_show(taps);

    if (taps >= DEV_UNLOCK_TAPS) {
        taps = 0;
        dot_hide();
        if (s_dev_panel) dev_panel_close();
        else             pin_open(false);
    }
}

static void dev_mode_init(void)
{
    dev_nvs_read_serial(s_serial_num, sizeof(s_serial_num));
    if (esp_lv_adapter_lock(-1) != ESP_OK) return;
    if (objects.sysinfo_device_name_value || objects.settings_user_name_value) {
        char dn_buf[DEV_NAME_MAX];
        dev_nvs_read_device_name(dn_buf, sizeof(dn_buf));
        if (objects.sysinfo_device_name_value) lv_label_set_text(objects.sysinfo_device_name_value, dn_buf);
        if (objects.settings_user_name_value)  lv_label_set_text(objects.settings_user_name_value, dn_buf);
    }
    if (objects.settings_user_pin_switch) {
        /* Switch marcado = "habilitar contrasena" -> refleja hmi_get_lock_enabled() directo */
        if (hmi_get_lock_enabled())
            lv_obj_add_state(objects.settings_user_pin_switch, LV_STATE_CHECKED);
        else
            lv_obj_remove_state(objects.settings_user_pin_switch, LV_STATE_CHECKED);
    }
    if (objects.sysinfo_content_version) {
        lv_obj_add_flag(objects.sysinfo_content_version, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(objects.sysinfo_content_version,
                            dev_tap_event_cb, LV_EVENT_CLICKED, NULL);
    }
    // Cuando el usuario cambia a un tab normal, el panel DEV se cierra solo
    lv_obj_add_event_cb(objects.sysinfo_btn_device,  dev_nav_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.sysinfo_btn_version, dev_nav_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.sysinfo_btn_guide,   dev_nav_close_cb, LV_EVENT_CLICKED, NULL);
    esp_lv_adapter_unlock();
    ESP_LOGW(TAG, "*** DEV_MODE: toca 3x el panel VERSION en SysInfo para activar ***");
}

// Llamado desde hmi_theme_apply en actions.c cuando el usuario cambia de tema
void hmi_dev_retheme(void)
{
    if (s_dev_panel || s_serial_panel || s_joy_panel || s_test_panel) {
        dev_panel_close();
        dev_panel_create();
    }
}

// =====================================================================
// LOCK SCREEN — pantalla de bloqueo al encender
// =====================================================================
#ifdef LOCK_SCREEN_ENABLE
#define NVS_KEY_LOCK_PIN  "lock_pin"
#define LOCK_PIN_DEF      "0000"   // PIN de usuario por defecto (independiente del PIN DEV)

// Paleta identica al tema "Clasico" de la app (negro + dorado)
#define LS_COL_BG1        0x000000  // bg_screen
#define LS_COL_BG2        0x141414  // bg_nav (degradado sutil arriba)
#define LS_COL_CARD       0x1A1A1A  // bg_card
#define LS_COL_BORDER     0x252525  // bd_card
#define LS_COL_TXT        0xFFFFFF  // txt_primary
#define LS_COL_TXT_DIM    0xAAAAAA  // txt_secondary
#define LS_COL_ACCENT     0xF5C518  // txt_accent / bg_btn_active (dorado)
#define LS_COL_ACCENT_TXT 0x1A1A1A  // txt_btn_active (texto oscuro sobre dorado)
#define LS_COL_BTN_BG     0x2A2A2A  // bg_btn_inactive
#define LS_COL_BTN_BD     0x3C3C3C  // bd_btn_inactive
#define LS_COL_OK         0x27AE60  // clr_bar (verde exito)
#define LS_COL_ERR        0xE74C3C  // rojo critico (igual que alerta de bateria)

static void ls_nvs_read_pin(char *out, size_t max)
{
    strncpy(out, LOCK_PIN_DEF, max); out[max - 1] = '\0';
    nvs_handle_t h;
    if (nvs_open(NVS_DEV_NS, NVS_READONLY, &h) == ESP_OK) {
        size_t sz = max;
        nvs_get_str(h, NVS_KEY_LOCK_PIN, out, &sz);
        nvs_close(h);
    }
}

static void ls_nvs_write_pin(const char *pin)
{
    nvs_handle_t h;
    if (nvs_open(NVS_DEV_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, NVS_KEY_LOCK_PIN, pin);
        nvs_commit(h);
        nvs_close(h);
    }
}

#define NVS_KEY_LOCK_ENABLED "lock_en"

static bool ls_lock_enabled_read(void)
{
    uint8_t v = 1;
    nvs_handle_t h;
    if (nvs_open(NVS_DEV_NS, NVS_READONLY, &h) == ESP_OK) {
        nvs_get_u8(h, NVS_KEY_LOCK_ENABLED, &v);
        nvs_close(h);
    }
    return v != 0;
}

static void ls_lock_enabled_write(bool en)
{
    nvs_handle_t h;
    if (nvs_open(NVS_DEV_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY_LOCK_ENABLED, en ? 1 : 0);
        nvs_commit(h);
        nvs_close(h);
    }
}

// ---- Editor de PIN de bloqueo (independiente del PIN de DEV) ----
static lv_obj_t *s_lp_panel   = NULL;
static lv_obj_t *s_lp_display = NULL;
static char      s_lp_buf[DEV_PIN_LEN + 1];
static int       s_lp_len     = 0;

static void lp_close(void)
{
    if (s_lp_panel) { lv_obj_del(s_lp_panel); s_lp_panel = NULL; s_lp_display = NULL; }
    s_lp_len = 0; s_lp_buf[0] = '\0';
}

static void lp_update_display(void)
{
    if (!s_lp_display) return;
    char disp[DEV_PIN_LEN * 2 + 1] = "";
    for (int i = 0; i < DEV_PIN_LEN; i++) {
        if (i > 0) strcat(disp, " ");
        strcat(disp, i < s_lp_len ? "*" : "_");
    }
    lv_label_set_text(s_lp_display, disp);
}

static void lp_key_cb(lv_event_t *e)
{
    if (!s_lp_panel) return;
    intptr_t k = (intptr_t)lv_event_get_user_data(e);

    if (k == 'X') {
        if (s_lp_len > 0) { s_lp_len--; s_lp_buf[s_lp_len] = '\0'; }
    } else if (k == 'K') {
        if (s_lp_len != DEV_PIN_LEN) return;
        ls_nvs_write_pin(s_lp_buf);
        hmi_log(LOG_OK, "PIN de bloqueo actualizado");
        lp_close();
        return;
    } else if (s_lp_len < DEV_PIN_LEN) {
        s_lp_buf[s_lp_len++] = (char)k;
        s_lp_buf[s_lp_len]   = '\0';
    }
    lp_update_display();
}

static void lp_open(void)
{
    if (s_lp_panel) return;
    s_lp_len = 0; s_lp_buf[0] = '\0';

    lv_obj_t *p = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_size(p, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_70, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_radius(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    s_lp_panel = p;

    lv_obj_t *card = lv_obj_create(p);
    lv_obj_set_size(card, 310, LV_SIZE_CONTENT);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_color(card, lv_color_hex(LS_COL_ACCENT), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 14, 0);
    lv_obj_set_style_pad_row(card, 10, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, g_lang->title_new_lock_pin);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(LS_COL_ACCENT), 0);
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

    s_lp_display = lv_label_create(card);
    lv_label_set_text(s_lp_display, "_ _ _ _");
    lv_obj_set_style_text_font(s_lp_display, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_lp_display, lv_color_hex(LS_COL_TXT), 0);
    lv_obj_set_style_text_align(s_lp_display, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_lp_display, LV_PCT(100));

    lv_obj_t *hint = lv_label_create(card);
    lv_label_set_text(hint, g_lang->sub_lock_pin_digits);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(LS_COL_TXT_DIM), 0);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(hint, LV_PCT(100));

    lv_obj_t *grid = lv_obj_create(card);
    lv_obj_set_width(grid, LV_PCT(100));
    lv_obj_set_height(grid, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(grid, 0, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_style_pad_gap(grid, 6, 0);
    lv_obj_remove_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    const char    *keys[] = {"1","2","3","4","5","6","7","8","9","<","0","OK"};
    const intptr_t ids[]  = {'1','2','3','4','5','6','7','8','9','X','0','K'};
    for (int i = 0; i < 12; i++) {
        lv_obj_t *btn = lv_button_create(grid);
        lv_obj_set_size(btn, 72, 52);
        lv_obj_add_event_cb(btn, lp_key_cb, LV_EVENT_CLICKED, (void *)ids[i]);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_shadow_opa(btn, 0, 0);
        if (ids[i] == 'K')
            lv_obj_set_style_bg_color(btn, lv_color_hex(LS_COL_ACCENT), 0);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, keys[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
        lv_obj_center(lbl);
    }

    lv_obj_t *cancel = lv_button_create(card);
    lv_obj_set_width(cancel, LV_PCT(100));
    lv_obj_set_style_bg_color(cancel, lv_color_hex(0x333333), 0);
    lv_obj_set_style_shadow_opa(cancel, 0, 0);
    lv_obj_set_style_radius(cancel, 8, 0);
    lv_obj_add_event_cb(cancel, (lv_event_cb_t)lp_close, LV_EVENT_CLICKED, NULL);
    lv_obj_t *clbl = lv_label_create(cancel);
    lv_label_set_text(clbl, g_lang->btn_cancel);
    lv_obj_set_style_text_font(clbl, &lv_font_montserrat_14, 0);
    lv_obj_center(clbl);
}

static void ls_update_dots(void)
{
    for (int i = 0; i < 4; i++) {
        if (!s_ls_dots[i]) continue;
        if (i < s_ls_len) {
            lv_obj_set_style_bg_color(s_ls_dots[i],   lv_color_hex(LS_COL_TXT), 0);
            lv_obj_set_style_bg_opa(s_ls_dots[i],     LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(s_ls_dots[i], lv_color_hex(LS_COL_TXT), 0);
        } else {
            lv_obj_set_style_bg_opa(s_ls_dots[i],     LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_color(s_ls_dots[i], lv_color_hex(LS_COL_BTN_BD), 0);
        }
    }
}

static void ls_dots_color(lv_color_t col)
{
    for (int i = 0; i < 4; i++) {
        if (!s_ls_dots[i]) continue;
        lv_obj_set_style_bg_color(s_ls_dots[i],   col, 0);
        lv_obj_set_style_bg_opa(s_ls_dots[i],     LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(s_ls_dots[i], col, 0);
    }
}

static void ls_anim_x_cb(void *obj, int32_t v)
{
    lv_obj_set_style_translate_x((lv_obj_t *)obj, v, 0);
}

static void ls_reveal_x_cb(void *obj, int32_t v)
{
    lv_obj_set_style_translate_x((lv_obj_t *)obj, v, 0);
}

static void ls_reveal_opa_cb(void *obj, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)v, 0);
}

static void ls_set_card_state(int state) // 0=neutro 1=ok 2=error
{
    if (!s_ls_card) return;
    lv_color_t bd;
    switch (state) {
        case 1:  bd = lv_color_hex(LS_COL_OK);  break;
        case 2:  bd = lv_color_hex(LS_COL_ERR); break;
        default: bd = lv_color_hex(LS_COL_BORDER); break;
    }
    lv_obj_set_style_border_color(s_ls_card, bd, 0);
}

static void ls_set_status(const char *msg, lv_color_t col)
{
    if (!s_ls_status_lbl) return;
    lv_label_set_text(s_ls_status_lbl, msg);
    lv_obj_set_style_text_color(s_ls_status_lbl, col, 0);
}

static void ls_unlock_timer_cb(lv_timer_t *t)
{
    lv_timer_delete(t);
    xSemaphoreGive(s_ls_sem);
}

static void ls_err_timer_cb(lv_timer_t *t)
{
    lv_timer_delete(t);
    if (s_ls_card) lv_obj_set_style_translate_x(s_ls_card, 0, 0);
    ls_set_card_state(0);
    ls_set_status("", lv_color_hex(LS_COL_TXT_DIM));
    s_ls_len = 0; s_ls_buf[0] = '\0';
    ls_update_dots();
    s_ls_busy = false;
}

// Revela el teclado numerico deslizandolo junto al panel (como el HTML de referencia)
static void ls_reveal_keypad(void)
{
    if (!s_ls_keypad_wrap || !lv_obj_has_flag(s_ls_keypad_wrap, LV_OBJ_FLAG_HIDDEN)) return;

    lv_obj_remove_flag(s_ls_keypad_wrap, LV_OBJ_FLAG_HIDDEN);
    if (s_ls_hint_lbl) lv_obj_add_flag(s_ls_hint_lbl, LV_OBJ_FLAG_HIDDEN);
    if (s_ls_dotcon)   lv_obj_remove_flag(s_ls_dotcon, LV_OBJ_FLAG_HIDDEN);
    if (s_ls_overlay)  lv_obj_remove_flag(s_ls_overlay, LV_OBJ_FLAG_HIDDEN);
    if (s_ls_field) {
        lv_obj_set_style_border_color(s_ls_field, lv_color_hex(LS_COL_ACCENT), 0);
        lv_obj_set_style_border_opa(s_ls_field, LV_OPA_COVER, 0);
    }

    lv_obj_set_style_opa(s_ls_keypad_wrap, LV_OPA_TRANSP, 0);
    lv_obj_set_style_translate_x(s_ls_keypad_wrap, 24, 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_ls_keypad_wrap);
    lv_anim_set_exec_cb(&a, ls_reveal_x_cb);
    lv_anim_set_values(&a, 24, 0);
    lv_anim_set_time(&a, 260);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);

    lv_anim_t a2;
    lv_anim_init(&a2);
    lv_anim_set_var(&a2, s_ls_keypad_wrap);
    lv_anim_set_exec_cb(&a2, ls_reveal_opa_cb);
    lv_anim_set_values(&a2, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a2, 260);
    lv_anim_start(&a2);
}

static void ls_hide_keypad_anim_done_cb(lv_anim_t *a)
{
    (void)a;
    if (s_ls_keypad_wrap) lv_obj_add_flag(s_ls_keypad_wrap, LV_OBJ_FLAG_HIDDEN);
    if (s_ls_dotcon)      lv_obj_add_flag(s_ls_dotcon, LV_OBJ_FLAG_HIDDEN);
    if (s_ls_hint_lbl)    lv_obj_remove_flag(s_ls_hint_lbl, LV_OBJ_FLAG_HIDDEN);
    if (s_ls_overlay)     lv_obj_add_flag(s_ls_overlay, LV_OBJ_FLAG_HIDDEN);
    if (s_ls_field) {
        lv_obj_set_style_border_color(s_ls_field, lv_color_hex(LS_COL_BORDER), 0);
        lv_obj_set_style_border_opa(s_ls_field, LV_OPA_COVER, 0);
    }
    s_ls_len = 0; s_ls_buf[0] = '\0';
    ls_update_dots();
}

static void ls_hide_keypad(void)
{
    if (!s_ls_keypad_wrap || lv_obj_has_flag(s_ls_keypad_wrap, LV_OBJ_FLAG_HIDDEN)) return;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_ls_keypad_wrap);
    lv_anim_set_exec_cb(&a, ls_reveal_x_cb);
    lv_anim_set_values(&a, 0, 24);
    lv_anim_set_time(&a, 220);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_start(&a);

    lv_anim_t a2;
    lv_anim_init(&a2);
    lv_anim_set_var(&a2, s_ls_keypad_wrap);
    lv_anim_set_exec_cb(&a2, ls_reveal_opa_cb);
    lv_anim_set_values(&a2, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a2, 220);
    lv_anim_set_completed_cb(&a2, ls_hide_keypad_anim_done_cb);
    lv_anim_start(&a2);
}

static void ls_field_click_cb(lv_event_t *e)
{
    (void)e;
    if (s_ls_busy) return;
    ls_reveal_keypad();
}

static void ls_overlay_click_cb(lv_event_t *e)
{
    (void)e;
    if (s_ls_busy) return;
    ls_hide_keypad();
}

static void ls_do_validate(void)
{
    if (s_ls_busy || s_ls_len == 0) return;
    s_ls_busy = true;

    char stored[DEV_PIN_LEN + 1];
    ls_nvs_read_pin(stored, sizeof(stored));

    if (s_ls_len == DEV_PIN_LEN && strcmp(s_ls_buf, stored) == 0) {
        ls_dots_color(lv_color_hex(LS_COL_OK));
        ls_set_card_state(1);
        ls_set_status(g_lang->lbl_lock_access_granted, lv_color_hex(LS_COL_OK));
        lv_timer_t *t = lv_timer_create(ls_unlock_timer_cb, 550, NULL);
        lv_timer_set_repeat_count(t, 1);
    } else {
        ls_dots_color(lv_color_hex(LS_COL_ERR));
        ls_set_card_state(2);
        ls_set_status(g_lang->lbl_lock_pin_incorrect, lv_color_hex(LS_COL_ERR));
        if (s_ls_card) {
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, s_ls_card);
            lv_anim_set_exec_cb(&a, ls_anim_x_cb);
            lv_anim_set_values(&a, -8, 8);
            lv_anim_set_time(&a, 55);
            lv_anim_set_playback_time(&a, 55);
            lv_anim_set_repeat_count(&a, 4);
            lv_anim_start(&a);
        }
        lv_timer_t *t = lv_timer_create(ls_err_timer_cb, 900, NULL);
        lv_timer_set_repeat_count(t, 1);
    }
}

static void ls_key_cb(lv_event_t *e)
{
    if (s_ls_busy) return;
    intptr_t k = (intptr_t)lv_event_get_user_data(e);

    if (k == 'X') { // borrar
        if (s_ls_len > 0) { s_ls_len--; s_ls_buf[s_ls_len] = '\0'; }
        ls_update_dots();
        return;
    }
    if (k == 'K') { // OK
        ls_do_validate();
        return;
    }
    if (s_ls_len >= DEV_PIN_LEN) return;
    s_ls_buf[s_ls_len++] = (char)('0' + (int)k);
    s_ls_buf[s_ls_len]   = '\0';
    ls_update_dots();

    if (s_ls_len == DEV_PIN_LEN) ls_do_validate();
}

static void lock_screen_create(void)
{
    if (s_ls_sem == NULL) s_ls_sem = xSemaphoreCreateBinary();
    s_ls_len  = 0; s_ls_buf[0] = '\0';
    s_ls_busy = false;

    // ── Raiz: pantalla completa, negro solido ──────────────────────
    lv_obj_t *panel = lv_obj_create(lv_scr_act());
    s_ls_panel = panel;
    lv_obj_set_pos(panel, 0, 0);
    lv_obj_set_size(panel, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(panel, lv_color_hex(LS_COL_BG1), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_radius(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_opa(panel, LV_OPA_TRANSP, 0);  // se revela con fade-in al final

    // ── Overlay: toques fuera del teclado lo cierran ──────────────
    lv_obj_t *overlay = lv_obj_create(panel);
    s_ls_overlay = overlay;
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_size(overlay, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_set_style_radius(overlay, 0, 0);
    lv_obj_remove_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(overlay, ls_overlay_click_cb, LV_EVENT_CLICKED, NULL);

    // ── Logotipo Welltepp (arriba-derecha) ─────────────────────────
    lv_obj_t *logo = lv_img_create(panel);
    lv_img_set_src(logo, &lock_logo_wordmark);
    lv_obj_align(logo, LV_ALIGN_TOP_RIGHT, -22, 16);

    // ── Isotipo Welltepp (abajo-centro) ────────────────────────────
    lv_obj_t *iso = lv_img_create(panel);
    lv_img_set_src(iso, &lock_logo_isotipo);
    lv_obj_set_style_image_opa(iso, 230, 0);
    lv_obj_align(iso, LV_ALIGN_BOTTOM_MID, 0, -28);

    // ── Modo sin contrasena: saludo con avatar y entrada automatica ──
    if (!ls_lock_enabled_read()) {
        char greet_name[DEV_NAME_MAX];
        dev_nvs_read_device_name(greet_name, sizeof(greet_name));
        char greet_initial[2] = { greet_name[0] ? greet_name[0] : 'R', '\0' };

        lv_obj_t *avatar = lv_obj_create(panel);
        lv_obj_set_size(avatar, 100, 100);
        lv_obj_set_style_radius(avatar, 50, 0);
        lv_obj_set_style_bg_color(avatar, lv_color_hex(LS_COL_BTN_BG), 0);
        lv_obj_set_style_bg_opa(avatar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(avatar, lv_color_hex(LS_COL_ACCENT), 0);
        lv_obj_set_style_border_width(avatar, 2, 0);
        lv_obj_set_style_border_opa(avatar, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(avatar, 0, 0);
        lv_obj_remove_flag(avatar, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(avatar, LV_ALIGN_CENTER, 0, -50);

        lv_obj_t *av_lbl = lv_label_create(avatar);
        lv_label_set_text(av_lbl, greet_initial);
        lv_obj_set_style_text_font(av_lbl, &lv_font_montserrat_32, 0);
        lv_obj_set_style_text_color(av_lbl, lv_color_hex(LS_COL_TXT), 0);
        lv_obj_center(av_lbl);

        char greet_msg[DEV_NAME_MAX + 8];
        snprintf(greet_msg, sizeof(greet_msg), g_lang->lbl_lock_greeting_prefix, greet_name);

        lv_obj_t *greet = lv_label_create(panel);
        lv_label_set_text(greet, greet_msg);
        lv_obj_set_style_text_font(greet, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(greet, lv_color_hex(LS_COL_TXT), 0);
        lv_obj_set_style_text_align(greet, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(greet, LV_ALIGN_CENTER, 0, 30);

        lv_obj_fade_in(panel, 400, 0);
        lv_timer_t *t = lv_timer_create(ls_unlock_timer_cb, 2500, NULL);
        lv_timer_set_repeat_count(t, 1);
        return;
    }

    // ── Fila central: tarjeta + teclado ────────────────────────────
    lv_obj_t *center = lv_obj_create(panel);
    lv_obj_set_size(center, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(center, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(center, 0, 0);
    lv_obj_set_style_pad_all(center, 0, 0);
    lv_obj_set_style_pad_column(center, 28, 0);
    lv_obj_remove_flag(center, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(center, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_flex_flow(center, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(center, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_center(center);

    // ── Tarjeta (panel del tema clasico) ────────────────────────────
    lv_obj_t *card = lv_obj_create(center);
    s_ls_card = card;
    lv_obj_set_size(card, 360, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, lv_color_hex(LS_COL_CARD), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(LS_COL_BORDER), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 20, 0);
    lv_obj_set_style_pad_hor(card, 30, 0);
    lv_obj_set_style_pad_top(card, 30, 0);
    lv_obj_set_style_pad_bottom(card, 24, 0);
    lv_obj_set_style_pad_row(card, 16, 0);
    lv_obj_set_style_shadow_width(card, 30, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_50, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Avatar
    lv_obj_t *avatar = lv_obj_create(card);
    lv_obj_set_size(avatar, 100, 100);
    lv_obj_set_style_radius(avatar, 50, 0);
    lv_obj_set_style_bg_color(avatar, lv_color_hex(LS_COL_BTN_BG), 0);
    lv_obj_set_style_bg_opa(avatar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(avatar, lv_color_hex(LS_COL_ACCENT), 0);
    lv_obj_set_style_border_width(avatar, 2, 0);
    lv_obj_set_style_border_opa(avatar, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(avatar, 0, 0);
    lv_obj_remove_flag(avatar, LV_OBJ_FLAG_SCROLLABLE);
    // Nombre del dispositivo (mismo "Device Name" editable de System Info)
    char ls_dev_name[DEV_NAME_MAX];
    dev_nvs_read_device_name(ls_dev_name, sizeof(ls_dev_name));
    char ls_initial[2] = { ls_dev_name[0] ? ls_dev_name[0] : 'R', '\0' };

    lv_obj_t *av_lbl = lv_label_create(avatar);
    lv_label_set_text(av_lbl, ls_initial);
    lv_obj_set_style_text_font(av_lbl, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(av_lbl, lv_color_hex(LS_COL_TXT), 0);
    lv_obj_center(av_lbl);

    lv_obj_t *name = lv_label_create(card);
    lv_label_set_text(name, ls_dev_name);
    lv_obj_set_style_text_font(name, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(name, lv_color_hex(LS_COL_TXT), 0);

    // Campo "Toca para ingresar"
    lv_obj_t *field = lv_obj_create(card);
    s_ls_field = field;
    lv_obj_set_size(field, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(field, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(field, LV_OPA_40, 0);
    lv_obj_set_style_border_color(field, lv_color_hex(LS_COL_BORDER), 0);
    lv_obj_set_style_border_width(field, 1, 0);
    lv_obj_set_style_border_opa(field, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(field, 24, 0);
    lv_obj_set_style_pad_hor(field, 16, 0);
    lv_obj_set_style_pad_ver(field, 9, 0);
    lv_obj_remove_flag(field, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(field, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(field, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(field, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(field, ls_field_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *hint = lv_label_create(field);
    s_ls_hint_lbl = hint;
    lv_label_set_text(hint, g_lang->lbl_lock_tap_to_enter);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(LS_COL_TXT_DIM), 0);

    lv_obj_t *dotcon = lv_obj_create(field);
    s_ls_dotcon = dotcon;
    lv_obj_set_size(dotcon, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(dotcon, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dotcon, 0, 0);
    lv_obj_set_style_pad_all(dotcon, 0, 0);
    lv_obj_set_style_pad_column(dotcon, 10, 0);
    lv_obj_remove_flag(dotcon, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(dotcon, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dotcon, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(dotcon, LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < 4; i++) {
        lv_obj_t *dot = lv_obj_create(dotcon);
        s_ls_dots[i] = dot;
        lv_obj_set_size(dot, 11, 11);
        lv_obj_set_style_radius(dot, 6, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(dot, lv_color_hex(LS_COL_BTN_BD), 0);
        lv_obj_set_style_border_width(dot, 2, 0);
        lv_obj_set_style_border_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(dot, 0, 0);
        lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    }

    lv_obj_t *chevron = lv_label_create(field);
    lv_label_set_text(chevron, ">");
    lv_obj_set_style_text_font(chevron, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(chevron, lv_color_hex(LS_COL_TXT_DIM), 0);

    // Mensaje de estado
    lv_obj_t *status = lv_label_create(card);
    s_ls_status_lbl = status;
    lv_label_set_text(status, "");
    lv_obj_set_style_text_font(status, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(status, lv_color_hex(LS_COL_TXT_DIM), 0);

    // ── Teclado numerico deslizante ──────────────────────────────────
    lv_obj_t *kp_wrap = lv_obj_create(center);
    s_ls_keypad_wrap = kp_wrap;
    lv_obj_set_size(kp_wrap, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(kp_wrap, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(kp_wrap, 0, 0);
    lv_obj_set_style_pad_all(kp_wrap, 0, 0);
    lv_obj_remove_flag(kp_wrap, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(kp_wrap, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *keypad = lv_obj_create(kp_wrap);
    // 3 columnas * 102px + 2 huecos * 10px + padding 16px*2 + borde 1px*2 = 360 (igual que la tarjeta)
    lv_obj_set_size(keypad, 3 * 102 + 2 * 10 + 2 * 16 + 2 * 1, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(keypad, lv_color_hex(LS_COL_CARD), 0);
    lv_obj_set_style_bg_opa(keypad, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(keypad, lv_color_hex(LS_COL_BORDER), 0);
    lv_obj_set_style_border_width(keypad, 1, 0);
    lv_obj_set_style_border_opa(keypad, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(keypad, 20, 0);
    lv_obj_set_style_pad_all(keypad, 16, 0);
    lv_obj_set_style_pad_gap(keypad, 10, 0);
    lv_obj_set_style_shadow_width(keypad, 24, 0);
    lv_obj_set_style_shadow_opa(keypad, LV_OPA_40, 0);
    lv_obj_remove_flag(keypad, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(keypad, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(keypad, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    const char    *klbls[] = {"1","2","3","4","5","6","7","8","9","<","0","OK"};
    const intptr_t kvals[] = {  1,  2,  3,  4,  5,  6,  7,  8,  9,'X',  0,'K'};
    for (int i = 0; i < 12; i++) {
        bool is_ok = (kvals[i] == 'K');
        lv_obj_t *btn = lv_button_create(keypad);
        lv_obj_set_size(btn, 102, 64);
        lv_obj_set_style_radius(btn, 14, 0);
        lv_obj_set_style_shadow_opa(btn, 0, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(is_ok ? LS_COL_ACCENT : LS_COL_BTN_BG), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(LS_COL_ACCENT), LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn, lv_color_hex(is_ok ? LS_COL_ACCENT : LS_COL_BTN_BD), 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_border_opa(btn, LV_OPA_COVER, 0);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, klbls[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(is_ok ? LS_COL_ACCENT_TXT : LS_COL_TXT_DIM), 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(LS_COL_ACCENT_TXT), LV_STATE_PRESSED);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(btn, ls_key_cb, LV_EVENT_CLICKED, (void *)kvals[i]);
    }

    lv_obj_fade_in(panel, 400, 0);
}

// ---- Wrappers expuestos a actions.c (Settings > User) ----
void hmi_open_device_name_editor(void) { dn_open(); }
void hmi_open_lock_pin_editor(void)    { lp_open(); }
void hmi_set_lock_enabled(bool enabled) { ls_lock_enabled_write(enabled); }
bool hmi_get_lock_enabled(void)         { return ls_lock_enabled_read(); }
#endif // LOCK_SCREEN_ENABLE

#endif // DEV_MODE

#ifdef TEST_BATTERY_VOLTAGE
// Secuencia ASCENDENTE: empieza en critico y sube hasta llena, luego repite
static const int32_t bat_sim_values[] = {
    13200,  // rojo extremo (13.2 V — "APAGAR" + borde rojo)
    13800,  // rojo         (13.8 V — "Bateria critica")
    14200,  // rojo         (14.2 V — todavia critico)
    14400,  // naranja      (14.4 V — limite naranja/amarillo)
    15000,  // amarillo     (15.0 V — nivel medio)
    15200,  // amarillo     (15.2 V — limite amarillo/verde)
    16800,  // verde        (16.8 V — bateria llena)
};

void vTaskBatterySimTest(void *pvParameters)
{
    // Esperar a que la UI este lista (splash + ui_init)
    vTaskDelay(pdMS_TO_TICKS(200));

    int idx = 0;
    const int count = sizeof(bat_sim_values) / sizeof(bat_sim_values[0]);

    while (1)
    {
        hmi_handle_reg(HMI_REG_ROBOT_VOLTAGE, bat_sim_values[idx]);
        ESP_LOGW(TAG, "[SIM BAT] %.2f V  paso %d/%d",
                 bat_sim_values[idx] / 1000.0f, idx + 1, count);
        idx = (idx + 1) % count;
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
#endif

//*************************************************************************************
//*****************************FUNCIONES DE UTILIDAD **********************************
//*************************************************************************************
void hmi_process_buffer(uint8_t *buffer, uint16_t len)
{
    uint8_t reg;
    int32_t value;

    for (int i = 0; i <= len - 7; i++)
    {
        if (buffer[i] == HMI_HEADER_1 && buffer[i+1] == HMI_HEADER_2)
        {
            reg = buffer[i+2];

            value = ((int32_t)buffer[i+3] << 24) |
                    ((int32_t)buffer[i+4] << 16) |
                    ((int32_t)buffer[i+5] << 8 ) |
                    ((int32_t)buffer[i+6]);

            hmi_handle_reg(reg, value);
#ifdef TEST_UART_RX_DISPLAY
            rx_disp_log_frame(reg, value);
#endif
            i += 6; // saltar al siguiente frame
        }
    }
}

static bool bat_alert_active = false;

static void bat_blink_anim_cb(void *obj, int32_t opa)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)opa, 0);
}

static lv_timer_t *bat_blink_border_timer = NULL;
static bool blink_border_state = false;

static void bat_blink_border_cb(lv_timer_t *timer)
{
    blink_border_state = !blink_border_state;
    if (objects.obj6 != NULL) {
        lv_obj_set_style_border_color(objects.obj6,
            blink_border_state ? lv_color_hex(0xE74C3C) : lv_color_hex(0xff252525),
            LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(objects.obj6,
            blink_border_state ? 4 : 2,
            LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (objects.obj0 != NULL) {
        lv_obj_set_style_bg_color(objects.obj0,
            blink_border_state ? lv_color_hex(0xE74C3C) : lv_color_hex(0xff393939),
            LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

// --- Cache de registros que la consola manda una sola vez al conectar ---
// Si llegan mientras ui_init() todavia no termino de construir los widgets
// (splash, pantalla de bloqueo, etc.), objects.xxx sigue siendo NULL y el
// valor se pierde para siempre porque nadie lo vuelve a mandar. Se guarda
// el valor crudo aqui y se reaplica una vez desde app_main() (ver
// hmi_reapply_cached_boot_regs()) apenas panels_startup_init() termina.
static int32_t s_cached_robot_serial = -1;
static int32_t s_cached_robot_model  = -1;
static int32_t s_cached_fw_version   = -1;

static void apply_robot_serial(int32_t value)
{
    char buff[30];
    snprintf(buff, sizeof(buff), "Robot S/N   :  RD90R-%06ld", value);
    set_var_robot_serial(buff);

    HMI_LV_SAFE_OBJ(objects.robot_serial_number,
        lv_label_set_text_static(objects.robot_serial_number, get_var_robot_serial()));

    char sn_buff[20];
    snprintf(sn_buff, sizeof(sn_buff), "RD90R-%06ld", value);
    HMI_LV_SAFE_OBJ(objects.sysinfo_robot_serial_value,
        lv_label_set_text(objects.sysinfo_robot_serial_value, sn_buff));
}

static void apply_robot_model(int32_t value)
{
    static const char *model_names[] = { "RD80", "RD90", "RD100" };
    const char *model = (value >= 0 && value < 3) ? model_names[value] : "RD??";
    HMI_LV_LOCKED(
        if (objects.robot_model_label) {
            lv_label_set_text(objects.robot_model_label, model);
        }
        if (objects.robot_model_pill) {
            lv_obj_remove_flag(objects.robot_model_pill, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_bigview_robot_model_label) {
            lv_label_set_text(s_bigview_robot_model_label, model);
            lv_obj_remove_flag(s_bigview_robot_model_label, LV_OBJ_FLAG_HIDDEN);
        }
    );
}

static void apply_fw_version(int32_t value)
{
    uint8_t major = (value >> 16) & 0xFF;
    uint8_t minor = (value >> 8)  & 0xFF;
    uint8_t patch = value & 0xFF;
    char buff[40];
    snprintf(buff, sizeof(buff), "Console firmware: v%u.%u.%u", major, minor, patch);
    HMI_LV_SAFE_OBJ(objects.console_fw_version_label,
        lv_label_set_text(objects.console_fw_version_label, buff));
}

void hmi_reapply_cached_boot_regs(void)
{
    if (s_cached_robot_serial >= 0) apply_robot_serial(s_cached_robot_serial);
    if (s_cached_robot_model  >= 0) apply_robot_model(s_cached_robot_model);
    if (s_cached_fw_version   >= 0) apply_fw_version(s_cached_fw_version);
}

//*************************************************************************************
// Settings > Camera: interruptor de 3 posiciones para RL1 (solo reversa).
// ON = fuerza RL1 encendido sin importar el estado del motor.
// OFF = desactivado, no manda nada (ni al elegir esta opcion ni por reversa).
// AUTO = comportamiento de siempre, sigue la reversa (default).
//*************************************************************************************
typedef enum { RL1_MODE_ON = 0, RL1_MODE_OFF, RL1_MODE_AUTO } rl1_mode_t;
static rl1_mode_t s_rl1_mode = RL1_MODE_OFF;
static bool s_rl1_sent = false;           // ultimo valor de RL1 realmente mandado
static bool s_last_reversing_state = false; // ultimo estado de reversa conocido (para resincronizar al volver a AUTO)

// Combo para abrir el panel de manejo: reversa (HMI_REG_MOTOR) + joystick
// de servo hacia abajo (HMI_REG_JOY2), AMBOS a la vez. Se llama desde los
// dos handlers cada vez que cualquiera de las dos condiciones cambia.
static bool s_servo_down_state = false;
static bool s_combo_open_fired = false; // evita reabrir en cada mensaje mientras se sostienen ambos
static void bigview_check_reverse_servo_combo(void)
{
    bool both = s_last_reversing_state && s_servo_down_state;
    if (both && !s_combo_open_fired) {
        s_combo_open_fired = true;
        HMI_LV_LOCKED(encoder_bigview_open());
    } else if (!both) {
        s_combo_open_fired = false;
    }
}

static void camera_rl1_mode_style_update(void)
{
    hmi_style_btn(objects.cam_rl1_btn_on,   s_rl1_mode == RL1_MODE_ON);
    hmi_style_btn(objects.cam_rl1_btn_off,  s_rl1_mode == RL1_MODE_OFF);
    hmi_style_btn(objects.cam_rl1_btn_auto, s_rl1_mode == RL1_MODE_AUTO);
}

static void camera_rl1_on_cb(lv_event_t *e)
{
    (void)e;
    s_rl1_mode = RL1_MODE_ON;
    s_rl1_sent = true;
    hmi_send_data(HMI_REG_RL1, 1);
    camera_rl1_mode_style_update();
}

static void camera_rl1_off_cb(lv_event_t *e)
{
    (void)e;
    // Apaga el rele de una (no lo deja en lo que haya quedado) y despues
    // ya no vuelve a mandar nada mientras siga en OFF, aunque el robot
    // entre/salga de reversa.
    s_rl1_mode = RL1_MODE_OFF;
    s_rl1_sent = false;
    hmi_send_data(HMI_REG_RL1, 0);
    camera_rl1_mode_style_update();
}

static void camera_rl1_auto_cb(lv_event_t *e)
{
    (void)e;
    s_rl1_mode = RL1_MODE_AUTO;
    // Resincroniza de inmediato con el estado real de reversa, por si
    // quedo desfasado mientras estaba en ON/OFF.
    if (s_last_reversing_state != s_rl1_sent) {
        s_rl1_sent = s_last_reversing_state;
        hmi_send_data(HMI_REG_RL1, s_rl1_sent ? 1 : 0);
    }
    camera_rl1_mode_style_update();
}

// Cablea los 3 botones y deja el estado inicial (AUTO) — llamar una vez
// desde app_main() junto con el resto del cableado post ui_init().
static void camera_rl1_mode_wire(void)
{
    if (objects.cam_rl1_btn_on)
        lv_obj_add_event_cb(objects.cam_rl1_btn_on, camera_rl1_on_cb, LV_EVENT_CLICKED, NULL);
    if (objects.cam_rl1_btn_off)
        lv_obj_add_event_cb(objects.cam_rl1_btn_off, camera_rl1_off_cb, LV_EVENT_CLICKED, NULL);
    if (objects.cam_rl1_btn_auto)
        lv_obj_add_event_cb(objects.cam_rl1_btn_auto, camera_rl1_auto_cb, LV_EVENT_CLICKED, NULL);
    camera_rl1_mode_style_update();
}

// Centro real del joystick de servo (JOY2) en reposo — confirmado en
// pruebas (~2132, no 0) — y zona muerta a partir de ahi para considerar
// que se empujo hacia abajo (abajo = Y aumenta).
#define JOY2_Y_CENTER      2132
#define JOY_SERVO_DEADZONE 300

void hmi_handle_reg(uint8_t reg, int32_t value)
{
    switch (reg)
    {
    case HMI_REG_ONLINE_INDICATOR:
    {
        static bool s_robot_connected = false;
        if (!s_robot_connected && value > 0) {
            s_robot_connected = true;
            hmi_log(LOG_RX, "Robot connected");
        } else if (s_robot_connected && value == 0) {
            s_robot_connected = false;
            hmi_log(LOG_WARN, "Robot disconnected");
            // El pill "RD.." depende del robot conectado: si se desconecta,
            // se oculta igual que se apaga el LED verde. Reaparece cuando
            // llegue el proximo HMI_REG_ROBOT_MODEL (0x25) tras la
            // reconexion. Solo se dispara en el flanco de desconexion (no
            // en cada mensaje con value=0) para no ocultarlo por ruido.
            HMI_LV_SAFE_OBJ(objects.robot_model_pill,
                lv_obj_add_flag(objects.robot_model_pill, LV_OBJ_FLAG_HIDDEN));
            HMI_LV_SAFE_OBJ(s_bigview_robot_model_label,
                lv_obj_add_flag(s_bigview_robot_model_label, LV_OBJ_FLAG_HIDDEN));
            // El S/N tambien queda obsoleto al desconectar: limpiarlo para
            // no mostrar el ultimo valor de un robot que ya no esta.
            HMI_LV_SAFE_OBJ(objects.sysinfo_robot_serial_value,
                lv_label_set_text(objects.sysinfo_robot_serial_value, "---"));
            HMI_LV_SAFE_OBJ(objects.robot_serial_number,
                lv_label_set_text(objects.robot_serial_number, "---"));
        }
        HMI_LV_SAFE_OBJ(objects.led_online, lv_led_set_brightness(objects.led_online, (uint8_t)value));
        HMI_LV_SAFE_OBJ(s_bigview_led_online, lv_led_set_brightness(s_bigview_led_online, (uint8_t)value));
        ESP_LOGI(TAG, "HMI_REG_ONLINE_INDICATOR: %d", value);
        break;
    }

    case HMI_REG_BLUETOOTH_INDICATOR:
    {
        HMI_LV_SAFE_OBJ(objects.led_bluetooth, lv_led_set_brightness(objects.led_bluetooth, (uint8_t)value));
        HMI_LV_SAFE_OBJ(s_bigview_led_bt, lv_led_set_brightness(s_bigview_led_bt, (uint8_t)value));
        HMI_LV_SAFE_OBJ(objects.bt_panel_led, lv_led_set_brightness(objects.bt_panel_led, (uint8_t)value));
        s_bt_connected = (value != 0);
        HMI_LV_SAFE_OBJ(objects.bt_panel_status_label,
            lv_label_set_text(objects.bt_panel_status_label, bt_status_text(s_bt_connected)));
        if (!s_bt_connected) {
            s_bt_mac_hi = 0;
            HMI_LV_SAFE_OBJ(objects.bt_panel_mac_label, lv_label_set_text(objects.bt_panel_mac_label, "---"));
        }
        ESP_LOGI(TAG, "HMI_REG_BLUETOOTH_INDICATOR: %d", (uint8_t)value);
        break;
    }

    case HMI_REG_ROBOT_LED_CHANGED:
    {
        // Brillo del LED del robot cambiado del lado de la consola (ej. un
        // boton fisico) — solo actualiza el slider/label en pantalla, NO
        // reenvia el registro (action_robot_brightness_changed ya lo manda
        // cuando el cambio sale de aca; reenviarlo entraria en loop).
        int32_t slider_value = (value * 100 + 511) / 1023; // redondeado, no truncado
        HMI_LV_LOCKED(
            if (objects.obj16) {
                lv_slider_set_value(objects.obj16, slider_value, LV_ANIM_OFF);
            }
            if (objects.brightness_label) {
                char buff[16];
                snprintf(buff, sizeof(buff), "%d %%", (int)slider_value);
                set_var_brightness(buff);
                lv_label_set_text_static(objects.brightness_label, get_var_brightness());
            }
            hmi_bigview_led_level_refresh();
        );
        ESP_LOGI(TAG, "HMI_REG_ROBOT_LED_CHANGED: %d", (int)value);
        break;
    }

    // RECEPCION
    case HMI_REG_ENCODER:
    {
        HMI_LV_SAFE_OBJ(objects.encoder_value, hmi_encoder_set_raw((int32_t)value));
        ESP_LOGI(TAG, "Encoder: %d", (int32_t)value);
        break;
    }

    case HMI_REG_CONSOLE_VOLTAGE:
    {
        float value_f = (float)value / 1000.0f;
        uint8_t percent = battery_percent((uint16_t)value);

        char buff[10];
        snprintf(buff, sizeof(buff), "%2.2f V", value_f);
        set_var_console_voltage(buff);
        snprintf(buff, sizeof(buff), "%3u %%", percent);
        set_var_console_voltage_percent(buff);

        ESP_LOGW(TAG, ">>> CONSOLE_VOLTAGE recibido: %d mV = %.2f V = %d%%", value, value_f, percent);

        if (objects.console_voltage != NULL || objects.console_voltage_percent_label != NULL || objects.console_voltage_percent_bar != NULL)
        {
            esp_err_t lock_ret = esp_lv_adapter_lock(-1);
            ESP_LOGW(TAG, ">>> LVGL lock: %s", lock_ret == ESP_OK ? "OK" : "FALLO");
            if (lock_ret == ESP_OK) {
                if (objects.console_voltage != NULL) {
                    lv_label_set_text_static(objects.console_voltage,
                        g_bat_display_percent ? get_var_console_voltage_percent() : get_var_console_voltage());
                }
                if (objects.console_voltage_percent_label != NULL) {
                    lv_label_set_text_static(objects.console_voltage_percent_label, get_var_console_voltage_percent());
                }
                if (objects.console_voltage_percent_bar != NULL) {
                    lv_bar_set_value(objects.console_voltage_percent_bar, (int32_t)percent, LV_ANIM_OFF);
                }
                esp_lv_adapter_unlock();
                ESP_LOGW(TAG, ">>> UI actualizada: %s", get_var_console_voltage());
            }
        } else {
            ESP_LOGE(TAG, ">>> OBJECTS NULOS - UI no actualiza");
        }
        HMI_LV_SAFE_OBJ(s_bigview_console_batt_label, bigview_battery_refresh());
        break;
    }

    case HMI_REG_ROBOT_VOLTAGE:
    {
        float value_f = (float)value / 1000.0f;
        uint8_t percent = battery_percent(value);

        char buff[10];
        snprintf(buff, sizeof(buff), "%2.2f V", value_f);
        set_var_robot_voltage(buff);
        char robot_pct_buff[8];
        snprintf(robot_pct_buff, sizeof(robot_pct_buff), "%3u %%", percent);
        set_var_robot_voltage_percent(robot_pct_buff);

        lv_color_t bat_color;
        bool show_alert    = false;
        bool extreme_crit  = false;

        if (value <= 0) {
            bat_color = lv_color_hex(0x555555);             // gris — sin datos
        } else if (value >= 15200) {
            bat_color = lv_color_hex(0x27AE60);             // verde
        } else if (value >= 14400) {
            bat_color = lv_color_hex(0xF5C518);             // amarillo
        } else if (value >= 14000) {
            bat_color = lv_color_hex(0xE67E22);             // naranja
        } else {
            bat_color   = lv_color_hex(0xE74C3C);           // rojo
            show_alert  = true;
            extreme_crit = (value <= 13200);
        }

        HMI_LV_LOCKED(
            if (objects.robot_voltage != NULL) {
                lv_label_set_text(objects.robot_voltage,
                    g_bat_display_percent ? robot_pct_buff : get_var_robot_voltage());
            }
            if (objects.robot_voltage_percent_bar != NULL) {
                lv_bar_set_value(objects.robot_voltage_percent_bar, (int32_t)percent, LV_ANIM_OFF);
                lv_obj_set_style_bg_color(objects.robot_voltage_percent_bar, bat_color, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                lv_obj_invalidate(objects.robot_voltage_percent_bar);
            }
            if (objects.robot_battery_alert != NULL) {
                if (show_alert) {
                    lv_label_set_text(objects.robot_battery_alert,
                        extreme_crit ? g_lang->alert_shutdown : g_lang->alert_battery_critical);
                    lv_obj_remove_flag(objects.robot_battery_alert, LV_OBJ_FLAG_HIDDEN);
                } else {
                    lv_obj_add_flag(objects.robot_battery_alert, LV_OBJ_FLAG_HIDDEN);
                }
            }
            // Parpadeo: arrancar al entrar en alerta, detener al salir
            if (show_alert && !bat_alert_active) {
                bat_alert_active = true;
                hmi_log(LOG_WARN, extreme_crit ? "Battery CRITICAL - SHUTDOWN" : "Battery low");
                // Barra de batería (fade animation)
                lv_anim_t a;
                lv_anim_init(&a);
                lv_anim_set_var(&a, objects.robot_voltage_percent_bar);
                lv_anim_set_exec_cb(&a, bat_blink_anim_cb);
                lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
                lv_anim_set_duration(&a, 400);
                lv_anim_set_playback_duration(&a, 400);
                lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
                lv_anim_start(&a);
                // Marco obj6 + barra superior obj0 (timer toggle)
                blink_border_state = true;
                if (objects.obj6 != NULL) {
                    lv_obj_set_style_border_color(objects.obj6, lv_color_hex(0xE74C3C), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(objects.obj6, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                if (objects.obj0 != NULL) {
                    lv_obj_set_style_bg_color(objects.obj0, lv_color_hex(0xE74C3C), LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                if (bat_blink_border_timer == NULL) {
                    bat_blink_border_timer = lv_timer_create(bat_blink_border_cb, 500, NULL);
                }
            } else if (!show_alert && bat_alert_active) {
                bat_alert_active = false;
                // Detener animación de barra
                lv_anim_delete(objects.robot_voltage_percent_bar, bat_blink_anim_cb);
                lv_obj_set_style_opa(objects.robot_voltage_percent_bar, LV_OPA_COVER, 0);
                // Detener timer y restaurar colores normales
                if (bat_blink_border_timer != NULL) {
                    lv_timer_delete(bat_blink_border_timer);
                    bat_blink_border_timer = NULL;
                }
                if (objects.obj6 != NULL) {
                    lv_obj_set_style_border_color(objects.obj6, hmi_theme_bd_card(), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(objects.obj6, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                if (objects.obj0 != NULL) {
                    lv_obj_set_style_bg_color(objects.obj0, hmi_theme_bg_topbar(), LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
            bigview_battery_refresh();
        );

        ESP_LOGI(TAG, "HMI_REG_ROBOT_VOLTAGE: %d", value);
        break;
    }

    case HMI_REG_ANGLE_X:
    {
        int16_t value_f = (int16_t)value / 100;
        char buff[10];
        snprintf(buff, sizeof(buff), "%3d °", value_f);
        set_var_angle_x_value(buff);

        HMI_LV_SAFE_OBJ(objects.angle_x, lv_label_set_text_static(objects.angle_x, get_var_angle_x_value()));
        // Grafico del panel de manejo desactivado para X por ahora — solo
        // se pidio ver Y. La tarjeta TILT arriba sigue mostrando X normal.
        ESP_LOGI(TAG, "HMI_REG_ANGLE_X: %d", value);
        break;
    }

    case HMI_REG_ANGLE_Y:
    {
        int16_t value_f = (int16_t)value / 100;
        char buff[10];
        snprintf(buff, sizeof(buff), "%3d °", value_f);
        set_var_angle_y_value(buff);

        HMI_LV_SAFE_OBJ(objects.angle_y, lv_label_set_text_static(objects.angle_y, get_var_angle_y_value()));
        // Solo cachea el valor — el trazo lo empuja hmi_encoder_set_raw()
        // cuando el encoder avanza, no la llegada de este registro.
        // *** PRUEBA: sin decimal, entero truncado (value_f) en vez de
        // (float)value/100.0f — para comparar como se ve el dibujo. ***
        s_bigview_current_pitch = (float)value_f;
        ESP_LOGI(TAG, "HMI_REG_ANGLE_Y: %d", value);
        break;
    }

    case HMI_REG_BLUETOOTH_PASSWORD:
    {
        char buff[10];
        snprintf(buff, sizeof(buff), "%06ld", value);
        set_var_bluetooth_password_string(buff);

        HMI_LV_SAFE_OBJ(objects.bluetooth_password_label, lv_label_set_text_static(objects.bluetooth_password_label, get_var_bluetooth_password_string()));
        HMI_LV_SAFE_OBJ(objects.bt_panel_password_label, lv_label_set_text_static(objects.bt_panel_password_label, get_var_bluetooth_password_string()));
        ESP_LOGI(TAG, "HMI_REG_BLUETOOTH_PASSWORD: %06d", value);
        break;
    }

    case HMI_REG_ROBOT_SERIAL:
    {
        s_cached_robot_serial = value;
        apply_robot_serial(value);
        ESP_LOGI(TAG, "HMI_REG_ROBOT_SERIAL: %06ld", value);
        break;
    }

    case HMI_REG_ROBOT_MODEL:
    {
        // Pill "RD.." de la barra superior (antes de Online) — arranca oculto
        // y aparece con el primer valor que llega.
        s_cached_robot_model = value;
        apply_robot_model(value);
        ESP_LOGI(TAG, "HMI_REG_ROBOT_MODEL: %d", (int)value);
        break;
    }

    case HMI_REG_BLUETOOTH_MAC_HI:
    {
        // Bytes 5-4 de la MAC — se cachean y se muestran recien cuando
        // llegue el LO (bytes 3-0), que la consola manda justo despues.
        s_bt_mac_hi = (uint16_t)(value & 0xFFFF);
        ESP_LOGI(TAG, "HMI_REG_BLUETOOTH_MAC_HI: 0x%04X", s_bt_mac_hi);
        break;
    }

    case HMI_REG_BLUETOOTH_MAC_LO:
    {
        uint32_t lo = (uint32_t)value;
        uint8_t mac[6] = {
            (uint8_t)(lo & 0xFF), (uint8_t)((lo >> 8) & 0xFF),
            (uint8_t)((lo >> 16) & 0xFF), (uint8_t)((lo >> 24) & 0xFF),
            (uint8_t)(s_bt_mac_hi & 0xFF), (uint8_t)((s_bt_mac_hi >> 8) & 0xFF),
        };
        char buff[18];
        snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
        HMI_LV_SAFE_OBJ(objects.bt_panel_mac_label, lv_label_set_text(objects.bt_panel_mac_label, buff));
        ESP_LOGI(TAG, "HMI_REG_BLUETOOTH_MAC_LO: %s", buff);
        break;
    }

    case HMI_REG_JOY1:
    {
        int16_t joy1_x = (int16_t)((value >> 16) & 0xFFFF);
        int16_t joy1_y = (int16_t)(value & 0xFFFF);
        ESP_LOGI(TAG, "HMI_REG_JOY1: x=%d y=%d", joy1_x, joy1_y);
#ifdef DEV_MODE
        s_joy_j1x = joy1_x; s_joy_j1y = joy1_y;
#endif
        break;
    }

    case HMI_REG_JOY2:
    {
        int16_t joy2_x = (int16_t)((value >> 16) & 0xFFFF);
        int16_t joy2_y = (int16_t)(value & 0xFFFF);
        ESP_LOGI(TAG, "HMI_REG_JOY2: x=%d y=%d", joy2_x, joy2_y);
#ifdef DEV_MODE
        s_joy_j2x = joy2_x; s_joy_j2y = joy2_y;
        dev_joy_log_update();
#endif
        // JOY2 = joystick de servo (camara/cabeza-cuello). El centro NO es
        // 0 — confirmado en pruebas que reposa en ~2132 y que "abajo"
        // AUMENTA el valor de Y. Se combina con reversa en
        // bigview_check_reverse_servo_combo() — deadzone ajustable en
        // JOY_SERVO_DEADZONE.
        s_servo_down_state = ((int32_t)joy2_y - JOY2_Y_CENTER) > JOY_SERVO_DEADZONE;
        bigview_check_reverse_servo_combo();
        break;
    }

    case HMI_REG_BUTTONS:
    {
        bool btn_j1 = (value >> 1) & 0x01;
        bool btn_j2 = (value)      & 0x01;
        ESP_LOGI(TAG, "HMI_REG_BUTTONS: J1=%d J2=%d", btn_j1, btn_j2);
        if (btn_j1) hmi_log(LOG_RX, "Boton J1 presionado");
        if (btn_j2) hmi_log(LOG_RX, "Boton J2 presionado");
#ifdef DEV_MODE
        s_joy_btn1 = btn_j1; s_joy_btn2 = btn_j2;
        dev_joy_log_update();
#endif
        break;
    }

    case HMI_REG_PONG:
    {
        s_last_pong_ms  = (uint32_t)(esp_timer_get_time() / 1000ULL);
        s_pong_received = true;
        HMI_LV_SAFE_OBJ(led_hmi_conn, lv_led_set_brightness(led_hmi_conn, 255));
        ESP_LOGW(TAG, "<<< PONG recibido: comunicacion bidireccional OK");
        hmi_log(LOG_OK, "PONG OK - bidireccional");
        break;
    }

    case HMI_REG_P1:
    {
        ESP_LOGI(TAG, "HMI_REG_P1: %d", (int)value);
#ifdef DEV_MODE
        s_p1_value = (int16_t)(value & 0xFFFF);
        dev_joy_log_update();
#endif
        break;
    }

    case HMI_REG_MOTOR:
    {
        int32_t cmd = (value >> 16) & 0xFFFF;
        ESP_LOGI(TAG, "HMI_REG_MOTOR: cmd=%d vel=%d", (int)cmd, (int)(value & 0xFFFF));
#ifdef DEV_MODE
        s_motor_cmd = (int16_t)cmd;
        s_motor_vel = (int16_t)(value & 0xFFFF);
#endif
        // Settings > Camera: el LED y el cuadro "RETROCEDIENDO"/"AVANZANDO"
        // son fijos (no se ocultan ni cambian de tamano, para que nada se
        // corra de lugar); solo el brillo del LED y el color del texto
        // cambian con el estado.
        bool reversing = (cmd == MOTOR_CMD_REVERSE);
        bool advancing = (cmd == MOTOR_CMD_FORWARD);
        HMI_LV_LOCKED(
            if (objects.cam_reverse_led) {
                lv_led_set_brightness(objects.cam_reverse_led, reversing ? 255 : 0);
            }
            if (objects.cam_reverse_label) {
                lv_color_t txt = reversing ? lv_color_hex(0xffe74c3c) : lv_color_hex(0xffaaaaaa);
                lv_obj_set_style_text_color(objects.cam_reverse_label, txt, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            if (objects.cam_forward_led) {
                lv_led_set_brightness(objects.cam_forward_led, advancing ? 255 : 0);
            }
            if (objects.cam_forward_label) {
                lv_color_t txt = advancing ? lv_color_hex(0xff27ae60) : lv_color_hex(0xffaaaaaa);
                lv_obj_set_style_text_color(objects.cam_forward_label, txt, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        );
        // RL2 (avance): sin interruptor, sigue el motor siempre, solo al
        // cambiar de estado.
        static bool s_rl2_sent = false;
        if (advancing != s_rl2_sent) {
            s_rl2_sent = advancing;
            hmi_send_data(HMI_REG_RL2, advancing ? 1 : 0);
        }

        // RL1 (reversa): tiene el interruptor de 3 posiciones (Settings >
        // Camera). Solo se auto-manda en modo AUTO; en ON/OFF el usuario
        // manda el control (ver camera_rl1_*_cb()).
        s_last_reversing_state = reversing;
        if (s_rl1_mode == RL1_MODE_AUTO && reversing != s_rl1_sent) {
            s_rl1_sent = reversing;
            hmi_send_data(HMI_REG_RL1, reversing ? 1 : 0);
        }

        // Combo para abrir el panel de manejo: reversa + joystick de servo
        // (JOY2) hacia abajo, AMBOS a la vez. Ver bigview_check_reverse_servo_combo().
        bigview_check_reverse_servo_combo();
        break;
    }

    case HMI_REG_SRV1_ANGLE:
    {
        ESP_LOGI(TAG, "HMI_REG_SRV1_ANGLE: %d", (int)value);
#ifdef DEV_MODE
        s_srv1_angle = (int16_t)value;
#endif
        break;
    }

    case HMI_REG_SRV2_ANGLE:
    {
        ESP_LOGI(TAG, "HMI_REG_SRV2_ANGLE: %d", (int)value);
#ifdef DEV_MODE
        s_srv2_angle = (int16_t)value;
#endif
        break;
    }

    case HMI_REG_SRV3_ANGLE:
    {
        ESP_LOGI(TAG, "HMI_REG_SRV3_ANGLE: %d", (int)value);
#ifdef DEV_MODE
        s_srv3_angle = (int16_t)value;
#endif
        break;
    }

    case HMI_REG_OTA_STATUS:
    {
        s_console_ota_last_value = value;
        const char *text = console_ota_status_text(value);

        HMI_LV_LOCKED(
            if (objects.console_ota_status_label) {
                lv_label_set_text(objects.console_ota_status_label, text);
            }
            if (objects.console_ota_led) {
                bool busy = (value == 0 || value == 3);
                bool ok   = (value == 2 || value == 4);
                lv_led_set_color(objects.console_ota_led,
                    ok ? lv_color_hex(0xff00971c) : (busy ? lv_color_hex(0xfff5c518) : lv_color_hex(0xffe74c3c)));
                lv_led_set_brightness(objects.console_ota_led, (value == 1) ? 0 : 255);
            }
        );
        ESP_LOGI(TAG, "HMI_REG_OTA_STATUS: %d (%s)", (int)value, text);
        break;
    }

    case HMI_REG_FW_VERSION:
    {
        s_cached_fw_version = value;
        apply_fw_version(value);
        uint8_t major = (value >> 16) & 0xFF;
        uint8_t minor = (value >> 8)  & 0xFF;
        uint8_t patch = value & 0xFF;
        ESP_LOGI(TAG, "HMI_REG_FW_VERSION: v%u.%u.%u", major, minor, patch);
        break;
    }

    case HMI_REG_WIFI_STATUS:
    {
        s_console_wifi_connected = (value != 0);
        if (!s_console_wifi_connected) s_console_wifi_requested_on = false;
        HMI_LV_LOCKED(console_wifi_ui_refresh());
        ESP_LOGI(TAG, "HMI_REG_WIFI_STATUS: %d", (int)value);
        break;
    }

    default:
        ESP_LOGW(TAG, "Unknown REG: 0x%02X", reg);
        break;
    }
}


//*************************************************************************************
//********************************* SISTEMA DE LOGS **********************************
//*************************************************************************************
#define LOG_MAX_LINES  12
#define LOG_LINE_LEN   58

static char      s_log_lines[LOG_MAX_LINES][LOG_LINE_LEN];
static int       s_log_head  = 0;
static int       s_log_count = 0;

// Timer LVGL: apaga LED si no llega PONG en HMI_CONN_TIMEOUT_MS
static void hmi_conn_check_cb(lv_timer_t *t)
{
    if (!led_hmi_conn) return;
    if (!s_pong_received) { lv_led_set_brightness(led_hmi_conn, 0); return; }
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
    bool alive = (now_ms - s_last_pong_ms) < HMI_CONN_TIMEOUT_MS;
    lv_led_set_brightness(led_hmi_conn, alive ? 255 : 0);
}

static void hmi_conn_indicator_create(void)
{
    // Ampliar el pill Online hacia la izquierda para no chocar con Bluetooth
    lv_obj_t *pill = lv_obj_get_parent(objects.led_online);  // objects.obj2
    lv_obj_set_width(pill, 130);
    lv_obj_set_x(pill, 427);  // 451 - 24 = borde derecho queda igual que antes

    // Crear LED amarillo como hijo del mismo pill
    led_hmi_conn = lv_led_create(pill);
    lv_obj_set_size(led_hmi_conn, 12, 12);
    lv_led_set_color(led_hmi_conn, lv_color_hex(0xFFBF00));
    lv_led_set_brightness(led_hmi_conn, 0);
    // Mover a index 1: [verde][amarillo][Online]
    lv_obj_move_to_index(led_hmi_conn, 1);

    // Timer que revisa cada segundo si sigue llegando PONG
    lv_timer_create(hmi_conn_check_cb, 1000, NULL);
}

static void hmi_log_refresh(void)
{
    static char buf[LOG_MAX_LINES * LOG_LINE_LEN + 1];
    buf[0] = '\0';
    int start = (s_log_count < LOG_MAX_LINES) ? 0 : s_log_head;
    for (int i = 0; i < s_log_count; i++) {
        int idx = (start + i) % LOG_MAX_LINES;
        strcat(buf, s_log_lines[idx]);
        if (i < s_log_count - 1) strcat(buf, "\n");
    }
    HMI_LV_SAFE_OBJ(objects.sysinfo_logs_body, lv_label_set_text(objects.sysinfo_logs_body, buf));
    HMI_LV_SAFE_OBJ(s_logs_tbox, lv_obj_scroll_to_y(s_logs_tbox, LV_COORD_MAX, LV_ANIM_OFF));
}

static void logs_clear_cb(lv_event_t *e)
{
    s_log_head = 0; s_log_count = 0;
    memset(s_log_lines, 0, sizeof(s_log_lines));
    if (objects.sysinfo_logs_body)
        lv_label_set_text(objects.sysinfo_logs_body, "Limpiado");
}

static void logs_show_cb(lv_event_t *e)
{
    if (!s_logs_panel) return;
    lv_obj_add_flag(objects.sysinfo_content_device,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.sysinfo_content_version, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.sysinfo_content_guide,   LV_OBJ_FLAG_HIDDEN);
    if (objects.sysinfo_content_update) lv_obj_add_flag(objects.sysinfo_content_update, LV_OBJ_FLAG_HIDDEN);
    if (s_dev_panel)    lv_obj_add_flag(s_dev_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_serial_panel) lv_obj_add_flag(s_serial_panel, LV_OBJ_FLAG_HIDDEN);
    if (s_joy_panel)    lv_obj_add_flag(s_joy_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_val_panel)    lv_obj_add_flag(s_val_panel,    LV_OBJ_FLAG_HIDDEN);
    if (s_test_panel)   lv_obj_add_flag(s_test_panel,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_logs_panel, LV_OBJ_FLAG_HIDDEN);
    hmi_style_btn(objects.sysinfo_btn_device,  false);
    hmi_style_btn(objects.sysinfo_btn_version, false);
    hmi_style_btn(objects.sysinfo_btn_guide,   false);
    hmi_style_btn(s_logs_btn, true);
    if (s_dev_btn)    hmi_style_btn(s_dev_btn,    false);
    if (s_serial_btn) hmi_style_btn(s_serial_btn, false);
    if (s_joy_btn)    hmi_style_btn(s_joy_btn,    false);
    if (s_val_btn)    hmi_style_btn(s_val_btn,    false);
    if (s_test_btn)   hmi_style_btn(s_test_btn,   false);
}

static void logs_panel_create(lv_obj_t *content_area, lv_obj_t *nav_col)
{
    if (s_logs_panel) return;
    lv_obj_t *p = lv_obj_create(content_area);
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_size(p, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(p, 0, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_set_style_pad_row(p, 6, 0);
    lv_obj_set_style_radius(p, 0, 0);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(p, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(p, LV_OBJ_FLAG_HIDDEN);
    s_logs_panel = p;

    lv_obj_t *title = lv_label_create(p);
    lv_label_set_text(title, " SYSTEM LOGS");
    lv_obj_set_size(title, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, hmi_theme_accent(), 0);
    lv_obj_set_style_border_side(title, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_width(title, 4, 0);
    lv_obj_set_style_border_color(title, hmi_theme_accent(), 0);
    lv_obj_set_style_border_opa(title, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(title, 8, 0);

    lv_obj_t *tbox = lv_obj_create(p);
    lv_obj_set_width(tbox, LV_PCT(100));
    lv_obj_set_flex_grow(tbox, 1);
    lv_obj_set_style_bg_color(tbox, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(tbox, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tbox, 1, 0);
    lv_obj_set_style_border_color(tbox, hmi_theme_accent(), 0);
    lv_obj_set_style_border_opa(tbox, LV_OPA_50, 0);
    lv_obj_set_style_radius(tbox, 6, 0);
    lv_obj_set_style_pad_all(tbox, 6, 0);
    s_logs_tbox = tbox;

    lv_obj_t *lbl = lv_label_create(tbox);
    lv_label_set_text(lbl, "Esperando logs...");
    lv_obj_set_width(lbl, LV_PCT(100));
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x7EC8A0), 0);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    objects.sysinfo_logs_body = lbl;

    lv_obj_t *btn = lv_button_create(p);
    lv_obj_set_size(btn, LV_PCT(100), 40);
    lv_obj_set_style_shadow_opa(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btn, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    hmi_style_btn(btn, false);
    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Limpiar logs");
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(btn_lbl);
    lv_obj_add_event_cb(btn, logs_clear_cb, LV_EVENT_CLICKED, NULL);

    // Boton Logs en columna de navegacion
    lv_obj_t *logs_btn = lv_button_create(nav_col);
    s_logs_btn = logs_btn;
    lv_obj_set_size(logs_btn, LV_PCT(100), 60);
    lv_obj_set_style_shadow_opa(logs_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(logs_btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *logs_btn_lbl = lv_label_create(logs_btn);
    lv_obj_set_style_align(logs_btn_lbl, LV_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(logs_btn_lbl, &lv_font_montserrat_18, 0);
    lv_label_set_text(logs_btn_lbl, "Logs");
    hmi_style_btn(logs_btn, false);
    lv_obj_add_event_cb(logs_btn, logs_show_cb, LV_EVENT_CLICKED, NULL);

    // Mostrar logs acumulados durante el arranque
    hmi_log_refresh();
}

void hmi_log(log_type_t type, const char *msg)
{
    static const char *prefix[] = { "[OK ] ", "[ERR] ", "[RX ] ", "[TX ] ", "[WARN]" };
    snprintf(s_log_lines[s_log_head], LOG_LINE_LEN,
             "%s %s", prefix[type], msg);
    s_log_head = (s_log_head + 1) % LOG_MAX_LINES;
    if (s_log_count < LOG_MAX_LINES) s_log_count++;
    hmi_log_refresh();
}

#ifdef DEV_MODE
#endif

static uint32_t hmi_get_boot_count(void)
{
    nvs_handle_t handle;
    uint32_t count = 0;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_get_u32(handle, "boot_count", &count);
        count++;
        nvs_set_u32(handle, "boot_count", count);
        nvs_commit(handle);
        nvs_close(handle);
    }
    return count;
}

void hmi_send_data(uint8_t reg, int32_t value)
{
    hmi_tx_frame_t frame = {
        .reg = reg,
        .value = value};

    if(pdFALSE == xQueueSend(xQueueHmiTx, &frame, pdMS_TO_TICKS(10))) {
        ESP_LOGE(TAG, "xQueueHmiTx Llena");
    }
}

static inline uint8_t battery_percent(uint16_t mv)
{
    if (mv <= 12790)
        return 0;
    if (mv >= 16720)
        return 100;

    return (uint8_t)(((mv - 12790) * 100) / (16720 - 12790));
}

//*************************************************************************************
//***************************** CONFIGURACION DE HARDWARE *****************************
//*************************************************************************************

// Funciones de aplicacion
void vHardwareInit(void)
{
    // Inicializar I2C0 master
    i2c0_master_init();

    // Inicializar tactil GT911
    app_touch_init();

    // Inicializar Lcd (despues de obtener nums_fb de esp_lv_adapter)
    app_lcd_init();

    // Inicializar brillo
    lcd_brightness_init();

    // Setear el brillo a 0 (Inicialmente)
    lcd_set_brightness(0);

    // Uart Init
    vUartInit();
}

void i2c0_master_init(void)
{
    // Inicializar bus
    i2c_master_bus_config_t i2c0_bus_config = {
        .i2c_port = I2C0_I2C_PORT,
        .sda_io_num = I2C0_SDA_PIN,
        .scl_io_num = I2C0_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .flags.enable_internal_pullup = true // Si se ahorraron los pines ya son mercenarios
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c0_bus_config, &i2c0_bus_handle));
}

void lcd_brightness_init(void)
{
    // Setup LEDC peripheral for PWM backlight control
    ESP_LOGI(TAG, "Prender BackLight");
    const ledc_channel_config_t LCD_backlight_channel = {
        .gpio_num = BK_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LCD_LEDC_CH,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = 1,
        .duty = 0,
        .hpoint = 0};
    const ledc_timer_config_t LCD_backlight_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = 1,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK};

    ledc_timer_config(&LCD_backlight_timer);
    ledc_channel_config(&LCD_backlight_channel);
}

void app_lcd_init()
{
    ESP_LOGI(TAG, "Prender Alimentacion MIPI DSI PHY");
    esp_ldo_channel_config_t ldo_mipi_phy_config = {
        .chan_id = MIPI_DSI_PHY_PWR_LDO_CHAN,
        .voltage_mv = MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV,
    };
    esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy);

    ESP_LOGI(TAG, "Initialize MIPI DSI bus");
    esp_lcd_dsi_bus_config_t bus_config = ST7701_PANEL_BUS_DSI_2CH_CONFIG();
    bus_config.lane_bit_rate_mbps = 600;
    esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus);

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_dbi_io_config_t dbi_config = ST7701_PANEL_IO_DBI_CONFIG();
    esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &mipi_dbi_io);

    ESP_LOGI(TAG, "Install LCD driver of st7701");
    // https://community.nxp.com/t5/i-MX-Processors/i-MX6-MIPI-DSI-Display-ST7701-controller/m-p/839536
    esp_lcd_dpi_panel_config_t dpi_config = {
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = 27,
        .virtual_channel = 0,
        .pixel_format = MIPI_DPI_PX_FORMAT,
        .num_fbs = num_fbs,
        .video_timing = {
            .h_size = LCD_H_RES,
            .v_size = LCD_V_RES,
            .hsync_back_porch = 12,
            .hsync_front_porch = 38,
            .hsync_pulse_width = 12,
            .vsync_back_porch = 4,
            .vsync_front_porch = 18,
            .vsync_pulse_width = 8,
        },
        .flags.use_dma2d = true,
    };

    st7701_vendor_config_t vendor_config = {
        .init_cmds = lcd_cmd,
        .init_cmds_size = sizeof(lcd_cmd) / sizeof(st7701_lcd_init_cmd_t),
        .mipi_config = {
            .dsi_bus = mipi_dsi_bus,
            .dpi_config = &dpi_config,
        },
        .flags = {
            .use_mipi_interface = 1,
        }};

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = MIPI_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = MIPI_LCD_BIT_PER_PIXEL,
        .vendor_config = &vendor_config,
    };
    esp_lcd_new_panel_st7701(mipi_dbi_io, &panel_config, &panel_handle);
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_disp_on_off(panel_handle, false);
}

void lcd_set_brightness(int brightness_percent)
{
    brightness_percent = (brightness_percent > 100) ? 100 : (brightness_percent < 0) ? 0
                                                                                     : brightness_percent;

    uint32_t duty_cycle = (1023 * brightness_percent) / 100;

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LCD_LEDC_CH, duty_cycle);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LCD_LEDC_CH);
}

// Inicializar Panel Tactil
void app_touch_init()
{
    ESP_LOGI(TAG, "Inicializando I2C0 - GT911");

    esp_lcd_panel_io_handle_t tp_io_handle = NULL;

    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    tp_io_config.scl_speed_hz = I2C0_DEV_FREQ_Hz; // 400khz

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c0_bus_handle, &tp_io_config, &tp_io_handle));

    esp_lcd_touch_io_gt911_config_t tp_gt911_config = {
        .dev_addr = tp_io_config.dev_addr};

    // Registrar panel tactil
    esp_lcd_touch_config_t tp_touch_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = GPIO_NUM_NC,
        .levels = {
            .reset = 0,
            .interrupt = 0},
        .flags = {
            .swap_xy = true,
            .mirror_x = true,
            .mirror_y = false,
        },
        .driver_data = &tp_gt911_config,
    };

    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_touch_cfg, &tp_touch_handle));
}

void vUartInit(void)
{
    uart_config_t uart_config = {
        .baud_rate = HMI_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .stop_bits = UART_STOP_BITS_1,
        .parity = UART_PARITY_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
    };
    // Establecer la configuracion
    uart_param_config(HMI_UART_PORT, &uart_config);

    // Pines de UART (DEV_MODE los lee de NVS; si no hay valor usa el #define)
#ifdef DEV_MODE
    int uart_tx = dev_uart_tx_pin();
    int uart_rx = dev_uart_rx_pin();
    ESP_LOGW(TAG, "UART pines (NVS): TX=GPIO%d  RX=GPIO%d", uart_tx, uart_rx);
#else
    int uart_tx = HMI_UART_TXD;
    int uart_rx = HMI_UART_RXD;
#endif
    uart_set_pin(HMI_UART_PORT, uart_tx, uart_rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Instalar el controlador
    uart_driver_install(HMI_UART_PORT, UART_BUFFER_SIZE, UART_BUFFER_SIZE, 50, &xQueueUartEvent, 0);
}