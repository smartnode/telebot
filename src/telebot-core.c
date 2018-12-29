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
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <telebot-private.h>
#include <telebot-common.h>
#include <telebot-core.h>

static size_t write_data_cb(void *contents, size_t size, size_t nmemb,
        void *userp)
{
    telebot_core_handler_t *core_h = (telebot_core_handler_t *) userp;
    size_t r_size = size * nmemb;

    core_h->resp_data = realloc(core_h->resp_data, core_h->resp_size +
            r_size + 1);
    if (core_h->resp_data == NULL) {
        ERR("Failed to allocate memory, size:%u", (unsigned int)r_size);
        return 0;
    }

    memcpy((core_h->resp_data  + core_h->resp_size), contents, r_size);
    core_h->resp_size += r_size;
    core_h->resp_data[core_h->resp_size] = 0;

    return r_size;
}

static telebot_error_e telebot_core_curl_perform(telebot_core_handler_t *core_h,
        const char *method, struct curl_httppost *post)
{
    CURL *curl_h;
    CURLcode res;
    long resp_code = 0L;

    //Wait for other in-progress request
    while (core_h->busy) /* NOOP */;

    core_h->resp_data = (char *)malloc(1);
    core_h->resp_size = 0;
    core_h->busy = true;

    curl_h = curl_easy_init();

    if (curl_h == NULL) {
        ERR("Failed to init curl");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    char URL[TELEBOT_URL_SIZE];
    snprintf(URL, TELEBOT_URL_SIZE, "%s/bot%s/%s", TELEBOT_API_URL,
            core_h->token, method);
    curl_easy_setopt(curl_h, CURLOPT_URL, URL);
    curl_easy_setopt(curl_h, CURLOPT_WRITEFUNCTION, write_data_cb);
    curl_easy_setopt(curl_h, CURLOPT_WRITEDATA, core_h);

    if (post != NULL)
        curl_easy_setopt(curl_h, CURLOPT_HTTPPOST, post);

    res = curl_easy_perform(curl_h);
    if (res != CURLE_OK) {
        ERR("Failed to curl_easy_perform\nError: %s (%d)",
                curl_easy_strerror(res), res);
        if (core_h->resp_data != NULL)
            free(core_h->resp_data);
        core_h->resp_data= NULL;
        core_h->resp_size = 0;
        curl_easy_cleanup(curl_h);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    curl_easy_getinfo(curl_h, CURLINFO_RESPONSE_CODE, &resp_code);
    if (resp_code != 200L) {
        ERR("Wrong HTTP response received, response: %ld", resp_code);
        if (core_h->resp_data != NULL)
            free(core_h->resp_data);
        core_h->resp_data = NULL;
        core_h->resp_size = 0;
        curl_easy_cleanup(curl_h);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    DBG("Response: %s", core_h->resp_data);

    curl_easy_cleanup(curl_h);

    core_h->busy = false;
    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_core_create(telebot_core_handler_t **core_h, char *token)
{
    if ((token == NULL) || (core_h == NULL)) {
        ERR("Token(0x%p) or core_h(0x%p) is NULL", token, core_h);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    telebot_core_handler_t *_core_h = malloc(sizeof(telebot_core_handler_t));
    if (_core_h == NULL) {
        ERR("Failed to allocate memory");
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }
    _core_h->token = strdup(token);
    _core_h->resp_data = NULL;
    _core_h->resp_size = 0;
    _core_h->busy = false;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    *core_h = _core_h;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_core_destroy(telebot_core_handler_t *core_h)
{
    curl_global_cleanup();

    if (core_h == NULL) {
        ERR("Handler is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (core_h->token != NULL) {
        memset(core_h->token, 'X', strlen(core_h->token));
        free(core_h->token);
        core_h->token = NULL;
    }

    if (core_h->resp_data != NULL)
        free(core_h->resp_data);

    free(core_h);
    core_h = NULL;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_core_get_updates(telebot_core_handler_t *core_h,
        int offset, int limit, int timeout, const char *allowed_updates)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    if (limit > TELEBOT_UPDATE_COUNT_MAX_LIMIT)
        limit = TELEBOT_UPDATE_COUNT_MAX_LIMIT;

    char offset_str[16];
    snprintf(offset_str, sizeof(offset_str), "%d", offset);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "offset",
            CURLFORM_COPYCONTENTS, offset_str, CURLFORM_END);
    char limit_str[16];
    snprintf(limit_str, sizeof(limit_str), "%d", limit);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "limit",
            CURLFORM_COPYCONTENTS, limit_str, CURLFORM_END);
    char timeout_str[16];
    snprintf(timeout_str, sizeof(timeout_str), "%d", timeout);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "timeout",
            CURLFORM_COPYCONTENTS, timeout_str, CURLFORM_END);
    if (allowed_updates)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "allowed_updates",
                CURLFORM_COPYCONTENTS, allowed_updates, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_UPDATES, post);
}

telebot_error_e telebot_core_set_webhook(telebot_core_handler_t *core_h,
        char *url, char *certificate, int max_connections,
        char *allowed_updates)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (url == NULL)) {
        ERR("Handler, token, or url is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "url",
            CURLFORM_COPYCONTENTS, url, CURLFORM_END);
    if (certificate != NULL)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "certificate",
                CURLFORM_FILE, certificate, CURLFORM_END);
    char max_conn_str[16];
    snprintf(max_conn_str, sizeof(max_conn_str), "%d", max_connections);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "max_connections",
            CURLFORM_COPYCONTENTS, max_conn_str, CURLFORM_END);
    if (allowed_updates)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "allowed_updates",
                CURLFORM_COPYCONTENTS, allowed_updates, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SET_WEBHOOK, post);
}

