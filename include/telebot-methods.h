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

#ifndef __TELEBOT_METHODS_H__
#define __TELEBOT_METHODS_H__

#include <stdbool.h>
#include <stddef.h>
#include "telebot-common.h"
#include "telebot-types.h"
#include "telebot-core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        telebot-methods.h
 * @ingroup     TELEBOT_API
 * @brief       This file contains methods used to create telegram bot
 * @author      Elmurod Talipov
 * @date        2020-04-19
 */

/**
 * @addtogroup TELEBOT_API
 * @{
 */

/**
 * @brief Initial function to use telebot APIs.
 *
 * This function must be used first to call, and it creates handler. This call
 * MUST have corresponding call to #telebot_destroy() when operation is complete.
 *
 * @param[out] handle Pointer to create telebot handler to use APIs, it must be
 * destroyed with #telebot_destroy().
 * @param[in] token Telegram Bot token to use.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_create(telebot_handler_t *handle, const char *token);

/**
 * @brief Final function to use telebo APIs
 *
 * This function must be the last function to call for a telebot use.
 * It is the opposite of the #telebot_create() function.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_destroy(telebot_handler_t handle);

/**
 * @brief Set proxy address to use telebot behind proxy.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] addr Proxy address to use.
 * @param[in] auth Proxy authorization information.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_set_proxy(telebot_handler_t handle, char *addr, char *auth);

/**
 * @brief Get proxy address currently used.
 *
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[out] addr Current proxy address or NULL, MUST be freed after use.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_get_proxy(telebot_handler_t handle, char **addr);

/**
 * @brief This function is used to get latest updates.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] offset Identifier of the first update to be returned. The
 * negative offset can be specified to retrieve updates starting from -offset
 * update from the end of the updates queue.
 * @param[in] limit Number of updates to be retrieved. Values between 1-100
 * are accepted. Defaults to 100.
 * @param[in] timeout Timeout in seconds for long polling.
 * Defaults to 0, i.e. usual short polling. Should be positive,
 * short polling should be used for testing purposes only.
 * @param[in] allowed_updates An array of types of updates you want your bot to receive.
 * Refers to #telebot_update_type_e.
 * @param[in] allowed_updates_count Number of update types.
 * @param[out] updates An array of update objects, it needs to be released with
 * #telebot_put_updates after use.
 * @param[out] count Number of updates received.
 * @return On success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_get_updates(telebot_handler_t handle, int offset,
        int limit, int timeout, const telebot_update_type_e allowed_updates[],
        int allowed_updates_count, telebot_update_t **updates, int *count);

/**
 * @brief This function is used to release memory used for obtained updates.
 *
 * @param[in] updates Updates to release, as returned by #telebot_get_updates().
 * @param[in] count Number of entries in @p updates.
 * @return #TELEBOT_ERROR_NONE on success, #TELEBOT_ERROR_INVALID_PARAMETER for a NULL argument.
 */
telebot_error_e telebot_put_updates(telebot_update_t *updates, int count);

/**
 * @brief Releases the members of a message returned by the library.
 *
 * Use it for messages returned by functions such as #telebot_send_game(),
 * #telebot_send_invoice() and #telebot_set_game_score(). The struct itself is
 * not freed, so it may live on the stack.
 *
 * @param[in] message The message to release.
 * @return #TELEBOT_ERROR_NONE on success, #TELEBOT_ERROR_INVALID_PARAMETER if @p message is NULL.
 */
telebot_error_e telebot_put_message(telebot_message_t *message);

/**
 * @brief Seconds to wait after a call returned #TELEBOT_ERROR_RATE_LIMITED.
 *
 * Telegram limits how fast a bot may send (roughly one message per second per
 * chat, 20 per minute in a group). When a call returns
 * #TELEBOT_ERROR_RATE_LIMITED, wait this long before repeating it:
 * @code
 * if (telebot_send_message(bot, chat_id, text, NULL, false, NULL) == TELEBOT_ERROR_RATE_LIMITED)
 *     sleep(telebot_get_retry_after(bot));
 * @endcode
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @return The retry_after value of the most recent call, or 0 if it was not rate-limited.
 */
int telebot_get_retry_after(telebot_handler_t handle);


/**
 * @brief Sets a URL that receives updates through an HTTPS POST request.
 *
 * While a webhook is set, #telebot_get_updates() does not work. Use @p secret_token and check the
 *     X-Telegram-Bot-Api-Secret-Token header to make sure a request really comes from Telegram.
 *
 * Bot API method: setWebhook.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] url HTTPS URL that receives the updates; an empty string removes the webhook.
 * @param[in] certificate Optional public key of a self-signed certificate, uploaded from a path or
 *     memory, or NULL.
 * @param[in] max_connections Maximum simultaneous HTTPS connections, 1-100; 0 for the default of
 *     40.
 * @param[in] allowed_updates Update types to receive, or NULL for all.
 * @param[in] allowed_updates_count Number of entries in @p allowed_updates; 0 for all types.
 * @param[in] drop_pending_updates Drop all updates waiting to be delivered.
 * @param[in] secret_token Optional secret, 1-256 characters of A-Z, a-z, 0-9, _ and -, sent back in
 *     every request; or NULL.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_set_webhook(telebot_handler_t handle, const char *url,
        const telebot_input_file_t *certificate, int max_connections,
        const telebot_update_type_e allowed_updates[], int allowed_updates_count, bool drop_pending_updates,
        const char *secret_token);

/**
 * @brief This function is used to remove webhook integration if you decide to
 * switch back to getUpdates.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_delete_webhook(telebot_handler_t handle);


/**
 * @brief This function is used to get information about telegram bot itself.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[out] info Telegram webhoook information, it needs to be released with
 * #telebot_put_webhook_info after use.
 * @return On success, #TELEBOT_ERROR_NONE is returned, and webhook information is
 * stored in input parameter.
 */
telebot_error_e telebot_get_webhook_info(telebot_handler_t handle,
        telebot_webhook_info_t *info);

/**
 * @brief This function is used to release memory used for obtained information
 * about telegram bot itself.
 *
 * @param[in] info Telegram webhook information object obtained with
 * #telebot_get_webhook_info.
 * @return on Success, #TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_put_webhook_info(telebot_webhook_info_t *info);

/**
 * @brief This function is used to get information about telegram bot itself.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[out] me Telegram user object, it needs to be released with
 * #telebot_put_me after use.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, and user object is
 * stored in input parameter.
 */
