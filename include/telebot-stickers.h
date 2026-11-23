
/*
 * telebot
 *
 * Copyright (c) 2020 Elmurod Talipov.
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

#ifndef __TELEBOT_STICKERS_H__
#define __TELEBOT_STICKERS_H__

#include <stdbool.h>
#include "telebot-common.h"
#include "telebot-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        telebot-stickers.h
 * @ingroup     TELEBOT_API
 * @brief       This file contains stickers feature of telegram bot
 * @author      Elmurod Talipov
 * @date        2020-04-19
 */

/**
 * @addtogroup TELEBOT_API
 * @{
 */

/**
 * @brief This object represents a sticker set.
 */
typedef struct telebot_sticker_set {
    /** Sticker set name, as used in t.me/addstickers/ links */
    char *name;

    /** Sticker set title */
    char *title;

    /** Type of stickers in the set: "regular", "mask" or "custom_emoji" */
    char *sticker_type;

    /** List of all set stickers */
    telebot_sticker_t *stickers;

    /** Number of entries in stickers */
    int count_stickers;

    /** Optional. Sticker set thumbnail in .WEBP, .TGS or .WEBM format */
    struct telebot_photo *thumbnail;
} telebot_sticker_set_t;

/**
 * @brief Sends a static .WEBP, animated .TGS or video .WEBM sticker.
 *
 * Bot API method: sendSticker.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] sticker Sticker to send: a file_id or HTTP URL, a local file, or bytes in memory; see
 *     #telebot_input_file_t.
 * @param[in] emoji Optional emoji associated with an uploaded sticker, or NULL.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_sticker(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *sticker, const char *emoji, const telebot_send_options_t *options);

/**
 * @brief Get a sticker set.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] name Name of the sticker set.
 * @param[out] stickers Pointer to sticker set, must be released
 * with #telebot_put_sticker_set.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_get_sticker_set(telebot_handler_t handle, const char *name,
    telebot_sticker_set_t *stickers);

/**
 * @brief Release sticker set obtained with #telebot_get_sticker_set()
 *
 * @param[in] stickers Pointer to sticker set to be released.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_put_sticker_set(telebot_sticker_set_t *stickers);

/**
 * @brief Release a sticker object.
 *
 * @param[in] sticker Pointer to sticker to be released.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_put_sticker(telebot_sticker_t *sticker);

/**
 * @brief Releases an array of stickers, such as the one returned by
 * #telebot_get_forum_topic_icon_stickers(), including the array itself.
 *
 * @param[in] stickers Array to release.
 * @param[in] count Number of entries in @p stickers.
 * @return #TELEBOT_ERROR_NONE on success, #TELEBOT_ERROR_INVALID_PARAMETER if @p stickers is NULL.
 */
telebot_error_e telebot_put_stickers(telebot_sticker_t *stickers, int count);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_STICKERS_H__ */