telebot_error_e telebot_core_delete_webhook(telebot_core_handler_t *core_h)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_DELETE_WEBHOOK, NULL);
}

telebot_error_e telebot_core_get_webhook_info(telebot_core_handler_t *core_h)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_WEBHOOK_INFO, NULL);
}

telebot_error_e telebot_core_get_me(telebot_core_handler_t *core_h)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_ME, NULL);
}

telebot_error_e telebot_core_send_message(telebot_core_handler_t *core_h,
        long long int chat_id, char *text, char *parse_mode, bool disable_web_page_preview,
        bool disable_notification, int reply_to_message_id, const char *reply_markup)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (text == NULL)) {
        ERR("Handler, token or text is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "text",
            CURLFORM_COPYCONTENTS, text, CURLFORM_END);
    if (parse_mode)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "parse_mode",
                CURLFORM_COPYCONTENTS, parse_mode, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "disable_notification",
            CURLFORM_COPYCONTENTS, (disable_notification) ? "true" : "false",
            CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "disable_web_page_preview",
            CURLFORM_COPYCONTENTS, (disable_web_page_preview) ? "true" : "false",
            CURLFORM_END);
    if (reply_to_message_id > 0) {
        char reply_to_message_id_str[16];
        snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d",
                reply_to_message_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id",
                CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    }
    if (reply_markup)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup",
                CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_MESSAGE, post);
}

telebot_error_e telebot_core_forward_message(telebot_core_handler_t *core_h,
        long long int chat_id, long long int from_chat_id, bool disable_notification, int message_id)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler or token is NULL.");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (message_id <= 0) {
        ERR("Valid message_id is required.");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    curl_formadd (&post, &last, CURLFORM_COPYNAME, "from_chat_id",
            CURLFORM_COPYCONTENTS, from_chat_id, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "disable_notification",
            CURLFORM_COPYCONTENTS, (disable_notification) ? "true" : "false",
            CURLFORM_END);
    char message_id_str[16];
    snprintf(message_id_str, sizeof(message_id_str), "%d", message_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "message_id",
            CURLFORM_COPYCONTENTS, message_id_str, CURLFORM_END);

    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_FORWARD_MESSAGE, post);
}

telebot_error_e telebot_core_send_photo(telebot_core_handler_t *core_h,
        long long int chat_id, char *photo, bool is_file, char *caption,
        bool disable_notification, int reply_to_message_id, char *reply_markup)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (photo == NULL)) {
        ERR("Handler, token or photo is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    if (is_file)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "photo",
                CURLFORM_FILE, photo, CURLFORM_END);
    else
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "photo",
                CURLFORM_COPYCONTENTS, photo, CURLFORM_END);
    if (caption)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "caption",
                CURLFORM_COPYCONTENTS, caption, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "disable_notification",
            CURLFORM_COPYCONTENTS, (disable_notification) ? "true" : "false",
            CURLFORM_END);
    if (reply_to_message_id > 0) {
        char reply_to_message_id_str[16];
        snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d",
                reply_to_message_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id",
                CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    }
    if (reply_markup)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup",
                CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_PHOTO, post);
}

