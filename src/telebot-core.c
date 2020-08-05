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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <telebot-private.h>
#include <telebot-common.h>
#include <telebot-core.h>

void telebot_core_put_response(telebot_core_response_t *response)
{
    if (response)
    {
        TELEBOT_SAFE_FZCNT(response->data, response->size);
    }
}

static size_t write_data_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
    telebot_core_response_t *resp = (telebot_core_response_t *)userp;
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

static telebot_error_e telebot_core_curl_perform(telebot_core_handler_t *core_h,
    const char *method, telebot_core_mime_t mimes[], size_t size,
    telebot_core_response_t *resp)
{
    CURLcode res;
    CURL *curl_h = NULL;
    curl_mime *mime = NULL;
    long resp_code = 0L;
    int ret = TELEBOT_ERROR_NONE;

    if (resp == NULL)
    {
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }
    resp->data = (char *)malloc(1);
    resp->size = 0;

    curl_h = curl_easy_init();
    if (curl_h == NULL)
    {
        ERR("Failed to init curl");
        ret = TELEBOT_ERROR_OUT_OF_MEMORY;
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
            ret = TELEBOT_ERROR_OUT_OF_MEMORY;
            goto finish;
        }
        for (int index = 0; index < size; index++)
        {
            curl_mimepart *part = curl_mime_addpart(mime);
            if (part == NULL)
            {
                ERR("Failed to create mime part");
                ret = TELEBOT_ERROR_OUT_OF_MEMORY;
                goto finish;
            }
            curl_mime_name(part, mimes[index].name);
            if (mimes[index].type == TELEBOT_MIME_TYPE_FILE)
                curl_mime_filedata(part, mimes[index].data);
            else
                curl_mime_data(part, mimes[index].data, CURL_ZERO_TERMINATED);
        }

        curl_easy_setopt(curl_h, CURLOPT_MIMEPOST, mime);
    }

    res = curl_easy_perform(curl_h);
    if (res != CURLE_OK)
    {
        ERR("Failed to curl_easy_perform\nError: %s (%d)", curl_easy_strerror(res), res);
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

    DBG("Response: %s", resp->data);

finish:
    if (ret != TELEBOT_ERROR_NONE)
        TELEBOT_SAFE_FZCNT(resp->data, resp->size);
    if (mime)
        curl_mime_free(mime);
    if (curl_h)
        curl_easy_cleanup(curl_h);

    return ret;
}

telebot_error_e telebot_core_create(telebot_core_handler_t **core_h, const char *token)
{
    if ((token == NULL) || (core_h == NULL))
    {
        ERR("Token(0x%p) or core_h(0x%p) is NULL", token, core_h);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }
    *core_h = NULL;

    telebot_core_handler_t *_core_h = malloc(sizeof(telebot_core_handler_t));
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

telebot_error_e telebot_core_destroy(telebot_core_handler_t **core_h)
{
    curl_global_cleanup();

    if ((core_h == NULL) || (*core_h == NULL))
    {
        ERR("Handler is NULL");
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

telebot_error_e telebot_core_set_proxy(telebot_core_handler_t *core_h,
    const char *addr, const char *auth)
{
    if ((addr == NULL) || (core_h == NULL))
    {
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    core_h->proxy_addr = strdup(addr);
    if (core_h->proxy_addr == NULL)
    {
        ERR("Failed to allocate memor for proxy address");
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    if (auth != NULL)
    {
        core_h->proxy_auth = strdup(auth);
        if (core_h->proxy_auth == NULL)
        {
            ERR("Failed to allocate memor for proxy authorization");
            TELEBOT_SAFE_FREE(core_h->proxy_addr);
            return TELEBOT_ERROR_OUT_OF_MEMORY;
        }
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_core_get_proxy(telebot_core_handler_t *core_h, char **addr)
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

telebot_error_e telebot_core_get_updates(telebot_core_handler_t *core_h,
    int offset, int limit, int timeout, const char *allowed_updates,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (limit > TELEBOT_UPDATE_COUNT_MAX_LIMIT)
        limit = TELEBOT_UPDATE_COUNT_MAX_LIMIT;

    int index = 0;
    telebot_core_mime_t mimes[4]; // number of arguments
    mimes[index].name = "offset";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", offset);
    ++index;

    mimes[index].name = "limit";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", limit);
    ++index;

    mimes[index].name = "timeout";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", timeout);
    ++index;

    if (allowed_updates)
    {
        mimes[index].name = "allowed_updates";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", allowed_updates);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_UPDATES, mimes, index, response);
}

telebot_error_e telebot_core_set_webhook(telebot_core_handler_t *core_h, const char *url,
    const char *certificate, int max_connections, const char *allowed_updates,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (url == NULL))
    {
        ERR("Handler, token, or url is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[4]; // number of arguments
    mimes[index].name = "url";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", url);
    ++index;

    if (certificate != NULL)
    {
        mimes[index].name = "certificate";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", certificate);
        ++index;
    }

    mimes[index].name = "max_connections";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", max_connections);

    if (allowed_updates != NULL)
    {
        mimes[index].name = "allowed_updates";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", allowed_updates);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SET_WEBHOOK, mimes, index, response);
}

telebot_error_e telebot_core_delete_webhook(telebot_core_handler_t *core_h,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_DELETE_WEBHOOK, NULL, 0, response);
}

telebot_error_e telebot_core_get_webhook_info(telebot_core_handler_t *core_h,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_WEBHOOK_INFO, NULL, 0, response);
}

telebot_error_e telebot_core_get_me(telebot_core_handler_t *core_h,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_ME, NULL, 0, response);
}

telebot_error_e telebot_core_send_message(telebot_core_handler_t *core_h,
    long long int chat_id, const char *text, const char *parse_mode,
    bool disable_web_page_preview, bool disable_notification, int reply_to_message_id,
    const char *reply_markup, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (text == NULL))
    {
        ERR("Handler, token or text is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[7]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "text";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", text);
    ++index;

    if (parse_mode != NULL)
    {
        mimes[index].name = "parse_mode";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", parse_mode);
        ++index;
    }

    mimes[index].name = "disable_notification";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (disable_notification ? "true" : "false"));
    ++index;

    mimes[index].name = "disable_web_page_preview";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (disable_web_page_preview ? "true" : "false"));
    ++index;

    if (reply_to_message_id > 0)
    {
        mimes[index].name = "reply_to_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", reply_to_message_id);
        ++index;
    }

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_MESSAGE, mimes, index, response);
}

telebot_error_e telebot_core_forward_message(telebot_core_handler_t *core_h,
    long long int chat_id, long long int from_chat_id, bool disable_notification,
    int message_id, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler or token is NULL.");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (message_id <= 0)
    {
        ERR("Valid message_id is required.");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[4]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "from_chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", from_chat_id);
    ++index;

    mimes[index].name = "disable_notification";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (disable_notification ? "true" : "false"));
    ++index;

    mimes[index].name = "message_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", message_id);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_FORWARD_MESSAGE, mimes, index, response);
}

