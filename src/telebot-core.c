#define _GNU_SOURCE

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

#include <curl/curl.h>
#include <curl/easy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json.h>
#include <json_object.h>
#include <telebot-common.h>
#include <telebot-core.h>
#include <telebot-private.h>
#include <unistd.h>

telebot_error_e telebot_core_get_response_code(telebot_core_response_t response)
{
    if (response)
        return response->ret;
    else
        // response is NULL, probably could not allocate memory
        return TELEBOT_ERROR_OUT_OF_MEMORY;
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

    curl_global_init(CURL_GLOBAL_DEFAULT);

    *core_h = _core_h;
    return TELEBOT_ERROR_NONE;
}

telebot_error_e
telebot_core_destroy(telebot_core_handler_t *core_h)
{
    curl_global_cleanup();

    if ((core_h == NULL) || (*core_h == NULL))
    {
        ERR("Core handler is null");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

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

    core_h->proxy_addr = strdup(addr);
    if (core_h->proxy_addr == NULL)
    {
        ERR("Failed to allocate memorу for proxy address");
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    if (auth != NULL)
    {
        core_h->proxy_auth = strdup(auth);
        if (core_h->proxy_auth == NULL)
        {
            ERR("Failed to allocate memorу for proxy authorization");
            TELEBOT_SAFE_FREE(core_h->proxy_addr);
            return TELEBOT_ERROR_OUT_OF_MEMORY;
        }
    }

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

static size_t write_data_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
    telebot_core_response_t resp = (telebot_core_response_t)userp;
    size_t r_size = size * nmemb;

    char *data = (char *)realloc(resp->data, resp->size + r_size + 1);
    if (data == NULL)
    {
        ERR("Failed to allocate memory, size:%u", (unsigned int)r_size);
        TELEBOT_SAFE_FZCNT(resp->data, resp->size);
        return 0;
    }
    memcpy((data + resp->size), contents, r_size);
    resp->data = data;
    resp->size += r_size;
    resp->data[resp->size] = 0;

    return r_size;
}

static void telebot_core_copy_mime_data_to_part(telebot_core_mime_t *mime, curl_mimepart *part)
{
    curl_mime_name(part, mime->name);

    char buffer[TELEBOT_BUFFER_PAGE];
    switch (mime->type)
    {
    case TELEBOT_MIME_TYPE_CHAR:
        snprintf(buffer, sizeof(buffer), "%c", mime->data.c);
        curl_mime_data(part, buffer, CURL_ZERO_TERMINATED);
        break;
    case TELEBOT_MIME_TYPE_INT:
        snprintf(buffer, sizeof(buffer), "%d", mime->data.d);
        curl_mime_data(part, buffer, CURL_ZERO_TERMINATED);
        break;
    case TELEBOT_MIME_TYPE_U_INT:
        snprintf(buffer, sizeof(buffer), "%u", mime->data.u);
        curl_mime_data(part, buffer, CURL_ZERO_TERMINATED);
        break;
    case TELEBOT_MIME_TYPE_LONG_INT:
        snprintf(buffer, sizeof(buffer), "%ld", mime->data.ld);
        curl_mime_data(part, buffer, CURL_ZERO_TERMINATED);
        break;
    case TELEBOT_MIME_TYPE_U_LONG_INT:
        snprintf(buffer, sizeof(buffer), "%lu", mime->data.lu);
        curl_mime_data(part, buffer, CURL_ZERO_TERMINATED);
        break;
    case TELEBOT_MIME_TYPE_LONG_LONG_INT:
        snprintf(buffer, sizeof(buffer), "%lld", mime->data.lld);
        curl_mime_data(part, buffer, CURL_ZERO_TERMINATED);
        break;
    case TELEBOT_MIME_TYPE_U_LONG_LONG_INT:
        snprintf(buffer, sizeof(buffer), "%llu", mime->data.llu);
        curl_mime_data(part, buffer, CURL_ZERO_TERMINATED);
        break;
    case TELEBOT_MIME_TYPE_FLOAT:
        snprintf(buffer, sizeof(buffer), "%f", mime->data.f);
        curl_mime_data(part, buffer, CURL_ZERO_TERMINATED);
        break;
    case TELEBOT_MIME_TYPE_DOUBLE:
        snprintf(buffer, sizeof(buffer), "%lf", mime->data.lf);
        curl_mime_data(part, buffer, CURL_ZERO_TERMINATED);
        break;
    case TELEBOT_MIME_TYPE_LONG_DOUBLE:
        snprintf(buffer, sizeof(buffer), "%Lf", mime->data.llf);
        curl_mime_data(part, buffer, CURL_ZERO_TERMINATED);
        break;
    case TELEBOT_MIME_TYPE_STRING:
        curl_mime_data(part, mime->data.s, CURL_ZERO_TERMINATED);
        break;
    case TELEBOT_MIME_TYPE_FILE:
        curl_mime_filedata(part, mime->data.s);
        break;
    case TELEBOT_MIME_TYPE_MAX:
    default:
        ERR("Invalid type: %d", mime->type);
        break;
    }
}

static telebot_core_response_t
telebot_core_curl_perform(telebot_core_handler_t core_h, const char *method, telebot_core_mime_t mimes[], size_t size)
{
    CURLcode res;
    CURL *curl_h = NULL;
    curl_mime *mime = NULL;
    long resp_code = 0L;

    telebot_core_response_t resp = calloc(1, sizeof(struct telebot_core_response));
    if (resp == NULL)
    {
        ERR("Failed to allocate memory for response");
        return NULL;
    }

    if (core_h == NULL)
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

    resp->data = (char *)malloc(1);
    resp->size = 0;
    resp->ret = TELEBOT_ERROR_NONE;

    curl_h = curl_easy_init();
    if (curl_h == NULL)
    {
        ERR("Failed to init curl");
        resp->ret = TELEBOT_ERROR_OUT_OF_MEMORY;
        goto finish;
    }

    char URL[TELEBOT_URL_SIZE];
    snprintf(URL, TELEBOT_URL_SIZE, "%s/bot%s/%s", TELEBOT_API_URL, core_h->token, method);
    curl_easy_setopt(curl_h, CURLOPT_URL, URL);
    curl_easy_setopt(curl_h, CURLOPT_WRITEFUNCTION, write_data_cb);
    curl_easy_setopt(curl_h, CURLOPT_WRITEDATA, resp);

    if (core_h->proxy_addr != NULL)
    {
        curl_easy_setopt(curl_h, CURLOPT_PROXY, core_h->proxy_addr);
        if (core_h->proxy_auth != NULL)
        {
            curl_easy_setopt(curl_h, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
            curl_easy_setopt(curl_h, CURLOPT_PROXYUSERPWD, core_h->proxy_auth);
        }
    }

    if (size > 0)
    {
        mime = curl_mime_init(curl_h);
        if (mime == NULL)
        {
            ERR("Failed to create mime");
            resp->ret = TELEBOT_ERROR_OUT_OF_MEMORY;
            goto finish;
        }
        for (size_t index = 0; index < size; index++)
        {
            curl_mimepart *part = curl_mime_addpart(mime);
            if (part == NULL)
            {
                ERR("Failed to create mime part");
                resp->ret = TELEBOT_ERROR_OUT_OF_MEMORY;
                goto finish;
            }
            telebot_core_copy_mime_data_to_part(&mimes[index], part);
        }

        curl_easy_setopt(curl_h, CURLOPT_MIMEPOST, mime);
    }

    res = curl_easy_perform(curl_h);
    if (res != CURLE_OK)
    {
        ERR("Failed to curl_easy_perform\nError: %s (%d)", curl_easy_strerror(res), res);
        resp->ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    curl_easy_getinfo(curl_h, CURLINFO_RESPONSE_CODE, &resp_code);
    if (resp_code != 200L)
    {
        ERR("Wrong HTTP response received, response: %ld", resp_code);
        resp->ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    DBG("Response: %s", resp->data);

finish:
    if (resp->ret != TELEBOT_ERROR_NONE)
        TELEBOT_SAFE_FZCNT(resp->data, resp->size);
    if (mime)
        curl_mime_free(mime);
    if (curl_h)
        curl_easy_cleanup(curl_h);

    return resp;
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_UPDATES, mimes, count);
}

static telebot_core_response_t telebot_core_get_error_response(telebot_error_e ret)
{
    telebot_core_response_t resp = calloc(1, sizeof(struct telebot_core_response));
    if (resp)
        resp->ret = ret;

    return resp;
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
        mimes[count].name = "certificate";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = certificate;
        count++;
    }

    mimes[count].name = "max_connections";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = max_connections;

    if (allowed_updates != NULL)
    {
        mimes[count].name = "allowed_updates";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = allowed_updates;
        count++;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SET_WEBHOOK, mimes, count);
}

telebot_core_response_t
telebot_core_delete_webhook(telebot_core_handler_t core_h)
{
    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_DELETE_WEBHOOK, NULL, 0);
}

telebot_core_response_t
telebot_core_get_webhook_info(telebot_core_handler_t core_h)
{
    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_WEBHOOK_INFO, NULL, 0);
}

telebot_core_response_t
telebot_core_get_me(telebot_core_handler_t core_h)
{
    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_ME, NULL, 0);
}