telebot_error_e telebot_get_me(telebot_handler_t handle, telebot_user_t *me);

/**
 * @brief This function is used to release memory used for obtained information
 * about telegram bot itself.
 *
 * @param[in] me Pointer to telegram user object obtained with #telebot_get_me.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 */
telebot_error_e telebot_put_me(telebot_user_t *me);

/**
 * @brief This function is used to release memory used for user object.
 *
 * @param[in] user Pointer to telegram user object.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 */
telebot_error_e telebot_put_user(telebot_user_t *user);

/**
 * @brief Sends a text message.
 *
 * Bot API method: sendMessage.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] text Text of the message, 1-4096 characters after entity parsing.
 * @param[in] parse_mode Optional formatting: "MarkdownV2", "HTML" or "Markdown"; NULL for plain
 *     text.
 * @param[in] disable_link_preview Do not show a preview for links in the text.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_message(telebot_handler_t handle, long long int chat_id, const char *text,
        const char *parse_mode, bool disable_link_preview, const telebot_send_options_t *options);

/**
 * @brief Forwards a message of any kind.
 *
 * Only message_thread_id, disable_notification, protect_content and message_effect_id are taken
 *     from @p options.
 *
 * Bot API method: forwardMessage.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] from_chat_id Chat where the original message was sent.
 * @param[in] message_id Identifier of the message in @p from_chat_id.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_forward_message(telebot_handler_t handle, long long int chat_id,
        long long int from_chat_id, int message_id, const telebot_send_options_t *options);

/**
 * @brief Sends a photo.
 *
 * Upload limit is 10 MB; width plus height must not exceed 10000 and the aspect ratio must be at
 *     most 20.
 *
 * Bot API method: sendPhoto.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] photo Photo to send: a file_id or HTTP URL, a local file, or bytes in memory; see
 *     #telebot_input_file_t.
 * @param[in] caption Optional caption, 0-1024 characters after entity parsing, or NULL.
 * @param[in] parse_mode Optional formatting of the caption: "MarkdownV2", "HTML" or "Markdown";
 *     NULL for plain text.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_photo(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *photo, const char *caption, const char *parse_mode,
        const telebot_send_options_t *options);


/**
 * @brief Sends an audio file to be shown in the music player.
 *
 * Use MP3 or M4A, up to 50 MB. For voice messages use #telebot_send_voice().
 *
 * Bot API method: sendAudio.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] audio Audio file to send: a file_id or HTTP URL, a local file, or bytes in memory; see
 *     #telebot_input_file_t.
 * @param[in] caption Optional caption, 0-1024 characters after entity parsing, or NULL.
 * @param[in] parse_mode Optional formatting of the caption: "MarkdownV2", "HTML" or "Markdown";
 *     NULL for plain text.
 * @param[in] duration Duration in seconds, or 0 if unknown.
 * @param[in] performer Optional performer, or NULL.
 * @param[in] title Optional track name, or NULL.
 * @param[in] thumbnail Optional thumbnail (JPEG, under 200 kB, at most 320x320), uploaded from a
 *     path or memory, or NULL. A file_id or URL is rejected because Telegram never reuses
 *     thumbnails.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_audio(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *audio, const char *caption, const char *parse_mode, int duration,
        const char *performer, const char *title, const telebot_input_file_t *thumbnail,
        const telebot_send_options_t *options);

/**
 * @brief Sends a general file.
 *
 * Files of any type, up to 50 MB.
 *
 * Bot API method: sendDocument.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] document File to send: a file_id or HTTP URL, a local file, or bytes in memory; see
 *     #telebot_input_file_t.
 * @param[in] thumbnail Optional thumbnail (JPEG, under 200 kB, at most 320x320), uploaded from a
 *     path or memory, or NULL. A file_id or URL is rejected because Telegram never reuses
 *     thumbnails.
 * @param[in] caption Optional caption, 0-1024 characters after entity parsing, or NULL.
 * @param[in] parse_mode Optional formatting of the caption: "MarkdownV2", "HTML" or "Markdown";
 *     NULL for plain text.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_document(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *document, const telebot_input_file_t *thumbnail, const char *caption,
        const char *parse_mode, const telebot_send_options_t *options);

/**
 * @brief Sends an MPEG4 video.
 *
 * Bot API method: sendVideo.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] video Video to send: a file_id or HTTP URL, a local file, or bytes in memory; see
 *     #telebot_input_file_t.
 * @param[in] duration Duration in seconds, or 0 if unknown.
 * @param[in] width Width in pixels, or 0 if unknown.
 * @param[in] height Height in pixels, or 0 if unknown.
 * @param[in] thumbnail Optional thumbnail (JPEG, under 200 kB, at most 320x320), uploaded from a
 *     path or memory, or NULL. A file_id or URL is rejected because Telegram never reuses
 *     thumbnails.
 * @param[in] caption Optional caption, 0-1024 characters after entity parsing, or NULL.
 * @param[in] parse_mode Optional formatting of the caption: "MarkdownV2", "HTML" or "Markdown";
 *     NULL for plain text.
 * @param[in] supports_streaming The video is suitable for streaming.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_video(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *video, int duration, int width, int height,
        const telebot_input_file_t *thumbnail, const char *caption, const char *parse_mode,
        bool supports_streaming, const telebot_send_options_t *options);

/**
 * @brief Sends a GIF or an H.264/MPEG-4 AVC video without sound.
 *
 * Bot API method: sendAnimation.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] animation Animation to send: a file_id or HTTP URL, a local file, or bytes in memory;
 *     see #telebot_input_file_t.
 * @param[in] duration Duration in seconds, or 0 if unknown.
 * @param[in] width Width in pixels, or 0 if unknown.
 * @param[in] height Height in pixels, or 0 if unknown.
 * @param[in] thumbnail Optional thumbnail (JPEG, under 200 kB, at most 320x320), uploaded from a
 *     path or memory, or NULL. A file_id or URL is rejected because Telegram never reuses
 *     thumbnails.
 * @param[in] caption Optional caption, 0-1024 characters after entity parsing, or NULL.
 * @param[in] parse_mode Optional formatting of the caption: "MarkdownV2", "HTML" or "Markdown";
 *     NULL for plain text.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_animation(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *animation, int duration, int width, int height,
        const telebot_input_file_t *thumbnail, const char *caption, const char *parse_mode,
        const telebot_send_options_t *options);


/**
 * @brief Sends a voice message.
 *
 * Use OGG encoded with OPUS, MP3 or M4A, up to 50 MB.
 *
 * Bot API method: sendVoice.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] voice Audio file to send: a file_id or HTTP URL, a local file, or bytes in memory; see
 *     #telebot_input_file_t.
 * @param[in] caption Optional caption, 0-1024 characters after entity parsing, or NULL.
 * @param[in] parse_mode Optional formatting of the caption: "MarkdownV2", "HTML" or "Markdown";
 *     NULL for plain text.
 * @param[in] duration Duration in seconds, or 0 if unknown.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_voice(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *voice, const char *caption, const char *parse_mode, int duration,
        const telebot_send_options_t *options);

/**
 * @brief Sends a rounded square video message of up to 1 minute.
 *
 * Sending video notes by URL is not supported by Telegram.
 *
 * Bot API method: sendVideoNote.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] video_note Video note to send: a file_id or HTTP URL, a local file, or bytes in
 *     memory; see #telebot_input_file_t.
 * @param[in] duration Duration in seconds, or 0 if unknown.
 * @param[in] length Video width and height (the diameter), or 0 if unknown.
 * @param[in] thumbnail Optional thumbnail (JPEG, under 200 kB, at most 320x320), uploaded from a
 *     path or memory, or NULL. A file_id or URL is rejected because Telegram never reuses
 *     thumbnails.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_video_note(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *video_note, int duration, int length,
        const telebot_input_file_t *thumbnail, const telebot_send_options_t *options);

/**
 * @brief Sends 2-10 photos, videos, documents or audios as an album.
 *
 * Documents and audios can only be grouped with items of the same type. reply_markup in @p options
 *     is ignored, because albums cannot carry a keyboard.
 *
 * Bot API method: sendMediaGroup.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] media Album items, see #telebot_input_media_t.
 * @param[in] count Number of items in @p media, 2-10.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_media_group(telebot_handler_t handle, long long int chat_id,
        const telebot_input_media_t media[], int count, const telebot_send_options_t *options);

/**
 * @brief Sends a point on the map.
 *
 * Bot API method: sendLocation.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] latitude Latitude of the location.
 * @param[in] longitude Longitude of the location.
 * @param[in] live_period Seconds the live location updates (60-86400), or 0 for a static location.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_location(telebot_handler_t handle, long long int chat_id, double latitude,
        double longitude, int live_period, const telebot_send_options_t *options);

/**
 * @brief Edit live location messages sent by the bot or via the bot (for inline
 * bots). A location can be edited until its live_period expires or editing is
 * explicitly disabled by a call to #telebot_stop_message_live_location().
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] message_id Required if inline_message_id is not specified.
 * Identifier of the sent message.
 * @param[in] inline_message_id Required if chat_id and message_id are not
 * specified. Identifier of the inline message.
 * @param[in] latitude Latitude of location.
 * @param[in] longitude Longitude of location.
 * @param[in] reply_markup A JSON-serialized object for a new inline keyboard.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_edit_message_live_location(telebot_handler_t handle,
    long long int chat_id, int message_id, const char *inline_message_id,
    float latitude, float longitude, const char *reply_markup);

/**
 * @brief Stop updating a live location message sent by the bot or via the bot
 * (for inline bots) before live_period expires.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] message_id Required if inline_message_id is not specified.
 * Identifier of the sent message.
 * @param[in] inline_message_id Required if chat_id and message_id are not
 * specified. Identifier of the inline message.
 * @param[in] reply_markup A JSON-serialized object for a new inline keyboard.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_stop_message_live_location(telebot_handler_t handle,
    long long int chat_id, int message_id, char *inline_message_id,
    const char *reply_markup);

/**
 * @brief Sends information about a venue.
 *
 * Bot API method: sendVenue.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] latitude Latitude of the venue.
 * @param[in] longitude Longitude of the venue.
 * @param[in] title Name of the venue.
 * @param[in] address Address of the venue.
 * @param[in] foursquare_id Optional Foursquare identifier, or NULL.
 * @param[in] foursquare_type Optional Foursquare type, e.g. "food/icecream", or NULL.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_venue(telebot_handler_t handle, long long int chat_id, double latitude,
        double longitude, const char *title, const char *address, const char *foursquare_id,
        const char *foursquare_type, const telebot_send_options_t *options);

/**
 * @brief Sends a phone contact.
 *
 * Bot API method: sendContact.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] phone_number Contact phone number.
 * @param[in] first_name Contact first name.
 * @param[in] last_name Optional contact last name, or NULL.
 * @param[in] vcard Optional vCard data, 0-2048 bytes, or NULL.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_contact(telebot_handler_t handle, long long int chat_id,
        const char *phone_number, const char *first_name, const char *last_name, const char *vcard,
        const telebot_send_options_t *options);

/**
 * @brief Sends a native poll or quiz.
 *
 * Bot API method: sendPoll.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] question Poll question, 1-300 characters.
 * @param[in] answers Answer options, 1-100 characters each.
 * @param[in] count_answers Number of answer options, 1-12.
 * @param[in] is_anonymous The poll is anonymous (Telegram's default is true, so pass it
 *     explicitly).
 * @param[in] type "regular" or "quiz"; NULL for "regular".
 * @param[in] allows_multiple_answers Voters may choose several answers.
 * @param[in] correct_option_ids Quiz only: zero-based indexes of the correct answers, or NULL.
 * @param[in] count_correct Number of entries in @p correct_option_ids, 0 for none.
 * @param[in] is_closed Send the poll already closed.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_poll(telebot_handler_t handle, long long int chat_id, const char *question,
        const char *const answers[], int count_answers, bool is_anonymous, const char *type,
        bool allows_multiple_answers, const int correct_option_ids[], int count_correct, bool is_closed,
        const telebot_send_options_t *options);

/**
 * @brief Sends an animated emoji showing a random value.
 *
 * Bot API method: sendDice.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] emoji One of the supported emoji ("🎲", "🎯", "🏀", "⚽", "🎳", "🎰"), or NULL for the die.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_dice(telebot_handler_t handle, long long int chat_id, const char *emoji,
        const telebot_send_options_t *options);

/**
 * @brief Shows a status such as "typing" or "uploading photo" for 5 seconds.
 *
 * Bot API method: sendChatAction.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] message_thread_id Forum topic or private-chat topic, or 0.
 * @param[in] action One of "typing", "upload_photo", "record_video", "upload_video",
 *     "record_voice", "upload_voice", "upload_document", "choose_sticker", "find_location",
 *     "record_video_note" or "upload_video_note".
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_chat_action(telebot_handler_t handle, long long int chat_id,
        int message_thread_id, const char *action);

/**
 * @brief Gets a list of profile pictures of a user.
 *
 * Bot API method: getUserProfilePhotos.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] offset Number of the first photo to return; 0 for the newest.
 * @param[in] limit Number of photos to return, 1-100.
 * @param[out] photos Receives the photos; release with #telebot_put_user_profile_photos().
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_get_user_profile_photos(telebot_handler_t handle, long long int user_id, int offset,
        int limit, telebot_user_profile_photos_t *photos);

/**
 * @brief This function is used to free memory allocated for user profile
 * pictures object
 * @param[in] photos A pointer to user profile photo object, obtained with
 * #telebot_get_user_profile_photos.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_put_user_profile_photos(telebot_user_profile_photos_t *photos);

/**
 * @brief Use this method to get user profile audios.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] offset Number of entries to skip; 0 to start from the first.
 * @param[in] limit Maximum number of entries to return.
 * @param[out] audios Receives the audios; release them with #telebot_put_user_profile_audios().
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_get_user_profile_audios(telebot_handler_t handle,
    long long int user_id, int offset, int limit, telebot_user_profile_audios_t *audios);

/**
 * @brief Release user profile audios obtained with #telebot_get_user_profile_audios.
 *
 * @param[in] audios Audios to release.
 * @return #TELEBOT_ERROR_NONE on success, #TELEBOT_ERROR_INVALID_PARAMETER for a NULL argument.
 */