telebot_error_e telebot_core_send_photo(telebot_core_handler_t *core_h,
    long long int chat_id, const char *photo, bool is_file, const char *caption,
    const char *parse_mode, bool disable_notification, int reply_to_message_id,
    const char *reply_markup, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (photo == NULL))
    {
        ERR("Handler, token or photo is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[7]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "photo";
    mimes[index].type = (is_file ? TELEBOT_MIME_TYPE_FILE : TELEBOT_MIME_TYPE_DATA);
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", photo);
    ++index;

    if (caption != NULL)
    {
        mimes[index].name = "caption";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", caption);
        ++index;
        if (parse_mode != NULL)
        {
            mimes[index].name = "parse_mode";
            mimes[index].type = TELEBOT_MIME_TYPE_DATA;
            snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", parse_mode);
            ++index;
        }
    }

    mimes[index].name = "disable_notification";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (disable_notification ? "true" : "false"));
    ++index;

    if (reply_to_message_id > 0)
    {
        mimes[index].name = "reply_to_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", reply_to_message_id);
        ++index;
    }

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_PHOTO, mimes, index, response);
}

telebot_error_e telebot_core_send_audio(telebot_core_handler_t *core_h,
    long long int chat_id, const char *audio, bool is_file, const char *caption,
    const char *parse_mode, int duration, const char *performer, const char *title,
    const char *thumb, bool disable_notification, int reply_to_message_id,
    const char *reply_markup, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (audio == NULL))
    {
        ERR("Handler, token or audio is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[11]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "audio";
    mimes[index].type = (is_file ? TELEBOT_MIME_TYPE_FILE : TELEBOT_MIME_TYPE_DATA);
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", audio);
    ++index;

    if (caption != NULL)
    {
        mimes[index].name = "caption";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", caption);
        ++index;
        if (parse_mode != NULL)
        {
            mimes[index].name = "parse_mode";
            mimes[index].type = TELEBOT_MIME_TYPE_DATA;
            snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", parse_mode);
            ++index;
        }
    }

    if (duration > 0)
    {
        mimes[index].name = "duration";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", duration);
        ++index;
    }

    if (performer != NULL)
    {
        mimes[index].name = "performer";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", performer);
        ++index;
    }

    if (title != NULL)
    {
        mimes[index].name = "title";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", title);
        ++index;
    }

    if (thumb != NULL)
    {
        mimes[index].name = "thumb";
        mimes[index].type = TELEBOT_MIME_TYPE_FILE;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", thumb);
        ++index;
    }

    mimes[index].name = "disable_notification";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (disable_notification ? "true" : "false"));
    ++index;

    if (reply_to_message_id > 0)
    {
        mimes[index].name = "reply_to_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", reply_to_message_id);
        ++index;
    }

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_AUDIO, mimes, index, response);
}

