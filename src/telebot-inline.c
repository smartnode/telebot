#include <json.h>
#include <telebot-core.h>
#include <telebot-methods.h>
#include <telebot-parser.h>
#include <telebot-private.h>

telebot_error_e telebot_answer_inline_query(telebot_handler_t handle,
                                            const char *inline_query_id, const char *results, int cache_time,
                                            bool is_personal, const char *next_offset, const char *button)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_answer_inline_query(handle->core_h, inline_query_id,
                                                                         results, cache_time, is_personal,
                                                                         next_offset, button);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_save_prepared_inline_message(telebot_handler_t handle,
                                                     long long int user_id, const char *result, bool allow_user_chats,
                                                     bool allow_bot_chats, bool allow_group_chats, bool allow_channel_chats,
                                                     telebot_prepared_inline_message_t *prepared_message)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;
    if (prepared_message == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_save_prepared_inline_message(handle->core_h, user_id, result,
                                                                                  allow_user_chats, allow_bot_chats,
                                                                                  allow_group_chats, allow_channel_chats);
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

    struct json_object *res_obj = NULL;
    if (!json_object_object_get_ex(obj, "result", &res_obj))
    {
        ret = TELEBOT_ERROR_OPERATION_FAILED;
        goto finish;
    }

    ret = telebot_parser_get_prepared_inline_message(res_obj, prepared_message);

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_put_prepared_inline_message(telebot_prepared_inline_message_t *prepared_message)
{
    if (prepared_message == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    TELEBOT_SAFE_FREE(prepared_message->id);
    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_answer_web_app_query(telebot_handler_t handle, const char *web_app_query_id, const char *result, char **inline_message_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_answer_web_app_query(handle->core_h, web_app_query_id, result);
    int ret = telebot_core_get_response_code(response);
    if (ret != TELEBOT_ERROR_NONE)
        goto finish;

    if (inline_message_id)
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

        struct json_object *result_obj = NULL;
        if (json_object_object_get_ex(obj, "result", &result_obj))
        {
            struct json_object *imid_obj = NULL;
            if (json_object_object_get_ex(result_obj, "inline_message_id", &imid_obj))
            {
                *inline_message_id = TELEBOT_SAFE_STRDUP(json_object_get_string(imid_obj));
            }
        }
    }

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
    return ret;
}
