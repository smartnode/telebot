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
#include "telebot-limits.h"
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

long long int telebot_core_get_response_migrate_to_chat_id(telebot_core_response_t response)
{
    return response ? response->migrate_to_chat_id : 0;
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

    *addr = NULL;
    if (core_h->proxy_addr == NULL)
        return TELEBOT_ERROR_NONE;

    *addr = strdup(core_h->proxy_addr);
    return (*addr != NULL) ? TELEBOT_ERROR_NONE : TELEBOT_ERROR_OUT_OF_MEMORY;
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
        resp->ret = (status == 429L) ? TELEBOT_ERROR_RATE_LIMITED : TELEBOT_ERROR_OPERATION_FAILED;

        /* {"ok":false,"error_code":...,"parameters":{"retry_after":N,"migrate_to_chat_id":M}} */
        cJSON *root = (resp->data && strstr(resp->data, "\"parameters\"")) ? cJSON_Parse(resp->data) : NULL;
        cJSON *params = NULL, *value = NULL;
        if (telebot_json_get_ex(root, "parameters", &params))
        {
            if (telebot_json_get_ex(params, "retry_after", &value))
                resp->retry_after = (int)telebot_json_get_int(value);
            if (telebot_json_get_ex(params, "migrate_to_chat_id", &value))
                resp->migrate_to_chat_id = telebot_json_get_int(value);
        }
        telebot_json_put(root);
    }
    else
    {
        DBG("Response: %s", resp->data ? resp->data : "");
    }
    core_h->last_retry_after = resp->retry_after;

    return resp;
}

telebot_core_response_t
telebot_core_call_method(telebot_core_handler_t core_h, const char *method, const telebot_param_t params[],
                         size_t count)
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
            telebot_request_file(&req, p->name, p->file);
        else
            telebot_request_str(&req, p->name, p->value);
    }
    return telebot_core_request(core_h, method, &req);
}

telebot_core_response_t
telebot_core_call(telebot_core_handler_t core_h, const char *method, const telebot_param_t params[], size_t count)
{
    return telebot_core_call_method(core_h, method, params, count);
}

telebot_core_response_t
telebot_core_get_updates(telebot_core_handler_t core_h, int offset, int limit, int timeout, const char *allowed_updates)
{
    if (limit > TELEBOT_UPDATE_COUNT_MAX_LIMIT)
        limit = TELEBOT_UPDATE_COUNT_MAX_LIMIT;

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_int(&req, "offset", offset);

    telebot_request_int(&req, "limit", limit);

    telebot_request_int(&req, "timeout", timeout);

    if (allowed_updates)
    {
        telebot_request_str(&req, "allowed_updates", allowed_updates);
    }

    return telebot_core_request(core_h, TELEBOT_METHOD_GET_UPDATES, &req);
}

telebot_core_response_t telebot_core_error_response(telebot_error_e ret)
{
    telebot_core_response_t resp = calloc(1, sizeof(struct telebot_core_response));
    if (resp)
        resp->ret = ret;

    return resp;
}

telebot_core_response_t telebot_core_request(telebot_core_handler_t core_h, const char *method,
                                             telebot_request_t *req)
{
    telebot_core_response_t response = (req->error != TELEBOT_ERROR_NONE)
                                           ? telebot_core_error_response(req->error)
                                           : telebot_core_perform(core_h, method, req->parts, req->count);
    telebot_request_release(req);
    return response;
}

telebot_core_response_t
telebot_core_set_webhook(telebot_core_handler_t core_h, const char *url, const telebot_input_file_t *certificate,
                         int max_connections, const char *allowed_updates, bool drop_pending_updates,
                         const char *secret_token)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_str_required(&req, "url", url);
    telebot_request_upload(&req, "certificate", certificate); /* self-signed public key, always uploaded */
    telebot_request_id(&req, "max_connections", max_connections);
    telebot_request_str(&req, "allowed_updates", allowed_updates);
    telebot_request_flag(&req, "drop_pending_updates", drop_pending_updates);
    telebot_request_str(&req, "secret_token", secret_token);
    return telebot_core_request(core_h, TELEBOT_METHOD_SET_WEBHOOK, &req);
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
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);

    telebot_request_double(&req, "latitude", (double)(latitude));

    telebot_request_double(&req, "longitude", (double)(longitude));

    if (message_id > 0)
    {
        telebot_request_int(&req, "message_id", message_id);
    }

    if (inline_message_id != NULL)
    {
        telebot_request_str(&req, "inline_message_id", inline_message_id);
    }

    if (reply_markup != NULL)
    {
        telebot_request_str(&req, "reply_markup", reply_markup);
    }

    return telebot_core_request(core_h, TELEBOT_METHOD_EDIT_MESSAGE_LIVE_LOCATION, &req);
}