telebot_core_response_t
telebot_core_send_message(telebot_core_handler_t core_h, long long int chat_id, const char *text, const char *parse_mode,
                          bool disable_web_page_preview, bool disable_notification, int reply_to_message_id,
                          const char *reply_markup)
{
    CHECK_ARG_NULL(text);

    int count = 0;
    telebot_core_mime_t mimes[7]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "text";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = text;
    count++;

    if (parse_mode != NULL)
    {
        mimes[count].name = "parse_mode";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = parse_mode;
        count++;
    }

    mimes[count].name = "disable_notification";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_notification ? "true" : "false";
    count++;

    mimes[count].name = "disable_web_page_preview";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_web_page_preview ? "true" : "false";
    count++;

    if (reply_to_message_id > 0)
    {
        mimes[count].name = "reply_to_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = reply_to_message_id;
        count++;
    }

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_MESSAGE, mimes, count);
}

telebot_core_response_t
telebot_core_forward_message(telebot_core_handler_t core_h, long long int chat_id, long long int from_chat_id,
                             bool disable_notification, int message_id)
{
    if (message_id <= 0)
    {
        ERR("Valid message_id is required.");
        return telebot_core_get_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    int count = 0;
    telebot_core_mime_t mimes[4]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "from_chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = from_chat_id;
    count++;

    mimes[count].name = "disable_notification";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_notification ? "true" : "false";
    count++;

    mimes[count].name = "message_id";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = message_id;
    count++;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_FORWARD_MESSAGE, mimes, count);
}

telebot_core_response_t
telebot_core_send_photo(telebot_core_handler_t core_h, long long int chat_id, const char *photo, bool is_file,
                        const char *caption, const char *parse_mode, bool disable_notification,
                        int reply_to_message_id, const char *reply_markup)
{
    CHECK_ARG_NULL(photo);

    int count = 0;
    telebot_core_mime_t mimes[7]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "photo";
    mimes[count].type = is_file ? TELEBOT_MIME_TYPE_FILE : TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = photo;
    count++;

    if (caption != NULL)
    {
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
    }

    mimes[count].name = "disable_notification";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_notification ? "true" : "false";
    count++;

    if (reply_to_message_id > 0)
    {
        mimes[count].name = "reply_to_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = reply_to_message_id;
        count++;
    }

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_PHOTO, mimes, count);
}

