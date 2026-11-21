/*
 * telebot
 *
 * Copyright (c) 2015 Elmurod Talipov.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "telebot-json.h"
#include <telebot-common.h>
#include <telebot-core.h>
#include <telebot-private.h>
#include "telebot-config.h"
#include "telebot-transport.h"
#include "telebot-request.h"
#include <unistd.h>

telebot_error_e telebot_core_get_response_code(telebot_core_response_t response)
{
    if (response)
        return response->ret;
    else
        // response is NULL, probably could not allocate memory
        return TELEBOT_ERROR_OUT_OF_MEMORY;
}

int telebot_core_get_response_retry_after(telebot_core_response_t response)
{
    return response ? response->retry_after : 0;
}

int telebot_core_get_retry_after(telebot_core_handler_t core_h)
{
    return core_h ? core_h->last_retry_after : 0;
}

const char *telebot_core_get_response_data(telebot_core_response_t response)
{
    if (response)
        return response->data;
    else
        return NULL;
}

void telebot_core_put_response(telebot_core_response_t response)
{
    if (response)
    {
        TELEBOT_SAFE_FZCNT(response->data, response->size);
        TELEBOT_SAFE_FREE(response);
    }
}

telebot_error_e
telebot_core_create(telebot_core_handler_t *core_h, const char *token)
{
    if ((core_h == NULL) || (token == NULL))
    {
        ERR("Either pointer for core handler (%p) or token (%p) is null", core_h, token);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    *core_h = NULL;

    telebot_core_handler_t _core_h = malloc(sizeof(struct telebot_core_handler));
    if (_core_h == NULL)
    {
        ERR("Failed to allocate memory");
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    _core_h->token = strdup(token);
    if (_core_h->token == NULL)
    {
        ERR("Failed to allocate memory for token");
        TELEBOT_SAFE_FREE(_core_h);
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    _core_h->proxy_addr = NULL;
    _core_h->proxy_auth = NULL;

    _core_h->transport = telebot_transport_create_default();
    if (_core_h->transport == NULL)
    {
        ERR("Failed to create HTTP transport");
        memset(_core_h->token, 0xFF, strlen(_core_h->token));
        TELEBOT_SAFE_FREE(_core_h->token);
        TELEBOT_SAFE_FREE(_core_h);
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    *core_h = _core_h;
    return TELEBOT_ERROR_NONE;
}

telebot_error_e
telebot_core_destroy(telebot_core_handler_t *core_h)
{
    if ((core_h == NULL) || (*core_h == NULL))
    {
        ERR("Core handler is null");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    telebot_transport_destroy((*core_h)->transport);
    (*core_h)->transport = NULL;

    if ((*core_h)->token != NULL)
    {
        memset((*core_h)->token, 0xFF, strlen((*core_h)->token));
        TELEBOT_SAFE_FREE((*core_h)->token);
    }

    if ((*core_h)->proxy_addr != NULL)
    {
        memset((*core_h)->proxy_addr, 0xFF, strlen((*core_h)->proxy_addr));
        TELEBOT_SAFE_FREE((*core_h)->proxy_addr);
    }

    if ((*core_h)->proxy_auth != NULL)
    {
        memset((*core_h)->proxy_auth, 0xFF, strlen((*core_h)->proxy_auth));
        TELEBOT_SAFE_FREE((*core_h)->proxy_auth);
    }

    TELEBOT_SAFE_FREE(*core_h);
    return TELEBOT_ERROR_NONE;
}

telebot_error_e
telebot_core_set_proxy(telebot_core_handler_t core_h, const char *addr, const char *auth)
{
    if ((addr == NULL) || (core_h == NULL))
    {
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    telebot_error_e ret = telebot_transport_set_proxy(core_h->transport, addr, auth);
    if (ret != TELEBOT_ERROR_NONE)
    {
        ERR("Transport '%s' could not set proxy", core_h->transport->ops->name);
        return ret;
    }

    /* Keep a copy of the address for telebot_core_get_proxy() */
    char *copy = strdup(addr);
    if (copy == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    TELEBOT_SAFE_FREE(core_h->proxy_addr);
    core_h->proxy_addr = copy;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e
telebot_core_get_proxy(telebot_core_handler_t core_h, char **addr)
{
    if ((addr == NULL) || (core_h == NULL))
    {
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (core_h->proxy_addr)
        *addr = strdup(core_h->proxy_addr);
    else
        *addr = NULL;

    return TELEBOT_ERROR_NONE;
}

void telebot_core_set_transport(telebot_core_handler_t core_h, telebot_transport_t *transport)
{
    if ((core_h == NULL) || (transport == NULL))
        return;
    telebot_transport_destroy(core_h->transport);
    core_h->transport = transport;
}

static telebot_core_response_t
telebot_core_perform(telebot_core_handler_t core_h, const char *method, const telebot_core_mime_t mimes[], size_t size)
{
    telebot_core_response_t resp = calloc(1, sizeof(struct telebot_core_response));
    if (resp == NULL)
    {
        ERR("Failed to allocate memory for response");
        return NULL;
    }

    if ((core_h == NULL) || (core_h->transport == NULL))
    {
        ERR("Core handler is NULL");
        resp->ret = TELEBOT_ERROR_INVALID_PARAMETER;
        return resp;
    }

    if (core_h->token == NULL)
    {
        ERR("Token is NULL, this should not happen");
        resp->ret = TELEBOT_ERROR_OPERATION_FAILED;
        return resp;
    }

    char url[TELEBOT_URL_SIZE];
    int n = snprintf(url, sizeof(url), "%s/bot%s/%s", TELEBOT_API_URL, core_h->token, method);
    if ((n < 0) || ((size_t)n >= sizeof(url)))
    {
        ERR("URL for method '%s' is too long", method);
        resp->ret = TELEBOT_ERROR_INVALID_PARAMETER;
        return resp;
    }

    telebot_memory_sink_t sink;
    telebot_memory_sink_init(&sink, TELEBOT_MAX_RESPONSE_SIZE);

    const telebot_http_request_t req = {.url = url, .parts = mimes, .count = size};
    long status = 0;
    resp->ret = telebot_transport_perform(core_h->transport, &req, &sink.base, &status);
    if ((resp->ret != TELEBOT_ERROR_NONE) && (sink.error != TELEBOT_ERROR_NONE))
        resp->ret = sink.error;

    /*
     * Keep the body even on HTTP errors: Telegram explains failures in it,
     * e.g. {"ok":false,"error_code":400,"description":"Bad Request: ..."}.
     */
    resp->data = telebot_memory_sink_take(&sink, &resp->size);

    if ((resp->ret == TELEBOT_ERROR_NONE) && (status != 200L))
    {
        ERR("%s failed with HTTP %ld: %s", method, status, resp->data ? resp->data : "(no body)");
        resp->ret = TELEBOT_ERROR_OPERATION_FAILED;
        if (status == 429L)
        {
            /* {"ok":false,"error_code":429,"parameters":{"retry_after":N}} */
            resp->ret = TELEBOT_ERROR_RATE_LIMITED;
            cJSON *root = resp->data ? cJSON_Parse(resp->data) : NULL;
            cJSON *params = NULL, *retry = NULL;
            if (tb_json_get_ex(root, "parameters", &params) && tb_json_get_ex(params, "retry_after", &retry))
                resp->retry_after = (int)tb_json_get_int(retry);
            tb_json_put(root);
        }
    }
    else
    {
        DBG("Response: %s", resp->data ? resp->data : "");
    }
    core_h->last_retry_after = resp->retry_after;

    return resp;
}

telebot_core_response_t
telebot_core_call(telebot_core_handler_t core_h, const char *method, const telebot_param_t params[], size_t count)
{
    if ((method == NULL) || (method[0] == '\0') || ((count > 0) && (params == NULL)))
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);

    telebot_request_t req;
    telebot_request_init(&req);
    for (size_t i = 0; i < count; i++)
    {
        const telebot_param_t *p = &params[i];
        if ((p->name == NULL) || ((p->value == NULL) == (p->file == NULL)))
        {
            ERR("Parameter %zu must have a name and exactly one of value or file", i);
            return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
        }
        if (p->file != NULL)
            tb_req_file(&req, p->name, p->file);
        else
            tb_req_str(&req, p->name, p->value);
    }
    return telebot_core_request(core_h, method, &req);
}

telebot_core_response_t
telebot_core_get_updates(telebot_core_handler_t core_h, int offset, int limit, int timeout, const char *allowed_updates)
{
    if (limit > TELEBOT_UPDATE_COUNT_MAX_LIMIT)
        limit = TELEBOT_UPDATE_COUNT_MAX_LIMIT;

    size_t count = 0;
    telebot_core_mime_t mimes[4]; // number of arguments
    mimes[count].name = "offset";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = offset;
    count++;

    mimes[count].name = "limit";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = limit;
    count++;

    mimes[count].name = "timeout";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = timeout;
    count++;

    if (allowed_updates)
    {
        mimes[count].name = "allowed_updates";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = allowed_updates;
        count++;
    }

    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_UPDATES, mimes, count);
}

