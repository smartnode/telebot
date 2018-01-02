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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        telebot-core-api.h
 * @ingroup     TELEBOT_CORE_API
 * @brief       This file contains core API for the telegram bot interface
 * @author      Elmurod Talipov
 * @date        2015-10-17
 * @version     0.1
 */

/**
 * @defgroup TELEBOT_CORE_API		Core Telegram Bot API
 * @brief The APIs for the telegram bot interface
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
    char *resp_data; /**< Telegam response object */
    size_t resp_size; /**< Telegam response size */
    bool busy; /**< Mark another request is in progress */
} telebot_core_handler_t;

/**
 * @brief Start function to use telebot core APIs.
 *
 * This function must be used first to call, and it creates handler that is used
 * as input to other functions in telebot core interface. This call MUST have
 * corresponding call to #telebot_core_destroy when operation is complete.
 * @param core_h A pointer to a handler, which will be allocated and created.
 * @param token Telegram bot token to use.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_create(telebot_core_handler_t **core_h, char *token);

/**
 * @brief Final function to use telebot core APIs
 *
 * This function must be the last function to call for a telebot core use.
 * It is the opposite of the telebot_core_create function and MUST be called
 * with the same handler as the input that a telebot_core_create call
 * created.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_destroy(telebot_core_handler_t *core_h);

/**
 * @brief This function is used to receive incoming updates (long polling).
 * It will not work if an outgoing webhook is set up. In order to avoid getting
 * duplicate updates, recalculate offset after each server response.
 * @param core_h Telebot core handler creating using telebot_core_create().
 * @param offset Identifier of the first update to be returned. Must be greater
 * by one than the highest among the identifiers of previously received updates.
 * By default, updates starting with the earliest unconfirmed update are returned.
 * An update is considered confirmed as soon as the function is called with an
 * offset higher than its update_id.
 * @param limit Limits the number of updates to be retrieved. Values between
 * 1—100 are accepted. Defaults to 100.
 * @param timeout Timeout in seconds for long polling. Defaults to 0, i.e. usual
 * short polling.
 * @param allowed_updates List the types of update you want your bot to receive.
 * Specify an empty list to receive all updates regardless of type (default).
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data. It MUST be freed after use.
 */
telebot_error_e telebot_core_get_updates(telebot_core_handler_t *core_h,
        int offset, int limit, int timeout, const char *allowed_updates);

/**
 * @brief This function is used to specify a url and receive incoming updates
 * via an outgoing webhook. Whenever there is an update for the bot, we will
 * send an HTTPS POST request to the specified url, containing a JSON-serialized
 * Update. In case of an unsuccessful request, we will give up after a reasonable
 * amount of attempts.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param url HTTPS url to send updates to. Use an empty string to remove webhook
 * integration
 * @param certificate A path to to a public key certificate to upload server.
 * @param max_connections Optional  Maximum allowed number of simultaneous
 * HTTPS connections to the webhook for update delivery, 1-100. Defaults to 40.
 * Use lower values to limit the load on your bot's server, and higher values
 * to increase your bot's throughput.
 * @param allowed_updates List the types of updates you want your bot to
 * receive. For example, specify ["message", "edited_channel_post",
 * "callback_query"] to only receive updates of these types.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data. All core API responses are JSON objects.
 * It MUST be freed after use.
 */
telebot_error_e telebot_core_set_webhook(telebot_core_handler_t *core_h, char *url,
        char *certificate, int max_connections, char *allowed_updates);

/**
 * @brief This function is used to remove webhook integration if you decide to
 * switch back to getUpdates.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data. It MUST be freed after use.
 */
telebot_error_e telebot_core_delete_webhook(telebot_core_handler_t *core_h);

/**
 * @brief This function is used to get current webhook status.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data. It MUST be freed after use.
 */
telebot_error_e telebot_core_get_webhook_info(telebot_core_handler_t *core_h);

/**
 * @brief This function gets basic information about the bot.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data. All core API responses are JSON objects.
 * It MUST be freed after use.
 */
