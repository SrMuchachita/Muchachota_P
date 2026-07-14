#ifndef OTA_HTTP_H
#define OTA_HTTP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *version_url;      // URL raw.githubusercontent.com al version.json
    const char *firmware_url;     // URL releases/latest/download/<NOMBRE>.bin
    uint32_t    check_interval_sec; // segundos entre chequeos
} ota_http_config_t;

// Arranca la tarea de OTA. Debe llamarse una sola vez, despues de wifi_init().
void ota_http_start(const ota_http_config_t *config);

// Llamar cuando WiFi obtiene IP (IP_EVENT_STA_GOT_IP) para despertar la tarea de OTA.
void ota_http_notify_connected(void);

#ifdef __cplusplus
}
#endif

#endif // OTA_HTTP_H