telebot_error_e telebot_core_send_document(telebot_core_handler_t *core_h,
    long long int chat_id, const char *document, bool is_file, const char *thumb,
    const char *caption, const char *parse_mode, bool disable_notification,
    int reply_to_message_id, const char *reply_markup,
    telebot_core_response_t *response)

{
    if ((core_h == NULL) || (core_h->token == NULL) || (document == NULL))
    {
        ERR("Handler, token or document is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[8]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "document";
    mimes[index].type = (is_file ? TELEBOT_MIME_TYPE_FILE : TELEBOT_MIME_TYPE_DATA);
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", document);
    ++index;

    if (thumb != NULL)
    {
        mimes[index].name = "thumb";
        mimes[index].type = TELEBOT_MIME_TYPE_FILE;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", thumb);
        ++index;
    }

    if (caption != NULL)
    {
        mimes[index].name = "caption";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", caption);
        ++index;
        if (parse_mode != NULL)
        {
            mimes[index].name = "parse_mode";
            mimes[index].type = TELEBOT_MIME_TYPE_DATA;
            snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", parse_mode);
            ++index;
        }
    }

    mimes[index].name = "disable_notification";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (disable_notification ? "true" : "false"));
    ++index;

    if (reply_to_message_id > 0)
    {
        mimes[index].name = "reply_to_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", reply_to_message_id);
        ++index;
    }

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_DOCUMENT, mimes, index, response);
}

telebot_error_e telebot_core_send_video(telebot_core_handler_t *core_h,
    long long int chat_id, const char *video, bool is_file, int duration,
    int width, int height, const char *thumb, const char *caption,
    const char *parse_mode, bool supports_streaming, bool disable_notification,
    int reply_to_message_id, const char *reply_markup,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (video == NULL))
    {
        ERR("Handler, token or document is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[12]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "video";
    mimes[index].type = (is_file ? TELEBOT_MIME_TYPE_FILE : TELEBOT_MIME_TYPE_DATA);
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", video);
    ++index;

    if (duration > 0)
    {
        mimes[index].name = "duration";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", duration);
        ++index;
    }

    if (width > 0)
    {
        mimes[index].name = "width";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", width);
        ++index;
    }

    if (height > 0)
    {
        mimes[index].name = "height";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", height);
        ++index;
    }

    if (thumb != NULL)
    {
        mimes[index].name = "thumb";
        mimes[index].type = TELEBOT_MIME_TYPE_FILE;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", thumb);
        ++index;
    }

    if (caption != NULL)
    {
        mimes[index].name = "caption";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", caption);
        ++index;
        if (parse_mode != NULL)
        {
            mimes[index].name = "parse_mode";
            mimes[index].type = TELEBOT_MIME_TYPE_DATA;
            snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", parse_mode);
            ++index;
        }
    }

    mimes[index].name = "supports_streaming";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (supports_streaming ? "true" : "false"));
    ++index;

    mimes[index].name = "disable_notification";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (disable_notification ? "true" : "false"));
    ++index;

    if (reply_to_message_id > 0)
    {
        mimes[index].name = "reply_to_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", reply_to_message_id);
        ++index;
    }

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_VIDEO, mimes, index, response);
}

telebot_error_e telebot_core_send_animation(telebot_core_handler_t *core_h,
    long long int chat_id, const char *animation, bool is_file, int duration,
    int width, int height, const char *thumb, const char *caption,
    const char *parse_mode, bool disable_notification, int reply_to_message_id,
    const char *reply_markup, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (animation == NULL))
    {
        ERR("Handler, token or document is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[11]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "animation";
    mimes[index].type = (is_file ? TELEBOT_MIME_TYPE_FILE : TELEBOT_MIME_TYPE_DATA);
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", animation);
    ++index;

    if (duration > 0)
    {
        mimes[index].name = "duration";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", duration);
        ++index;
    }

    if (width > 0)
    {
        mimes[index].name = "width";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", width);
        ++index;
    }

    if (height > 0)
    {
        mimes[index].name = "height";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", height);
        ++index;
    }

    if (thumb != NULL)
    {
        mimes[index].name = "thumb";
        mimes[index].type = TELEBOT_MIME_TYPE_FILE;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", thumb);
        ++index;
    }

    if (caption != NULL)
    {
        mimes[index].name = "caption";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", caption);
        ++index;
        if (parse_mode != NULL)
        {
            mimes[index].name = "parse_mode";
            mimes[index].type = TELEBOT_MIME_TYPE_DATA;
            snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", parse_mode);
            ++index;
        }
    }

    mimes[index].name = "disable_notification";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (disable_notification ? "true" : "false"));
    ++index;

    if (reply_to_message_id > 0)
    {
        mimes[index].name = "reply_to_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", reply_to_message_id);
        ++index;
    }

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_ANIMATION, mimes, index, response);
}