telebot_error_e telebot_put_user_profile_audios(telebot_user_profile_audios_t *audios);

/**
 * @brief Changes the profile photo of the bot.
 *
 * Bot API method: setMyProfilePhoto.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] photo New photo, uploaded from a path or memory. A file_id or URL is rejected.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_set_my_profile_photo(telebot_handler_t handle, const telebot_input_file_t *photo);

/**
 * @brief Use this method to remove the bot's profile photo.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] photo_id Identifier of the profile photo to remove.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_remove_my_profile_photo(telebot_handler_t handle,
    const char *photo_id);

/**
 * @brief Use this method to get information about the business connection.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] business_connection_id Identifier of the business connection.
 * @param[out] connection Receives the business connection; release it with
 *     #telebot_put_business_connection().
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_get_business_connection(telebot_handler_t handle,
    const char *business_connection_id, telebot_business_connection_t *connection);

/**
 * @brief Release business connection obtained with #telebot_get_business_connection.
 *
 * @param[in] connection Business connection to release.
 * @return #TELEBOT_ERROR_NONE on success, #TELEBOT_ERROR_INVALID_PARAMETER for a NULL argument.
 */
telebot_error_e telebot_put_business_connection(telebot_business_connection_t *connection);

/**
 * @brief Use this method to get user chat boosts.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @param[out] boosts Receives the boosts; release them with #telebot_put_user_chat_boosts().
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_get_user_chat_boosts(telebot_handler_t handle,
    long long int chat_id, long long int user_id, telebot_user_chat_boosts_t *boosts);

/**
 * @brief Release user chat boosts obtained with #telebot_get_user_chat_boosts.
 *
 * @param[in] boosts Boosts to release.
 * @return #TELEBOT_ERROR_NONE on success, #TELEBOT_ERROR_INVALID_PARAMETER for a NULL argument.
 */