telebot_core_response_t telebot_core_error_response(telebot_error_e ret)
{
    telebot_core_response_t resp = calloc(1, sizeof(struct telebot_core_response));
    if (resp)
        resp->ret = ret;

    return resp;
}

telebot_core_response_t telebot_core_request(telebot_core_handler_t core_h, const char *method,
                                             const telebot_request_t *req)
{
    if (req->error != TELEBOT_ERROR_NONE)
        return telebot_core_error_response(req->error);
    return telebot_core_perform(core_h, method, req->parts, req->count);
}

telebot_core_response_t
telebot_core_set_webhook(telebot_core_handler_t core_h, const char *url, const char *certificate, int max_connections,
                         const char *allowed_updates)
{
    CHECK_ARG_NULL(url);

    size_t count = 0;
    telebot_core_mime_t mimes[4]; // number of arguments
    mimes[count].name = "url";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = url;
    ++count;

    if (certificate != NULL)
    {
        /* The public key certificate is an InputFile, so it is uploaded */
        mimes[count].name = "certificate";
        mimes[count].type = TELEBOT_MIME_TYPE_FILE;
        mimes[count].data.s = certificate;
        count++;
    }

    if (max_connections > 0)
    {
        mimes[count].name = "max_connections";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = max_connections;
        count++;
    }

    if (allowed_updates != NULL)
    {
        mimes[count].name = "allowed_updates";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = allowed_updates;
        count++;
    }

    return telebot_core_perform(core_h, TELEBOT_METHOD_SET_WEBHOOK, mimes, count);
}

telebot_core_response_t
telebot_core_delete_webhook(telebot_core_handler_t core_h)
{
    return telebot_core_perform(core_h, TELEBOT_METHOD_DELETE_WEBHOOK, NULL, 0);
}

telebot_core_response_t
telebot_core_get_webhook_info(telebot_core_handler_t core_h)
{
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_WEBHOOK_INFO, NULL, 0);
}

telebot_core_response_t
telebot_core_get_me(telebot_core_handler_t core_h)
{
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_ME, NULL, 0);
}

telebot_core_response_t
telebot_core_edit_message_live_location(telebot_core_handler_t core_h, long long int chat_id, int message_id,
                                        const char *inline_message_id, float latitude, float longitude,
                                        const char *reply_markup)
{
    int count = 0;
    telebot_core_mime_t mimes[6]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "latitude";
    mimes[count].type = TELEBOT_MIME_TYPE_FLOAT;
    mimes[count].data.f = latitude;
    count++;

    mimes[count].name = "longitude";
    mimes[count].type = TELEBOT_MIME_TYPE_FLOAT;
    mimes[count].data.f = longitude;
    count++;

    if (message_id > 0)
    {
        mimes[count].name = "message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = message_id;
        count++;
    }

    if (inline_message_id != NULL)
    {
        mimes[count].name = "inline_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = inline_message_id;
        count++;
    }

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_perform(core_h, TELEBOT_METHOD_EDIT_MESSAGE_LIVE_LOCATION, mimes, count);
}

telebot_core_response_t
telebot_core_stop_message_live_location(telebot_core_handler_t core_h, long long int chat_id, int message_id,
                                        char *inline_message_id, const char *reply_markup)
{
    int count = 0;
    telebot_core_mime_t mimes[4]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    if (message_id > 0)
    {
        mimes[count].name = "message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = message_id;
        count++;
    }

    if (inline_message_id != NULL)
    {
        mimes[count].name = "inline_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = inline_message_id;
        count++;
    }

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_perform(core_h, TELEBOT_METHOD_STOP_MESSAGE_LIVE_LOCATION, mimes, count);
}