telebot_error_e telebot_core_send_voice(telebot_core_handler_t *core_h,
    long long int chat_id, const char *voice, bool is_file, const char *caption,
    const char *parse_mode, int duration, bool disable_notification,
    int reply_to_message_id, const char *reply_markup,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (voice == NULL))
    {
        ERR("Handler, token or voice is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[8]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "voice";
    mimes[index].type = (is_file ? TELEBOT_MIME_TYPE_FILE : TELEBOT_MIME_TYPE_DATA);
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", voice);
    ++index;

    if (duration > 0)
    {
        mimes[index].name = "duration";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", duration);
        ++index;
    }

    if (caption != NULL)
    {
        mimes[index].name = "caption";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", caption);
        ++index;
        if (parse_mode != NULL)
        {
            mimes[index].name = "parse_mode";
            mimes[index].type = TELEBOT_MIME_TYPE_DATA;
            snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", parse_mode);
            ++index;
        }
    }

    mimes[index].name = "disable_notification";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (disable_notification ? "true" : "false"));
    ++index;

    if (reply_to_message_id > 0)
    {
        mimes[index].name = "reply_to_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", reply_to_message_id);
        ++index;
    }

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_VOICE, mimes, index, response);
}

telebot_error_e telebot_core_send_video_note(telebot_core_handler_t *core_h,
    long long int chat_id, char *video_note, bool is_file, int duration, int length,
    const char *thumb, bool disable_notification, int reply_to_message_id,
    const char *reply_markup, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (video_note == NULL))
    {
        ERR("Handler, token or video_note is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[8]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "video_note";
    mimes[index].type = (is_file ? TELEBOT_MIME_TYPE_FILE : TELEBOT_MIME_TYPE_DATA);
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", video_note);
    ++index;

    if (duration > 0)
    {
        mimes[index].name = "duration";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", duration);
        ++index;
    }

    if (length > 0)
    {
        mimes[index].name = "length";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", length);
        ++index;
    }

    if (thumb != NULL)
    {
        mimes[index].name = "thumb";
        mimes[index].type = TELEBOT_MIME_TYPE_FILE;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", thumb);
        ++index;
    }

    mimes[index].name = "disable_notification";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (disable_notification ? "true" : "false"));
    ++index;

    if (reply_to_message_id > 0)
    {
        mimes[index].name = "reply_to_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", reply_to_message_id);
        ++index;
    }

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_VIDEO_NOTE, mimes, index, response);
}

telebot_error_e telebot_core_send_location(telebot_core_handler_t *core_h,
    long long int chat_id, float latitude, float longitude, int live_period,
    bool disable_notification, int reply_to_message_id, const char *reply_markup,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[7]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "latitude";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%f", latitude);
    ++index;

    mimes[index].name = "longitude";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%f", longitude);
    ++index;

    if (live_period > 0)
    {
        mimes[index].name = "live_period";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", live_period);
        ++index;
    }

    mimes[index].name = "disable_notification";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (disable_notification ? "true" : "false"));
    ++index;

    if (reply_to_message_id > 0)
    {
        mimes[index].name = "reply_to_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", reply_to_message_id);
        ++index;
    }

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_LOCATION, mimes, index, response);
}

telebot_error_e telebot_core_edit_message_live_location(telebot_core_handler_t *core_h,
    long long int chat_id, int message_id, const char *inline_message_id,
    float latitude, float longitude, const char *reply_markup,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[6]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "latitude";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%f", latitude);
    ++index;

    mimes[index].name = "longitude";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%f", longitude);
    ++index;

    if (message_id > 0)
    {
        mimes[index].name = "message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", message_id);
        ++index;
    }

    if (inline_message_id != NULL)
    {
        mimes[index].name = "inline_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", inline_message_id);
        ++index;
    }

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_EDIT_MESSAGE_LIVE_LOCATION, mimes, index, response);
}

telebot_error_e telebot_core_stop_message_live_location(telebot_core_handler_t *core_h,
    long long int chat_id, int message_id, char *inline_message_id,
    const char *reply_markup, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[4]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    if (message_id > 0)
    {
        mimes[index].name = "message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", message_id);
        ++index;
    }

    if (inline_message_id != NULL)
    {
        mimes[index].name = "inline_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", inline_message_id);
        ++index;
    }

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_STOP_MESSAGE_LIVE_LOCATION, mimes, index, response);
}

