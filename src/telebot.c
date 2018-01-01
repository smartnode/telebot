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
#include <assert.h>
#include <errno.h>
#include <json.h>
#include <json_object.h>
#include <telebot.h>
#include <telebot-core.h>
#include <telebot-private.h>
#include <telebot-parser.h>

typedef struct telebot_handler_s {
    telebot_core_handler_t *core_h;
    int offset;
} telebot_hdata_t;

static const char *telebot_update_type_str[UPDATE_TYPE_MAX] = {
    "message", "edited_message", "channel_post",
    "edited_channel_post", "inline_query",
    "chonse_inline_result", "callback_query",
    "shipping_query", "pre_checkout_query"
};

telebot_error_e telebot_create(telebot_handler_t *handle, char *token)
{
    if ((token == NULL) || (handle == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_hdata_t *_handle = (telebot_hdata_t *)malloc(sizeof(telebot_hdata_t));
    if (_handle == NULL) {
        ERR("Failed to allocate memory");
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    telebot_error_e ret = telebot_core_create(&(_handle->core_h), token);
    if (ret != TELEBOT_ERROR_NONE) {
        free(_handle);
        return ret;
    }

    _handle->offset = 0;

    *handle = _handle;
    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_destroy(telebot_handler_t handle)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_destroy(_handle->core_h);

    free(_handle);
    _handle = NULL;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_get_updates(telebot_handler_t handle, int offset,
        int limit, int timeout, telebot_update_type_e allowed_updates[],
        int allowed_updates_count, telebot_update_t **updates, int *count)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((updates == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    *updates = NULL;
    *count = 0;

    int i = 0;
    char allowed_updates_str[1024] = "";
    if (allowed_updates_count > 0) {
        snprintf(allowed_updates_str, 1024, "%s", "[");
        for (i=0;i<allowed_updates_count;i++) {
            if (i < (allowed_updates_count-1)) //intermediate element
                snprintf(allowed_updates_str, 1024, "%s%s,", allowed_updates_str,
                        telebot_update_type_str[allowed_updates[i]]);
            else // last element
                snprintf(allowed_updates_str, 1024, "%s%s", allowed_updates_str,
                        telebot_update_type_str[allowed_updates[i]]);
        }
        snprintf(allowed_updates_str, 1024, "%s%s", allowed_updates_str,"]");
    }

    int _offset = offset != 0 ? offset : _handle->offset;
    int _timeout = timeout > 0 ? timeout : 0;
    int _limit = TELEBOT_UPDATE_COUNT_MAX_LIMIT;
    if ((limit > 0) && (limit < TELEBOT_UPDATE_COUNT_MAX_LIMIT))
        _limit = limit;

    int ret = telebot_core_get_updates(_handle->core_h, _offset,
            _limit, _timeout, allowed_updates_str);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    struct json_object *obj = telebot_parser_str_to_obj(_handle->core_h->resp_data);
    free(_handle->core_h->resp_data);
    _handle->core_h->resp_data = NULL;
    _handle->core_h->resp_size = 0;

    if (obj == NULL)
        return TELEBOT_ERROR_OPERATION_FAILED;

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
    if (!json_object_object_get_ex(obj, "result", &result)) {
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
        if (ups[index].update_id >= _handle->offset)
            _handle->offset = ups[index].update_id + 1;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_get_me(telebot_handler_t handle, telebot_user_t *me)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (me == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(me, 0, sizeof(telebot_user_t));
    telebot_error_e ret = telebot_core_get_me(_handle->core_h);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    struct json_object *obj = telebot_parser_str_to_obj(_handle->core_h->resp_data);
    free(_handle->core_h->resp_data);
    _handle->core_h->resp_data = NULL;
    _handle->core_h->resp_size = 0;

    if (obj == NULL)
        return TELEBOT_ERROR_OPERATION_FAILED;

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

    ret = telebot_parser_get_user(result, me);
    json_object_put(result);
    json_object_put(obj);

    if (ret != TELEBOT_ERROR_NONE)
        return TELEBOT_ERROR_OPERATION_FAILED;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_send_message(telebot_handler_t handle, int chat_id,
        char *text, char *parse_mode, bool disable_web_page_preview,
        bool disable_notification, int reply_to_message_id, const char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((chat_id <= 0) || (text == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_message(_handle->core_h, chat_id,
            text, parse_mode, disable_web_page_preview, disable_notification,
            reply_to_message_id, reply_markup);

    if (_handle->core_h->resp_data) {
        free(_handle->core_h->resp_data);
        _handle->core_h->resp_data = NULL;
        _handle->core_h->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_forward_message(telebot_handler_t handle, int chat_id,
        char *from_chat_id, bool disable_notification, int message_id)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((chat_id <= 0) || (message_id <= 0))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_forward_message(_handle->core_h, chat_id,
            from_chat_id, disable_notification, message_id);

    if (_handle->core_h->resp_data) {
        free(_handle->core_h->resp_data);
        _handle->core_h->resp_data = NULL;
        _handle->core_h->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_send_photo(telebot_handler_t handle, int chat_id,
        char *photo, bool is_file, char *caption, bool disable_notification,
        int reply_to_message_id, char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((chat_id <= 0) || (photo == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_photo(_handle->core_h, chat_id, photo,
            is_file, caption, disable_notification, reply_to_message_id, reply_markup);

    if (_handle->core_h->resp_data) {
        free(_handle->core_h->resp_data);
        _handle->core_h->resp_data = NULL;
        _handle->core_h->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_send_audio(telebot_handler_t handle, int chat_id,
        char *audio, bool is_file, int duration, char *performer, char *title,
        bool disable_notification, int reply_to_message_id, char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((chat_id <= 0) ||  (audio == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_audio(_handle->core_h, chat_id, audio,
            is_file, duration, performer, title, disable_notification,
            reply_to_message_id, reply_markup);

    if (_handle->core_h->resp_data) {
        free(_handle->core_h->resp_data);
        _handle->core_h->resp_data = NULL;
        _handle->core_h->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_send_document(telebot_handler_t handle, int chat_id,
        char *document, bool is_file, bool disable_notification,
        int reply_to_message_id, char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((chat_id <= 0) || (document == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_document(_handle->core_h, chat_id,
            document, is_file, disable_notification, reply_to_message_id, reply_markup);

    if (_handle->core_h->resp_data) {
        free(_handle->core_h->resp_data);
        _handle->core_h->resp_data = NULL;
        _handle->core_h->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_send_video(telebot_handler_t handle, int chat_id,
        char *video, bool is_file, int duration, int width, int height,
        char *caption, bool disable_notification, int reply_to_message_id,
        char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((chat_id <= 0) || (video == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_video(_handle->core_h, chat_id, video,
            is_file, duration, caption, disable_notification, reply_to_message_id,
            reply_markup);

    if (_handle->core_h->resp_data) {
        free(_handle->core_h->resp_data);
        _handle->core_h->resp_data = NULL;
        _handle->core_h->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_send_voice(telebot_handler_t handle, int chat_id,
        char *voice, bool is_file, char *caption, int duration,
        bool disable_notification, int reply_to_message_id, char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((chat_id <= 0) || (voice == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_voice(_handle->core_h, chat_id, voice,
            is_file, duration, disable_notification, reply_to_message_id, reply_markup);

    if (_handle->core_h->resp_data) {
        free(_handle->core_h->resp_data);
        _handle->core_h->resp_data = NULL;
        _handle->core_h->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_send_video_note(telebot_handler_t handle, int chat_id,
        char *video_note, bool is_file, int duration, int length,
        bool disable_notification, int reply_to_message_id, char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((chat_id <= 0) || (video_note == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_video_note(_handle->core_h, chat_id, video_note,
            is_file, duration, length, disable_notification, reply_to_message_id, reply_markup);

    if (_handle->core_h->resp_data) {
        free(_handle->core_h->resp_data);
        _handle->core_h->resp_data = NULL;
        _handle->core_h->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_send_location(telebot_handler_t handle, int chat_id,
        float latitude, float longitude, bool disable_notification,
        int reply_to_message_id, char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (chat_id <= 0)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_location(_handle->core_h, chat_id,
            latitude, longitude, disable_notification, reply_to_message_id, reply_markup);

    if (_handle->core_h->resp_data) {
        free(_handle->core_h->resp_data);
        _handle->core_h->resp_data = NULL;
        _handle->core_h->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_send_chat_action(telebot_handler_t handle, int chat_id,
        char *action)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (chat_id <= 0)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_chat_action(_handle->core_h,
            chat_id, action);

    if (_handle->core_h->resp_data) {
        free(_handle->core_h->resp_data);
        _handle->core_h->resp_data = NULL;
        _handle->core_h->resp_size = 0;
    }

    return ret;
}

telebot_error_e telebot_get_user_profile_photos(telebot_handler_t handle,
        int user_id, int offset, int limit, telebot_user_profile_photos_t *photos)
{
    if (photos == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(photos, 0, sizeof(telebot_user_profile_photos_t));

    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((limit <= 0) || (limit > TELEBOT_USER_PROFILE_PHOTOS_LIMIT))
        limit = TELEBOT_USER_PROFILE_PHOTOS_LIMIT;

    telebot_error_e ret = telebot_core_get_user_profile_photos(_handle->core_h,
            user_id, offset, limit);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    struct json_object *obj = telebot_parser_str_to_obj(_handle->core_h->resp_data);
    free(_handle->core_h->resp_data);
    _handle->core_h->resp_data = NULL;
    _handle->core_h->resp_size = 0;

    if (obj == NULL)
        return TELEBOT_ERROR_OPERATION_FAILED;

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
    if (!json_object_object_get_ex(obj, "result", &result)) {
        json_object_put(obj);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    ret = telebot_parser_get_user_profile_photos(result, photos);
    json_object_put(result);
    json_object_put(obj);

    return ret;
}

telebot_error_e telebot_download_file(telebot_handler_t handle, char *file_id, char *path)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (file_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_get_file(_handle->core_h, file_id);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    struct json_object *obj = telebot_parser_str_to_obj(_handle->core_h->resp_data);
    if (obj == NULL) {
        free(_handle->core_h->resp_data);
        _handle->core_h->resp_data = NULL;
        _handle->core_h->resp_size = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    char *file_path;
    ret = telebot_parser_get_file_path(obj, &file_path);
    json_object_put(obj);
    free(_handle->core_h->resp_data);
    _handle->core_h->resp_data = NULL;
    _handle->core_h->resp_size = 0;

    if (file_path == NULL)
        return TELEBOT_ERROR_OPERATION_FAILED;

    ret = telebot_core_download_file(_handle->core_h, file_path, path);
    free(file_path);
    if (_handle->core_h->resp_data != NULL) {
        free(_handle->core_h->resp_data);
        _handle->core_h->resp_data = NULL;
        _handle->core_h->resp_size = 0;
    }

    return ret;
}

#if 0
telebot_error_e telebot_delete_message(int chat_id, int message_id)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((chat_id <= 0) || (message_id <= 0))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_delete_message(g_handler, chat_id, message_id);

    if (g_handler->resp_data) {
        free(g_handler->resp_data);
        g_handler->resp_data = NULL;
        g_handler->resp_size = 0;
    }

    return ret;
}



telebot_error_e telebot_send_sticker(int chat_id, char *sticker,
        bool is_file, int reply_to_message_id, char *reply_markup)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((chat_id <= 0) || (sticker == NULL))
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

telebot_error_e telebot_create_reply_keyboard(bool resize, bool one_time, bool selective,
        telebot_keyboard_t **keyboard)
{
    if (keyboard == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    *keyboard = NULL;

    telebot_keyboard_t *kbd = malloc(sizeof(telebot_keyboard_t));
    if (kbd == NULL) {
        ERR("Failed to allocate memory for keyboard object");
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }
    kbd->keyboard_obj = json_object_new_object();
    if (kbd->keyboard_obj == NULL) {
        ERR("Failed to allocate memory for keyboard group");
        free(kbd);
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    kbd->rows = json_object_new_array();
    if (kbd->rows == NULL) {
        ERR("Failed to allocate memory for keyboard rows");
        json_object_put(kbd->keyboard_obj);
        free(kbd);
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    kbd->current_row = json_object_new_array();
    if (kbd->current_row == NULL) {
        ERR("Failed to allocate memory for keyboard current row");
        json_object_put(kbd->keyboard_obj);
        free(kbd);
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    json_object_array_add(kbd->rows, kbd->current_row);
    json_object_object_add(kbd->keyboard_obj, "keyboard", kbd->rows);

    if (resize) {
        json_object_object_add(kbd->keyboard_obj, "resize_keyboard",
                json_object_new_boolean(true));
    }
    if (one_time) {
        json_object_object_add(kbd->keyboard_obj, "one_time_keyboard",
                json_object_new_boolean(true));
    }
    if (selective) {
        json_object_object_add(kbd->keyboard_obj, "selective",
                json_object_new_boolean(true));
    }

    *keyboard = kbd;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e  telebot_create_inline_keyboard(telebot_keyboard_t **keyboard)
{
    if (keyboard == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    *keyboard = NULL;

    telebot_keyboard_t *kbd = malloc(sizeof(telebot_keyboard_t));
    if (kbd == NULL) {
        ERR("Failed to allocate memory for keyboard object");
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }
    kbd->keyboard_obj = json_object_new_object();
    if (kbd->keyboard_obj == NULL) {
        ERR("Failed to allocate memory for keyboard group");
        free(kbd);
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    kbd->rows = json_object_new_array();
    if (kbd->rows == NULL) {
        ERR("Failed to allocate memory for keyboard rows");
        json_object_put(kbd->keyboard_obj);
        free(kbd);
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    kbd->current_row = json_object_new_array();
    if (kbd->current_row == NULL) {
        ERR("Failed to allocate memory for keyboard current row");
        json_object_put(kbd->keyboard_obj);
        free(kbd);
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    json_object_array_add(kbd->rows, kbd->current_row);
    json_object_object_add(kbd->keyboard_obj, "keyboard", kbd->rows);


    *keyboard = kbd;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_destroy_keyboard(telebot_keyboard_t *keyboard)
{
    /* TODO: Should we release all objects within keyboard object
       size_t rows = json_object_array_length(keyboard->rows);
       for (i=0;i<rows;i++) {
       size_t buttons =
       }
       */

    json_object_put(keyboard->keyboard_obj);
    free(keyboard);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_keyboard_add_row(telebot_keyboard_t *keyboard)
{
    if (keyboard == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    keyboard->current_row = json_object_new_array();
    if (keyboard->current_row == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    json_object_array_add(keyboard->rows, keyboard->current_row);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_keyboard_add_reply_button(telebot_keyboard_t* keyboard,
        char* text, bool request_contact, bool request_location)
{
    if ((!request_contact) && !(request_location)) {
        json_object_array_add(keyboard->current_row, json_object_new_string(text));
    } else {
        json_object* button = json_object_new_object();
        json_object_object_add(button, "text",
                json_object_new_string(text));
        if(request_contact) {
            json_object_object_add(button, "request_contact", json_object_new_boolean(true));
        }
        if(request_location) {
            json_object_object_add(button, "request_location", json_object_new_boolean(true));
        }
        json_object_array_add(keyboard->current_row, button);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_keyboard_add_inline_button(telebot_keyboard_t *keyboard,
        char* text, char* url, char* callback_data, char* switch_inline_query,
        char* switch_inline_query_current_chat, bool pay)
{
    json_object* button = json_object_new_object();
    json_object_object_add(button, "text", json_object_new_string(text));
    if (url) {
        json_object_object_add(button, "url",
                json_object_new_string(url));
    }
    if (callback_data) {
        size_t len = strlen(callback_data);
        assert(len >= 1 && len <= 64);
        json_object_object_add(button, "callback_data",
                json_object_new_string(callback_data));
    }
    if (switch_inline_query) {
        json_object_object_add(button, "switch_inline_query",
                json_object_new_string(switch_inline_query));
    }
    if (switch_inline_query_current_chat) {
        json_object_object_add(button, "switch_inline_query_current_chat",
                json_object_new_string(switch_inline_query_current_chat));
    }
    if (pay) {
        json_object_object_add(button, "pay",
                json_object_new_boolean(true));
    }

    json_object_array_add(keyboard->current_row, button);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_keyboard_stringify(telebot_keyboard_t *keyboard,
        char **keyboard_str)
{
    if (keyboard_str == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (keyboard == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    *keyboard_str = NULL;
    const char* result = json_object_to_json_string_ext(keyboard->keyboard_obj, JSON_C_TO_STRING_PLAIN);

    if (result == NULL)
        return TELEBOT_ERROR_OPERATION_FAILED;

    *keyboard_str = strdup(result);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_answer_callback_query(const char *callback_query_id, char *text,
        bool show_alert, char *url, int cache_time)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (callback_query_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_answer_callback_query(g_handler,
            callback_query_id, text,
            show_alert, url,
            cache_time);

    if (g_handler->resp_data) {
        free(g_handler->resp_data);
        g_handler->resp_data = NULL;
        g_handler->resp_size = 0;
    }

    return ret;
}

#endif
