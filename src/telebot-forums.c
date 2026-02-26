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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json.h>
#include <telebot-core.h>
#include <telebot-methods.h>
#include <telebot-parser.h>
#include <telebot-private.h>

telebot_error_e telebot_create_forum_topic(telebot_handler_t handle, long long int chat_id,
                                           const char *name, int icon_color, const char *icon_custom_emoji_id,
                                           telebot_forum_topic_t *topic)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;
    if (topic == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_create_forum_topic(handle->core_h, chat_id, name, icon_color, icon_custom_emoji_id);
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

    ret = telebot_parser_get_forum_topic(result, topic);

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_put_forum_topic(telebot_forum_topic_t *topic)
{
    if (topic == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    TELEBOT_SAFE_FREE(topic->name);
    TELEBOT_SAFE_FREE(topic->icon_custom_emoji_id);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_edit_forum_topic(telebot_handler_t handle, long long int chat_id, int message_thread_id,
                                         const char *name, const char *icon_custom_emoji_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_edit_forum_topic(handle->core_h, chat_id, message_thread_id, name, icon_custom_emoji_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_close_forum_topic(telebot_handler_t handle, long long int chat_id, int message_thread_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_close_forum_topic(handle->core_h, chat_id, message_thread_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_reopen_forum_topic(telebot_handler_t handle, long long int chat_id, int message_thread_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_reopen_forum_topic(handle->core_h, chat_id, message_thread_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_delete_forum_topic(telebot_handler_t handle, long long int chat_id, int message_thread_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_delete_forum_topic(handle->core_h, chat_id, message_thread_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_unpin_all_forum_topic_messages(telebot_handler_t handle, long long int chat_id, int message_thread_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_unpin_all_forum_topic_messages(handle->core_h, chat_id, message_thread_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_forum_topic_icon_stickers(telebot_handler_t handle, telebot_sticker_t **stickers, int *count)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;
    if ((stickers == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_get_forum_topic_icon_stickers(handle->core_h);
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

    ret = telebot_parser_get_stickers(result, stickers, count);

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_edit_general_forum_topic(telebot_handler_t handle, long long int chat_id, const char *name)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_edit_general_forum_topic(handle->core_h, chat_id, name);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_close_general_forum_topic(telebot_handler_t handle, long long int chat_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_close_general_forum_topic(handle->core_h, chat_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_reopen_general_forum_topic(telebot_handler_t handle, long long int chat_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_reopen_general_forum_topic(handle->core_h, chat_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_hide_general_forum_topic(telebot_handler_t handle, long long int chat_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_hide_general_forum_topic(handle->core_h, chat_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_unhide_general_forum_topic(telebot_handler_t handle, long long int chat_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_unhide_general_forum_topic(handle->core_h, chat_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_unpin_all_general_forum_topic_messages(telebot_handler_t handle, long long int chat_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_unpin_all_general_forum_topic_messages(handle->core_h, chat_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}