telebot_core_response_t
telebot_core_stop_message_live_location(telebot_core_handler_t core_h, long long int chat_id, int message_id,
                                        const char *inline_message_id, const char *reply_markup)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);

    if (message_id > 0)
    {
        telebot_request_int(&req, "message_id", message_id);
    }

    if (inline_message_id != NULL)
    {
        telebot_request_str(&req, "inline_message_id", inline_message_id);
    }

    if (reply_markup != NULL)
    {
        telebot_request_str(&req, "reply_markup", reply_markup);
    }

    return telebot_core_request(core_h, TELEBOT_METHOD_STOP_MESSAGE_LIVE_LOCATION, &req);
}

telebot_core_response_t
telebot_core_get_file(telebot_core_handler_t core_h, const char *file_id)
{
    if (file_id == NULL)
    {
        ERR("Argument 'file_id' is required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_str(&req, "file_id", file_id);

    return telebot_core_request(core_h, TELEBOT_METHOD_GET_FILE, &req);
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
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);

    return telebot_core_request(core_h, TELEBOT_METHOD_EXPORT_CHAT_INVITE_LINK, &req);
}

telebot_core_response_t
telebot_core_delete_chat_photo(telebot_core_handler_t core_h, long long int chat_id)
{
    if (chat_id == 0)
    {
        ERR("A chat id is required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);

    return telebot_core_request(core_h, TELEBOT_METHOD_DELETE_CHAT_PHOTO, &req);
}

telebot_core_response_t
telebot_core_set_chat_title(telebot_core_handler_t core_h, long long int chat_id, const char *title)
{
    if (chat_id == 0)
    {
        ERR("A chat id is required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }
    if ((title == NULL) || (strlen(title) > 255))
    {
        ERR("Valid title is required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);

    telebot_request_str(&req, "title", title);

    return telebot_core_request(core_h, TELEBOT_METHOD_SET_CHAT_TITLE, &req);
}

telebot_core_response_t
telebot_core_set_chat_description(telebot_core_handler_t core_h, long long int chat_id, const char *description)
{
    if (chat_id == 0)
    {
        ERR("A chat id is required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }
    if ((description == NULL) || (strlen(description) > 255))
    {
        ERR("Valid description is required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);

    telebot_request_str(&req, "description", description);

    return telebot_core_request(core_h, TELEBOT_METHOD_SET_CHAT_DESCRIPTION, &req);
}

telebot_core_response_t
telebot_core_pin_chat_message(telebot_core_handler_t core_h, long long int chat_id, int message_id,
                              bool disable_notification)
{
    if (chat_id == 0)
    {
        ERR("A chat id is required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }
    if (message_id <= 0)
    {
        ERR("Valid message_id is required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);

    telebot_request_int(&req, "message_id", message_id);

    telebot_request_bool(&req, "disable_notification", disable_notification);

    return telebot_core_request(core_h, TELEBOT_METHOD_PIN_CHAT_MESSAGE, &req);
}

telebot_core_response_t
telebot_core_unpin_chat_message(telebot_core_handler_t core_h, long long int chat_id)
{
    if (chat_id == 0)
    {
        ERR("A chat id is required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);

    return telebot_core_request(core_h, TELEBOT_METHOD_UNPIN_CHAT_MESSAGE, &req);
}

telebot_core_response_t
telebot_core_leave_chat(telebot_core_handler_t core_h, long long int chat_id)
{
    if (chat_id == 0)
    {
        ERR("A chat id is required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);

    return telebot_core_request(core_h, TELEBOT_METHOD_LEAVE_CHAT, &req);
}

telebot_core_response_t
telebot_core_get_chat(telebot_core_handler_t core_h, long long int chat_id)
{
    if (chat_id == 0)
    {
        ERR("A chat id is required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);

    return telebot_core_request(core_h, TELEBOT_METHOD_GET_CHAT, &req);
}

telebot_core_response_t
telebot_core_get_chat_members_count(telebot_core_handler_t core_h, long long int chat_id)
{
    if (chat_id == 0)
    {
        ERR("A chat id is required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);

    return telebot_core_request(core_h, TELEBOT_METHOD_GET_CHAT_MEMBERS_COUNT, &req);
}

telebot_core_response_t
telebot_core_set_chat_sticker_set(telebot_core_handler_t core_h, long long int chat_id, const char *sticker_set_name)
{
    if (chat_id == 0)
    {
        ERR("A chat id is required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }
    if (sticker_set_name == NULL)
    {
        ERR("Argument 'sticker_set_name' is required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);

    telebot_request_str(&req, "sticker_set_name", sticker_set_name);

    return telebot_core_request(core_h, TELEBOT_METHOD_SET_CHAT_STICKER_SET, &req);
}

telebot_core_response_t
telebot_core_delete_chat_sticker_set(telebot_core_handler_t core_h, long long int chat_id)
{
    if (chat_id == 0)
    {
        ERR("A chat id is required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);

    return telebot_core_request(core_h, TELEBOT_METHOD_DEL_CHAT_STICKER_SET, &req);
}

telebot_core_response_t
telebot_core_answer_callback_query(telebot_core_handler_t core_h, const char *callback_query_id, const char *text,
                                   bool show_alert, const char *url, int cache_time)
{
    if (callback_query_id == NULL)
    {
        ERR("Argument 'callback_query_id' is required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_str(&req, "callback_query_id", callback_query_id);

    if (text != NULL)
    {
        telebot_request_str(&req, "text", text);
    }

    telebot_request_bool(&req, "show_alert", show_alert);

    if (url != NULL)
    {
        telebot_request_str(&req, "url", url);
    }

    if (cache_time > 0)
    {
        telebot_request_int(&req, "cache_time", cache_time);
    }

    return telebot_core_request(core_h, TELEBOT_METHOD_ANSWER_CALLBACK_QUERY, &req);
}

telebot_core_response_t
telebot_core_set_my_commands(telebot_core_handler_t core_h, const char *commands)
{
    if (commands == NULL)
    {
        ERR("Argument 'commands' is required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_str(&req, "commands", commands);

    return telebot_core_request(core_h, TELEBOT_METHOD_SET_MY_COMMANDS, &req);
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

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);

    if (message_id > 0)
    {
        telebot_request_int(&req, "message_id", message_id);
    }

    if (inline_message_id != NULL)
    {
        telebot_request_str(&req, "inline_message_id", inline_message_id);
    }

    telebot_request_str(&req, "caption", caption);

    if (parse_mode != NULL)
    {
        telebot_request_str(&req, "parse_mode", parse_mode);
    }

    if (reply_markup != NULL)
    {
        telebot_request_str(&req, "reply_markup", reply_markup);
    }

    return telebot_core_request(core_h, TELEBOT_METHOD_EDIT_MESSAGE_CAPTION, &req);
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

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);

    if (message_id > 0)
    {
        telebot_request_int(&req, "message_id", message_id);
    }

    if (inline_message_id != NULL)
    {
        telebot_request_str(&req, "inline_message_id", inline_message_id);
    }

    if (reply_markup != NULL)
    {
        telebot_request_str(&req, "reply_markup", reply_markup);
    }

    return telebot_core_request(core_h, TELEBOT_METHOD_EDIT_MESSAGE_REPLY_MARKUP, &req);
}

telebot_core_response_t
telebot_core_stop_poll(telebot_core_handler_t core_h, long long int chat_id, int message_id, const char *reply_markup)
{
    if ((chat_id == 0) || (message_id <= 0))
    {
        ERR("Valid chat_id and message_id required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);

    telebot_request_int(&req, "message_id", message_id);

    if (reply_markup != NULL)
    {
        telebot_request_str(&req, "reply_markup", reply_markup);
    }

    return telebot_core_request(core_h, TELEBOT_METHOD_STOP_POLL, &req);
}

telebot_core_response_t
telebot_core_delete_message(telebot_core_handler_t core_h, long long int chat_id, int message_id)
{
    if ((chat_id == 0) || (message_id <= 0))
    {
        ERR("Valid chat_id and message_id required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);

    telebot_request_int(&req, "message_id", message_id);

    return telebot_core_request(core_h, TELEBOT_METHOD_DELETE_MESSAGE, &req);
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
    telebot_request_t req;
    telebot_request_init(&req);
    if (name)
    {
        telebot_request_str(&req, "name", name);
    }
    if (language_code)
    {
        telebot_request_str(&req, "language_code", language_code);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_SET_MY_NAME, &req);
}

telebot_core_response_t
telebot_core_get_my_name(telebot_core_handler_t core_h, const char *language_code)
{
    telebot_request_t req;
    telebot_request_init(&req);
    if (language_code)
    {
        telebot_request_str(&req, "language_code", language_code);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_MY_NAME, &req);
}

telebot_core_response_t
telebot_core_set_my_description(telebot_core_handler_t core_h, const char *description, const char *language_code)
{
    telebot_request_t req;
    telebot_request_init(&req);
    if (description)
    {
        telebot_request_str(&req, "description", description);
    }
    if (language_code)
    {
        telebot_request_str(&req, "language_code", language_code);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_SET_MY_DESCRIPTION, &req);
}

telebot_core_response_t
telebot_core_get_my_description(telebot_core_handler_t core_h, const char *language_code)
{
    telebot_request_t req;
    telebot_request_init(&req);
    if (language_code)
    {
        telebot_request_str(&req, "language_code", language_code);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_MY_DESCRIPTION, &req);
}

telebot_core_response_t
telebot_core_set_my_short_description(telebot_core_handler_t core_h, const char *short_description,
                                      const char *language_code)
{
    telebot_request_t req;
    telebot_request_init(&req);
    if (short_description)
    {
        telebot_request_str(&req, "short_description", short_description);
    }
    if (language_code)
    {
        telebot_request_str(&req, "language_code", language_code);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_SET_MY_SHORT_DESCRIPTION, &req);
}

telebot_core_response_t
telebot_core_get_my_short_description(telebot_core_handler_t core_h, const char *language_code)
{
    telebot_request_t req;
    telebot_request_init(&req);
    if (language_code)
    {
        telebot_request_str(&req, "language_code", language_code);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_MY_SHORT_DESCRIPTION, &req);
}

telebot_core_response_t
telebot_core_set_chat_menu_button(telebot_core_handler_t core_h, long long int chat_id, const char *menu_button)
{
    telebot_request_t req;
    telebot_request_init(&req);
    if (chat_id != 0)
    {
        telebot_request_ll(&req, "chat_id", chat_id);
    }
    if (menu_button)
    {
        telebot_request_str(&req, "menu_button", menu_button);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_SET_CHAT_MENU_BUTTON, &req);
}

telebot_core_response_t
telebot_core_get_chat_menu_button(telebot_core_handler_t core_h, long long int chat_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    if (chat_id != 0)
    {
        telebot_request_ll(&req, "chat_id", chat_id);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_CHAT_MENU_BUTTON, &req);
}

telebot_core_response_t
telebot_core_set_my_default_administrator_rights(telebot_core_handler_t core_h, const char *rights, bool for_channels)
{
    telebot_request_t req;
    telebot_request_init(&req);
    if (rights)
    {
        telebot_request_str(&req, "rights", rights);
    }
    telebot_request_bool(&req, "for_channels", for_channels);
    return telebot_core_request(core_h, TELEBOT_METHOD_SET_MY_DEFAULT_ADMIN_RIGHTS, &req);
}

telebot_core_response_t
telebot_core_get_my_default_administrator_rights(telebot_core_handler_t core_h, bool for_channels)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_bool(&req, "for_channels", for_channels);
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_MY_DEFAULT_ADMIN_RIGHTS, &req);
}

telebot_core_response_t
telebot_core_delete_my_commands(telebot_core_handler_t core_h, const char *scope, const char *language_code)
{
    telebot_request_t req;
    telebot_request_init(&req);
    if (scope)
    {
        telebot_request_str(&req, "scope", scope);
    }
    if (language_code)
    {
        telebot_request_str(&req, "language_code", language_code);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_DELETE_MY_COMMANDS, &req);
}

telebot_core_response_t
telebot_core_ban_chat_sender_chat(telebot_core_handler_t core_h, long long int chat_id, long long int sender_chat_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_ll(&req, "sender_chat_id", sender_chat_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_BAN_CHAT_SENDER_CHAT, &req);
}

telebot_core_response_t
telebot_core_unban_chat_sender_chat(telebot_core_handler_t core_h, long long int chat_id, long long int sender_chat_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_ll(&req, "sender_chat_id", sender_chat_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_UNBAN_CHAT_SENDER_CHAT, &req);
}

telebot_core_response_t
telebot_core_create_chat_invite_link(telebot_core_handler_t core_h, long long int chat_id, const char *name,
                                     long expire_date, int member_limit, bool creates_join_request)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    if (name)
    {
        telebot_request_str(&req, "name", name);
    }
    if (expire_date > 0)
    {
        telebot_request_ll(&req, "expire_date", (long long)(expire_date));
    }
    if (member_limit > 0)
    {
        telebot_request_int(&req, "member_limit", member_limit);
    }
    telebot_request_bool(&req, "creates_join_request", creates_join_request);
    return telebot_core_request(core_h, TELEBOT_METHOD_CREATE_CHAT_INVITE_LINK, &req);
}

telebot_core_response_t
telebot_core_edit_chat_invite_link(telebot_core_handler_t core_h, long long int chat_id, const char *invite_link,
                                   const char *name, long expire_date, int member_limit, bool creates_join_request)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_str(&req, "invite_link", invite_link);
    if (name)
    {
        telebot_request_str(&req, "name", name);
    }
    if (expire_date > 0)
    {
        telebot_request_ll(&req, "expire_date", (long long)(expire_date));
    }
    if (member_limit > 0)
    {
        telebot_request_int(&req, "member_limit", member_limit);
    }
    telebot_request_bool(&req, "creates_join_request", creates_join_request);
    return telebot_core_request(core_h, TELEBOT_METHOD_EDIT_CHAT_INVITE_LINK, &req);
}

telebot_core_response_t
telebot_core_revoke_chat_invite_link(telebot_core_handler_t core_h, long long int chat_id, const char *invite_link)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_str(&req, "invite_link", invite_link);
    return telebot_core_request(core_h, TELEBOT_METHOD_REVOKE_CHAT_INVITE_LINK, &req);
}

telebot_core_response_t
telebot_core_create_forum_topic(telebot_core_handler_t core_h, long long int chat_id, const char *name,
                                int icon_color, const char *icon_custom_emoji_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_str(&req, "name", name);
    if (icon_color > 0)
    {
        telebot_request_int(&req, "icon_color", icon_color);
    }
    if (icon_custom_emoji_id)
    {
        telebot_request_str(&req, "icon_custom_emoji_id", icon_custom_emoji_id);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_CREATE_FORUM_TOPIC, &req);
}

telebot_core_response_t
telebot_core_edit_forum_topic(telebot_core_handler_t core_h, long long int chat_id, int message_thread_id,
                              const char *name, const char *icon_custom_emoji_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_int(&req, "message_thread_id", message_thread_id);
    if (name)
    {
        telebot_request_str(&req, "name", name);
    }
    if (icon_custom_emoji_id)
    {
        telebot_request_str(&req, "icon_custom_emoji_id", icon_custom_emoji_id);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_EDIT_FORUM_TOPIC, &req);
}

telebot_core_response_t
telebot_core_close_forum_topic(telebot_core_handler_t core_h, long long int chat_id, int message_thread_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_int(&req, "message_thread_id", message_thread_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_CLOSE_FORUM_TOPIC, &req);
}

telebot_core_response_t
telebot_core_reopen_forum_topic(telebot_core_handler_t core_h, long long int chat_id, int message_thread_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_int(&req, "message_thread_id", message_thread_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_REOPEN_FORUM_TOPIC, &req);
}

telebot_core_response_t
telebot_core_delete_forum_topic(telebot_core_handler_t core_h, long long int chat_id, int message_thread_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_int(&req, "message_thread_id", message_thread_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_DELETE_FORUM_TOPIC, &req);
}

telebot_core_response_t
telebot_core_unpin_all_forum_topic_messages(telebot_core_handler_t core_h, long long int chat_id, int message_thread_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_int(&req, "message_thread_id", message_thread_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_UNPIN_ALL_FORUM_TOPIC_MESSAGES, &req);
}

telebot_core_response_t
telebot_core_get_forum_topic_icon_stickers(telebot_core_handler_t core_h)
{
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_FORUM_TOPIC_ICON_STICKERS, NULL, 0);
}

telebot_core_response_t
telebot_core_edit_general_forum_topic(telebot_core_handler_t core_h, long long int chat_id, const char *name)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_str(&req, "name", name);
    return telebot_core_request(core_h, TELEBOT_METHOD_EDIT_GENERAL_FORUM_TOPIC, &req);
}

telebot_core_response_t
telebot_core_close_general_forum_topic(telebot_core_handler_t core_h, long long int chat_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_CLOSE_GENERAL_FORUM_TOPIC, &req);
}

telebot_core_response_t
telebot_core_reopen_general_forum_topic(telebot_core_handler_t core_h, long long int chat_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_REOPEN_GENERAL_FORUM_TOPIC, &req);
}

telebot_core_response_t
telebot_core_hide_general_forum_topic(telebot_core_handler_t core_h, long long int chat_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_HIDE_GENERAL_FORUM_TOPIC, &req);
}

telebot_core_response_t
telebot_core_unhide_general_forum_topic(telebot_core_handler_t core_h, long long int chat_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_UNHIDE_GENERAL_FORUM_TOPIC, &req);
}

telebot_core_response_t
telebot_core_unpin_all_general_forum_topic_messages(telebot_core_handler_t core_h, long long int chat_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_UNPIN_ALL_GENERAL_FORUM_TOPIC_MESSAGES, &req);
}

telebot_core_response_t
telebot_core_get_sticker_set(telebot_core_handler_t core_h, const char *name)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_str_required(&req, "name", name);
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_STICKER_SET, &req);
}

telebot_core_response_t
telebot_core_get_custom_emoji_stickers(telebot_core_handler_t core_h, const char *custom_emoji_ids)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_str(&req, "custom_emoji_ids", custom_emoji_ids);
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_CUSTOM_EMOJI_STICKERS, &req);
}

telebot_core_response_t
telebot_core_answer_web_app_query(telebot_core_handler_t core_h, const char *web_app_query_id, const char *result)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_str(&req, "web_app_query_id", web_app_query_id);
    telebot_request_str(&req, "result", result);
    return telebot_core_request(core_h, TELEBOT_METHOD_ANSWER_WEB_APP_QUERY, &req);
}

telebot_core_response_t
telebot_core_delete_messages(telebot_core_handler_t core_h, long long int chat_id, const char *message_ids)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_str(&req, "message_ids", message_ids);
    return telebot_core_request(core_h, TELEBOT_METHOD_DELETE_MESSAGES, &req);
}

telebot_core_response_t
telebot_core_send_paid_media(telebot_core_handler_t core_h, long long int chat_id, int star_count, const char *media,
                             const char *caption, const char *parse_mode, const char *caption_entities,
                             bool show_caption_above_media, bool disable_notification, bool protect_content,
                             const char *reply_parameters, const char *reply_markup)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_int(&req, "star_count", star_count);
    telebot_request_str(&req, "media", media);
    if (caption)
    {
        telebot_request_str(&req, "caption", caption);
    }
    if (parse_mode)
    {
        telebot_request_str(&req, "parse_mode", parse_mode);
    }
    if (caption_entities)
    {
        telebot_request_str(&req, "caption_entities", caption_entities);
    }
    telebot_request_bool(&req, "show_caption_above_media", show_caption_above_media);
    telebot_request_bool(&req, "disable_notification", disable_notification);
    telebot_request_bool(&req, "protect_content", protect_content);
    if (reply_parameters)
    {
        telebot_request_str(&req, "reply_parameters", reply_parameters);
    }
    if (reply_markup)
    {
        telebot_request_str(&req, "reply_markup", reply_markup);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_PAID_MEDIA, &req);
}

telebot_core_response_t
telebot_core_answer_inline_query(telebot_core_handler_t core_h, const char *inline_query_id, const char *results,
                                 int cache_time, bool is_personal, const char *next_offset, const char *button)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_str(&req, "inline_query_id", inline_query_id);
    telebot_request_str(&req, "results", results);
    if (cache_time > 0)
    {
        telebot_request_int(&req, "cache_time", cache_time);
    }
    if (is_personal)
    {
        telebot_request_str(&req, "is_personal", "true");
    }
    if (next_offset)
    {
        telebot_request_str(&req, "next_offset", next_offset);
    }
    if (button)
    {
        telebot_request_str(&req, "button", button);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_ANSWER_INLINE_QUERY, &req);
}

telebot_core_response_t
telebot_core_save_prepared_inline_message(telebot_core_handler_t core_h, long long int user_id, const char *result,
                                          bool allow_user_chats, bool allow_bot_chats, bool allow_group_chats,
                                          bool allow_channel_chats)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "user_id", user_id);
    telebot_request_str(&req, "result", result);
    if (allow_user_chats)
    {
        telebot_request_str(&req, "allow_user_chats", "true");
    }
    if (allow_bot_chats)
    {
        telebot_request_str(&req, "allow_bot_chats", "true");
    }
    if (allow_group_chats)
    {
        telebot_request_str(&req, "allow_group_chats", "true");
    }
    if (allow_channel_chats)
    {
        telebot_request_str(&req, "allow_channel_chats", "true");
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_SAVE_PREPARED_INLINE_MESSAGE, &req);
}

telebot_core_response_t
telebot_core_send_gift(telebot_core_handler_t core_h, long long int user_id, long long int chat_id, const char *gift_id,
                       bool pay_for_upgrade, const char *text, const char *text_parse_mode,
                       const char *text_entities)
{
    telebot_request_t req;
    telebot_request_init(&req);
    if (user_id > 0)
    {
        telebot_request_ll(&req, "user_id", user_id);
    }
    if (chat_id != 0)
    {
        telebot_request_ll(&req, "chat_id", chat_id);
    }
    telebot_request_str(&req, "gift_id", gift_id);
    telebot_request_bool(&req, "pay_for_upgrade", pay_for_upgrade);
    if (text)
    {
        telebot_request_str(&req, "text", text);
    }
    if (text_parse_mode)
    {
        telebot_request_str(&req, "text_parse_mode", text_parse_mode);
    }
    if (text_entities)
    {
        telebot_request_str(&req, "text_entities", text_entities);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_GIFT, &req);
}

telebot_core_response_t
telebot_core_gift_premium_subscription(telebot_core_handler_t core_h, long long int user_id, int month_count,
                                       int star_count, const char *text, const char *text_parse_mode,
                                       const char *text_entities)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "user_id", user_id);
    telebot_request_int(&req, "month_count", month_count);
    telebot_request_int(&req, "star_count", star_count);
    if (text)
    {
        telebot_request_str(&req, "text", text);
    }
    if (text_parse_mode)
    {
        telebot_request_str(&req, "text_parse_mode", text_parse_mode);
    }
    if (text_entities)
    {
        telebot_request_str(&req, "text_entities", text_entities);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_GIFT_PREMIUM_SUBSCRIPTION, &req);
}

telebot_core_response_t
telebot_core_get_my_star_balance(telebot_core_handler_t core_h)
{
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_MY_STAR_BALANCE, NULL, 0);
}

telebot_core_response_t
telebot_core_get_star_transactions(telebot_core_handler_t core_h, int offset, int limit)
{
    telebot_request_t req;
    telebot_request_init(&req);
    if (offset > 0)
    {
        telebot_request_int(&req, "offset", offset);
    }
    if (limit > 0)
    {
        telebot_request_int(&req, "limit", limit);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_STAR_TRANSACTIONS, &req);
}

telebot_core_response_t
telebot_core_refund_star_payment(telebot_core_handler_t core_h, long long int user_id,
                                 const char *telegram_payment_charge_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "user_id", user_id);
    telebot_request_str(&req, "telegram_payment_charge_id", telegram_payment_charge_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_REFUND_STAR_PAYMENT, &req);
}

telebot_core_response_t
telebot_core_get_available_gifts(telebot_core_handler_t core_h)
{
    return telebot_core_perform(core_h, TELEBOT_METHOD_GET_AVAILABLE_GIFTS, NULL, 0);
}

telebot_core_response_t
telebot_core_get_user_gifts(telebot_core_handler_t core_h, long long int user_id, int offset, int limit)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "user_id", user_id);
    if (offset > 0)
    {
        telebot_request_int(&req, "offset", offset);
    }
    if (limit > 0)
    {
        telebot_request_int(&req, "limit", limit);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_USER_GIFTS, &req);
}

telebot_core_response_t
telebot_core_get_chat_gifts(telebot_core_handler_t core_h, long long int chat_id, int offset, int limit)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    if (offset > 0)
    {
        telebot_request_int(&req, "offset", offset);
    }
    if (limit > 0)
    {
        telebot_request_int(&req, "limit", limit);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_CHAT_GIFTS, &req);
}

telebot_core_response_t
telebot_core_upgrade_gift(telebot_core_handler_t core_h, const char *gift_id, bool pay_for_upgrade, const char *text,
                          const char *text_parse_mode, const char *text_entities)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_str(&req, "gift_id", gift_id);
    telebot_request_bool(&req, "pay_for_upgrade", pay_for_upgrade);
    if (text)
    {
        telebot_request_str(&req, "text", text);
    }
    if (text_parse_mode)
    {
        telebot_request_str(&req, "text_parse_mode", text_parse_mode);
    }
    if (text_entities)
    {
        telebot_request_str(&req, "text_entities", text_entities);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_UPGRADE_GIFT, &req);
}

telebot_core_response_t
telebot_core_transfer_gift(telebot_core_handler_t core_h, long long int user_id, long long int chat_id,
                           const char *gift_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "user_id", user_id);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_str(&req, "gift_id", gift_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_TRANSFER_GIFT, &req);
}

telebot_core_response_t
telebot_core_convert_gift_to_stars(telebot_core_handler_t core_h, const char *gift_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_str(&req, "gift_id", gift_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_CONVERT_GIFT_TO_STARS, &req);
}

telebot_core_response_t
telebot_core_get_user_profile_audios(telebot_core_handler_t core_h, long long int user_id, int offset, int limit)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "user_id", user_id);
    if (offset > 0)
    {
        telebot_request_int(&req, "offset", offset);
    }
    if (limit > 0)
    {
        telebot_request_int(&req, "limit", limit);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_USER_AUDIOS, &req);
}

telebot_core_response_t
telebot_core_set_my_profile_photo(telebot_core_handler_t core_h, const telebot_input_file_t *photo)
{
    telebot_request_t req;
    telebot_request_init(&req);
    if (photo == NULL)
        telebot_request_str_required(&req, "photo", NULL);
    /* InputProfilePhotoStatic: the image is attached as a separate part */
    telebot_request_str(&req, "photo", "{\"type\":\"static\",\"photo\":\"attach://file0\"}");
    telebot_request_upload(&req, "file0", photo);
    return telebot_core_request(core_h, TELEBOT_METHOD_SET_MY_PROFILE_PHOTO, &req);
}

telebot_core_response_t
telebot_core_remove_my_profile_photo(telebot_core_handler_t core_h, const char *photo_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_str(&req, "photo_id", photo_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_REMOVE_MY_PROFILE_PHOTO, &req);
}

telebot_core_response_t
telebot_core_get_business_connection(telebot_core_handler_t core_h, const char *business_connection_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_str(&req, "business_connection_id", business_connection_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_BUSINESS_CONNECTION, &req);
}

telebot_core_response_t
telebot_core_get_user_chat_boosts(telebot_core_handler_t core_h, long long int chat_id, long long int user_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_ll(&req, "user_id", user_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_USER_CHAT_BOOSTS, &req);
}

telebot_core_response_t
telebot_core_set_game_score(telebot_core_handler_t core_h, long long int user_id, int score, bool force,
                            bool disable_edit_message, long long int chat_id, int message_id,
                            const char *inline_message_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "user_id", user_id);
    telebot_request_int(&req, "score", score);
    telebot_request_bool(&req, "force", force);
    telebot_request_bool(&req, "disable_edit_message", disable_edit_message);
    if (chat_id != 0)
    {
        telebot_request_ll(&req, "chat_id", chat_id);
    }
    if (message_id > 0)
    {
        telebot_request_int(&req, "message_id", message_id);
    }
    if (inline_message_id)
    {
        telebot_request_str(&req, "inline_message_id", inline_message_id);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_SET_GAME_SCORE, &req);
}

telebot_core_response_t
telebot_core_get_game_high_scores(telebot_core_handler_t core_h, long long int user_id, long long int chat_id,
                                  int message_id, const char *inline_message_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "user_id", user_id);
    if (chat_id != 0)
    {
        telebot_request_ll(&req, "chat_id", chat_id);
    }
    if (message_id > 0)
    {
        telebot_request_int(&req, "message_id", message_id);
    }
    if (inline_message_id)
    {
        telebot_request_str(&req, "inline_message_id", inline_message_id);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_GAME_HIGH_SCORES, &req);
}