telebot_core_response_t
telebot_core_get_file(telebot_core_handler_t core_h, const char *file_id)
{
    CHECK_ARG_NULL(file_id);

    int count = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[count].name = "file_id";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = file_id;
    count++;

    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_FILE, mimes, count);
}

telebot_error_e
telebot_core_download_file(telebot_core_handler_t core_h, const char *file_path, const char *out_file)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (core_h->transport == NULL) ||
        (file_path == NULL) || (out_file == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    char url[TELEBOT_URL_SIZE];
    int n = snprintf(url, sizeof(url), "%s/file/bot%s/%s", TELEBOT_API_URL, core_h->token, file_path);
    if ((n < 0) || ((size_t)n >= sizeof(url)))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    FILE *fp = fopen(out_file, "wb");
    if (fp == NULL)
    {
        ERR("Failed to open '%s' for writing", out_file);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    telebot_file_sink_t sink;
    telebot_file_sink_init(&sink, fp);

    const telebot_http_request_t req = {.url = url, .parts = NULL, .count = 0};
    long status = 0;
    telebot_error_e ret = telebot_transport_perform(core_h->transport, &req, &sink.base, &status);
    if ((ret == TELEBOT_ERROR_NONE) && (status != 200L))
    {
        ERR("Download failed with HTTP %ld", status);
        ret = TELEBOT_ERROR_OPERATION_FAILED;
    }

    if ((fclose(fp) != 0) && (ret == TELEBOT_ERROR_NONE))
        ret = TELEBOT_ERROR_OPERATION_FAILED;
    if (ret != TELEBOT_ERROR_NONE)
        unlink(out_file);

    return ret;
}

telebot_core_response_t
telebot_core_export_chat_invite_link(telebot_core_handler_t core_h, long long int chat_id)
{
    int count = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    return telebot_core_perform(core_h, TELEBOT_METHOD_EXPORT_CHAT_INVITE_LINK, mimes, count);
}

telebot_core_response_t
telebot_core_delete_chat_photo(telebot_core_handler_t core_h, long long int chat_id)
{
    CHECK_ARG_CONDITION((chat_id <= 0), "Invalid chat id");

    int count = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    return telebot_core_perform(core_h, TELEBOT_METHOD_DELETE_CHAT_PHOTO, mimes, count);
}

telebot_core_response_t
telebot_core_set_chat_title(telebot_core_handler_t core_h, long long int chat_id, const char *title)
{
    CHECK_ARG_CONDITION((chat_id <= 0), "Valid chat id is required");
    CHECK_ARG_CONDITION((title == NULL) || (strlen(title) > 255), "Valid title is required");

    int count = 0;
    telebot_core_mime_t mimes[2]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "title";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = title;
    count++;

    return telebot_core_perform(core_h, TELEBOT_METHOD_SET_CHAT_TITLE, mimes, count);
}

telebot_core_response_t
telebot_core_set_chat_description(telebot_core_handler_t core_h, long long int chat_id, const char *description)
{
    CHECK_ARG_CONDITION((chat_id <= 0), "Valid chat id is required");
    CHECK_ARG_CONDITION((description == NULL) || (strlen(description) > 255),
                        "Valid description is required");

    int count = 0;
    telebot_core_mime_t mimes[2]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "description";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = description;
    count++;

    return telebot_core_perform(core_h, TELEBOT_METHOD_SET_CHAT_DESCRIPTION, mimes, count);
}

telebot_core_response_t
telebot_core_pin_chat_message(telebot_core_handler_t core_h, long long int chat_id, int message_id,
                              bool disable_notification)
{
    CHECK_ARG_CONDITION((chat_id <= 0), "Valid chat id is required");
    CHECK_ARG_CONDITION((message_id <= 0), "Valid message_id is required");

    int count = 0;
    telebot_core_mime_t mimes[3]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "message_id";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = message_id;
    count++;

    mimes[count].name = "disable_notification";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_notification ? "true" : "false";
    count++;

    return telebot_core_perform(core_h, TELEBOT_METHOD_PIN_CHAT_MESSAGE, mimes, count);
}

telebot_core_response_t
telebot_core_unpin_chat_message(telebot_core_handler_t core_h, long long int chat_id)
{
    CHECK_ARG_CONDITION((chat_id <= 0), "Valid chat id is required");

    int count = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    return telebot_core_perform(core_h, TELEBOT_METHOD_UNPIN_CHAT_MESSAGE, mimes, count);
}

telebot_core_response_t
telebot_core_leave_chat(telebot_core_handler_t core_h, long long int chat_id)
{
    CHECK_ARG_CONDITION((chat_id <= 0), "Valid chat id is required");

    int count = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    return telebot_core_perform(core_h, TELEBOT_METHOD_LEAVE_CHAT, mimes, count);
}

telebot_core_response_t
telebot_core_get_chat(telebot_core_handler_t core_h, long long int chat_id)
{
    CHECK_ARG_CONDITION((chat_id <= 0), "Valid chat id is required");

    int count = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_CHAT, mimes, count);
}

telebot_core_response_t
telebot_core_get_chat_members_count(telebot_core_handler_t core_h, long long int chat_id)
{
    CHECK_ARG_CONDITION((chat_id <= 0), "Valid chat id is required");

    int count = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_CHAT_MEMBERS_COUNT, mimes, count);
}

telebot_core_response_t
telebot_core_set_chat_sticker_set(telebot_core_handler_t core_h, long long int chat_id, const char *sticker_set_name)
{
    CHECK_ARG_CONDITION((chat_id <= 0), "Valid chat id is required");
    CHECK_ARG_NULL(sticker_set_name);

    int count = 0;
    telebot_core_mime_t mimes[2]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "sticker_set_name";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = sticker_set_name;
    count++;

    return telebot_core_perform(core_h, TELEBOT_METHOD_SET_CHAT_STICKER_SET, mimes, count);
}

telebot_core_response_t
telebot_core_delete_chat_sticker_set(telebot_core_handler_t core_h, long long int chat_id)
{
    CHECK_ARG_CONDITION((chat_id <= 0), "Valid chat id is required");

    int count = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    return telebot_core_perform(core_h, TELEBOT_METHOD_DEL_CHAT_STICKER_SET, mimes, count);
}

