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
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <json.h>
#include <json_object.h>
#include <telebot.h>
#include <telebot-core.h>
#include <telebot-private.h>
#include <telebot-parser.h>

typedef struct telebot_handler_s
{
    telebot_core_handler_t *core_h;
    int offset;
} telebot_hdata_t;

static const char *telebot_update_type_str[TELEBOT_UPDATE_TYPE_MAX] = {
    "message", "edited_message", "channel_post",
    "edited_channel_post", "inline_query",
    "chonse_inline_result", "callback_query",
    "shipping_query", "pre_checkout_query",
    "poll", "poll_answer"
};

static void telebot_put_user(telebot_user_t *user);
static void telebot_put_chat_photo(telebot_chat_photo_t *photo);
static void telebot_put_chat_permissions(telebot_chat_permissions_t *permissions);
static void telebot_put_message(telebot_message_t *msg);
static void telebot_put_telebot_message_entity(telebot_message_entity_t *entity);
static void telebot_put_audio(telebot_audio_t *audio);
static void telebot_put_document(telebot_document_t *document);
static void telebot_put_photo(telebot_photo_t *photo);
static void telebot_put_video(telebot_video_t *video);
static void telebot_put_animation(telebot_animation_t *animation);
static void telebot_put_voice(telebot_voice_t *voice);
static void telebot_put_video_note(telebot_video_note_t *vnote);
static void telebot_put_poll(telebot_poll_t *poll);
static void telebot_put_poll_answer(telebot_poll_answer_t *answer);
static void telebot_put_dice(telebot_dice_t *dice);
static void telebot_put_contact(telebot_contact_t *contact);
static void telebot_put_location(telebot_location_t *location);
static void telebot_put_venue(telebot_venue_t *venue);
static void telebot_put_file(telebot_file_t *file);
static void telebot_put_callback_query(telebot_callback_query_t *query);
//static void telebot_put_game(telebot_document_t *game);
//static void telebot_put_invoice(telebot_invoice_t *invoice);
//static void telebot_put_payment(telebot_successful_payment_t *payment);