telebot_error_e telebot_core_send_venue(telebot_core_handler_t *core_h,
    long long int chat_id, float latitude, float longitude, const char *title,
    const char *address, const char *foursquare_id, const char *foursquare_type,
    bool disable_notification, int reply_to_message_id, const char *reply_markup,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (title == NULL) ||
        (address == NULL))
    {
        ERR("Handler, token, title, or address is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[10]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "latitude";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%f", latitude);
    ++index;

    mimes[index].name = "longitude";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%f", longitude);
    ++index;

    mimes[index].name = "title";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", title);
    ++index;

    mimes[index].name = "address";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", address);
    ++index;

    if (foursquare_id != NULL)
    {
        mimes[index].name = "foursquare_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", foursquare_id);
        ++index;
    }

    if (foursquare_type != NULL)
    {
        mimes[index].name = "foursquare_type";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", foursquare_type);
        ++index;
    }

    mimes[index].name = "disable_notification";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (disable_notification ? "true" : "false"));
    ++index;

    if (reply_to_message_id > 0)
    {
        mimes[index].name = "reply_to_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", reply_to_message_id);
        ++index;
    }

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_VENUE, mimes, index, response);
}

telebot_error_e telebot_core_send_contact(telebot_core_handler_t *core_h,
    long long int chat_id, const char *phone_number, const char *first_name,
    const char *last_name, const char *vcard, bool disable_notification,
    int reply_to_message_id, const char *reply_markup,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (phone_number == NULL) || (first_name == NULL))
    {
        ERR("Handler, token, phone_number, or first_name is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[8]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "phone_number";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", phone_number);
    ++index;

    mimes[index].name = "first_name";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", first_name);
    ++index;

    if (last_name != NULL)
    {
        mimes[index].name = "last_name";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", last_name);
        ++index;
    }

    if (vcard != NULL)
    {
        mimes[index].name = "vcard";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", vcard);
        ++index;
    }

    mimes[index].name = "disable_notification";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (disable_notification ? "true" : "false"));
    ++index;

    if (reply_to_message_id > 0)
    {
        mimes[index].name = "reply_to_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", reply_to_message_id);
        ++index;
    }

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_CONTACT, mimes, index, response);
}

telebot_error_e telebot_core_send_poll(telebot_core_handler_t *core_h,
    long long int chat_id, const char *question, const char *options,
    bool is_anonymous, const char *type, bool allows_multiple_answers,
    int correct_option_id, bool is_closed, bool disable_notification,
    int reply_to_message_id, const char *reply_markup,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (question == NULL) || (options == NULL))
    {
        ERR("Handler, token, phone_number, or first_name is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[11]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "question";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", question);
    ++index;

    mimes[index].name = "options";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", options);
    ++index;

    mimes[index].name = "is_anonymous";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (is_anonymous ? "true" : "false"));
    ++index;

    if (type != NULL)
    {
        mimes[index].name = "type";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", type);
        ++index;
    }

    mimes[index].name = "allows_multiple_answers";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (allows_multiple_answers ? "true" : "false"));
    ++index;

    if (correct_option_id >= 0)
    {
        mimes[index].name = "correct_option_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", correct_option_id);
        ++index;
    }

    mimes[index].name = "is_closed";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (is_closed ? "true" : "false"));
    ++index;

    mimes[index].name = "disable_notification";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (disable_notification ? "true" : "false"));
    ++index;

    if (reply_to_message_id > 0)
    {
        mimes[index].name = "reply_to_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", reply_to_message_id);
        ++index;
    }

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_POLL, mimes, index, response);
}

telebot_error_e telebot_core_send_dice(telebot_core_handler_t *core_h,
    long long int chat_id, bool disable_notification, int reply_to_message_id,
    const char *reply_markup, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, token, phone_number, or first_name is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[4]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "disable_notification";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (disable_notification ? "true" : "false"));
    ++index;

    if (reply_to_message_id > 0)
    {
        mimes[index].name = "reply_to_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", reply_to_message_id);
        ++index;
    }

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_DICE, mimes, index, response);
}

telebot_error_e telebot_core_send_chat_action(telebot_core_handler_t *core_h,
    long long int chat_id, const char *action, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (action == NULL))
    {
        ERR("Handler, token or action is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[2]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "action";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", action);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_CHAT_ACTION, mimes, index, response);
}

telebot_error_e telebot_core_get_user_profile_photos(telebot_core_handler_t *core_h,
    int user_id, int offset, int limit, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (user_id <= 0)
    {
        ERR("Invalid value of user_id");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[3]; // number of arguments
    mimes[index].name = "user_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", user_id);
    ++index;

    mimes[index].name = "offset";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", offset);
    ++index;

    mimes[index].name = "limit";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", limit);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_USER_PHOTOS, mimes, index, response);
}