telebot_error_e telebot_core_send_audio(telebot_core_handler_t *core_h,
        long long int chat_id, char *audio, bool is_file, int duration, char *performer,
        char *title, bool disable_notification, int reply_to_message_id,
        char *reply_markup)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (audio == NULL)) {
        ERR("Handler, token or audio is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    if (is_file)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "audio",
                CURLFORM_FILE, audio, CURLFORM_END);
    else
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "audio",
                CURLFORM_COPYCONTENTS, audio, CURLFORM_END);
    if (duration > 0) {
        char duration_str[16];
        snprintf(duration_str, sizeof(duration_str), "%d", duration);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "duration",
                CURLFORM_COPYCONTENTS, duration_str, CURLFORM_END);
    }
    if (performer)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "performer",
                CURLFORM_COPYCONTENTS, performer, CURLFORM_END);
    if (title)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "title",
                CURLFORM_COPYCONTENTS, title, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "disable_notification",
            CURLFORM_COPYCONTENTS, (disable_notification) ? "true" : "false",
            CURLFORM_END);
    if (reply_to_message_id > 0) {
        char reply_to_message_id_str[16];
        snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d",
                reply_to_message_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id",
                CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    }
    if (reply_markup)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup",
                CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_AUDIO, post);
}

telebot_error_e telebot_core_send_document(telebot_core_handler_t *core_h,
        long long int chat_id, char *document, bool is_file, bool disable_notification,
        int reply_to_message_id, char *reply_markup)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (document == NULL)) {
        ERR("Handler, token or document is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    if (is_file)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "document",
                CURLFORM_FILE, document, CURLFORM_END);
    else
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "document",
                CURLFORM_COPYCONTENTS, document, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "disable_notification",
            CURLFORM_COPYCONTENTS, (disable_notification) ? "true" : "false",
            CURLFORM_END);
    if (reply_to_message_id > 0) {
        char reply_to_message_id_str[16];
        snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d",
                reply_to_message_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id",
                CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    }
    if (reply_markup)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup",
                CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_DOCUMENT, post);
}

telebot_error_e telebot_core_send_video(telebot_core_handler_t *core_h,
        long long int chat_id, char *video, bool is_file, int duration, char *caption,
        bool disable_notification, int reply_to_message_id, char *reply_markup)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (video == NULL)) {
        ERR("Handler, token or document is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    if (is_file)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "video",
                CURLFORM_FILE, video, CURLFORM_END);
    else
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "video",
                CURLFORM_COPYCONTENTS, video, CURLFORM_END);
    if (duration > 0) {
        char duration_str[16];
        snprintf(duration_str, sizeof(duration_str), "%d", duration);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "duration",
                CURLFORM_COPYCONTENTS, duration_str, CURLFORM_END);
    }
    if (caption)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "caption",
                CURLFORM_COPYCONTENTS, caption, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "disable_notification",
            CURLFORM_COPYCONTENTS, (disable_notification) ? "true" : "false",
            CURLFORM_END);
    if (reply_to_message_id > 0) {
        char reply_to_message_id_str[16];
        snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d",
                reply_to_message_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id",
                CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    }
    if (reply_markup)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup",
                CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_VIDEO, post);
}

telebot_error_e telebot_core_send_voice(telebot_core_handler_t *core_h,
        long long int chat_id, char *voice, bool is_file, int duration,
        bool disable_notification, int reply_to_message_id, char *reply_markup)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (voice == NULL)) {
        ERR("Handler, token or voice is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    if (is_file)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "voice",
                CURLFORM_FILE, voice, CURLFORM_END);
    else
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "voice",
                CURLFORM_COPYCONTENTS, voice, CURLFORM_END);
    if (duration > 0) {
        char duration_str[16];
        snprintf(duration_str, sizeof(duration_str), "%d", duration);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "duration",
                CURLFORM_COPYCONTENTS, duration_str, CURLFORM_END);
    }
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "disable_notification",
            CURLFORM_COPYCONTENTS, (disable_notification) ? "true" : "false",
            CURLFORM_END);
    if (reply_to_message_id > 0) {
        char reply_to_message_id_str[16];
        snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d",
                reply_to_message_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id",
                CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    }
    if (reply_markup)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup",
                CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_VOICE, post);
}

telebot_error_e telebot_core_send_video_note(telebot_core_handler_t *core_h,
        long long int chat_id, char *video_note, bool is_file, int duration, int length,
        bool disable_notification, int reply_to_message_id, char *reply_markup)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (video_note == NULL)) {
        ERR("Handler, token or video_note is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    if (is_file)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "video_note",
                CURLFORM_FILE, video_note, CURLFORM_END);
    else
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "video_note",
                CURLFORM_COPYCONTENTS, video_note, CURLFORM_END);
    if (duration > 0) {
        char duration_str[16];
        snprintf(duration_str, sizeof(duration_str), "%d", duration);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "duration",
                CURLFORM_COPYCONTENTS, duration_str, CURLFORM_END);
    }
    if (length > 0) {
        char length_str[16];
        snprintf(length_str, sizeof(length_str), "%d", length);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "length",
                CURLFORM_COPYCONTENTS, length_str, CURLFORM_END);
    }
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "disable_notification",
            CURLFORM_COPYCONTENTS, (disable_notification) ? "true" : "false",
            CURLFORM_END);
    if (reply_to_message_id > 0) {
        char reply_to_message_id_str[16];
        snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d",
                reply_to_message_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id",
                CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    }
    if (reply_markup)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup",
                CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_SEND_VIDEO_NOTE, post);
}