telebot_core_response_t
telebot_core_send_audio(telebot_core_handler_t core_h, long long int chat_id, const char *audio, bool is_file,
                        const char *caption, const char *parse_mode, int duration, const char *performer,
                        const char *title, const char *thumb, bool disable_notification, int reply_to_message_id,
                        const char *reply_markup)
{
    CHECK_ARG_NULL(audio);

    int count = 0;
    telebot_core_mime_t mimes[11]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "audio";
    mimes[count].type = is_file ? TELEBOT_MIME_TYPE_FILE : TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = audio;
    count++;

    if (caption != NULL)
    {
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
    }

    if (duration > 0)
    {
        mimes[count].name = "duration";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = duration;
        count++;
    }

    if (performer != NULL)
    {
        mimes[count].name = "performer";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = performer;
        count++;
    }

    if (title != NULL)
    {
        mimes[count].name = "title";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = title;
        count++;
    }

    if (thumb != NULL)
    {
        mimes[count].name = "thumb";
        mimes[count].type = TELEBOT_MIME_TYPE_FILE;
        mimes[count].data.s = thumb;
        count++;
    }

    mimes[count].name = "disable_notification";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_notification ? "true" : "false";
    count++;

    if (reply_to_message_id > 0)
    {
        mimes[count].name = "reply_to_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = reply_to_message_id;
        count++;
    }

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_AUDIO, mimes, count);
}

telebot_core_response_t
telebot_core_send_document(telebot_core_handler_t core_h, long long int chat_id, const char *document, bool is_file,
                           const char *thumb, const char *caption, const char *parse_mode, bool disable_notification,
                           int reply_to_message_id, const char *reply_markup)
{
    CHECK_ARG_NULL(document);

    int count = 0;
    telebot_core_mime_t mimes[8]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "document";
    mimes[count].type = is_file ? TELEBOT_MIME_TYPE_FILE : TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = document;
    count++;

    if (thumb != NULL)
    {
        mimes[count].name = "thumb";
        mimes[count].type = TELEBOT_MIME_TYPE_FILE;
        mimes[count].data.s = thumb;
        count++;
    }

    if (caption != NULL)
    {
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
    }

    mimes[count].name = "disable_notification";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_notification ? "true" : "false";
    count++;

    if (reply_to_message_id > 0)
    {
        mimes[count].name = "reply_to_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = reply_to_message_id;
        count++;
    }

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_DOCUMENT, mimes, count);
}

telebot_core_response_t
telebot_core_send_video(telebot_core_handler_t core_h, long long int chat_id, const char *video, bool is_file,
                        int duration, int width, int height, const char *thumb, const char *caption,
                        const char *parse_mode, bool supports_streaming, bool disable_notification,
                        int reply_to_message_id, const char *reply_markup)
{
    CHECK_ARG_NULL(video);

    int count = 0;
    telebot_core_mime_t mimes[12]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "video";
    mimes[count].type = is_file ? TELEBOT_MIME_TYPE_FILE : TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = video;
    count++;

    if (duration > 0)
    {
        mimes[count].name = "duration";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = duration;
        count++;
    }

    if (width > 0)
    {
        mimes[count].name = "width";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = width;
        count++;
    }

    if (height > 0)
    {
        mimes[count].name = "height";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = height;
        count++;
    }

    if (thumb != NULL)
    {
        mimes[count].name = "thumb";
        mimes[count].type = TELEBOT_MIME_TYPE_FILE;
        mimes[count].data.s = thumb;
        count++;
    }

    if (caption != NULL)
    {
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
    }

    mimes[count].name = "supports_streaming";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = supports_streaming ? "true" : "false";
    count++;

    mimes[count].name = "disable_notification";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_notification ? "true" : "false";
    count++;

    if (reply_to_message_id > 0)
    {
        mimes[count].name = "reply_to_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = reply_to_message_id;
        count++;
    }

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_VIDEO, mimes, count);
}

telebot_core_response_t
telebot_core_send_animation(telebot_core_handler_t core_h, long long int chat_id, const char *animation, bool is_file,
                            int duration, int width, int height, const char *thumb, const char *caption,
                            const char *parse_mode, bool disable_notification, int reply_to_message_id,
                            const char *reply_markup)
{
    CHECK_ARG_NULL(animation);

    int count = 0;
    telebot_core_mime_t mimes[11]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "animation";
    mimes[count].type = is_file ? TELEBOT_MIME_TYPE_FILE : TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = animation;
    count++;

    if (duration > 0)
    {
        mimes[count].name = "duration";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = duration;
        count++;
    }

    if (width > 0)
    {
        mimes[count].name = "width";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = width;
        count++;
    }

    if (height > 0)
    {
        mimes[count].name = "height";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = height;
        count++;
    }

    if (thumb != NULL)
    {
        mimes[count].name = "thumb";
        mimes[count].type = TELEBOT_MIME_TYPE_FILE;
        mimes[count].data.s = thumb;
        count++;
    }

    if (caption != NULL)
    {
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
    }

    mimes[count].name = "disable_notification";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_notification ? "true" : "false";
    count++;

    if (reply_to_message_id > 0)
    {
        mimes[count].name = "reply_to_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = reply_to_message_id;
        count++;
    }

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_ANIMATION, mimes, count);
}

