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
#include <telebot-methods.h>
#include <telebot-core.h>
#include <telebot-parser.h>
#include <telebot-stickers.h>
#include <telebot-payments.h>
#include <telebot-private.h>

static const char *telebot_update_type_str[TELEBOT_UPDATE_TYPE_MAX] = {
    "message",
    "edited_message",
    "channel_post",
    "edited_channel_post",
    "inline_query",
    "chosen_inline_result",
    "callback_query",
    "shipping_query",
    "pre_checkout_query",
    "poll",
    "poll_answer",
    "my_chat_member",
    "chat_member",
    "chat_join_request",
    "message_reaction",
    "message_reaction_count",
    "chat_boost",
    "removed_chat_boost"};

static void telebot_put_chat_photo(telebot_chat_photo_t *photo);
static void telebot_put_chat_permissions(telebot_chat_permissions_t *permissions);
static void telebot_put_chat_location(telebot_chat_location_t *chat_location);
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
static void telebot_put_gift_info(telebot_gift_info_t *gift_info);
static void telebot_put_unique_gift_info(telebot_unique_gift_info_t *gift_info);
static void telebot_put_game(telebot_game_t *game);
static void telebot_put_invoice(telebot_invoice_t *invoice);
static void telebot_put_successful_payment(telebot_successful_payment_t *payment);
static void telebot_put_passport_data(telebot_passport_data_t *passport_data);
static void telebot_put_proximity_alert_triggered(telebot_proximity_alert_triggered_t *alert);
static void telebot_put_forum_topic_created(telebot_forum_topic_created_t *topic);
static void telebot_put_forum_topic_edited(telebot_forum_topic_edited_t *topic);
static void telebot_put_video_chat_scheduled(telebot_video_chat_scheduled_t *scheduled);
static void telebot_put_video_chat_ended(telebot_video_chat_ended_t *ended);
static void telebot_put_video_chat_participants_invited(telebot_video_chat_participants_invited_t *invited);
static void telebot_put_web_app_data(telebot_web_app_data_t *data);
static void telebot_put_inline_keyboard_markup(telebot_inline_keyboard_markup_t *markup);
static void telebot_put_chat_member_updated(telebot_chat_member_updated_t *updated);
static void telebot_put_chat_join_request(telebot_chat_join_request_t *request);
static void telebot_put_message_reaction_updated(telebot_message_reaction_updated_t *updated);
static void telebot_put_message_reaction_count_updated(telebot_message_reaction_count_updated_t *updated);
static void telebot_put_chat_boost_updated(telebot_chat_boost_updated_t *updated);
static void telebot_put_chat_boost_removed(telebot_chat_boost_removed_t *removed);
static void telebot_put_inline_query(telebot_inline_query_t *query);
static void telebot_put_chat_invite_link_internal(telebot_chat_invite_link_t *invite_link);
static void telebot_put_chosen_inline_result(telebot_chosen_inline_result_t *result);
static void telebot_put_shipping_query(telebot_shipping_query_t *query);
static void telebot_put_pre_checkout_query(telebot_pre_checkout_query_t *query);