telebot_core_response_t
telebot_core_answer_callback_query(telebot_core_handler_t core_h, const char *callback_query_id, const char *text,
                                   bool show_alert, const char *url, int cache_time)
{
    CHECK_ARG_NULL(callback_query_id);

    int count = 0;
    telebot_core_mime_t mimes[5]; // number of arguments
    mimes[count].name = "callback_query_id";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = callback_query_id;
    count++;

    if (text != NULL)
    {
        mimes[count].name = "text";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = text;
        count++;
    }

    mimes[count].name = "show_alert";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = show_alert ? "true" : "false";
    count++;

    if (url != NULL)
    {
        mimes[count].name = "url";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = url;
        count++;
    }

    if (cache_time > 0)
    {
        mimes[count].name = "cache_time";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = cache_time;
        count++;
    }

    return telebot_core_perform(core_h, TELEBOT_METHOD_ANSWER_CALLBACK_QUERY, mimes, count);
}

telebot_core_response_t
telebot_core_set_my_commands(telebot_core_handler_t core_h, const char *commands)
{
    CHECK_ARG_NULL(commands);

    int count = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[count].name = "commands";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = commands;
    count++;

    return telebot_core_perform(core_h, TELEBOT_METHOD_SET_MY_COMMANDS, mimes, count);
}

telebot_core_response_t
telebot_core_get_my_commands(telebot_core_handler_t core_h)
{
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_MY_COMMANDS, NULL, 0);
}

telebot_core_response_t
telebot_core_edit_message_caption(telebot_core_handler_t core_h, long long int chat_id, int message_id,
                                  const char *inline_message_id, const char *caption, const char *parse_mode,
                                  const char *reply_markup)
{
    if (((chat_id == 0) || (message_id <= 0)) && (inline_message_id == NULL))
    {
        ERR("Either valid chat_id & message_id or inline_message_id required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    int count = 0;
    telebot_core_mime_t mimes[6]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    if (message_id > 0)
    {
        mimes[count].name = "message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = message_id;
        count++;
    }

    if (inline_message_id != NULL)
    {
        mimes[count].name = "inline_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = inline_message_id;
        count++;
    }

    mimes[count].name = "caption";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = caption;
    count++;

    if (parse_mode != NULL)
    {
        mimes[count].name = "parse_mode";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = parse_mode;
        count++;
    }

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_perform(core_h, TELEBOT_METHOD_EDIT_MESSAGE_CAPTION, mimes, count);
}

telebot_core_response_t
telebot_core_edit_message_reply_markup(telebot_core_handler_t core_h, long long int chat_id, int message_id,
                                       const char *inline_message_id, const char *reply_markup)
{
    if (((chat_id == 0) || (message_id <= 0)) && (inline_message_id == NULL))
    {
        ERR("Either valid chat_id & message_id or inline_message_id required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    int count = 0;
    telebot_core_mime_t mimes[4]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    if (message_id > 0)
    {
        mimes[count].name = "message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = message_id;
        count++;
    }

    if (inline_message_id != NULL)
    {
        mimes[count].name = "inline_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = inline_message_id;
        count++;
    }

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_perform(core_h, TELEBOT_METHOD_EDIT_MESSAGE_REPLY_MARKUP, mimes, count);
}

telebot_core_response_t
telebot_core_stop_poll(telebot_core_handler_t core_h, long long int chat_id, int message_id, const char *reply_markup)
{
    CHECK_ARG_CONDITION((chat_id <= 0) || (message_id <= 0),
                        "Valid chat_id and message_id required");

    int count = 0;
    telebot_core_mime_t mimes[3]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "message_id";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = message_id;
    count++;

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_perform(core_h, TELEBOT_METHOD_STOP_POLL, mimes, count);
}

telebot_core_response_t
telebot_core_delete_message(telebot_core_handler_t core_h, long long int chat_id, int message_id)
{
    CHECK_ARG_CONDITION((chat_id <= 0) || (message_id <= 0),
                        "Valid chat_id and message_id required");

    int count = 0;
    telebot_core_mime_t mimes[2]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "message_id";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = message_id;
    count++;

    return telebot_core_perform(core_h, TELEBOT_METHOD_DELETE_MESSAGE, mimes, count);
}

telebot_core_response_t
telebot_core_log_out(telebot_core_handler_t core_h)
{
    return telebot_core_perform(core_h, TELEBOT_METHOD_LOG_OUT, NULL, 0);
}

telebot_core_response_t
telebot_core_close(telebot_core_handler_t core_h)
{
    return telebot_core_perform(core_h, TELEBOT_METHOD_CLOSE, NULL, 0);
}

telebot_core_response_t
telebot_core_set_my_name(telebot_core_handler_t core_h, const char *name, const char *language_code)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    if (name)
    {
        mimes[count].name = "name";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = name;
        count++;
    }
    if (language_code)
    {
        mimes[count].name = "language_code";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = language_code;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_SET_MY_NAME, mimes, count);
}

telebot_core_response_t
telebot_core_get_my_name(telebot_core_handler_t core_h, const char *language_code)
{
    int count = 0;
    telebot_core_mime_t mimes[1];
    if (language_code)
    {
        mimes[count].name = "language_code";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = language_code;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_MY_NAME, mimes, count);
}

telebot_core_response_t
telebot_core_set_my_description(telebot_core_handler_t core_h, const char *description, const char *language_code)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    if (description)
    {
        mimes[count].name = "description";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = description;
        count++;
    }
    if (language_code)
    {
        mimes[count].name = "language_code";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = language_code;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_SET_MY_DESCRIPTION, mimes, count);
}

telebot_core_response_t
telebot_core_get_my_description(telebot_core_handler_t core_h, const char *language_code)
{
    int count = 0;
    telebot_core_mime_t mimes[1];
    if (language_code)
    {
        mimes[count].name = "language_code";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = language_code;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_MY_DESCRIPTION, mimes, count);
}

telebot_core_response_t
telebot_core_set_my_short_description(telebot_core_handler_t core_h, const char *short_description,
                                      const char *language_code)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    if (short_description)
    {
        mimes[count].name = "short_description";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = short_description;
        count++;
    }
    if (language_code)
    {
        mimes[count].name = "language_code";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = language_code;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_SET_MY_SHORT_DESCRIPTION, mimes, count);
}

telebot_core_response_t
telebot_core_get_my_short_description(telebot_core_handler_t core_h, const char *language_code)
{
    int count = 0;
    telebot_core_mime_t mimes[1];
    if (language_code)
    {
        mimes[count].name = "language_code";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = language_code;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_MY_SHORT_DESCRIPTION, mimes, count);
}

telebot_core_response_t
telebot_core_set_chat_menu_button(telebot_core_handler_t core_h, long long int chat_id, const char *menu_button)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    if (chat_id != 0)
    {
        mimes[count].name = "chat_id";
        mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
        mimes[count].data.lld = chat_id;
        count++;
    }
    if (menu_button)
    {
        mimes[count].name = "menu_button";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = menu_button;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_SET_CHAT_MENU_BUTTON, mimes, count);
}

