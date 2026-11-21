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
 * @brief Sends a game.
 *
 * Bot API method: sendGame.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] game_short_name Short name of the game, set up via \@BotFather.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @param[out] message Receives the sent message; release it with #telebot_put_message().
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_game(telebot_handler_t handle, long long int chat_id,
        const char *game_short_name, const telebot_send_options_t *options, telebot_message_t *message);

/**
 * @brief Use this method to set the score of the specified user in a game.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] score New score; must be non-negative.
 * @param[in] force Allow the score to decrease, e.g. to fix a mistake or ban a cheater.
 * @param[in] disable_edit_message Do not edit the game message to show the new scoreboard.
 * @param[in] chat_id Chat of the game message, or 0 when using @p inline_message_id.
 * @param[in] message_id Identifier of the message, or 0 when using @p inline_message_id.
 * @param[in] inline_message_id Identifier of an inline message, or NULL when using @p chat_id and
 *     @p message_id.
 * @param[out] message Receives the edited message when the game message belongs to the bot; release
 *     it with #telebot_put_message().
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_set_game_score(telebot_handler_t handle,
    long long int user_id, int score, bool force, bool disable_edit_message,
    long long int chat_id, int message_id, const char *inline_message_id,
    telebot_message_t *message);

/**
 * @brief Use this method to get data for high score tables.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] chat_id Chat of the game message, or 0 when using @p inline_message_id.
 * @param[in] message_id Identifier of the message, or 0 when using @p inline_message_id.
 * @param[in] inline_message_id Identifier of an inline message, or NULL when using @p chat_id and
 *     @p message_id.
 * @param[out] high_scores Receives the scoreboard; release it with #telebot_put_game_high_scores().
 * @param[out] count Receives the number of entries in @p high_scores.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_get_game_high_scores(telebot_handler_t handle,
    long long int user_id, long long int chat_id, int message_id, const char *inline_message_id,
    telebot_game_high_score_t **high_scores, int *count);

/**
 * @brief Release game high scores obtained with #telebot_get_game_high_scores.
 *
 * @param[in] high_scores Scoreboard to release.
 * @param[in] count Number of entries in @p high_scores.
 * @return #TELEBOT_ERROR_NONE on success, #TELEBOT_ERROR_INVALID_PARAMETER for a NULL argument.
 */
telebot_error_e telebot_put_game_high_scores(telebot_game_high_score_t *high_scores, int count);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_GAMES_H__ */
