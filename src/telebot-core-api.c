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
#include <telebot-core-api.h>

static size_t write_data_cb(void *contents, size_t size, size_t nmemb, 
        void *userp)
{
    size_t r_size = size * nmemb;
    telebot_resp_t *resp = (telebot_resp_t *) userp;

    resp->data = realloc(resp->data, resp->size + r_size + 1);
    if (resp->data == NULL) {
        ERR("Failed to allocate memory, size:%u", (unsigned int)r_size);
        return 0;
    }

    memcpy((resp->data + resp->size), contents, r_size);
    resp->size += r_size;
    resp->data[resp->size] = 0;

    return r_size;
}

static telebot_error_e telebot_core_curl_perform(telebot_core_h *handler, 
        const char *method, struct curl_httppost *post)
{
    CURL *curl_h;
    CURLcode res;
    long resp_code = 0L;

    telebot_resp_t *resp = handler->response;
    resp->data = (char *)malloc(1);
    resp->size = 0;

    curl_h = curl_easy_init();

    if (curl_h == NULL) {
        ERR("Failed to init curl");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    char URL[TELEBOT_URL_SIZE];
    snprintf(URL, TELEBOT_URL_SIZE, "%s/bot%s/%s", TELEBOT_API_URL, 
            handler->token, method);
    curl_easy_setopt(curl_h, CURLOPT_URL, URL);
    curl_easy_setopt(curl_h, CURLOPT_WRITEFUNCTION, write_data_cb);
    curl_easy_setopt(curl_h, CURLOPT_WRITEDATA, resp);

    if (post != NULL)
        curl_easy_setopt(curl_h, CURLOPT_HTTPPOST, post);

    res = curl_easy_perform(curl_h);
    if (res != CURLE_OK) {
        ERR("Failed to curl_easy_perform\nError: %s (%d)", 
                curl_easy_strerror(res), res);
        if (resp->data != NULL)
            free(resp->data);
        resp->data = NULL;
        resp->size = 0;
        curl_easy_cleanup(curl_h);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    curl_easy_getinfo(curl_h, CURLINFO_RESPONSE_CODE, &resp_code);
    if (resp_code != 200L) {
        ERR("Wrong HTTP response received, response: %ld", resp_code); 
        if (resp->data != NULL)
            free(resp->data);
        resp->data = NULL;
        resp->size = 0;
        curl_easy_cleanup(curl_h);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    curl_easy_cleanup(curl_h);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_core_create(telebot_core_h *handler, char *token)
{
    if ((token == NULL) || (handler == NULL)) {
        ERR("Token(0x%p) or handler(0x%p) is NULL",
                token, handler);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    handler->token = strdup(token);
    handler->offset = 0;
    handler->response = (telebot_resp_t *)malloc(sizeof(telebot_resp_t));
    handler->response->data = NULL;
    handler->response->size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_core_destroy(telebot_core_h *handler)
{
    curl_global_cleanup();

    if (handler == NULL) {
        ERR("Handler(0x%p) is NULL", handler);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (handler->token != NULL) {
        memset(handler->token, 'X', strlen(handler->token));
        free(handler->token);
        handler->token = NULL;
    }

    if (handler->response->data != NULL)
        free(handler->response->data);

    if (handler->response != NULL)
        free(handler->response);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_core_get_me(telebot_core_h *handler)
{
    if (handler == NULL) {
        ERR("Handler is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((handler->token == NULL) || (handler->response == NULL)) {
        ERR("Token(0x%p) or response(0x%p) is NULL", handler->token, 
                handler->response);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    return telebot_core_curl_perform(handler, TELEBOT_METHOD_GET_ME, NULL);
}

telebot_error_e telebot_core_get_updates(telebot_core_h *handler, int offset, 
        int limit, int timeout)
{
    if (handler == NULL) {
        ERR("Handler is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((handler->token == NULL) || (handler->response == NULL)) {
        ERR("Token(0x%p) or response(0x%p) is NULL", handler->token, 
                handler->response);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    if (limit > TELEBOT_UPDATE_COUNT_MAX_LIMIT)
        limit = TELEBOT_UPDATE_COUNT_MAX_LIMIT;

    char offset_str[16], limit_str[16], timeout_str[16];
    snprintf(offset_str, sizeof(offset_str), "%d", offset);
    snprintf(limit_str, sizeof(limit_str), "%d", limit);
    snprintf(timeout_str, sizeof(timeout_str), "%d", timeout);

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "offset", 
            CURLFORM_COPYCONTENTS, offset_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "limit", 
            CURLFORM_COPYCONTENTS, limit_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "timeout", 
            CURLFORM_COPYCONTENTS, timeout_str, CURLFORM_END);

    return telebot_core_curl_perform(handler, TELEBOT_METHOD_GET_UPDATES, post);
}

telebot_error_e telebot_core_get_user_profile_photos(telebot_core_h *handler, 
        int user_id, int offset, int limit)
{
    if (handler == NULL) {
        ERR("Handler is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((handler->token == NULL) || (handler->response == NULL)) {
        ERR("Token(0x%p) or response(0x%p) is NULL", handler->token, 
                handler->response);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char offset_str[16], limit_str[16], user_id_str[16];
    snprintf(user_id_str, sizeof(user_id_str), "%d", user_id);
    snprintf(offset_str, sizeof(offset_str), "%d", offset);
    snprintf(limit_str, sizeof(limit_str), "%d", limit);

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "user_id", 
            CURLFORM_COPYCONTENTS, user_id_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "offset", 
            CURLFORM_COPYCONTENTS, offset_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "limit", 
            CURLFORM_COPYCONTENTS, limit_str, CURLFORM_END);

    return telebot_core_curl_perform(handler, TELEBOT_METHOD_GET_USERPHOTOS, post);
}

telebot_error_e telebot_core_get_file(telebot_core_h *handler, char *file_id)
{
    if (handler == NULL) {
        ERR("Handler is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((handler->token == NULL) || (handler->response == NULL)) {
        ERR("Token(0x%p) or response(0x%p) is NULL", handler->token, 
                handler->response);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if (file_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;
    
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "file_id", 
            CURLFORM_COPYCONTENTS, file_id, CURLFORM_END);

    return telebot_core_curl_perform(handler, TELEBOT_METHOD_GET_FILE, post);
}

telebot_error_e telebot_core_send_message(telebot_core_h *handler, char *chat_id, 
        char *text, char *parse_mode, bool disable_web_page_preview,
	int reply_to_message_id, char *reply_markup)
{
    if (handler == NULL) {
        ERR("Handler is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((handler->token == NULL) || (handler->response == NULL)) {
        ERR("Token(0x%p) or response(0x%p) is NULL", handler->token, 
                handler->response);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char reply_to_message_id_str[16];
    snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d", 
            reply_to_message_id);

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id", 
            CURLFORM_COPYCONTENTS, chat_id, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "text", 
            CURLFORM_COPYCONTENTS, text, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "parse_mode",
                CURLFORM_COPYCONTENTS, parse_mode, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "disable_web_page_preview", 
            CURLFORM_COPYCONTENTS, (disable_web_page_preview) ? "true" : "false", 
            CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id", 
            CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup", 
            CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(handler, TELEBOT_METHOD_SEND_MESSAGE, post);
}

telebot_error_e telebot_core_forward_message(telebot_core_h *handler, 
        char *chat_id, char *from_chat_id, int message_id)
{
    if (handler == NULL) {
        ERR("Handler is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((handler->token == NULL) || (handler->response == NULL)) {
        ERR("Token(0x%p) or response(0x%p) is NULL", handler->token, 
                handler->response);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char message_id_str[16];
    snprintf(message_id_str, sizeof(message_id_str), "%d", message_id);

    curl_formadd (&post, &last, CURLFORM_COPYNAME, "chat_id", 
            CURLFORM_COPYCONTENTS, chat_id, CURLFORM_END);
    curl_formadd (&post, &last, CURLFORM_COPYNAME, "from_chat_id", 
            CURLFORM_COPYCONTENTS, from_chat_id, CURLFORM_END);
    curl_formadd (&post, &last, CURLFORM_COPYNAME, "message_id", 
            CURLFORM_COPYCONTENTS, message_id_str, CURLFORM_END);

    return telebot_core_curl_perform(handler, TELEBOT_METHOD_FORWARD_MESSAGE, post);
}

telebot_error_e telebot_core_send_photo(telebot_core_h *handler, char *chat_id, 
        char *photo, bool is_file, char *caption, int reply_to_message_id, 
        char *reply_markup)
{
    if (handler == NULL) {
        ERR("Handler is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((handler->token == NULL) || (handler->response == NULL)) {
        ERR("Token(0x%p) or response(0x%p) is NULL", handler->token, 
                handler->response);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }
     
    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char reply_to_message_id_str[16];
    snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d", 
            reply_to_message_id);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id", 
            CURLFORM_COPYCONTENTS, chat_id, CURLFORM_END);

    if (is_file)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "photo", 
                CURLFORM_FILE, photo, CURLFORM_END);
    else
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "photo", 
                CURLFORM_COPYCONTENTS, photo, CURLFORM_END);
    
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "caption", 
            CURLFORM_COPYCONTENTS, caption, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id", 
            CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup", 
            CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(handler, TELEBOT_METHOD_SEND_PHOTO, post);
}

telebot_error_e telebot_core_send_audio(telebot_core_h *handler, char *chat_id, 
        char *audio, bool is_file, int duration, char *performer, char *title, 
        int reply_to_message_id, char *reply_markup)
{
    if (handler == NULL) {
        ERR("Handler is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((handler->token == NULL) || (handler->response == NULL)) {
        ERR("Token(0x%p) or response(0x%p) is NULL", handler->token, 
                handler->response);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char duration_str[16], reply_to_message_id_str[16];
    snprintf(duration_str, sizeof(duration_str), "%d", duration);
    snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d", 
            reply_to_message_id);

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id", 
            CURLFORM_COPYCONTENTS, chat_id, CURLFORM_END);
   
    if (is_file) 
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "audio", 
            CURLFORM_FILE, audio, CURLFORM_END);
    else 
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "audio", 
            CURLFORM_COPYCONTENTS, audio, CURLFORM_END);

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "duration", 
            CURLFORM_COPYCONTENTS, duration_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "performer", 
            CURLFORM_COPYCONTENTS, performer, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "title", 
            CURLFORM_COPYCONTENTS, title, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id", 
            CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup", 
            CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(handler, TELEBOT_METHOD_SEND_AUDIO, post);
}

telebot_error_e telebot_core_send_document(telebot_core_h *handler, char *chat_id, 
        char *document, bool is_file, int reply_to_message_id, char *reply_markup)
{
    if (handler == NULL) {
        ERR("Handler is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((handler->token == NULL) || (handler->response == NULL)) {
        ERR("Token(0x%p) or response(0x%p) is NULL", handler->token, 
                handler->response);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }
    
    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char reply_to_message_id_str[16];
    snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d", 
            reply_to_message_id);

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id", 
            CURLFORM_COPYCONTENTS, chat_id, CURLFORM_END);
    
    if (is_file)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "document", 
               CURLFORM_FILE, document, CURLFORM_END);
    else
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "document", 
                CURLFORM_COPYCONTENTS, document, CURLFORM_END);

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id", 
            CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup", 
            CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(handler, TELEBOT_METHOD_SEND_PHOTO, post);
}

telebot_error_e telebot_core_send_sticker(telebot_core_h *handler, char *chat_id, 
        char *sticker, bool is_file, int reply_to_message_id, char *reply_markup)
{
    if (handler == NULL) {
        ERR("Handler is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((handler->token == NULL) || (handler->response == NULL)) {
        ERR("Token(0x%p) or response(0x%p) is NULL", handler->token, 
                handler->response);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char reply_to_message_id_str[16];
    snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d", 
            reply_to_message_id);

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id", 
            CURLFORM_COPYCONTENTS, chat_id, CURLFORM_END);
    
    if (is_file)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "sticker", 
                CURLFORM_FILE, sticker, CURLFORM_END);
    else 
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "sticker", 
                CURLFORM_COPYCONTENTS, sticker, CURLFORM_END);

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id", 
            CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup", 
            CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(handler, TELEBOT_METHOD_SEND_STICKER, post);
}

telebot_error_e telebot_core_send_video(telebot_core_h *handler, char *chat_id, 
        char *video, bool is_file, int duration, char *caption, 
        int reply_to_message_id, char *reply_markup)
{
    if (handler == NULL) {
        ERR("Handler is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((handler->token == NULL) || (handler->response == NULL)) {
        ERR("Token(0x%p) or response(0x%p) is NULL", handler->token, 
                handler->response);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char duration_str[16], reply_to_message_id_str[16];
    snprintf(duration_str, sizeof(duration_str), "%d", duration);
    snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d", 
            reply_to_message_id);

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id", 
            CURLFORM_COPYCONTENTS, chat_id, CURLFORM_END);

    if (is_file)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "video", 
                CURLFORM_FILE, video, CURLFORM_END);
    else
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "video", 
                CURLFORM_COPYCONTENTS, video, CURLFORM_END);

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "duration", 
            CURLFORM_COPYCONTENTS, duration_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "caption", 
            CURLFORM_COPYCONTENTS, caption, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id", 
            CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup", 
            CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(handler, TELEBOT_METHOD_SEND_VIDEO, post);
}

telebot_error_e telebot_core_send_voice(telebot_core_h *handler, char *chat_id, 
        char *voice, bool is_file, int duration, int reply_to_message_id,
        char *reply_markup)
{
    if (handler == NULL) {
        ERR("Handler is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((handler->token == NULL) || (handler->response == NULL)) {
        ERR("Token(0x%p) or response(0x%p) is NULL", handler->token, 
                handler->response);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char duration_str[16], reply_to_message_id_str[16];
    snprintf(duration_str, sizeof(duration_str), "%d", duration);
    snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d", 
            reply_to_message_id);

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id", 
            CURLFORM_COPYCONTENTS, chat_id, CURLFORM_END);
   
    if (is_file)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "voice", 
                CURLFORM_FILE, voice, CURLFORM_END);
    else 
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "voice", 
                CURLFORM_COPYCONTENTS, voice, CURLFORM_END);

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "duration", 
            CURLFORM_COPYCONTENTS, duration_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id", 
            CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup", 
            CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(handler, TELEBOT_METHOD_SEND_VOICE, post);
}

telebot_error_e telebot_core_send_location(telebot_core_h *handler, 
        char *chat_id, float latitude, float longitude, int reply_to_message_id, 
        char *reply_markup)
{
    if (handler == NULL) {
        ERR("Handler is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((handler->token == NULL) || (handler->response == NULL)) {
        ERR("Token(0x%p) or response(0x%p) is NULL", handler->token, 
                handler->response);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char latitude_str[32], longitude_str[32];
    char reply_to_message_id_str[16];
    snprintf(latitude_str, sizeof(latitude_str), "%f", latitude);
    snprintf(longitude_str, sizeof(longitude_str), "%f", longitude);
    snprintf(reply_to_message_id_str, sizeof(reply_to_message_id_str), "%d", 
            reply_to_message_id);

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id", 
            CURLFORM_COPYCONTENTS, chat_id, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "latitude", 
            CURLFORM_COPYCONTENTS, latitude_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "longitude", 
            CURLFORM_COPYCONTENTS, longitude_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_to_message_id", 
            CURLFORM_COPYCONTENTS, reply_to_message_id_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "reply_markup", 
            CURLFORM_COPYCONTENTS, reply_markup, CURLFORM_END);

    return telebot_core_curl_perform(handler, TELEBOT_METHOD_SEND_LOCATION, post);
}

telebot_error_e telebot_core_send_chat_action(telebot_core_h *handler, 
        char *chat_id, char *action)
{
    if (handler == NULL) {
        ERR("Handler is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((handler->token == NULL) || (handler->response == NULL)) {
        ERR("Token(0x%p) or response(0x%p) is NULL", handler->token, 
                handler->response);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "chat_id", 
            CURLFORM_COPYCONTENTS, chat_id, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "action", 
            CURLFORM_COPYCONTENTS, action, CURLFORM_END);

    return telebot_core_curl_perform(handler, TELEBOT_METHOD_SEND_CHATACTION, post);
}

telebot_error_e telebot_core_set_web_hook(telebot_core_h *handler, char *url, 
        char *certificate_file)
{
    if (handler == NULL) {
        ERR("Handler is NULL");
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    if ((handler->token == NULL) || (handler->response == NULL)) {
        ERR("Token(0x%p) or response(0x%p) is NULL", handler->token, 
                handler->response);
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "url", 
            CURLFORM_COPYCONTENTS, url, CURLFORM_END);
    
    if (certificate_file != NULL)
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "certificate", 
                CURLFORM_FILE, certificate_file, CURLFORM_END);

    return telebot_core_curl_perform(handler, TELEBOT_METHOD_SET_WEBHOOK, post);
}

static size_t write_file_cb(void *contents, size_t size, size_t nmemb, 
        void *userp)
{
    size_t written = fwrite(contents, size, nmemb, (FILE *)userp);
    return written;
}

telebot_error_e telebot_core_download_file(telebot_core_h *handler, 
        char *file_path, char *out_file)
{
    if ((handler == NULL) || (handler->token == NULL))
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
            handler->token, file_path);

    curl_easy_setopt(curl_h, CURLOPT_URL, URL);
    curl_easy_setopt(curl_h, CURLOPT_WRITEFUNCTION, write_file_cb);
    curl_easy_setopt(curl_h, CURLOPT_WRITEDATA, fp);

    res = curl_easy_perform(curl_h);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl_h);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    curl_easy_getinfo(curl_h, CURLINFO_RESPONSE_CODE, &resp_code);
    if (resp_code != 200L) {
        fclose(fp);
        unlink(out_file);
        curl_easy_cleanup(curl_h);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    fclose(fp);

    curl_easy_cleanup(curl_h);

    return TELEBOT_ERROR_NONE;
}

