
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
 * @brief This object describes the position on faces where a mask should be
 * placed by default.
 */
typedef struct telebot_mask_position {
    /**
     * The part of the face relative to which the mask should be placed.
     * One of "forehead", "eyes", "mouth", or "chin".
     */
    char *point;

    /**
     * Shift by X-axis measured in widths of the mask scaled to the face size,
     * from left to right. For example, choosing -1.0 will place mask just to
     * the left of the default mask position.
     */
    float x_shift;

    /**
     * Shift by Y-axis measured in heights of the mask scaled to the face size,
     * from top to bottom. For example, 1.0 will place the mask just below the
     * default mask position.
     */
    float y_shift;

    /** Mask scaling coefficient. For example, 2.0 means double size. */
    float scale;
} telebot_mask_position_t;


/**
 * @brief This object represents a sticker.
 */
typedef struct telebot_sticker {
    /** Identifier for this file, which can be used to download or reuse the file */
    char *file_id;

    /**
     * Unique identifier for this file, which is supposed to be the same over time
     * and for different bots. Can't be used to download or reuse the file.
     */
    char *file_unique_id;

    /** Sticker width */
    int width;

    /** Sticker height */
    int height;

    /** True, if the sticker is animated */
    bool is_animated;

    /** Optional. Sticker thumbnail in .webp or .jpg format */
    struct telebot_photo *thumb;

    /** Optional. Emoji associated with the sticker. */
    char *emoji;

    /** Optional. Name of the sticker set to which the sticker belongs */
    char *set_name;

    /** Optional. For mask stickers, the position where the mask should be placed. */
    struct telebot_mask_position *mask_position;

    /** Optional. File size */
    int file_size;
} telebot_sticker_t;


/**
 * @brief This object represents a sticker set.
 */
typedef struct telebot_sticker_set {
    /** Sticker set name */
    char *name;

    /** Sticker set title */
    char *title;

    /** True, if the sticker is animated */
    bool is_animated;

    /** True, if the sticker set contains masks */
    bool contains_masks;

    /** List of all set stickers */
    telebot_sticker_t *stickers;

    /* Number of stickers */
    int count_stickers;

    /** Optional. Sticker set thumbnail in .webp or .jpg format */
    struct telebot_photo *thumb;

} telebot_sticker_set_t;

/**
 * @brief Send static .WEBP or animated .TGS stickers.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param[in] sticker Sticker file to send. It is either a file_id to resend
 * a sticker that is already on the Telegram servers, or a path to file.
 * @param[in] is_file False if sticker is file_id, true, if sticker is a file path.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original
 * message.
 * @param[in] reply_markup Additional interface options. An object for a custom
 * reply keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_send_sticker(telebot_handler_t handle, long long int chat_id,
        const char *sticker, bool is_file, bool disable_notification,
        int reply_to_message_id, char *reply_markup);

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
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] stickers Pointer to sticker set to be released.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_put_sticker_set(telebot_sticker_set_t *stickers);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_STICKERS_H__ */