telebot_error_e telebot_core_get_me(telebot_core_handler_t *core_h);

/**
 * @brief This function is used to send text messages.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param text Text of the message to be sent.
 * @param parse_mode Send Markdown, if you want Telegram apps to show bold,
 * italic and inline URLs in your bot's message.
 * @param disable_web_page_preview Disables link previews for links in this message.
 * @param disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom
 * reply keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data. It MUST be freed after use.
 */
telebot_error_e telebot_core_send_message(telebot_core_handler_t *core_h, int chat_id,
        char *text, char *parse_mode, bool disable_web_page_preview,
        bool disable_notification, int reply_to_message_id, const char *reply_markup);

/**
 * @brief This function is used to forward messages of any kind.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param from_chat_id Unique identifier for the chat where the original
 * message was sent (or channel username in the format \@channelusername).
 * @param disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param message_id Unique message identifier.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data that contains the sent message.
 * It MUST be freed after use.
 */
telebot_error_e telebot_core_forward_message(telebot_core_handler_t *core_h,
        int chat_id, int from_chat_id, bool disable_notification, int message_id);

/**
 * @brief This functionis used to send photos.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param photo Photo to send. It is either file_id as String to resend a photo
 * that is already on the Telegram servers, or a path to photo file.
 * @param is_file False if photo is file_id, true, if photo is a file path.
 * @param caption Photo caption. (may also be used when resending photos).
 * @param disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom
 * reply keyboard, instructions to hide keyboard or to force a reply from
 * the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data that contains the sent message.
 * It MUST be freed after use.
 */
telebot_error_e telebot_core_send_photo(telebot_core_handler_t *core_h, int chat_id,
        char *photo, bool is_file, char *caption, bool disable_notification,
        int reply_to_message_id, char *reply_markup);
/**
 * @brief This function is used to to send audio files. if you want Telegram
 * clients to display them in the music player. Your audio must be in the .mp3
 * format. Bots can currently send audio files of up to 50 MB in size. For
 * backward compatibility, when the fields title and performer are both empty
 * and the mime-type of the file to be sent is not audio/mpeg, the file will be
 * sent as a playable voice message. For this to work, the audio must be in
 * an .ogg file encoded with OPUS. For sending voice messages, use the
 * telegram_core_send_voice() function instead.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param audio Audio file to send. It is either a file_id as String to resend an
 * audio that is already on the Telegram servers, or a path to audio file.
 * @param is_file False if audio is file_id, true, if audio is a file path.
 * @param duration Duration of sent audio in seconds.
 * @param performer The performer of the audio.
 * @param title The track name of the audio.
 * @param disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data that contains the sent message.
 * It MUST be freed after use.
 */
telebot_error_e telebot_core_send_audio(telebot_core_handler_t *core_h, int chat_id,
        char *audio, bool is_file, int duration, char *performer, char *title,
        bool disable_notification, int reply_to_message_id, char *reply_markup);

/**
 * @brief This function is used to send general files.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param document Document file to send. It is either a file_id as String to
 * resend a file that is already on the Telegram servers, or a path to file.
 * @param is_file False if document is file_id, true, if document is a file path.
 * @param disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data that contains the sent message.
 * It MUST be freed after use.
 */
telebot_error_e telebot_core_send_document(telebot_core_handler_t *core_h,
        int chat_id, char *document, bool is_file, bool disable_notification,
        int reply_to_message_id, char *reply_markup);

/**
 * @brief This function is used to send video files, Telegram clients support
 * mp4 videos (other formats may be sent as Document).
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param video Video file to send. It is either a file_id as String to resend
 * a video that is already on the Telegram servers, or a path to video file.
 * @param is_file False if video is file_id, true, if video is a file path.
 * @param duration Duration of sent video in seconds.
 * @param caption Video caption (may also be used when resending videos).
 * @param disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data that contains the sent message.
 * It MUST be freed after use.
 */
telebot_error_e telebot_core_send_video(telebot_core_handler_t *core_h,
        int chat_id, char *video, bool is_file, int duration, char *caption,
        bool disable_notification, int reply_to_message_id, char *reply_markup);

