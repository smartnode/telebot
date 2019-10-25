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

const char *telebot_update_type_str[UPDATE_TYPE_MAX] = {
    "message", "edited_message", "channel_post",
    "edited_channel_post", "inline_query",
    "chonse_inline_result", "callback_query",
    "shipping_query", "pre_checkout_query"
};


/* Free and nullify and zero count */
#define tb_sfree(addr)               if (addr) { free(addr); addr = NULL; }
#define tb_sfree_zcnt(addr, count)   tb_sfree(addr); count = 0;

static void telebot_free_user(telebot_user_t *user);
static void telebot_free_chat(telebot_chat_t *chat);
static void telebot_free_chat_photo(telebot_chat_photo_t *photo);
static void telebot_free_message(telebot_message_t *msg);
static void telebot_free_telebot_message_entity(telebot_message_entity_t *entity);
static void telebot_free_audio(telebot_audio_t *audio);
static void telebot_free_document(telebot_document_t *document);
static void telebot_free_photo(telebot_photo_t *photo);
static void telebot_free_sticker(telebot_sticker_t *sticker);
static void telebot_free_video(telebot_video_t *video);
static void telebot_free_animation(telebot_animation_t *animation);
static void telebot_free_voice(telebot_voice_t *voice);
static void telebot_free_video_note(telebot_video_note_t *vnote);
static void telebot_free_contact(telebot_contact_t *contact);
static void telebot_free_location(telebot_location_t *location);
static void telebot_free_venue(telebot_venue_t *venue);
static void telebot_free_callback_query(telebot_callback_query_t *query);
static void telebot_free_mask_position(telebot_mask_position_t *mask);
//static void telebot_free_game(telebot_document_t *game);
//static void telebot_free_invoice(telebot_invoice_t *invoice);
//static void telebot_free_payment(telebot_successful_payment_t *payment);

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
        tb_sfree(_handle);
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
    tb_sfree(_handle);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_set_proxy(telebot_handler_t handle, char *addr, char *auth)
{
    if (addr == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_error_e ret = telebot_core_set_proxy(_handle->core_h, addr, auth);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_get_proxy(telebot_handler_t handle, char **addr)
{
    if (addr == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    *addr = NULL;

    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    return telebot_core_get_proxy(_handle->core_h, addr);
}


telebot_error_e telebot_get_updates(telebot_handler_t handle, int offset,
        int limit, int timeout, telebot_update_type_e allowed_updates[],
        int allowed_updates_count, telebot_update_t **updates, int *count)
{
    int i = 0;
    char allowed_updates_str[TELEBOT_BUFFER_PAGE] = {0, };
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((updates == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    *updates = NULL;
    *count = 0;

    if (allowed_updates_count > 0) {
        strncat(allowed_updates_str, "[", TELEBOT_BUFFER_BLOCK);
        for (i=0;i<allowed_updates_count;i++) {
            strncat(allowed_updates_str, telebot_update_type_str[allowed_updates[i]],
                TELEBOT_BUFFER_BLOCK);
            if (i < (allowed_updates_count-1)) //intermediate element
                strncat(allowed_updates_str, ",", TELEBOT_BUFFER_BLOCK);
        }
        strncat(allowed_updates_str, "]", TELEBOT_BUFFER_BLOCK);
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
    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    if (obj == NULL)
        return TELEBOT_ERROR_OPERATION_FAILED;

    struct json_object *ok;
    if (!json_object_object_get_ex(obj, "ok", &ok)) {
        json_object_put(obj);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    if (!json_object_get_boolean(ok)) {
        json_object_put(obj);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *result;
    if (!json_object_object_get_ex(obj, "result", &result)) {
        json_object_put(obj);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    ret = telebot_parser_get_updates(result, updates, count);
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

telebot_error_e telebot_free_updates(telebot_update_t *updates, int count)
{
    int index;

    if ((updates == NULL) || (count <= 0))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    for (index=0;index<count; index++) {
        if (updates[index].update_type == UPDATE_TYPE_MESSAGE)
            telebot_free_message(&(updates[index].message));
        else if (updates[index].update_type == UPDATE_TYPE_EDITED_MESSAGE)
            telebot_free_message(&(updates[index].edited_message));
        else if (updates[index].update_type == UPDATE_TYPE_CHANNEL_POST)
            telebot_free_message(&(updates[index].channel_post));
        else if (updates[index].update_type == UPDATE_TYPE_EDITED_CHANNEL_POST)
            telebot_free_message(&(updates[index].edited_channel_post));
        else if (updates[index].update_type == UPDATE_TYPE_CALLBACK_QUERY)
            telebot_free_callback_query(&(updates[index].callback_query));
    }

    tb_sfree(updates);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_get_me(telebot_handler_t handle, telebot_user_t **me)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (me == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    *me = (telebot_user_t*)malloc(sizeof(telebot_user_t));
    if (*me == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    telebot_error_e ret = telebot_core_get_me(_handle->core_h);
    if (ret != TELEBOT_ERROR_NONE) {
        tb_sfree(*me);
        return ret;
    }

    struct json_object *obj = telebot_parser_str_to_obj(_handle->core_h->resp_data);
    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    if (obj == NULL) {
        tb_sfree(*me);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *ok;
    if (!json_object_object_get_ex(obj, "ok", &ok)) {
        json_object_put(obj);
        tb_sfree(*me);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    if (!json_object_get_boolean(ok)) {
        json_object_put(obj);
        tb_sfree(*me);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *result;
    if (!json_object_object_get_ex(obj, "result", &result)){
        json_object_put(obj);
        tb_sfree(*me);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    ret = telebot_parser_get_user(result, *me);
    json_object_put(obj);

    if (ret != TELEBOT_ERROR_NONE) {
        tb_sfree(*me);
        return ret;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_free_me(telebot_user_t *me)
{
    if (me == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_free_user(me);
    tb_sfree(me);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_set_webhook(telebot_handler_t handle, char *url,
        char *certificate, int max_connections,
        telebot_update_type_e allowed_updates[], int allowed_updates_count)
{
    int i = 0;
    char allowed_updates_str[TELEBOT_BUFFER_PAGE] = {0, };
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (url == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (allowed_updates_count > 0) {
        strncat(allowed_updates_str, "[", TELEBOT_BUFFER_BLOCK);
        for (i=0;i<allowed_updates_count;i++) {
            strncat(allowed_updates_str, telebot_update_type_str[allowed_updates[i]],
                TELEBOT_BUFFER_BLOCK);
            if (i < (allowed_updates_count-1)) //intermediate element
                strncat(allowed_updates_str, ",", TELEBOT_BUFFER_BLOCK);
        }
        strncat(allowed_updates_str, "]", TELEBOT_BUFFER_BLOCK);
    }

    telebot_error_e ret = telebot_core_set_webhook(_handle->core_h, url,
            certificate, max_connections, allowed_updates_str);

    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    return ret;
}

telebot_error_e telebot_delete_webhook(telebot_handler_t handle)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_error_e ret = telebot_core_delete_webhook(_handle->core_h);

    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    return ret;
}


telebot_error_e telebot_get_webhook_info(telebot_handler_t handle,
        telebot_webhook_info_t **info)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    *info = (telebot_webhook_info_t*)malloc(sizeof(telebot_webhook_info_t));
    if (*info == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    telebot_error_e ret = telebot_core_get_webhook_info(_handle->core_h);
    if (ret != TELEBOT_ERROR_NONE) {
        tb_sfree(*info);
        return ret;
    }

    struct json_object *obj = telebot_parser_str_to_obj(_handle->core_h->resp_data);
    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    if (obj == NULL) {
        tb_sfree(*info);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *ok;
    if (!json_object_object_get_ex(obj, "ok", &ok)) {
        json_object_put(obj);
        tb_sfree(*info);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    if (!json_object_get_boolean(ok)) {
        json_object_put(obj);
        tb_sfree(*info);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *result;
    if (!json_object_object_get_ex(obj, "result", &result)){
        json_object_put(obj);
        tb_sfree(*info);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    ret = telebot_parser_get_webhook_info(result, *info);
    json_object_put(obj);

    if (ret != TELEBOT_ERROR_NONE) {
        tb_sfree(*info);
        return ret;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_free_webhook_info(telebot_webhook_info_t *info)
{
    if (info == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    tb_sfree(info->url);
    tb_sfree(info->last_error_message);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_send_message(telebot_handler_t handle, long long int chat_id,
        char *text, char *parse_mode, bool disable_web_page_preview,
        bool disable_notification, int reply_to_message_id,
        const char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (text == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_message(_handle->core_h, chat_id,
            text, parse_mode, disable_web_page_preview, disable_notification,
            reply_to_message_id, reply_markup);

    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    return ret;
}

telebot_error_e telebot_forward_message(telebot_handler_t handle, long long int chat_id,
        long long int from_chat_id, bool disable_notification, int message_id)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (message_id <= 0)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_forward_message(_handle->core_h, chat_id,
            from_chat_id, disable_notification, message_id);

    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    return ret;
}

telebot_error_e telebot_send_photo(telebot_handler_t handle, long long int chat_id,
        char *photo, bool is_file, char *caption, bool disable_notification,
        int reply_to_message_id, char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (photo == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_photo(_handle->core_h, chat_id, photo,
            is_file, caption, disable_notification, reply_to_message_id, reply_markup);

    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    return ret;
}

telebot_error_e telebot_send_audio(telebot_handler_t handle, long long int chat_id,
        char *audio, bool is_file, int duration, char *performer, char *title,
        bool disable_notification, int reply_to_message_id, char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (audio == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_audio(_handle->core_h, chat_id, audio,
            is_file, duration, performer, title, disable_notification,
            reply_to_message_id, reply_markup);

    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    return ret;
}

telebot_error_e telebot_send_document(telebot_handler_t handle, long long int chat_id,
        char *document, bool is_file, bool disable_notification,
        int reply_to_message_id, char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (document == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_document(_handle->core_h, chat_id,
            document, is_file, disable_notification, reply_to_message_id, reply_markup);

    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    return ret;
}

telebot_error_e telebot_send_video(telebot_handler_t handle, long long int chat_id,
        char *video, bool is_file, int duration, int width, int height,
        char *caption, bool disable_notification, int reply_to_message_id,
        char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (video == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_video(_handle->core_h, chat_id, video,
            is_file, duration, caption, disable_notification, reply_to_message_id,
            reply_markup);

    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    return ret;
}

telebot_error_e telebot_send_animation(telebot_handler_t handle, long long int chat_id,
        char *video, bool is_file, int duration, int width, int height,
        char *caption, bool disable_notification, int reply_to_message_id,
        char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (video == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_animation(_handle->core_h, chat_id, video,
            is_file, duration, caption, disable_notification, reply_to_message_id,
            reply_markup);

    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    return ret;
}

telebot_error_e telebot_send_voice(telebot_handler_t handle, long long int chat_id,
        char *voice, bool is_file, char *caption, int duration,
        bool disable_notification, int reply_to_message_id, char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (voice == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_voice(_handle->core_h, chat_id, voice,
            is_file, duration, disable_notification, reply_to_message_id, reply_markup);

    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    return ret;
}

telebot_error_e telebot_send_video_note(telebot_handler_t handle, long long int chat_id,
        char *video_note, bool is_file, int duration, int length,
        bool disable_notification, int reply_to_message_id, char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (video_note == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_video_note(_handle->core_h, chat_id,
            video_note, is_file, duration, length, disable_notification,
            reply_to_message_id, reply_markup);

    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    return ret;
}

telebot_error_e telebot_send_location(telebot_handler_t handle, long long int chat_id,
        float latitude, float longitude, bool disable_notification,
        int reply_to_message_id, char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_error_e ret = telebot_core_send_location(_handle->core_h, chat_id,
            latitude, longitude, disable_notification, reply_to_message_id, reply_markup);

    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    return ret;
}

telebot_error_e telebot_send_contact(telebot_handler_t handle, long long int chat_id,
        char *phone_number, char *first_name, char *last_name,
        bool disable_notification, int reply_to_message_id, char *reply_markup)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((phone_number == NULL) || (first_name == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_send_contact(_handle->core_h, chat_id,
            phone_number, first_name, last_name, disable_notification,
            reply_to_message_id, reply_markup);

    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    return ret;
}

telebot_error_e telebot_send_chat_action(telebot_handler_t handle, long long int chat_id,
        char *action)
{
    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_error_e ret = telebot_core_send_chat_action(_handle->core_h,
            chat_id, action);

    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    return ret;
}

telebot_error_e telebot_get_user_profile_photos(telebot_handler_t handle,
        int user_id, int offset, int limit, telebot_user_profile_photos_t **photos)
{
    if (photos == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_hdata_t * _handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((limit <= 0) || (limit > TELEBOT_USER_PROFILE_PHOTOS_LIMIT))
        limit = TELEBOT_USER_PROFILE_PHOTOS_LIMIT;

    *photos = malloc(sizeof(telebot_user_profile_photos_t));
    if (*photos == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    telebot_error_e ret = telebot_core_get_user_profile_photos(_handle->core_h,
            user_id, offset, limit);
    if (ret != TELEBOT_ERROR_NONE) {
        tb_sfree(*photos);
        return ret;
    }

    struct json_object *obj = telebot_parser_str_to_obj(_handle->core_h->resp_data);
    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    if (obj == NULL) {
        tb_sfree(*photos);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *ok;
    if (!json_object_object_get_ex(obj, "ok", &ok)) {
        json_object_put(obj);
        tb_sfree(*photos);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    if (!json_object_get_boolean(ok)) {
        json_object_put(obj);
        tb_sfree(*photos);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *result;
    if (!json_object_object_get_ex(obj, "result", &result)) {
        json_object_put(obj);
        tb_sfree(*photos);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    ret = telebot_parser_get_user_profile_photos(result, *photos);
    json_object_put(obj);

    if (ret != TELEBOT_ERROR_NONE) {
        tb_sfree(*photos);
        return ret;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_free_user_profile_photos(telebot_user_profile_photos_t *photos)
{
    if (photos == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    int total = photos->current_count;
    int subtotal = 4, i, j;
    for (i=0;i<total;i++)
        for(j=0;j<subtotal;j++)
            telebot_free_photo(&(photos->photos[j][i]));
    photos->current_count = 0;
    photos->total_count = 0;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_download_file(telebot_handler_t handle, char *file_id,
        char *path)
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
        tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    char *file_path;
    ret = telebot_parser_get_file_path(obj, &file_path);
    json_object_put(obj);
    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    if (file_path == NULL)
        return TELEBOT_ERROR_OPERATION_FAILED;

    ret = telebot_core_download_file(_handle->core_h, file_path, path);
    free(file_path);
    tb_sfree_zcnt(_handle->core_h->resp_data, _handle->core_h->resp_size);

    return ret;
}

#if 0
telebot_error_e telebot_delete_message(long long int chat_id, int message_id)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((chat_id <= 0) || (message_id <= 0))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_error_e ret = telebot_core_delete_message(_handle->core_h, chat_id, message_id);
    tb_sfree(_handle->core_h->resp_data, &(_handle->core_h->resp_size));

    return ret;
}

telebot_error_e telebot_send_sticker(long long int chat_id, char *sticker,
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

#endif

/* Utility functions for releasing memory */
static void telebot_free_user(telebot_user_t *user)
{
    if (user == NULL) return;
    tb_sfree(user->first_name);
    tb_sfree(user->last_name);
    tb_sfree(user->username);
    tb_sfree(user->language_code);
}

static void telebot_free_chat(telebot_chat_t *chat)
{
    if (chat == NULL) return;
    tb_sfree(chat->type);
    tb_sfree(chat->title);
    tb_sfree(chat->username);
    tb_sfree(chat->first_name);
    tb_sfree(chat->last_name);

    telebot_free_chat_photo(chat->photo);
    tb_sfree(chat->photo);

    tb_sfree(chat->description);
    tb_sfree(chat->invite_link);
    tb_sfree(chat->sticker_set_name);

    telebot_free_message(chat->pinned_message);
    tb_sfree(chat->pinned_message);
}

static void telebot_free_chat_photo(telebot_chat_photo_t *photo)
{
    if (photo == NULL) return;
    tb_sfree(photo->small_file_id);
    tb_sfree(photo->big_file_id);
}

static void telebot_free_message(telebot_message_t *msg)
{
    int index;
    if (msg == NULL) return;

    telebot_free_user(msg->from);
    tb_sfree(msg->from);

    telebot_free_chat(msg->chat);
    tb_sfree(msg->chat);

    telebot_free_user(msg->forward_from);
    tb_sfree(msg->forward_from);

    telebot_free_message(msg->reply_to_message);
    tb_sfree(msg->reply_to_message);

    tb_sfree(msg->media_group_id);
    tb_sfree(msg->author_signature);
    tb_sfree(msg->text);

    if (msg->entities) {
        for (index=0;index<msg->count_entities;index++)
            telebot_free_telebot_message_entity(&(msg->entities[index]));
        tb_sfree(msg->entities);
        msg->count_entities = 0;
    }

    if (msg->caption_entities) {
        for (index=0;index<msg->count_caption_entities;index++)
            telebot_free_telebot_message_entity(&(msg->caption_entities[index]));
        tb_sfree(msg->caption_entities);
        msg->count_caption_entities = 0;
    }

    telebot_free_audio(msg->audio);
    tb_sfree(msg->audio);

    telebot_free_document(msg->document);
    tb_sfree(msg->document);

    if (msg->photos) {
        for (index=0;index<msg->count_photos;index++)
            telebot_free_photo(&(msg->photos[index]));
        tb_sfree(msg->photos);
        msg->count_photos = 0;
    }

    telebot_free_sticker(msg->sticker);
    tb_sfree(msg->sticker);

    telebot_free_video(msg->video);
    tb_sfree(msg->video);

    telebot_free_animation(msg->animation);
    tb_sfree(msg->animation);

    telebot_free_voice(msg->voice);
    tb_sfree(msg->voice);

    telebot_free_video_note(msg->video_note);
    tb_sfree(msg->video_note);

    tb_sfree(msg->caption);

    telebot_free_contact(msg->contact);
    tb_sfree(msg->contact);

    telebot_free_location(msg->location);
    tb_sfree(msg->location);

    telebot_free_venue(msg->venue);
    tb_sfree(msg->venue);

    if (msg->new_chat_members) {
        for (index=0;index<msg->count_new_chat_members;index++)
            telebot_free_user(&(msg->new_chat_members[index]));
        tb_sfree(msg->new_chat_members);
        msg->count_new_chat_members = 0;
    }

    if (msg->left_chat_members) {
        for (index=0;index<msg->count_left_chat_members;index++)
            telebot_free_user(&(msg->left_chat_members[index]));
        tb_sfree(msg->left_chat_members);
        msg->count_left_chat_members = 0;
    }

    tb_sfree(msg->new_chat_title);

    if (msg->new_chat_photos) {
        for (index=0;index<msg->count_new_chat_photos;index++)
            telebot_free_photo(&(msg->new_chat_photos[index]));
        tb_sfree(msg->new_chat_photos);
        msg->count_new_chat_photos = 0;
    }

    telebot_free_message(msg->pinned_message);

    //TODO: free msg->game;
    //TODO: free msg->invoice;
    //TODO: free msg->successful_payment;
}

static void telebot_free_telebot_message_entity(telebot_message_entity_t *entity)
{
    if (entity == NULL) return;

    tb_sfree(entity->type);
    tb_sfree(entity->url);
    telebot_free_user(entity->user);
    tb_sfree(entity->user);
}

static void telebot_free_photo(telebot_photo_t *photo)
{
    if (photo == NULL) return;

    tb_sfree(photo->file_id);
}

static void telebot_free_audio(telebot_audio_t *audio)
{
    if (audio == NULL) return;

    tb_sfree(audio->file_id);
    tb_sfree(audio->performer);
    tb_sfree(audio->title);
    tb_sfree(audio->mime_type);
}

static void telebot_free_document(telebot_document_t *document)
{
    if (document == NULL) return;

    tb_sfree(document->file_id);
    telebot_free_photo(document->thumb);
    tb_sfree(document->thumb);
    tb_sfree(document->file_name);
    tb_sfree(document->mime_type);
}

static void telebot_free_video(telebot_video_t *video)
{
    if (video == NULL) return;

    tb_sfree(video->file_id);
    telebot_free_photo(video->thumb);
    tb_sfree(video->thumb);
    tb_sfree(video->mime_type);
}

static void telebot_free_animation(telebot_animation_t *animation)
{
    if (animation == NULL) return;

    tb_sfree(animation->file_id);
    telebot_free_photo(animation->thumb);
    tb_sfree(animation->thumb);
    tb_sfree(animation->mime_type);
}

static void telebot_free_voice(telebot_voice_t *voice)
{
    if (voice == NULL) return;

    tb_sfree(voice->file_id);
    tb_sfree(voice->mime_type);
}

static void telebot_free_video_note(telebot_video_note_t *vnote)
{
    if (vnote == NULL) return;

    tb_sfree(vnote->file_id);
    telebot_free_photo(vnote->thumb);
    tb_sfree(vnote->thumb);
}

static void telebot_free_contact(telebot_contact_t *contact)
{
    if (contact == NULL) return;

    tb_sfree(contact->phone_number);
    tb_sfree(contact->first_name);
    tb_sfree(contact->last_name);
}

static void telebot_free_location(telebot_location_t *location)
{
    //Nothing to free
    return;
}

static void telebot_free_venue(telebot_venue_t *venue)
{
    if (venue == NULL) return;

    telebot_free_location(venue->location);
    tb_sfree(venue->location);
    tb_sfree(venue->title);
    tb_sfree(venue->address);
    tb_sfree(venue->foursquare_id);
}

static void telebot_free_callback_query(telebot_callback_query_t *query)
{
    if (query == NULL) return;

    tb_sfree(query->id);
    telebot_free_user(query->from);
    tb_sfree(query->from);
    telebot_free_message(query->message);
    tb_sfree(query->message);
    tb_sfree(query->inline_message_id);
    tb_sfree(query->chat_instance);
    tb_sfree(query->data);
    tb_sfree(query->game_short_name);
}

static void telebot_free_mask_position(telebot_mask_position_t *mask)
{
    if (mask == NULL) return;

    tb_sfree(mask->point);
}

static void telebot_free_sticker(telebot_sticker_t *sticker)
{
    if (sticker == NULL) return;

    tb_sfree(sticker->file_id);
    telebot_free_photo(sticker->thumb);
    tb_sfree(sticker->thumb);
    tb_sfree(sticker->emoji);
    tb_sfree(sticker->set_name);
    telebot_free_mask_position(sticker->mask_position);
}

//TODO: static void telebot_free_invoice(telebot_invoice_t *invoice);
//TODO: static void telebot_free_payment(telebot_successful_payment_t *payment);
//TODO: static void telebot_free_game(telebot_game_t *game);