telebot_error_e telebot_core_send_location(telebot_core_handler_t *core_h,
        long long int chat_id, float latitude, float longitude, bool disable_notification,
        int reply_to_message_id, char *reply_markup)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    char latitude_str[32];
    snprintf(latitude_str, sizeof(latitude_str), "%f", latitude);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "latitude",
            CURLFORM_COPYCONTENTS, latitude_str, CURLFORM_END);
    char longitude_str[32];
    snprintf(longitude_str, sizeof(longitude_str), "%f", longitude);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "longitude",
            CURLFORM_COPYCONTENTS, longitude_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "disable_notification",
            CURLFORM_COPYCONTENTS, (disable_notification) ? "true" : "false",
            CURLFORM_END);
    if (reply_to_message_id > 0) {
        char reply_to_message_id_str[16];
        snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d",
                reply_to_message_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id",
                CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    }
    if (reply_markup)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup",
                CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_LOCATION, post);
}

telebot_error_e telebot_core_edit_message_live_location(telebot_core_handler_t *core_h,
        long long int chat_id, int message_id, char *inline_message_id, float latitude,
        float longitude, bool disable_notification, char *reply_markup)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (((chat_id == 0) || (message_id <= 0)) && (inline_message_id == NULL)) {
        ERR("Either valid chat_id & message_id or inline_message_id required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    if (chat_id != 0) {
        char chat_id_str[16];
        snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
                CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    }
    if (message_id > 0) {
        char message_id_str[16];
        snprintf(message_id_str, sizeof(message_id_str), "%d", message_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "message_id",
                CURLFORM_COPYCONTENTS, message_id_str, CURLFORM_END);
    }
    if (inline_message_id)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "inline_message_id",
                CURLFORM_COPYCONTENTS, inline_message_id, CURLFORM_END);
    char latitude_str[32];
    snprintf(latitude_str, sizeof(latitude_str), "%f", latitude);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "latitude",
            CURLFORM_COPYCONTENTS, latitude_str, CURLFORM_END);
    char longitude_str[32];
    snprintf(longitude_str, sizeof(longitude_str), "%f", longitude);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "longitude",
            CURLFORM_COPYCONTENTS, longitude_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "disable_notification",
            CURLFORM_COPYCONTENTS, (disable_notification) ? "true" : "false",
            CURLFORM_END);
    if (reply_markup)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup",
                CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_EDIT_MESSAGE_LIVE_LOCATION, post);
}

telebot_error_e telebot_core_stop_message_live_location(telebot_core_handler_t *core_h,
        long long int chat_id, int message_id, char *inline_message_id, char *reply_markup)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (((chat_id == 0) || (message_id <= 0)) && (inline_message_id == NULL)) {
        ERR("Either valid chat_id & message_id or inline_message_id required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    if (chat_id != 0) {
        char chat_id_str[16];
        snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
                CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    }
    if (message_id > 0) {
        char message_id_str[16];
        snprintf(message_id_str, sizeof(message_id_str), "%d", message_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "message_id",
                CURLFORM_COPYCONTENTS, message_id_str, CURLFORM_END);
    }
    if (inline_message_id)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "inline_message_id",
                CURLFORM_COPYCONTENTS, inline_message_id, CURLFORM_END);
    if (reply_markup)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup",
                CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_STOP_MESSAGE_LIVE_LOCATION, post);
}

telebot_error_e telebot_core_send_venue(telebot_core_handler_t *core_h,
        long long int chat_id,float latitude, float longitude, char *title, char *address,
        char *foursquare_id, bool disable_notification, int reply_to_message_id,
        char *reply_markup)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (title == NULL) ||
            (address == NULL)) {
        ERR("Handler, token, title, or address is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    char latitude_str[32];
    snprintf(latitude_str, sizeof(latitude_str), "%f", latitude);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "latitude",
            CURLFORM_COPYCONTENTS, latitude_str, CURLFORM_END);
    char longitude_str[32];
    snprintf(longitude_str, sizeof(longitude_str), "%f", longitude);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "longitude",
            CURLFORM_COPYCONTENTS, longitude_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "title",
            CURLFORM_COPYCONTENTS, title, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "address",
            CURLFORM_COPYCONTENTS, address, CURLFORM_END);
    if (foursquare_id)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "foursquare_id",
                CURLFORM_COPYCONTENTS, foursquare_id, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "disable_notification",
            CURLFORM_COPYCONTENTS, (disable_notification) ? "true" : "false",
            CURLFORM_END);
    if (reply_to_message_id > 0) {
        char reply_to_message_id_str[16];
        snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d",
                reply_to_message_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id",
                CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    }
    if (reply_markup)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup",
                CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_VENUE, post);
}

