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
 * @brief Description of an invoice, shared by #telebot_send_invoice() and
 * #telebot_create_invoice_link().
 *
 * Zero-initialise it and set what you need; title, description, payload,
 * currency and prices are required. For payments in Telegram Stars use
 * currency "XTR" and leave provider_token NULL.
 */
typedef struct telebot_invoice_params {
    const char *title;                  /**< Product name, 1-32 characters */
    const char *description;            /**< Product description, 1-255 characters */
    const char *payload;                /**< Bot-defined payload, 1-128 bytes, not shown to the user */
    const char *provider_token;         /**< Payment provider token; NULL for Telegram Stars */
    const char *currency;               /**< Three-letter ISO 4217 code, or "XTR" for Telegram Stars */
    const char *prices;                 /**< JSON array of LabeledPrice, e.g. [{"label":"Cup","amount":50}] */
    int max_tip_amount;                 /**< Maximum tip in the smallest currency units; 0 for no tips */
    const char *suggested_tip_amounts;  /**< JSON array of up to 4 suggested tips, or NULL */
    const char *start_parameter;        /**< sendInvoice only: deep-linking parameter, or NULL */
    const char *provider_data;          /**< JSON data for the payment provider, or NULL */
    const char *photo_url;              /**< URL of a product photo, or NULL */
    int photo_size;                     /**< Photo size in bytes, or 0 */
    int photo_width;                    /**< Photo width, or 0 */
    int photo_height;                   /**< Photo height, or 0 */
    bool need_name;                     /**< Require the user's full name */
    bool need_phone_number;             /**< Require the user's phone number */
    bool need_email;                    /**< Require the user's email address */
    bool need_shipping_address;         /**< Require the user's shipping address */
    bool send_phone_number_to_provider; /**< Pass the phone number to the provider */
    bool send_email_to_provider;        /**< Pass the email address to the provider */
    bool is_flexible;                   /**< The final price depends on the shipping method */
} telebot_invoice_params_t;

/**
 * @brief Sends an invoice.
 *
 * Bot API method: sendInvoice.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] invoice What is being sold and how to pay; see #telebot_invoice_params_t.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @param[out] message Receives the sent message; release it with #telebot_put_message().
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_invoice(telebot_handler_t handle, long long int chat_id,
        const telebot_invoice_params_t *invoice, const telebot_send_options_t *options,
        telebot_message_t *message);

/**
 * @brief Creates a link for an invoice that can be shared anywhere.
 *
 * start_parameter in @p invoice is ignored.
 *
 * Bot API method: createInvoiceLink.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] invoice What is being sold and how to pay; see #telebot_invoice_params_t.
 * @param[out] invoice_link Receives the link; release it with free().
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_create_invoice_link(telebot_handler_t handle,
        const telebot_invoice_params_t *invoice, char **invoice_link);

/**
 * @brief Use this method to reply to shipping queries.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] shipping_query_id Identifier of the shipping query to answer.
 * @param[in] ok True if the order can be processed; false to reject it with @p error_message.
 * @param[in] shipping_options JSON array of available ShippingOption when @p ok is true, or NULL.
 * @param[in] error_message Reason shown to the user when @p ok is false, or NULL.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_answer_shipping_query(telebot_handler_t handle,
    const char *shipping_query_id, bool ok, const char *shipping_options,
    const char *error_message);

/**
 * @brief Use this method to respond to pre-checkout queries.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] pre_checkout_query_id Identifier of the pre-checkout query to answer.
 * @param[in] ok True if the order can be processed; false to reject it with @p error_message.
 * @param[in] error_message Reason shown to the user when @p ok is false, or NULL.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_answer_pre_checkout_query(telebot_handler_t handle,
    const char *pre_checkout_query_id, bool ok, const char *error_message);

/**
 * @brief Use this method to get the current Telegram Stars balance of the bot.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[out] balance Receives the bot's Telegram Stars balance.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_get_my_star_balance(telebot_handler_t handle, long long int *balance);

/**
 * @brief Use this method to get the bot's Telegram Star transactions.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] offset Number of entries to skip; 0 to start from the first.
 * @param[in] limit Maximum number of entries to return.
 * @param[out] transactions Receives the transactions; release them with
 *     #telebot_put_star_transactions().
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_get_star_transactions(telebot_handler_t handle,
    int offset, int limit, telebot_star_transactions_t *transactions);

/**
 * @brief Release star transactions obtained with #telebot_get_star_transactions.
 *
 * @param[in] transactions Transactions to release.
 * @return #TELEBOT_ERROR_NONE on success, #TELEBOT_ERROR_INVALID_PARAMETER for a NULL argument.
 */
telebot_error_e telebot_put_star_transactions(telebot_star_transactions_t *transactions);

/**
 * @brief Use this method to refund a successful payment in Telegram Stars.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] telegram_payment_charge_id Telegram payment identifier of the charge to refund.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_refund_star_payment(telebot_handler_t handle,
    long long int user_id, const char *telegram_payment_charge_id);

/**
 * @brief Use this method to get a list of gifts that can be sent by the bot to users.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[out] gifts Receives the gifts; release them with #telebot_put_available_gifts().
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_get_available_gifts(telebot_handler_t handle,
    telebot_gifts_t *gifts);

/**
 * @brief Release available gifts obtained with #telebot_get_available_gifts.
 *
 * @param[in] gifts Gifts to release.
 * @return #TELEBOT_ERROR_NONE on success, #TELEBOT_ERROR_INVALID_PARAMETER for a NULL argument.
 */
