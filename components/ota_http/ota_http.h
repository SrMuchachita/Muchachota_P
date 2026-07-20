#ifndef OTA_HTTP_H
#define OTA_HTTP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Llamado desde la tarea de OTA (NO desde la tarea de render de LVGL) cuando
// se detecta una version remota distinta a la instalada. La tarea de OTA
// queda bloqueada esperando ota_http_confirm_update() antes de descargar y
// flashear — si el caller quiere mutar UI de LVGL desde este callback debe
// tomar el lock del adaptador (esp_lv_adapter_lock/unlock), igual que
// cualquier otro callback que corre fuera de la tarea de render.
typedef void (*ota_http_update_available_cb_t)(const char *new_version);

// Llamado justo antes de esp_restart() tras una descarga OTA exitosa (misma
// tarea de OTA). Pensado para apagar el backlight antes del reset: el panel
// pierde su configuracion durante el reinicio de hardware y muestra video sin
// inicializar (tipicamente un color solido) hasta que el firmware nuevo lo
// reprograma varios segundos despues — con el backlight apagado, esa ventana
// queda oculta (pantalla negra) en vez de visible.
typedef void (*ota_http_before_restart_cb_t)(void);

// Llamado justo antes de arrancar la descarga+flasheo real (esp_https_ota()).
// Pensado para apagar el backlight durante toda la descarga: escribir cada
// pagina a la flash SPI pausa brevemente AMBOS nucleos (proteccion de bajo
// nivel de ESP-IDF, no evitable por scheduling), asi que la tarea de render
// de LVGL no puede mantener el panel al dia durante esas pausas — eso se ve
// como parpadeo. Con el backlight apagado, el parpadeo queda oculto.
typedef void (*ota_http_before_download_cb_t)(void);

// Llamado si la descarga/flasheo fallo (red caida, etc.) — a diferencia del
// caso exitoso, aca el equipo NO reinicia y sigue usandose con normalidad,
// asi que hay que restaurar lo que before_download() haya cambiado (ej.
// volver a prender el backlight).
typedef void (*ota_http_download_failed_cb_t)(void);

typedef struct {
    const char *version_url;      // URL raw.githubusercontent.com al version.json
    const char *firmware_url;     // URL releases/latest/download/<NOMBRE>.bin
    uint32_t    check_interval_sec; // segundos entre chequeos
    ota_http_update_available_cb_t on_update_available; // NULL = auto-actualiza sin avisar (comportamiento viejo)
    ota_http_before_restart_cb_t   before_restart;      // NULL = no hace nada antes de reiniciar
    ota_http_before_download_cb_t  before_download;     // NULL = no hace nada antes de descargar
    ota_http_download_failed_cb_t  on_download_failed;  // NULL = no hace nada si la descarga falla
} ota_http_config_t;

// Arranca la tarea de OTA. Debe llamarse una sola vez, despues de wifi_init().
void ota_http_start(const ota_http_config_t *config);

// Llamar cuando WiFi obtiene IP (IP_EVENT_STA_GOT_IP) para despertar la tarea de OTA.
void ota_http_notify_connected(void);

// Llamar desde el boton "Actualizar" de la UI para autorizar la descarga y el
// flasheo de la version detectada en on_update_available(). No hace nada si
// no hay una actualizacion pendiente.
void ota_http_confirm_update(void);

#ifdef __cplusplus
}
#endif

#endif // OTA_HTTP_H
