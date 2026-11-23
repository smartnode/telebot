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

#include "telebot-json.h"
#include <telebot-core.h>
#include <telebot-methods.h>
#include <telebot-parser.h>
#include <telebot-private.h>

telebot_error_e telebot_send_game(telebot_handler_t handle, long long int chat_id,
        const char *game_short_name, const telebot_send_options_t *options, telebot_message_t *message)
{
    if (handle == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    if (message == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct cJSON *obj = NULL;
    telebot_core_response_t response = telebot_core_send_game(handle->core_h, chat_id, game_short_name, options);
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
    ret = arena ? telebot_parser_get_message(result, message) : TELEBOT_ERROR_OUT_OF_MEMORY;
    if (ret == TELEBOT_ERROR_NONE)
        ret = telebot_owner_attach(message, arena); /* message now owns the arena */
    telebot_arena_finish(arena, ret == TELEBOT_ERROR_NONE);
    if (ret != TELEBOT_ERROR_NONE)
        memset(message, 0, sizeof(*message));

finish:
    if (obj)
        telebot_json_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_game_score(telebot_handler_t handle,
                                       long long int user_id, int score, bool force, bool disable_edit_message,
                                       long long int chat_id, int message_id, const char *inline_message_id,
                                       telebot_message_t *message)
{
    if (handle == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct cJSON *obj = NULL;
    telebot_core_response_t response = telebot_core_set_game_score(handle->core_h, user_id, score, force,
                                                                    disable_edit_message, chat_id, message_id, inline_message_id);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    if (message)
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
            /* An inline game message returns `true` instead of a message */
            if (cJSON_IsObject(result))
            {
                telebot_arena_t *arena = telebot_arena_begin();
                ret = arena ? telebot_parser_get_message(result, message) : TELEBOT_ERROR_OUT_OF_MEMORY;
                if (ret == TELEBOT_ERROR_NONE)
                    ret = telebot_owner_attach(message, arena); /* message now owns the arena */
                telebot_arena_finish(arena, ret == TELEBOT_ERROR_NONE);
                if (ret != TELEBOT_ERROR_NONE)
                    memset(message, 0, sizeof(*message));
            }
        }
    }

finish:
    if (obj)
        telebot_json_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_game_high_scores(telebot_handler_t handle,
                                             long long int user_id, long long int chat_id, int message_id, const char *inline_message_id,
                                             telebot_game_high_score_t **high_scores, int *count)
{
    if (handle == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    if ((high_scores == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct cJSON *obj = NULL;
    telebot_core_response_t response = telebot_core_get_game_high_scores(handle->core_h, user_id, chat_id, message_id, inline_message_id);
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

    int array_len = telebot_json_array_length(result);
    if (array_len <= 0)
    {
        *count = 0;
        *high_scores = NULL;
        goto finish;
    }

    /* The array and everything in it live in one arena owned by the array's hidden header */
    telebot_arena_t *arena = telebot_arena_begin();
    *high_scores = arena ? telebot_owned_calloc(array_len, sizeof(telebot_game_high_score_t)) : NULL;
    if (*high_scores != NULL)
    {
        *count = array_len;
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_game_high_score(telebot_json_array_get_idx(result, i), &((*high_scores)[i]));
    }
    telebot_arena_finish(arena, *high_scores != NULL);
    if (*high_scores == NULL)
    {
        ret = TELEBOT_ERROR_OUT_OF_MEMORY;
        goto finish;
    }

finish:
    if (obj)
        telebot_json_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_put_game_high_scores(telebot_game_high_score_t *high_scores, int count)
{
    if ((high_scores == NULL) || (count <= 0))
        return TELEBOT_ERROR_INVALID_PARAMETER;
    /* the array and everything it points to share one arena */
    return telebot_owned_release(high_scores);
}
