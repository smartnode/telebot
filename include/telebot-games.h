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

#ifndef __TELEBOT_GAMES_H__
#define __TELEBOT_GAMES_H__

#include <stdbool.h>
#include "telebot-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        telebot-games.h
 * @ingroup     TELEBOT_API
 * @brief       This file contains games feature of telegram bot
 * @author      Elmurod Talipov
 * @date        2026-02-27
 */

/**
 * @addtogroup TELEBOT_API
 * @{
 */

/**
 * @brief Use this method to send a game.
 */
telebot_error_e telebot_send_game(telebot_handler_t handle,
    long long int chat_id, int message_thread_id, const char *game_short_name,
    bool disable_notification, bool protect_content, const char *reply_parameters,
    const char *reply_markup, telebot_message_t *message);

/**
 * @brief Use this method to set the score of the specified user in a game.
 */
telebot_error_e telebot_set_game_score(telebot_handler_t handle,
    long long int user_id, int score, bool force, bool disable_edit_message,
    long long int chat_id, int message_id, const char *inline_message_id,
    telebot_message_t *message);

/**
 * @brief Use this method to get data for high score tables.
 */
telebot_error_e telebot_get_game_high_scores(telebot_handler_t handle,
    long long int user_id, long long int chat_id, int message_id, const char *inline_message_id,
    telebot_game_high_score_t **high_scores, int *count);

/**
 * @brief Release game high scores obtained with #telebot_get_game_high_scores.
 */
telebot_error_e telebot_put_game_high_scores(telebot_game_high_score_t *high_scores, int count);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_GAMES_H__ */