telebot_error_e telebot_core_get_file(telebot_core_handler_t *core_h, const char *file_id,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (file_id == NULL))
    {
        ERR("Handler, token or file_id is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[index].name = "file_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", file_id);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_FILE, mimes, index, response);
}

static size_t write_file_cb(void *contents, size_t size, size_t nmemb,
                            void *userp)
{
    size_t written = fwrite(contents, size, nmemb, (FILE *)userp);
    return written;
}

telebot_error_e telebot_core_download_file(telebot_core_handler_t *core_h,
        const char *file_path, const char *out_file)
{
    int ret = TELEBOT_ERROR_NONE;
    if ((core_h == NULL) || (core_h->token == NULL) || (file_path == NULL) ||
        (out_file == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    CURL *curl_h = NULL; 
    CURLcode res;
    long resp_code = 0L;

    FILE *fp = fopen(out_file, "w");
    if (fp == NULL)
    {
        ret = TELEBOT_ERROR_INVALID_PARAMETER;
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

telebot_error_e telebot_core_kick_chat_member(telebot_core_handler_t *core_h,
    long long int chat_id, int user_id, long until_date, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (user_id <= 0)
    {
        ERR("Valid user_id is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[3]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "user_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", user_id);
    ++index;

    if (until_date > 0)
    {
        mimes[index].name = "until_date";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%ld", until_date);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_KICK_CHAT_MEMBER, mimes, index, response);
}

telebot_error_e telebot_core_unban_chat_member(telebot_core_handler_t *core_h,
    long long int chat_id, int user_id, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (user_id <= 0)
    {
        ERR("Valid user_id is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[2]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "user_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", user_id);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_UNBAN_CHAT_MEMBER, mimes, index, response);
}

telebot_error_e telebot_core_restrict_chat_member(telebot_core_handler_t *core_h,
    long long int chat_id, int user_id, long until_date, bool can_send_messages,
    bool can_send_media_messages, bool can_send_polls, bool can_send_other_messages,
    bool can_add_web_page_previews, bool can_change_info, bool can_invite_users,
    bool can_pin_messages, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (user_id <= 0)
    {
        ERR("Valid user_id is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[11]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "user_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", user_id);
    ++index;

    if (until_date > 0)
    {
        mimes[index].name = "until_date";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%ld", until_date);
        ++index;
    }

    mimes[index].name = "can_send_messages";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_send_messages ? "true" : "false"));
    ++index;

    mimes[index].name = "can_send_media_messages";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_send_media_messages ? "true" : "false"));
    ++index;

    mimes[index].name = "can_send_polls";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_send_polls ? "true" : "false"));
    ++index;

    mimes[index].name = "can_send_other_messages";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_send_other_messages ? "true" : "false"));
    ++index;

    mimes[index].name = "can_add_web_page_previews";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_add_web_page_previews ? "true" : "false"));
    ++index;

    mimes[index].name = "can_change_info";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_change_info ? "true" : "false"));
    ++index;

    mimes[index].name = "can_invite_users";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_invite_users ? "true" : "false"));
    ++index;

    mimes[index].name = "can_pin_messages";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_invite_users ? "true" : "false"));
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_RESTRICT_CHAT_MEMBER, mimes, index, response);
}

telebot_error_e telebot_core_promote_chat_member(telebot_core_handler_t *core_h,
    long long int chat_id, int user_id, bool can_change_info, bool can_post_messages,
    bool can_edit_messages, bool can_delete_messages, bool can_invite_users,
    bool can_restrict_members, bool can_pin_messages, bool can_promote_members,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (user_id <= 0)
    {
        ERR("Valid user_id is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[10]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "user_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", user_id);
    ++index;

    mimes[index].name = "can_change_info";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_change_info ? "true" : "false"));
    ++index;

    mimes[index].name = "can_post_messages";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_post_messages ? "true" : "false"));
    ++index;

    mimes[index].name = "can_edit_messages";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_edit_messages ? "true" : "false"));
    ++index;

    mimes[index].name = "can_delete_messages";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_delete_messages ? "true" : "false"));
    ++index;

    mimes[index].name = "can_invite_users";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_invite_users ? "true" : "false"));
    ++index;

    mimes[index].name = "can_restrict_members";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_restrict_members ? "true" : "false"));
    ++index;

    mimes[index].name = "can_pin_messages";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_pin_messages ? "true" : "false"));
    ++index;

    mimes[index].name = "can_promote_members";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_promote_members ? "true" : "false"));
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_PROMOTE_CHAT_MEMBER, mimes, index, response);
}