telebot_core_response_t
telebot_core_get_chat_menu_button(telebot_core_handler_t core_h, long long int chat_id)
{
    int count = 0;
    telebot_core_mime_t mimes[1];
    if (chat_id != 0)
    {
        mimes[count].name = "chat_id";
        mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
        mimes[count].data.lld = chat_id;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_CHAT_MENU_BUTTON, mimes, count);
}

telebot_core_response_t
telebot_core_set_my_default_administrator_rights(telebot_core_handler_t core_h, const char *rights, bool for_channels)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    if (rights)
    {
        mimes[count].name = "rights";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = rights;
        count++;
    }
    mimes[count].name = "for_channels";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = for_channels ? "true" : "false";
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_SET_MY_DEFAULT_ADMIN_RIGHTS, mimes, count);
}

telebot_core_response_t
telebot_core_get_my_default_administrator_rights(telebot_core_handler_t core_h, bool for_channels)
{
    int count = 0;
    telebot_core_mime_t mimes[1];
    mimes[count].name = "for_channels";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = for_channels ? "true" : "false";
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_MY_DEFAULT_ADMIN_RIGHTS, mimes, count);
}

telebot_core_response_t
telebot_core_delete_my_commands(telebot_core_handler_t core_h, const char *scope, const char *language_code)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    if (scope)
    {
        mimes[count].name = "scope";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = scope;
        count++;
    }
    if (language_code)
    {
        mimes[count].name = "language_code";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = language_code;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_DELETE_MY_COMMANDS, mimes, count);
}

telebot_core_response_t
telebot_core_ban_chat_sender_chat(telebot_core_handler_t core_h, long long int chat_id, long long int sender_chat_id)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    mimes[count].name = "sender_chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = sender_chat_id;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_BAN_CHAT_SENDER_CHAT, mimes, count);
}

telebot_core_response_t
telebot_core_unban_chat_sender_chat(telebot_core_handler_t core_h, long long int chat_id, long long int sender_chat_id)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    mimes[count].name = "sender_chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = sender_chat_id;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_UNBAN_CHAT_SENDER_CHAT, mimes, count);
}

telebot_core_response_t
telebot_core_create_chat_invite_link(telebot_core_handler_t core_h, long long int chat_id, const char *name,
                                     long expire_date, int member_limit, bool creates_join_request)
{
    int count = 0;
    telebot_core_mime_t mimes[6];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    if (name)
    {
        mimes[count].name = "name";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = name;
        count++;
    }
    if (expire_date > 0)
    {
        mimes[count].name = "expire_date";
        mimes[count].type = TELEBOT_MIME_TYPE_LONG_INT;
        mimes[count].data.ld = expire_date;
        count++;
    }
    if (member_limit > 0)
    {
        mimes[count].name = "member_limit";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = member_limit;
        count++;
    }
    mimes[count].name = "creates_join_request";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = creates_join_request ? "true" : "false";
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_CREATE_CHAT_INVITE_LINK, mimes, count);
}

telebot_core_response_t
telebot_core_edit_chat_invite_link(telebot_core_handler_t core_h, long long int chat_id, const char *invite_link,
                                   const char *name, long expire_date, int member_limit, bool creates_join_request)
{
    int count = 0;
    telebot_core_mime_t mimes[7];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    mimes[count].name = "invite_link";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = invite_link;
    count++;
    if (name)
    {
        mimes[count].name = "name";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = name;
        count++;
    }
    if (expire_date > 0)
    {
        mimes[count].name = "expire_date";
        mimes[count].type = TELEBOT_MIME_TYPE_LONG_INT;
        mimes[count].data.ld = expire_date;
        count++;
    }
    if (member_limit > 0)
    {
        mimes[count].name = "member_limit";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = member_limit;
        count++;
    }
    mimes[count].name = "creates_join_request";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = creates_join_request ? "true" : "false";
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_EDIT_CHAT_INVITE_LINK, mimes, count);
}

telebot_core_response_t
telebot_core_revoke_chat_invite_link(telebot_core_handler_t core_h, long long int chat_id, const char *invite_link)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    mimes[count].name = "invite_link";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = invite_link;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_REVOKE_CHAT_INVITE_LINK, mimes, count);
}

telebot_core_response_t
telebot_core_create_forum_topic(telebot_core_handler_t core_h, long long int chat_id, const char *name,
                                int icon_color, const char *icon_custom_emoji_id)
{
    int count = 0;
    telebot_core_mime_t mimes[5];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    mimes[count].name = "name";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = name;
    count++;
    if (icon_color > 0)
    {
        mimes[count].name = "icon_color";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = icon_color;
        count++;
    }
    if (icon_custom_emoji_id)
    {
        mimes[count].name = "icon_custom_emoji_id";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = icon_custom_emoji_id;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_CREATE_FORUM_TOPIC, mimes, count);
}

telebot_core_response_t
telebot_core_edit_forum_topic(telebot_core_handler_t core_h, long long int chat_id, int message_thread_id,
                              const char *name, const char *icon_custom_emoji_id)
{
    int count = 0;
    telebot_core_mime_t mimes[4];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    mimes[count].name = "message_thread_id";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = message_thread_id;
    count++;
    if (name)
    {
        mimes[count].name = "name";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = name;
        count++;
    }
    if (icon_custom_emoji_id)
    {
        mimes[count].name = "icon_custom_emoji_id";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = icon_custom_emoji_id;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_EDIT_FORUM_TOPIC, mimes, count);
}