telebot_error_e telebot_create(telebot_handler_t *handle, char *token)
{
    if ((token == NULL) || (handle == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_hdata_t *_handle = (telebot_hdata_t *)malloc(sizeof(telebot_hdata_t));
    if (_handle == NULL)
    {
        ERR("Failed to allocate memory");
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    telebot_error_e ret = telebot_core_create(&(_handle->core_h), token);
    if (ret != TELEBOT_ERROR_NONE)
    {
        TELEBOT_SAFE_FREE(_handle);
        return ret;
    }

    _handle->offset = 0;

    *handle = _handle;
    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_destroy(telebot_handler_t handle)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_destroy(&(_handle->core_h));
    TELEBOT_SAFE_FREE(_handle);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_set_proxy(telebot_handler_t handle, char *addr, char *auth)
{
    if (addr == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
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

    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    return telebot_core_get_proxy(_handle->core_h, addr);
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

    const char *str_allowed_updates = NULL;
    struct json_object *array = NULL;
    if (allowed_updates_count > 0)
    {
        array = json_object_new_array();
        for (int i = 0; i < allowed_updates_count; i++) {
            const char *item = telebot_update_type_str[allowed_updates[i]];
            json_object_array_add(array, json_object_new_string(item));
        }
        str_allowed_updates = json_object_to_json_string(array);
        DBG("Allowed updates: %s", str_allowed_updates);
    }

    int _offset = offset != 0 ? offset : _handle->offset;
    int _timeout = timeout > 0 ? timeout : 0;
    int _limit = TELEBOT_UPDATE_COUNT_MAX_LIMIT;
    if ((limit > 0) && (limit < TELEBOT_UPDATE_COUNT_MAX_LIMIT))
        _limit = limit;

    telebot_core_response_t response;
    int ret = telebot_core_get_updates(_handle->core_h, _offset, _limit, _timeout,
        str_allowed_updates, &response);
    if (ret != TELEBOT_ERROR_NONE) {
        if (array) json_object_put(array);
        return ret;
    }
    if (array) json_object_put(array);

    struct json_object *obj = telebot_parser_str_to_obj(response.data);
    if (obj == NULL)
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *ok = NULL;
    if (!json_object_object_get_ex(obj, "ok", &ok) || !json_object_get_boolean(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *result = NULL;
    if (!json_object_object_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_updates(result, updates, count);
    if (ret == TELEBOT_ERROR_NONE)
    {
        telebot_update_t *ups = *updates;
        for (int index = 0; index < *count; index++)
        {
            if (ups[index].update_id >= _handle->offset)
                _handle->offset = ups[index].update_id + 1;
        }
    }

finish:
    if (obj) json_object_put(obj);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_put_updates(telebot_update_t *updates, int count)
{
    if ((updates == NULL) || (count <= 0))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    for (int index = 0; index < count; index++)
    {
        switch (updates[index].update_type)
        {
        case TELEBOT_UPDATE_TYPE_MESSAGE:
            telebot_put_message(&(updates[index].message));
            break;
        case TELEBOT_UPDATE_TYPE_EDITED_MESSAGE:
            telebot_put_message(&(updates[index].edited_message));
            break;
        case TELEBOT_UPDATE_TYPE_CHANNEL_POST:
            telebot_put_message(&(updates[index].channel_post));
            break;
        case TELEBOT_UPDATE_TYPE_EDITED_CHANNEL_POST:
            telebot_put_message(&(updates[index].edited_channel_post));
            break;
        case TELEBOT_UPDATE_TYPE_CALLBACK_QUERY:
            telebot_put_callback_query(&(updates[index].callback_query));
            break;
        case TELEBOT_UPDATE_TYPE_POLL:
            telebot_put_poll(&(updates[index].poll));
            break;
        case TELEBOT_UPDATE_TYPE_POLL_ANSWER:
            telebot_put_poll_answer(&(updates[index].poll_anser));
        default:
            ERR("Unsupported update type: %d", updates[index].update_type);
        }
    }

    TELEBOT_SAFE_FREE(updates);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_get_me(telebot_handler_t handle, telebot_user_t *me)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (me == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_get_me(_handle->core_h, &response);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    struct json_object *obj = telebot_parser_str_to_obj(response.data);
    if (obj == NULL)
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *ok = NULL;
    if (!json_object_object_get_ex(obj, "ok", &ok) || !json_object_get_boolean(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *result = NULL;
    if (!json_object_object_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_user(result, me);

finish:
    if (ret) telebot_put_me(me);
    if (obj) json_object_put(obj);
    telebot_core_put_response(&response);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_put_me(telebot_user_t *me)
{
    if (me == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_put_user(me);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_set_webhook(telebot_handler_t handle, char *url,
    char *certificate, int max_connections,telebot_update_type_e allowed_updates[],
    int allowed_updates_count)
{
    int i = 0;
    char allowed_updates_str[TELEBOT_BUFFER_PAGE] = {
        0,
    };
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (url == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (allowed_updates_count > 0)
    {
        strncat(allowed_updates_str, "[", TELEBOT_BUFFER_BLOCK);
        for (i = 0; i < allowed_updates_count; i++)
        {
            strncat(allowed_updates_str, telebot_update_type_str[allowed_updates[i]],
                    TELEBOT_BUFFER_BLOCK);
            if (i < (allowed_updates_count - 1)) //intermediate element
                strncat(allowed_updates_str, ",", TELEBOT_BUFFER_BLOCK);
        }
        strncat(allowed_updates_str, "]", TELEBOT_BUFFER_BLOCK);
    }

    telebot_core_response_t response;
    int ret = telebot_core_set_webhook(_handle->core_h, url, certificate,
        max_connections, allowed_updates_str, &response);

    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_delete_webhook(telebot_handler_t handle)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_delete_webhook(_handle->core_h, &response);

    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_get_webhook_info(telebot_handler_t handle, telebot_webhook_info_t *info)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (info == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_get_webhook_info(_handle->core_h, &response);
    if (ret != TELEBOT_ERROR_NONE)
    {
        return ret;
    }

    struct json_object *obj = telebot_parser_str_to_obj(response.data);
    if (obj == NULL)
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *ok = NULL;
    if (!json_object_object_get_ex(obj, "ok", &ok) || !json_object_get_boolean(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *result = NULL;
    if (!json_object_object_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_webhook_info(result, info);

finish:
    if (ret) telebot_put_webhook_info(info);
    if (obj) json_object_put(obj);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_put_webhook_info(telebot_webhook_info_t *info)
{
    if (info == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    TELEBOT_SAFE_FREE(info->url);
    TELEBOT_SAFE_FREE(info->last_error_message);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_send_message(telebot_handler_t handle, long long int chat_id,
    const char *text, const char *parse_mode, bool disable_web_page_preview,
    bool disable_notification, int reply_to_message_id, const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (text == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_send_message(_handle->core_h, chat_id, text, parse_mode,
            disable_web_page_preview, disable_notification, reply_to_message_id,
            reply_markup, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_forward_message(telebot_handler_t handle, long long int chat_id,
    long long int from_chat_id, bool disable_notification, int message_id)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (message_id <= 0)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_forward_message(_handle->core_h, chat_id, from_chat_id,
        disable_notification, message_id, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_send_photo(telebot_handler_t handle, long long int chat_id,
    const char *photo, bool is_file, const char *caption, const char *parse_mode,
    bool disable_notification, int reply_to_message_id, const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (photo == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_send_photo(_handle->core_h, chat_id, photo, is_file, caption,
        parse_mode, disable_notification, reply_to_message_id, reply_markup, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_send_audio(telebot_handler_t handle, long long int chat_id,
    const char *audio, bool is_file, const char *caption, const char *parse_mode,
    int duration, const char *performer, const char *title, const char *thumb,
    bool disable_notification, int reply_to_message_id, const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (audio == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_send_audio(_handle->core_h, chat_id, audio, is_file, caption,
        parse_mode, duration, performer, title, thumb, disable_notification,
        reply_to_message_id, reply_markup, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_send_document(telebot_handler_t handle, long long int chat_id,
    const char *document, bool is_file, const char *thumb, const char *caption,
    const char *parse_mode, bool disable_notification, int reply_to_message_id,
    const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (document == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_send_document(_handle->core_h, chat_id, document, is_file,
        thumb, caption, parse_mode, disable_notification, reply_to_message_id,
        reply_markup, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_send_video(telebot_handler_t handle, long long int chat_id,
    const char *video, bool is_file, int duration, int width, int height,
    const char *thumb, const char *caption, const char *parse_mode,
    bool supports_streaming, bool disable_notification, int reply_to_message_id,
    const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (video == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_send_video(_handle->core_h, chat_id, video, is_file, duration,
        width, height, thumb, caption, parse_mode, supports_streaming, disable_notification,
        reply_to_message_id, reply_markup, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_send_animation(telebot_handler_t handle, long long int chat_id,
    const char *animation, bool is_file, int duration, int width, int height,
    const char *thumb, const char *caption, const char *parse_mode,
    bool disable_notification, int reply_to_message_id, const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (animation == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_send_animation(_handle->core_h, chat_id, animation, is_file,
         duration, width, height, thumb, caption, parse_mode, disable_notification,
         reply_to_message_id, reply_markup, &response);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;
    telebot_core_put_response(&response);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_send_voice(telebot_handler_t handle, long long int chat_id,
    const char *voice, bool is_file, const char *caption, const char *parse_mode,
    int duration, bool disable_notification, int reply_to_message_id,
    const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (voice == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_send_voice(_handle->core_h, chat_id, voice, is_file,
        caption, parse_mode, duration, disable_notification, reply_to_message_id,
        reply_markup, &response);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;
    telebot_core_put_response(&response);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_send_video_note(telebot_handler_t handle, long long int chat_id,
        char *video_note, bool is_file, int duration, int length, const char *thumb,
        bool disable_notification, int reply_to_message_id, const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (video_note == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_send_video_note(_handle->core_h, chat_id, video_note,
        is_file, duration, length, thumb, disable_notification, reply_to_message_id,
        reply_markup, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_send_location(telebot_handler_t handle, long long int chat_id,
    float latitude, float longitude, int live_period, bool disable_notification,
    int reply_to_message_id, const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_send_location(_handle->core_h, chat_id, latitude, longitude,
        live_period, disable_notification, reply_to_message_id, reply_markup, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_edit_message_live_location(telebot_handler_t handle,
    long long int chat_id, int message_id, const char *inline_message_id,
    float latitude, float longitude, const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_edit_message_live_location(_handle->core_h, chat_id,
        message_id, inline_message_id, latitude, longitude, reply_markup, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_stop_message_live_location(telebot_handler_t handle,
    long long int chat_id, int message_id, char *inline_message_id,
    const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_stop_message_live_location(_handle->core_h, chat_id,
        message_id, inline_message_id, reply_markup, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_send_venue(telebot_handler_t handle, long long int chat_id,
        float latitude, float longitude, const char *title, const char *address,
        const char *foursquare_id, const char *foursquare_type,bool disable_notification,
        int reply_to_message_id, const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((title == NULL) || (address == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_send_venue(_handle->core_h, chat_id, latitude, longitude,
        title, address, foursquare_id, foursquare_type, disable_notification,
        reply_to_message_id, reply_markup, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_send_contact(telebot_handler_t handle, long long int chat_id,
    const char *phone_number, const char *first_name, const char *last_name,
    const char *vcard, bool disable_notification, int reply_to_message_id,
    const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((phone_number == NULL) || (first_name == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_send_contact(_handle->core_h, chat_id, phone_number,
        first_name, last_name, vcard, disable_notification, reply_to_message_id,
        reply_markup, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_send_poll(telebot_handler_t handle, long long int chat_id,
    const char *question, const char **options, int count_options, bool is_anonymous,
    const char *type, bool allows_multiple_answers, int correct_option_id, bool is_closed,
    bool disable_notification, int reply_to_message_id, const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((question == NULL) || (options == NULL) || (count_options <= 0))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *array = json_object_new_array();
    for (int i = 0; i < count_options; i++)
        json_object_array_add(array, json_object_new_string(options[i]));

    const char *array_options = json_object_to_json_string(array);
    DBG("Poll options: %s", array_options);

    telebot_core_response_t response;
    int ret = telebot_core_send_poll(_handle->core_h, chat_id, question, array_options,
        is_anonymous, type, allows_multiple_answers, correct_option_id, is_closed,
        disable_notification, reply_to_message_id, reply_markup, &response);
    telebot_core_put_response(&response);
    json_object_put(array);

    return ret;
}

telebot_error_e telebot_send_dice(telebot_handler_t handle, long long int chat_id,
    bool disable_notification, int reply_to_message_id, const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_send_dice(_handle->core_h, chat_id, disable_notification,
        reply_to_message_id, reply_markup, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_send_chat_action(telebot_handler_t handle, long long int chat_id,
    char *action)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_send_chat_action(_handle->core_h, chat_id, action, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_get_user_profile_photos(telebot_handler_t handle, int user_id,
    int offset, int limit, telebot_user_profile_photos_t *photos)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (photos == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if ((limit <= 0) || (limit > TELEBOT_USER_PROFILE_PHOTOS_LIMIT))
        limit = TELEBOT_USER_PROFILE_PHOTOS_LIMIT;

    telebot_core_response_t response;
    int ret = telebot_core_get_user_profile_photos(_handle->core_h, user_id, offset,
        limit, &response);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    struct json_object *obj = telebot_parser_str_to_obj(response.data);
    if (obj == NULL)
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *ok = NULL;
    if (!json_object_object_get_ex(obj, "ok", &ok) || !json_object_get_boolean(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *result = NULL;
    if (!json_object_object_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_user_profile_photos(result, photos);

finish:
    if (ret) telebot_put_user_profile_photos(photos);
    if (obj) json_object_put(obj);
    telebot_core_put_response(&response);
    return ret;
}

telebot_error_e telebot_put_user_profile_photos(telebot_user_profile_photos_t *photos)
{
    if (photos == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    int total = photos->current_count;
    int subtotal = 4;
    for (int i = 0; i < total; i++)
        for (int j = 0; j < subtotal; j++)
            telebot_put_photo(&(photos->photos[j][i]));
    photos->current_count = 0;
    photos->total_count = 0;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_download_file(telebot_handler_t handle, const char *file_id, const char *path)
{
    telebot_file_t file;
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (file_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_get_file(_handle->core_h, file_id, &response);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    struct json_object *obj = telebot_parser_str_to_obj(response.data);
    if (obj == NULL)
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *ok = NULL;
    if (!json_object_object_get_ex(obj, "ok", &ok) || !json_object_get_boolean(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *result = NULL;
    if (!json_object_object_get_ex(obj, "result", &result)) {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_file(result, &file);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    if (file.file_path == NULL) {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_core_download_file(_handle->core_h, file.file_path, path);

finish:
    telebot_put_file(&file);
    if (obj) json_object_put(obj);
    telebot_core_put_response(&response);
    return ret;
}

telebot_error_e telebot_kick_chat_member(telebot_handler_t handle, long long int chat_id,
    int user_id, long until_date)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_kick_chat_member(_handle->core_h, chat_id, user_id,
        until_date, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_unban_chat_member(telebot_handler_t handle, long long int chat_id,
     int user_id)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_unban_chat_member(_handle->core_h, chat_id, user_id, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_restrict_chat_member(telebot_handler_t handle,
    long long int chat_id, int user_id, long until_date, bool can_send_messages,
    bool can_send_media_messages, bool can_send_polls, bool can_send_other_messages,
    bool can_add_web_page_previews, bool can_change_info, bool can_invite_users,
    bool can_pin_messages)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_restrict_chat_member(_handle->core_h, chat_id, user_id,
        until_date, can_send_messages, can_send_media_messages, can_send_polls,
        can_send_other_messages, can_add_web_page_previews, can_change_info,
        can_invite_users, can_pin_messages, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_promote_chat_member(telebot_handler_t handle,
    long long int chat_id, int user_id, bool can_change_info, bool can_post_messages,
    bool can_edit_messages, bool can_delete_messages, bool can_invite_users,
    bool can_restrict_members, bool can_pin_messages, bool can_promote_members)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_promote_chat_member(_handle->core_h, chat_id, user_id,
        can_change_info, can_post_messages, can_edit_messages, can_delete_messages,
        can_invite_users, can_restrict_members, can_pin_messages, can_promote_members,
        &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_set_chat_admin_custom_title(telebot_handler_t handle,
    long long int chat_id, int user_id, const char *custom_title)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (custom_title == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_set_chat_admin_custom_title(_handle->core_h, chat_id,
        user_id, custom_title, &response);
    telebot_core_put_response(&response);

    return ret;
}


telebot_error_e telebot_set_chat_permissions(telebot_handler_t handle,
    long long int chat_id, bool can_send_messages, bool can_send_media_messages,
    bool can_send_polls, bool can_send_other_messages, bool can_add_web_page_previews,
    bool can_change_info, bool can_invite_users, bool can_pin_messages)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_set_chat_permissions(_handle->core_h, chat_id,
        can_send_messages, can_send_media_messages, can_send_polls,
        can_send_other_messages, can_add_web_page_previews, can_change_info,
        can_invite_users, can_pin_messages, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_export_chat_invite_link(telebot_handler_t handle,
    long long int chat_id, char **invite_link)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (invite_link == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    *invite_link = NULL;

    telebot_core_response_t response;
    int ret = telebot_core_export_chat_invite_link(_handle->core_h, chat_id, &response);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    struct json_object *obj = telebot_parser_str_to_obj(response.data);
    if (obj == NULL) {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *ok = NULL;
    if (!json_object_object_get_ex(obj, "ok", &ok) || !json_object_get_boolean(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *result = NULL;
    if (json_object_object_get_ex(obj, "result", &result))
    {
        *invite_link = TELEBOT_SAFE_STRDUP(json_object_get_string(result));
        if (*invite_link == NULL)
            ret = TELEBOT_ERROR_OUT_OF_MEMORY;
        else
            ret = TELEBOT_ERROR_NONE;
    }
    else
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
    }

finish:
    if (obj) json_object_put(obj);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_set_chat_photo(telebot_handler_t handle, long long int chat_id,
    const char *photo)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_set_chat_photo(_handle->core_h, chat_id, photo, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_delete_chat_photo(telebot_handler_t handle, long long int chat_id)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_delete_chat_photo(_handle->core_h, chat_id, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_set_chat_title(telebot_handler_t handle, long long int chat_id,
    const char *title)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (title == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_set_chat_title(_handle->core_h, chat_id, title, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_set_chat_description(telebot_handler_t handle,
    long long int chat_id, const char *description)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (description == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_set_chat_description(_handle->core_h, chat_id, description,
        &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_pin_chat_message(telebot_handler_t handle, long long int chat_id,
    int message_id, bool disable_notification)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_pin_chat_message(_handle->core_h, chat_id, message_id,
        disable_notification, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_unpin_chat_message(telebot_handler_t handle, long long int chat_id)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_unpin_chat_message(_handle->core_h, chat_id, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_leave_chat(telebot_handler_t handle, long long int chat_id)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_leave_chat(_handle->core_h, chat_id, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_get_chat(telebot_handler_t handle, long long int chat_id,
    telebot_chat_t *chat)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (chat == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_get_chat(_handle->core_h, chat_id, &response);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    struct json_object *obj = telebot_parser_str_to_obj(response.data);
    if (obj == NULL)
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *ok = NULL;
    if (!json_object_object_get_ex(obj, "ok", &ok) || !json_object_get_boolean(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *result = NULL;
    if (!json_object_object_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_chat(result, chat);

finish:
    if (ret) telebot_put_chat(chat);
    if (obj) json_object_put(obj);
    telebot_core_put_response(&response);
    return ret;
}

telebot_error_e telebot_get_chat_admins(telebot_handler_t handle, long long int chat_id,
    telebot_chat_member_t **admins, int *count)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((admins == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    *admins = NULL;
    *count = 0;

    telebot_core_response_t response;
    int ret = telebot_core_get_chat_admins(_handle->core_h, chat_id, &response);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    struct json_object *obj = telebot_parser_str_to_obj(response.data);
    if (obj == NULL)
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *ok = NULL;
    if (!json_object_object_get_ex(obj, "ok", &ok) || !json_object_get_boolean(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *result = NULL;
    if (!json_object_object_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_chat_admins(result, admins, count);

finish:
    if (ret) telebot_put_chat_admins(*admins, *count);
    if (obj) json_object_put(obj);
    telebot_core_put_response(&response);
    return ret;
}

telebot_error_e telebot_put_chat_admins(telebot_chat_member_t *admins, int count)
{
    if (admins == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    for (int index = 0; index < count; index++)
    {
        telebot_put_chat_member(&admins[index]);
    }
    TELEBOT_SAFE_FREE(admins);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_get_chat_members_count(telebot_handler_t handle,
    long long int chat_id, int *count)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (count == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    *count = 0;

    telebot_core_response_t response;
    int ret = telebot_core_get_chat_members_count(_handle->core_h, chat_id, &response);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    struct json_object *obj = telebot_parser_str_to_obj(response.data);
    if (obj == NULL) {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *ok = NULL;
    if (!json_object_object_get_ex(obj, "ok", &ok) || !json_object_get_boolean(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *result = NULL;
    if (json_object_object_get_ex(obj, "result", &result))
        *count = json_object_get_int(result);
    else
        ret = TELEBOT_ERROR_OPERATION_FAILED;

finish:
    if (obj) json_object_put(obj);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_get_chat_member(telebot_handler_t handle, long long int chat_id,
    int user_id, telebot_chat_member_t *member)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (member == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_get_chat_member(_handle->core_h, chat_id, user_id, &response);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    struct json_object *obj = telebot_parser_str_to_obj(response.data);
    if (obj == NULL)
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *ok = NULL;
    if (!json_object_object_get_ex(obj, "ok", &ok) || !json_object_get_boolean(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *result = NULL;
    if (!json_object_object_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_chat_member(result, member);

finish:
    if (ret) telebot_put_chat_member(member);
    if (obj) json_object_put(obj);
    telebot_core_put_response(&response);
    return ret;
}

telebot_error_e telebot_put_chat_member(telebot_chat_member_t *member)
{
    if (member == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_put_user(member->user);
    TELEBOT_SAFE_FREE(member->user);
    TELEBOT_SAFE_FREE(member->status);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_set_chat_sticker_set(telebot_handler_t handle,
        long long int chat_id, const char *sticker_set_name)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (sticker_set_name == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_set_chat_sticker_set(_handle->core_h, chat_id,
        sticker_set_name, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_delete_chat_sticker_set(telebot_handler_t handle,
    long long int chat_id)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_delete_chat_sticker_set(_handle->core_h, chat_id, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_answer_callback_query(telebot_handler_t handle,
    const char *callback_query_id, const char *text, bool show_alert,
    const char *url, int cache_time)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (callback_query_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_answer_callback_query(_handle->core_h, callback_query_id,
        text, show_alert, url, cache_time, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_set_my_commands(telebot_handler_t handle,
    telebot_bot_command_t commands[], int count)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((commands == NULL) || (count <= 0))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *array = json_object_new_array();
    for (int i = 0; i < count; i++)
    {
        if ((commands[i].command == NULL) || (commands[i].description == NULL))
            return TELEBOT_ERROR_INVALID_PARAMETER;

        struct json_object *obj = json_object_new_object();
        json_object_object_add(obj, "command", json_object_new_string(commands[i].command));
        json_object_object_add(obj, "description", json_object_new_string(commands[i].description));
        json_object_array_add(array, obj);
    }

    const char *array_options = json_object_to_json_string(array);
    DBG("Commands: %s", array_options);

    telebot_core_response_t response;
    int ret = telebot_core_set_my_commands(_handle->core_h, array_options, &response);
    telebot_core_put_response(&response);
    json_object_put(array);

    return ret;
}

telebot_error_e telebot_get_my_commands(telebot_handler_t handle,
    telebot_bot_command_t **commands, int *count)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((commands == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;
    *commands = NULL;
    *count = 0;

    telebot_core_response_t response;
    int ret = telebot_core_get_my_commands(_handle->core_h, &response);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    struct json_object *obj = telebot_parser_str_to_obj(response.data);
    if (obj == NULL)
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *ok = NULL;
    if (!json_object_object_get_ex(obj, "ok", &ok) || !json_object_get_boolean(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct json_object *result = NULL;
    if (!json_object_object_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_array_bot_command(result, commands, count);

finish:
    if (ret) telebot_put_my_commands(*commands, *count);
    if (obj) json_object_put(obj);
    telebot_core_put_response(&response);
    return ret;
}

telebot_error_e telebot_put_my_commands(telebot_bot_command_t *commands, int count)
{
    if ((commands == NULL) || (count <= 0))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    for (int i = 0; i < count; i++)
    {
        TELEBOT_SAFE_FREE(commands[i].command);
        TELEBOT_SAFE_FREE(commands[i].description);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_edit_message_text(telebot_handler_t handle,
    long long int chat_id, int message_id, const char *inline_message_id,
    const char *text, const char *parse_mode, bool disable_web_page_preview,
    const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (text == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    int ret = telebot_core_edit_message_text(_handle->core_h, chat_id, message_id,
        inline_message_id, text, parse_mode, disable_web_page_preview,
        reply_markup, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_edit_message_caption(telebot_handler_t handle,
    long long int chat_id, int message_id, const char *inline_message_id,
    const char *caption, const char *parse_mode, const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_edit_message_caption(_handle->core_h, chat_id, message_id,
        inline_message_id, caption, parse_mode, reply_markup, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_edit_message_reply_markup(telebot_handler_t handle,
    long long int chat_id, int message_id, const char *inline_message_id,
    const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_edit_message_reply_markup(_handle->core_h, chat_id,
        message_id, inline_message_id, reply_markup, &response);
    telebot_core_put_response(&response);

    return ret;
}


telebot_error_e telebot_stop_poll(telebot_handler_t handle, long long int chat_id,
    int message_id, const char *reply_markup)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_stop_poll(_handle->core_h, chat_id,
        message_id, reply_markup, &response);
    telebot_core_put_response(&response);

    return ret;
}

telebot_error_e telebot_delete_message(telebot_handler_t handle, long long int chat_id,
    int message_id)
{
    telebot_hdata_t *_handle = (telebot_hdata_t *)handle;
    if (_handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    int ret = telebot_core_delete_message(_handle->core_h, chat_id,
        message_id, &response);
    telebot_core_put_response(&response);

    return ret;
}

/* Utility functions for releasing memory */
static void telebot_put_user(telebot_user_t *user)
{
    if (user == NULL)
        return;
    TELEBOT_SAFE_FREE(user->first_name);
    TELEBOT_SAFE_FREE(user->last_name);
    TELEBOT_SAFE_FREE(user->username);
    TELEBOT_SAFE_FREE(user->language_code);
}

telebot_error_e telebot_put_chat(telebot_chat_t *chat)
{
    if (chat == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    TELEBOT_SAFE_FREE(chat->type);
    TELEBOT_SAFE_FREE(chat->title);
    TELEBOT_SAFE_FREE(chat->username);
    TELEBOT_SAFE_FREE(chat->first_name);
    TELEBOT_SAFE_FREE(chat->last_name);

    telebot_put_chat_photo(chat->photo);
    TELEBOT_SAFE_FREE(chat->photo);

    TELEBOT_SAFE_FREE(chat->description);
    TELEBOT_SAFE_FREE(chat->invite_link);

    telebot_put_message(chat->pinned_message);
    TELEBOT_SAFE_FREE(chat->pinned_message);

    telebot_put_chat_permissions(chat->permissions);
    TELEBOT_SAFE_FREE(chat->permissions);

    TELEBOT_SAFE_FREE(chat->sticker_set_name);

    return TELEBOT_ERROR_NONE;
}

static void telebot_put_chat_photo(telebot_chat_photo_t *photo)
{
    if (photo == NULL)
        return;
    TELEBOT_SAFE_FREE(photo->small_file_id);
    TELEBOT_SAFE_FREE(photo->small_file_unique_id);
    TELEBOT_SAFE_FREE(photo->big_file_id);
    TELEBOT_SAFE_FREE(photo->big_file_unique_id);
}

static void telebot_put_chat_permissions(telebot_chat_permissions_t *permissions)
{
    if (permissions == NULL)
        return;
    //Nothing for now
    return;
}

static void telebot_put_message(telebot_message_t *msg)
{
    if (msg == NULL)
        return;

    telebot_put_user(msg->from);
    TELEBOT_SAFE_FREE(msg->from);

    telebot_put_chat(msg->chat);
    TELEBOT_SAFE_FREE(msg->chat);

    telebot_put_user(msg->forward_from);
    TELEBOT_SAFE_FREE(msg->forward_from);

    telebot_put_chat(msg->forward_from_chat);
    TELEBOT_SAFE_FREE(msg->forward_from_chat);

    TELEBOT_SAFE_FREE(msg->forward_signature);
    TELEBOT_SAFE_FREE(msg->forward_sender_name);

    telebot_put_message(msg->reply_to_message);
    TELEBOT_SAFE_FREE(msg->reply_to_message);

    TELEBOT_SAFE_FREE(msg->media_group_id);
    TELEBOT_SAFE_FREE(msg->author_signature);
    TELEBOT_SAFE_FREE(msg->text);

    if (msg->entities)
    {
        for (int index = 0; index < msg->count_entities; index++)
            telebot_put_telebot_message_entity(&(msg->entities[index]));
        TELEBOT_SAFE_FREE(msg->entities);
        msg->count_entities = 0;
    }

    if (msg->caption_entities)
    {
        for (int index = 0; index < msg->count_caption_entities; index++)
            telebot_put_telebot_message_entity(&(msg->caption_entities[index]));
        TELEBOT_SAFE_FREE(msg->caption_entities);
        msg->count_caption_entities = 0;
    }

    telebot_put_audio(msg->audio);
    TELEBOT_SAFE_FREE(msg->audio);

    telebot_put_document(msg->document);
    TELEBOT_SAFE_FREE(msg->document);

    //TODO
    //telebot_put_game(msg->game);
    //TELEBOT_SAFE_FREE(msg->game);

    telebot_put_animation(msg->animation);
    TELEBOT_SAFE_FREE(msg->animation);

    if (msg->photos)
    {
        for (int index = 0; index < msg->count_photos; index++)
            telebot_put_photo(&(msg->photos[index]));
        TELEBOT_SAFE_FREE(msg->photos);
        msg->count_photos = 0;
    }

    //TODO
    //telebot_put_sticker(msg->sticker);
    //TELEBOT_SAFE_FREE(msg->sticker);

    telebot_put_video(msg->video);
    TELEBOT_SAFE_FREE(msg->video);

    telebot_put_voice(msg->voice);
    TELEBOT_SAFE_FREE(msg->voice);

    telebot_put_video_note(msg->video_note);
    TELEBOT_SAFE_FREE(msg->video_note);

    TELEBOT_SAFE_FREE(msg->caption);

    telebot_put_contact(msg->contact);
    TELEBOT_SAFE_FREE(msg->contact);

    telebot_put_location(msg->location);
    TELEBOT_SAFE_FREE(msg->location);

    telebot_put_venue(msg->venue);
    TELEBOT_SAFE_FREE(msg->venue);

    telebot_put_poll(msg->poll);
    TELEBOT_SAFE_FREE(msg->poll);

    telebot_put_dice(msg->dice);
    TELEBOT_SAFE_FREE(msg->dice);

    if (msg->new_chat_members)
    {
        for (int index = 0; index < msg->count_new_chat_members; index++)
            telebot_put_user(&(msg->new_chat_members[index]));
        TELEBOT_SAFE_FREE(msg->new_chat_members);
        msg->count_new_chat_members = 0;
    }

    if (msg->left_chat_members)
    {
        for (int index = 0; index < msg->count_left_chat_members; index++)
            telebot_put_user(&(msg->left_chat_members[index]));
        TELEBOT_SAFE_FREE(msg->left_chat_members);
        msg->count_left_chat_members = 0;
    }

    TELEBOT_SAFE_FREE(msg->new_chat_title);

    if (msg->new_chat_photos)
    {
        for (int index = 0; index < msg->count_new_chat_photos; index++)
            telebot_put_photo(&(msg->new_chat_photos[index]));
        TELEBOT_SAFE_FREE(msg->new_chat_photos);
        msg->count_new_chat_photos = 0;
    }

    telebot_put_message(msg->pinned_message);

    //TODO
    //telebot_put_invoice(msg->invoice);
    //TELEBOT_SAFE_FREE(msg->invoice);

    //TODO
    //telebot_put_payment(msg->successful_payment);
    //TELEBOT_SAFE_FREE(msg->successful_payment);

    //TODO
    //telebot_put_passport_data(msg->passport_data);
    //TELEBOT_SAFE_FREE(msg->passport_data);

    //TODO
    //telebot_put_inline_keyboard_markup(msg->reply_markup);
    //TELEBOT_SAFE_FREE(msg->reply_markup);
}

static void telebot_put_telebot_message_entity(telebot_message_entity_t *entity)
{
    if (entity == NULL)
        return;

    TELEBOT_SAFE_FREE(entity->type);
    TELEBOT_SAFE_FREE(entity->url);

    telebot_put_user(entity->user);
    TELEBOT_SAFE_FREE(entity->user);
    TELEBOT_SAFE_FREE(entity->language);
}

static void telebot_put_photo(telebot_photo_t *photo)
{
    if (photo == NULL)
        return;

    TELEBOT_SAFE_FREE(photo->file_id);
    TELEBOT_SAFE_FREE(photo->file_unique_id);
}

static void telebot_put_audio(telebot_audio_t *audio)
{
    if (audio == NULL)
        return;

    TELEBOT_SAFE_FREE(audio->file_id);
    TELEBOT_SAFE_FREE(audio->file_unique_id);
    TELEBOT_SAFE_FREE(audio->performer);
    TELEBOT_SAFE_FREE(audio->title);
    TELEBOT_SAFE_FREE(audio->mime_type);

    telebot_put_photo(audio->thumb);
    TELEBOT_SAFE_FREE(audio->thumb);
}

static void telebot_put_document(telebot_document_t *document)
{
    if (document == NULL)
        return;

    TELEBOT_SAFE_FREE(document->file_id);
    TELEBOT_SAFE_FREE(document->file_unique_id);

    telebot_put_photo(document->thumb);
    TELEBOT_SAFE_FREE(document->thumb);

    TELEBOT_SAFE_FREE(document->file_name);
    TELEBOT_SAFE_FREE(document->mime_type);
}

static void telebot_put_video(telebot_video_t *video)
{
    if (video == NULL)
        return;

    TELEBOT_SAFE_FREE(video->file_id);
    TELEBOT_SAFE_FREE(video->file_unique_id);

    telebot_put_photo(video->thumb);
    TELEBOT_SAFE_FREE(video->thumb);
    TELEBOT_SAFE_FREE(video->mime_type);
}

static void telebot_put_animation(telebot_animation_t *animation)
{
    if (animation == NULL)
        return;

    TELEBOT_SAFE_FREE(animation->file_id);
    TELEBOT_SAFE_FREE(animation->file_unique_id);

    telebot_put_photo(animation->thumb);
    TELEBOT_SAFE_FREE(animation->thumb);
    TELEBOT_SAFE_FREE(animation->file_name);
    TELEBOT_SAFE_FREE(animation->mime_type);
}

static void telebot_put_voice(telebot_voice_t *voice)
{
    if (voice == NULL)
        return;

    TELEBOT_SAFE_FREE(voice->file_id);
    TELEBOT_SAFE_FREE(voice->file_unique_id);
    TELEBOT_SAFE_FREE(voice->mime_type);
}

static void telebot_put_video_note(telebot_video_note_t *vnote)
{
    if (vnote == NULL)
        return;

    TELEBOT_SAFE_FREE(vnote->file_id);
    TELEBOT_SAFE_FREE(vnote->file_unique_id);
    telebot_put_photo(vnote->thumb);
    TELEBOT_SAFE_FREE(vnote->thumb);
}

static void telebot_put_contact(telebot_contact_t *contact)
{
    if (contact == NULL)
        return;

    TELEBOT_SAFE_FREE(contact->phone_number);
    TELEBOT_SAFE_FREE(contact->first_name);
    TELEBOT_SAFE_FREE(contact->last_name);
    TELEBOT_SAFE_FREE(contact->vcard);
}

static void telebot_put_location(telebot_location_t *location)
{
    if (location == NULL)
        return;
    //Nothing to free
    return;
}

static void telebot_put_venue(telebot_venue_t *venue)
{
    if (venue == NULL)
        return;

    telebot_put_location(venue->location);
    TELEBOT_SAFE_FREE(venue->location);
    TELEBOT_SAFE_FREE(venue->title);
    TELEBOT_SAFE_FREE(venue->address);
    TELEBOT_SAFE_FREE(venue->foursquare_id);
    TELEBOT_SAFE_FREE(venue->foursquare_type);
}

static void telebot_put_file(telebot_file_t *file)
{
    if (file == NULL)
        return;

    TELEBOT_SAFE_FREE(file->file_id);
    TELEBOT_SAFE_FREE(file->file_unique_id);
    TELEBOT_SAFE_FREE(file->file_path);
}

static void telebot_put_poll_option(telebot_poll_option_t *poll_option)
{
    if (poll_option == NULL)
        return;

    TELEBOT_SAFE_FREE(poll_option->text);
}

static void telebot_put_poll_answer(telebot_poll_answer_t *answer)
{
    if (answer == NULL)
        return;

    TELEBOT_SAFE_FREE(answer->poll_id);
    telebot_put_user(answer->user);
    TELEBOT_SAFE_FREE(answer->user);
    TELEBOT_SAFE_FREE(answer->option_ids);
}

static void telebot_put_poll(telebot_poll_t *poll)
{
    if (poll == NULL)
        return;

    TELEBOT_SAFE_FREE(poll->id);
    TELEBOT_SAFE_FREE(poll->question);
    if (poll->options)
    {
        for (int index = 0; index < poll->count_options; index++)
            telebot_put_poll_option(&(poll->options[index]));
    }
    TELEBOT_SAFE_FREE(poll->options);
    TELEBOT_SAFE_FREE(poll->type);
}

static void telebot_put_dice(telebot_dice_t *dice)
{
    if (dice == NULL)
        return;
    //Nothing to free
    return;
}

static void telebot_put_callback_query(telebot_callback_query_t *query)
{
    if (query == NULL)
        return;

    TELEBOT_SAFE_FREE(query->id);
    telebot_put_user(query->from);
    TELEBOT_SAFE_FREE(query->from);
    telebot_put_message(query->message);
    TELEBOT_SAFE_FREE(query->message);
    TELEBOT_SAFE_FREE(query->inline_message_id);
    TELEBOT_SAFE_FREE(query->chat_instance);
    TELEBOT_SAFE_FREE(query->data);
    TELEBOT_SAFE_FREE(query->game_short_name);
}

//TODO: static void telebot_put_invoice(telebot_invoice_t *invoice);
//TODO: static void telebot_put_payment(telebot_successful_payment_t *payment);
//TODO: static void telebot_put_game(telebot_game_t *game);