telebot_error_e telebot_put_user_chat_boosts(telebot_user_chat_boosts_t *boosts);

/**
 * @brief This function is used to download file.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] file_id Identifier of the file to download, e.g. the file_id of a received photo or document.
 * @param[in] path Local path to write the file to; it is created or truncated.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_download_file(telebot_handler_t handle, const char *file_id,
    const char *path);

/**
 * @brief Unbans a previously banned user.
 *
 * Bot API method: unbanChatMember.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] only_if_banned Do nothing if the user is not banned; otherwise an unban also removes a
 *     member from the chat.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_unban_chat_member(telebot_handler_t handle, long long int chat_id,
        long long int user_id, bool only_if_banned);

/**
 * @brief Restricts a user in a supergroup.
 *
 * Pass all permissions as true to lift restrictions.
 *
 * Bot API method: restrictChatMember.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] permissions Permissions the user keeps; anything left false is denied.
 * @param[in] use_independent_chat_permissions Apply each permission exactly as given, instead of
 *     letting some imply others.
 * @param[in] until_date Unix time when the measure ends; 0 (or more than 366 days ahead) means
 *     forever.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_restrict_chat_member(telebot_handler_t handle, long long int chat_id,
        long long int user_id, const telebot_chat_permissions_t *permissions,
        bool use_independent_chat_permissions, long until_date);

/**
 * @brief Promotes or demotes a user in a supergroup or channel.
 *
 * Pass NULL or all rights false to demote the user.
 *
 * Bot API method: promoteChatMember.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] rights Rights to grant, or NULL to demote.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_promote_chat_member(telebot_handler_t handle, long long int chat_id,
        long long int user_id, const telebot_chat_administrator_rights_t *rights);

/**
 * @brief Sets a custom title for an administrator the bot promoted.
 *
 * Bot API method: setChatAdministratorCustomTitle.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] custom_title New title, 0-16 characters, emoji are not allowed.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_set_chat_admin_custom_title(telebot_handler_t handle, long long int chat_id,
        long long int user_id, const char *custom_title);

/**
 * @brief Sets the default permissions of all members of a group or supergroup.
 *
 * Bot API method: setChatPermissions.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] permissions New default permissions; anything left false is denied.
 * @param[in] use_independent_chat_permissions Apply each permission exactly as given, instead of
 *     letting some imply others.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_set_chat_permissions(telebot_handler_t handle, long long int chat_id,
        const telebot_chat_permissions_t *permissions, bool use_independent_chat_permissions);

/**
 * @brief Generate a new invite link for a chat; any previously generated link is
 * revoked. The bot must be an administrator in the chat for this to work and must
 * have the appropriate admin rights.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[out] invite_link Exported invite link, must be freed after use.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_export_chat_invite_link(telebot_handler_t handle,
    long long int chat_id, char **invite_link);

/**
 * @brief Sets a new profile photo for a group, supergroup or channel.
 *
 * Bot API method: setChatPhoto.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] photo New chat photo, uploaded from a path or memory. A file_id or URL is rejected.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_set_chat_photo(telebot_handler_t handle, long long int chat_id,
        const telebot_input_file_t *photo);

/**
 * @brief Delete a chat photo. Photos can't be changed for private chats. The bot
 * must be an administrator in the chat for this to work and must have the
 * appropriate admin rights.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_delete_chat_photo(telebot_handler_t handle, long long int chat_id);

/**
 * @brief Change the title of a chat. Titles can't be
 * changed for private chats. The bot must be an administrator in the chat for
 * this to work and must have the appropriate admin rights.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] title New chat title, 1-255 characters.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_set_chat_title(telebot_handler_t handle, long long int chat_id,
    const char *title);

/**
 * @brief Change the description of a supergroup or a channel. The bot must be
 * an administrator in the chat for this to work and must have the appropriate
 * admin rights.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] description New chat description, 0-255 characters.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_set_chat_description(telebot_handler_t handle,
    long long int chat_id, const char *description);

/**
 * @brief Pin a message in a supergroup or a channel. The bot must be an administrator
 * in the chat for this to work and must have the 'can_pin_messages' admin right
 * in the supergroup or 'can_edit_messages' admin right in the channel.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] message_id Identifier of a message to pin.
 * @param[in] disable_notification  Pass True, if it is not necessary to send
 * a notification to all chat members about the new pinned message.
 * Notifications are always disabled in channels.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_pin_chat_message(telebot_handler_t handle, long long int chat_id,
    int message_id, bool disable_notification);

/**
 * @brief Unpin a message in a supergroup or a channel. The bot must be an administrator
 * in the chat for this to work and must have the 'can_pin_messages' admin right
 * in the supergroup or 'can_edit_messages' admin right in the channel.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_unpin_chat_message(telebot_handler_t handle, long long int chat_id);

/**
 * @brief Leave a group, supergroup or channel.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_leave_chat(telebot_handler_t handle, long long int chat_id);

/**
 * @brief Get up to date information about the chat (current name of the user for
 * one-on-one conversations, current username of a user, group or channel, etc).
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[out] chat Pointer to chat, must be released with #telebot_put_chat().
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_get_chat(telebot_handler_t handle, long long int chat_id,
    telebot_chat_t *chat);


/**
 * @brief Release chat obtained with #telebot_get_chat().
 * @param[in] chat Pointer to chat
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e  telebot_put_chat(telebot_chat_t *chat);

/**
 * @brief Gets the administrators of a chat.
 *
 * Bot API method: getChatAdministrators.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] return_bots Also return other bots that are administrators.
 * @param[out] admins Receives the administrators; release them with #telebot_put_chat_admins().
 * @param[out] count Receives the number of entries in @p admins.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_get_chat_admins(telebot_handler_t handle, long long int chat_id, bool return_bots,
        telebot_chat_member_t **admins, int *count);

/**
 * @brief Release chat admins obtained with #telebot_get_chat_admins().
 *
 * @param[in] admins Administrators to release, as returned by #telebot_get_chat_admins().
 * @param[in] count Number of entries in @p admins.
 * @return #TELEBOT_ERROR_NONE on success, #TELEBOT_ERROR_INVALID_PARAMETER for a NULL argument.
 */
