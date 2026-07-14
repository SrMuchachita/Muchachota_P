// ANSI C
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>

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

// WiFi / OTA
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_hosted.h"
#include "ota_http.h"
#include "ui/lock_logos.h" // logotipo/isotipo Welltepp para pantalla de bloqueo
#include "main.h"


//*****************************************RECURSOS DE MODULO MAIN************************************************/
#define TASK_SIZE (6 * 1024) // KB
#define TAG "RD90Hmio"


//******************************************** SERIAL NUMBER ****************************************************************** */
#define SERIAL_NUMBER     "260005" // 6CARACTERES

//******************************************** VERSION INFO ****************************************************************** */
#define FW_VERSION        "v1.2.1"  // Version de firmware
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
static void settings_nav_enable_scroll(void);
static void hmi_wifi_set_enabled(bool enable);
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

#define OTA_VERSION_URL  "https://raw.githubusercontent.com/SrMuchachita/Muchachota_P/main/version.json"
#define OTA_FIRMWARE_URL "https://github.com/SrMuchachita/Muchachota_P/releases/latest/download/WP_P_V9_UP.bin"
#define OTA_CHECK 20 // segundos entre chequeos de version

// Estado del panel "Update" (System Info): WiFi apagado por defecto al
// arrancar, se prende/apaga a mano desde la UI. No se persiste en NVS.
// Los widgets (objects.update_led/update_status_label/update_toggle_btn) se
// crean en ui/screens.c junto con el resto de System Info.
static bool s_wifi_initialized = false; // wifi_init() ya corrio alguna vez
static bool s_wifi_enabled     = false; // estado logico actual (on/off)

// Sin lock: se llama tanto desde contexto LVGL (click del boton toggle) como,
// via HMI_LV_LOCKED, desde wifi_event_handler (tarea del event loop de ESP-IDF).
static void update_panel_set_status(const char *text, bool led_on)
{
    if (objects.update_status_label) lv_label_set_text(objects.update_status_label, text);
    if (objects.update_led) lv_led_set_brightness(objects.update_led, led_on ? 255 : 0);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        HMI_LV_LOCKED(update_panel_set_status("Conectando...", false));
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // Si el usuario apago el WiFi a proposito (hmi_wifi_set_enabled(false)
        // ya hizo esp_wifi_disconnect()), no reintentar ni pisar el estado "apagado".
        if (s_wifi_enabled) {
            ESP_LOGW(TAG, "WiFi desconectado, reintentando...");
            esp_wifi_connect();
            HMI_LV_LOCKED(update_panel_set_status("Reconectando...", false));
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi conectado, IP: " IPSTR, IP2STR(&event->ip_info.ip));
        ota_http_notify_connected();
        char buf[48];
        snprintf(buf, sizeof(buf), "Conectado: " IPSTR, IP2STR(&event->ip_info.ip));
        HMI_LV_LOCKED(update_panel_set_status(buf, true));
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

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
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

    // Colas
    xQueueHmiTx = xQueueCreate(50, sizeof(hmi_tx_frame_t));
    if (xQueueHmiTx == NULL)
    {
        // La cola no se creó porque no había suficiente memoria en el heap de FreeRTOS
        ESP_LOGE(TAG, "Error al crear xQueueHmiTx");
        return;
    }

    xQueueHmiRx = xQueueCreate(50, sizeof(hmi_rx_frame_t));
    if (xQueueHmiRx == NULL)
    {
        // La cola no se creó porque no había suficiente memoria en el heap de FreeRTOS
        ESP_LOGE(TAG, "Error al crear xQueueHmiRx");
        return;
    }

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

    // Creacion de tareas
    xTaskCreate(vTaskHmiTransmit,   "HMI Tx",    TASK_SIZE, NULL, 5, NULL);
    xTaskCreate(vTaskUartHmiEvents, "HMI Event", TASK_SIZE, NULL, 5, NULL);
    xTaskCreate(vTaskHmiRxProcess,  "HMI Rx",   TASK_SIZE, NULL, 6, NULL);
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
                row = lv_obj_get_child(panel, 1); if (row) lv_label_set_text(lv_obj_get_child(row, 1), FW_VERSION);
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
#define NVS_KEY_ENC_PERIM_CX100 "enc_perim"   // perimetro del rodillo, centesimas de mm
#define NVS_KEY_ENC_PPR         "enc_ppr"     // pulsos por vuelta del encoder
#define NVS_KEY_ENC_MODE_METERS "enc_mode_m"  // 0=Pulsos, 1=Metros (toggle de GENERAL CONTROLS)
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
        if (!s_wifi_initialized) {
            wifi_init();
            s_wifi_initialized = true;

            ota_http_config_t ota_cfg = {
                .version_url        = OTA_VERSION_URL,
                .firmware_url       = OTA_FIRMWARE_URL,
                .check_interval_sec = OTA_CHECK,
            };
            ota_http_start(&ota_cfg);
        } else {
            esp_wifi_start();
        }
        update_panel_set_status("Conectando...", false);
    } else {
        if (s_wifi_initialized) {
            esp_wifi_disconnect();
            esp_wifi_stop();
        }
        update_panel_set_status("WiFi apagado", false);
    }

    if (objects.update_toggle_btn) {
        lv_obj_t *lbl = lv_obj_get_child(objects.update_toggle_btn, 0);
        if (lbl) lv_label_set_text(lbl, s_wifi_enabled ? "Desactivar WiFi" : "Activar WiFi");
        hmi_style_btn(objects.update_toggle_btn, s_wifi_enabled);
    }
}

