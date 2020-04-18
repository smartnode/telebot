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

#ifndef __TELEBOT_CORE_API_H__
#define __TELEBOT_CORE_API_H__

#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        telebot-core-api.h
 * @ingroup     TELEBOT_CORE_API
 * @brief       This file contains core API for the telegram bot interface
 * @author      Elmurod Talipov
 * @date        2020-04-19
 * @version     4.7.0
 */

/**
 * @defgroup TELEBOT_CORE_API Core Telegram Bot API
 * @brief The APIs for the telegram bot interface, response is unparsed json sting.
 *
 *
 * @addtogroup TELEBOT_CORE_API
 * @{
 */

/**
 * @brief This object represents a core telebot handler.
 */
typedef struct telebot_core_handler {
    char *token; /**< Telegam bot token */
    char *proxy_addr;
    char *proxy_auth;
} telebot_core_handler_t;

/**
 * @brief This object represents a telegram bot response.
 */
typedef struct telebot_core_response {
    size_t size; /**< Telegam bot response size */
    char *data; /**< Telegam bot response object */
} telebot_core_response_t;

/**
 * @brief Start function to use telebot core APIs.
 *
 * This function must be used first to call, and it creates handler that is used
 * as input to other functions in telebot core interface. This call MUST have
 * corresponding call to #telebot_core_destroy when operation is complete.
 * @param core_h[in,out] A pointer to a handler, which will be allocated and created.
 * Obtained handler MUST be released with #telebot_core_destroy()
 * @param token[in] Telegram bot token to use.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_create(telebot_core_handler_t **core_h, const char *token);

/**
 * @brief Final function to use telebot core APIs
 *
 * This function must be the last function to call for a telebot core use.
 * It is the opposite of the telebot_core_create function and MUST be called
 * with the same handler as the input that a telebot_core_create call
 * created.
 * @param[in,out] core_h The A pointer to telebot core handler created with #telebot_core_create().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_destroy(telebot_core_handler_t **core_h);

/**
 * @brief Set proxy address to use telebot behind proxy
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] addr Proxy address in full.
 * @param[in] auth Proxy authorization informatio.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_set_proxy(telebot_core_handler_t *core_h, const char *addr,
        const char *auth);

/**
 * @brief Get currently used proxy address
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[out] addr Current proxy address or NULL, MUST be freed after use.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_get_proxy(telebot_core_handler_t *core_h, char **addr);

/**
 * @brief Receive incoming updates (long polling). It will not work if an outgoing
 * webhook is set up. In order to avoid getting duplicate updates, recalculate
 * offset after each server response.
 * @param[in] core_h Telebot core handler creating using telebot_core_create().
 * @param[in] offset Identifier of the first update to be returned. Must be greater
 * by one than the highest among the identifiers of previously received updates.
 * By default, updates starting with the earliest unconfirmed update are returned.
 * An update is considered confirmed as soon as the function is called with an
 * offset higher than its update_id.
 * @param[in] limit Limits the number of updates to be retrieved. Values between
 * 1—100 are accepted. Defaults to 100.
 * @param[in] timeout Timeout in seconds for long polling. Defaults to 0, i.e. usual
 * short polling.
 * @param[in] allowed_updates List the types of update you want your bot to receive.
 * Specify an empty list to receive all updates regardless of type (default).
 * @param[out] response Response data, MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_get_updates(telebot_core_handler_t *core_h, int offset,
        int limit, int timeout, const char *allowed_updates,
        telebot_core_response_t *response);

/**
 * @brief Specify a url and receive incoming updates via an outgoing webhook.
 * Whenever there is an update for the bot, we will send an HTTPS POST request
 * to the specified url, containing a JSON-serialized Update. In case of an
 * unsuccessful request, we will give up after a reasonable amount of attempts.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] url HTTPS url to send updates to. Use an empty string to remove webhook
 * integration
 * @param[in] certificate A path to to a public key certificate to upload server.
 * @param[in] max_connections Optional  Maximum allowed number of simultaneous
 * HTTPS connections to the webhook for update delivery, 1-100. Defaults to 40.
 * Use lower values to limit the load on your bot's server, and higher values
 * to increase your bot's throughput.
 * @param[in] allowed_updates List the types of updates you want your bot to
 * receive. For example, specify ["message", "edited_channel_post",
 * "callback_query"] to only receive updates of these types.
 * @param[out] response Response data, MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_set_webhook(telebot_core_handler_t *core_h, const char *url,
        const char *certificate, int max_connections, const char *allowed_updates,
        telebot_core_response_t *response);

/**
 * @brief Remove webhook integration if you decide to switch back to getUpdates.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[out] response Response data, MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_delete_webhook(telebot_core_handler_t *core_h,
        telebot_core_response_t *response);

/**
 * @brief Get current webhook status.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[out] response Response data, MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_get_webhook_info(telebot_core_handler_t *core_h,
        telebot_core_response_t *response);

/**
 * @brief Get basic information about the bot.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[out] response Response data, MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_get_me(telebot_core_handler_t *core_h,
        telebot_core_response_t *response);

/**
 * @brief Send text messages.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] text Text of the message to be sent, 1-4096 characters after entities parsing.
 * @param[in] parse_mode Send Markdown or HTML, if you want Telegram apps to show bold,
 * italic, fixed-width or inline URLs in your bot's message.
 * @param[in] disable_web_page_preview Disables link previews for links in this message.
 * @param[in] disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom
 * reply keyboard, instructions to hide keyboard or to force a reply from the user.
 * @param[out] response Response data that contains the sent message on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_send_message(telebot_core_handler_t *core_h,
        long long int chat_id, const char *text, const char *parse_mode,
        bool disable_web_page_preview, bool disable_notification, int reply_to_message_id,
        const char *reply_markup, telebot_core_response_t *response);

/**
 * @brief Forward messages of any kind.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] from_chat_id Unique identifier for the chat where the original
 * message was sent (or channel username in the format \@channelusername).
 * @param[in] disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param[in] message_id Unique message identifier.
 * @param[out] response Response data that contains the sent message on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_forward_message(telebot_core_handler_t *core_h,
        long long int chat_id, long long int from_chat_id, bool disable_notification,
        int message_id, telebot_core_response_t *response);

/**
 * @brief Send photos.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] photo Photo to send. It is either file_id as String to resend a photo
 * that is already on the Telegram servers, or a path to photo file.
 * @param[in] is_file False if photo is file_id, true, if photo is a file path.
 * @param[in] caption Photo caption. (may also be used when resending photos).
 * @param[in] parse_mode Send Markdown or HTML, if you want Telegram apps to show bold,
 * italic, fixed-width or inline URLs in your bot's message.
 * @param[in] disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom
 * reply keyboard, instructions to hide keyboard or to force a reply from
 * the user.
 * @param[out] response Response data that contains the sent message on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_send_photo(telebot_core_handler_t *core_h,
        long long int chat_id, const char *photo, bool is_file, const char *caption,
        const char *parse_mode, bool disable_notification, int reply_to_message_id,
        const char *reply_markup, telebot_core_response_t *response);

/**
 * @brief Send audio files. if you want Telegram clients to display them in the
 * music player. Your audio must be in the .mp3 format. Bots can currently send
 * audio files of up to 50 MB in size. For backward compatibility, when the fields
 * title and performer are both empty and the mime-type of the file to be sent is
 * not audio/mpeg, the file will be sent as a playable voice message. For this to
 * work, the audio must be in an .ogg file encoded with OPUS. For sending voice
 * messages, use the telegram_core_send_voice() function instead.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] audio Audio file to send. It is either a file_id as String to resend an
 * audio that is already on the Telegram servers, or a path to audio file.
 * @param[in] is_file False if audio is file_id, true, if audio is a file path.
 * @param[in] caption Audio caption. (may also be used when resending audios).
 * @param[in] parse_mode Send Markdown or HTML, if you want Telegram apps to show bold,
 * italic, fixed-width or inline URLs in your bot's message.
 * @param[in] duration Duration of sent audio in seconds.
 * @param[in] performer The performer of the audio.
 * @param[in] title The track name of the audio.
 * @param[in] thumb Thumbnail file path of the file sent; can be ignored if thumbnail
 * generation for the file is supported server-side. The thumbnail should be in
 * JPEG format and less than 200 kB in size. A thumbnail's width and height should not
 * exceed 320. Optional. Set to NULL to ignore.
 * @param[in] disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @param[out] response Response data that contains the sent message on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_send_audio(telebot_core_handler_t *core_h,
        long long int chat_id, const char *audio, bool is_file, const char *caption,
        const char *parse_mode, int duration, const char *performer, const char *title,
        const char *thumb, bool disable_notification, int reply_to_message_id,
        const char *reply_markup, telebot_core_response_t *response);

/**
 * @brief Send general files.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] document Document file to send. It is either a file_id as String to
 * resend a file that is already on the Telegram servers, or a path to file.
 * @param[in] is_file False if document is file_id, true, if document is a file path.
 * @param[in] thumb Thumbnail file path of the file sent; can be ignored if thumbnail
 * generation for the file is supported server-side. The thumbnail should be in
 * JPEG format and less than 200 kB in size. A thumbnail's width and height should not
 * exceed 320. Optional. Set to NULL to ignore.
 * @param[in] caption Document caption. (may also be used when resending documents).
 * @param[in] parse_mode Send Markdown or HTML, if you want Telegram apps to show bold,
 * italic, fixed-width or inline URLs in your bot's message.
 * @param[in] disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @param[out] response Response data that contains the sent message on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_send_document(telebot_core_handler_t *core_h,
        long long int chat_id, const char *document, bool is_file, const char *thumb,
        const char *caption, const char *parse_mode, bool disable_notification,
        int reply_to_message_id, const char *reply_markup,
        telebot_core_response_t *response);

/**
 * @brief Send video files, Telegram clients support mp4 videos (other formats
 * may be sent as Document).
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] video Video file to send. It is either a file_id as String to resend
 * a video that is already on the Telegram servers, or a path to video file.
 * @param[in] is_file False if video is file_id, true, if video is a file path.
 * @param[in] duration Duration of sent video in seconds. Optional, set 0 for ignoring.
 * @param[in] width Video width. Optional, set 0 for ignoring.
 * @param[in] height Video heigh. Optional, set 0 for ignoring.
 * @param[in] thumb Thumbnail file path of the file sent; can be ignored if thumbnail
 * generation for the file is supported server-side. The thumbnail should be in
 * JPEG format and less than 200 kB in size. A thumbnail's width and height should not
 * exceed 320. Optional. Set to NULL to ignore.
 * @param[in] caption Video caption. (may also be used when resending videos).
 * @param[in] parse_mode Send Markdown or HTML, if you want Telegram apps to show bold,
 * italic, fixed-width or inline URLs in your bot's message.
 * @param[in] supports_streaming Pass True, if the uploaded video is suitable for streaming.
 * @param[in] disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @param[out] response Response data that contains the sent message on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_send_video(telebot_core_handler_t *core_h,
        long long int chat_id, const char *video, bool is_file, int duration,
        int width, int height, const char *thumb, const char *caption,
        const char *parse_mode, bool supports_streaming, bool disable_notification,
        int reply_to_message_id, const char *reply_markup,
        telebot_core_response_t *response);

/**
 * @brief Send animation files (GIF or H.264/MPEG-4 AVC without sound).
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] animation Animation file to send. It is either a file_id to resend
 * an animation that exists on the Telegram servers, or a path to animation file.
 * @param[in] is_file False if animation is file_id, true, if animation is a file path.
 * @param[in] duration Duration of sent animation in seconds.
 * @param[in] width Animation width
 * @param[in] height Animation height
 * @param[in] thumb Thumbnail file path of the file sent; can be ignored if thumbnail
 * generation for the file is supported server-side. The thumbnail should be in
 * JPEG format and less than 200 kB in size. A thumbnail's width and height should not
 * exceed 320. Optional. Set to NULL to ignore.
 * @param[in] caption Animation caption. (may also be used when resending animations).
 * @param[in] parse_mode Send Markdown or HTML, if you want Telegram apps to show bold,
 * @param[in] disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @param[out] response Response data that contains the sent message on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_send_animation(telebot_core_handler_t *core_h,
        long long int chat_id, const char *animation, bool is_file, int duration,
        int width, int height, const char *thumb, const char *caption,
        const char *parse_mode, bool disable_notification, int reply_to_message_id,
        const char *reply_markup, telebot_core_response_t *response);

/**
 * @brief Send audio files, if you want Telegram clients to display the file as
 * a playable voice message. For this to work, your audio must be in an .ogg file
 * encoded with OPUS (other formats may be sent as Audio or Document).
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] voice Audio file to send. It is either a file_id as String to resend
 * a audio that is already on the Telegram servers, or a path to audio file.
 * @param[in] is_file False if voice is file_id, true, if voice is a file path.
 * @param[in] caption Voice message caption. (may also be used when resending).
 * @param[in] parse_mode Send Markdown or HTML, if you want Telegram apps to show bold,
 * @param[in] duration Duration of sent voice/audio in seconds.
 * @param[in] disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @param[out] response Response data that contains the sent message on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_send_voice(telebot_core_handler_t *core_h,
        long long int chat_id, const char *voice, bool is_file, const char *caption,
        const char *parse_mode, int duration, bool disable_notification,
        int reply_to_message_id, const char *reply_markup,
        telebot_core_response_t *response);

/**
 * @brief Send video messages. As of v.4.0, Telegram clients support rounded square
 * mp4 videos of up to 1 minute long.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] video_not Video note to send. It is either a file_id to resend
 * a video note that exists on the Telegram servers, or a path to video note file.
 * @param[in] is_file False if video note is file_id, true, if video note is a file path.
 * @param[in] duration Duration of sent video in seconds.
 * @param[in] length Video width and height, i.e. diameter of the video message.
 * @param[in] thumb Thumbnail file path of the file sent; can be ignored if thumbnail
 * generation for the file is supported server-side. The thumbnail should be in
 * JPEG format and less than 200 kB in size. A thumbnail's width and height should not
 * exceed 320. Optional. Set to NULL to ignore.
 * @param[in] disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @param[out] response Response data that contains the sent message on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_send_video_note(telebot_core_handler_t *core_h,
        long long int chat_id, char *video_note, bool is_file, int duration, int length,
        const char *thumb, bool disable_notification, int reply_to_message_id,
        const char *reply_markup, telebot_core_response_t *response);

/**
 * @brief Send point on the map.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] latitude Latitude of location.
 * @param[in] longitude Longitude of location.
 * @param[in] live_period Period in secnods for which the location will be updated.
 * Should be between 60 and 86400.
 * @param[in] disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @param[out] response Response data that contains the sent message on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_send_location(telebot_core_handler_t *core_h,
        long long int chat_id, float latitude, float longitude, int live_period,
        bool disable_notification, int reply_to_message_id, const char *reply_markup,
        telebot_core_response_t *response);

/**
 * @brief Edit live location messages sent by the bot or via the bot (for inline
 * bots). A location can be edited until its live_period expires or editing is
 * explicitly disabled by a call to #telebot_core_stop_message_live_location().
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] message_id Required if inline_message_id is not specified.
 * Identifier of the sent message.
 * @param[in] inline_message_id Required if chat_id and message_id are not
 * specified. Identifier of the inline message.
 * @param[in] latitude Latitude of location.
 * @param[in] longitude Longitude of location.
 * @param[in] reply_markup A JSON-serialized object for a new inline keyboard.
 * @param[out] response Response data that contains the sent message on success.
 * Otherwise, True is returned. It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_edit_message_live_location(telebot_core_handler_t *core_h,
        long long int chat_id, int message_id, const char *inline_message_id,
        float latitude, float longitude, const char *reply_markup,
        telebot_core_response_t *response);

/**
 * @brief Stop updating a live location message sent by the bot or via the bot
 * (for inline bots) before live_period expires.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] message_id Required if inline_message_id is not specified.
 * Identifier of the sent message.
 * @param[in] inline_message_id Required if chat_id and message_id are not
 * specified. Identifier of the inline message.
 * @param[in] reply_markup A JSON-serialized object for a new inline keyboard.
 * @param[out] response Response data that contains the sent message on success.
 * Otherwise, True is returned. It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_stop_message_live_location(telebot_core_handler_t *core_h,
        long long int chat_id, int message_id, char *inline_message_id,
        const char *reply_markup, telebot_core_response_t *response);

/**
 * @brief Send information about a venue.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] latitude Latitude of venue.
 * @param[in] longitude Longitude of venue.
 * @param[in] title Name of the venue
 * @param[in] address Address of the venue.
 * @param[in] foursquare_id Foursquare identifier of the venue.
 * @param[in] foursquare_type Foursquare type of the venue, if known.
 * @param[in] disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. A JSON-serialized
 * object for an inline keyboard, custom reply keyboard, instructions to remove
 * reply keyboard or to force a reply from the user.
 * @param[out] response Response data that contains the sent message on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_send_venue(telebot_core_handler_t *core_h,
        long long int chat_id, float latitude, float longitude, const char *title,
        const char *address, const char *foursquare_id, const char *foursquare_type,
        bool disable_notification, int reply_to_message_id, const char *reply_markup,
        telebot_core_response_t *response);

/**
 * @brief Send phone contacts.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] phone_number Contact's phone numbers.
 * @param[in] first_name Contact's first name.
 * @param[in] last_name Contact's last name.
 * @param[in] vcard Additional data about the contact in the form of a vCard, 0-2048 bytes.
 * @param[in] disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. A JSON-serialized
 * object for an inline keyboard, custom reply keyboard, instructions to remove
 * reply keyboard or to force a reply from the user.
 * @param[out] response Response data that contains the sent message on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_send_contact(telebot_core_handler_t *core_h,
        long long int chat_id, const char *phone_number, const char *first_name,
        const char *last_name, const char *vcard, bool disable_notification,
        int reply_to_message_id, const char *reply_markup,
        telebot_core_response_t *response);

/**
 * @brief Send a native poll.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] question Poll question, 1-255 characters.
 * @param[in] options A JSON-serialized list of answer options, 2-10 strings 1-100
 * characters each.
 * @param[in] is_anonymous True, if the poll needs to be anonymous, defaults to True.
 * @param[in] type Poll type, “quiz” or “regular”, defaults to “regular”.
 * @param[in] allows_multiple_answers True, if the poll allows multiple answers,
 * ignored for polls in quiz mode, defaults to False
 * @param[in] correct_option_id 0-based identifier of the correct answer option,
 * required for polls in quiz mode.
 * @param[in] is_closed Pass True, if the poll needs to be immediately closed.
 * This can be useful for poll preview.
 * @param[in] disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. A JSON-serialized
 * object for an inline keyboard, custom reply keyboard, instructions to remove
 * reply keyboard or to force a reply from the user.
 * @param[out] response Response data that contains the sent message on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_send_poll(telebot_core_handler_t *core_h,
        long long int chat_id, const char *question, const char *options,
        bool is_anonymous, const char *type, bool allows_multiple_answers,
        int correct_option_id, bool is_closed, bool disable_notification,
        int reply_to_message_id, const char *reply_markup,
        telebot_core_response_t *response);

/**
 * @brief Send a dice, which will have a random value from 1 to 6.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param[in] disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. A JSON-serialized
 * object for an inline keyboard, custom reply keyboard, instructions to remove
 * reply keyboard or to force a reply from the user.
 * @param[out] response Response data that contains the sent message on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_send_dice(telebot_core_handler_t *core_h,
        long long int chat_id, bool disable_notification,
        int reply_to_message_id, const char *reply_markup,
        telebot_core_response_t *response);

/**
 * @brief Tell the user that something is happening on the bot's side. The status
 * is set for 5 seconds or less (when a message arrives from your bot, Telegram
 * clients clear its typing status). Example: The ImageBot needs some time to process
 * a request and upload the image. Instead of sending a text message along the lines
 * of "Retrieving image, please wait…", the bot may use #telebot_core_send_chat_action()
 * with action = upload_photo. The user will see a "sending photo" status for the bot.
 * It is only recommended to use when a response from the bot will take a noticeable
 * amount of time to arrive.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param[in] action Type of action to broadcast. Choose one, depending on what the
 * user is about to receive: typing for text messages, upload_photo for photos,
 * record_video or upload_video for videos, record_audio or upload_audio for
 * audio files, upload_document for general files, find_location for location
 * data.
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_send_chat_action(telebot_core_handler_t *core_h,
        long long int chat_id, const char *action, telebot_core_response_t *response);

/**
 * @brief Get user profile pictures object
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] offset Sequential number of the first photo to be returned. By default,
 * all photos are returned.
 * @param[in] limit Limits the number of photos to be retrieved. Values between
 * 1—100 are accepted. Defaults to 100.
 * @param[out] response Response data that contains user's profile photos on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_get_user_profile_photos(telebot_core_handler_t *core_h,
        int user_id, int offset, int limit, telebot_core_response_t *response);

/**
 * @brief Get basic info about a file and prepare it for
 * downloading. For the moment, bots can download files of up to 20MB in size.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] file_id File identifier to get info about.
 * @param[out] response Response data that contains file object on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_get_file(telebot_core_handler_t *core_h, const char *file_id,
        telebot_core_response_t *response);

/**
 * @brief Download file using file_path obtained with
 * telebot_core_get_file(). It is guaranteed that the link will be valid for
 * at least 1 hour. When the link expires, a new one can be requested by
 * calling telebot_core_get_file() again.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] file_path A file path take from the response of telebot_core_get_file()
 * @param[in] out_file Full path to download and save file.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value. No response, i.e., core_h->resp_data contains nothing.
 */