/**
 * @brief This function is used to send audio files, if you want Telegram
 * clients to display the file as a playable voice message. For this to work,
 * your audio must be in an .ogg file encoded with OPUS (other formats may be
 * sent as Audio or Document). Bots can currently send voice messages of up to
 * 50 MB in size.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param voice Audio file to send. It is either a file_id as String to resend
 * a audio that is already on the Telegram servers, or a path to audio file.
 * @param is_file False if voice is file_id, true, if voice is a file path.
 * @param duration Duration of sent voice/audio in seconds.
 * @param disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data that contains the sent message.
 * It MUST be freed after use.
 */
telebot_error_e telebot_core_send_voice(telebot_core_handler_t *core_h,
        int chat_id, char *voice, bool is_file, int duration,
        bool disable_notification, int reply_to_message_id, char *reply_markup);


/**
 * @brief This function is used to send video messages.
 * As of v.4.0, Telegram clients support rounded square mp4 videos of up to 1 minute long.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param voice Audio file to send. It is either a file_id as String to resend
 * a audio that is already on the Telegram servers, or a path to audio file.
 * @param is_file False if voice is file_id, true, if voice is a file path.
 * @param duration Duration of sent voice/audio in seconds.
 * @param disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data that contains the sent message.
 * It MUST be freed after use.
 */
telebot_error_e telebot_core_send_video_note(telebot_core_handler_t *core_h,
        int chat_id, char *video_note, bool is_file, int duration, int length,
        bool disable_notification, int reply_to_message_id, char *reply_markup);

/**
 * @brief This function is used to send point on the map.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param latitude Latitude of location.
 * @param longitude Longitude of location.
 * @param disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data that contains the sent message.
 * It MUST be freed after use.
 */
telebot_error_e telebot_core_send_location(telebot_core_handler_t *core_h,
        int chat_id, float latitude, float longitude, bool disable_notification,
        int reply_to_message_id, char *reply_markup);

/**
 * @brief This function is used to edit live location messages sent by the bot or via
 * the bot (for inline bots). A location can be edited until its live_period
 * expires or editing is explicitly disabled by a call to
 * #telebot_core_stop_message_live_location().
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param message_id Required if inline_message_id is not specified.
 * Identifier of the sent message.
 * @param inline_message_id Required if chat_id and message_id are not
 * specified. Identifier of the inline message.
 * @param latitude Latitude of location.
 * @param longitude Longitude of location.
 * @param disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param reply_markup A JSON-serialized object for a new inline keyboard.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data that contains the sent message.
 * It MUST be freed after use.
 */
telebot_error_e telebot_core_edit_message_live_location(telebot_core_handler_t *core_h,
        int chat_id, int message_id, char *inline_message_id, float latitude,
        float longitude, bool disable_notification, char *reply_markup);

/**
 * @brief This function is used to stop updating a live location message sent by the
 * bot or via the bot (for inline bots) before live_period expires.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param message_id Required if inline_message_id is not specified.
 * Identifier of the sent message.
 * @param inline_message_id Required if chat_id and message_id are not
 * specified. Identifier of the inline message.
 * @param reply_markup A JSON-serialized object for a new inline keyboard.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data that contains the sent message.
 * It MUST be freed after use.
 */
telebot_error_e telebot_core_stop_message_live_location(telebot_core_handler_t *core_h,
        int chat_id, int message_id, char *inline_message_id, char *reply_markup);

/**
 * @brief This function is used to send information about a venue.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param latitude Latitude of venue.
 * @param longitude Longitude of venue.
 * @param title Name of the venue
 * @param address Address of the venue.
 * @param foursquare_id Foursquare identifier of the venue.
 * @param disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. A JSON-serialized
 * object for an inline keyboard, custom reply keyboard, instructions to remove
 * reply keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data that contains the sent message.
 * It MUST be freed after use.
 */
telebot_error_e telebot_core_send_venue(telebot_core_handler_t *core_h, int chat_id,
        float latitude, float longitude, char *title, char *address, char *foursquare_id,
        bool disable_notification, int reply_to_message_id, char *reply_markup);