telebot_core_response_t
telebot_core_close_forum_topic(telebot_core_handler_t core_h, long long int chat_id, int message_thread_id)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    mimes[count].name = "message_thread_id";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = message_thread_id;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_CLOSE_FORUM_TOPIC, mimes, count);
}

telebot_core_response_t
telebot_core_reopen_forum_topic(telebot_core_handler_t core_h, long long int chat_id, int message_thread_id)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    mimes[count].name = "message_thread_id";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = message_thread_id;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_REOPEN_FORUM_TOPIC, mimes, count);
}

telebot_core_response_t
telebot_core_delete_forum_topic(telebot_core_handler_t core_h, long long int chat_id, int message_thread_id)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    mimes[count].name = "message_thread_id";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = message_thread_id;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_DELETE_FORUM_TOPIC, mimes, count);
}

telebot_core_response_t
telebot_core_unpin_all_forum_topic_messages(telebot_core_handler_t core_h, long long int chat_id, int message_thread_id)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    mimes[count].name = "message_thread_id";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = message_thread_id;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_UNPIN_ALL_FORUM_TOPIC_MESSAGES, mimes, count);
}

telebot_core_response_t
telebot_core_get_forum_topic_icon_stickers(telebot_core_handler_t core_h)
{
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_FORUM_TOPIC_ICON_STICKERS, NULL, 0);
}

telebot_core_response_t
telebot_core_edit_general_forum_topic(telebot_core_handler_t core_h, long long int chat_id, const char *name)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    mimes[count].name = "name";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = name;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_EDIT_GENERAL_FORUM_TOPIC, mimes, count);
}

telebot_core_response_t
telebot_core_close_general_forum_topic(telebot_core_handler_t core_h, long long int chat_id)
{
    int count = 0;
    telebot_core_mime_t mimes[1];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_CLOSE_GENERAL_FORUM_TOPIC, mimes, count);
}

telebot_core_response_t
telebot_core_reopen_general_forum_topic(telebot_core_handler_t core_h, long long int chat_id)
{
    int count = 0;
    telebot_core_mime_t mimes[1];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_REOPEN_GENERAL_FORUM_TOPIC, mimes, count);
}

telebot_core_response_t
telebot_core_hide_general_forum_topic(telebot_core_handler_t core_h, long long int chat_id)
{
    int count = 0;
    telebot_core_mime_t mimes[1];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_HIDE_GENERAL_FORUM_TOPIC, mimes, count);
}

telebot_core_response_t
telebot_core_unhide_general_forum_topic(telebot_core_handler_t core_h, long long int chat_id)
{
    int count = 0;
    telebot_core_mime_t mimes[1];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_UNHIDE_GENERAL_FORUM_TOPIC, mimes, count);
}

telebot_core_response_t
telebot_core_unpin_all_general_forum_topic_messages(telebot_core_handler_t core_h, long long int chat_id)
{
    int count = 0;
    telebot_core_mime_t mimes[1];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_UNPIN_ALL_GENERAL_FORUM_TOPIC_MESSAGES, mimes, count);
}

telebot_core_response_t
telebot_core_get_sticker_set(telebot_core_handler_t core_h, const char *name)
{
    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_str_required(&req, "name", name);
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_STICKER_SET, &req);
}

telebot_core_response_t
telebot_core_get_custom_emoji_stickers(telebot_core_handler_t core_h, const char *custom_emoji_ids)
{
    int count = 0;
    telebot_core_mime_t mimes[1];
    mimes[count].name = "custom_emoji_ids";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = custom_emoji_ids;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_CUSTOM_EMOJI_STICKERS, mimes, count);
}

telebot_core_response_t
telebot_core_answer_web_app_query(telebot_core_handler_t core_h, const char *web_app_query_id, const char *result)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    mimes[count].name = "web_app_query_id";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = web_app_query_id;
    count++;
    mimes[count].name = "result";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = result;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_ANSWER_WEB_APP_QUERY, mimes, count);
}

telebot_core_response_t
telebot_core_delete_messages(telebot_core_handler_t core_h, long long int chat_id, const char *message_ids)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    mimes[count].name = "message_ids";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = message_ids;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_DELETE_MESSAGES, mimes, count);
}

