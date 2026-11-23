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

#ifndef __TELEBOT_INLINE_H__
#define __TELEBOT_INLINE_H__

#include <stdbool.h>
#include "telebot-common.h"
#include "telebot-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        telebot-inline.h
 * @ingroup     TELEBOT_API
 * @brief       This file contains inline mode feature of telegram bot
 * @author      Elmurod Talipov
 * @date        2026-02-27
 */

/**
 * @addtogroup TELEBOT_API
 * @{
 */

/**
 * @brief Use this method to send answers to an inline query.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] inline_query_id Identifier of the inline query to answer.
 * @param[in] results JSON array of InlineQueryResult, at most 50.
 * @param[in] cache_time Seconds the result may be cached on the server; 0 for the default of 300.
 * @param[in] is_personal Cache the results only for the user who sent the query.
 * @param[in] next_offset Offset to pass in the next query to get more results, or NULL when there
 *     are no more.
 * @param[in] button Optional JSON InlineQueryResultsButton shown above the results, or NULL.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_answer_inline_query(telebot_handler_t handle,
    const char *inline_query_id, const char *results, int cache_time,
    bool is_personal, const char *next_offset, const char *button);

/**
 * @brief Use this method to save a prepared inline message.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] result JSON InlineQueryResult describing the message to send.
 * @param[in] allow_user_chats The message can be sent to private chats with users.
 * @param[in] allow_bot_chats The message can be sent to private chats with bots.
 * @param[in] allow_group_chats The message can be sent to group and supergroup chats.
 * @param[in] allow_channel_chats The message can be sent to channel chats.
 * @param[out] prepared_message Receives the prepared message; release it with
 *     #telebot_put_prepared_inline_message().
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_save_prepared_inline_message(telebot_handler_t handle,
    long long int user_id, const char *result, bool allow_user_chats,
    bool allow_bot_chats, bool allow_group_chats, bool allow_channel_chats,
    telebot_prepared_inline_message_t *prepared_message);

/**
 * @brief Release prepared inline message.
 *
 * @param[in] prepared_message Prepared message to release.
 * @return #TELEBOT_ERROR_NONE on success, #TELEBOT_ERROR_INVALID_PARAMETER for a NULL argument.
 */
telebot_error_e telebot_put_prepared_inline_message(telebot_prepared_inline_message_t *prepared_message);

/**
 * @brief Use this method to send answers to an inline query to a user from a Web App.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] web_app_query_id Identifier of the Web App query to answer.
 * @param[in] result JSON InlineQueryResult describing the message to send.
 * @param[out] inline_message_id Identifier of an inline message, or NULL when using @p chat_id and
 *     @p message_id.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_answer_web_app_query(telebot_handler_t handle,
    const char *web_app_query_id, const char *result, char **inline_message_id);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_INLINE_H__ */