telebot_core_response_t
telebot_core_send_voice(telebot_core_handler_t core_h, long long int chat_id, const char *voice, bool is_file,
                        const char *caption, const char *parse_mode, int duration, bool disable_notification,
                        int reply_to_message_id, const char *reply_markup)
{
    CHECK_ARG_NULL(voice);

    int count = 0;
    telebot_core_mime_t mimes[8]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "voice";
    mimes[count].type = is_file ? TELEBOT_MIME_TYPE_FILE : TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = voice;
    count++;

    if (duration > 0)
    {
        mimes[count].name = "duration";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = duration;
        count++;
    }

    if (caption != NULL)
    {
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
    }

    mimes[count].name = "disable_notification";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_notification ? "true" : "false";
    count++;

    if (reply_to_message_id > 0)
    {
        mimes[count].name = "reply_to_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = reply_to_message_id;
        count++;
    }

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_VOICE, mimes, count);
}

telebot_core_response_t
telebot_core_send_video_note(telebot_core_handler_t core_h, long long int chat_id, char *video_note, bool is_file,
                             int duration, int length, const char *thumb, bool disable_notification,
                             int reply_to_message_id, const char *reply_markup)
{
    CHECK_ARG_NULL(video_note);

    int count = 0;
    telebot_core_mime_t mimes[8]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count]
        .name = "video_note";
    mimes[count].type = is_file ? TELEBOT_MIME_TYPE_FILE : TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = video_note;
    count++;

    if (duration > 0)
    {
        mimes[count].name = "duration";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = duration;
        count++;
    }

    if (length > 0)
    {
        mimes[count].name = "length";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = length;
        count++;
    }

    if (thumb != NULL)
    {
        mimes[count].name = "thumb";
        mimes[count].type = TELEBOT_MIME_TYPE_FILE;
        mimes[count].data.s = thumb;
        count++;
    }

    mimes[count].name = "disable_notification";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_notification ? "true" : "false";
    count++;

    if (reply_to_message_id > 0)
    {
        mimes[count].name = "reply_to_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = reply_to_message_id;
        count++;
    }

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_VIDEO_NOTE, mimes, count);
}

// Helper function to determine media type based on file extension
static const char *telebot_core_get_media_type(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    if (ext == NULL)
        return "document"; // No extension, treat as document

    ext++; // Skip the dot

    // Convert to lowercase for comparison
    char ext_lower[10];
    int i = 0;
    while (ext[i] != '\0' && i < 9)
    {
        ext_lower[i] = (ext[i] >= 'A' && ext[i] <= 'Z') ? ext[i] - 'A' + 'a' : ext[i];
        i++;
    }
    ext_lower[i] = '\0';

    // Check for photo extensions
    if (strcmp(ext_lower, "jpg") == 0 || strcmp(ext_lower, "jpeg") == 0 ||
        strcmp(ext_lower, "png") == 0 || strcmp(ext_lower, "bmp") == 0 ||
        strcmp(ext_lower, "tiff") == 0 || strcmp(ext_lower, "webp") == 0)
    {
        return "photo";
    }
    // Check for video extensions
    else if (strcmp(ext_lower, "mp4") == 0 || strcmp(ext_lower, "mpeg") == 0 ||
             strcmp(ext_lower, "avi") == 0 || strcmp(ext_lower, "mov") == 0 ||
             strcmp(ext_lower, "mkv") == 0 || strcmp(ext_lower, "wmv") == 0 ||
             strcmp(ext_lower, "flv") == 0 || strcmp(ext_lower, "webm") == 0 ||
             strcmp(ext_lower, "3gp") == 0 || strcmp(ext_lower, "m4v") == 0)
    {
        return "video";
    }
    // Check for audio extensions
    else if (strcmp(ext_lower, "mp3") == 0 || strcmp(ext_lower, "m4a") == 0 ||
             strcmp(ext_lower, "flac") == 0 || strcmp(ext_lower, "ogg") == 0 ||
             strcmp(ext_lower, "oga") == 0 || strcmp(ext_lower, "wav") == 0 ||
             strcmp(ext_lower, "aac") == 0 || strcmp(ext_lower, "opus") == 0)
    {
        return "audio";
    }
    // Everything else is treated as document (including gif)
    else
    {
        return "document";
    }
}