telebot_error_e telebot_core_set_chat_admin_custom_title(telebot_core_handler_t *core_h,
    long long int chat_id, int user_id, const char *custom_title,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (user_id <= 0)
    {
        ERR("Valid user_id is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[3]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "user_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", user_id);
    ++index;

    mimes[index].name = "custom_title";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", custom_title);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SET_CHAT_ADMIN_TITLE, mimes, index, response);
}

telebot_error_e telebot_core_set_chat_permissions(telebot_core_handler_t *core_h,
    long long int chat_id, bool can_send_messages, bool can_send_media_messages,
    bool can_send_polls, bool can_send_other_messages, bool can_add_web_page_previews,
    bool can_change_info, bool can_invite_users, bool can_pin_messages,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[9]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "can_send_messages";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_send_messages ? "true" : "false"));
    ++index;

    mimes[index].name = "can_send_media_messages";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_send_media_messages ? "true" : "false"));
    ++index;

    mimes[index].name = "can_send_polls";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_send_polls ? "true" : "false"));
    ++index;

    mimes[index].name = "can_send_other_messages";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_send_other_messages ? "true" : "false"));
    ++index;

    mimes[index].name = "can_add_web_page_previews";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_add_web_page_previews ? "true" : "false"));
    ++index;

    mimes[index].name = "can_change_info";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_change_info ? "true" : "false"));
    ++index;

    mimes[index].name = "can_invite_users";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_invite_users ? "true" : "false"));
    ++index;

    mimes[index].name = "can_pin_messages";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (can_invite_users ? "true" : "false"));
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SET_CHAT_PERMISSIONS, mimes, index, response);
}

telebot_error_e telebot_core_export_chat_invite_link(telebot_core_handler_t *core_h,
    long long int chat_id, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_EXPORT_CHAT_INVITE_LINK, mimes, index, response);
}

telebot_error_e telebot_core_set_chat_photo(telebot_core_handler_t *core_h,
    long long int chat_id, const char *photo, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (photo == NULL)
    {
        ERR("Valid photo is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[2]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "photo";
    mimes[index].type = TELEBOT_MIME_TYPE_FILE;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", photo);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SET_CHAT_PHOTO, mimes, index, response);
}

telebot_error_e telebot_core_delete_chat_photo(telebot_core_handler_t *core_h,
    long long int chat_id, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_DELETE_CHAT_PHOTO, mimes, index, response);
}

telebot_error_e telebot_core_set_chat_title(telebot_core_handler_t *core_h,
    long long int chat_id, const char *title, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((title == NULL) || (strlen(title) > 255))
    {
        ERR("Valid title is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[2]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "title";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", title);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SET_CHAT_TITLE, mimes, index, response);
}

telebot_error_e telebot_core_set_chat_description(telebot_core_handler_t *core_h,
    long long int chat_id, const char *description, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((description == NULL) || (strlen(description) > 255))
    {
        ERR("Valid title is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[2]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "description";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", description);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SET_CHAT_DESCRIPTION, mimes, index, response);
}

telebot_error_e telebot_core_pin_chat_message(telebot_core_handler_t *core_h,
    long long int chat_id, int message_id, bool disable_notification,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (message_id <= 0)
    {
        ERR("Valid message_id is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[3]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "message_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", message_id);
    ++index;

    mimes[index].name = "message_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (disable_notification ? "True" : "False"));
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_PIN_CHAT_MESSAGE, mimes, index, response);
}

telebot_error_e telebot_core_unpin_chat_message(telebot_core_handler_t *core_h,
    long long int chat_id, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_UNPIN_CHAT_MESSAGE, mimes, index, response);
}

telebot_error_e telebot_core_leave_chat(telebot_core_handler_t *core_h,
    long long int chat_id, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_LEAVE_CHAT, mimes, index, response);
}

telebot_error_e telebot_core_get_chat(telebot_core_handler_t *core_h,
    long long int chat_id, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_CHAT, mimes, index, response);
}

telebot_error_e telebot_core_get_chat_admins(telebot_core_handler_t *core_h,
    long long int chat_id, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_CHAT_ADMINS, mimes, index, response);
}

telebot_error_e telebot_core_get_chat_members_count(telebot_core_handler_t *core_h,
    long long int chat_id, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_CHAT_MEMBERS_COUNT, mimes, index, response);
}

telebot_error_e telebot_core_get_chat_member(telebot_core_handler_t *core_h,
    long long int chat_id, int user_id, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (user_id <= 0)
    {
        ERR("Valid user_id is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[2]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "user_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", user_id);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_CHAT_MEMBER, mimes, index, response);
}

telebot_error_e telebot_core_set_chat_sticker_set(telebot_core_handler_t *core_h,
    long long int chat_id, const char *sticker_set_name,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (sticker_set_name == NULL)
    {
        ERR("Valid sticker_set_name is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[2]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "sticker_set_name";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", sticker_set_name);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SET_CHAT_STICKER_SET, mimes, index, response);
}

telebot_error_e telebot_core_delete_chat_sticker_set(telebot_core_handler_t *core_h,
    long long int chat_id, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_DEL_CHAT_STICKER_SET, mimes, index, response);
}

telebot_error_e telebot_core_answer_callback_query(telebot_core_handler_t *core_h,
    const char *callback_query_id, const char *text, bool show_alert,
    const char *url, int cache_time, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL) ||
        (callback_query_id == NULL))
    {
        ERR("Handler, token or callback_query_id is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[5]; // number of arguments
    mimes[index].name = "callback_query_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", callback_query_id);
    ++index;

    if (text != NULL)
    {
        mimes[index].name = "text";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", text);
        ++index;
    }

    mimes[index].name = "show_alert";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (show_alert ? "True" : "False"));
    ++index;

    if (url != NULL)
    {
        mimes[index].name = "url";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", url);
        ++index;
    }

    if (cache_time > 0)
    {
        mimes[index].name = "cache_time";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", cache_time);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_ANSWER_CALLBACK_QUERY, mimes, index, response);
}