telebot_core_response_t
telebot_core_edit_message_media(telebot_core_handler_t core_h, long long int chat_id, int message_id,
                                const char *inline_message_id, const char *media, const char *reply_markup)
{
    if (((chat_id == 0) || (message_id <= 0)) && (inline_message_id == NULL))
    {
        ERR("Either valid chat_id & message_id or inline_message_id required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    int count = 0;
    telebot_core_mime_t mimes[5];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    if (message_id > 0)
    {
        mimes[count].name = "message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = message_id;
        count++;
    }

    if (inline_message_id != NULL)
    {
        mimes[count].name = "inline_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = inline_message_id;
        count++;
    }

    mimes[count].name = "media";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = media;
    count++;

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_perform(core_h, TELEBOT_METHOD_EDIT_MESSAGE_MEDIA, mimes, count);
}

telebot_core_response_t
telebot_core_send_paid_media(telebot_core_handler_t core_h, long long int chat_id, int star_count, const char *media,
                             const char *caption, const char *parse_mode, const char *caption_entities,
                             bool show_caption_above_media, bool disable_notification, bool protect_content,
                             const char *reply_parameters, const char *reply_markup)
{
    int count = 0;
    telebot_core_mime_t mimes[12];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    mimes[count].name = "star_count";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = star_count;
    count++;
    mimes[count].name = "media";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = media;
    count++;
    if (caption)
    {
        mimes[count].name = "caption";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = caption;
        count++;
    }
    if (parse_mode)
    {
        mimes[count].name = "parse_mode";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = parse_mode;
        count++;
    }
    if (caption_entities)
    {
        mimes[count].name = "caption_entities";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = caption_entities;
        count++;
    }
    mimes[count].name = "show_caption_above_media";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = show_caption_above_media ? "true" : "false";
    count++;
    mimes[count].name = "disable_notification";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_notification ? "true" : "false";
    count++;
    mimes[count].name = "protect_content";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = protect_content ? "true" : "false";
    count++;
    if (reply_parameters)
    {
        mimes[count].name = "reply_parameters";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_parameters;
        count++;
    }
    if (reply_markup)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_SEND_PAID_MEDIA, mimes, count);
}

telebot_core_response_t
telebot_core_answer_inline_query(telebot_core_handler_t core_h, const char *inline_query_id, const char *results,
                                 int cache_time, bool is_personal, const char *next_offset, const char *button)
{
    int count = 0;
    telebot_core_mime_t mimes[6];
    mimes[count].name = "inline_query_id";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = inline_query_id;
    count++;
    mimes[count].name = "results";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = results;
    count++;
    if (cache_time > 0)
    {
        mimes[count].name = "cache_time";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = cache_time;
        count++;
    }
    if (is_personal)
    {
        mimes[count].name = "is_personal";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = "true";
        count++;
    }
    if (next_offset)
    {
        mimes[count].name = "next_offset";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = next_offset;
        count++;
    }
    if (button)
    {
        mimes[count].name = "button";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = button;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_ANSWER_INLINE_QUERY, mimes, count);
}

telebot_core_response_t
telebot_core_save_prepared_inline_message(telebot_core_handler_t core_h, long long int user_id, const char *result,
                                          bool allow_user_chats, bool allow_bot_chats, bool allow_group_chats,
                                          bool allow_channel_chats)
{
    int count = 0;
    telebot_core_mime_t mimes[6];
    mimes[count].name = "user_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = user_id;
    count++;
    mimes[count].name = "result";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = result;
    count++;
    if (allow_user_chats)
    {
        mimes[count].name = "allow_user_chats";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = "true";
        count++;
    }
    if (allow_bot_chats)
    {
        mimes[count].name = "allow_bot_chats";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = "true";
        count++;
    }
    if (allow_group_chats)
    {
        mimes[count].name = "allow_group_chats";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = "true";
        count++;
    }
    if (allow_channel_chats)
    {
        mimes[count].name = "allow_channel_chats";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = "true";
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_SAVE_PREPARED_INLINE_MESSAGE, mimes, count);
}

telebot_core_response_t
telebot_core_send_gift(telebot_core_handler_t core_h, long long int user_id, long long int chat_id, const char *gift_id,
                       bool pay_for_upgrade, const char *text, const char *text_parse_mode,
                       const char *text_entities)
{
    int count = 0;
    telebot_core_mime_t mimes[7];
    if (user_id > 0)
    {
        mimes[count].name = "user_id";
        mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
        mimes[count].data.lld = user_id;
        count++;
    }
    if (chat_id != 0)
    {
        mimes[count].name = "chat_id";
        mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
        mimes[count].data.lld = chat_id;
        count++;
    }
    mimes[count].name = "gift_id";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = gift_id;
    count++;
    mimes[count].name = "pay_for_upgrade";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = pay_for_upgrade ? "true" : "false";
    count++;
    if (text)
    {
        mimes[count].name = "text";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = text;
        count++;
    }
    if (text_parse_mode)
    {
        mimes[count].name = "text_parse_mode";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = text_parse_mode;
        count++;
    }
    if (text_entities)
    {
        mimes[count].name = "text_entities";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = text_entities;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_SEND_GIFT, mimes, count);
}

telebot_core_response_t
telebot_core_gift_premium_subscription(telebot_core_handler_t core_h, long long int user_id, int month_count,
                                       int star_count, const char *text, const char *text_parse_mode,
                                       const char *text_entities)
{
    int count = 0;
    telebot_core_mime_t mimes[7];
    mimes[count].name = "user_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = user_id;
    count++;
    mimes[count].name = "month_count";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = month_count;
    count++;
    mimes[count].name = "star_count";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = star_count;
    count++;
    if (text)
    {
        mimes[count].name = "text";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = text;
        count++;
    }
    if (text_parse_mode)
    {
        mimes[count].name = "text_parse_mode";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = text_parse_mode;
        count++;
    }
    if (text_entities)
    {
        mimes[count].name = "text_entities";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = text_entities;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_GIFT_PREMIUM_SUBSCRIPTION, mimes, count);
}

telebot_core_response_t
telebot_core_get_my_star_balance(telebot_core_handler_t core_h)
{
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_MY_STAR_BALANCE, NULL, 0);
}

telebot_core_response_t
telebot_core_get_star_transactions(telebot_core_handler_t core_h, int offset, int limit)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    if (offset > 0)
    {
        mimes[count].name = "offset";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = offset;
        count++;
    }
    if (limit > 0)
    {
        mimes[count].name = "limit";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = limit;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_STAR_TRANSACTIONS, mimes, count);
}

telebot_core_response_t
telebot_core_refund_star_payment(telebot_core_handler_t core_h, long long int user_id,
                                 const char *telegram_payment_charge_id)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    mimes[count].name = "user_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = user_id;
    count++;
    mimes[count].name = "telegram_payment_charge_id";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = telegram_payment_charge_id;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_REFUND_STAR_PAYMENT, mimes, count);
}

telebot_core_response_t
telebot_core_get_available_gifts(telebot_core_handler_t core_h)
{
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_AVAILABLE_GIFTS, NULL, 0);
}

telebot_core_response_t
telebot_core_get_user_gifts(telebot_core_handler_t core_h, long long int user_id, int offset, int limit)
{
    int count = 0;
    telebot_core_mime_t mimes[3];
    mimes[count].name = "user_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = user_id;
    count++;
    if (offset > 0)
    {
        mimes[count].name = "offset";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = offset;
        count++;
    }
    if (limit > 0)
    {
        mimes[count].name = "limit";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = limit;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_USER_GIFTS, mimes, count);
}

telebot_core_response_t
telebot_core_get_chat_gifts(telebot_core_handler_t core_h, long long int chat_id, int offset, int limit)
{
    int count = 0;
    telebot_core_mime_t mimes[3];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    if (offset > 0)
    {
        mimes[count].name = "offset";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = offset;
        count++;
    }
    if (limit > 0)
    {
        mimes[count].name = "limit";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = limit;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_CHAT_GIFTS, mimes, count);
}

telebot_core_response_t
telebot_core_upgrade_gift(telebot_core_handler_t core_h, const char *gift_id, bool pay_for_upgrade, const char *text,
                          const char *text_parse_mode, const char *text_entities)
{
    int count = 0;
    telebot_core_mime_t mimes[5];
    mimes[count].name = "gift_id";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = gift_id;
    count++;
    mimes[count].name = "pay_for_upgrade";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = pay_for_upgrade ? "true" : "false";
    count++;
    if (text)
    {
        mimes[count].name = "text";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = text;
        count++;
    }
    if (text_parse_mode)
    {
        mimes[count].name = "text_parse_mode";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = text_parse_mode;
        count++;
    }
    if (text_entities)
    {
        mimes[count].name = "text_entities";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = text_entities;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_UPGRADE_GIFT, mimes, count);
}

