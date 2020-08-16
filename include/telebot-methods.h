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
telebot_error_e telebot_create(telebot_handler_t *handle, char *token);

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
        int limit, int timeout, telebot_update_type_e allowed_updates[],
        int allowed_updates_count, telebot_update_t **updates, int *count);

/**
 * @brief This function is used to release memory used for obtained updates.
 *
 * @param[int] updates Pointer to udpates obtained with #telebot_get_updates.
 * @param[int] count Number of updates obtained with #telebot_get_updates.
 * @return On success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_put_updates(telebot_update_t *updates, int count);


/**
 * @brief This function is used to specify a url and receive incoming updates
 * via an outgoing webhook. Whenever there is an update for the bot, we will
 * send an HTTPS POST request to the specified url, containing a JSON-serialized
 * Update. In case of an unsuccessful request, we will give up after a reasonable
 * amount of attempts.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] url HTTPS url to send updates to. Use an empty string to remove
 * webhook integration
 * @param[in] certificate A path to to a public key certificate to upload server.
 * @param[in] max_connections Optional  Maximum allowed number of simultaneous
 * HTTPS connections to the webhook for update delivery, 1-100. Defaults to 40.
 * Use lower values to limit the load on your bot's server, and higher values
 * to increase your bot's throughput.
 * @param[in] allowed_updates Array of the update types you want your bot to
 * receive.
 * @param[in] allowed_updates_count Size of array of the update types.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_set_webhook(telebot_handler_t handle, char *url,
    char *certificate, int max_connections,telebot_update_type_e allowed_updates[],
    int allowed_updates_count);

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
 * @brief Send text messages.
 * @param[in] handle The telebot handler created with #telebot_create().
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
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_send_message(telebot_handler_t handle, long long int chat_id,
    const char *text, const char *parse_mode, bool disable_web_page_preview,
    bool disable_notification, int reply_to_message_id, const char *reply_markup);

/**
 * @brief Forward messages of any kind.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] from_chat_id Unique identifier for the chat where the original
 * message was sent (or channel username in the format \@channelusername).
 * @param[in] disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param[in] message_id Unique message identifier.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_forward_message(telebot_handler_t handle, long long int chat_id,
    long long int from_chat_id, bool disable_notification, int message_id);

/**
 * @brief Send photos.
 * @param[in] handle The telebot handler created with #telebot_create().
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
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_send_photo(telebot_handler_t handle, long long int chat_id,
    const char *photo, bool is_file, const char *caption, const char *parse_mode,
    bool disable_notification, int reply_to_message_id, const char *reply_markup);


/**
 * @brief Send audio files. if you want Telegram clients to display them in the
 * music player. Your audio must be in the .mp3 format. Bots can currently send
 * audio files of up to 50 MB in size. For backward compatibility, when the fields
 * title and performer are both empty and the mime-type of the file to be sent is
 * not audio/mpeg, the file will be sent as a playable voice message. For this to
 * work, the audio must be in an .ogg file encoded with OPUS. For sending voice
 * messages, use the telegram_send_voice() function instead.
 * @param[in] handle The telebot handler created with #telebot_create().
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
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_send_audio(telebot_handler_t handle, long long int chat_id,
    const char *audio, bool is_file, const char *caption, const char *parse_mode,
    int duration, const char *performer, const char *title, const char *thumb,
    bool disable_notification, int reply_to_message_id, const char *reply_markup);

/**
 * @brief Send general files.
 * @param[in] handle The telebot handler created with #telebot_create().
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
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_send_document(telebot_handler_t handle, long long int chat_id,
    const char *document, bool is_file, const char *thumb, const char *caption,
    const char *parse_mode, bool disable_notification, int reply_to_message_id,
    const char *reply_markup);

/**
 * @brief Send video files, Telegram clients support mp4 videos (other formats
 * may be sent as Document).
 * @param[in] handle The telebot handler created with #telebot_create().
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
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_send_video(telebot_handler_t handle, long long int chat_id,
    const char *video, bool is_file, int duration, int width, int height,
    const char *thumb, const char *caption, const char *parse_mode,
    bool supports_streaming, bool disable_notification, int reply_to_message_id,
    const char *reply_markup);

/**
 * @brief Send animation files (GIF or H.264/MPEG-4 AVC without sound).
 * @param[in] handle The telebot handler created with #telebot_create().
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
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_send_animation(telebot_handler_t handle, long long int chat_id,
    const char *animation, bool is_file, int duration, int width, int height,
    const char *thumb, const char *caption, const char *parse_mode,
    bool disable_notification, int reply_to_message_id, const char *reply_markup);


/**
 * @brief Send audio files, if you want Telegram clients to display the file as
 * a playable voice message. For this to work, your audio must be in an .ogg file
 * encoded with OPUS (other formats may be sent as Audio or Document).
 * @param[in] handle The telebot handler created with #telebot_create().
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
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_send_voice(telebot_handler_t handle, long long int chat_id,
    const char *voice, bool is_file, const char *caption, const char *parse_mode,
    int duration, bool disable_notification, int reply_to_message_id,
    const char *reply_markup);

/**
 * @brief Send video messages. As of v.4.0, Telegram clients support rounded square
 * mp4 videos of up to 1 minute long.
 * @param[in] handle The telebot handler created with #telebot_create().
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
 * It MUST be freed with #telebot_put_response().
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_send_video_note(telebot_handler_t handle, long long int chat_id,
    char *video_note, bool is_file, int duration, int length, const char *thumb,
    bool disable_notification, int reply_to_message_id, const char *reply_markup);

/**
 * @brief Send point on the map.
 * @param[in] handle The telebot handler created with #telebot_create().
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
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_send_location(telebot_handler_t handle, long long int chat_id,
    float latitude, float longitude, int live_period, bool disable_notification,
    int reply_to_message_id, const char *reply_markup);

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
 * @brief Send information about a venue.
 * @param[in] handle The telebot handler created with #telebot_create().
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
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_send_venue(telebot_handler_t handle, long long int chat_id,
    float latitude, float longitude, const char *title, const char *address,
    const char *foursquare_id, const char *foursquare_type, bool disable_notification,
    int reply_to_message_id, const char *reply_markup);

/**
 * @brief Send phone contacts.
 * @param[in] handle The telebot handler created with #telebot_create().
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
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_send_contact(telebot_handler_t handle, long long int chat_id,
    const char *phone_number, const char *first_name, const char *last_name,
    const char *vcard, bool disable_notification, int reply_to_message_id,
    const char *reply_markup);

/**
 * @brief Send a native poll.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] question Poll question, 1-255 characters.
 * @param[in] options A aray of answer options, 2-10 strings 1-100 characters each.
 * @param[in] count_options Number of answer options.
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
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_send_poll(telebot_handler_t handle, long long int chat_id,
    const char *question, const char **options, int count_options, bool is_anonymous,
    const char *type, bool allows_multiple_answers, int correct_option_id, bool is_closed,
    bool disable_notification, int reply_to_message_id, const char *reply_markup);

/**
 * @brief Send a dice, which will have a random value from 1 to 6.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param[in] disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. A JSON-serialized
 * object for an inline keyboard, custom reply keyboard, instructions to remove
 * reply keyboard or to force a reply from the user.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_send_dice(telebot_handler_t handle, long long int chat_id,
    bool disable_notification, int reply_to_message_id, const char *reply_markup);

/**
 * @brief Tell the user that something is happening on the bot's side. The status
 * is set for 5 seconds or less (when a message arrives from your bot, Telegram
 * clients clear its typing status). Example: The ImageBot needs some time to process
 * a request and upload the image. Instead of sending a text message along the lines
 * of "Retrieving image, please wait…", the bot may use #telebot_send_chat_action()
 * with action = upload_photo. The user will see a "sending photo" status for the bot.
 * It is only recommended to use when a response from the bot will take a noticeable
 * amount of time to arrive.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param[in] action Type of action to broadcast. Choose one, depending on what the
 * user is about to receive: typing for text messages, upload_photo for photos,
 * record_video or upload_video for videos, record_audio or upload_audio for
 * audio files, upload_document for general files, find_location for location
 * data.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_send_chat_action(telebot_handler_t handle, long long int chat_id,
    char *action);

/**
 * @brief This function is used to get user profile pictures object
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] offset Sequential number of the first photo to be returned.
 * By default, up to 10 photos are returned.
 * @param[in] limit Limits the number of photos to be retrieved.
 * Values between 1-100 are accepted. Defaults to 100.
 * @param[out] photos A pointer to user profile photo object. It MUST be
 * released  with #telebot_put_user_profile_photos after use.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_get_user_profile_photos(telebot_handler_t handle,
        int user_id, int offset, int limit, telebot_user_profile_photos_t *photos);

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
 * @brief This function is used to download file.
 * @param[in] file_id File identifier to get info about.
 * @param[in] path A path where the file is downloaded
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_download_file(telebot_handler_t handle, const char *file_id,
    const char *path);

/**
 * @brief Kick a user from a group, a supergroup or a channel. In the case of
 * supergroups and channels, the user will not be able to return to the group on
 * their own using invite links, etc., unless unbanned first. The bot must be an
 * administrator in the chat for this to work and must have the appropriate admin
 * rights.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param[in] user_id Unique identifier of the target user
 * @param[in] until_date Date when the user will be unbanned, unix time. If user is
 * banned for more than 366 days or less than 30 seconds from the current time
 * they are considered to be banned forever.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_kick_chat_member(telebot_handler_t handle, long long int chat_id,
    int user_id, long until_date);

/**
 * @brief Unban a previously kicked user in a supergroup or channel. The user will
 * not return to the group or channel automatically, but will be able to join via
 * link, etc. The bot must be an administrator for this to work.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param[in] user_id Unique identifier of the target user
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_unban_chat_member(telebot_handler_t handle, long long int chat_id,
     int user_id);

/**
 * @brief Restrict a user in a supergroup. The bot must be an administrator in
 * the supergroup for this to work and must have the appropriate admin rights.
 * Pass true for all boolean parameters to lift restrictions from a user.
 * @param[in] handle The telebot handler created with #telebot_create().
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
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_restrict_chat_member(telebot_handler_t handle,
    long long int chat_id, int user_id, long until_date, bool can_send_messages,
    bool can_send_media_messages, bool can_send_polls, bool can_send_other_messages,
    bool can_add_web_page_previews, bool can_change_info, bool can_invite_users,
    bool can_pin_messages);

/**
 * @brief Promote or demote a user in a supergroup or a channel. The bot must be
 * an administrator in the chat for this to work and must have the appropriate
 * admin rights. Pass False for all boolean parameters to demote a user.
 * @param[in] handle The telebot handler created with #telebot_create().
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
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_promote_chat_member(telebot_handler_t handle,
    long long int chat_id, int user_id, bool can_change_info, bool can_post_messages,
    bool can_edit_messages, bool can_delete_messages, bool can_invite_users,
    bool can_restrict_members, bool can_pin_messages, bool can_promote_members);

/**
 * @brief Export an invite link to a supergroup or a channel. The bot must be
 * an administrator in the chat for this to work and must have the appropriate
 * admin rights.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] custom_title New custom title for the administrator; 0-16 characters,
 * emoji are not allowed.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_set_chat_admin_custom_title(telebot_handler_t handle,
    long long int chat_id, int user_id, const char *custom_title);

/**
 * @brief Set default chat permissions for all members. The bot must be an administrator
 * in the group or a supergroup for this to work and must have the 'can_restrict_members'
 * admin rights.
 * @param[in] handle The telebot handler created with #telebot_create().
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
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_set_chat_permissions(telebot_handler_t handle,
    long long int chat_id, bool can_send_messages, bool can_send_media_messages,
    bool can_send_polls, bool can_send_other_messages, bool can_add_web_page_previews,
    bool can_change_info, bool can_invite_users, bool can_pin_messages);

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
 * @brief Set a new profile photo for the chat. Photos can't be changed for private
 * chats. The bot must be an administrator in the chat for this to work and must
 * have the appropriate admin rights.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] photo New chat photo file path.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_set_chat_photo(telebot_handler_t handle, long long int chat_id,
    const char *photo);

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
 * @brief Get a list of administrators in a chat. Response contains an array of
 * objects that contains information about all chat administrators except other
 * bots. If the chat is a group or a supergroup and no administrators were
 * appointed, only the creator will be returned.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[out] admin Pointer to chat administrators, must be released
 * with #telebot_put_chat_admins().
 * @param[out] count Pointer to get a number of administrators.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_get_chat_admins(telebot_handler_t handle, long long int chat_id,
    telebot_chat_member_t **admin, int *count);

/**
 * @brief Release chat admins obtained with #telebot_get_chat_admins().
 * @param[in] chat Pointer to chat administrators to be released.
 * @param[in] count Number of chat administrators.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
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
 * @brief Get information about a member of a chat.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] user_id Unique identifier of the target user.
 * @param[out] member Pointer to chat member to obtain, must be released
 * with #telebot_put_chat_member().
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_get_chat_member(telebot_handler_t handle, long long int chat_id,
    int user_id, telebot_chat_member_t *member);

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
 * a Game and accepted the conditions via @Botfather, specify the URL that opens
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
 * @brief Edit text and game messages sent by the bot or via the bot (for inline bots).
 * @param[in] handle The telebot handler created with #telebot_create().
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
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_edit_message_text(telebot_handler_t handle,
    long long int chat_id, int message_id, const char *inline_message_id,
    const char *text, const char *parse_mode, bool disable_web_page_preview,
    const char *reply_markup);

/**
 * @brief Edit captions of messages sent by the bot or via the bot (for inline bots).
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Required if inline_message_id. Unique identifier for the target
 * chat or username of the target channel (in the format \@channelusername).
 * @param[in] message_id Required if inline_message_id is not specified.
 * Identifier of the sent message.
 * @param[in] inline_message_id Required if chat_id and message_id are not
 * specified. Identifier of the inline message.
 * @param[in] caption New caption of the message.
 * @param[in] reply_markup A JSON-serialized object for an inline keyboard.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
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

telebot_error_e telebot_stop_poll(telebot_handler_t handle, long long int chat_id,
    int message_id, const char *reply_markup);

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
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target message_id Message identifier to be deleted.
 * @return on Success, #TELEBOT_ERROR_NONE is returned, otherwise a negative
 * error value.
 */
telebot_error_e telebot_delete_message(telebot_handler_t handle, long long int chat_id,
    int message_id);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_METHODS_H__ */
