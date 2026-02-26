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

#include <json.h>
#include <telebot-core.h>
#include <telebot-methods.h>
#include <telebot-parser.h>
#include <telebot-private.h>

telebot_error_e telebot_send_game(telebot_handler_t handle,
                                  long long int chat_id, int message_thread_id, const char *game_short_name,
                                  bool disable_notification, bool protect_content, const char *reply_parameters,
                                  const char *reply_markup, telebot_message_t *message)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;
    if (message == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_send_game(handle->core_h, chat_id, message_thread_id,
                                                               game_short_name, disable_notification, protect_content,
                                                               reply_parameters, reply_markup);
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

    ret = telebot_parser_get_message(result, message);

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_set_game_score(telebot_handler_t handle,
                                       long long int user_id, int score, bool force, bool disable_edit_message,
                                       long long int chat_id, int message_id, const char *inline_message_id,
                                       telebot_message_t *message)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    struct json_object *obj = NULL;
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

        struct json_object *ok = NULL;
        if (!json_object_object_get_ex(obj, "ok", &ok) || !json_object_get_boolean(ok))
        {
            ret = TELEBOT_ERROR_OPERATION_FAILED;
            goto finish;
        }

        struct json_object *result = NULL;
        if (json_object_object_get_ex(obj, "result", &result))
        {
            if (json_object_get_type(result) == json_type_object)
                ret = telebot_parser_get_message(result, message);
        }
    }

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_game_high_scores(telebot_handler_t handle,
                                             long long int user_id, long long int chat_id, int message_id, const char *inline_message_id,
                                             telebot_game_high_score_t **high_scores, int *count)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;
    if ((high_scores == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
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

    int array_len = json_object_array_length(result);
    if (array_len <= 0)
    {
        *count = 0;
        *high_scores = NULL;
        goto finish;
    }

    *high_scores = calloc(array_len, sizeof(telebot_game_high_score_t));
    if (*high_scores == NULL)
    {
        ret = TELEBOT_ERROR_OUT_OF_MEMORY;
        goto finish;
    }
    *count = array_len;

    for (int i = 0; i < array_len; i++)
    {
        telebot_parser_get_game_high_score(json_object_array_get_idx(result, i), &((*high_scores)[i]));
    }

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_put_game_high_scores(telebot_game_high_score_t *high_scores, int count)
{
    if (high_scores == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    for (int i = 0; i < count; i++)
    {
        telebot_put_user(high_scores[i].user);
        TELEBOT_SAFE_FREE(high_scores[i].user);
    }
    free(high_scores);

    return TELEBOT_ERROR_NONE;
}
