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
#include "telebot-json.h"
#include "telebot-config.h"
#include <telebot-methods.h>
#include <telebot-core.h>
#include <telebot-parser.h>
#include <telebot-stickers.h>
#include <telebot-payments.h>
#include <telebot-private.h>

static void telebot_put_chat_photo(telebot_chat_photo_t *photo);
static void telebot_put_chat_permissions(telebot_chat_permissions_t *permissions);
static void telebot_put_chat_location(telebot_chat_location_t *chat_location);
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

/*
 * Shared tail of every "call, then parse result into a struct" API: checks the
 * response, verifies "ok", and hands "result" to `parse`. Releases the response.
 */
typedef telebot_error_e (*telebot_result_parser_t)(cJSON *result, void *out);

static telebot_error_e telebot_call_and_parse(telebot_core_response_t response, telebot_result_parser_t parse,
                                              void *out)
{
    telebot_error_e ret = telebot_core_get_response_code(response);
    cJSON *root = NULL;
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    root = telebot_parser_str_to_obj(telebot_core_get_response_data(response));
    cJSON *ok = NULL, *result = NULL;
    if ((root == NULL) || !tb_json_get_ex(root, "ok", &ok) || !tb_json_get_bool(ok) ||
        !tb_json_get_ex(root, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }
    ret = parse(result, out);

finish:
    tb_json_put(root);
    telebot_core_put_response(response);
    return ret;
}

static telebot_error_e parse_sticker_set(cJSON *result, void *out)
{
    return telebot_parser_get_sticker_set(result, out);
}

static telebot_error_e parse_invite_link(cJSON *result, void *out)
{
    return telebot_parser_get_chat_invite_link(result, out);
}

/* For APIs whose result is just `true` */
static telebot_error_e telebot_call(telebot_core_response_t response)
{
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

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

/*
 * Serialises an allowed_updates filter as a JSON array of update names.
 * count <= 0 means "no filter": *out stays NULL and the field is omitted.
 * The caller frees *out.
 */
static telebot_error_e telebot_allowed_updates_json(const telebot_update_type_e types[], int count, char **out)
{
    *out = NULL;
    if (count <= 0)
        return TELEBOT_ERROR_NONE;
    if (types == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    for (int i = 0; i < count; i++)
    {
        if (((unsigned)types[i]) >= TELEBOT_UPDATE_TYPE_MAX)
        {
            ERR("Invalid update type %d", (int)types[i]);
            return TELEBOT_ERROR_INVALID_PARAMETER;
        }
    }

    cJSON *array = cJSON_CreateArray();
    for (int i = 0; array && (i < count); i++)
        cJSON_AddItemToArray(array, cJSON_CreateString(telebot_update_type_names[types[i]]));

    *out = array ? cJSON_PrintUnformatted(array) : NULL;
    tb_json_put(array);
    return (*out != NULL) ? TELEBOT_ERROR_NONE : TELEBOT_ERROR_OUT_OF_MEMORY;
}

telebot_error_e
telebot_get_updates(telebot_handler_t handle, int offset, int limit, int timeout,
                    telebot_update_type_e allowed_updates[], int allowed_updates_count,
                    telebot_update_t **updates, int *count)
{
    int ret = TELEBOT_ERROR_NONE;
    telebot_core_response_t response;
    struct cJSON *obj = NULL;

    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((updates == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    *updates = NULL;
    *count = 0;

    char *str_allowed_updates = NULL;
    ret = telebot_allowed_updates_json(allowed_updates, allowed_updates_count, &str_allowed_updates);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    int _offset = offset != 0 ? offset : handle->offset;
    int _timeout = timeout > 0 ? timeout : 0;
    /* limit <= 0 selects the build-time default (small on memory-constrained targets) */
    int _limit = TELEBOT_DEFAULT_UPDATE_LIMIT;
    if (limit > 0)
        _limit = (limit < TELEBOT_UPDATE_COUNT_MAX_LIMIT) ? limit : TELEBOT_UPDATE_COUNT_MAX_LIMIT;

    response = telebot_core_get_updates(handle->core_h, _offset, _limit, _timeout, str_allowed_updates);
    TELEBOT_SAFE_FREE(str_allowed_updates);

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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!tb_json_get_ex(obj, "result", &result))
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
        tb_json_put(obj);
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
        case TELEBOT_UPDATE_TYPE_GUEST_MESSAGE:
            telebot_put_message(&(updates[index].guest_message));
            break;
        case TELEBOT_UPDATE_TYPE_STOPPED_MESSAGE_GENERATION:
            telebot_put_chat(updates[index].stopped_message_generation.chat);
            TELEBOT_SAFE_FREE(updates[index].stopped_message_generation.chat);
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
        case TELEBOT_UPDATE_TYPE_UNKNOWN:
            break; /* nothing was parsed */
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
    struct cJSON *obj = NULL;

    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (me == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    /* The cleanup path frees *me, so it must start from a known state */
    memset(me, 0, sizeof(*me));

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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!tb_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_user(result, me);

finish:
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_me(me);

    if (obj)
        tb_json_put(obj);

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

    char *allowed_updates_str = NULL;
    telebot_error_e err = telebot_allowed_updates_json(allowed_updates, allowed_updates_count, &allowed_updates_str);
    if (err != TELEBOT_ERROR_NONE)
        return err;

    response = telebot_core_set_webhook(handle->core_h, url, certificate, max_connections, allowed_updates_str);
    free(allowed_updates_str);
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

    memset(info, 0, sizeof(*info));

    struct cJSON *obj = NULL;
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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!tb_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_webhook_info(result, info);

finish:
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_webhook_info(info);

    if (obj)
        tb_json_put(obj);

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
        const char *parse_mode, bool disable_link_preview, const telebot_send_options_t *options)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_send_message(handle->core_h, chat_id, text, parse_mode, disable_link_preview, options);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_forward_message(telebot_handler_t handle, long long int chat_id,
        long long int from_chat_id, int message_id, const telebot_send_options_t *options)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_forward_message(handle->core_h, chat_id, from_chat_id, message_id, options);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_send_photo(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *photo, const char *caption, const char *parse_mode,
        const telebot_send_options_t *options)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_send_photo(handle->core_h, chat_id, photo, caption, parse_mode, options);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_send_audio(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *audio, const char *caption, const char *parse_mode, int duration,
        const char *performer, const char *title, const telebot_input_file_t *thumbnail,
        const telebot_send_options_t *options)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_send_audio(handle->core_h, chat_id, audio, caption, parse_mode, duration, performer, title, thumbnail, options);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_send_document(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *document, const telebot_input_file_t *thumbnail, const char *caption,
        const char *parse_mode, const telebot_send_options_t *options)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_send_document(handle->core_h, chat_id, document, thumbnail, caption, parse_mode, options);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_send_video(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *video, int duration, int width, int height,
        const telebot_input_file_t *thumbnail, const char *caption, const char *parse_mode,
        bool supports_streaming, const telebot_send_options_t *options)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_send_video(handle->core_h, chat_id, video, duration, width, height, thumbnail, caption, parse_mode, supports_streaming, options);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_send_animation(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *animation, int duration, int width, int height,
        const telebot_input_file_t *thumbnail, const char *caption, const char *parse_mode,
        const telebot_send_options_t *options)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_send_animation(handle->core_h, chat_id, animation, duration, width, height, thumbnail, caption, parse_mode, options);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_send_voice(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *voice, const char *caption, const char *parse_mode, int duration,
        const telebot_send_options_t *options)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_send_voice(handle->core_h, chat_id, voice, caption, parse_mode, duration, options);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_send_video_note(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *video_note, int duration, int length,
        const telebot_input_file_t *thumbnail, const telebot_send_options_t *options)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_send_video_note(handle->core_h, chat_id, video_note, duration, length, thumbnail, options);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_send_media_group(telebot_handler_t handle, long long int chat_id,
        const telebot_input_media_t media[], int count, const telebot_send_options_t *options)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_send_media_group(handle->core_h, chat_id, media, count, options);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_send_location(telebot_handler_t handle, long long int chat_id, double latitude,
        double longitude, int live_period, const telebot_send_options_t *options)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_send_location(handle->core_h, chat_id, latitude, longitude, live_period, options);
    telebot_error_e ret = telebot_core_get_response_code(response);
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

telebot_error_e telebot_send_venue(telebot_handler_t handle, long long int chat_id, double latitude,
        double longitude, const char *title, const char *address, const char *foursquare_id,
        const char *foursquare_type, const telebot_send_options_t *options)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_send_venue(handle->core_h, chat_id, latitude, longitude, title, address, foursquare_id, foursquare_type, options);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_send_contact(telebot_handler_t handle, long long int chat_id,
        const char *phone_number, const char *first_name, const char *last_name, const char *vcard,
        const telebot_send_options_t *options)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_send_contact(handle->core_h, chat_id, phone_number, first_name, last_name, vcard, options);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_send_poll(telebot_handler_t handle, long long int chat_id, const char *question,
        const char *const answers[], int count_answers, bool is_anonymous, const char *type,
        bool allows_multiple_answers, const int correct_option_ids[], int count_correct, bool is_closed,
        const telebot_send_options_t *options)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_send_poll(handle->core_h, chat_id, question, answers, count_answers, is_anonymous, type, allows_multiple_answers, correct_option_ids, count_correct, is_closed, options);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_send_dice(telebot_handler_t handle, long long int chat_id, const char *emoji,
        const telebot_send_options_t *options)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_send_dice(handle->core_h, chat_id, emoji, options);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_send_chat_action(telebot_handler_t handle, long long int chat_id,
        int message_thread_id, const char *action)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_send_chat_action(handle->core_h, chat_id, message_thread_id, action);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_user_profile_photos(telebot_handler_t handle, long long int user_id, int offset,
        int limit, telebot_user_profile_photos_t *photos)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (photos == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(photos, 0, sizeof(*photos));

    if ((limit <= 0) || (limit > TELEBOT_USER_PROFILE_PHOTOS_LIMIT))
        limit = TELEBOT_USER_PROFILE_PHOTOS_LIMIT;

    struct cJSON *obj = NULL;
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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!tb_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_user_profile_photos(result, photos);

finish:
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_user_profile_photos(photos);

    if (obj)
        tb_json_put(obj);

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

    struct cJSON *obj = NULL;
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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!tb_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_user_profile_audios(result, audios);

finish:
    if (obj)
        tb_json_put(obj);

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

    struct cJSON *obj = NULL;
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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!tb_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_business_connection(result, connection);

finish:
    if (obj)
        tb_json_put(obj);

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

    struct cJSON *obj = NULL;
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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!tb_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_user_chat_boosts(result, boosts);

finish:
    if (obj)
        tb_json_put(obj);

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

    struct cJSON *obj = NULL;
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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!tb_json_get_ex(obj, "result", &result))
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
        tb_json_put(obj);

    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_unban_chat_member(telebot_handler_t handle, long long int chat_id,
        long long int user_id, bool only_if_banned)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_unban_chat_member(handle->core_h, chat_id, user_id, only_if_banned);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_restrict_chat_member(telebot_handler_t handle, long long int chat_id,
        long long int user_id, const telebot_chat_permissions_t *permissions,
        bool use_independent_chat_permissions, long until_date)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_restrict_chat_member(handle->core_h, chat_id, user_id, permissions, use_independent_chat_permissions, until_date);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_promote_chat_member(telebot_handler_t handle, long long int chat_id,
        long long int user_id, const telebot_chat_administrator_rights_t *rights)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_promote_chat_member(handle->core_h, chat_id, user_id, rights);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_chat_admin_custom_title(telebot_handler_t handle, long long int chat_id,
        long long int user_id, const char *custom_title)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_set_chat_admin_custom_title(handle->core_h, chat_id, user_id, custom_title);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_chat_permissions(telebot_handler_t handle, long long int chat_id,
        const telebot_chat_permissions_t *permissions, bool use_independent_chat_permissions)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_set_chat_permissions(handle->core_h, chat_id, permissions, use_independent_chat_permissions);
    telebot_error_e ret = telebot_core_get_response_code(response);
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

    struct cJSON *obj = NULL;
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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (tb_json_get_ex(obj, "result", &result))
    {
        *invite_link = TELEBOT_SAFE_STRDUP(tb_json_get_string(result));
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
        tb_json_put(obj);

    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_chat_photo(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *photo)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_set_chat_photo(handle->core_h, chat_id, photo);
    telebot_error_e ret = telebot_core_get_response_code(response);
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

    memset(chat, 0, sizeof(*chat));

    struct cJSON *obj = NULL;
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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!tb_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_chat(result, chat);

finish:
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_chat(chat);

    if (obj)
        tb_json_put(obj);

    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_chat_admins(telebot_handler_t handle, long long int chat_id, bool return_bots,
        telebot_chat_member_t **admins, int *count)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((admins == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    *admins = NULL;
    *count = 0;

    struct cJSON *obj = NULL;
    telebot_core_response_t response = telebot_core_get_chat_admins(handle->core_h, chat_id, return_bots);
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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!tb_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_chat_admins(result, admins, count);

finish:
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_chat_admins(*admins, *count);

    if (obj)
        tb_json_put(obj);

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

    struct cJSON *obj = NULL;
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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (tb_json_get_ex(obj, "result", &result))
        *count = tb_json_get_int(result);
    else
        ret = TELEBOT_ERROR_OPERATION_FAILED;

finish:
    if (obj)
        tb_json_put(obj);

    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_chat_member(telebot_handler_t handle, long long int chat_id,
        long long int user_id, telebot_chat_member_t *member)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (member == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(member, 0, sizeof(*member));

    struct cJSON *obj = NULL;
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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!tb_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_chat_member(result, member);

finish:
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_chat_member(member);

    if (obj)
        tb_json_put(obj);

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
    TELEBOT_SAFE_FREE(member->custom_title);
    TELEBOT_SAFE_FREE(member->tag);

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

    for (int i = 0; i < count; i++)
    {
        if ((commands[i].command == NULL) || (commands[i].description == NULL))
            return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    cJSON *array = cJSON_CreateArray();
    for (int i = 0; array && (i < count); i++)
    {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "command", commands[i].command);
        cJSON_AddStringToObject(obj, "description", commands[i].description);
        cJSON_AddItemToArray(array, obj);
    }

    char *array_options = array ? cJSON_PrintUnformatted(array) : NULL;
    tb_json_put(array);
    if (array_options == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    DBG("Commands: %s", array_options);

    telebot_core_response_t response = telebot_core_set_my_commands(handle->core_h, array_options);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    free(array_options);
    return ret;
}

telebot_error_e telebot_get_my_commands(telebot_handler_t handle,
                                        telebot_bot_command_t **commands, int *count)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if ((commands == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct cJSON *obj = NULL;
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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!tb_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_array_bot_command(result, commands, count);

finish:
    if (ret)
        telebot_put_my_commands(*commands, *count);
    if (obj)
        tb_json_put(obj);
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

telebot_error_e telebot_edit_message_text(telebot_handler_t handle, long long int chat_id, int message_id,
        const char *inline_message_id, const char *text, const char *parse_mode, bool disable_link_preview,
        const char *reply_markup)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_edit_message_text(handle->core_h, chat_id, message_id, inline_message_id, text, parse_mode, disable_link_preview, reply_markup);
    telebot_error_e ret = telebot_core_get_response_code(response);
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
    if (chat_location == NULL)
        return;

    TELEBOT_SAFE_FREE(chat_location->address);
    telebot_put_location(chat_location->location);
}

/* ---- Release helpers for message sub-objects (every pointer field, NULL-safe) ---- */

static void telebot_put_message_origin(telebot_message_origin_t *origin)
{
    if (origin == NULL)
        return;
    TELEBOT_SAFE_FREE(origin->type);
    telebot_put_user(origin->sender_user);
    TELEBOT_SAFE_FREE(origin->sender_user);
    TELEBOT_SAFE_FREE(origin->sender_user_name);
    telebot_put_chat(origin->sender_chat);
    TELEBOT_SAFE_FREE(origin->sender_chat);
    TELEBOT_SAFE_FREE(origin->author_signature);
}

static void telebot_put_text_quote(telebot_text_quote_t *quote)
{
    if (quote == NULL)
        return;
    TELEBOT_SAFE_FREE(quote->text);
    for (int i = 0; i < quote->count_entities; i++)
        telebot_put_telebot_message_entity(&quote->entities[i]);
    TELEBOT_SAFE_FZCNT(quote->entities, quote->count_entities);
}

static void telebot_put_story(telebot_story_t *story)
{
    if (story == NULL)
        return;
    telebot_put_chat(story->chat);
    TELEBOT_SAFE_FREE(story->chat);
}

static void telebot_put_link_preview_options(telebot_link_preview_options_t *options)
{
    if (options == NULL)
        return;
    TELEBOT_SAFE_FREE(options->url);
}

static void telebot_put_photos(telebot_photo_t **photos, int *count)
{
    for (int i = 0; i < *count; i++)
        telebot_put_photo(&(*photos)[i]);
    TELEBOT_SAFE_FZCNT(*photos, *count);
}

static void telebot_put_paid_media_info(telebot_paid_media_info_t *info)
{
    if (info == NULL)
        return;
    for (int i = 0; i < info->count_paid_media; i++)
    {
        telebot_paid_media_t *media = &info->paid_media[i];
        TELEBOT_SAFE_FREE(media->type);
        telebot_put_photos(&media->photos, &media->count_photos);
        telebot_put_video(media->video);
        TELEBOT_SAFE_FREE(media->video);
    }
    TELEBOT_SAFE_FZCNT(info->paid_media, info->count_paid_media);
}

static void telebot_put_external_reply_info(telebot_external_reply_info_t *info)
{
    if (info == NULL)
        return;
    telebot_put_message_origin(&info->origin);
    telebot_put_chat(info->chat);
    TELEBOT_SAFE_FREE(info->chat);
    telebot_put_link_preview_options(info->link_preview_options);
    TELEBOT_SAFE_FREE(info->link_preview_options);
    telebot_put_animation(info->animation);
    TELEBOT_SAFE_FREE(info->animation);
    telebot_put_audio(info->audio);
    TELEBOT_SAFE_FREE(info->audio);
    telebot_put_document(info->document);
    TELEBOT_SAFE_FREE(info->document);
    telebot_put_paid_media_info(info->paid_media);
    TELEBOT_SAFE_FREE(info->paid_media);
    telebot_put_photos(&info->photos, &info->count_photos);
    telebot_put_sticker(info->sticker);
    TELEBOT_SAFE_FREE(info->sticker);
    telebot_put_story(info->story);
    TELEBOT_SAFE_FREE(info->story);
    telebot_put_video(info->video);
    TELEBOT_SAFE_FREE(info->video);
    telebot_put_video_note(info->video_note);
    TELEBOT_SAFE_FREE(info->video_note);
    telebot_put_voice(info->voice);
    TELEBOT_SAFE_FREE(info->voice);
    telebot_put_contact(info->contact);
    TELEBOT_SAFE_FREE(info->contact);
    telebot_put_dice(info->dice);
    TELEBOT_SAFE_FREE(info->dice);
    telebot_put_game(info->game);
    TELEBOT_SAFE_FREE(info->game);
    TELEBOT_SAFE_FREE(info->giveaway);         /* not parsed yet: no owned members */
    TELEBOT_SAFE_FREE(info->giveaway_winners); /* not parsed yet: no owned members */
    telebot_put_invoice(info->invoice);
    TELEBOT_SAFE_FREE(info->invoice);
    telebot_put_location(info->location);
    TELEBOT_SAFE_FREE(info->location);
    telebot_put_poll(info->poll);
    TELEBOT_SAFE_FREE(info->poll);
    telebot_put_venue(info->venue);
    TELEBOT_SAFE_FREE(info->venue);
}

static void telebot_put_refunded_payment(telebot_refunded_payment_t *payment)
{
    if (payment == NULL)
        return;
    TELEBOT_SAFE_FREE(payment->currency);
    TELEBOT_SAFE_FREE(payment->invoice_payload);
    TELEBOT_SAFE_FREE(payment->telegram_payment_charge_id);
    TELEBOT_SAFE_FREE(payment->provider_payment_charge_id);
}

static void telebot_put_users_shared(telebot_users_shared_t *shared)
{
    if (shared == NULL)
        return;
    for (int i = 0; i < shared->count_users; i++)
    {
        telebot_shared_user_t *user = &shared->users[i];
        TELEBOT_SAFE_FREE(user->first_name);
        TELEBOT_SAFE_FREE(user->last_name);
        TELEBOT_SAFE_FREE(user->username);
        telebot_put_photos(&user->photo, &user->count_photo);
    }
    TELEBOT_SAFE_FZCNT(shared->users, shared->count_users);
}

/* Frees the members a message owns that have no dedicated release function elsewhere */
static void telebot_put_message_extras(telebot_message_t *msg)
{
    TELEBOT_SAFE_FREE(msg->sender_tag);
    TELEBOT_SAFE_FREE(msg->business_connection_id);
    TELEBOT_SAFE_FREE(msg->effect_id);
    telebot_put_user(msg->sender_business_bot);
    TELEBOT_SAFE_FREE(msg->sender_business_bot);
    telebot_put_message_origin(msg->forward_origin);
    TELEBOT_SAFE_FREE(msg->forward_origin);
    telebot_put_external_reply_info(msg->external_reply);
    TELEBOT_SAFE_FREE(msg->external_reply);
    telebot_put_text_quote(msg->quote);
    TELEBOT_SAFE_FREE(msg->quote);
    telebot_put_story(msg->reply_to_story);
    TELEBOT_SAFE_FREE(msg->reply_to_story);
    telebot_put_link_preview_options(msg->link_preview_options);
    TELEBOT_SAFE_FREE(msg->link_preview_options);
    telebot_put_paid_media_info(msg->paid_media);
    TELEBOT_SAFE_FREE(msg->paid_media);
    telebot_put_refunded_payment(msg->refunded_payment);
    TELEBOT_SAFE_FREE(msg->refunded_payment);
    if (msg->write_access_allowed)
        TELEBOT_SAFE_FREE(msg->write_access_allowed->web_app_name);
    TELEBOT_SAFE_FREE(msg->write_access_allowed);
    telebot_put_users_shared(msg->users_shared);
    TELEBOT_SAFE_FREE(msg->users_shared);
    TELEBOT_SAFE_FREE(msg->chat_shared);
    TELEBOT_SAFE_FREE(msg->boost_added);
    if (msg->chat_background_set)
        TELEBOT_SAFE_FREE(msg->chat_background_set->type);
    TELEBOT_SAFE_FREE(msg->chat_background_set);
    TELEBOT_SAFE_FREE(msg->giveaway_created);
    if (msg->giveaway_completed)
    {
        telebot_put_message(msg->giveaway_completed->giveaway_message);
        TELEBOT_SAFE_FREE(msg->giveaway_completed->giveaway_message);
    }
    TELEBOT_SAFE_FREE(msg->giveaway_completed);
}

telebot_error_e telebot_put_message(telebot_message_t *msg)
{
    if (msg == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_put_message_extras(msg);

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

    return TELEBOT_ERROR_NONE;
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

    telebot_put_photo(audio->thumbnail);
    TELEBOT_SAFE_FREE(audio->thumbnail);
}

static void telebot_put_document(telebot_document_t *document)
{
    if (document == NULL)
        return;

    TELEBOT_SAFE_FREE(document->file_id);
    TELEBOT_SAFE_FREE(document->file_unique_id);

    telebot_put_photo(document->thumbnail);
    TELEBOT_SAFE_FREE(document->thumbnail);

    TELEBOT_SAFE_FREE(document->file_name);
    TELEBOT_SAFE_FREE(document->mime_type);
}

static void telebot_put_video(telebot_video_t *video)
{
    if (video == NULL)
        return;

    TELEBOT_SAFE_FREE(video->file_id);
    TELEBOT_SAFE_FREE(video->file_unique_id);

    telebot_put_photo(video->thumbnail);
    TELEBOT_SAFE_FREE(video->thumbnail);
    TELEBOT_SAFE_FREE(video->mime_type);
}

static void telebot_put_animation(telebot_animation_t *animation)
{
    if (animation == NULL)
        return;

    TELEBOT_SAFE_FREE(animation->file_id);
    TELEBOT_SAFE_FREE(animation->file_unique_id);

    telebot_put_photo(animation->thumbnail);
    TELEBOT_SAFE_FREE(animation->thumbnail);
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
    telebot_put_photo(vnote->thumbnail);
    TELEBOT_SAFE_FREE(vnote->thumbnail);
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
    TELEBOT_SAFE_FREE(sticker->type);
    telebot_put_photo(sticker->thumbnail);
    TELEBOT_SAFE_FREE(sticker->thumbnail);
    TELEBOT_SAFE_FREE(sticker->emoji);
    TELEBOT_SAFE_FREE(sticker->set_name);
    if (sticker->mask_position)
        TELEBOT_SAFE_FREE(sticker->mask_position->point);
    TELEBOT_SAFE_FREE(sticker->mask_position);
    TELEBOT_SAFE_FREE(sticker->custom_emoji_id);

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

    struct cJSON *obj = NULL;
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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (tb_json_get_ex(obj, "result", &result))
    {
        struct cJSON *name_obj = NULL;
        if (tb_json_get_ex(result, "name", &name_obj))
        {
            *name = TELEBOT_SAFE_STRDUP(tb_json_get_string(name_obj));
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
        tb_json_put(obj);
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

    struct cJSON *obj = NULL;
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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (tb_json_get_ex(obj, "result", &result))
    {
        struct cJSON *desc_obj = NULL;
        if (tb_json_get_ex(result, "description", &desc_obj))
        {
            *description = TELEBOT_SAFE_STRDUP(tb_json_get_string(desc_obj));
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
        tb_json_put(obj);
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

    struct cJSON *obj = NULL;
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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (tb_json_get_ex(obj, "result", &result))
    {
        struct cJSON *desc_obj = NULL;
        if (tb_json_get_ex(result, "short_description", &desc_obj))
        {
            *short_description = TELEBOT_SAFE_STRDUP(tb_json_get_string(desc_obj));
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
        tb_json_put(obj);
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

    struct cJSON *obj = NULL;
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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (tb_json_get_ex(obj, "result", &result))
    {
        *menu_button = TELEBOT_SAFE_STRDUP(tb_json_get_string(result));
    }
    else
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
    }

finish:
    if (obj)
        tb_json_put(obj);
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

    struct cJSON *obj = NULL;
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

    struct cJSON *ok = NULL;
    if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (tb_json_get_ex(obj, "result", &result))
    {
        *rights = TELEBOT_SAFE_STRDUP(tb_json_get_string(result));
    }
    else
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
    }

finish:
    if (obj)
        tb_json_put(obj);
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

telebot_error_e telebot_copy_message(telebot_handler_t handle, long long int chat_id,
        long long int from_chat_id, int message_id, const char *caption, const char *parse_mode,
        const telebot_send_options_t *options, int *message_id_out)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    struct cJSON *obj = NULL;
    telebot_core_response_t response = telebot_core_copy_message(handle->core_h, chat_id, from_chat_id, message_id, caption, parse_mode, options);
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

        struct cJSON *ok = NULL;
        if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
        {
            ret = TELEBOT_ERROR_OPERATION_FAILED;
            goto finish;
        }

        struct cJSON *result = NULL;
        if (tb_json_get_ex(obj, "result", &result))
        {
            struct cJSON *mid_obj = NULL;
            if (tb_json_get_ex(result, "message_id", &mid_obj))
            {
                *message_id_out = tb_json_get_int(mid_obj);
            }
        }
    }

finish:
    if (obj)
        tb_json_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_copy_messages(telebot_handler_t handle, long long int chat_id,
        long long int from_chat_id, const int message_ids[], int count, bool remove_caption,
        const telebot_send_options_t *options, int **message_ids_out, int *count_out)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    struct cJSON *obj = NULL;
    telebot_core_response_t response = telebot_core_copy_messages(handle->core_h, chat_id, from_chat_id, message_ids, count, remove_caption, options);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    if (message_ids_out && count_out)
    {
        const char *rdata = telebot_core_get_response_data(response);
        obj = telebot_parser_str_to_obj(rdata);
        if (obj == NULL)
        {
            ret = TELEBOT_ERROR_OPERATION_FAILED;
            goto finish;
        }

        struct cJSON *ok = NULL;
        if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
        {
            ret = TELEBOT_ERROR_OPERATION_FAILED;
            goto finish;
        }

        struct cJSON *result = NULL;
        if (tb_json_get_ex(obj, "result", &result))
        {
            int array_len = tb_json_array_length(result);
            *count_out = array_len;
            *message_ids_out = calloc(array_len, sizeof(int));
            for (int i = 0; i < array_len; i++)
            {
                struct cJSON *mid_obj = tb_json_array_get_idx(result, i);
                (*message_ids_out)[i] = tb_json_get_int(mid_obj);
            }
        }
    }

finish:
    if (obj)
        tb_json_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_forward_messages(telebot_handler_t handle, long long int chat_id,
        long long int from_chat_id, const int message_ids[], int count,
        const telebot_send_options_t *options, int **message_ids_out, int *count_out)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    struct cJSON *obj = NULL;
    telebot_core_response_t response = telebot_core_forward_messages(handle->core_h, chat_id, from_chat_id, message_ids, count, options);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    if (message_ids_out && count_out)
    {
        const char *rdata = telebot_core_get_response_data(response);
        obj = telebot_parser_str_to_obj(rdata);
        if (obj == NULL)
        {
            ret = TELEBOT_ERROR_OPERATION_FAILED;
            goto finish;
        }

        struct cJSON *ok = NULL;
        if (!tb_json_get_ex(obj, "ok", &ok) || !tb_json_get_bool(ok))
        {
            ret = TELEBOT_ERROR_OPERATION_FAILED;
            goto finish;
        }

        struct cJSON *result = NULL;
        if (tb_json_get_ex(obj, "result", &result))
        {
            int array_len = tb_json_array_length(result);
            *count_out = array_len;
            *message_ids_out = calloc(array_len, sizeof(int));
            for (int i = 0; i < array_len; i++)
            {
                struct cJSON *mid_obj = tb_json_array_get_idx(result, i);
                (*message_ids_out)[i] = tb_json_get_int(mid_obj);
            }
        }
    }

finish:
    if (obj)
        tb_json_put(obj);
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

telebot_error_e telebot_send_sticker(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *sticker, const char *emoji, const telebot_send_options_t *options)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_send_sticker(handle->core_h, chat_id, sticker, emoji, options);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_message_reaction(telebot_handler_t handle, long long int chat_id, int message_id,
        const char *emoji, bool is_big)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_set_message_reaction(handle->core_h, chat_id, message_id, emoji, is_big);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_ban_chat_member(telebot_handler_t handle, long long int chat_id,
        long long int user_id, long until_date, bool revoke_messages)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_ban_chat_member(handle->core_h, chat_id, user_id, until_date, revoke_messages);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_approve_chat_join_request(telebot_handler_t handle, long long int chat_id,
        long long int user_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_approve_chat_join_request(handle->core_h, chat_id, user_id);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_decline_chat_join_request(telebot_handler_t handle, long long int chat_id,
        long long int user_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_decline_chat_join_request(handle->core_h, chat_id, user_id);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_sticker_set(telebot_handler_t handle, const char *name, telebot_sticker_set_t *stickers)
{
    if ((handle == NULL) || (stickers == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;
    memset(stickers, 0, sizeof(*stickers));

    telebot_error_e ret =
        telebot_call_and_parse(telebot_core_get_sticker_set(handle->core_h, name), parse_sticker_set, stickers);
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_sticker_set(stickers);
    return ret;
}

telebot_error_e telebot_put_sticker_set(telebot_sticker_set_t *stickers)
{
    if (stickers == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    TELEBOT_SAFE_FREE(stickers->name);
    TELEBOT_SAFE_FREE(stickers->title);
    TELEBOT_SAFE_FREE(stickers->sticker_type);
    for (int i = 0; i < stickers->count_stickers; i++)
        telebot_put_sticker(&stickers->stickers[i]);
    TELEBOT_SAFE_FZCNT(stickers->stickers, stickers->count_stickers);
    telebot_put_photo(stickers->thumbnail);
    TELEBOT_SAFE_FREE(stickers->thumbnail);
    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_ban_chat_sender_chat(telebot_handler_t handle, long long int chat_id,
                                             long long int sender_chat_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;
    return telebot_call(telebot_core_ban_chat_sender_chat(handle->core_h, chat_id, sender_chat_id));
}

telebot_error_e telebot_unban_chat_sender_chat(telebot_handler_t handle, long long int chat_id,
                                               long long int sender_chat_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;
    return telebot_call(telebot_core_unban_chat_sender_chat(handle->core_h, chat_id, sender_chat_id));
}

telebot_error_e telebot_create_chat_invite_link(telebot_handler_t handle, long long int chat_id, const char *name,
                                                long expire_date, int member_limit, bool creates_join_request,
                                                telebot_chat_invite_link_t *invite_link)
{
    if ((handle == NULL) || (invite_link == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;
    memset(invite_link, 0, sizeof(*invite_link));
    telebot_error_e ret = telebot_call_and_parse(
        telebot_core_create_chat_invite_link(handle->core_h, chat_id, name, expire_date, member_limit,
                                             creates_join_request),
        parse_invite_link, invite_link);
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_chat_invite_link(invite_link);
    return ret;
}

telebot_error_e telebot_edit_chat_invite_link(telebot_handler_t handle, long long int chat_id,
                                              const char *invite_link_str, const char *name, long expire_date,
                                              int member_limit, bool creates_join_request,
                                              telebot_chat_invite_link_t *invite_link)
{
    if ((handle == NULL) || (invite_link == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;
    memset(invite_link, 0, sizeof(*invite_link));
    telebot_error_e ret = telebot_call_and_parse(
        telebot_core_edit_chat_invite_link(handle->core_h, chat_id, invite_link_str, name, expire_date,
                                           member_limit, creates_join_request),
        parse_invite_link, invite_link);
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_chat_invite_link(invite_link);
    return ret;
}

telebot_error_e telebot_revoke_chat_invite_link(telebot_handler_t handle, long long int chat_id,
                                                const char *invite_link_str, telebot_chat_invite_link_t *invite_link)
{
    if ((handle == NULL) || (invite_link == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;
    memset(invite_link, 0, sizeof(*invite_link));
    telebot_error_e ret = telebot_call_and_parse(
        telebot_core_revoke_chat_invite_link(handle->core_h, chat_id, invite_link_str), parse_invite_link,
        invite_link);
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_chat_invite_link(invite_link);
    return ret;
}

telebot_error_e telebot_edit_message_media(telebot_handler_t handle, long long int chat_id, int message_id,
                                           const char *inline_message_id, const char *media,
                                           const char *reply_markup)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;
    return telebot_call(
        telebot_core_edit_message_media(handle->core_h, chat_id, message_id, inline_message_id, media, reply_markup));
}

telebot_error_e telebot_put_stickers(telebot_sticker_t *stickers, int count)
{
    if (stickers == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    for (int i = 0; i < count; i++)
        telebot_put_sticker(&stickers[i]);
    free(stickers);
    return TELEBOT_ERROR_NONE;
}

int telebot_get_retry_after(telebot_handler_t handle)
{
    return handle ? telebot_core_get_retry_after(handle->core_h) : 0;
}

telebot_error_e telebot_send_message_draft(telebot_handler_t handle, long long int chat_id,
        int message_thread_id, int draft_id, const char *text, const char *parse_mode, bool can_stop,
        bool keep_on_stop)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_send_message_draft(handle->core_h, chat_id, message_thread_id, draft_id, text, parse_mode, can_stop, keep_on_stop);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_unpin_all_chat_messages(telebot_handler_t handle, long long int chat_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_unpin_all_chat_messages(handle->core_h, chat_id);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_delete_message_reaction(telebot_handler_t handle, long long int chat_id,
        int message_id, long long int user_id, long long int actor_chat_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_delete_message_reaction(handle->core_h, chat_id, message_id, user_id, actor_chat_id);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_delete_all_message_reactions(telebot_handler_t handle, long long int chat_id,
        long long int user_id, long long int actor_chat_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_delete_all_message_reactions(handle->core_h, chat_id, user_id, actor_chat_id);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_chat_member_tag(telebot_handler_t handle, long long int chat_id,
        long long int user_id, const char *tag)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_set_chat_member_tag(handle->core_h, chat_id, user_id, tag);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_verify_user(telebot_handler_t handle, long long int user_id,
        const char *custom_description, const char *parse_mode)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_verify_user(handle->core_h, user_id, custom_description, parse_mode);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_verify_chat(telebot_handler_t handle, long long int chat_id,
        const char *custom_description, const char *parse_mode)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_verify_chat(handle->core_h, chat_id, custom_description, parse_mode);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_remove_user_verification(telebot_handler_t handle, long long int user_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_remove_user_verification(handle->core_h, user_id);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_remove_chat_verification(telebot_handler_t handle, long long int chat_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_remove_chat_verification(handle->core_h, chat_id);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}