telebot_error_e telebot_put_chat_admins(telebot_chat_member_t *admins, int count);

/**
 * @brief Get the number of members in a chat.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[out] count Number of chat members.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_get_chat_members_count(telebot_handler_t handle,
    long long int chat_id, int *count);

/**
 * @brief Gets information about a member of a chat.
 *
 * Bot API method: getChatMember.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @param[out] member Receives the member; release with #telebot_put_chat_member().
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_get_chat_member(telebot_handler_t handle, long long int chat_id,
        long long int user_id, telebot_chat_member_t *member);

/**
 * @brief Release chat member otained with #telebot_get_chat_member().
 * @param[in] member Pointer to chat member to be released.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_put_chat_member(telebot_chat_member_t *member);

/**
 * @brief Set a new group sticker set for a supergroup. The bot must be an administrator
 * in the chat for this to work and must have the appropriate admin rights. Use the field
 * can_set_sticker_set optionally returned in #telebot_get_chat requests to check
 * if the bot can use it.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] sticker_set_name Name of the sticker set to be set as the group
 * sticker set.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_set_chat_sticker_set(telebot_handler_t handle,
        long long int chat_id, const char *sticker_set_name);

/**
 * @brief Delete a group sticker set from a supergroup. The bot must be an administrator
 * in the chat for this to work and must have the appropriate admin rights. Use the
 * field can_set_sticker_set optionally returned in #telebot_get_chat requests
 * to check if the bot can use this method.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_delete_chat_sticker_set(telebot_handler_t handle,
    long long int chat_id);

/**
 * @brief Send answers to callback queries sent from inline keyboards.
 * The answer will be displayed to the user as a notification at the top of
 * the chat screen or as an alert.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] callback_query_id Unique identifier for the query to be answered.
 * @param[in] text Text of the notification. If not specified, nothing will be shown
 * to the user, 0-200 characters
 * @param[in] show_alert If true, an alert will be shown by the client instead of
 * a notification at the top of the chat screen. Defaults to false.
 * @param[in] url URL that will be opened by the user's client. If you have created
 * a Game and accepted the conditions via \@BotFather, specify the URL that opens
 * your game - note that this will only work if the query comes from a
 * callback_game button.
 * @param[in] cache_time The maximum amount of time in seconds that the result of
 * the callback query may be cached client-side. Telegram apps will support
 * caching starting in version 3.14.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_answer_callback_query(telebot_handler_t handle,
    const char *callback_query_id, const char *text, bool show_alert,
    const char *url, int cache_time);

/**
 * @brief Change the list of the bot's commands.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] commands Array of telegram bot commands.
 * @param[in] count The number of commands in array.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_set_my_commands(telebot_handler_t handle,
    telebot_bot_command_t commands[], int count);

/**
 * @brief Get the current list of the bot's commands.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[out] commands Pointer to commands to be obtained, which must be
 * released with #telebot_put_my_commands().
 * @param[out] count Pointer to place the number of commands.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_get_my_commands(telebot_handler_t handle,
    telebot_bot_command_t **commands, int *count);

/**
 * @brief Release bot commands obtained with #telebot_get_my_commands().
 * @param[in] commands Pointer to commands to be released.
 * @param[in] count The number of commands to be released.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_put_my_commands(telebot_bot_command_t *commands, int count);

/**
 * @brief Edits the text of a message.
 *
 * Identify the message either by @p chat_id and @p message_id, or by @p inline_message_id.
 *
 * Bot API method: editMessageText.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] message_id Identifier of the message to edit, or 0 when using @p inline_message_id.
 * @param[in] inline_message_id Identifier of an inline message, or NULL.
 * @param[in] text New text, 1-4096 characters after entity parsing.
 * @param[in] parse_mode Optional formatting: "MarkdownV2", "HTML" or "Markdown"; NULL for plain
 *     text.
 * @param[in] disable_link_preview Do not show a preview for links in the text.
 * @param[in] reply_markup Optional JSON inline keyboard, or NULL.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_edit_message_text(telebot_handler_t handle, long long int chat_id, int message_id,
        const char *inline_message_id, const char *text, const char *parse_mode, bool disable_link_preview,
        const char *reply_markup);

/**
 * @brief Edit captions of messages sent by the bot or via the bot (for inline bots).
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] message_id Identifier of the message, or 0 when using @p inline_message_id.
 * @param[in] inline_message_id Identifier of an inline message, or NULL when using @p chat_id and
 *     @p message_id.
 * @param[in] caption New caption, 0-1024 characters after entity parsing, or NULL to remove it.
 * @param[in] parse_mode Optional formatting: "MarkdownV2", "HTML" or "Markdown"; NULL for plain
 *     text.
 * @param[in] reply_markup Optional JSON inline keyboard, or NULL.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_edit_message_caption(telebot_handler_t handle,
    long long int chat_id, int message_id, const char *inline_message_id,
    const char *caption, const char *parse_mode, const char *reply_markup);

/**
 * @brief Edit only the reply markup of messages sent by the bot or via the
 * bot (for inline bots).
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Required if inline_message_id. Unique identifier for the target
 * chat or username of the target channel (in the format \@channelusername).
 * @param[in] message_id Required if inline_message_id is not specified.
 * Identifier of the sent message.
 * @param[in] inline_message_id Required if chat_id and message_id are not
 * specified. Identifier of the inline message.
 * @param[in] reply_markup A JSON-serialized object for an inline keyboard.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */

telebot_error_e telebot_edit_message_reply_markup(telebot_handler_t handle,
    long long int chat_id, int message_id, const char *inline_message_id,
    const char *reply_markup);

/**
 * @brief Stop a poll which was sent by the bot.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] message_id Identifier of the message with the poll.
 * @param[in] reply_markup Optional JSON inline keyboard, or NULL.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_stop_poll(telebot_handler_t handle, long long int chat_id,
    int message_id, const char *reply_markup);

/**
 * @brief Delete a message, including service messages, with the following
 *  limitations:
 *  - A message can only be deleted if it was sent less than 48 hours ago.
 *  - Bots can delete outgoing messages in groups and supergroups.
 *  - Bots granted can_post_messages permissions can delete outgoing messages
 *    in channels.
 *  - If the bot is an administrator of a group, it can delete any message there.
 *  - If the bot has can_delete_messages permission in a supergroup or a channel,
 *    it can delete any message there.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] message_id Identifier of the message, or 0 when using @p inline_message_id.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_delete_message(telebot_handler_t handle, long long int chat_id,
    int message_id);

/**
 * @brief Use this method to log out from the cloud Bot API server before
 * launching the bot locally.
 * @param[in] handle The telebot handler.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_log_out(telebot_handler_t handle);

/**
 * @brief Use this method to close the bot instance before moving it from one
 * local server to another.
 * @param[in] handle The telebot handler.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_close(telebot_handler_t handle);

/**
 * @brief Use this method to change the bot's name.
 * @param[in] handle The telebot handler.
 * @param[in] name New bot name; 0-64 characters.
 * @param[in] language_code A two-letter ISO 639-1 language code.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_set_my_name(telebot_handler_t handle,
    const char *name, const char *language_code);

/**
 * @brief Use this method to get the current bot name for the given user language.
 * @param[in] handle The telebot handler.
 * @param[in] language_code A two-letter ISO 639-1 language code.
 * @param[out] name Pointer to get bot name, MUST be freed after use.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_get_my_name(telebot_handler_t handle,
    const char *language_code, char **name);

/**
 * @brief Use this method to change the bot's description.
 * @param[in] handle The telebot handler.
 * @param[in] description New bot description; 0-512 characters.
 * @param[in] language_code A two-letter ISO 639-1 language code.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_set_my_description(telebot_handler_t handle,
    const char *description, const char *language_code);

/**
 * @brief Use this method to get the current bot description for the given user language.
 * @param[in] handle The telebot handler.
 * @param[in] language_code A two-letter ISO 639-1 language code.
 * @param[out] description Pointer to get bot description, MUST be freed after use.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_get_my_description(telebot_handler_t handle,
    const char *language_code, char **description);

/**
 * @brief Use this method to change the bot's short description.
 * @param[in] handle The telebot handler.
 * @param[in] short_description New short description; 0-120 characters.
 * @param[in] language_code A two-letter ISO 639-1 language code.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_set_my_short_description(telebot_handler_t handle,
    const char *short_description, const char *language_code);

/**
 * @brief Use this method to get the current bot short description for the given user language.
 * @param[in] handle The telebot handler.
 * @param[in] language_code A two-letter ISO 639-1 language code.
 * @param[out] short_description Pointer to get bot short description, MUST be freed after use.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_get_my_short_description(telebot_handler_t handle,
    const char *language_code, char **short_description);

/**
 * @brief Use this method to change the bot's menu button in a private chat, or the default menu button.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target private chat.
 * @param[in] menu_button A JSON-serialized object for the bot's new menu button.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_set_chat_menu_button(telebot_handler_t handle,
    long long int chat_id, const char *menu_button);

/**
 * @brief Use this method to get the current value of the bot's menu button in a private chat, or the default menu button.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target private chat.
 * @param[out] menu_button Pointer to get menu button, MUST be freed after use.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_get_chat_menu_button(telebot_handler_t handle,
    long long int chat_id, char **menu_button);

/**
 * @brief Use this method to change the default administrator rights requested by the bot when it's added as an administrator to groups or channels.
 * @param[in] handle The telebot handler.
 * @param[in] rights A JSON-serialized object describing new default administrator rights.
 * @param[in] for_channels Pass True to change the default administrator rights of the bot in channels.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_set_my_default_administrator_rights(telebot_handler_t handle,
    const char *rights, bool for_channels);

/**
 * @brief Use this method to get the current default administrator rights of the bot.
 * @param[in] handle The telebot handler.
 * @param[in] for_channels Pass True to get default administrator rights of the bot in channels.
 * @param[out] rights Pointer to get admin rights, MUST be freed after use.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_get_my_default_administrator_rights(telebot_handler_t handle,
    bool for_channels, char **rights);

/**
 * @brief Use this method to delete the list of the bot's commands for the given scope and user language.
 * @param[in] handle The telebot handler.
 * @param[in] scope A JSON-serialized object, describing scope of users for which the commands are relevant.
 * @param[in] language_code A two-letter ISO 639-1 language code.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_delete_my_commands(telebot_handler_t handle,
    const char *scope, const char *language_code);

/**
 * @brief Copies a message without a link to the original.
 *
 * Bot API method: copyMessage.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] from_chat_id Chat where the original message was sent.
 * @param[in] message_id Identifier of the message in @p from_chat_id.
 * @param[in] caption Optional new caption for media, or NULL to keep the original.
 * @param[in] parse_mode Optional formatting of the caption: "MarkdownV2", "HTML" or "Markdown";
 *     NULL for plain text.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @param[out] message_id_out Optional: receives the identifier of the new message. May be NULL.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_copy_message(telebot_handler_t handle, long long int chat_id,
        long long int from_chat_id, int message_id, const char *caption, const char *parse_mode,
        const telebot_send_options_t *options, int *message_id_out);

/**
 * @brief Copies several messages without links to the originals.
 *
 * Only message_thread_id, disable_notification and protect_content are taken from @p options.
 *
 * Bot API method: copyMessages.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] from_chat_id Chat where the original messages were sent.
 * @param[in] message_ids Identifiers of 1-100 messages in @p from_chat_id, in increasing order.
 * @param[in] count Number of identifiers in @p message_ids.
 * @param[in] remove_caption Copy media without their captions.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @param[out] message_ids_out Optional: receives the identifiers of the sent messages; release with
 *     free(). May be NULL.
 * @param[out] count_out Optional: receives the number of entries in @p message_ids_out. May be
 *     NULL.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_copy_messages(telebot_handler_t handle, long long int chat_id,
        long long int from_chat_id, const int message_ids[], int count, bool remove_caption,
        const telebot_send_options_t *options, int **message_ids_out, int *count_out);

/**
 * @brief Forwards several messages at once, keeping album grouping.
 *
 * Only message_thread_id, disable_notification and protect_content are taken from @p options.
 *
 * Bot API method: forwardMessages.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] from_chat_id Chat where the original messages were sent.
 * @param[in] message_ids Identifiers of 1-100 messages in @p from_chat_id, in increasing order.
 * @param[in] count Number of identifiers in @p message_ids.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @param[out] message_ids_out Optional: receives the identifiers of the sent messages; release with
 *     free(). May be NULL.
 * @param[out] count_out Optional: receives the number of entries in @p message_ids_out. May be
 *     NULL.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_forward_messages(telebot_handler_t handle, long long int chat_id,
        long long int from_chat_id, const int message_ids[], int count,
        const telebot_send_options_t *options, int **message_ids_out, int *count_out);

/**
 * @brief Use this method to delete multiple messages in a chat.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] message_ids A JSON-serialized list of 1-100 identifiers of messages to delete.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_delete_messages(telebot_handler_t handle,
    long long int chat_id, const char *message_ids);

/**
 * @brief Bans a user from a group, supergroup or channel.
 *
 * The bot must be an administrator with the right to restrict members.
 *
 * Bot API method: banChatMember.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] until_date Unix time when the measure ends; 0 (or more than 366 days ahead) means
 *     forever.
 * @param[in] revoke_messages Delete all messages from the chat for the user.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_ban_chat_member(telebot_handler_t handle, long long int chat_id,
        long long int user_id, long until_date, bool revoke_messages);

/**
 * @brief Use this method to ban a channel chat in a supergroup or a channel.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] sender_chat_id Unique identifier of the target sender chat.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_ban_chat_sender_chat(telebot_handler_t handle,
    long long int chat_id, long long int sender_chat_id);

/**
 * @brief Use this method to unban a previously banned channel chat in a supergroup or a channel.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] sender_chat_id Unique identifier of the target sender chat.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_unban_chat_sender_chat(telebot_handler_t handle,
    long long int chat_id, long long int sender_chat_id);

/**
 * @brief Use this method to create an additional invite link for a chat.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] name Invite link name; 0-32 characters.
 * @param[in] expire_date Point in time (Unix timestamp) when the link will expire.
 * @param[in] member_limit The maximum number of users that can be members of the chat simultaneously after joining the chat via this invite link; 1-99999.
 * @param[in] creates_join_request True, if users joining the chat via the link need to be approved by chat administrators.
 * @param[out] invite_link Pointer to get ChatInviteLink, MUST be released with #telebot_put_chat_invite_link.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_create_chat_invite_link(telebot_handler_t handle,
    long long int chat_id, const char *name, long expire_date, int member_limit,
    bool creates_join_request, telebot_chat_invite_link_t *invite_link);

/**
 * @brief Use this method to edit a non-primary invite link created by the bot.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] invite_link_str The invite link to edit.
 * @param[in] name Invite link name; 0-32 characters.
 * @param[in] expire_date Point in time (Unix timestamp) when the link will expire.
 * @param[in] member_limit The maximum number of users that can be members of the chat simultaneously after joining the chat via this invite link; 1-99999.
 * @param[in] creates_join_request True, if users joining the chat via the link need to be approved by chat administrators.
 * @param[out] invite_link Pointer to get ChatInviteLink, MUST be released with #telebot_put_chat_invite_link.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_edit_chat_invite_link(telebot_handler_t handle,
    long long int chat_id, const char *invite_link_str, const char *name,
    long expire_date, int member_limit, bool creates_join_request,
    telebot_chat_invite_link_t *invite_link);

/**
 * @brief Use this method to revoke an invite link created by the bot.
 * @param[in] handle The telebot handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] invite_link_str The invite link to revoke.
 * @param[out] invite_link Pointer to get ChatInviteLink, MUST be released with #telebot_put_chat_invite_link.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_revoke_chat_invite_link(telebot_handler_t handle,
    long long int chat_id, const char *invite_link_str, telebot_chat_invite_link_t *invite_link);

/**
 * @brief Release chat invite link obtained with #telebot_create_chat_invite_link etc.
 * @param[in] invite_link Pointer to chat invite link to be released.
 * @return on Success, #TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_put_chat_invite_link(telebot_chat_invite_link_t *invite_link);

/**
 * @brief Approves a request to join a chat.
 *
 * Bot API method: approveChatJoinRequest.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_approve_chat_join_request(telebot_handler_t handle, long long int chat_id,
        long long int user_id);

/**
 * @brief Declines a request to join a chat.
 *
 * Bot API method: declineChatJoinRequest.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_decline_chat_join_request(telebot_handler_t handle, long long int chat_id,
        long long int user_id);

/**
 * @brief Sets or removes the bot's reaction to a message.
 *
 * Bot API method: setMessageReaction.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] message_id Identifier of the message to react to.
 * @param[in] emoji Reaction emoji such as "👍", or NULL to remove the bot's reaction.
 * @param[in] is_big Show the reaction with a big animation.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_set_message_reaction(telebot_handler_t handle, long long int chat_id, int message_id,
        const char *emoji, bool is_big);

/**
 * @brief Replaces the photo, video, audio or document of a message.
 *
 * Identify the message either by @p chat_id and @p message_id, or by @p inline_message_id. New
 *     media can be uploaded from a path or memory, or given as a file_id or URL.
 *
 * Bot API method: editMessageMedia.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] message_id Identifier of the message to edit, or 0 when using @p inline_message_id.
 * @param[in] inline_message_id Identifier of an inline message, or NULL.
 * @param[in] media New content of the message; see #telebot_input_media_t.
 * @param[in] reply_markup Optional JSON inline keyboard, or NULL.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_edit_message_media(telebot_handler_t handle, long long int chat_id, int message_id,
        const char *inline_message_id, const telebot_input_media_t *media, const char *reply_markup);

/**
 * @brief Streams a partial message to a private chat while it is being generated.
 *
 * The draft is a temporary preview that disappears after about 30 seconds or when the bot sends a
 *     message. Call it repeatedly with the same @p draft_id to update the text with an animation,
 *     then send the final text with #telebot_send_message(). With @p can_stop, a stop button is
 *     shown and pressing it delivers a #TELEBOT_UPDATE_TYPE_STOPPED_MESSAGE_GENERATION update.
 *
 * Bot API method: sendMessageDraft.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target private chat.
 * @param[in] message_thread_id Topic in the private chat, or 0.
 * @param[in] draft_id Non-zero identifier of the draft; reuse it for every update of the same
 *     draft.
 * @param[in] text Current text, 0-4096 characters after entity parsing; NULL or "" shows a
 *     "Thinking" placeholder.
 * @param[in] parse_mode Optional formatting: "MarkdownV2", "HTML" or "Markdown"; NULL for plain
 *     text.
 * @param[in] can_stop Show the user a button that stops further drafts.
 * @param[in] keep_on_stop Keep the draft visible for a while after the stop button is pressed.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_send_message_draft(telebot_handler_t handle, long long int chat_id,
        int message_thread_id, int draft_id, const char *text, const char *parse_mode, bool can_stop,
        bool keep_on_stop);

/**
 * @brief Unpins all pinned messages in a chat.
 *
 * In groups and channels the bot needs the can_pin_messages or can_edit_messages right.
 *
 * Bot API method: unpinAllChatMessages.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_unpin_all_chat_messages(telebot_handler_t handle, long long int chat_id);

/**
 * @brief Removes the reaction a user or chat added to a message.
 *
 * Pass exactly one of @p user_id and @p actor_chat_id. The bot needs the can_delete_messages right.
 *
 * Bot API method: deleteMessageReaction.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] message_id Identifier of the message.
 * @param[in] user_id User whose reaction is removed, or 0.
 * @param[in] actor_chat_id Chat whose reaction is removed, or 0.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_delete_message_reaction(telebot_handler_t handle, long long int chat_id,
        int message_id, long long int user_id, long long int actor_chat_id);

/**
 * @brief Removes up to 10000 recent reactions added by a user or chat.
 *
 * Pass exactly one of @p user_id and @p actor_chat_id. The bot needs the can_delete_messages right.
 *
 * Bot API method: deleteAllMessageReactions.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id User whose reactions are removed, or 0.
 * @param[in] actor_chat_id Chat whose reactions are removed, or 0.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_delete_all_message_reactions(telebot_handler_t handle, long long int chat_id,
        long long int user_id, long long int actor_chat_id);

/**
 * @brief Sets or removes the tag of a member of a group or supergroup.
 *
 * The bot needs the can_manage_tags right.
 *
 * Bot API method: setChatMemberTag.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] tag New tag, 0-16 characters without emoji, or NULL to remove the tag.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_set_chat_member_tag(telebot_handler_t handle, long long int chat_id,
        long long int user_id, const char *tag);

/**
 * @brief Verifies a user on behalf of the organization the bot represents.
 *
 * Bot API method: verifyUser.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] custom_description Optional verification description, 0-70 characters, or NULL.
 * @param[in] parse_mode Optional formatting of @p custom_description (only text links are
 *     supported), or NULL.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_verify_user(telebot_handler_t handle, long long int user_id,
        const char *custom_description, const char *parse_mode);

/**
 * @brief Verifies a chat on behalf of the organization the bot represents.
 *
 * Bot API method: verifyChat.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] custom_description Optional verification description, 0-70 characters, or NULL.
 * @param[in] parse_mode Optional formatting of @p custom_description (only text links are
 *     supported), or NULL.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_verify_chat(telebot_handler_t handle, long long int chat_id,
        const char *custom_description, const char *parse_mode);

/**
 * @brief Removes the verification of a user that the organization added.
 *
 * Bot API method: removeUserVerification.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] user_id Unique identifier of the target user.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_remove_user_verification(telebot_handler_t handle, long long int user_id);

/**
 * @brief Removes the verification of a chat that the organization added.
 *
 * Bot API method: removeChatVerification.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_remove_chat_verification(telebot_handler_t handle, long long int chat_id);

/**
 * @brief Answers a query from a widget.
 *
 * Answer within 15 seconds. The answer goes only to the widget that sent the query and does not
 *     change the data of the widget.
 *
 * Bot API method: answerWidgetQuery.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] widget_query_id Identifier of the query, from #telebot_widget_query_t.
 * @param[in] data Optional data returned to the widget, 0-1024 bytes, or NULL.
 * @return #TELEBOT_ERROR_NONE on success, otherwise a negative #telebot_error_e value.
 */
telebot_error_e telebot_answer_widget_query(telebot_handler_t handle, const char *widget_query_id,
        const char *data);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_METHODS_H__ */
