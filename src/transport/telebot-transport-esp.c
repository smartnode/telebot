/*
 * telebot
 *
 * Copyright (c) 2026 Elmurod Talipov.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * ESP-IDF transport backend built on esp_http_client.
 *
 * esp_http_client has no multipart support, so the body comes from the
 * portable streaming encoder in telebot-multipart.c: one counting pass for
 * Content-Length, then one sending pass. Uploads never have to fit in RAM.
 * A single heap buffer of TELEBOT_IO_CHUNK_SIZE per request is used for file
 * reads and for reading the response; nothing large lives on the stack.
 */

#include <stdlib.h>
#include <string.h>

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_random.h"

#include "telebot-limits.h"
#include "telebot-multipart.h"
#include "telebot-transport.h"

typedef struct esp_transport
{
    telebot_transport_t base;
} esp_transport_t;

static bool client_write(void *ctx, const void *data, size_t len)
{
    esp_http_client_handle_t client = ctx;
    const char *p = data;
    while (len > 0)
    {
        int n = esp_http_client_write(client, p, (int)len);
        if (n <= 0)
        {
            ERR("esp_http_client_write failed (%d)", n);
            return false;
        }
        p += n;
        len -= (size_t)n;
    }
    return true;
}

static telebot_error_e esp_perform(telebot_transport_t *self, const telebot_http_request_t *req,
                                   telebot_sink_t *sink, long *http_status)
{
    (void)self;
    *http_status = 0;

    const bool is_post = req->count > 0;
    telebot_error_e ret = TELEBOT_ERROR_NONE;

    char *chunk = malloc(TELEBOT_IO_CHUNK_SIZE);
    if (chunk == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    const esp_http_client_config_t cfg = {
        .url = req->url,
        .method = is_post ? HTTP_METHOD_POST : HTTP_METHOD_GET,
        .timeout_ms = TELEBOT_HTTP_TIMEOUT_MS,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL)
    {
        free(chunk);
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    char boundary[TELEBOT_MULTIPART_BOUNDARY_SIZE];
    telebot_multipart_writer_t counter = {.write = NULL};
    if (is_post)
    {
        uint8_t rnd[8];
        esp_fill_random(rnd, sizeof(rnd));
        telebot_multipart_boundary(boundary, rnd);

        ret = telebot_multipart_encode(&counter, boundary, req->parts, req->count, chunk, TELEBOT_IO_CHUNK_SIZE);
        if (ret != TELEBOT_ERROR_NONE)
            goto cleanup; /* e.g. a file that does not exist */

        char content_type[sizeof("multipart/form-data; boundary=") + TELEBOT_MULTIPART_BOUNDARY_SIZE];
        telebot_multipart_content_type(content_type, sizeof(content_type), boundary);
        esp_http_client_set_header(client, "Content-Type", content_type);
    }

    esp_err_t err = esp_http_client_open(client, is_post ? (int)counter.total : 0);
    if (err != ESP_OK)
    {
        ERR("esp_http_client_open failed: %s", esp_err_to_name(err));
        ret = TELEBOT_ERROR_NO_CONNECTION;
        goto cleanup;
    }

    if (is_post)
    {
        telebot_multipart_writer_t writer = {.write = client_write, .ctx = client};
        ret = telebot_multipart_encode(&writer, boundary, req->parts, req->count, chunk, TELEBOT_IO_CHUNK_SIZE);
        if ((ret == TELEBOT_ERROR_NONE) && (writer.total != counter.total))
        {
            ERR("Request body changed while sending (%zu != %zu bytes)", writer.total, counter.total);
            ret = TELEBOT_ERROR_OPERATION_FAILED;
        }
        if (ret != TELEBOT_ERROR_NONE)
            goto close;
    }

    if (esp_http_client_fetch_headers(client) < 0)
    {
        ERR("Failed to read response headers");
        ret = TELEBOT_ERROR_NO_CONNECTION;
        goto close;
    }

    *http_status = esp_http_client_get_status_code(client);

    for (;;)
    {
        int n = esp_http_client_read(client, chunk, TELEBOT_IO_CHUNK_SIZE);
        if (n < 0)
        {
            ERR("Failed to read response body (%d)", n);
            ret = TELEBOT_ERROR_NO_CONNECTION;
            break;
        }
        if (n == 0)
            break;
        if (!telebot_sink_write(sink, chunk, (size_t)n))
        {
            ret = TELEBOT_ERROR_OPERATION_FAILED; /* the core reports the sink's own error */
            break;
        }
    }

close:
    esp_http_client_close(client);
cleanup:
    esp_http_client_cleanup(client);
    free(chunk);
    return ret;
}

static void esp_destroy(telebot_transport_t *self)
{
    free(TELEBOT_CONTAINER_OF(self, esp_transport_t, base));
}

static const telebot_transport_ops_t esp_transport_ops = {
    .name = "esp_http_client",
    .perform = esp_perform,
    .set_proxy = NULL, /* esp_http_client has no proxy support */
    .destroy = esp_destroy,
};

telebot_transport_t *telebot_transport_create_default(void)
{
    esp_transport_t *t = calloc(1, sizeof(*t));
    if (t == NULL)
        return NULL;
    t->base.ops = &esp_transport_ops;
    return &t->base;
}