telebot_core_response_t
telebot_core_transfer_gift(telebot_core_handler_t core_h, long long int user_id, long long int chat_id,
                           const char *gift_id)
{
    int count = 0;
    telebot_core_mime_t mimes[3];
    mimes[count].name = "user_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = user_id;
    count++;
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    mimes[count].name = "gift_id";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = gift_id;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_TRANSFER_GIFT, mimes, count);
}

telebot_core_response_t
telebot_core_convert_gift_to_stars(telebot_core_handler_t core_h, const char *gift_id)
{
    int count = 0;
    telebot_core_mime_t mimes[1];
    mimes[count].name = "gift_id";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = gift_id;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_CONVERT_GIFT_TO_STARS, mimes, count);
}

telebot_core_response_t
telebot_core_get_user_profile_audios(telebot_core_handler_t core_h, long long int user_id, int offset, int limit)
{
    int count = 0;
    telebot_core_mime_t mimes[3];
    mimes[count].name = "user_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = user_id;
    count++;
    if (offset > 0)
    {
        mimes[count].name = "offset";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = offset;
        count++;
    }
    if (limit > 0)
    {
        mimes[count].name = "limit";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = limit;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_USER_AUDIOS, mimes, count);
}

telebot_core_response_t
telebot_core_set_my_profile_photo(telebot_core_handler_t core_h, const char *photo)
{
    int count = 0;
    telebot_core_mime_t mimes[1];
    mimes[count].name = "photo";
    mimes[count].type = TELEBOT_MIME_TYPE_FILE;
    mimes[count].data.s = photo;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_SET_MY_PROFILE_PHOTO, mimes, count);
}

telebot_core_response_t
telebot_core_remove_my_profile_photo(telebot_core_handler_t core_h, const char *photo_id)
{
    int count = 0;
    telebot_core_mime_t mimes[1];
    mimes[count].name = "photo_id";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = photo_id;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_REMOVE_MY_PROFILE_PHOTO, mimes, count);
}

telebot_core_response_t
telebot_core_get_business_connection(telebot_core_handler_t core_h, const char *business_connection_id)
{
    int count = 0;
    telebot_core_mime_t mimes[1];
    mimes[count].name = "business_connection_id";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = business_connection_id;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_BUSINESS_CONNECTION, mimes, count);
}

telebot_core_response_t
telebot_core_get_user_chat_boosts(telebot_core_handler_t core_h, long long int chat_id, long long int user_id)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;
    mimes[count].name = "user_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = user_id;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_USER_CHAT_BOOSTS, mimes, count);
}

telebot_core_response_t
telebot_core_set_game_score(telebot_core_handler_t core_h, long long int user_id, int score, bool force,
                            bool disable_edit_message, long long int chat_id, int message_id,
                            const char *inline_message_id)
{
    int count = 0;
    telebot_core_mime_t mimes[7];
    mimes[count].name = "user_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = user_id;
    count++;
    mimes[count].name = "score";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = score;
    count++;
    mimes[count].name = "force";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = force ? "true" : "false";
    count++;
    mimes[count].name = "disable_edit_message";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_edit_message ? "true" : "false";
    count++;
    if (chat_id != 0)
    {
        mimes[count].name = "chat_id";
        mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
        mimes[count].data.lld = chat_id;
        count++;
    }
    if (message_id > 0)
    {
        mimes[count].name = "message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = message_id;
        count++;
    }
    if (inline_message_id)
    {
        mimes[count].name = "inline_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = inline_message_id;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_SET_GAME_SCORE, mimes, count);
}

telebot_core_response_t
telebot_core_get_game_high_scores(telebot_core_handler_t core_h, long long int user_id, long long int chat_id,
                                  int message_id, const char *inline_message_id)
{
    int count = 0;
    telebot_core_mime_t mimes[4];
    mimes[count].name = "user_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = user_id;
    count++;
    if (chat_id != 0)
    {
        mimes[count].name = "chat_id";
        mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
        mimes[count].data.lld = chat_id;
        count++;
    }
    if (message_id > 0)
    {
        mimes[count].name = "message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = message_id;
        count++;
    }
    if (inline_message_id)
    {
        mimes[count].name = "inline_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = inline_message_id;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_GAME_HIGH_SCORES, mimes, count);
}

telebot_core_response_t
telebot_core_set_passport_data_errors(telebot_core_handler_t core_h, long long int user_id, const char *errors)
{
    int count = 0;
    telebot_core_mime_t mimes[2];
    mimes[count].name = "user_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = user_id;
    count++;
    mimes[count].name = "errors";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = errors;
    count++;
    return telebot_core_perform(core_h, TELEBOT_METHOD_SET_PASSPORT_DATA_ERRORS, mimes, count);
}

telebot_core_response_t
telebot_core_answer_shipping_query(telebot_core_handler_t core_h, const char *shipping_query_id, bool ok,
                                   const char *shipping_options, const char *error_message)
{
    int count = 0;
    telebot_core_mime_t mimes[4];
    mimes[count].name = "shipping_query_id";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = shipping_query_id;
    count++;
    mimes[count].name = "ok";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = ok ? "true" : "false";
    count++;
    if (shipping_options)
    {
        mimes[count].name = "shipping_options";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = shipping_options;
        count++;
    }
    if (error_message)
    {
        mimes[count].name = "error_message";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = error_message;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_ANSWER_SHIPPING_QUERY, mimes, count);
}

telebot_core_response_t
telebot_core_answer_pre_checkout_query(telebot_core_handler_t core_h, const char *pre_checkout_query_id, bool ok,
                                       const char *error_message)
{
    int count = 0;
    telebot_core_mime_t mimes[3];
    mimes[count].name = "pre_checkout_query_id";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = pre_checkout_query_id;
    count++;
    mimes[count].name = "ok";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = ok ? "true" : "false";
    count++;
    if (error_message)
    {
        mimes[count].name = "error_message";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = error_message;
        count++;
    }
    return telebot_core_perform(core_h, TELEBOT_METHOD_ANSWER_PRE_CHECKOUT_QUERY, mimes, count);
}
