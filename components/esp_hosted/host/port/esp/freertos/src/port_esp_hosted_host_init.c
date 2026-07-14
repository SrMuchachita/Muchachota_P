/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_hosted.h"

#include "esp_private/startup_internal.h"

// NOTA (fork local, ver main/idf_component.yml override_path): el
// constructor de C original corria esp_hosted_init() incondicionalmente
// antes de app_main(), sin importar si el WiFi llegaba a usarse — hace un
// handshake SDIO bloqueante (~1.5s x2 reintentos) con el coprocesador
// ESP32-C6 que sumaba costo fijo al arranque de una app cuyo WiFi arranca
// apagado por defecto. Se desactivo el constructor: esp_hosted_init() sigue
// siendo publica (esp_hosted.h) e idempotente (guardada por
// esp_hosted_init_done), y ahora se llama explicitamente desde
// main/main.c::wifi_init() la primera vez que el usuario activa el WiFi.
//ESP_SYSTEM_INIT_FN(esp_hosted_host_init, BIT(0), 120)
#if 0
static void __attribute__((constructor)) esp_hosted_host_init(void)
{
	ESP_LOGI(TAG, "ESP Hosted : Host chip_ip[%d]", CONFIG_IDF_FIRMWARE_CHIP_ID);
	ESP_ERROR_CHECK(esp_hosted_init());
}

static void __attribute__((destructor)) esp_hosted_host_deinit(void)
{
	ESP_LOGI(TAG, "ESP Hosted deinit");
	esp_hosted_deinit();
}
#endif
