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


/*
 * Shared tail of every "call, then parse result into a struct" API: checks the
 * response, verifies "ok", and hands "result" to `parse`. Releases the response.
 */
typedef telebot_error_e (*telebot_result_parser_t)(cJSON *result, void *out);

static telebot_error_e telebot_call_and_parse(telebot_core_response_t response, telebot_result_parser_t parse,
                                              void *out, size_t out_size)
{
    telebot_error_e ret = telebot_core_get_response_code(response);
    cJSON *root = NULL;
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    root = telebot_parser_str_to_obj(telebot_core_get_response_data(response));
    cJSON *ok = NULL, *result = NULL;
    if ((root == NULL) || !telebot_json_get_ex(root, "ok", &ok) || !telebot_json_get_bool(ok) ||
        !telebot_json_get_ex(root, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }
    telebot_arena_t *arena = telebot_arena_begin();
    ret = arena ? parse(result, out) : TELEBOT_ERROR_OUT_OF_MEMORY;
    if (ret == TELEBOT_ERROR_NONE)
        ret = telebot_owner_attach(out, arena);
    telebot_arena_finish(arena, ret == TELEBOT_ERROR_NONE);
    if (ret != TELEBOT_ERROR_NONE)
        memset(out, 0, out_size);

finish:
    telebot_json_put(root);
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

telebot_error_e telebot_create(telebot_handler_t *handle, const char *token)
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

typedef struct update_type_list
{
    const telebot_update_type_e *types;
    int count;
} update_type_list_t;

/* ["message","callback_query",...] */
static void compose_update_types(telebot_json_writer_t *w, const void *ctx)
{
    const update_type_list_t *list = ctx;
    telebot_json_write_raw(w, "[");
    for (int i = 0; i < list->count; i++)
    {
        if (i > 0)
            telebot_json_write_raw(w, ",");
        telebot_json_write_string(w, telebot_update_type_names[list->types[i]]);
    }
    telebot_json_write_raw(w, "]");
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

    const update_type_list_t list = {types, count};
    *out = telebot_json_compose(compose_update_types, &list);
    return (*out != NULL) ? TELEBOT_ERROR_NONE : TELEBOT_ERROR_OUT_OF_MEMORY;
}

telebot_error_e
telebot_get_updates(telebot_handler_t handle, int offset, int limit, int timeout,
                    const telebot_update_type_e allowed_updates[], int allowed_updates_count,
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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!telebot_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    /* The whole batch lives in one arena, owned by the array's hidden header */
    telebot_arena_t *arena = telebot_arena_begin();
    ret = arena ? telebot_parser_get_updates(result, updates, count) : TELEBOT_ERROR_OUT_OF_MEMORY;
    bool owned = (ret == TELEBOT_ERROR_NONE) && (*count > 0);
    DBG("Parsed %d updates into %zu bytes", owned ? *count : 0, telebot_arena_used(arena));
    telebot_arena_finish(arena, owned);
    if (!owned)
    {
        *updates = NULL;
        *count = 0;
    }

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
        telebot_json_put(obj);
    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_put_updates(telebot_update_t *updates, int count)
{
    if ((updates == NULL) || (count <= 0))
        return TELEBOT_ERROR_INVALID_PARAMETER;
    /* the array and everything it points to share one arena */
    return telebot_owned_release(updates);
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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!telebot_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    telebot_arena_t *arena = telebot_arena_begin();
    ret = arena ? telebot_parser_get_user(result, me) : TELEBOT_ERROR_OUT_OF_MEMORY;
    if (ret == TELEBOT_ERROR_NONE)
        ret = telebot_owner_attach(me, arena); /* me now owns the arena */
    telebot_arena_finish(arena, ret == TELEBOT_ERROR_NONE);
    if (ret != TELEBOT_ERROR_NONE)
        memset(me, 0, sizeof(*me));

finish:
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_me(me);

    if (obj)
        telebot_json_put(obj);

    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_put_me(telebot_user_t *me)
{
    /* releases everything the result points to; a copy of it is rejected */
    return telebot_owner_release(me, sizeof(*me));
}

telebot_error_e telebot_set_webhook(telebot_handler_t handle, const char *url,
                                    const telebot_input_file_t *certificate, int max_connections,
                                    const telebot_update_type_e allowed_updates[], int allowed_updates_count,
                                    bool drop_pending_updates, const char *secret_token)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    char *allowed_updates_str = NULL;
    telebot_error_e err = telebot_allowed_updates_json(allowed_updates, allowed_updates_count, &allowed_updates_str);
    if (err != TELEBOT_ERROR_NONE)
        return err;

    telebot_core_response_t response =
        telebot_core_set_webhook(handle->core_h, url, certificate, max_connections, allowed_updates_str,
                                 drop_pending_updates, secret_token);
    free(allowed_updates_str);
    return telebot_call(response);
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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!telebot_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    telebot_arena_t *arena = telebot_arena_begin();
    ret = arena ? telebot_parser_get_webhook_info(result, info) : TELEBOT_ERROR_OUT_OF_MEMORY;
    if (ret == TELEBOT_ERROR_NONE)
        ret = telebot_owner_attach(info, arena); /* info now owns the arena */
    telebot_arena_finish(arena, ret == TELEBOT_ERROR_NONE);
    if (ret != TELEBOT_ERROR_NONE)
        memset(info, 0, sizeof(*info));

finish:
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_webhook_info(info);

    if (obj)
        telebot_json_put(obj);

    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_put_webhook_info(telebot_webhook_info_t *info)
{
    /* releases everything the result points to; a copy of it is rejected */
    return telebot_owner_release(info, sizeof(*info));
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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!telebot_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    telebot_arena_t *arena = telebot_arena_begin();
    ret = arena ? telebot_parser_get_user_profile_photos(result, photos) : TELEBOT_ERROR_OUT_OF_MEMORY;
    if (ret == TELEBOT_ERROR_NONE)
        ret = telebot_owner_attach(photos, arena); /* photos now owns the arena */
    telebot_arena_finish(arena, ret == TELEBOT_ERROR_NONE);
    if (ret != TELEBOT_ERROR_NONE)
        memset(photos, 0, sizeof(*photos));

finish:
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_user_profile_photos(photos);

    if (obj)
        telebot_json_put(obj);

    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_put_user_profile_photos(telebot_user_profile_photos_t *photos)
{
    /* releases everything the result points to; a copy of it is rejected */
    return telebot_owner_release(photos, sizeof(*photos));
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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!telebot_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    telebot_arena_t *arena = telebot_arena_begin();
    ret = arena ? telebot_parser_get_user_profile_audios(result, audios) : TELEBOT_ERROR_OUT_OF_MEMORY;
    if (ret == TELEBOT_ERROR_NONE)
        ret = telebot_owner_attach(audios, arena); /* audios now owns the arena */
    telebot_arena_finish(arena, ret == TELEBOT_ERROR_NONE);
    if (ret != TELEBOT_ERROR_NONE)
        memset(audios, 0, sizeof(*audios));

finish:
    if (obj)
        telebot_json_put(obj);

    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_put_user_profile_audios(telebot_user_profile_audios_t *audios)
{
    /* releases everything the result points to; a copy of it is rejected */
    return telebot_owner_release(audios, sizeof(*audios));
}

telebot_error_e telebot_set_my_profile_photo(telebot_handler_t handle, const telebot_input_file_t *photo)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_set_my_profile_photo(handle->core_h, photo);
    telebot_error_e ret = telebot_core_get_response_code(response);
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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!telebot_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    telebot_arena_t *arena = telebot_arena_begin();
    ret = arena ? telebot_parser_get_business_connection(result, connection) : TELEBOT_ERROR_OUT_OF_MEMORY;
    if (ret == TELEBOT_ERROR_NONE)
        ret = telebot_owner_attach(connection, arena); /* connection now owns the arena */
    telebot_arena_finish(arena, ret == TELEBOT_ERROR_NONE);
    if (ret != TELEBOT_ERROR_NONE)
        memset(connection, 0, sizeof(*connection));

finish:
    if (obj)
        telebot_json_put(obj);

    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_put_business_connection(telebot_business_connection_t *connection)
{
    /* releases everything the result points to; a copy of it is rejected */
    return telebot_owner_release(connection, sizeof(*connection));
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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!telebot_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    telebot_arena_t *arena = telebot_arena_begin();
    ret = arena ? telebot_parser_get_user_chat_boosts(result, boosts) : TELEBOT_ERROR_OUT_OF_MEMORY;
    if (ret == TELEBOT_ERROR_NONE)
        ret = telebot_owner_attach(boosts, arena); /* boosts now owns the arena */
    telebot_arena_finish(arena, ret == TELEBOT_ERROR_NONE);
    if (ret != TELEBOT_ERROR_NONE)
        memset(boosts, 0, sizeof(*boosts));

finish:
    if (obj)
        telebot_json_put(obj);

    telebot_core_put_response(response);

    return ret;
}

telebot_error_e telebot_put_user_chat_boosts(telebot_user_chat_boosts_t *boosts)
{
    /* releases everything the result points to; a copy of it is rejected */
    return telebot_owner_release(boosts, sizeof(*boosts));
}

telebot_error_e telebot_download_file(telebot_handler_t handle, const char *file_id, const char *path)
{
    telebot_file_t file = {0};
    telebot_arena_t *file_arena = NULL; /* holds `file` until the download is done */
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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!telebot_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    file_arena = telebot_arena_begin();
    ret = file_arena ? telebot_parser_get_file(result, &file) : TELEBOT_ERROR_OUT_OF_MEMORY;
    telebot_arena_finish(file_arena, true);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    if (file.file_path == NULL)
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_core_download_file(handle->core_h, file.file_path, path);

finish:
    telebot_arena_destroy(file_arena);

    if (obj)
        telebot_json_put(obj);

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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (telebot_json_get_ex(obj, "result", &result))
    {
        *invite_link = TELEBOT_SAFE_STRDUP(telebot_json_get_string(result));
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
        telebot_json_put(obj);

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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!telebot_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    telebot_arena_t *arena = telebot_arena_begin();
    ret = arena ? telebot_parser_get_chat(result, chat) : TELEBOT_ERROR_OUT_OF_MEMORY;
    if (ret == TELEBOT_ERROR_NONE)
        ret = telebot_owner_attach(chat, arena); /* chat now owns the arena */
    telebot_arena_finish(arena, ret == TELEBOT_ERROR_NONE);
    if (ret != TELEBOT_ERROR_NONE)
        memset(chat, 0, sizeof(*chat));

finish:
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_chat(chat);

    if (obj)
        telebot_json_put(obj);

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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!telebot_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    telebot_arena_t *arena = telebot_arena_begin();
    ret = arena ? telebot_parser_get_chat_admins(result, admins, count) : TELEBOT_ERROR_OUT_OF_MEMORY;
    bool owned = (ret == TELEBOT_ERROR_NONE) && (*count > 0); /* the array header owns the arena */
    telebot_arena_finish(arena, owned);
    if (!owned)
    {
        *admins = NULL;
        *count = 0;
    }

finish:
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_chat_admins(*admins, *count);

    if (obj)
        telebot_json_put(obj);

    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_put_chat_admins(telebot_chat_member_t *admins, int count)
{
    if ((admins == NULL) || (count <= 0))
        return TELEBOT_ERROR_INVALID_PARAMETER;
    /* the array and everything it points to share one arena */
    return telebot_owned_release(admins);
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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (telebot_json_get_ex(obj, "result", &result))
        *count = telebot_json_get_int(result);
    else
        ret = TELEBOT_ERROR_OPERATION_FAILED;

finish:
    if (obj)
        telebot_json_put(obj);

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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!telebot_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    telebot_arena_t *arena = telebot_arena_begin();
    ret = arena ? telebot_parser_get_chat_member(result, member) : TELEBOT_ERROR_OUT_OF_MEMORY;
    if (ret == TELEBOT_ERROR_NONE)
        ret = telebot_owner_attach(member, arena); /* member now owns the arena */
    telebot_arena_finish(arena, ret == TELEBOT_ERROR_NONE);
    if (ret != TELEBOT_ERROR_NONE)
        memset(member, 0, sizeof(*member));

finish:
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_chat_member(member);

    if (obj)
        telebot_json_put(obj);

    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_put_chat_member(telebot_chat_member_t *member)
{
    /* releases everything the result points to; a copy of it is rejected */
    return telebot_owner_release(member, sizeof(*member));
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

typedef struct command_list
{
    const telebot_bot_command_t *commands;
    int count;
} command_list_t;

/* [{"command":...,"description":...},...] */
static void compose_commands(telebot_json_writer_t *w, const void *ctx)
{
    const command_list_t *list = ctx;
    telebot_json_write_raw(w, "[");
    for (int i = 0; i < list->count; i++)
    {
        telebot_json_write_raw(w, (i > 0) ? ",{\"command\":" : "{\"command\":");
        telebot_json_write_string(w, list->commands[i].command);
        telebot_json_write_raw(w, ",\"description\":");
        telebot_json_write_string(w, list->commands[i].description);
        telebot_json_write_raw(w, "}");
    }
    telebot_json_write_raw(w, "]");
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

    const command_list_t list = {commands, count};
    char *array_options = telebot_json_compose(compose_commands, &list);
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

    *commands = NULL;
    *count = 0;

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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (!telebot_json_get_ex(obj, "result", &result))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    telebot_arena_t *arena = telebot_arena_begin();
    ret = arena ? telebot_parser_get_array_bot_command(result, commands, count) : TELEBOT_ERROR_OUT_OF_MEMORY;
    bool owned = (ret == TELEBOT_ERROR_NONE) && (*count > 0); /* the array header owns the arena */
    telebot_arena_finish(arena, owned);
    if (!owned)
    {
        *commands = NULL;
        *count = 0;
    }

finish:
    if (ret)
        telebot_put_my_commands(*commands, *count);
    if (obj)
        telebot_json_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_put_my_commands(telebot_bot_command_t *commands, int count)
{
    if ((commands == NULL) || (count <= 0))
        return TELEBOT_ERROR_INVALID_PARAMETER;
    /* the array and everything it points to share one arena */
    return telebot_owned_release(commands);
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
    /* releases everything the result points to; a copy of it is rejected */
    return telebot_owner_release(user, sizeof(*user));
}


telebot_error_e telebot_put_chat(telebot_chat_t *chat)
{
    /* releases everything the result points to; a copy of it is rejected */
    return telebot_owner_release(chat, sizeof(*chat));
}

/* ---- Release helpers for message sub-objects (every pointer field, NULL-safe) ---- */

telebot_error_e telebot_put_message(telebot_message_t *msg)
{
    /* releases everything the result points to; a copy of it is rejected */
    return telebot_owner_release(msg, sizeof(*msg));
}

telebot_error_e telebot_put_sticker(telebot_sticker_t *sticker)
{
    /* releases everything the result points to; a copy of it is rejected */
    return telebot_owner_release(sticker, sizeof(*sticker));
}

telebot_error_e telebot_put_gift(telebot_gift_t *gift)
{
    /* releases everything the result points to; a copy of it is rejected */
    return telebot_owner_release(gift, sizeof(*gift));
}

telebot_error_e telebot_put_user_gift(telebot_user_gift_t *user_gift)
{
    /* releases everything the result points to; a copy of it is rejected */
    return telebot_owner_release(user_gift, sizeof(*user_gift));
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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (telebot_json_get_ex(obj, "result", &result))
    {
        struct cJSON *name_obj = NULL;
        if (telebot_json_get_ex(result, "name", &name_obj))
        {
            *name = TELEBOT_SAFE_STRDUP(telebot_json_get_string(name_obj));
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
        telebot_json_put(obj);
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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (telebot_json_get_ex(obj, "result", &result))
    {
        struct cJSON *desc_obj = NULL;
        if (telebot_json_get_ex(result, "description", &desc_obj))
        {
            *description = TELEBOT_SAFE_STRDUP(telebot_json_get_string(desc_obj));
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
        telebot_json_put(obj);
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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (telebot_json_get_ex(obj, "result", &result))
    {
        struct cJSON *desc_obj = NULL;
        if (telebot_json_get_ex(result, "short_description", &desc_obj))
        {
            *short_description = TELEBOT_SAFE_STRDUP(telebot_json_get_string(desc_obj));
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
        telebot_json_put(obj);
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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (telebot_json_get_ex(obj, "result", &result))
    {
        *menu_button = TELEBOT_SAFE_STRDUP(telebot_json_get_string(result));
    }
    else
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
    }

finish:
    if (obj)
        telebot_json_put(obj);
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
    if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    struct cJSON *result = NULL;
    if (telebot_json_get_ex(obj, "result", &result))
    {
        *rights = TELEBOT_SAFE_STRDUP(telebot_json_get_string(result));
    }
    else
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
    }

finish:
    if (obj)
        telebot_json_put(obj);
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
        if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
        {
            ret = TELEBOT_ERROR_OPERATION_FAILED;
            goto finish;
        }

        struct cJSON *result = NULL;
        if (telebot_json_get_ex(obj, "result", &result))
        {
            struct cJSON *mid_obj = NULL;
            if (telebot_json_get_ex(result, "message_id", &mid_obj))
            {
                *message_id_out = telebot_json_get_int(mid_obj);
            }
        }
    }

finish:
    if (obj)
        telebot_json_put(obj);
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
        if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
        {
            ret = TELEBOT_ERROR_OPERATION_FAILED;
            goto finish;
        }

        struct cJSON *result = NULL;
        if (telebot_json_get_ex(obj, "result", &result))
        {
            int array_len = telebot_json_array_length(result);
            *count_out = array_len;
            *message_ids_out = calloc(array_len, sizeof(int));
            for (int i = 0; i < array_len; i++)
            {
                struct cJSON *mid_obj = telebot_json_array_get_idx(result, i);
                (*message_ids_out)[i] = telebot_json_get_int(mid_obj);
            }
        }
    }

finish:
    if (obj)
        telebot_json_put(obj);
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
        if (!telebot_json_get_ex(obj, "ok", &ok) || !telebot_json_get_bool(ok))
        {
            ret = TELEBOT_ERROR_OPERATION_FAILED;
            goto finish;
        }

        struct cJSON *result = NULL;
        if (telebot_json_get_ex(obj, "result", &result))
        {
            int array_len = telebot_json_array_length(result);
            *count_out = array_len;
            *message_ids_out = calloc(array_len, sizeof(int));
            for (int i = 0; i < array_len; i++)
            {
                struct cJSON *mid_obj = telebot_json_array_get_idx(result, i);
                (*message_ids_out)[i] = telebot_json_get_int(mid_obj);
            }
        }
    }

finish:
    if (obj)
        telebot_json_put(obj);
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

telebot_error_e telebot_put_chat_invite_link(telebot_chat_invite_link_t *invite_link)
{
    /* releases everything the result points to; a copy of it is rejected */
    return telebot_owner_release(invite_link, sizeof(*invite_link));
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
        telebot_call_and_parse(telebot_core_get_sticker_set(handle->core_h, name), parse_sticker_set, stickers, sizeof(*stickers));
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_sticker_set(stickers);
    return ret;
}

telebot_error_e telebot_put_sticker_set(telebot_sticker_set_t *stickers)
{
    /* releases everything the result points to; a copy of it is rejected */
    return telebot_owner_release(stickers, sizeof(*stickers));
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
        parse_invite_link, invite_link, sizeof(*invite_link));
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
        parse_invite_link, invite_link, sizeof(*invite_link));
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
        telebot_core_revoke_chat_invite_link(handle->core_h, chat_id, invite_link_str), parse_invite_link, invite_link, sizeof(*invite_link));
    if (ret != TELEBOT_ERROR_NONE)
        telebot_put_chat_invite_link(invite_link);
    return ret;
}

telebot_error_e telebot_edit_message_media(telebot_handler_t handle, long long int chat_id, int message_id,
        const char *inline_message_id, const telebot_input_media_t *media, const char *reply_markup)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_edit_message_media(handle->core_h, chat_id, message_id, inline_message_id, media, reply_markup);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_put_stickers(telebot_sticker_t *stickers, int count)
{
    if ((stickers == NULL) || (count <= 0))
        return TELEBOT_ERROR_INVALID_PARAMETER;
    /* the array and everything it points to share one arena */
    return telebot_owned_release(stickers);
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

telebot_error_e telebot_answer_widget_query(telebot_handler_t handle, const char *widget_query_id,
        const char *data)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_answer_widget_query(handle->core_h, widget_query_id, data);
    telebot_error_e ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}