telebot_error_e telebot_create(telebot_handler_t *handle, char *token)
{
    if ((token == NULL) || (handle == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_handler_t _handle = calloc(1, sizeof(struct telebot_handler));
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
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_destroy(&(handle->core_h));
    TELEBOT_SAFE_FREE(handle);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_set_proxy(telebot_handler_t handle, char *addr, char *auth)
{
    if (addr == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_error_e ret = telebot_core_set_proxy(handle->core_h, addr, auth);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_get_proxy(telebot_handler_t handle, char **addr)
{
    if (addr == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    return telebot_core_get_proxy(handle->core_h, addr);
}

telebot_error_e
telebot_get_updates(telebot_handler_t handle, int offset, int limit, int timeout,
                    telebot_update_type_e allowed_updates[], int allowed_updates_count,
                    telebot_update_t **updates, int *count)
{
    int ret = TELEBOT_ERROR_NONE;
    telebot_core_response_t response;
    struct json_object *obj = NULL;

    if (handle == NULL)
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
        for (int i = 0; i < allowed_updates_count; i++)
        {
            const char *item = telebot_update_type_str[allowed_updates[i]];
            json_object_array_add(array, json_object_new_string(item));
        }
        str_allowed_updates = json_object_to_json_string(array);
        DBG("Allowed updates: %s", str_allowed_updates);
    }

    int _offset = offset != 0 ? offset : handle->offset;
    int _timeout = timeout > 0 ? timeout : 0;
    int _limit = TELEBOT_UPDATE_COUNT_MAX_LIMIT;
    if ((limit > 0) && (limit < TELEBOT_UPDATE_COUNT_MAX_LIMIT))
        _limit = limit;

    response = telebot_core_get_updates(handle->core_h, _offset, _limit, _timeout, str_allowed_updates);
    if (array)
        json_object_put(array);

    ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *response_data = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(response_data);
    ;
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
            if (ups[index].update_id >= handle->offset)
                handle->offset = ups[index].update_id + 1;
        }
    }

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);

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
            telebot_put_poll_answer(&(updates[index].poll_answer));
            break;
        case TELEBOT_UPDATE_TYPE_MY_CHAT_MEMBER:
            telebot_put_chat_member_updated(&(updates[index].my_chat_member));
            break;
        case TELEBOT_UPDATE_TYPE_CHAT_MEMBER:
            telebot_put_chat_member_updated(&(updates[index].chat_member));
            break;
        case TELEBOT_UPDATE_TYPE_CHAT_JOIN_REQUEST:
            telebot_put_chat_join_request(&(updates[index].chat_join_request));
            break;
        case TELEBOT_UPDATE_TYPE_MESSAGE_REACTION:
            telebot_put_message_reaction_updated(&(updates[index].message_reaction));
            break;
        case TELEBOT_UPDATE_TYPE_MESSAGE_REACTION_COUNT:
            telebot_put_message_reaction_count_updated(&(updates[index].message_reaction_count));
            break;
        case TELEBOT_UPDATE_TYPE_CHAT_BOOST:
            telebot_put_chat_boost_updated(&(updates[index].chat_boost));
            break;
        case TELEBOT_UPDATE_TYPE_REMOVED_CHAT_BOOST:
            telebot_put_chat_boost_removed(&(updates[index].chat_boost_removed));
            break;
        case TELEBOT_UPDATE_TYPE_INLINE_QUERY:
            telebot_put_inline_query(&(updates[index].inline_query));
            break;
        case TELEBOT_UPDATE_TYPE_CHOSEN_INLINE_RESULT:
            telebot_put_chosen_inline_result(&(updates[index].chosen_inline_result));
            break;
        case TELEBOT_UPDATE_TYPE_SHIPPING_QUERY:
            telebot_put_shipping_query(&(updates[index].shipping_query));
            break;
        case TELEBOT_UPDATE_TYPE_PRE_CHECKOUT_QUERY:
            telebot_put_pre_checkout_query(&(updates[index].pre_checkout_query));
            break;
        default:
            ERR("Unsupported update type: %d", updates[index].update_type);
        }
    }

    TELEBOT_SAFE_FREE(updates);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_get_me(telebot_handler_t handle, telebot_user_t *me)
{
    int ret = TELEBOT_ERROR_NONE;
    telebot_core_response_t response;
    struct json_object *obj = NULL;

    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (me == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    response = telebot_core_get_me(handle->core_h);
    ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *response_data = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(response_data);
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
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_me(me);

    if (obj)
        json_object_put(obj);

    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_put_me(telebot_user_t *me)
{
    return telebot_put_user(me);
}

telebot_error_e telebot_set_webhook(telebot_handler_t handle, char *url, char *certificate, int max_connections,
                                    telebot_update_type_e allowed_updates[], int allowed_updates_count)
{
    telebot_core_response_t response;

    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (url == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    char allowed_updates_str[TELEBOT_BUFFER_PAGE] = {};
    if (allowed_updates_count > 0)
    {
        strncat(allowed_updates_str, "[", TELEBOT_BUFFER_BLOCK);
        for (int index = 0; index < allowed_updates_count; index++)
        {
            strncat(allowed_updates_str, telebot_update_type_str[allowed_updates[index]],
                    TELEBOT_BUFFER_BLOCK);
            if (index < (allowed_updates_count - 1)) // intermediate element
                strncat(allowed_updates_str, ",", TELEBOT_BUFFER_BLOCK);
        }
        strncat(allowed_updates_str, "]", TELEBOT_BUFFER_BLOCK);
    }

    response = telebot_core_set_webhook(handle->core_h, url, certificate, max_connections, allowed_updates_str);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_delete_webhook(telebot_handler_t handle)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_delete_webhook(handle->core_h);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_get_webhook_info(telebot_handler_t handle, telebot_webhook_info_t *info)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (info == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_get_webhook_info(handle->core_h);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *response_data = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(response_data);
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
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_webhook_info(info);

    if (obj)
        json_object_put(obj);

    telebot_core_put_response(response);

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

telebot_error_e telebot_send_message(telebot_handler_t handle, long long int chat_id, const char *text,
                                     const char *parse_mode, bool disable_web_page_preview, bool disable_notification,
                                     int reply_to_message_id, const char *reply_markup)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (text == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_send_message(handle->core_h, chat_id, text, parse_mode,
                                                                 disable_web_page_preview, disable_notification,
                                                                 reply_to_message_id, reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_forward_message(telebot_handler_t handle, long long int chat_id, long long int from_chat_id,
                                        bool disable_notification, int message_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (message_id <= 0)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_forward_message(handle->core_h, chat_id, from_chat_id,
                                                                    disable_notification, message_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_send_photo(telebot_handler_t handle, long long int chat_id, const char *photo, bool is_file,
                                   const char *caption, const char *parse_mode, bool disable_notification,
                                   int reply_to_message_id, const char *reply_markup)
{
    telebot_core_response_t response;

    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (photo == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    response = telebot_core_send_photo(handle->core_h, chat_id, photo, is_file, caption,
                                       parse_mode, disable_notification, reply_to_message_id,
                                       reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_send_audio(telebot_handler_t handle, long long int chat_id, const char *audio, bool is_file,
                                   const char *caption, const char *parse_mode, int duration, const char *performer,
                                   const char *title, const char *thumb, bool disable_notification,
                                   int reply_to_message_id, const char *reply_markup)
{
    telebot_core_response_t response;

    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (audio == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    response = telebot_core_send_audio(handle->core_h, chat_id, audio, is_file, caption,
                                       parse_mode, duration, performer, title, thumb,
                                       disable_notification, reply_to_message_id, reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_send_document(telebot_handler_t handle, long long int chat_id,
                                      const char *document, bool is_file, const char *thumb, const char *caption,
                                      const char *parse_mode, bool disable_notification, int reply_to_message_id,
                                      const char *reply_markup)
{
    telebot_core_response_t response;

    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (document == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    response = telebot_core_send_document(handle->core_h, chat_id, document, is_file, thumb, caption, parse_mode,
                                          disable_notification, reply_to_message_id, reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_send_video(telebot_handler_t handle, long long int chat_id, const char *video, bool is_file,
                                   int duration, int width, int height, const char *thumb, const char *caption,
                                   const char *parse_mode, bool supports_streaming, bool disable_notification,
                                   int reply_to_message_id, const char *reply_markup)
{
    telebot_core_response_t response;
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (video == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    response = telebot_core_send_video(handle->core_h, chat_id, video, is_file, duration, width, height, thumb,
                                       caption, parse_mode, supports_streaming, disable_notification,
                                       reply_to_message_id, reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_send_animation(telebot_handler_t handle, long long int chat_id, const char *animation,
                                       bool is_file, int duration, int width, int height, const char *thumb,
                                       const char *caption, const char *parse_mode, bool disable_notification,
                                       int reply_to_message_id, const char *reply_markup)
{
    telebot_core_response_t response;
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (animation == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    response = telebot_core_send_animation(handle->core_h, chat_id, animation, is_file, duration, width, height,
                                           thumb, caption, parse_mode, disable_notification, reply_to_message_id,
                                           reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_send_voice(telebot_handler_t handle, long long int chat_id, const char *voice, bool is_file,
                                   const char *caption, const char *parse_mode, int duration, bool disable_notification,
                                   int reply_to_message_id, const char *reply_markup)
{
    telebot_core_response_t response;

    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (voice == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    response = telebot_core_send_voice(handle->core_h, chat_id, voice, is_file, caption, parse_mode, duration,
                                       disable_notification, reply_to_message_id, reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_send_video_note(telebot_handler_t handle, long long int chat_id,
                                        char *video_note, bool is_file, int duration, int length, const char *thumb,
                                        bool disable_notification, int reply_to_message_id, const char *reply_markup)
{
    telebot_core_response_t response;

    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (video_note == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    response = telebot_core_send_video_note(handle->core_h, chat_id, video_note, is_file, duration, length, thumb,
                                            disable_notification, reply_to_message_id, reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_send_media_group(telebot_handler_t handle, long long int chat_id, char *media_paths[],
                                         int count, bool disable_notification, int reply_to_message_id)
{
    telebot_core_response_t response;

    if ((media_paths == NULL) || (count < 2) || (count > 10))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    response = telebot_core_send_media_group(handle->core_h, chat_id, media_paths, count, disable_notification,
                                             reply_to_message_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_send_location(telebot_handler_t handle, long long int chat_id,
                                      float latitude, float longitude, int live_period, bool disable_notification,
                                      int reply_to_message_id, const char *reply_markup)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    response = telebot_core_send_location(handle->core_h, chat_id, latitude, longitude,
                                          live_period, disable_notification, reply_to_message_id, reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_edit_message_live_location(telebot_handler_t handle,
                                                   long long int chat_id, int message_id, const char *inline_message_id,
                                                   float latitude, float longitude, const char *reply_markup)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    response = telebot_core_edit_message_live_location(handle->core_h, chat_id,
                                                       message_id, inline_message_id, latitude, longitude, reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_stop_message_live_location(telebot_handler_t handle,
                                                   long long int chat_id, int message_id, char *inline_message_id,
                                                   const char *reply_markup)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    response = telebot_core_stop_message_live_location(handle->core_h, chat_id, message_id, inline_message_id, reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_send_venue(telebot_handler_t handle, long long int chat_id,
                                   float latitude, float longitude, const char *title, const char *address,
                                   const char *foursquare_id, const char *foursquare_type, bool disable_notification,
                                   int reply_to_message_id, const char *reply_markup)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((title == NULL) || (address == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    response = telebot_core_send_venue(handle->core_h, chat_id, latitude, longitude, title, address, foursquare_id,
                                       foursquare_type, disable_notification, reply_to_message_id, reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_send_contact(telebot_handler_t handle, long long int chat_id,
                                     const char *phone_number, const char *first_name, const char *last_name,
                                     const char *vcard, bool disable_notification, int reply_to_message_id,
                                     const char *reply_markup)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((phone_number == NULL) || (first_name == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    response = telebot_core_send_contact(handle->core_h, chat_id, phone_number,
                                         first_name, last_name, vcard, disable_notification, reply_to_message_id,
                                         reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_send_poll(telebot_handler_t handle, long long int chat_id, const char *question,
                                  const char **options, int count_options, bool is_anonymous, const char *type,
                                  bool allows_multiple_answers, int correct_option_id, bool is_closed,
                                  bool disable_notification, int reply_to_message_id, const char *reply_markup)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((question == NULL) || (options == NULL) || (count_options <= 0))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *array = json_object_new_array();
    for (int i = 0; i < count_options; i++)
        json_object_array_add(array, json_object_new_string(options[i]));

    const char *array_options = json_object_to_json_string(array);
    DBG("Poll options: %s", array_options);

    telebot_core_response_t response;
    response = telebot_core_send_poll(handle->core_h, chat_id, question, array_options, is_anonymous, type,
                                      allows_multiple_answers, correct_option_id, is_closed, disable_notification,
                                      reply_to_message_id, reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    json_object_put(array);

    return ret;
}

telebot_error_e telebot_send_dice(telebot_handler_t handle, long long int chat_id, bool disable_notification,
                                  int reply_to_message_id, const char *reply_markup)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    response = telebot_core_send_dice(handle->core_h, chat_id, disable_notification,
                                      reply_to_message_id, reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_send_chat_action(telebot_handler_t handle, long long int chat_id, char *action)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    response = telebot_core_send_chat_action(handle->core_h, chat_id, action);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_get_user_profile_photos(telebot_handler_t handle, int user_id, int offset, int limit,
                                                telebot_user_profile_photos_t *photos)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (photos == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if ((limit <= 0) || (limit > TELEBOT_USER_PROFILE_PHOTOS_LIMIT))
        limit = TELEBOT_USER_PROFILE_PHOTOS_LIMIT;

    struct json_object *obj = NULL;
    telebot_core_response_t response;
    response = telebot_core_get_user_profile_photos(handle->core_h, user_id, offset, limit);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *rdata = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(rdata);
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
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_user_profile_photos(photos);

    if (obj)
        json_object_put(obj);

    telebot_core_put_response(response);

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

telebot_error_e telebot_get_user_profile_audios(telebot_handler_t handle,
                                                 long long int user_id, int offset, int limit, telebot_user_profile_audios_t *audios)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (audios == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response;
    response = telebot_core_get_user_profile_audios(handle->core_h, user_id, offset, limit);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *rdata = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(rdata);
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

    ret = telebot_parser_get_user_profile_audios(result, audios);

finish:
    if (obj)
        json_object_put(obj);

    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_put_user_profile_audios(telebot_user_profile_audios_t *audios)
{
    if (audios == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (audios->audios)
    {
        for (int i = 0; i < audios->count; i++)
        {
            telebot_put_audio(&(audios->audios[i]));
        }
        free(audios->audios);
    }
    audios->audios = NULL;
    audios->count = 0;
    audios->total_count = 0;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_set_my_profile_photo(telebot_handler_t handle,
                                              const char *photo)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (photo == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_set_my_profile_photo(handle->core_h, photo);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_remove_my_profile_photo(telebot_handler_t handle,
                                                 const char *photo_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (photo_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_remove_my_profile_photo(handle->core_h, photo_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_business_connection(telebot_handler_t handle,
                                                 const char *business_connection_id, telebot_business_connection_t *connection)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (connection == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response;
    response = telebot_core_get_business_connection(handle->core_h, business_connection_id);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *rdata = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(rdata);
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

    ret = telebot_parser_get_business_connection(result, connection);

finish:
    if (obj)
        json_object_put(obj);

    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_put_business_connection(telebot_business_connection_t *connection)
{
    if (connection == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    TELEBOT_SAFE_FREE(connection->id);
    telebot_put_user(connection->user);
    TELEBOT_SAFE_FREE(connection->user);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_get_user_chat_boosts(telebot_handler_t handle,
                                              long long int chat_id, long long int user_id, telebot_user_chat_boosts_t *boosts)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (boosts == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response;
    response = telebot_core_get_user_chat_boosts(handle->core_h, chat_id, user_id);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *rdata = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(rdata);
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

    ret = telebot_parser_get_user_chat_boosts(result, boosts);

finish:
    if (obj)
        json_object_put(obj);

    telebot_core_put_response(response);

    return ret;
}

static void telebot_put_chat_boost_source(telebot_chat_boost_source_t *source)
{
    if (source == NULL)
        return;
    TELEBOT_SAFE_FREE(source->source);
    telebot_put_user(source->user);
    TELEBOT_SAFE_FREE(source->user);
}

static void telebot_put_chat_boost(telebot_chat_boost_t *boost)
{
    if (boost == NULL)
        return;
    TELEBOT_SAFE_FREE(boost->boost_id);
    telebot_put_chat_boost_source(boost->source);
    TELEBOT_SAFE_FREE(boost->source);
}

telebot_error_e telebot_put_user_chat_boosts(telebot_user_chat_boosts_t *boosts)
{
    if (boosts == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (boosts->boosts)
    {
        for (int i = 0; i < boosts->count; i++)
        {
            telebot_put_chat_boost(&(boosts->boosts[i]));
        }
        free(boosts->boosts);
    }
    boosts->boosts = NULL;
    boosts->count = 0;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_download_file(telebot_handler_t handle, const char *file_id, const char *path)
{
    telebot_file_t file;
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (file_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response;
    response = telebot_core_get_file(handle->core_h, file_id);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *rdata = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(rdata);
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

    ret = telebot_parser_get_file(result, &file);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    if (file.file_path == NULL)
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_core_download_file(handle->core_h, file.file_path, path);

finish:
    telebot_put_file(&file);

    if (obj)
        json_object_put(obj);

    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_kick_chat_member(telebot_handler_t handle, long long int chat_id, int user_id, long until_date)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_kick_chat_member(handle->core_h, chat_id, user_id, until_date);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_unban_chat_member(telebot_handler_t handle, long long int chat_id,
                                          int user_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_unban_chat_member(handle->core_h, chat_id, user_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_restrict_chat_member(telebot_handler_t handle, long long int chat_id, int user_id,
                                             long until_date, bool can_send_messages, bool can_send_media_messages,
                                             bool can_send_polls, bool can_send_other_messages,
                                             bool can_add_web_page_previews, bool can_change_info, bool can_invite_users,
                                             bool can_pin_messages)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    response = telebot_core_restrict_chat_member(handle->core_h, chat_id, user_id,
                                                 until_date, can_send_messages, can_send_media_messages, can_send_polls,
                                                 can_send_other_messages, can_add_web_page_previews, can_change_info,
                                                 can_invite_users, can_pin_messages);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_promote_chat_member(telebot_handler_t handle,
                                            long long int chat_id, int user_id, bool can_change_info, bool can_post_messages,
                                            bool can_edit_messages, bool can_delete_messages, bool can_invite_users,
                                            bool can_restrict_members, bool can_pin_messages, bool can_promote_members)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_promote_chat_member(handle->core_h, chat_id, user_id,
                                                                        can_change_info, can_post_messages,
                                                                        can_edit_messages, can_delete_messages,
                                                                        can_invite_users, can_restrict_members,
                                                                        can_pin_messages, can_promote_members);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_chat_admin_custom_title(telebot_handler_t handle,
                                                    long long int chat_id, int user_id, const char *custom_title)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (custom_title == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response;
    response = telebot_core_set_chat_admin_custom_title(handle->core_h, chat_id,
                                                        user_id, custom_title);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_chat_permissions(telebot_handler_t handle,
                                             long long int chat_id, bool can_send_messages, bool can_send_media_messages,
                                             bool can_send_polls, bool can_send_other_messages, bool can_add_web_page_previews,
                                             bool can_change_info, bool can_invite_users, bool can_pin_messages)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    response = telebot_core_set_chat_permissions(handle->core_h, chat_id,
                                                 can_send_messages, can_send_media_messages, can_send_polls,
                                                 can_send_other_messages, can_add_web_page_previews, can_change_info,
                                                 can_invite_users, can_pin_messages);
    int ret = telebot_core_get_response_code(response);

    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_export_chat_invite_link(telebot_handler_t handle,
                                                long long int chat_id, char **invite_link)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (invite_link == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response;
    response = telebot_core_export_chat_invite_link(handle->core_h, chat_id);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *rdata = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(rdata);
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
    if (obj)
        json_object_put(obj);

    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_chat_photo(telebot_handler_t handle, long long int chat_id,
                                       const char *photo)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_set_chat_photo(handle->core_h, chat_id, photo);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_delete_chat_photo(telebot_handler_t handle, long long int chat_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_delete_chat_photo(handle->core_h, chat_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_chat_title(telebot_handler_t handle, long long int chat_id,
                                       const char *title)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (title == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_set_chat_title(handle->core_h, chat_id, title);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_chat_description(telebot_handler_t handle,
                                             long long int chat_id, const char *description)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (description == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_set_chat_description(handle->core_h, chat_id, description);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_pin_chat_message(telebot_handler_t handle, long long int chat_id,
                                         int message_id, bool disable_notification)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    response = telebot_core_pin_chat_message(handle->core_h, chat_id, message_id,
                                             disable_notification);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_unpin_chat_message(telebot_handler_t handle, long long int chat_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    response = telebot_core_unpin_chat_message(handle->core_h, chat_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_leave_chat(telebot_handler_t handle, long long int chat_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response;
    response = telebot_core_leave_chat(handle->core_h, chat_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_chat(telebot_handler_t handle, long long int chat_id,
                                 telebot_chat_t *chat)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (chat == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_get_chat(handle->core_h, chat_id);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *rdata = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(rdata);
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
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_chat(chat);

    if (obj)
        json_object_put(obj);

    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_chat_admins(telebot_handler_t handle, long long int chat_id,
                                        telebot_chat_member_t **admins, int *count)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((admins == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_get_chat_admins(handle->core_h, chat_id);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *rdata = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(rdata);
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
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_chat_admins(*admins, *count);

    if (obj)
        json_object_put(obj);

    telebot_core_put_response(response);
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

telebot_error_e telebot_get_chat_members_count(telebot_handler_t handle, long long int chat_id, int *count)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (count == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_get_chat_members_count(handle->core_h, chat_id);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *rdata = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(rdata);
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
    if (json_object_object_get_ex(obj, "result", &result))
        *count = json_object_get_int(result);
    else
        ret = TELEBOT_ERROR_OPERATION_FAILED;

finish:
    if (obj)
        json_object_put(obj);

    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_chat_member(telebot_handler_t handle, long long int chat_id,
                                        int user_id, telebot_chat_member_t *member)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (member == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_get_chat_member(handle->core_h, chat_id, user_id);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *rdata = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(rdata);
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
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_chat_member(member);

    if (obj)
        json_object_put(obj);

    telebot_core_put_response(response);
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
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (sticker_set_name == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_set_chat_sticker_set(handle->core_h, chat_id, sticker_set_name);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_delete_chat_sticker_set(telebot_handler_t handle,
                                                long long int chat_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_delete_chat_sticker_set(handle->core_h, chat_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_answer_callback_query(telebot_handler_t handle,
                                              const char *callback_query_id, const char *text, bool show_alert,
                                              const char *url, int cache_time)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (callback_query_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_answer_callback_query(handle->core_h, callback_query_id,
                                                                          text, show_alert, url, cache_time);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_my_commands(telebot_handler_t handle,
                                        telebot_bot_command_t commands[], int count)
{
    if (handle == NULL)
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

    telebot_core_response_t response = telebot_core_set_my_commands(handle->core_h, array_options);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    json_object_put(array);
    return ret;
}

telebot_error_e telebot_get_my_commands(telebot_handler_t handle,
                                        telebot_bot_command_t **commands, int *count)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((commands == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_get_my_commands(handle->core_h);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *rdata = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(rdata);
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
    if (ret)
        telebot_put_my_commands(*commands, *count);
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
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
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (text == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_edit_message_text(handle->core_h, chat_id, message_id,
                                                                      inline_message_id, text, parse_mode,
                                                                      disable_web_page_preview,
                                                                      reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_edit_message_caption(telebot_handler_t handle,
                                             long long int chat_id, int message_id, const char *inline_message_id,
                                             const char *caption, const char *parse_mode, const char *reply_markup)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_edit_message_caption(handle->core_h, chat_id, message_id,
                                                                         inline_message_id, caption, parse_mode,
                                                                         reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_edit_message_reply_markup(telebot_handler_t handle,
                                                  long long int chat_id, int message_id, const char *inline_message_id,
                                                  const char *reply_markup)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_edit_message_reply_markup(handle->core_h, chat_id, message_id,
                                                                              inline_message_id, reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_stop_poll(telebot_handler_t handle, long long int chat_id,
                                  int message_id, const char *reply_markup)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_stop_poll(handle->core_h, chat_id, message_id, reply_markup);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_delete_message(telebot_handler_t handle, long long int chat_id, int message_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_delete_message(handle->core_h, chat_id, message_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

/* Utility functions for releasing memory */
telebot_error_e telebot_put_user(telebot_user_t *user)
{
    if (user == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    TELEBOT_SAFE_FREE(user->first_name);
    TELEBOT_SAFE_FREE(user->last_name);
    TELEBOT_SAFE_FREE(user->username);
    TELEBOT_SAFE_FREE(user->language_code);

    return TELEBOT_ERROR_NONE;
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

    for (size_t index = 0; index < chat->count_active_usernames; index++)
        TELEBOT_SAFE_FREE(chat->active_usernames[index]);
    TELEBOT_SAFE_FREE(chat->active_usernames);
    chat->count_active_usernames = 0;

    TELEBOT_SAFE_FREE(chat->emoji_status_custom_emoji_id);
    TELEBOT_SAFE_FREE(chat->bio);

    TELEBOT_SAFE_FREE(chat->description);
    TELEBOT_SAFE_FREE(chat->invite_link);

    telebot_put_message(chat->pinned_message);
    TELEBOT_SAFE_FREE(chat->pinned_message);

    telebot_put_chat_permissions(chat->permissions);
    TELEBOT_SAFE_FREE(chat->permissions);

    TELEBOT_SAFE_FREE(chat->sticker_set_name);
    telebot_put_chat_location(chat->location);
    TELEBOT_SAFE_FREE(chat->location);

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
    // Nothing for now
    return;
}

static void telebot_put_chat_location(telebot_chat_location_t *chat_location)
{
    TELEBOT_SAFE_FREE(chat_location->address);
    telebot_put_location(chat_location->location);
}

static void telebot_put_message(telebot_message_t *msg)
{
    if (msg == NULL)
        return;

    telebot_put_user(msg->from);
    TELEBOT_SAFE_FREE(msg->from);

    telebot_put_chat(msg->sender_chat);
    TELEBOT_SAFE_FREE(msg->sender_chat);

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

    telebot_put_user(msg->via_bot);
    TELEBOT_SAFE_FREE(msg->via_bot);

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

    telebot_put_animation(msg->animation);
    TELEBOT_SAFE_FREE(msg->animation);

    telebot_put_audio(msg->audio);
    TELEBOT_SAFE_FREE(msg->audio);

    telebot_put_document(msg->document);
    TELEBOT_SAFE_FREE(msg->document);

    if (msg->photos)
    {
        for (int index = 0; index < msg->count_photos; index++)
            telebot_put_photo(&(msg->photos[index]));
        TELEBOT_SAFE_FREE(msg->photos);
        msg->count_photos = 0;
    }

    telebot_put_sticker(msg->sticker);
    TELEBOT_SAFE_FREE(msg->sticker);

    telebot_put_video(msg->video);
    TELEBOT_SAFE_FREE(msg->video);

    telebot_put_video_note(msg->video_note);
    TELEBOT_SAFE_FREE(msg->video_note);

    telebot_put_voice(msg->voice);
    TELEBOT_SAFE_FREE(msg->voice);

    TELEBOT_SAFE_FREE(msg->caption);
    if (msg->caption_entities)
    {
        for (int index = 0; index < msg->count_caption_entities; index++)
            telebot_put_telebot_message_entity(&(msg->caption_entities[index]));
        TELEBOT_SAFE_FREE(msg->caption_entities);
        msg->count_caption_entities = 0;
    }

    telebot_put_contact(msg->contact);
    TELEBOT_SAFE_FREE(msg->contact);

    telebot_put_dice(msg->dice);
    TELEBOT_SAFE_FREE(msg->dice);

    telebot_put_game(msg->game);
    TELEBOT_SAFE_FREE(msg->game);

    telebot_put_poll(msg->poll);
    TELEBOT_SAFE_FREE(msg->poll);

    telebot_put_venue(msg->venue);
    TELEBOT_SAFE_FREE(msg->venue);

    telebot_put_location(msg->location);
    TELEBOT_SAFE_FREE(msg->location);

    telebot_put_gift_info(msg->gift);
    TELEBOT_SAFE_FREE(msg->gift);

    telebot_put_unique_gift_info(msg->unique_gift);
    TELEBOT_SAFE_FREE(msg->unique_gift);

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

    TELEBOT_SAFE_FREE(msg->message_auto_delete_timer_changed);

    telebot_put_message(msg->pinned_message);
    TELEBOT_SAFE_FREE(msg->pinned_message);

    telebot_put_invoice(msg->invoice);
    TELEBOT_SAFE_FREE(msg->invoice);

    telebot_put_successful_payment(msg->successful_payment);
    TELEBOT_SAFE_FREE(msg->successful_payment);

    TELEBOT_SAFE_FREE(msg->connected_website);

    telebot_put_passport_data(msg->passport_data);
    TELEBOT_SAFE_FREE(msg->passport_data);

    telebot_put_proximity_alert_triggered(msg->proximity_alert_triggered);
    TELEBOT_SAFE_FREE(msg->proximity_alert_triggered);

    telebot_put_forum_topic_created(msg->forum_topic_created);
    TELEBOT_SAFE_FREE(msg->forum_topic_created);

    telebot_put_forum_topic_edited(msg->forum_topic_edited);
    TELEBOT_SAFE_FREE(msg->forum_topic_edited);

    telebot_put_forum_topic_created((telebot_forum_topic_created_t *)msg->forum_topic_closed);
    TELEBOT_SAFE_FREE(msg->forum_topic_closed);

    telebot_put_forum_topic_created((telebot_forum_topic_created_t *)msg->forum_topic_reopened);
    TELEBOT_SAFE_FREE(msg->forum_topic_reopened);

    TELEBOT_SAFE_FREE(msg->general_forum_topic_hidden);
    TELEBOT_SAFE_FREE(msg->general_forum_topic_unhidden);

    telebot_put_video_chat_scheduled(msg->video_chat_scheduled);
    TELEBOT_SAFE_FREE(msg->video_chat_scheduled);

    TELEBOT_SAFE_FREE(msg->video_chat_started);

    telebot_put_video_chat_ended(msg->video_chat_ended);
    TELEBOT_SAFE_FREE(msg->video_chat_ended);

    telebot_put_video_chat_participants_invited(msg->video_chat_participants_invited);
    TELEBOT_SAFE_FREE(msg->video_chat_participants_invited);

    telebot_put_web_app_data(msg->web_app_data);
    TELEBOT_SAFE_FREE(msg->web_app_data);

    telebot_put_inline_keyboard_markup(msg->reply_markup);
    TELEBOT_SAFE_FREE(msg->reply_markup);
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
    // Nothing to free
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
    // Nothing to free
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

telebot_error_e telebot_put_sticker(telebot_sticker_t *sticker)
{
    if (sticker == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    TELEBOT_SAFE_FREE(sticker->file_id);
    TELEBOT_SAFE_FREE(sticker->file_unique_id);
    telebot_put_photo(sticker->thumb);
    TELEBOT_SAFE_FREE(sticker->thumb);
    TELEBOT_SAFE_FREE(sticker->emoji);
    TELEBOT_SAFE_FREE(sticker->set_name);
    // telebot_put_mask_position(sticker->mask_position);
    TELEBOT_SAFE_FREE(sticker->mask_position);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_put_gift(telebot_gift_t *gift)
{
    if (gift == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    TELEBOT_SAFE_FREE(gift->id);
    telebot_put_sticker(gift->sticker);
    TELEBOT_SAFE_FREE(gift->sticker);

    return TELEBOT_ERROR_NONE;
}

static void telebot_put_gift_info(telebot_gift_info_t *gift_info)
{
    if (gift_info == NULL)
        return;

    telebot_put_gift(&(gift_info->gift));
    TELEBOT_SAFE_FREE(gift_info->text);
    if (gift_info->entities)
    {
        for (int i = 0; i < gift_info->count_entities; i++)
            telebot_put_telebot_message_entity(&(gift_info->entities[i]));
        TELEBOT_SAFE_FREE(gift_info->entities);
    }
}

static void telebot_put_unique_gift(telebot_unique_gift_t *gift)
{
    if (gift == NULL)
        return;

    TELEBOT_SAFE_FREE(gift->gift_id);
    TELEBOT_SAFE_FREE(gift->name);
    telebot_put_sticker(gift->sticker);
    TELEBOT_SAFE_FREE(gift->sticker);
}

static void telebot_put_unique_gift_info(telebot_unique_gift_info_t *gift_info)
{
    if (gift_info == NULL)
        return;

    telebot_put_unique_gift(gift_info->gift);
    TELEBOT_SAFE_FREE(gift_info->gift);
    TELEBOT_SAFE_FREE(gift_info->origin);
}

telebot_error_e telebot_put_user_gift(telebot_user_gift_t *user_gift)
{
    if (user_gift == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    TELEBOT_SAFE_FREE(user_gift->gift_id);
    telebot_put_user(user_gift->sender_user);
    TELEBOT_SAFE_FREE(user_gift->sender_user);
    TELEBOT_SAFE_FREE(user_gift->text);
    if (user_gift->entities)
    {
        for (int i = 0; i < user_gift->count_entities; i++)
            telebot_put_telebot_message_entity(&(user_gift->entities[i]));
        TELEBOT_SAFE_FREE(user_gift->entities);
    }
    telebot_put_gift(user_gift->gift);
    TELEBOT_SAFE_FREE(user_gift->gift);

    return TELEBOT_ERROR_NONE;
}

static void telebot_put_game(telebot_game_t *game)
{
    if (game == NULL)
        return;

    TELEBOT_SAFE_FREE(game->title);
    TELEBOT_SAFE_FREE(game->description);
    if (game->photo)
    {
        for (int i = 0; i < game->count_photo; i++)
            telebot_put_photo(&(game->photo[i]));
        TELEBOT_SAFE_FREE(game->photo);
    }
    TELEBOT_SAFE_FREE(game->text);
    if (game->text_entities)
    {
        for (int i = 0; i < game->count_text_entities; i++)
            telebot_put_telebot_message_entity(&(game->text_entities[i]));
        TELEBOT_SAFE_FREE(game->text_entities);
    }
    telebot_put_animation(game->animation);
    TELEBOT_SAFE_FREE(game->animation);
}

telebot_error_e telebot_log_out(telebot_handler_t handle)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_log_out(handle->core_h);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_close(telebot_handler_t handle)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_close(handle->core_h);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_my_name(telebot_handler_t handle, const char *name, const char *language_code)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_set_my_name(handle->core_h, name, language_code);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_my_name(telebot_handler_t handle, const char *language_code, char **name)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;
    if (name == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_get_my_name(handle->core_h, language_code);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *rdata = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(rdata);
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
    if (json_object_object_get_ex(obj, "result", &result))
    {
        struct json_object *name_obj = NULL;
        if (json_object_object_get_ex(result, "name", &name_obj))
        {
            *name = TELEBOT_SAFE_STRDUP(json_object_get_string(name_obj));
        }
        else
        {
            ret = TELEBOT_ERROR_OPERATION_FAILED;
        }
    }
    else
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
    }

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_my_description(telebot_handler_t handle, const char *description, const char *language_code)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_set_my_description(handle->core_h, description, language_code);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_my_description(telebot_handler_t handle, const char *language_code, char **description)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;
    if (description == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_get_my_description(handle->core_h, language_code);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *rdata = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(rdata);
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
    if (json_object_object_get_ex(obj, "result", &result))
    {
        struct json_object *desc_obj = NULL;
        if (json_object_object_get_ex(result, "description", &desc_obj))
        {
            *description = TELEBOT_SAFE_STRDUP(json_object_get_string(desc_obj));
        }
        else
        {
            ret = TELEBOT_ERROR_OPERATION_FAILED;
        }
    }
    else
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
    }

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_my_short_description(telebot_handler_t handle, const char *short_description, const char *language_code)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_set_my_short_description(handle->core_h, short_description, language_code);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_my_short_description(telebot_handler_t handle, const char *language_code, char **short_description)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;
    if (short_description == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_get_my_short_description(handle->core_h, language_code);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *rdata = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(rdata);
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
    if (json_object_object_get_ex(obj, "result", &result))
    {
        struct json_object *desc_obj = NULL;
        if (json_object_object_get_ex(result, "short_description", &desc_obj))
        {
            *short_description = TELEBOT_SAFE_STRDUP(json_object_get_string(desc_obj));
        }
        else
        {
            ret = TELEBOT_ERROR_OPERATION_FAILED;
        }
    }
    else
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
    }

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_chat_menu_button(telebot_handler_t handle, long long int chat_id, const char *menu_button)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_set_chat_menu_button(handle->core_h, chat_id, menu_button);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_chat_menu_button(telebot_handler_t handle, long long int chat_id, char **menu_button)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;
    if (menu_button == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_get_chat_menu_button(handle->core_h, chat_id);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *rdata = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(rdata);
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
    if (json_object_object_get_ex(obj, "result", &result))
    {
        *menu_button = TELEBOT_SAFE_STRDUP(json_object_get_string(result));
    }
    else
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
    }

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_my_default_administrator_rights(telebot_handler_t handle, const char *rights, bool for_channels)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_set_my_default_administrator_rights(handle->core_h, rights, for_channels);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_my_default_administrator_rights(telebot_handler_t handle, bool for_channels, char **rights)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;
    if (rights == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_get_my_default_administrator_rights(handle->core_h, for_channels);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    const char *rdata = telebot_core_get_response_data(response);
    obj = telebot_parser_str_to_obj(rdata);
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
    if (json_object_object_get_ex(obj, "result", &result))
    {
        *rights = TELEBOT_SAFE_STRDUP(json_object_get_string(result));
    }
    else
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
    }

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_delete_my_commands(telebot_handler_t handle, const char *scope, const char *language_code)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_delete_my_commands(handle->core_h, scope, language_code);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_copy_message(telebot_handler_t handle, long long int chat_id, long long int from_chat_id, int message_id,
                                     const char *caption, const char *parse_mode, const char *caption_entities,
                                     bool disable_notification, bool protect_content, int reply_to_message_id,
                                     bool allow_sending_without_reply, const char *reply_markup, int *message_id_out)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_copy_message(handle->core_h, chat_id, from_chat_id, message_id,
                                                                 caption, parse_mode, caption_entities, disable_notification,
                                                                 protect_content, reply_to_message_id, allow_sending_without_reply,
                                                                 reply_markup);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    if (message_id_out)
    {
        const char *rdata = telebot_core_get_response_data(response);
        obj = telebot_parser_str_to_obj(rdata);
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
        if (json_object_object_get_ex(obj, "result", &result))
        {
            struct json_object *mid_obj = NULL;
            if (json_object_object_get_ex(result, "message_id", &mid_obj))
            {
                *message_id_out = json_object_get_int(mid_obj);
            }
        }
    }

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_copy_messages(telebot_handler_t handle, long long int chat_id, long long int from_chat_id, const char *message_ids,
                                      bool disable_notification, bool protect_content, bool remove_caption,
                                      int **message_ids_out, int *count)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_copy_messages(handle->core_h, chat_id, from_chat_id, message_ids,
                                                                  disable_notification, protect_content, remove_caption);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    if (message_ids_out && count)
    {
        const char *rdata = telebot_core_get_response_data(response);
        obj = telebot_parser_str_to_obj(rdata);
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
        if (json_object_object_get_ex(obj, "result", &result))
        {
            int array_len = json_object_array_length(result);
            *count = array_len;
            *message_ids_out = calloc(array_len, sizeof(int));
            for (int i = 0; i < array_len; i++)
            {
                struct json_object *mid_obj = json_object_array_get_idx(result, i);
                (*message_ids_out)[i] = json_object_get_int(mid_obj);
            }
        }
    }

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_forward_messages(telebot_handler_t handle, long long int chat_id, long long int from_chat_id, const char *message_ids,
                                         bool disable_notification, bool protect_content, int **message_ids_out, int *count)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_forward_messages(handle->core_h, chat_id, from_chat_id, message_ids,
                                                                     disable_notification, protect_content);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    if (message_ids_out && count)
    {
        const char *rdata = telebot_core_get_response_data(response);
        obj = telebot_parser_str_to_obj(rdata);
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
        if (json_object_object_get_ex(obj, "result", &result))
        {
            int array_len = json_object_array_length(result);
            *count = array_len;
            *message_ids_out = calloc(array_len, sizeof(int));
            for (int i = 0; i < array_len; i++)
            {
                struct json_object *mid_obj = json_object_array_get_idx(result, i);
                (*message_ids_out)[i] = json_object_get_int(mid_obj);
            }
        }
    }

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_delete_messages(telebot_handler_t handle, long long int chat_id, const char *message_ids)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_delete_messages(handle->core_h, chat_id, message_ids);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

static void telebot_put_invoice(telebot_invoice_t *invoice)
{
    if (invoice == NULL)
        return;
    TELEBOT_SAFE_FREE(invoice->title);
    TELEBOT_SAFE_FREE(invoice->description);
    TELEBOT_SAFE_FREE(invoice->start_parameter);
    TELEBOT_SAFE_FREE(invoice->currency);
}

static void telebot_put_shipping_address(telebot_shipping_address_t *address)
{
    if (address == NULL)
        return;
    TELEBOT_SAFE_FREE(address->country_code);
    TELEBOT_SAFE_FREE(address->state);
    TELEBOT_SAFE_FREE(address->city);
    TELEBOT_SAFE_FREE(address->street_line1);
    TELEBOT_SAFE_FREE(address->street_line2);
    TELEBOT_SAFE_FREE(address->post_code);
}

static void telebot_put_order_info(telebot_order_info_t *info)
{
    if (info == NULL)
        return;
    TELEBOT_SAFE_FREE(info->name);
    TELEBOT_SAFE_FREE(info->phone_number);
    TELEBOT_SAFE_FREE(info->email);
    telebot_put_shipping_address(info->shipping_address);
    TELEBOT_SAFE_FREE(info->shipping_address);
}

static void telebot_put_successful_payment(telebot_successful_payment_t *payment)
{
    if (payment == NULL)
        return;
    TELEBOT_SAFE_FREE(payment->currency);
    TELEBOT_SAFE_FREE(payment->invoice_payload);
    TELEBOT_SAFE_FREE(payment->shipping_option_id);
    telebot_put_order_info(payment->order_info);
    TELEBOT_SAFE_FREE(payment->order_info);
    TELEBOT_SAFE_FREE(payment->telegram_payment_charge_id);
    TELEBOT_SAFE_FREE(payment->provider_payment_charge_id);
}

static void telebot_put_passport_data(telebot_passport_data_t *passport_data)
{
    if (passport_data == NULL)
        return;
    if (passport_data->data)
    {
        for (int i = 0; i < passport_data->count_data; i++)
        {
            TELEBOT_SAFE_FREE(passport_data->data[i].type);
            TELEBOT_SAFE_FREE(passport_data->data[i].data);
            TELEBOT_SAFE_FREE(passport_data->data[i].phone_number);
            TELEBOT_SAFE_FREE(passport_data->data[i].email);
            if (passport_data->data[i].files)
            {
                for (int j = 0; j < passport_data->data[i].count_files; j++)
                {
                    TELEBOT_SAFE_FREE(passport_data->data[i].files[j].file_id);
                    TELEBOT_SAFE_FREE(passport_data->data[i].files[j].file_unique_id);
                }
                TELEBOT_SAFE_FREE(passport_data->data[i].files);
            }
            TELEBOT_SAFE_FREE(passport_data->data[i].hash);
        }
        TELEBOT_SAFE_FREE(passport_data->data);
    }
    if (passport_data->credentials)
    {
        TELEBOT_SAFE_FREE(passport_data->credentials->data);
        TELEBOT_SAFE_FREE(passport_data->credentials->hash);
        TELEBOT_SAFE_FREE(passport_data->credentials->secret);
        TELEBOT_SAFE_FREE(passport_data->credentials);
    }
}

static void telebot_put_proximity_alert_triggered(telebot_proximity_alert_triggered_t *alert)
{
    if (alert == NULL)
        return;
    telebot_put_user(alert->traveler);
    TELEBOT_SAFE_FREE(alert->traveler);
    telebot_put_user(alert->watcher);
    TELEBOT_SAFE_FREE(alert->watcher);
}

static void telebot_put_forum_topic_created(telebot_forum_topic_created_t *topic)
{
    if (topic == NULL)
        return;
    TELEBOT_SAFE_FREE(topic->name);
    TELEBOT_SAFE_FREE(topic->icon_custom_emoji_id);
}

static void telebot_put_forum_topic_edited(telebot_forum_topic_edited_t *topic)
{
    if (topic == NULL)
        return;
    TELEBOT_SAFE_FREE(topic->name);
    TELEBOT_SAFE_FREE(topic->icon_custom_emoji_id);
}

static void telebot_put_video_chat_scheduled(telebot_video_chat_scheduled_t *scheduled)
{
    if (scheduled == NULL)
        return;
}

static void telebot_put_video_chat_ended(telebot_video_chat_ended_t *ended)
{
    if (ended == NULL)
        return;
}

static void telebot_put_video_chat_participants_invited(telebot_video_chat_participants_invited_t *invited)
{
    if (invited == NULL)
        return;
    if (invited->users)
    {
        for (int i = 0; i < invited->count_users; i++)
            telebot_put_user(&(invited->users[i]));
        TELEBOT_SAFE_FREE(invited->users);
    }
}

static void telebot_put_web_app_data(telebot_web_app_data_t *data)
{
    if (data == NULL)
        return;
    TELEBOT_SAFE_FREE(data->data);
    TELEBOT_SAFE_FREE(data->button_text);
}

static void telebot_put_inline_keyboard_markup(telebot_inline_keyboard_markup_t *markup)
{
    if (markup == NULL)
        return;
    if (markup->inline_keyboard)
    {
        int count = markup->rows * markup->cols;
        for (int i = 0; i < count; i++)
        {
            TELEBOT_SAFE_FREE(markup->inline_keyboard[i].text);
            TELEBOT_SAFE_FREE(markup->inline_keyboard[i].url);
            TELEBOT_SAFE_FREE(markup->inline_keyboard[i].callback_data);
            TELEBOT_SAFE_FREE(markup->inline_keyboard[i].switch_inline_query);
            TELEBOT_SAFE_FREE(markup->inline_keyboard[i].switch_inline_query_current_chat);
        }
        TELEBOT_SAFE_FREE(markup->inline_keyboard);
    }
}

static void telebot_put_chat_member_updated(telebot_chat_member_updated_t *updated)
{
    if (updated == NULL)
        return;
    telebot_put_chat(updated->chat);
    TELEBOT_SAFE_FREE(updated->chat);
    telebot_put_user(updated->from);
    TELEBOT_SAFE_FREE(updated->from);
    telebot_put_chat_member(updated->old_chat_member);
    TELEBOT_SAFE_FREE(updated->old_chat_member);
    telebot_put_chat_member(updated->new_chat_member);
    TELEBOT_SAFE_FREE(updated->new_chat_member);
    telebot_put_chat_invite_link_internal(updated->invite_link);
    TELEBOT_SAFE_FREE(updated->invite_link);
}

static void telebot_put_chat_join_request(telebot_chat_join_request_t *request)
{
    if (request == NULL)
        return;
    telebot_put_chat(request->chat);
    TELEBOT_SAFE_FREE(request->chat);
    telebot_put_user(request->from);
    TELEBOT_SAFE_FREE(request->from);
    TELEBOT_SAFE_FREE(request->bio);
    telebot_put_chat_invite_link_internal(request->invite_link);
    TELEBOT_SAFE_FREE(request->invite_link);
}

static void telebot_put_reaction_type(telebot_reaction_type_t *reaction)
{
    if (reaction == NULL)
        return;
    TELEBOT_SAFE_FREE(reaction->type);
    TELEBOT_SAFE_FREE(reaction->emoji);
    TELEBOT_SAFE_FREE(reaction->custom_emoji_id);
}

static void telebot_put_message_reaction_updated(telebot_message_reaction_updated_t *updated)
{
    if (updated == NULL)
        return;
    telebot_put_chat(updated->chat);
    TELEBOT_SAFE_FREE(updated->chat);
    telebot_put_user(updated->user);
    TELEBOT_SAFE_FREE(updated->user);
    telebot_put_chat(updated->actor_chat);
    TELEBOT_SAFE_FREE(updated->actor_chat);
    if (updated->old_reaction)
    {
        for (int i = 0; i < updated->count_old_reaction; i++)
            telebot_put_reaction_type(&(updated->old_reaction[i]));
        TELEBOT_SAFE_FREE(updated->old_reaction);
    }
    if (updated->new_reaction)
    {
        for (int i = 0; i < updated->count_new_reaction; i++)
            telebot_put_reaction_type(&(updated->new_reaction[i]));
        TELEBOT_SAFE_FREE(updated->new_reaction);
    }
}

static void telebot_put_message_reaction_count_updated(telebot_message_reaction_count_updated_t *updated)
{
    if (updated == NULL)
        return;
    telebot_put_chat(updated->chat);
    TELEBOT_SAFE_FREE(updated->chat);
    if (updated->reactions)
    {
        for (int i = 0; i < updated->count_reactions; i++)
            telebot_put_reaction_type(&(updated->reactions[i].type));
        TELEBOT_SAFE_FREE(updated->reactions);
    }
}

static void telebot_put_chat_boost_updated(telebot_chat_boost_updated_t *updated)
{
    if (updated == NULL)
        return;
    telebot_put_chat(updated->chat);
    TELEBOT_SAFE_FREE(updated->chat);
    if (updated->boost)
    {
        TELEBOT_SAFE_FREE(updated->boost->boost_id);
        if (updated->boost->source)
        {
            TELEBOT_SAFE_FREE(updated->boost->source->source);
            telebot_put_user(updated->boost->source->user);
            TELEBOT_SAFE_FREE(updated->boost->source->user);
            TELEBOT_SAFE_FREE(updated->boost->source);
        }
        TELEBOT_SAFE_FREE(updated->boost);
    }
}

static void telebot_put_chat_boost_removed(telebot_chat_boost_removed_t *removed)
{
    if (removed == NULL)
        return;
    telebot_put_chat(removed->chat);
    TELEBOT_SAFE_FREE(removed->chat);
    TELEBOT_SAFE_FREE(removed->boost_id);
    if (removed->source)
    {
        TELEBOT_SAFE_FREE(removed->source->source);
        telebot_put_user(removed->source->user);
        TELEBOT_SAFE_FREE(removed->source->user);
        TELEBOT_SAFE_FREE(removed->source);
    }
}

static void telebot_put_inline_query(telebot_inline_query_t *query)
{
    if (query == NULL)
        return;
    TELEBOT_SAFE_FREE(query->id);
    telebot_put_user(query->from);
    TELEBOT_SAFE_FREE(query->from);
    TELEBOT_SAFE_FREE(query->query);
    TELEBOT_SAFE_FREE(query->offset);
    TELEBOT_SAFE_FREE(query->chat_type);
    telebot_put_location(query->location);
    TELEBOT_SAFE_FREE(query->location);
}

static void telebot_put_chosen_inline_result(telebot_chosen_inline_result_t *result)
{
    if (result == NULL)
        return;
    TELEBOT_SAFE_FREE(result->result_id);
    telebot_put_user(result->from);
    TELEBOT_SAFE_FREE(result->from);
    telebot_put_location(result->location);
    TELEBOT_SAFE_FREE(result->location);
    TELEBOT_SAFE_FREE(result->inline_message_id);
    TELEBOT_SAFE_FREE(result->query);
}

static void telebot_put_shipping_query(telebot_shipping_query_t *query)
{
    if (query == NULL)
        return;
    TELEBOT_SAFE_FREE(query->id);
    telebot_put_user(query->from);
    TELEBOT_SAFE_FREE(query->from);
    TELEBOT_SAFE_FREE(query->invoice_payload);
    telebot_put_shipping_address(query->shipping_address);
    TELEBOT_SAFE_FREE(query->shipping_address);
}

static void telebot_put_pre_checkout_query(telebot_pre_checkout_query_t *query)
{
    if (query == NULL)
        return;
    TELEBOT_SAFE_FREE(query->id);
    telebot_put_user(query->from);
    TELEBOT_SAFE_FREE(query->from);
    TELEBOT_SAFE_FREE(query->currency);
    TELEBOT_SAFE_FREE(query->invoice_payload);
    TELEBOT_SAFE_FREE(query->shipping_option_id);
    telebot_put_order_info(query->order_info);
    TELEBOT_SAFE_FREE(query->order_info);
}

static void telebot_put_chat_invite_link_internal(telebot_chat_invite_link_t *invite_link)
{
    if (invite_link == NULL)
        return;

    TELEBOT_SAFE_FREE(invite_link->invite_link);
    telebot_put_user(invite_link->creator);
    TELEBOT_SAFE_FREE(invite_link->creator);
    TELEBOT_SAFE_FREE(invite_link->name);
}

telebot_error_e telebot_put_chat_invite_link(telebot_chat_invite_link_t *invite_link)
{
    if (invite_link == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    telebot_put_chat_invite_link_internal(invite_link);
    return TELEBOT_ERROR_NONE;
}
