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

#ifndef __TELEBOT_PAYMENTS_H__
#define __TELEBOT_PAYMENTS_H__

#include <stdbool.h>
#include "telebot-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        telebot-payments.h
 * @ingroup     TELEBOT_API
 * @brief       This file contains payments feature of telegram bot
 * @author      Elmurod Talipov
 * @date        2026-02-27
 */

/**
 * @addtogroup TELEBOT_API
 * @{
 */

/**
 * @brief Use this method to send an invoice.
 */
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
    telebot_message_t *message);

/**
 * @brief Use this method to create a link for an invoice.
 */
telebot_error_e telebot_create_invoice_link(telebot_handler_t handle,
    const char *title, const char *description, const char *payload,
    const char *provider_token, const char *currency, const char *prices,
    int max_tip_amount, const char *suggested_tip_amounts, const char *provider_data,
    const char *photo_url, int photo_size, int photo_width, int photo_height,
    bool need_name, bool need_phone_number, bool need_email, bool need_shipping_address,
    bool send_phone_number_to_provider, bool send_email_to_provider, bool is_flexible,
    char **invoice_link);

/**
 * @brief Use this method to reply to shipping queries.
 */
telebot_error_e telebot_answer_shipping_query(telebot_handler_t handle,
    const char *shipping_query_id, bool ok, const char *shipping_options,
    const char *error_message);

/**
 * @brief Use this method to respond to pre-checkout queries.
 */
telebot_error_e telebot_answer_pre_checkout_query(telebot_handler_t handle,
    const char *pre_checkout_query_id, bool ok, const char *error_message);

/**
 * @brief Use this method to get the current Telegram Stars balance of the bot.
 */
telebot_error_e telebot_get_my_star_balance(telebot_handler_t handle, long long int *balance);

/**
 * @brief Use this method to get the bot's Telegram Star transactions.
 */
telebot_error_e telebot_get_star_transactions(telebot_handler_t handle,
    int offset, int limit, telebot_star_transactions_t *transactions);

/**
 * @brief Release star transactions obtained with #telebot_get_star_transactions.
 */
telebot_error_e telebot_put_star_transactions(telebot_star_transactions_t *transactions);

/**
 * @brief Use this method to refund a successful payment in Telegram Stars.
 */
telebot_error_e telebot_refund_star_payment(telebot_handler_t handle,
    long long int user_id, const char *telegram_payment_charge_id);

/**
 * @brief Use this method to get a list of gifts that can be sent by the bot to users.
 */
telebot_error_e telebot_get_available_gifts(telebot_handler_t handle,
    telebot_gifts_t *gifts);

/**
 * @brief Release available gifts obtained with #telebot_get_available_gifts.
 */
telebot_error_e telebot_put_available_gifts(telebot_gifts_t *gifts);

/**
 * @brief Release a gift object.
 */
telebot_error_e telebot_put_gift(telebot_gift_t *gift);

/**
 * @brief Use this method to get a list of gifts received by a user.
 */
telebot_error_e telebot_get_user_gifts(telebot_handler_t handle,
    long long int user_id, int offset, int limit, telebot_user_gifts_t *gifts);

/**
 * @brief Release user gifts obtained with #telebot_get_user_gifts.
 */
telebot_error_e telebot_put_user_gifts(telebot_user_gifts_t *gifts);

/**
 * @brief Release a user gift object.
 */
telebot_error_e telebot_put_user_gift(telebot_user_gift_t *gift);

/**
 * @brief Use this method to get a list of gifts received by a chat.
 */
telebot_error_e telebot_get_chat_gifts(telebot_handler_t handle,
    long long int chat_id, int offset, int limit, telebot_user_gifts_t *gifts);

/**
 * @brief Release chat gifts obtained with #telebot_get_chat_gifts.
 */
telebot_error_e telebot_put_chat_gifts(telebot_user_gifts_t *gifts);

/**
 * @brief Use this method to upgrade a gift.
 */
telebot_error_e telebot_upgrade_gift(telebot_handler_t handle,
    const char *gift_id, bool pay_for_upgrade, const char *text,
    const char *text_parse_mode, const char *text_entities);

/**
 * @brief Use this method to transfer a gift.
 */
telebot_error_e telebot_transfer_gift(telebot_handler_t handle,
    long long int user_id, long long int chat_id, const char *gift_id);

/**
 * @brief Use this method to convert a gift to Telegram Stars.
 */
telebot_error_e telebot_convert_gift_to_stars(telebot_handler_t handle,
    const char *gift_id);

/**
 * @brief Use this method to send a gift.
 */
telebot_error_e telebot_send_gift(telebot_handler_t handle,
    long long int user_id, long long int chat_id, const char *gift_id,
    bool pay_for_upgrade, const char *text, const char *text_parse_mode,
    const char *text_entities);

/**
 * @brief Use this method to gift a Telegram Premium subscription to a user.
 */
telebot_error_e telebot_gift_premium_subscription(telebot_handler_t handle,
    long long int user_id, int month_count, int star_count, const char *text,
    const char *text_parse_mode, const char *text_entities);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_PAYMENTS_H__ */