telebot_error_e telebot_core_download_file(telebot_core_handler_t *core_h,
        const char *file_path, const char *out_file);

/**
 * @brief Kick a user from a group, a supergroup or a channel. In the case of
 * supergroups and channels, the user will not be able to return to the group on
 * their own using invite links, etc., unless unbanned first. The bot must be an
 * administrator in the chat for this to work and must have the appropriate admin
 * rights.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param[in] user_id Unique identifier of the target user
 * @param[in] until_date Date when the user will be unbanned, unix time. If user is
 * banned for more than 366 days or less than 30 seconds from the current time
 * they are considered to be banned forever.
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_kick_chat_member(telebot_core_handler_t *core_h,
        long long int chat_id, int user_id, long until_date,
        telebot_core_response_t *response);

/**
 * @brief Unban a previously kicked user in a supergroup or channel. The user will
 * not return to the group or channel automatically, but will be able to join via
 * link, etc. The bot must be an administrator for this to work.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param[in] user_id Unique identifier of the target user
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_unban_chat_member(telebot_core_handler_t *core_h,
        long long int chat_id, int user_id, telebot_core_response_t *response);

/**
 * @brief Restrict a user in a supergroup. The bot must be an administrator in
 * the supergroup for this to work and must have the appropriate admin rights.
 * Pass true for all boolean parameters to lift restrictions from a user.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] until_date Date when restrictions will be lifted for the user, unix
 * time. If user is restricted for more than 366 days or less than 30 seconds
 * from the current time, they are considered to be restricted forever.
 * @param[in] can_send_messages Pass true, if the user is allowed to send text
 * messages, contacts, locations and venues
 * @param[in] can_send_media_messages Pass true, if the user is allowed to send
 * audios, documents, photos, videos, video notes and voice notes,
 * implies can_send_messages
 * @param[in can_send_polls Pass true, if the user is allowed to send polls,
 * implies can_send_messages
 * @param[in] can_send_other_messages Pass true, if the user can send animations,
 * games, stickers and use inline bots, implies can_send_media_messages
 * @param[in] can_add_web_page_previews Pass true, if the user may add web page
 * previews to their messages, implies can_send_media_messages.
 * @param[in] can_change_info Pass true, if the user is allowed to change the
 * chat title, photo and other settings. Ignored in public supergroups.
 * @param[in] can_invite_users 	Pass true, if the user is allowed to invite
 * new users to the chat.
 * @param[in] can_pin_messages Pass true, if the user is allowed to pin messages.
 * Ignored in public supergroups
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_restrict_chat_member(telebot_core_handler_t *core_h,
        long long int chat_id, int user_id, long until_date, bool can_send_messages,
        bool can_send_media_messages, bool can_send_polls, bool can_send_other_messages,
        bool can_add_web_page_previews, bool can_change_info, bool can_invite_users,
        bool can_pin_messages, telebot_core_response_t *response);

/**
 * @brief Promote or demote a user in a supergroup or a channel. The bot must be
 * an administrator in the chat for this to work and must have the appropriate
 * admin rights. Pass False for all boolean parameters to demote a user.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] can_change_info Pass true, if the administrator can change chat
 * title, photo and other settings.
 * @param[in] can_post_messages Pass true, if the administrator can create channel
 * posts, channels only.
 * @param[in] can_edit_messages Pass true, if the administrator can edit messages
 * of other users and can pin messages, channels only.
 * @param[in] can_delete_messages Pass true, if the administrator can delete
 * messages of other users.
 * @param[in] can_invite_users Pass true, if the administrator can invite new users
 * to the chat.
 * @param[in] can_restrict_members Pass true, if the administrator can restrict,
 * ban or unban chat members.
 * @param[in] can_pin_messages Pass true, if the administrator can pin messages,
 * supergroups only.
 * @param[in] can_promote_members Pass true, if the administrator can add new
 * administrators with a subset of his own privileges or demote administrators
 * that he has promoted, directly or indirectly (promoted by administrators
 * that were appointed by him).
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_promote_chat_member(telebot_core_handler_t *core_h,
        long long int chat_id, int user_id, bool can_change_info, bool can_post_messages,
        bool can_edit_messages, bool can_delete_messages, bool can_invite_users,
        bool can_restrict_members, bool can_pin_messages, bool can_promote_members,
        telebot_core_response_t *response);

/**
 * @brief Export an invite link to a supergroup or a channel. The bot must be
 * an administrator in the chat for this to work and must have the appropriate
 * admin rights.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] custom_title New custom title for the administrator; 0-16 characters,
 * emoji are not allowed.
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_set_chat_admin_custom_title(telebot_core_handler_t *core_h,
        long long int chat_id, int user_id, const char *custom_title,
        telebot_core_response_t *response);

/**
 * @brief Set default chat permissions for all members. The bot must be an administrator
 * in the group or a supergroup for this to work and must have the 'can_restrict_members'
 * admin rights.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param[in] can_send_messages Pass true, if the user is allowed to send text
 * messages, contacts, locations and venues
 * @param[in] can_send_media_messages Pass true, if the user is allowed to send
 * audios, documents, photos, videos, video notes and voice notes,
 * implies can_send_messages
 * @param[in can_send_polls Pass true, if the user is allowed to send polls,
 * implies can_send_messages
 * @param[in] can_send_other_messages Pass true, if the user can send animations,
 * games, stickers and use inline bots, implies can_send_media_messages
 * @param[in] can_add_web_page_previews Pass true, if the user may add web page
 * previews to their messages, implies can_send_media_messages.
 * @param[in] can_change_info Pass true, if the user is allowed to change the
 * chat title, photo and other settings. Ignored in public supergroups.
 * @param[in] can_invite_users 	Pass true, if the user is allowed to invite
 * new users to the chat.
 * @param[in] can_pin_messages Pass true, if the user is allowed to pin messages.
 * Ignored in public supergroups
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_set_chat_permissions(telebot_core_handler_t *core_h,
        long long int chat_id, bool can_send_messages, bool can_send_media_messages,
        bool can_send_polls, bool can_send_other_messages, bool can_add_web_page_previews,
        bool can_change_info, bool can_invite_users, bool can_pin_messages,
        telebot_core_response_t *response);

/**
 * @brief Generate a new invite link for a chat; any previously generated link is
 * revoked. The bot must be an administrator in the chat for this to work and must
 * have the appropriate admin rights.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[out] response Response data that contains the new invite link on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_export_chat_invite_link(telebot_core_handler_t *core_h,
        long long int chat_id, telebot_core_response_t *response);

/**
 * @brief Set a new profile photo for the chat. Photos can't be changed for private
 * chats. The bot must be an administrator in the chat for this to work and must
 * have the appropriate admin rights.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] photo New chat photo file path.
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_set_chat_photo(telebot_core_handler_t *core_h,
        long long int chat_id, const char *photo, telebot_core_response_t *response);

/**
 * @brief Delete a chat photo. Photos can't be changed for private chats. The bot
 * must be an administrator in the chat for this to work and must have the
 * appropriate admin rights.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_delete_chat_photo(telebot_core_handler_t *core_h,
        long long int chat_id, telebot_core_response_t *response);

/**
 * @brief Change the title of a chat. Titles can't be
 * changed for private chats. The bot must be an administrator in the chat for
 * this to work and must have the appropriate admin rights.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] title New chat title, 1-255 characters.
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_set_chat_title(telebot_core_handler_t *core_h,
        long long int chat_id, const char *title, telebot_core_response_t *response);

/**
 * @brief Change the description of a supergroup or a channel. The bot must be
 * an administrator in the chat for this to work and must have the appropriate
 * admin rights.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] description New chat description, 0-255 characters.
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_set_chat_description(telebot_core_handler_t *core_h,
        long long int chat_id, const char *description,
        telebot_core_response_t *response);

/**
 * @brief Pin a message in a supergroup or a channel. The bot must be an administrator
 * in the chat for this to work and must have the 'can_pin_messages' admin right
 * in the supergroup or 'can_edit_messages' admin right in the channel.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] message_id Identifier of a message to pin.
 * @param[in] disable_notification  Pass True, if it is not necessary to send
 * a notification to all chat members about the new pinned message.
 * Notifications are always disabled in channels.
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_pin_chat_message(telebot_core_handler_t *core_h,
        long long int chat_id, int message_id, bool disable_notification,
        telebot_core_response_t *response);

/**
 * @brief Unpin a message in a supergroup or a channel. The bot must be an administrator
 * in the chat for this to work and must have the 'can_pin_messages' admin right
 * in the supergroup or 'can_edit_messages' admin right in the channel.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_unpin_chat_message(telebot_core_handler_t *core_h,
        long long int chat_id, telebot_core_response_t *response);

/**
 * @brief Leave a group, supergroup or channel.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_leave_chat(telebot_core_handler_t *core_h,
        long long int chat_id, telebot_core_response_t *response);

/**
 * @brief Get up to date information about the chat (current name of the user for
 * one-on-one conversations, current username of a user, group or channel, etc).
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[out] response Response data that contains a Chat object on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_get_chat(telebot_core_handler_t *core_h,
        long long int chat_id, telebot_core_response_t *response);

/**
 * @brief Get a list of administrators in a chat. Response contains an array of
 * objects that contains information about all chat administrators except other
 * bots. If the chat is a group or a supergroup and no administrators were
 * appointed, only the creator will be returned.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[out] response Response data that contains a array of chat members on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_get_chat_admins(telebot_core_handler_t *core_h,
        long long int chat_id, telebot_core_response_t *response);

/**
 * @brief Get the number of members in a chat.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[out] response Response data that contains number of chat members on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_get_chat_members_count(telebot_core_handler_t *core_h,
        long long int chat_id, telebot_core_response_t *response);

/**
 * @brief Get information about a member of a chat.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] user_id Unique identifier of the target user.
 * @param[out] response Response data that contains a chat member on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_get_chat_member(telebot_core_handler_t *core_h,
        long long int chat_id, int user_id, telebot_core_response_t *response);

/**
 * @brief Set a new group sticker set for a supergroup. The bot must be an administrator
 * in the chat for this to work and must have the appropriate admin rights. Use the field
 * can_set_sticker_set optionally returned in #telebot_core_get_chat requests to check
 * if the bot can use it.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] sticker_set_name Name of the sticker set to be set as the group
 * sticker set.
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_set_chat_sticker_set(telebot_core_handler_t *core_h,
        long long int chat_id, const char *sticker_set_name,
        telebot_core_response_t *response);

/**
 * @brief Delete a group sticker set from a supergroup. The bot must be an administrator
 * in the chat for this to work and must have the appropriate admin rights. Use the
 * field can_set_sticker_set optionally returned in #telebot_core_get_chat requests
 * to check if the bot can use this method.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_delete_chat_sticker_set(telebot_core_handler_t *core_h,
        long long int chat_id, telebot_core_response_t *response);

/**
 * @brief Send answers to callback queries sent from inline keyboards.
 * The answer will be displayed to the user as a notification at the top of
 * the chat screen or as an alert.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] callback_query_id Unique identifier for the query to be answered.
 * @param[in] text Text of the notification. If not specified, nothing will be shown
 * to the user, 0-200 characters
 * @param[in] show_alert If true, an alert will be shown by the client instead of
 * a notification at the top of the chat screen. Defaults to false.
 * @param[in] url URL that will be opened by the user's client. If you have created
 * a Game and accepted the conditions via @Botfather, specify the URL that opens
 * your game - note that this will only work if the query comes from a
 * callback_game button.
 * @param[in] cache_time The maximum amount of time in seconds that the result of
 * the callback query may be cached client-side. Telegram apps will support
 * caching starting in version 3.14.
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_answer_callback_query(telebot_core_handler_t *core_h,
        const char *callback_query_id, const char *text, bool show_alert,
        const char *url, int cache_time, telebot_core_response_t *response);

/**
 * @brief Change the list of the bot's commands.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] commands A JSON-serialized list of bot commands to be set as the
 * list of the bot's commands. At most 100 commands can be specified.
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_set_my_commands(telebot_core_handler_t *core_h,
        const char *commands, telebot_core_response_t *response);

/**
 * @brief Get the current list of the bot's commands..
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[out] response Response data that contains "True" on success.
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_get_my_commands(telebot_core_handler_t *core_h,
        telebot_core_response_t *response);

/**
 * @brief Edit text and game messages sent by the bot or via the bot (for inline bots).
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Required if inline_message_id. Unique identifier for the target
 * chat or username of the target channel (in the format \@channelusername).
 * @param[in] message_id Required if inline_message_id is not specified.
 * Identifier of the sent message.
 * @param[in] inline_message_id Required if chat_id and message_id are not
 * specified. Identifier of the inline message.
 * @param[in] text New text of the message.
 * @param[in] parse_mode Send Markdown or HTML, if you want Telegram apps to show
 * bold, italic, fixed-width text or inline URLs in your bot's message.
 * @param[in] disable_web_page_priview Disables link previews for links in this message.
 * @param[in] reply_markup A JSON-serialized object for an inline keyboard.
 * @param[out] response Response data that contains the message sent on succes,
 * otherwise "True". It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_edit_message_text(telebot_core_handler_t *core_h,
        long long int chat_id, int message_id, const char *inline_message_id,
        const char *text, const char *parse_mode, bool disable_web_page_preview,
        const char *reply_markup, telebot_core_response_t *response);

/**
 * @brief Edit captions of messages sent by the bot or via the bot (for inline bots).
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Required if inline_message_id. Unique identifier for the target
 * chat or username of the target channel (in the format \@channelusername).
 * @param[in] message_id Required if inline_message_id is not specified.
 * Identifier of the sent message.
 * @param[in] inline_message_id Required if chat_id and message_id are not
 * specified. Identifier of the inline message.
 * @param[in] caption New caption of the message.
 * @param[in] reply_markup A JSON-serialized object for an inline keyboard.
 * @param[out] response Response data that contains the message sent on succes,
 * otherwise "True". It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_edit_message_caption(telebot_core_handler_t *core_h,
        long long int chat_id, int message_id, const char *inline_message_id,
        const char *caption, const char *parse_mode, const char *reply_markup,
        telebot_core_response_t *response);

/**
 * @brief Edit only the reply markup of messages sent by the bot or via the
 * bot (for inline bots).
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Required if inline_message_id. Unique identifier for the target
 * chat or username of the target channel (in the format \@channelusername).
 * @param[in] message_id Required if inline_message_id is not specified.
 * Identifier of the sent message.
 * @param[in] inline_message_id Required if chat_id and message_id are not
 * specified. Identifier of the inline message.
 * @param[in] reply_markup A JSON-serialized object for an inline keyboard.
 * @param[out] response Response data that contains the message sent on succes,
 * otherwise "True". It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */

telebot_error_e telebot_core_edit_message_reply_markup(telebot_core_handler_t *core_h,
        long long int chat_id, int message_id, const char *inline_message_id,
        const char *reply_markup, telebot_core_response_t *response);

/**
 * @brief Stop a poll which was sent by the bot.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Required if inline_message_id. Unique identifier for the target
 * chat or username of the target channel (in the format \@channelusername).
 * @param[in] message_id Required if inline_message_id is not specified.
 * Identifier of the sent message.
 * @param[in] inline_message_id Required if chat_id and message_id are not
 * specified. Identifier of the inline message.
 * @param[in] reply_markup A JSON-serialized object for an inline keyboard.
 * @param[out] response Response data that contains the message sent on succes,
 * otherwise "True". It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */

telebot_error_e telebot_core_stop_poll(telebot_core_handler_t *core_h,
        long long int chat_id, int message_id, const char *reply_markup,
        telebot_core_response_t *response);


/**
 * @brief Delete a message, including service messages, with the following
 limitations:
 *  - A message can only be deleted if it was sent less than 48 hours ago.
 *  - Bots can delete outgoing messages in groups and supergroups.
 *  - Bots granted can_post_messages permissions can delete outgoing messages
 *    in channels.
 *  - If the bot is an administrator of a group, it can delete any message there.
 *  - If the bot has can_delete_messages permission in a supergroup or a channel,
 *    it can delete any message there.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target message_id Message identifier to be deleted.
 * @param[out] response Response data that contains "True" on succes,
 * It MUST be freed with #telebot_core_put_response().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_delete_message(telebot_core_handler_t *core_h,
        long long int chat_id, int message_id, telebot_core_response_t *response);


/**
 * @brief Release response data obtained with telebot core methods.
 * @param[in] response Pointer to response to release.
 */
void telebot_core_put_response(telebot_core_response_t *response);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_CORE_API_H__ */