telebot_error_e telebot_core_send_contact(telebot_core_handler_t *core_h,
        long long int chat_id, char *phone_number, char *first_name, char *last_name,
        bool disable_notification, int reply_to_message_id, char *reply_markup)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (phone_number == NULL)
            || (first_name == NULL)) {
        ERR("Handler, token, phone_number, or first_name is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "phone_number",
            CURLFORM_COPYCONTENTS, phone_number, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "first_name",
            CURLFORM_COPYCONTENTS, first_name, CURLFORM_END);
    if (last_name)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "last_name",
                CURLFORM_COPYCONTENTS, last_name, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "disable_notification",
            CURLFORM_COPYCONTENTS, (disable_notification) ? "true" : "false",
            CURLFORM_END);
    if (reply_to_message_id > 0) {
        char reply_to_message_id_str[16];
        snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d",
                reply_to_message_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id",
                CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    }
    if (reply_markup)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup",
                CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_CONTACT, post);
}

telebot_error_e telebot_core_send_chat_action(telebot_core_handler_t *core_h,
        long long int chat_id, char *action)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (action == NULL)) {
        ERR("Handler, token or action is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "action",
            CURLFORM_COPYCONTENTS, action, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_CHAT_ACTION, post);
}

telebot_error_e telebot_core_get_user_profile_photos(telebot_core_handler_t *core_h,
        int user_id, int offset, int limit)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (user_id <= 0) {
        ERR("Invalid value of user_id");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char user_id_str[16];
    snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "user_id",
            CURLFORM_COPYCONTENTS, user_id_str, CURLFORM_END);
    char offset_str[16];
    snprintf(offset_str, sizeof(offset_str), "%d", offset);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "offset",
            CURLFORM_COPYCONTENTS, offset_str, CURLFORM_END);
    char limit_str[16];
    snprintf(limit_str, sizeof(limit_str), "%d", limit);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "limit",
            CURLFORM_COPYCONTENTS, limit_str, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_USER_PHOTOS, post);
}

telebot_error_e telebot_core_get_file(telebot_core_handler_t *core_h, char *file_id)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (file_id == NULL)) {
        ERR("Handler, token or file_id is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "file_id",
            CURLFORM_COPYCONTENTS, file_id, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_FILE, post);
}

static size_t write_file_cb(void *contents, size_t size, size_t nmemb,
        void *userp)
{
    size_t written = fwrite(contents, size, nmemb, (FILE *)userp);
    return written;
}