telebot_core_response_t
telebot_core_send_media_group(telebot_core_handler_t core_h, long long int chat_id, char *media_paths[], int path_count,
                              bool disable_notification, int reply_to_message_id)
{
    CHECK_ARG_NULL(media_paths);
    CHECK_ARG_CONDITION(path_count <= 0, "Invalid media path count, should be greater than 0");
    CHECK_ARG_CONDITION(path_count > 10, "Invalid media path count, should be less than or equal to 10")

    // Validate all media paths are non-NULL
    for (int i = 0; i < path_count; ++i)
    {
        if (media_paths[i] == NULL)
        {
            ERR("Invalid media path at index %d is null", i);
            return telebot_core_get_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
        }
    }

    // Build media array using json-c
    struct json_object *media_array = json_object_new_array();
    if (media_array == NULL)
    {
        ERR("Failed to create JSON media array");
        return telebot_core_get_error_response(TELEBOT_ERROR_OUT_OF_MEMORY);
    }

    // Allocate memory for filenames
    char **filenames = calloc(path_count, sizeof(char *));
    if (filenames == NULL)
    {
        json_object_put(media_array);
        ERR("Failed to allocate memory for filenames");
        return telebot_core_get_error_response(TELEBOT_ERROR_OUT_OF_MEMORY);
    }

    // Determine media types for validation
    const char **media_types = calloc(path_count, sizeof(char *));
    if (media_types == NULL)
    {
        for (int i = 0; i < path_count; i++)
        {
            free(filenames[i]);
        }
        free(filenames);
        json_object_put(media_array);
        ERR("Failed to allocate memory for media types");
        return telebot_core_get_error_response(TELEBOT_ERROR_OUT_OF_MEMORY);
        ;
    }

    for (int i = 0; i < path_count; ++i)
    {
        // Extract filename from path using basename
        const char *filename = basename(media_paths[i]);

        // Allocate memory for filename
        filenames[i] = strdup(filename);
        if (filenames[i] == NULL)
        {
            // Free previously allocated resources
            for (int j = 0; j < i; j++)
            {
                free(filenames[j]);
            }
            free(filenames);
            free(media_types);
            json_object_put(media_array);
            ERR("Failed to duplicate filename");
            return telebot_core_get_error_response(TELEBOT_ERROR_OUT_OF_MEMORY);
            ;
        }

        // Determine media type
        media_types[i] = telebot_core_get_media_type(filename);
    }

    // Validate media group composition
    // Count unique types in the group
    int photo_count = 0, video_count = 0, audio_count = 0, document_count = 0;
    for (int i = 0; i < path_count; i++)
    {
        if (strcmp(media_types[i], "photo") == 0)
            photo_count++;
        else if (strcmp(media_types[i], "video") == 0)
            video_count++;
        else if (strcmp(media_types[i], "audio") == 0)
            audio_count++;
        else if (strcmp(media_types[i], "document") == 0)
            document_count++;
    }

    // Check valid combinations:
    // 1. All of the same type
    // 2. Mixed photo and video only
    bool valid_combination = false;

    if (photo_count == path_count || video_count == path_count || audio_count == path_count || document_count == path_count)
    {
        // All same type - valid
        valid_combination = true;
    }
    else if (photo_count > 0 && video_count > 0 && audio_count == 0 && document_count == 0)
    {
        // Mixed photo and video only - valid
        valid_combination = true;
    }

    if (!valid_combination)
    {
        // Free allocated resources
        for (int i = 0; i < path_count; i++)
        {
            free(filenames[i]);
        }
        free(filenames);
        free(media_types);
        json_object_put(media_array);
        ERR("Invalid media group composition: only homogeneous groups or mixed photo/video groups are allowed");
        return telebot_core_get_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    // Create JSON objects for media array
    for (int i = 0; i < path_count; ++i)
    {
        struct json_object *item = json_object_new_object();
        json_object_object_add(item, "type", json_object_new_string(media_types[i]));

        // Create attach:// reference using snprintf instead of asprintf
        char attach_ref[256]; // Sufficient size for "attach://" + filename
        snprintf(attach_ref, sizeof(attach_ref), "attach://%s", filenames[i]);
        json_object_object_add(item, "media", json_object_new_string(attach_ref));
        json_object_array_add(media_array, item);
    }

    // Free temporary media types array
    free(media_types);

    const char *media_json_str = json_object_to_json_string(media_array);
    if (media_json_str == NULL)
    {
        // Free allocated filenames
        for (int i = 0; i < path_count; i++)
        {
            free(filenames[i]);
        }
        free(filenames);
        json_object_put(media_array);
        ERR("Failed to serialize media JSON");
        return telebot_core_get_error_response(TELEBOT_ERROR_OPERATION_FAILED);
    }

    // Prepare MIME parts
    telebot_core_mime_t mimes[20]; // max: chat_id + media + disable_notif + reply_id + 10 files
    int count = 0;

    // chat_id
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    // media (JSON string)
    mimes[count].name = "media";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = media_json_str;
    count++;

    // disable_notification
    mimes[count].name = "disable_notification";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_notification ? "true" : "false";
    count++;

    // reply_to_message_id (optional)
    if (reply_to_message_id > 0)
    {
        mimes[count].name = "reply_to_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = reply_to_message_id;
        count++;
    }

    // Attach actual photo files using the correct names
    for (int i = 0; i < path_count; ++i)
    {
        mimes[count].name = filenames[i]; // Use actual filename instead of generated name
        mimes[count].type = TELEBOT_MIME_TYPE_FILE;
        mimes[count].data.s = media_paths[i];
        count++;
    }

    // Perform request
    telebot_core_response_t response = telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_MEDIA_GROUP, mimes, count);

    // Clean up allocated filenames
    for (int i = 0; i < path_count; i++)
    {
        free(filenames[i]);
    }
    free(filenames);

    // Clean up JSON object
    json_object_put(media_array);

    return response;
}

telebot_core_response_t
telebot_core_send_location(telebot_core_handler_t core_h, long long int chat_id, float latitude, float longitude,
                           int live_period, bool disable_notification, int reply_to_message_id,
                           const char *reply_markup)
{
    int count = 0;
    telebot_core_mime_t mimes[7]; // number of arguments
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

    if (live_period > 0)
    {
        mimes[count].name = "live_period";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = live_period;
        count++;
    }

    mimes[count].name = "disable_notification";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_notification ? "true" : "false";
    count++;

    if (reply_to_message_id > 0)
    {
        mimes[count].name = "reply_to_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = reply_to_message_id;
        count++;
    }

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_LOCATION, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_EDIT_MESSAGE_LIVE_LOCATION, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_STOP_MESSAGE_LIVE_LOCATION, mimes, count);
}