telebot_error_e telebot_core_set_my_commands(telebot_core_handler_t *core_h,
        const char *commands, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (commands == NULL))
    {
        ERR("Handler, token, or commands is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[1]; // number of arguments
    mimes[index].name = "commands";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", commands);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SET_MY_COMMANDS, mimes, index, response);
}

telebot_error_e telebot_core_get_my_commands(telebot_core_handler_t *core_h,
        telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_MY_COMMANDS, NULL, 0, response);
}

telebot_error_e telebot_core_edit_message_text(telebot_core_handler_t *core_h,
    long long int chat_id, int message_id, const char *inline_message_id,
    const char *text, const char *parse_mode, bool disable_web_page_preview,
    const char *reply_markup, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (text == NULL))
    {
        ERR("Handler, token, or text is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (((chat_id == 0) || (message_id <= 0)) && (inline_message_id == NULL))
    {
        ERR("Either valid chat_id & message_id or inline_message_id required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[7]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    if (message_id > 0)
    {
        mimes[index].name = "message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", message_id);
        ++index;
    }

    if (inline_message_id != NULL)
    {
        mimes[index].name = "inline_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", inline_message_id);
        ++index;
    }

    mimes[index].name = "text";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", text);
    ++index;

    if (parse_mode != NULL)
    {
        mimes[index].name = "parse_mode";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", parse_mode);
        ++index;
    }

    mimes[index].name = "disable_web_page_preview";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", (disable_web_page_preview ? "true" : "false"));
    ++index;

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_EDIT_MESSAGE_TEXT, mimes, index, response);
}

telebot_error_e telebot_core_edit_message_caption(telebot_core_handler_t *core_h,
    long long int chat_id, int message_id, const char *inline_message_id,
    const char *caption, const char *parse_mode, const char *reply_markup,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (((chat_id == 0) || (message_id <= 0)) && (inline_message_id == NULL))
    {
        ERR("Either valid chat_id & message_id or inline_message_id required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[6]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    if (message_id > 0)
    {
        mimes[index].name = "message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", message_id);
        ++index;
    }

    if (inline_message_id != NULL)
    {
        mimes[index].name = "inline_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", inline_message_id);
        ++index;
    }

    mimes[index].name = "caption";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", caption);
    ++index;

    if (parse_mode != NULL)
    {
        mimes[index].name = "parse_mode";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", parse_mode);
        ++index;
    }

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_EDIT_MESSAGE_CAPTION, mimes, index, response);
}

telebot_error_e telebot_core_edit_message_reply_markup(telebot_core_handler_t *core_h,
    long long int chat_id, int message_id, const char *inline_message_id,
    const char *reply_markup, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (((chat_id == 0) || (message_id <= 0)) && (inline_message_id == NULL))
    {
        ERR("Either valid chat_id & message_id or inline_message_id required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[4]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    if (message_id > 0)
    {
        mimes[index].name = "message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", message_id);
        ++index;
    }

    if (inline_message_id != NULL)
    {
        mimes[index].name = "inline_message_id";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", inline_message_id);
        ++index;
    }

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_EDIT_MESSAGE_REPLY_MARKUP, mimes, index, response);
}

telebot_error_e telebot_core_stop_poll(telebot_core_handler_t *core_h,
    long long int chat_id, int message_id, const char *reply_markup,
    telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((chat_id == 0) || (message_id <= 0))
    {
        ERR("Valid chat_id and message_id required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[3]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "message_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", message_id);
    ++index;

    if (reply_markup != NULL)
    {
        mimes[index].name = "reply_markup";
        mimes[index].type = TELEBOT_MIME_TYPE_DATA;
        snprintf(mimes[index].data, sizeof(mimes[index].data), "%s", reply_markup);
        ++index;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_STOP_POLL, mimes, index, response);
}

telebot_error_e telebot_core_delete_message(telebot_core_handler_t *core_h,
    long long int chat_id, int message_id, telebot_core_response_t *response)
{
    if ((core_h == NULL) || (core_h->token == NULL))
    {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((message_id) <= 0)
    {
        ERR("message_id is invalid");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    int index = 0;
    telebot_core_mime_t mimes[2]; // number of arguments
    mimes[index].name = "chat_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%lld", chat_id);
    ++index;

    mimes[index].name = "message_id";
    mimes[index].type = TELEBOT_MIME_TYPE_DATA;
    snprintf(mimes[index].data, sizeof(mimes[index].data), "%d", message_id);
    ++index;

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_DELETE_MESSAGE, mimes, index, response);
}