telebot_error_e telebot_core_download_file(telebot_core_handler_t *core_h,
        char *file_path, char *out_file)
{
    if ((core_h == NULL) || (core_h->token == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    CURL *curl_h;
    CURLcode res;
    long resp_code = 0L;

    FILE *fp = fopen(out_file, "w");
    if (fp == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    curl_h = curl_easy_init();
    if (curl_h == NULL)
        return TELEBOT_ERROR_OPERATION_FAILED;

    char URL[TELEBOT_URL_SIZE];
    snprintf(URL, TELEBOT_URL_SIZE, "%s/file/bot%s/%s", TELEBOT_API_URL,
            core_h->token, file_path);

    curl_easy_setopt(curl_h, CURLOPT_URL, URL);
    curl_easy_setopt(curl_h, CURLOPT_WRITEFUNCTION, write_file_cb);
    curl_easy_setopt(curl_h, CURLOPT_WRITEDATA, fp);

    res = curl_easy_perform(curl_h);
    if (res != CURLE_OK) {
        ERR("Failed to curl_easy_perform\nError: %s (%d)",
                curl_easy_strerror(res), res);
        curl_easy_cleanup(curl_h);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    curl_easy_getinfo(curl_h, CURLINFO_RESPONSE_CODE, &resp_code);
    if (resp_code != 200L) {
        ERR("Wrong HTTP response received, response: %ld", resp_code);
        fclose(fp);
        unlink(out_file);
        curl_easy_cleanup(curl_h);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    fclose(fp);

    curl_easy_cleanup(curl_h);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_core_kick_chat_member(telebot_core_handler_t *core_h,
        long long int chat_id, int user_id, long until_date)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (user_id <= 0) {
        ERR("Valid user_id is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    char user_id_str[16];
    snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "user_id",
            CURLFORM_COPYCONTENTS, user_id_str, CURLFORM_END);
    char until_date_str[16];
    snprintf(until_date_str, sizeof(until_date_str), "%ld", until_date);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "until_date",
            CURLFORM_COPYCONTENTS, until_date_str, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_KICK_CHAT_MEMBER,
            post);
}

telebot_error_e telebot_core_unban_chat_member(telebot_core_handler_t *core_h,
        long long int chat_id, int user_id)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (user_id <= 0) {
        ERR("Valid user_id is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    char user_id_str[16];
    snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "user_id",
            CURLFORM_COPYCONTENTS, user_id_str, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_UNBAN_CHAT_MEMBER,
            post);
}

telebot_error_e telebot_core_restrict_chat_member(telebot_core_handler_t *core_h,
        long long int chat_id, int user_id, long until_date, bool can_send_messages,
        bool can_send_media_messages, bool can_send_other_messages,
        bool can_add_web_page_previews)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (user_id <= 0) {
        ERR("Valid user_id is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    char user_id_str[16];
    snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "user_id",
            CURLFORM_COPYCONTENTS, user_id_str, CURLFORM_END);
    char until_date_str[16];
    snprintf(until_date_str, sizeof(until_date_str), "%ld", until_date);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "until_date",
            CURLFORM_COPYCONTENTS, until_date_str, CURLFORM_END);
    if (can_send_messages)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "can_send_messages",
                CURLFORM_COPYCONTENTS, "true", CURLFORM_END);
    if (can_send_media_messages)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "can_send_media_messages",
                CURLFORM_COPYCONTENTS, "true", CURLFORM_END);
    if (can_send_other_messages)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "can_send_other_messages",
                CURLFORM_COPYCONTENTS, "true", CURLFORM_END);
    if (can_add_web_page_previews)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "can_add_web_page_previews",
                CURLFORM_COPYCONTENTS, "true", CURLFORM_END);

    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_RESTRICT_CHAT_MEMBER, post);
}

telebot_error_e telebot_core_promote_chat_member(telebot_core_handler_t *core_h,
        long long int chat_id, int user_id, bool can_post_messages, bool can_edit_messages,
        bool can_delete_messages, bool can_invite_users, bool can_restrict_members,
        bool can_pin_messages, bool can_promote_members)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (user_id <= 0) {
        ERR("Valid user_id is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    char user_id_str[16];
    snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "user_id",
            CURLFORM_COPYCONTENTS, user_id_str, CURLFORM_END);
    if (can_post_messages)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "can_post_messages",
                CURLFORM_COPYCONTENTS, "true", CURLFORM_END);
    if (can_edit_messages)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "can_edit_messages",
                CURLFORM_COPYCONTENTS, "true", CURLFORM_END);
    if (can_delete_messages)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "can_delete_messages",
                CURLFORM_COPYCONTENTS, "true", CURLFORM_END);
    if (can_invite_users)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "can_invite_users",
                CURLFORM_COPYCONTENTS, "true", CURLFORM_END);
    if (can_restrict_members)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "can_restrict_members",
                CURLFORM_COPYCONTENTS, "true", CURLFORM_END);
    if (can_pin_messages)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "can_pin_messages",
                CURLFORM_COPYCONTENTS, "true", CURLFORM_END);
    if (can_promote_members)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "can_promote_members",
                CURLFORM_COPYCONTENTS, "true", CURLFORM_END);

    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_RESTRICT_CHAT_MEMBER, post);
}

telebot_error_e telebot_core_export_chat_invite_link(telebot_core_handler_t *core_h,
        long long int chat_id)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_EXPORT_CHAT_INVITE_LINK, post);
}

telebot_error_e telebot_core_set_chat_photo(telebot_core_handler_t *core_h,
        long long int chat_id, char *photo)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (photo == NULL) {
        ERR("Valid photo is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "photo",
            CURLFORM_FILE, photo, CURLFORM_END);
    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_SET_CHAT_PHOTO, post);
}


telebot_error_e telebot_core_delete_chat_photo(telebot_core_handler_t *core_h,
        long long int chat_id, char *photo)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_DELETE_CHAT_PHOTO, post);
}