void hmi_update_panel_retheme(void)
{
    if (objects.update_toggle_btn) hmi_style_btn(objects.update_toggle_btn, s_wifi_enabled);
}

void update_wifi_toggle_cb(lv_event_t *e)
{
    (void)e;
    hmi_wifi_set_enabled(!s_wifi_enabled);
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
    snprintf(buf, sizeof(buf),
             "1 pulso = %.3f mm  \xC2\xB7  1 m = %.1f pulsos  \xC2\xB7  Formula: dist = pulsos x %.2f / (%ld x 1000)",
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
// GENERAL CONTROLS > tarjeta ENCODER — toggle Pulsos/Metros usando la calibracion
// guardada en Settings > Encoder (mismo patron que el toggle Voltaje/Porcentaje
// de bateria: g_bat_display_percent + apply_btn_style).
//*************************************************************************************
static bool     s_encoder_show_meters   = false;
static int32_t  s_encoder_last_raw      = 0;
static lv_obj_t *s_encoder_btn_pulses   = NULL;
static lv_obj_t *s_encoder_btn_meters   = NULL;

static void encoder_dashboard_refresh(void)
{
    if (!objects.encoder_value) return;

    if (s_encoder_show_meters) {
        int32_t perim_cx100 = dev_nvs_read_pin(NVS_KEY_ENC_PERIM_CX100, ENC_PERIM_CX100_DEF);
        int32_t ppr         = dev_nvs_read_pin(NVS_KEY_ENC_PPR, ENC_PPR_DEF);
        float perim_mm = perim_cx100 / 100.0f;
        float dist_m = (ppr > 0) ? ((float)s_encoder_last_raw * perim_mm) / ((float)ppr * 1000.0f) : 0.0f;
        char buf[24];
        snprintf(buf, sizeof(buf), "%.3f m", dist_m);
        lv_label_set_text(objects.encoder_value, buf);
    } else {
        char buf[20];
        snprintf(buf, sizeof(buf), "%06ld", (long)s_encoder_last_raw);
        set_var_encoder(buf);
        lv_label_set_text_static(objects.encoder_value, get_var_encoder());
    }
}

void hmi_encoder_set_raw(int32_t raw_pulses)
{
    s_encoder_last_raw = raw_pulses;
    encoder_dashboard_refresh();
}

// Aplica el modo (UI + estado) sin tocar NVS — usado para restaurar el valor
// guardado al arrancar, sin pisarlo con el default.
static void encoder_mode_apply(bool meters)
{
    s_encoder_show_meters = meters;
    hmi_style_btn(s_encoder_btn_pulses, !meters);
    hmi_style_btn(s_encoder_btn_meters,  meters);
    encoder_dashboard_refresh();
}

// Elegido por el usuario (toque en PULSOS/METROS): persiste en NVS y aplica.
static void encoder_mode_set(bool meters)
{
    dev_nvs_write_pin(NVS_KEY_ENC_MODE_METERS, meters ? 1 : 0);
    encoder_mode_apply(meters);
}
static void encoder_mode_pulses_cb(lv_event_t *e) { (void)e; encoder_mode_set(false); }
static void encoder_mode_meters_cb(lv_event_t *e) { (void)e; encoder_mode_set(true); }

// Se engancha a la tarjeta ENCODER ya creada por screens.c en GENERAL CONTROLS
// (objects.obj17), en el hueco libre entre el valor grande y el boton RESET.
static void encoder_display_toggle_create(void)
{
    if (!objects.obj17) return;

    lv_obj_t *btn_p = lv_button_create(objects.obj17);
    s_encoder_btn_pulses = btn_p;
    lv_obj_set_pos(btn_p, 320, 40);
    lv_obj_set_size(btn_p, 90, 22);
    lv_obj_set_style_radius(btn_p, 6, 0);
    lv_obj_set_style_shadow_opa(btn_p, 0, 0);
    lv_obj_t *lbl_p = lv_label_create(btn_p);
    lv_obj_set_style_align(lbl_p, LV_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl_p, &lv_font_montserrat_12, 0);
    lv_label_set_text(lbl_p, "PULSOS");
    lv_obj_add_event_cb(btn_p, encoder_mode_pulses_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_m = lv_button_create(objects.obj17);
    s_encoder_btn_meters = btn_m;
    lv_obj_set_pos(btn_m, 320, 68);
    lv_obj_set_size(btn_m, 90, 22);
    lv_obj_set_style_radius(btn_m, 6, 0);
    lv_obj_set_style_shadow_opa(btn_m, 0, 0);
    lv_obj_t *lbl_m = lv_label_create(btn_m);
    lv_obj_set_style_align(lbl_m, LV_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl_m, &lv_font_montserrat_12, 0);
    lv_label_set_text(lbl_m, "METROS");
    lv_obj_add_event_cb(btn_m, encoder_mode_meters_cb, LV_EVENT_CLICKED, NULL);

    // Restaurar el modo guardado en NVS (default: pulsos, igual que el label
    // original "0000000") sin volver a escribirlo — encoder_mode_apply() no
    // toca NVS, a diferencia de encoder_mode_set() (que es solo para cuando
    // el usuario toca un boton).
    bool saved_meters = dev_nvs_read_pin(NVS_KEY_ENC_MODE_METERS, 0) != 0;
    encoder_mode_apply(saved_meters);
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
    lv_label_set_text(title, "NUEVO PIN DE BLOQUEO");
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
    lv_label_set_text(hint, "4 digitos");
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
    lv_label_set_text(clbl, "Cancelar");
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
        ls_set_status("Acceso concedido", lv_color_hex(LS_COL_OK));
        lv_timer_t *t = lv_timer_create(ls_unlock_timer_cb, 550, NULL);
        lv_timer_set_repeat_count(t, 1);
    } else {
        ls_dots_color(lv_color_hex(LS_COL_ERR));
        ls_set_card_state(2);
        ls_set_status("PIN incorrecto", lv_color_hex(LS_COL_ERR));
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
        snprintf(greet_msg, sizeof(greet_msg), "HOLA %s", greet_name);

        lv_obj_t *greet = lv_label_create(panel);
        lv_label_set_text(greet, greet_msg);
        lv_obj_set_style_text_font(greet, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(greet, lv_color_hex(LS_COL_TXT), 0);
        lv_obj_set_style_text_align(greet, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(greet, LV_ALIGN_CENTER, 0, 30);

        lv_obj_t *sub = lv_label_create(panel);
        lv_label_set_text(sub, "Ingresando...");
        lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(sub, lv_color_hex(LS_COL_TXT_DIM), 0);
        lv_obj_align(sub, LV_ALIGN_CENTER, 0, 60);

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
    lv_label_set_text(hint, "Toca para ingresar");
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
        }
        HMI_LV_SAFE_OBJ(objects.led_online, lv_led_set_brightness(objects.led_online, (uint8_t)value));
        ESP_LOGI(TAG, "HMI_REG_ONLINE_INDICATOR: %d", value);
        break;
    }

    case HMI_REG_BLUETOOTH_INDICATOR:
    {
        HMI_LV_SAFE_OBJ(objects.led_bluetooth, lv_led_set_brightness(objects.led_bluetooth, (uint8_t)value));
        ESP_LOGI(TAG, "HMI_REG_BLUETOOTH_INDICATOR: %d", (uint8_t)value);
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
                    lv_obj_set_style_border_color(objects.obj6, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(objects.obj6, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
                if (objects.obj0 != NULL) {
                    lv_obj_set_style_bg_color(objects.obj0, lv_color_hex(0xff393939), LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
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
        ESP_LOGI(TAG, "HMI_REG_ANGLE_Y: %d", value);
        break;
    }

    case HMI_REG_BLUETOOTH_PASSWORD:
    {
        char buff[10];
        snprintf(buff, sizeof(buff), "%06ld", value);
        set_var_bluetooth_password_string(buff);

        HMI_LV_SAFE_OBJ(objects.bluetooth_password_label, lv_label_set_text_static(objects.bluetooth_password_label, get_var_bluetooth_password_string()));
        ESP_LOGI(TAG, "HMI_REG_BLUETOOTH_PASSWORD: %06d", value);
        break;
    }

    case HMI_REG_ROBOT_SERIAL:
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

        ESP_LOGI(TAG, "HMI_REG_ROBOT_SERIAL: %06ld", value);
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
        ESP_LOGI(TAG, "HMI_REG_MOTOR: cmd=%d vel=%d",
                 (int)((value >> 16) & 0xFFFF), (int)(value & 0xFFFF));
#ifdef DEV_MODE
        s_motor_cmd = (int16_t)((value >> 16) & 0xFFFF);
        s_motor_vel = (int16_t)(value & 0xFFFF);
#endif
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
    if (mv <= 12000)
        return 0;
    if (mv >= 16800)
        return 100;

    return (uint8_t)(((mv - 12000) * 100) / (16800 - 12000));
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