#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"
#include "esp_app_desc.h"
#include "esp_ota_ops.h"

#include "ota_http.h"

static const char *TAG = "ota_http";

static ota_http_config_t s_cfg;
static SemaphoreHandle_t s_wifi_connected_sem = NULL;
static bool s_started = false;

#define OTA_VERSION_BUF_LEN 128

typedef struct {
    char   buf[OTA_VERSION_BUF_LEN];
    size_t len;
} version_fetch_ctx_t;

static esp_err_t version_http_event_handler(esp_http_client_event_t *evt)
{
    version_fetch_ctx_t *ctx = (version_fetch_ctx_t *)evt->user_data;

    if (evt->event_id == HTTP_EVENT_ON_DATA && ctx != NULL) {
        size_t room = OTA_VERSION_BUF_LEN - 1 - ctx->len;
        size_t to_copy = (size_t)evt->data_len < room ? (size_t)evt->data_len : room;
        if (to_copy > 0) {
            memcpy(ctx->buf + ctx->len, evt->data, to_copy);
            ctx->len += to_copy;
            ctx->buf[ctx->len] = '\0';
        }
    }
    return ESP_OK;
}

// Busca "version":"x.x.x" a mano dentro del JSON descargado.
static esp_err_t parse_version_field(const char *json, char *out, size_t out_len)
{
    const char *key = strstr(json, "\"version\"");
    if (!key) return ESP_FAIL;

    const char *colon = strchr(key, ':');
    if (!colon) return ESP_FAIL;

    const char *quote1 = strchr(colon, '"');
    if (!quote1) return ESP_FAIL;
    quote1++;

    const char *quote2 = strchr(quote1, '"');
    if (!quote2) return ESP_FAIL;

    size_t len = (size_t)(quote2 - quote1);
    if (len == 0 || len >= out_len) return ESP_FAIL;

    memcpy(out, quote1, len);
    out[len] = '\0';
    return ESP_OK;
}

static esp_err_t fetch_remote_version(const char *version_url, char *out_version, size_t out_len)
{
    version_fetch_ctx_t ctx = { .len = 0 };
    ctx.buf[0] = '\0';

    esp_http_client_config_t http_cfg = {
        .url                   = version_url,
        .crt_bundle_attach     = esp_crt_bundle_attach,
        .max_redirection_count = 5,
        .timeout_ms            = 10000,
        .event_handler         = version_http_event_handler,
        .user_data             = &ctx,
    };

    esp_http_client_handle_t client = esp_http_client_init(&http_cfg);
    if (!client) {
        ESP_LOGE(TAG, "No se pudo crear cliente HTTP para version.json");
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error consultando version.json: %s", esp_err_to_name(err));
        return err;
    }
    if (status != 200) {
        ESP_LOGE(TAG, "version.json respondio HTTP %d", status);
        return ESP_FAIL;
    }

    return parse_version_field(ctx.buf, out_version, out_len);
}

static esp_err_t download_and_flash(const char *firmware_url)
{
    ESP_LOGW(TAG, "Descargando nuevo firmware: %s", firmware_url);

    esp_http_client_config_t http_cfg = {
        .url                   = firmware_url,
        .crt_bundle_attach     = esp_crt_bundle_attach,
        .max_redirection_count = 5,
        .keep_alive_enable     = true,
        .buffer_size           = 8192,
        .buffer_size_tx        = 4096,
        .timeout_ms            = 30000,
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    esp_err_t err = esp_https_ota(&ota_cfg);
    if (err == ESP_OK) {
        ESP_LOGW(TAG, "OTA OK, reiniciando...");
        esp_restart();
        return ESP_OK; // no se llega aqui
    }

    ESP_LOGE(TAG, "OTA fallo: %s", esp_err_to_name(err));
    return err;
}

static void ota_task(void *arg)
{
    (void)arg;

    // Espera a que WiFi este conectado antes del primer chequeo.
    xSemaphoreTake(s_wifi_connected_sem, portMAX_DELAY);

    const esp_app_desc_t *app_desc = esp_app_get_description();

    while (1) {
        char remote_version[OTA_VERSION_BUF_LEN];

        if (fetch_remote_version(s_cfg.version_url, remote_version, sizeof(remote_version)) == ESP_OK) {
            ESP_LOGI(TAG, "Version local: %s | Version remota: %s", app_desc->version, remote_version);

            if (strcmp(app_desc->version, remote_version) != 0) {
                download_and_flash(s_cfg.firmware_url);
                // Si download_and_flash tuvo exito, el dispositivo ya reinicio.
            } else {
                ESP_LOGI(TAG, "Firmware al dia");
            }
        } else {
            ESP_LOGW(TAG, "No se pudo consultar la version remota");
        }

        vTaskDelay(pdMS_TO_TICKS(s_cfg.check_interval_sec * 1000UL));

        // A partir del segundo chequeo no hace falta esperar el semaforo:
        // si ya se tomo una vez, seguimos re-chequeando en el intervalo configurado.
    }
}

void ota_http_start(const ota_http_config_t *config)
{
    // Idempotente: llamarla mas de una vez (p.ej. al reactivar el WiFi
    // despues de apagarlo desde la UI) no debe crear una segunda ota_task.
    if (s_started) return;
    s_started = true;

    s_cfg = *config;

    if (!s_wifi_connected_sem) {
        s_wifi_connected_sem = xSemaphoreCreateBinary();
    }

    xTaskCreate(ota_task, "ota_task", 8192, NULL, 5, NULL);
}

void ota_http_notify_connected(void)
{
    if (s_wifi_connected_sem) {
        xSemaphoreGive(s_wifi_connected_sem);
    }
}