telebot_error_e telebot_core_set_chat_title(telebot_core_handler_t *core_h,
        long long int chat_id, char *title)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((title == NULL) || (strlen(title) > 255)) {
        ERR("Valid title is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "title",
            CURLFORM_COPYCONTENTS, title, CURLFORM_END);
    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_SET_CHAT_TITLE, post);
}


telebot_error_e telebot_core_set_chat_description(telebot_core_handler_t *core_h,
        long long int chat_id, char *description)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    if (description)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "description",
                CURLFORM_COPYCONTENTS, description, CURLFORM_END);
    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_SET_CHAT_DESCRIPTION, post);
}

telebot_error_e telebot_core_pin_chat_message(telebot_core_handler_t *core_h,
        long long int chat_id, int message_id, bool disable_notification)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (message_id <=0) {
        ERR("Valid message_id is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    char message_id_str[16];
    snprintf(message_id_str, sizeof(message_id_str), "%d", message_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "message_id",
            CURLFORM_COPYCONTENTS, message_id_str, CURLFORM_END);
    if (disable_notification)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "disable_notification",
                CURLFORM_COPYCONTENTS, "true", CURLFORM_END);

    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_PIN_CHAT_MESSAGE, post);
}

telebot_error_e telebot_core_unpin_chat_message(telebot_core_handler_t *core_h,
        long long int chat_id)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);

    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_UNPIN_CHAT_MESSAGE, post);
}


telebot_error_e telebot_core_leave_chat(telebot_core_handler_t *core_h,
        long long int chat_id)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_LEAVE_CHAT, post);
}


telebot_error_e telebot_core_get_chat(telebot_core_handler_t *core_h,
        long long int chat_id)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_CHAT, post);
}

telebot_error_e telebot_core_get_chat_admins(telebot_core_handler_t *core_h,
        long long int chat_id)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_GET_CHAT_ADMINS, post);
}

telebot_error_e telebot_core_get_chat_members_count(telebot_core_handler_t *core_h,
        long long int chat_id)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);

    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_GET_CHAT_MEMBERS_COUNT, post);
}

telebot_error_e telebot_core_get_chat_member(telebot_core_handler_t *core_h,
        long long int chat_id)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);

    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_GET_CHAT_MEMBER, post);
}

telebot_error_e telebot_core_set_chat_sticker_set(telebot_core_handler_t *core_h,
        long long int chat_id, char *sticker_set_name)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (sticker_set_name == NULL) {
        ERR("Valid sticker_set_name is required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "sticker_set_name",
            CURLFORM_COPYCONTENTS, sticker_set_name, CURLFORM_END);

    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_SET_CHAT_STICKER_SET, post);
}

telebot_error_e telebot_core_delete_chat_sticker_set(telebot_core_handler_t *core_h,
        long long int chat_id)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler, or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);

    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_DEL_CHAT_STICKER_SET, post);
}

telebot_error_e telebot_core_answer_callback_query(telebot_core_handler_t * core_h,
        const char *callback_query_id, char *text, bool show_alert,
        char *url, int cache_time)
{
    if ((core_h == NULL) || (core_h->token == NULL) ||
            (callback_query_id == NULL)) {
        ERR("Handler, token or callback_query_id is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    curl_formadd (&post, &last, CURLFORM_COPYNAME, "callback_query_id",
            CURLFORM_COPYCONTENTS, callback_query_id, CURLFORM_END);
    if (text)
        curl_formadd (&post, &last, CURLFORM_COPYNAME, "text",
                CURLFORM_COPYCONTENTS, text, CURLFORM_END);
    if (show_alert)
        curl_formadd (&post, &last, CURLFORM_COPYNAME, "show_alert",
                CURLFORM_COPYCONTENTS, "true", CURLFORM_END);
    if (url)
        curl_formadd (&post, &last, CURLFORM_COPYNAME, "url",
                CURLFORM_COPYCONTENTS, url, CURLFORM_END);
    if (cache_time) {
        char cache_time_str[16];
        snprintf(cache_time_str, sizeof(cache_time_str), "%d", cache_time);
        curl_formadd (&post, &last, CURLFORM_COPYNAME, "cache_time",
                CURLFORM_COPYCONTENTS, cache_time_str, CURLFORM_END);
    }

    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_ANSWER_CALLBACK_QUERY, post);
}