telebot_core_response_t
telebot_core_send_venue(telebot_core_handler_t core_h, long long int chat_id, float latitude, float longitude,
                        const char *title, const char *address, const char *foursquare_id, const char *foursquare_type,
                        bool disable_notification, int reply_to_message_id, const char *reply_markup)
{
    CHECK_ARG_NULL(title);
    CHECK_ARG_NULL(address);

    int count = 0;
    telebot_core_mime_t mimes[10]; // number of arguments
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

    mimes[count].name = "title";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = title;
    count++;

    mimes[count].name = "address";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = address;
    count++;

    if (foursquare_id != NULL)
    {
        mimes[count].name = "foursquare_id";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = foursquare_id;
        count++;
    }

    if (foursquare_type != NULL)
    {
        mimes[count].name = "foursquare_type";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = foursquare_type;
        count++;
    }

    mimes[count].name = "disable_notification";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_notification ? "true" : "false";
    count++;

    if (reply_to_message_id > 0)
    {
        mimes[count].name = "reply_to_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = reply_to_message_id;
        count++;
    }

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_VENUE, mimes, count);
}

telebot_core_response_t
telebot_core_send_contact(telebot_core_handler_t core_h, long long int chat_id, const char *phone_number,
                          const char *first_name, const char *last_name, const char *vcard, bool disable_notification,
                          int reply_to_message_id, const char *reply_markup)
{
    CHECK_ARG_NULL(phone_number);
    CHECK_ARG_NULL(first_name);

    int count = 0;
    telebot_core_mime_t mimes[8]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "phone_number";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = phone_number;
    count++;

    mimes[count].name = "first_name";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = first_name;
    count++;

    if (last_name != NULL)
    {
        mimes[count].name = "last_name";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = last_name;
        count++;
    }

    if (vcard != NULL)
    {
        mimes[count].name = "vcard";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = vcard;
        count++;
    }

    mimes[count].name = "disable_notification";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_notification ? "true" : "false";
    count++;

    if (reply_to_message_id > 0)
    {
        mimes[count].name = "reply_to_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = reply_to_message_id;
        count++;
    }

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_CONTACT, mimes, count);
}

telebot_core_response_t
telebot_core_send_poll(telebot_core_handler_t core_h, long long int chat_id, const char *question, const char *options,
                       bool is_anonymous, const char *type, bool allows_multiple_answers, int correct_option_id,
                       bool is_closed, bool disable_notification, int reply_to_message_id, const char *reply_markup)
{
    CHECK_ARG_NULL(question);
    CHECK_ARG_NULL(options);

    int count = 0;
    telebot_core_mime_t mimes[11]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "question";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = question;
    count++;

    mimes[count].name = "options";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = options;
    count++;

    mimes[count].name = "is_anonymous";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = is_anonymous ? "true" : "false";
    count++;

    if (type != NULL)
    {
        mimes[count].name = "type";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = type;
        count++;
    }

    mimes[count].name = "allows_multiple_answers";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = allows_multiple_answers ? "true" : "false";
    count++;

    if (correct_option_id >= 0)
    {
        mimes[count].name = "correct_option_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = correct_option_id;
        count++;
    }

    mimes[count].name = "is_closed";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = is_closed ? "true" : "false";
    count++;

    mimes[count].name = "disable_notification";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_notification ? "true" : "false";
    count++;

    if (reply_to_message_id > 0)
    {
        mimes[count].name = "reply_to_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = reply_to_message_id;
        count++;
    }

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_POLL, mimes, count);
}

telebot_core_response_t
telebot_core_send_dice(telebot_core_handler_t core_h, long long int chat_id, bool disable_notification,
                       int reply_to_message_id, const char *reply_markup)
{
    int count = 0;
    telebot_core_mime_t mimes[4]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "disable_notification";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_notification ? "true" : "false";
    count++;

    if (reply_to_message_id > 0)
    {
        mimes[count].name = "reply_to_message_id";
        mimes[count].type = TELEBOT_MIME_TYPE_INT;
        mimes[count].data.d = reply_to_message_id;
        count++;
    }

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_DICE, mimes, count);
}

telebot_core_response_t
telebot_core_send_chat_action(telebot_core_handler_t core_h, long long int chat_id, const char *action)
{
    CHECK_ARG_NULL(action);

    int count = 0;
    telebot_core_mime_t mimes[2]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "action";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = action;
    count++;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_CHAT_ACTION, mimes, count);
}

telebot_core_response_t
telebot_core_get_user_profile_photos(telebot_core_handler_t core_h, int user_id, int offset, int limit)
{
    CHECK_ARG_CONDITION((user_id <= 0), "Invalid value of user_id");

    int count = 0;
    telebot_core_mime_t mimes[3]; // number of arguments
    mimes[count].name = "user_id";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = user_id;
    count++;

    mimes[count].name = "offset";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = offset;
    count++;

    mimes[count].name = "limit";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = limit;
    count++;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_USER_PHOTOS, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_FILE, mimes, count);
}

static size_t
write_file_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t written = fwrite(contents, size, nmemb, (FILE *)userp);
    return written;
}

