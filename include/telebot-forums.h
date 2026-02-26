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

#ifndef __TELEBOT_FORUMS_H__
#define __TELEBOT_FORUMS_H__

#include <stdbool.h>
#include "telebot-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        telebot-forums.h
 * @ingroup     TELEBOT_API
 * @brief       This file contains forums feature of telegram bot
 * @author      Elmurod Talipov
 * @date        2026-02-27
 */

/**
 * @addtogroup TELEBOT_API
 * @{
 */

/**
 * @brief Use this method to create a topic in a forum supergroup chat.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] name Topic name, 1-128 characters.
 * @param[in] icon_color Color of the topic icon in RGB format.
 * @param[in] icon_custom_emoji_id Unique identifier of the custom emoji used as the topic icon.
 * @param[out] topic Pointer to get ForumTopic, MUST be released with #telebot_put_forum_topic.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_create_forum_topic(telebot_handler_t handle,
    long long int chat_id, const char *name, int icon_color, const char *icon_custom_emoji_id,
    telebot_forum_topic_t *topic);

/**
 * @brief Release forum topic obtained with #telebot_create_forum_topic.
 * @param[in] topic Pointer to forum topic to be released.
 * @return on Success, #TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_put_forum_topic(telebot_forum_topic_t *topic);

/**
 * @brief Use this method to edit name and icon of a topic in a forum supergroup chat.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] message_thread_id Unique identifier for the target message thread of the forum topic.
 * @param[in] name New topic name, 0-128 characters.
 * @param[in] icon_custom_emoji_id New unique identifier of the custom emoji used as the topic icon.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_edit_forum_topic(telebot_handler_t handle,
    long long int chat_id, int message_thread_id, const char *name, const char *icon_custom_emoji_id);

/**
 * @brief Use this method to close an open topic in a forum supergroup chat.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] message_thread_id Unique identifier for the target message thread of the forum topic.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_close_forum_topic(telebot_handler_t handle,
    long long int chat_id, int message_thread_id);

/**
 * @brief Use this method to reopen a closed topic in a forum supergroup chat.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] message_thread_id Unique identifier for the target message thread of the forum topic.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_reopen_forum_topic(telebot_handler_t handle,
    long long int chat_id, int message_thread_id);

/**
 * @brief Use this method to delete a forum topic along with all its messages in a forum supergroup chat.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] message_thread_id Unique identifier for the target message thread of the forum topic.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_delete_forum_topic(telebot_handler_t handle,
    long long int chat_id, int message_thread_id);

/**
 * @brief Use this method to unpin all messages in a forum topic.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] message_thread_id Unique identifier for the target message thread of the forum topic.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_unpin_all_forum_topic_messages(telebot_handler_t handle,
    long long int chat_id, int message_thread_id);

/**
 * @brief Use this method to get custom emoji stickers, which can be used as a forum topic icon by any user.
 * @param[in] handle The telebot handler.
 * @param[out] stickers Pointer to stickers to be obtained, MUST be released with #telebot_put_stickers.
 * @param[out] count Pointer to get the number of stickers.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_get_forum_topic_icon_stickers(telebot_handler_t handle,
    telebot_sticker_t **stickers, int *count);

/**
 * @brief Use this method to edit the name of the 'General' topic in a forum supergroup chat.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] name New topic name, 1-128 characters.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_edit_general_forum_topic(telebot_handler_t handle,
    long long int chat_id, const char *name);

/**
 * @brief Use this method to close an open 'General' topic in a forum supergroup chat.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_close_general_forum_topic(telebot_handler_t handle,
    long long int chat_id);

/**
 * @brief Use this method to reopen a closed 'General' topic in a forum supergroup chat.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_reopen_general_forum_topic(telebot_handler_t handle,
    long long int chat_id);

/**
 * @brief Use this method to hide the 'General' topic in a forum supergroup chat.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_hide_general_forum_topic(telebot_handler_t handle,
    long long int chat_id);

/**
 * @brief Use this method to unhide the 'General' topic in a forum supergroup chat.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_unhide_general_forum_topic(telebot_handler_t handle,
    long long int chat_id);

/**
 * @brief Use this method to unpin all messages in a General forum topic.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_unpin_all_general_forum_topic_messages(telebot_handler_t handle,
    long long int chat_id);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_FORUMS_H__ */