telebot_error_e telebot_core_edit_message_text(telebot_core_handler_t *core_h,
        long long int chat_id, int message_id, char *inline_message_id, char *text,
        char *parse_mode, bool disable_web_page_preview, char *reply_markup)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (text == NULL)) {
        ERR("Handler, token, or text is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (((chat_id == 0) || (message_id <=0)) && (inline_message_id == NULL)) {
        ERR("Either valid chat_id & message_id or inline_message_id required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    if (chat_id != 0) {
        char chat_id_str[16];
        snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
                CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    }
    if (message_id > 0) {
        char message_id_str[16];
        snprintf(message_id_str, sizeof(message_id_str), "%d", message_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "message_id",
                CURLFORM_COPYCONTENTS, message_id_str, CURLFORM_END);
    }
    if (inline_message_id)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "inline_message_id",
                CURLFORM_COPYCONTENTS, inline_message_id, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "text",
            CURLFORM_COPYCONTENTS, text, CURLFORM_END);
    if (parse_mode)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "parse_mode",
                CURLFORM_COPYCONTENTS, parse_mode, CURLFORM_END);
    if (disable_web_page_preview)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "disable_web_page_preview",
                CURLFORM_COPYCONTENTS, "true", CURLFORM_END);
    if (reply_markup)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup",
                CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_EDIT_MESSAGE_TEXT, post);
}

telebot_error_e telebot_core_edit_message_caption(telebot_core_handler_t *core_h,
        long long int chat_id, int message_id, char *inline_message_id, char *caption,
        char *reply_markup)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (((chat_id == 0) || (message_id <=0)) && (inline_message_id == NULL)) {
        ERR("Either valid chat_id & message_id or inline_message_id required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    if (chat_id != 0) {
        char chat_id_str[16];
        snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
                CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    }
    if (message_id > 0) {
        char message_id_str[16];
        snprintf(message_id_str, sizeof(message_id_str), "%d", message_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "message_id",
                CURLFORM_COPYCONTENTS, message_id_str, CURLFORM_END);
    }
    if (inline_message_id)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "inline_message_id",
                CURLFORM_COPYCONTENTS, inline_message_id, CURLFORM_END);
    if (caption)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "caption",
                CURLFORM_COPYCONTENTS, caption, CURLFORM_END);
    if (reply_markup)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup",
                CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_EDIT_MESSAGE_CAPTION, post);
}


telebot_error_e telebot_core_edit_message_reply_markup(telebot_core_handler_t *core_h,
        long long int chat_id, int message_id, char *inline_message_id, char *reply_markup)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (((chat_id == 0) || (message_id <=0)) && (inline_message_id == NULL)) {
        ERR("Either valid chat_id & message_id or inline_message_id required");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    if (chat_id != 0) {
        char chat_id_str[16];
        snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
                CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    }
    if (message_id > 0) {
        char message_id_str[16];
        snprintf(message_id_str, sizeof(message_id_str), "%d", message_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "message_id",
                CURLFORM_COPYCONTENTS, message_id_str, CURLFORM_END);
    }
    if (inline_message_id)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "inline_message_id",
                CURLFORM_COPYCONTENTS, inline_message_id, CURLFORM_END);
    if (reply_markup)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup",
                CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(core_h,
            TELEBOT_METHOD_EDIT_MESSAGE_REPLY_MARKUP, post);
}

telebot_error_e telebot_core_delete_message(telebot_core_handler_t *core_h,
        long long int chat_id, int message_id)
{
    if ((core_h == NULL) || (core_h->token == NULL)) {
        ERR("Handler or token is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((message_id) <= 0) {
        ERR("message_id is invalid");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    char message_id_str[16];
    snprintf(message_id_str, sizeof(message_id_str), "%d", message_id);

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "message_id",
            CURLFORM_COPYCONTENTS, message_id_str, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_DELETE_MESSAGE, post);
}

telebot_error_e telebot_core_send_sticker(telebot_core_handler_t *core_h,
        long long int chat_id, char *sticker, bool is_file, bool disable_notification,
        int reply_to_message_id, char *reply_markup)
{
    if ((core_h == NULL) || (core_h->token == NULL) || (sticker == NULL)) {
        ERR("Handler, token or sticker is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char chat_id_str[16];
    snprintf(chat_id_str, sizeof(chat_id_str), "%lld", chat_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id",
            CURLFORM_COPYCONTENTS, chat_id_str, CURLFORM_END);
    if (is_file)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "sticker",
                CURLFORM_FILE, sticker, CURLFORM_END);
    else
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "sticker",
                CURLFORM_COPYCONTENTS, sticker, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "disable_notification",
            CURLFORM_COPYCONTENTS, (disable_notification) ? "true" : "false",
            CURLFORM_END);
    if (reply_to_message_id > 0) {
        char reply_to_message_id_str[16];
        snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d",
                reply_to_message_id);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id",
                CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    }
    if (reply_markup)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup",
                CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(core_h, TELEBOT_METHOD_SEND_STICKER, post);
}