telebot_error_e
telebot_core_download_file(telebot_core_handler_t core_h, const char *file_path, const char *out_file)
{
    int ret = TELEBOT_ERROR_NONE;
    if ((core_h == NULL) || (core_h->token == NULL) || (file_path == NULL) || (out_file == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    CURL *curl_h = NULL;
    CURLcode res;
    long resp_code = 0L;

    FILE *fp = fopen(out_file, "w");
    if (fp == NULL)
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    curl_h = curl_easy_init();
    if (curl_h == NULL)
    {
        ret = TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    char URL[TELEBOT_URL_SIZE];
    snprintf(URL, TELEBOT_URL_SIZE, "%s/file/bot%s/%s", TELEBOT_API_URL,
             core_h->token, file_path);

    curl_easy_setopt(curl_h, CURLOPT_URL, URL);
    curl_easy_setopt(curl_h, CURLOPT_WRITEFUNCTION, write_file_cb);
    curl_easy_setopt(curl_h, CURLOPT_WRITEDATA, fp);

    res = curl_easy_perform(curl_h);
    if (res != CURLE_OK)
    {
        ERR("Failed to curl_easy_perform\nError: %s (%d)",
            curl_easy_strerror(res), res);
        curl_easy_cleanup(curl_h);
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    curl_easy_getinfo(curl_h, CURLINFO_RESPONSE_CODE, &resp_code);
    if (resp_code != 200L)
    {
        ERR("Wrong HTTP response received, response: %ld", resp_code);
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

finish:
    if (fp)
        fclose(fp);
    if (ret != TELEBOT_ERROR_NONE)
        unlink(out_file);
    if (curl_h)
        curl_easy_cleanup(curl_h);

    return ret;
}

telebot_core_response_t
telebot_core_kick_chat_member(telebot_core_handler_t core_h, long long int chat_id, int user_id, long until_date)
{
    CHECK_ARG_CONDITION((user_id <= 0), "Valid user_id is required");

    int count = 0;
    telebot_core_mime_t mimes[3]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "user_id";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = user_id;
    count++;

    if (until_date > 0)
    {
        mimes[count].name = "until_date";
        mimes[count].type = TELEBOT_MIME_TYPE_LONG_INT;
        mimes[count].data.ld = until_date;
        count++;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_KICK_CHAT_MEMBER, mimes, count);
}

telebot_core_response_t
telebot_core_unban_chat_member(telebot_core_handler_t core_h, long long int chat_id, int user_id)
{
    CHECK_ARG_CONDITION((user_id <= 0), "Valid user_id is required");

    int count = 0;
    telebot_core_mime_t mimes[2]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "user_id";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = user_id;
    count++;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_UNBAN_CHAT_MEMBER, mimes, count);
}

telebot_core_response_t
telebot_core_restrict_chat_member(telebot_core_handler_t core_h, long long int chat_id, int user_id, long until_date,
                                  bool can_send_messages, bool can_send_media_messages, bool can_send_polls,
                                  bool can_send_other_messages, bool can_add_web_page_previews, bool can_change_info,
                                  bool can_invite_users, bool can_pin_messages)
{
    CHECK_ARG_CONDITION((user_id <= 0), "Valid user_id is required");

    int count = 0;
    telebot_core_mime_t mimes[11]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "user_id";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = user_id;
    count++;

    if (until_date > 0)
    {
        mimes[count].name = "until_date";
        mimes[count].type = TELEBOT_MIME_TYPE_LONG_INT;
        mimes[count].data.ld = until_date;
        count++;
    }

    mimes[count].name = "can_send_messages";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_send_messages ? "true" : "false";
    count++;

    mimes[count].name = "can_send_media_messages";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_send_media_messages ? "true" : "false";
    count++;

    mimes[count].name = "can_send_polls";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_send_polls ? "true" : "false";
    count++;

    mimes[count].name = "can_send_other_messages";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_send_other_messages ? "true" : "false";
    count++;

    mimes[count].name = "can_add_web_page_previews";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_add_web_page_previews ? "true" : "false";
    count++;

    mimes[count].name = "can_change_info";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_change_info ? "true" : "false";
    count++;

    mimes[count].name = "can_invite_users";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_invite_users ? "true" : "false";
    count++;

    mimes[count].name = "can_pin_messages";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_invite_users ? "true" : "false";
    count++;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_RESTRICT_CHAT_MEMBER, mimes, count);
}

telebot_core_response_t
telebot_core_promote_chat_member(telebot_core_handler_t core_h, long long int chat_id, int user_id, bool can_change_info,
                                 bool can_post_messages, bool can_edit_messages, bool can_delete_messages,
                                 bool can_invite_users, bool can_restrict_members, bool can_pin_messages,
                                 bool can_promote_members)
{
    CHECK_ARG_CONDITION((user_id <= 0), "Valid user_id is required");

    int count = 0;
    telebot_core_mime_t mimes[10]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "user_id";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = user_id;
    count++;

    mimes[count].name = "can_change_info";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_change_info ? "true" : "false";
    count++;

    mimes[count].name = "can_post_messages";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_post_messages ? "true" : "false";
    count++;

    mimes[count].name = "can_edit_messages";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_edit_messages ? "true" : "false";
    count++;

    mimes[count].name = "can_delete_messages";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_delete_messages ? "true" : "false";
    count++;

    mimes[count].name = "can_invite_users";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_invite_users ? "true" : "false";
    count++;

    mimes[count].name = "can_restrict_members";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_restrict_members ? "true" : "false";
    count++;

    mimes[count].name = "can_pin_messages";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_pin_messages ? "true" : "false";
    count++;

    mimes[count].name = "can_promote_members";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_promote_members ? "true" : "false";
    count++;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_PROMOTE_CHAT_MEMBER, mimes, count);
}

telebot_core_response_t
telebot_core_set_chat_admin_custom_title(telebot_core_handler_t core_h, long long int chat_id, int user_id,
                                         const char *custom_title)
{
    CHECK_ARG_CONDITION((user_id <= 0), "Valid user_id is required");

    int count = 0;
    telebot_core_mime_t mimes[3]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "user_id";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = user_id;
    count++;

    mimes[count].name = "custom_title";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = custom_title;
    count++;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SET_CHAT_ADMIN_TITLE, mimes, count);
}

telebot_core_response_t
telebot_core_set_chat_permissions(telebot_core_handler_t core_h, long long int chat_id, bool can_send_messages,
                                  bool can_send_media_messages, bool can_send_polls, bool can_send_other_messages,
                                  bool can_add_web_page_previews, bool can_change_info, bool can_invite_users,
                                  bool can_pin_messages)
{
    int count = 0;
    telebot_core_mime_t mimes[9]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "can_send_messages";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_send_messages ? "true" : "false";
    count++;

    mimes[count].name = "can_send_media_messages";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_send_media_messages ? "true" : "false";
    count++;

    mimes[count].name = "can_send_polls";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_send_polls ? "true" : "false";
    count++;

    mimes[count].name = "can_send_other_messages";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_send_other_messages ? "true" : "false";
    count++;

    mimes[count].name = "can_add_web_page_previews";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_add_web_page_previews ? "true" : "false";
    count++;

    mimes[count].name = "can_change_info";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_change_info ? "true" : "false";
    count++;

    mimes[count].name = "can_invite_users";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_invite_users ? "true" : "false";
    count++;

    mimes[count].name = "can_pin_messages";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = can_invite_users ? "true" : "false";
    count++;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SET_CHAT_PERMISSIONS, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_EXPORT_CHAT_INVITE_LINK, mimes, count);
}

