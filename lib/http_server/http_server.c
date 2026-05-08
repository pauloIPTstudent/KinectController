#include "http_server.h"
#include "esp_log.h"

static const char *TAG = "http_server";

httpd_handle_t http_server_start(const http_server_endpoints_t *endpoints)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_LOGI(TAG, "Starting HTTP Server");

    if (httpd_start(&server, &config) == ESP_OK)
    {
        if (endpoints && endpoints->endpoints && endpoints->count > 0)
        {
            for (size_t i = 0; i < endpoints->count; i++)
            {
                httpd_register_uri_handler(server, &endpoints->endpoints[i]);
                ESP_LOGI(TAG, "Registered endpoint: %s", endpoints->endpoints[i].uri);
            }
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to start HTTP Server");
    }

    return server;
}

void http_server_stop(httpd_handle_t server)
{
    if (server)
    {
        httpd_stop(server);
        ESP_LOGI(TAG, "HTTP Server stopped");
    }
}