telebot_core_response_t
telebot_core_set_passport_data_errors(telebot_core_handler_t core_h, long long int user_id, const char *errors)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "user_id", user_id);
    telebot_request_str(&req, "errors", errors);
    return telebot_core_request(core_h, TELEBOT_METHOD_SET_PASSPORT_DATA_ERRORS, &req);
}

telebot_core_response_t
telebot_core_answer_shipping_query(telebot_core_handler_t core_h, const char *shipping_query_id, bool ok,
                                   const char *shipping_options, const char *error_message)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_str(&req, "shipping_query_id", shipping_query_id);
    telebot_request_bool(&req, "ok", ok);
    if (shipping_options)
    {
        telebot_request_str(&req, "shipping_options", shipping_options);
    }
    if (error_message)
    {
        telebot_request_str(&req, "error_message", error_message);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_ANSWER_SHIPPING_QUERY, &req);
}

telebot_core_response_t
telebot_core_answer_pre_checkout_query(telebot_core_handler_t core_h, const char *pre_checkout_query_id, bool ok,
                                       const char *error_message)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_str(&req, "pre_checkout_query_id", pre_checkout_query_id);
    telebot_request_bool(&req, "ok", ok);
    if (error_message)
    {
        telebot_request_str(&req, "error_message", error_message);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_ANSWER_PRE_CHECKOUT_QUERY, &req);
}
