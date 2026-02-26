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
 */
telebot_error_e telebot_answer_inline_query(telebot_handler_t handle,
    const char *inline_query_id, const char *results, int cache_time,
    bool is_personal, const char *next_offset, const char *button);

/**
 * @brief Use this method to save a prepared inline message.
 */
telebot_error_e telebot_save_prepared_inline_message(telebot_handler_t handle,
    long long int user_id, const char *result, bool allow_user_chats,
    bool allow_bot_chats, bool allow_group_chats, bool allow_channel_chats,
    telebot_prepared_inline_message_t *prepared_message);

/**
 * @brief Release prepared inline message.
 */
telebot_error_e telebot_put_prepared_inline_message(telebot_prepared_inline_message_t *prepared_message);

/**
 * @brief Use this method to send answers to an inline query to a user from a Web App.
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
