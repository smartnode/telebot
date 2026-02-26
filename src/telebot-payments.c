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
#include <telebot-payments.h>
#include <telebot-private.h>

telebot_error_e telebot_send_invoice(telebot_handler_t handle,
                                     long long int chat_id, int message_thread_id, const char *title,
                                     const char *description, const char *payload, const char *provider_token,
                                     const char *currency, const char *prices, int max_tip_amount,
                                     const char *suggested_tip_amounts, const char *start_parameter,
                                     const char *provider_data, const char *photo_url, int photo_size,
                                     int photo_width, int photo_height, bool need_name, bool need_phone_number,
                                     bool need_email, bool need_shipping_address, bool send_phone_number_to_provider,
                                     bool send_email_to_provider, bool is_flexible, bool disable_notification,
                                     bool protect_content, const char *reply_parameters, const char *reply_markup,
                                     telebot_message_t *message)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;
    if (message == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_send_invoice(handle->core_h, chat_id, message_thread_id,
                                                                  title, description, payload, provider_token,
                                                                  currency, prices, max_tip_amount, suggested_tip_amounts,
                                                                  start_parameter, provider_data, photo_url, photo_size,
                                                                  photo_width, photo_height, need_name, need_phone_number,
                                                                  need_email, need_shipping_address, send_phone_number_to_provider,
                                                                  send_email_to_provider, is_flexible, disable_notification,
                                                                  protect_content, reply_parameters, reply_markup);
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

telebot_error_e telebot_create_invoice_link(telebot_handler_t handle,
                                            const char *title, const char *description, const char *payload,
                                            const char *provider_token, const char *currency, const char *prices,
                                            int max_tip_amount, const char *suggested_tip_amounts, const char *provider_data,
                                            const char *photo_url, int photo_size, int photo_width, int photo_height,
                                            bool need_name, bool need_phone_number, bool need_email, bool need_shipping_address,
                                            bool send_phone_number_to_provider, bool send_email_to_provider, bool is_flexible,
                                            char **invoice_link)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;
    if (invoice_link == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_create_invoice_link(handle->core_h, title, description, payload,
                                                                        provider_token, currency, prices, max_tip_amount,
                                                                        suggested_tip_amounts, provider_data, photo_url,
                                                                        photo_size, photo_width, photo_height, need_name,
                                                                        need_phone_number, need_email, need_shipping_address,
                                                                        send_phone_number_to_provider, send_email_to_provider,
                                                                        is_flexible);
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
        *invoice_link = TELEBOT_SAFE_STRDUP(json_object_get_string(result));
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

telebot_error_e telebot_answer_shipping_query(telebot_handler_t handle,
                                              const char *shipping_query_id, bool ok, const char *shipping_options,
                                              const char *error_message)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_answer_shipping_query(handle->core_h, shipping_query_id, ok, shipping_options, error_message);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_answer_pre_checkout_query(telebot_handler_t handle,
                                                  const char *pre_checkout_query_id, bool ok, const char *error_message)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_response_t response = telebot_core_answer_pre_checkout_query(handle->core_h, pre_checkout_query_id, ok, error_message);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_my_star_balance(telebot_handler_t handle, long long int *balance)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;
    if (balance == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_get_my_star_balance(handle->core_h);
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
        struct json_object *balance_obj = NULL;
        if (json_object_object_get_ex(result, "balance", &balance_obj))
        {
            *balance = json_object_get_int64(balance_obj);
        }
    }

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_star_transactions(telebot_handler_t handle,
                                               int offset, int limit, telebot_star_transactions_t *transactions)
{
    if (handle == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;
    if (transactions == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = NULL;
    telebot_core_response_t response = telebot_core_get_star_transactions(handle->core_h, offset, limit);
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

    ret = telebot_parser_get_star_transactions(result, transactions);

finish:
    if (obj)
        json_object_put(obj);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_put_star_transactions(telebot_star_transactions_t *transactions)
{
    if (transactions == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (transactions->transactions)
    {
        for (int i = 0; i < transactions->count_transactions; i++)
        {
            TELEBOT_SAFE_FREE(transactions->transactions[i].id);
            // ... free transaction partners etc if needed ...
        }
        free(transactions->transactions);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_refund_star_payment(telebot_handler_t handle,
                                             long long int user_id, const char *telegram_payment_charge_id)
{
    if (handle == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_refund_star_payment(handle->core_h, user_id, telegram_payment_charge_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_get_available_gifts(telebot_handler_t handle,
                                            telebot_gifts_t *gifts)
{
    if (handle == NULL || gifts == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_get_available_gifts(handle->core_h);
    int ret = telebot_core_get_response_code(response);
    if (ret == TELEBOT_ERROR_NONE)
    {
        struct json_object *obj = telebot_parser_str_to_obj(telebot_core_get_response_data(response));
        if (obj == NULL)
        {
            ret = TELEBOT_ERROR_OPERATION_FAILED;
        }
        else
        {
            ret = telebot_parser_get_gifts(obj, gifts);
            json_object_put(obj);
        }
    }
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_put_available_gifts(telebot_gifts_t *gifts)
{
    if (gifts == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    for (int i = 0; i < gifts->count; i++)
    {
        telebot_put_gift(&(gifts->gifts[i]));
    }
    TELEBOT_SAFE_FREE(gifts->gifts);
    gifts->count = 0;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_get_user_gifts(telebot_handler_t handle,
                                       long long int user_id, int offset, int limit, telebot_user_gifts_t *gifts)
{
    if (handle == NULL || gifts == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_get_user_gifts(handle->core_h, user_id, offset, limit);
    int ret = telebot_core_get_response_code(response);
    if (ret == TELEBOT_ERROR_NONE)
    {
        struct json_object *obj = telebot_parser_str_to_obj(telebot_core_get_response_data(response));
        if (obj == NULL)
        {
            ret = TELEBOT_ERROR_OPERATION_FAILED;
        }
        else
        {
            ret = telebot_parser_get_user_gifts(obj, gifts);
            json_object_put(obj);
        }
    }
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_put_user_gifts(telebot_user_gifts_t *gifts)
{
    if (gifts == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    for (int i = 0; i < gifts->count; i++)
    {
        telebot_put_user_gift(&(gifts->gifts[i]));
    }
    TELEBOT_SAFE_FREE(gifts->gifts);
    gifts->count = 0;
    gifts->total_count = 0;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_get_chat_gifts(telebot_handler_t handle,
                                       long long int chat_id, int offset, int limit, telebot_user_gifts_t *gifts)
{
    if (handle == NULL || gifts == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_get_chat_gifts(handle->core_h, chat_id, offset, limit);
    int ret = telebot_core_get_response_code(response);
    if (ret == TELEBOT_ERROR_NONE)
    {
        struct json_object *obj = telebot_parser_str_to_obj(telebot_core_get_response_data(response));
        if (obj == NULL)
        {
            ret = TELEBOT_ERROR_OPERATION_FAILED;
        }
        else
        {
            ret = telebot_parser_get_user_gifts(obj, gifts);
            json_object_put(obj);
        }
    }
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_put_chat_gifts(telebot_user_gifts_t *gifts)
{
    return telebot_put_user_gifts(gifts);
}

telebot_error_e telebot_upgrade_gift(telebot_handler_t handle,
                                     const char *gift_id, bool pay_for_upgrade, const char *text,
                                     const char *text_parse_mode, const char *text_entities)
{
    if (handle == NULL || gift_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_upgrade_gift(handle->core_h, gift_id, pay_for_upgrade, text, text_parse_mode, text_entities);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_transfer_gift(telebot_handler_t handle,
                                      long long int user_id, long long int chat_id, const char *gift_id)
{
    if (handle == NULL || gift_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_transfer_gift(handle->core_h, user_id, chat_id, gift_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_convert_gift_to_stars(telebot_handler_t handle,
                                              const char *gift_id)
{
    if (handle == NULL || gift_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_convert_gift_to_stars(handle->core_h, gift_id);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_send_gift(telebot_handler_t handle,
                                  long long int user_id, long long int chat_id, const char *gift_id,
                                  bool pay_for_upgrade, const char *text, const char *text_parse_mode,
                                  const char *text_entities)
{
    if (handle == NULL || gift_id == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_send_gift(handle->core_h, user_id, chat_id, gift_id, pay_for_upgrade, text, text_parse_mode, text_entities);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}

telebot_error_e telebot_gift_premium_subscription(telebot_handler_t handle,
                                                  long long int user_id, int month_count, int star_count, const char *text,
                                                  const char *text_parse_mode, const char *text_entities)
{
    if (handle == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_core_response_t response = telebot_core_gift_premium_subscription(handle->core_h, user_id, month_count, star_count, text, text_parse_mode, text_entities);
    int ret = telebot_core_get_response_code(response);
    telebot_core_put_response(response);
    return ret;
}