telebot_error_e telebot_put_available_gifts(telebot_gifts_t *gifts);

/**
 * @brief Release a gift object.
 *
 * @param[in] gift Gift to release; the struct itself is not freed.
 * @return #TELEBOT_ERROR_NONE on success, #TELEBOT_ERROR_INVALID_PARAMETER for a NULL argument.
 */
telebot_error_e telebot_put_gift(telebot_gift_t *gift);

/**
 * @brief Use this method to get a list of gifts received by a user.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] offset Number of entries to skip; 0 to start from the first.
 * @param[in] limit Maximum number of entries to return.
 * @param[out] gifts Receives the gifts; release them with #telebot_put_user_gifts().
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_get_user_gifts(telebot_handler_t handle,
    long long int user_id, int offset, int limit, telebot_user_gifts_t *gifts);

/**
 * @brief Release user gifts obtained with #telebot_get_user_gifts.
 *
 * @param[in] gifts Gifts to release.
 * @return #TELEBOT_ERROR_NONE on success, #TELEBOT_ERROR_INVALID_PARAMETER for a NULL argument.
 */
telebot_error_e telebot_put_user_gifts(telebot_user_gifts_t *gifts);

/**
 * @brief Release a user gift object.
 *
 * @param[in] gift Gift to release; the struct itself is not freed.
 * @return #TELEBOT_ERROR_NONE on success, #TELEBOT_ERROR_INVALID_PARAMETER for a NULL argument.
 */
telebot_error_e telebot_put_user_gift(telebot_user_gift_t *gift);

/**
 * @brief Use this method to get a list of gifts received by a chat.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] offset Number of entries to skip; 0 to start from the first.
 * @param[in] limit Maximum number of entries to return.
 * @param[out] gifts Receives the gifts; release them with #telebot_put_chat_gifts().
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_get_chat_gifts(telebot_handler_t handle,
    long long int chat_id, int offset, int limit, telebot_user_gifts_t *gifts);

/**
 * @brief Release chat gifts obtained with #telebot_get_chat_gifts.
 *
 * @param[in] gifts Gifts to release.
 * @return #TELEBOT_ERROR_NONE on success, #TELEBOT_ERROR_INVALID_PARAMETER for a NULL argument.
 */
telebot_error_e telebot_put_chat_gifts(telebot_user_gifts_t *gifts);

/**
 * @brief Use this method to upgrade a gift.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] gift_id Identifier of the gift.
 * @param[in] pay_for_upgrade Pay for the upgrade from the bot's balance, making it free for the
 *     receiver.
 * @param[in] text Optional text shown with the gift, 0-128 characters, or NULL.
 * @param[in] text_parse_mode Optional formatting of @p text: "MarkdownV2", "HTML" or "Markdown";
 *     NULL for plain text.
 * @param[in] text_entities Optional JSON array of MessageEntity for @p text, or NULL.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_upgrade_gift(telebot_handler_t handle,
    const char *gift_id, bool pay_for_upgrade, const char *text,
    const char *text_parse_mode, const char *text_entities);

/**
 * @brief Use this method to transfer a gift.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] gift_id Identifier of the gift.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_transfer_gift(telebot_handler_t handle,
    long long int user_id, long long int chat_id, const char *gift_id);

/**
 * @brief Use this method to convert a gift to Telegram Stars.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] gift_id Identifier of the gift.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_convert_gift_to_stars(telebot_handler_t handle,
    const char *gift_id);

/**
 * @brief Use this method to send a gift.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] gift_id Identifier of the gift.
 * @param[in] pay_for_upgrade Pay for the upgrade from the bot's balance, making it free for the
 *     receiver.
 * @param[in] text Optional text shown with the gift, 0-128 characters, or NULL.
 * @param[in] text_parse_mode Optional formatting of @p text: "MarkdownV2", "HTML" or "Markdown";
 *     NULL for plain text.
 * @param[in] text_entities Optional JSON array of MessageEntity for @p text, or NULL.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_gift(telebot_handler_t handle,
    long long int user_id, long long int chat_id, const char *gift_id,
    bool pay_for_upgrade, const char *text, const char *text_parse_mode,
    const char *text_entities);

/**
 * @brief Use this method to gift a Telegram Premium subscription to a user.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] month_count Number of months of Telegram Premium: 3, 6 or 12.
 * @param[in] star_count Number of Telegram Stars to pay: 1000 for 3 months, 1500 for 6 months, 2500
 *     for 12 months.
 * @param[in] text Optional text shown with the gift, 0-128 characters, or NULL.
 * @param[in] text_parse_mode Optional formatting of @p text: "MarkdownV2", "HTML" or "Markdown";
 *     NULL for plain text.
 * @param[in] text_entities Optional JSON array of MessageEntity for @p text, or NULL.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
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