/**
 * @brief This function is used to send phone contacts.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param phone_number Contact's phone numbers.
 * @param first_name Contact's first name.
 * @param last_name Contact's last name.
 * @param disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. A JSON-serialized
 * object for an inline keyboard, custom reply keyboard, instructions to remove
 * reply keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data that contains the sent message.
 * It MUST be freed after use.
 */
telebot_error_e telebot_core_send_contact(telebot_core_handler_t *core_h, int chat_id,
        char *phone_number, char *first_name, char *last_name, bool disable_notification,
        int reply_to_message_id, char *reply_markup);

/**
 * @brief This function is used to tell the user that something is happening on
 * the bot's side. The status is set for 5 seconds or less (when a message
 * arrives from your bot, Telegram clients clear its typing status).
 * Example: The ImageBot needs some time to process a request and upload the
 * image. Instead of sending a text message along the lines of “Retrieving image,
 * please wait…”, the bot may use sendChatAction with action = upload_photo.
 * The user will see a “sending photo” status for the bot.
 * It is only recommended to use when a response from the bot will take a
 * noticeable amount of time to arrive.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param action Type of action to broadcast. Choose one, depending on what the
 * user is about to receive: typing for text messages, upload_photo for photos,
 * record_video or upload_video for videos, record_audio or upload_audio for
 * audio files, upload_document for general files, find_location for location
 * data.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data that contains the sent message.
 * It MUST be freed after use.
 */
telebot_error_e telebot_core_send_chat_action(telebot_core_handler_t *core_h,
        int chat_id, char *action);

/**
 * @brief This function is used to get user profile pictures object
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param user_id Unique identifier of the target user.
 * @param offset Sequential number of the first photo to be returned. By default,
 * all photos are returned.
 * @param limit Limits the number of photos to be retrieved. Values between
 * 1—100 are accepted. Defaults to 100.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data. It MUST be freed after use.
 */
telebot_error_e telebot_core_get_user_profile_photos(telebot_core_handler_t *core_h,
        int user_id, int offset, int limit);

/**
 * @brief This function is used get basic info about a file and prepare it for
 * downloading. For the moment, bots can download files of up to 20MB in size.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param file_id File identifier to get info about.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data, which contains a File object.
 * It MUST be freed after use.
 */
telebot_error_e telebot_core_get_file(telebot_core_handler_t *core_h, char *file_id);

/**
 * @brief This function is used download file using file_path obtained with
 * telebot_core_get_file(). It is guaranteed that the link will be valid for
 * at least 1 hour. When the link expires, a new one can be requested by
 * calling telebot_core_get_file() again.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param file_path A file path take from the response of telebot_core_get_file()
 * @param out_file Full path to download and save file.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_download_file(telebot_core_handler_t *core_h,
        char *file_path, char *out_file);

/**
 * @brief This function is used to delete messages.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target message_id Message identifier to be deleted.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data. It MUST be freed after use.
 */
telebot_error_e telebot_core_delete_message(telebot_core_handler_t *core_h,
        int chat_id, int message_id);

/**
 * @brief This function is used to to send .webp stickers.
 * @param core_h The telebot core handler created with #telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param sticker Sticker file to send. It is either a file_id as String to
 * resend a sticker that is already on the Telegram servers, or a path to file.
 * @param is_file False if sticker is file_id, true, if sticker is a file path.
 * @param disable_notification Sends the message silently. Users will receive a
 * notification with no sound.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 * Response is placed in core_h->resp_data that contains the sent message.
 * It MUST be freed after use.
 */
telebot_error_e telebot_core_send_sticker(telebot_core_handler_t *core_h,
        int chat_id, char *sticker, bool is_file, bool disable_notification,
        int reply_to_message_id, char *reply_markup);


telebot_error_e telebot_core_answer_callback_query(telebot_core_handler_t *core_h,
        const char *callback_query_id,char *text, bool show_alert,
        char *url, int cache_time);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_CORE_API_H__ */
