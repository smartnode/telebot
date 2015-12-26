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
#include <pthread.h>
#include <errno.h>
#include <json.h>
#include <json_object.h>
#include <telebot-private.h>
#include <telebot-common.h>
#include <telebot-core-api.h>
#include <telebot-api.h>
#include <telebot-parser.h>

static telebot_update_cb_f g_update_cb;
static telebot_core_h *g_handler;
static bool g_run_telebot;
static void *telebot_polling_thread(void *data);

telebot_error_e telebot_create(char *token)
{
    g_handler = (telebot_core_h *)malloc(sizeof(telebot_core_h));
    if (g_handler == NULL) {
        ERR("Failed to allocate memory");
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    telebot_error_e ret = telebot_core_create(g_handler, token);
    if (ret != TELEBOT_ERROR_NONE) {
        free(g_handler);
        return ret;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_destroy()
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_destroy(g_handler);
    if (g_handler != NULL)
        free(g_handler);

    g_handler = NULL;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_start(telebot_update_cb_f update_cb)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (update_cb == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    pthread_t thread_id;
    pthread_attr_t attr;

    int ret = pthread_attr_init(&attr);
    if (ret != 0) {
        ERR("Failed to init pthread attributes, error: %d", errno);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (ret != 0) {
        ERR("Failed to set pthread detatched attribute, error: %d", errno);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    g_update_cb = update_cb;
    g_run_telebot = true;

    ret = pthread_create(&thread_id, &attr, telebot_polling_thread, NULL);
    if (ret != 0) {
        ERR("Failed to create thread, error: %d", errno);
        g_update_cb = NULL;
        g_run_telebot = false;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_stop()
{
    g_run_telebot = false;
    g_update_cb = NULL;

    return TELEBOT_ERROR_NONE;
}

static void *telebot_polling_thread(void *data)
{
    int index;
    telebot_error_e ret;

    while (g_run_telebot) {
        int count;
        telebot_update_t *updates;
        ret = telebot_get_updates(&updates, &count);
        if (ret != TELEBOT_ERROR_NONE)
            continue;

        for (index = 0;index < count; index++) {
            g_update_cb((const telebot_message_t *)&(updates[index].message));
        }

        free(updates);
        updates = NULL;

        usleep(TELEBOT_UPDATE_POLLING_INTERVAL);
    }

    pthread_exit(NULL);

    return NULL;
}

telebot_error_e telebot_get_me(telebot_user_t **me)
{
    if (me == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    *me = NULL;

    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_error_e ret = telebot_core_get_me(g_handler);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    struct json_object *obj = telebot_parser_str_to_obj(g_handler->resp_data);
    free(g_handler->resp_data);
    g_handler->resp_data = NULL;
    g_handler->resp_size = 0;

    struct json_object *ok;
    if (!json_object_object_get_ex(obj, "ok", &ok)) {
        json_object_put(obj);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    if (!json_object_get_boolean(ok)) {
        json_object_put(ok);
        json_object_put(obj);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    json_object_put(ok);

    struct json_object *result;
    if (!json_object_object_get_ex(obj, "result", &result)){
        json_object_put(obj);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    telebot_user_t *tmp = (telebot_user_t*)malloc(sizeof(telebot_user_t));
    if (tmp == NULL) {
        json_object_put(result);
        json_object_put(obj);
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    ret = telebot_parser_get_user(result, tmp);
    json_object_put(result);
    json_object_put(obj);

    if (ret != TELEBOT_ERROR_NONE) {
        free(tmp);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    *me = tmp;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_get_updates(telebot_update_t **updates, int *count)
{
    if (count == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (updates == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    *updates = NULL;
    *count = 0;

    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_error_e ret = telebot_core_get_updates(g_handler, g_handler->offset,
            TELEBOT_UPDATE_COUNT_PER_REQUEST, 0);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    struct json_object *obj = telebot_parser_str_to_obj(g_handler->resp_data);
    free(g_handler->resp_data);
    g_handler->resp_data = NULL;
    g_handler->resp_size = 0;

    struct json_object *ok;
    if (!json_object_object_get_ex(obj, "ok", &ok)) {
        json_object_put(obj);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    if (!json_object_get_boolean(ok)) {
        json_object_put(ok);
        json_object_put(obj);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    json_object_put(ok);

    struct json_object *result;
    if (!json_object_object_get_ex(obj, "result", &result)){
        json_object_put(obj);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    ret = telebot_parser_get_updates(result, updates, count);
    json_object_put(result);
    json_object_put(obj);

    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    int index, cnt = *count;
    telebot_update_t *ups = *updates;
    for (index = 0;index < cnt; index++) {
        if (ups[index].update_id >= g_handler->offset)
            g_handler->offset = ups[index].update_id + 1;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_get_user_profile_photos(int user_id, int offset,
        telebot_photo_t **photos, int *count)
{
    if (photos == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (count == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    *photos = NULL;
    *count = 0;

    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_error_e ret = telebot_core_get_user_profile_photos(g_handler,
            user_id, offset, TELEBOT_USER_PHOTOS_MAX_LIMIT);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    struct json_object *obj = telebot_parser_str_to_obj(g_handler->resp_data);
    free(g_handler->resp_data);
    g_handler->resp_data = NULL;
    g_handler->resp_size = 0;

    struct json_object *ok;
    if (!json_object_object_get_ex(obj, "ok", &ok)) {
        json_object_put(obj);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    if (!json_object_get_boolean(ok)) {
        json_object_put(ok);
        json_object_put(obj);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    json_object_put(ok);

    struct json_object *result;
    if (!json_object_object_get_ex(obj, "result", &result)){
        json_object_put(obj);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    ret = telebot_parser_get_profile_photos(result, photos, count);
    json_object_put(result);
    json_object_put(obj);

    return ret;
}

telebot_error_e telebot_download_file(char *file_id, char *path)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (file_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_get_file(g_handler, file_id);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    char *file_path;
    struct json_object *obj = telebot_parser_str_to_obj(g_handler->resp_data);
    ret = telebot_parser_get_file_path(obj, &file_path);
    free(g_handler->resp_data);
    g_handler->resp_data = NULL;
    g_handler->resp_size = 0;
    json_object_put(obj);

    if (file_path == NULL)
        return TELEBOT_ERROR_OPERATION_FAILED;

    ret = telebot_core_download_file(g_handler, file_path, path);
    free(file_path);
    if (g_handler->resp_data != NULL) {
        free(g_handler->resp_data);
        g_handler->resp_data = NULL;
        g_handler->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_send_message(char *chat_id, char *text, char *parse_mode,
        bool disable_web_page_preview, int reply_to_message_id, char *reply_markup)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (chat_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (text == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_message(g_handler, chat_id, text,
            parse_mode, disable_web_page_preview, reply_to_message_id,
            reply_markup);

    if (g_handler->resp_data) {
        free(g_handler->resp_data);
        g_handler->resp_data = NULL;
        g_handler->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_forward_message(char *chat_id, char *from_chat_id,
        int message_id)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (chat_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (message_id <= 0)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_forward_message(g_handler, chat_id,
            from_chat_id, message_id);

    if (g_handler->resp_data) {
        free(g_handler->resp_data);
        g_handler->resp_data = NULL;
        g_handler->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_send_photo(char *chat_id, char *photo, bool is_file,
        char *caption, int reply_to_message_id, char *reply_markup)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (chat_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (photo == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_photo(g_handler, chat_id, photo,
            is_file, caption, reply_to_message_id, reply_markup);

    if (g_handler->resp_data) {
        free(g_handler->resp_data);
        g_handler->resp_data = NULL;
        g_handler->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_send_audio(char *chat_id, char *audio, bool is_file,
        int duration, char *performer, char *title, int reply_to_message_id,
        char *reply_markup)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (chat_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (audio == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_audio(g_handler, chat_id, audio,
            is_file, duration, performer, title, reply_to_message_id,
            reply_markup);

    if (g_handler->resp_data) {
        free(g_handler->resp_data);
        g_handler->resp_data = NULL;
        g_handler->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_send_document(char *chat_id, char *document,
        bool is_file, int reply_to_message_id, char *reply_markup)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (chat_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (document == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_document(g_handler, chat_id,
            document, is_file, reply_to_message_id, reply_markup);

    if (g_handler->resp_data) {
        free(g_handler->resp_data);
        g_handler->resp_data = NULL;
        g_handler->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_send_sticker(char *chat_id, char *sticker,
        bool is_file, int reply_to_message_id, char *reply_markup)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (chat_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (sticker == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_sticker(g_handler, chat_id, sticker,
            is_file, reply_to_message_id, reply_markup);

    if (g_handler->resp_data) {
        free(g_handler->resp_data);
        g_handler->resp_data = NULL;
        g_handler->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_send_video(char *chat_id, char *video, bool is_file,
        int duration, char *caption, int reply_to_message_id, char *reply_markup)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (chat_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (video == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_video(g_handler, chat_id, video,
            is_file, duration, caption, reply_to_message_id, reply_markup);

    if (g_handler->resp_data) {
        free(g_handler->resp_data);
        g_handler->resp_data = NULL;
        g_handler->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_send_voice(char *chat_id, char *voice, bool is_file,
        int duration, int reply_to_message_id, char *reply_markup)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (chat_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (voice == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_voice(g_handler, chat_id, voice,
            is_file, duration, reply_to_message_id, reply_markup);

    if (g_handler->resp_data) {
        free(g_handler->resp_data);
        g_handler->resp_data = NULL;
        g_handler->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_send_location(char *chat_id, float latitude,
        float longitude, int reply_to_message_id, char *reply_markup)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (chat_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_location(g_handler, chat_id,
            latitude, longitude, reply_to_message_id, reply_markup);

    if (g_handler->resp_data) {
        free(g_handler->resp_data);
        g_handler->resp_data = NULL;
        g_handler->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_send_chat_action(char *chat_id, char *action)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (chat_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_chat_action(g_handler, chat_id,
            action);

    if (g_handler->resp_data) {
        free(g_handler->resp_data);
        g_handler->resp_data = NULL;
        g_handler->resp_size = 0;
    }

    return ret;
}