telebot_core_response_t
telebot_core_set_chat_photo(telebot_core_handler_t core_h, long long int chat_id, const char *photo)
{
    CHECK_ARG_NULL(photo);

    int count = 0;
    telebot_core_mime_t mimes[2]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "photo";
    mimes[count].type = TELEBOT_MIME_TYPE_FILE;
    mimes[count].data.s = photo;
    count++;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SET_CHAT_PHOTO, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_DELETE_CHAT_PHOTO, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SET_CHAT_TITLE, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SET_CHAT_DESCRIPTION, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_PIN_CHAT_MESSAGE, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_UNPIN_CHAT_MESSAGE, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_LEAVE_CHAT, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_CHAT, mimes, count);
}

telebot_core_response_t
telebot_core_get_chat_admins(telebot_core_handler_t core_h, long long int chat_id)
{
    CHECK_ARG_CONDITION((chat_id <= 0), "Valid chat id is required");

    int count = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_CHAT_ADMINS, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_CHAT_MEMBERS_COUNT, mimes, count);
}

telebot_core_response_t
telebot_core_get_chat_member(telebot_core_handler_t core_h, long long int chat_id, int user_id)
{
    CHECK_ARG_CONDITION((chat_id <= 0), "Valid chat id is required");
    CHECK_ARG_CONDITION((user_id <= 0), "Valid user_id is required");

    int count = 0;
    telebot_core_mime_t mimes[2]; // number of arguments
    mimes[count].name = "chat_id";
    mimes[count].type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    mimes[count].data.lld = chat_id;
    count++;

    mimes[count].name = "user_id";
    mimes[count].type = TELEBOT_MIME_TYPE_INT;
    mimes[count].data.d = user_id;
    count++;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_CHAT_MEMBER, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SET_CHAT_STICKER_SET, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_DEL_CHAT_STICKER_SET, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_ANSWER_CALLBACK_QUERY, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SET_MY_COMMANDS, mimes, count);
}

telebot_core_response_t
telebot_core_get_my_commands(telebot_core_handler_t core_h)
{
    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_MY_COMMANDS, NULL, 0);
}

telebot_core_response_t
telebot_core_edit_message_text(telebot_core_handler_t core_h, long long int chat_id, int message_id,
                               const char *inline_message_id, const char *text, const char *parse_mode,
                               bool disable_web_page_preview, const char *reply_markup)
{
    if (((chat_id == 0) || (message_id <= 0)) && (inline_message_id == NULL))
    {
        ERR("Either valid chat_id & message_id or inline_message_id required");
        return telebot_core_get_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    int count = 0;
    telebot_core_mime_t mimes[7]; // number of arguments
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

    mimes[count].name = "text";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = text;
    count++;

    if (parse_mode != NULL)
    {
        mimes[count].name = "parse_mode";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = parse_mode;
        count++;
    }

    mimes[count].name = "disable_web_page_preview";
    mimes[count].type = TELEBOT_MIME_TYPE_STRING;
    mimes[count].data.s = disable_web_page_preview ? "true" : "false";
    count++;

    if (reply_markup != NULL)
    {
        mimes[count].name = "reply_markup";
        mimes[count].type = TELEBOT_MIME_TYPE_STRING;
        mimes[count].data.s = reply_markup;
        count++;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_EDIT_MESSAGE_TEXT, mimes, count);
}

telebot_core_response_t
telebot_core_edit_message_caption(telebot_core_handler_t core_h, long long int chat_id, int message_id,
                                  const char *inline_message_id, const char *caption, const char *parse_mode,
                                  const char *reply_markup)
{
    if (((chat_id == 0) || (message_id <= 0)) && (inline_message_id == NULL))
    {
        ERR("Either valid chat_id & message_id or inline_message_id required");
        return telebot_core_get_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_EDIT_MESSAGE_CAPTION, mimes, count);
}

telebot_core_response_t
telebot_core_edit_message_reply_markup(telebot_core_handler_t core_h, long long int chat_id, int message_id,
                                       const char *inline_message_id, const char *reply_markup)
{
    if (((chat_id == 0) || (message_id <= 0)) && (inline_message_id == NULL))
    {
        ERR("Either valid chat_id & message_id or inline_message_id required");
        return telebot_core_get_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_EDIT_MESSAGE_REPLY_MARKUP, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_STOP_POLL, mimes, count);
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

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_DELETE_MESSAGE, mimes, count);
}
