#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "esp_http_server.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    httpd_uri_t *endpoints;   // ponteiro para array de endpoints
    size_t count;             // quantidade de endpoints
} http_server_endpoints_t;

/**
 * @brief Inicia o HTTP Server e registra os endpoints fornecidos
 * @param endpoints Lista de endpoints
 * @return handle do servidor
 */
httpd_handle_t http_server_start(const http_server_endpoints_t *endpoints);

/**
 * @brief Para o HTTP Server
 * @param server handle do servidor
 */
void http_server_stop(httpd_handle_t server);

#ifdef __cplusplus
}
#endif

#endif // HTTP_SERVER_H