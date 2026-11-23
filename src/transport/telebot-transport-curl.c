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

/* libcurl transport backend (POSIX hosts). */

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "telebot-limits.h"
#include "telebot-transport.h"

typedef struct curl_transport
{
    telebot_transport_t base;
    char *proxy_addr;
    char *proxy_auth;
} curl_transport_t;

static void secure_free(char **str)
{
    if (*str)
    {
        memset(*str, 0, strlen(*str));
        TELEBOT_SAFE_FREE(*str);
    }
}

static size_t curl_write_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t len = size * nmemb;
    return telebot_sink_write((telebot_sink_t *)userp, contents, len) ? len : 0;
}

static telebot_error_e curl_add_part(curl_mime *mime, const telebot_core_mime_t *field)
{
    curl_mimepart *part = curl_mime_addpart(mime);
    if (part == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    curl_mime_name(part, field->name);

    char scalar[TELEBOT_MIME_SCALAR_MAX];
    CURLcode rc = CURLE_OK;
    switch (field->type)
    {
    case TELEBOT_MIME_TYPE_STRING:
        rc = curl_mime_data(part, field->data.s, CURL_ZERO_TERMINATED);
        break;
    case TELEBOT_MIME_TYPE_FILE:
        rc = curl_mime_filedata(part, field->data.s);
        break;
    case TELEBOT_MIME_TYPE_BUFFER:
        rc = curl_mime_data(part, field->data.buf.ptr, field->data.buf.len);
        if (rc == CURLE_OK)
            rc = curl_mime_filename(part, field->data.buf.filename ? field->data.buf.filename : "file");
        break;
    default:
        if (telebot_mime_format_scalar(field, scalar) == NULL)
        {
            ERR("Invalid field type %d for '%s'", field->type, field->name);
            return TELEBOT_ERROR_INVALID_PARAMETER;
        }
        rc = curl_mime_data(part, scalar, CURL_ZERO_TERMINATED); /* libcurl copies the data */
        break;
    }

    if (rc != CURLE_OK)
    {
        ERR("Failed to add field '%s': %s", field->name, curl_easy_strerror(rc));
        return (rc == CURLE_READ_ERROR) ? TELEBOT_ERROR_INVALID_PARAMETER : TELEBOT_ERROR_OPERATION_FAILED;
    }
    return TELEBOT_ERROR_NONE;
}

static telebot_error_e curl_perform(telebot_transport_t *self, const telebot_http_request_t *req,
                                    telebot_sink_t *sink, long *http_status)
{
    curl_transport_t *t = TELEBOT_CONTAINER_OF(self, curl_transport_t, base);
    telebot_error_e ret = TELEBOT_ERROR_NONE;
    curl_mime *mime = NULL;

    *http_status = 0;

    CURL *curl_h = curl_easy_init();
    if (curl_h == NULL)
    {
        ERR("Failed to init curl");
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    curl_easy_setopt(curl_h, CURLOPT_URL, req->url);
    curl_easy_setopt(curl_h, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl_h, CURLOPT_WRITEDATA, sink);
    curl_easy_setopt(curl_h, CURLOPT_TIMEOUT_MS, (long)TELEBOT_HTTP_TIMEOUT_MS);
    curl_easy_setopt(curl_h, CURLOPT_NOSIGNAL, 1L);

    if (t->proxy_addr != NULL)
    {
        curl_easy_setopt(curl_h, CURLOPT_PROXY, t->proxy_addr);
        if (t->proxy_auth != NULL)
        {
            curl_easy_setopt(curl_h, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
            curl_easy_setopt(curl_h, CURLOPT_PROXYUSERPWD, t->proxy_auth);
        }
    }

    if (req->count > 0)
    {
        mime = curl_mime_init(curl_h);
        if (mime == NULL)
        {
            ret = TELEBOT_ERROR_OUT_OF_MEMORY;
            goto finish;
        }
        for (size_t i = 0; i < req->count; i++)
        {
            ret = curl_add_part(mime, &req->parts[i]);
            if (ret != TELEBOT_ERROR_NONE)
                goto finish;
        }
        curl_easy_setopt(curl_h, CURLOPT_MIMEPOST, mime);
    }

    CURLcode res = curl_easy_perform(curl_h);
    if (res != CURLE_OK)
    {
        ERR("curl_easy_perform failed: %s (%d)", curl_easy_strerror(res), res);
        ret = (res == CURLE_WRITE_ERROR) ? TELEBOT_ERROR_OUT_OF_MEMORY : TELEBOT_ERROR_NO_CONNECTION;
        goto finish;
    }

    curl_easy_getinfo(curl_h, CURLINFO_RESPONSE_CODE, http_status);

finish:
    if (mime)
        curl_mime_free(mime);
    curl_easy_cleanup(curl_h);
    return ret;
}

static telebot_error_e curl_set_proxy(telebot_transport_t *self, const char *addr, const char *auth)
{
    curl_transport_t *t = TELEBOT_CONTAINER_OF(self, curl_transport_t, base);

    char *new_addr = strdup(addr);
    char *new_auth = auth ? strdup(auth) : NULL;
    if ((new_addr == NULL) || (auth && (new_auth == NULL)))
    {
        free(new_addr);
        free(new_auth);
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    secure_free(&t->proxy_addr);
    secure_free(&t->proxy_auth);
    t->proxy_addr = new_addr;
    t->proxy_auth = new_auth;
    return TELEBOT_ERROR_NONE;
}

static void curl_destroy(telebot_transport_t *self)
{
    curl_transport_t *t = TELEBOT_CONTAINER_OF(self, curl_transport_t, base);
    secure_free(&t->proxy_addr);
    secure_free(&t->proxy_auth);
    free(t);
    curl_global_cleanup();
}

static const telebot_transport_ops_t curl_transport_ops = {
    .name = "curl",
    .perform = curl_perform,
    .set_proxy = curl_set_proxy,
    .destroy = curl_destroy,
};

telebot_transport_t *telebot_transport_create_default(void)
{
    curl_transport_t *t = calloc(1, sizeof(*t));
    if (t == NULL)
        return NULL;

    /* Reference-counted by libcurl; paired with curl_global_cleanup() in destroy. */
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
    {
        free(t);
        return NULL;
    }

    t->base.ops = &curl_transport_ops;
    return &t->base;
}
