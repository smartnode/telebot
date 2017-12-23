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
 * @brief       This file contains the API for the telegram bot interface
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
    int  offset; /**< Telegam last update id */
    char *resp_data; /**< Telegam response object */
    size_t resp_size; /**< Telegam response size */
} telebot_core_h;

/**
 * @brief Start function to use telebot core APIs.
 *
 * This function must be used first to call, and it creates handler that is used
 * as input to other functions in telebot core interface. This call MUST have
 * corresponding call to telebot_core_destroy when operation is complete.
 * @param handler A pointer to a handler.
 * @param token Telegram bot token to use.
 * @return On success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_core_create(telebot_core_h *handler, char *token);

/**
 * @brief Final function to use telebot core APIs
 *
 * This function must be the last function to call for a telebot core use.
 * It is the opposite of the telebot_core_create function and MUST be called
 * with the same handler as the input that a telebot_core_create call
 * created.
 * @param handler The telebot handler created with telebot_core_create().
 * @return On success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_core_destroy(telebot_core_h *handler);

/**
 * @brief This function gets basic information about the bot.
 * @param handler The telebot handler created with telebot_core_create().
 * @return On success, TELEBOT_ERROR_NONE is returned. Response is placed in
 * handler->resp_data. All core API responses are JSON objects. It MUST be freed
 * after use.
 */
telebot_error_e telebot_core_get_me(telebot_core_h *handler);

/**
 * @brief This function is used to receive incoming updates (long polling).
 * It will not work if an outgoing webhook is set up. In order to avoid getting
 * duplicate updates, recalculate offset after each server response.
 * @param handler telebot handler creating using telebot_core_create().
 * @param offset Identifier of the first update to be returned. Must be greater
 * by one than the highest among the identifiers of previously received updates.
 * By default, updates starting with the earliest unconfirmed update are returned.
 * An update is considered confirmed as soon as the function is called with an
 * offset higher than its update_id.
 * @param handler The telebot handler created with telebot_core_create().
 * @param limit Limits the number of updates to be retrieved. Values between
 * 1—100 are accepted. Defaults to 100.
 * @param timeout Timeout in seconds for long polling. Defaults to 0, i.e. usual
 * short polling.
 * @return on Success, TELEBOT_ERROR_NONE is returned. Response is placed in
 * handler->resp_data. It MUST be freed after use.
 */
telebot_error_e telebot_core_get_updates(telebot_core_h *handler, int offset,
        int limit, int timeout);

/**
 * @brief This function is used to get user profile pictures object
 * @param handler The telebot handler created with telebot_core_create().
 * @param user_id Unique identifier of the target user.
 * @param offset Sequential number of the first photo to be returned. By default,
 * all photos are returned.
 * @param limit Limits the number of photos to be retrieved. Values between
 * 1—100 are accepted. Defaults to 100.
 * @return on Success, TELEBOT_ERROR_NONE is returned. Response is placed in
 * handler->resp_data. It MUST be freed after use.
 */
telebot_error_e telebot_core_get_user_profile_photos(telebot_core_h *handler,
        int user_id, int offset, int limit);

/**
 * @brief This function is used get basic info about a file and prepare it for
 * downloading. For the moment, bots can download files of up to 20MB in size.
 * @param handler The telebot handler created with telebot_core_create().
 * @param file_id File identifier to get info about.
 * @return on Success, TELEBOT_ERROR_NONE is returned. Response is placed in
 * handler->resp_data, which contains a File object. It MUST be freed after use.
 */
telebot_error_e telebot_core_get_file(telebot_core_h *handler, char *file_id);

/**
 * @brief This function is used download file using file_path obtained with
 * telebot_core_get_file(). It is guaranteed that the link will be valid for
 * at least 1 hour. When the link expires, a new one can be requested by
 * calling telebot_core_get_file() again.
 * @param handler The telebot handler created with telebot_core_create().
 * @param file_path A file path take from the response of telebot_core_get_file()
 * @param out_file Full path to download and save file.
 * @return on Success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_core_download_file(telebot_core_h *handler, char *file_path,
    char *out_file);

/**
 * @brief This function is used to send text messages.
 * @param handler The telebot handler created with telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param text Text of the message to be sent.
 * @param parse_mode Send Markdown, if you want Telegram apps to show bold,
 * italic and inline URLs in your bot's message.
 * @param disable_web_page_preview Disables link previews for links in this message.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom
 * reply keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned. Response is placed in
 * handler->resp_data. It MUST be freed after use.
 */
telebot_error_e telebot_core_send_message(telebot_core_h *handler, char *chat_id,
        char *text, char *parse_mode, bool disable_web_page_preview,
        int reply_to_message_id, const char *reply_markup);

/**
 * @brief This function is used to forward messages of any kind.
 * @param handler The telebot handler created with telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param from_chat_id Unique identifier for the chat where the original
 * message was sent (or channel username in the format \@channelusername).
 * @param message_id Unique message identifier.
 * @return on Success, TELEBOT_ERROR_NONE is returned. Response is placed in
 * handler->resp_data that contains the sent message. It MUST be freed after use.
 */
telebot_error_e telebot_core_forward_message(telebot_core_h *handler,
        char *chat_id, char *from_chat_id, int message_id);

/**
 * @brief This functionis used to send photos.
 * @param handler The telebot handler created with telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param photo Photo to send. It is either file_id as String to resend a photo
 * that is already on the Telegram servers, or a path to photo file.
 * @param is_file False if photo is file_id, true, if photo is a file path.
 * @param caption Photo caption. (may also be used when resending photos).
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom
 * reply keyboard, instructions to hide keyboard or to force a reply from
 * the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned. Response is placed in
 * handler->resp_data that contains the sent message. It MUST be freed after use.
 */
telebot_error_e telebot_core_send_photo(telebot_core_h *handler, char *chat_id,
        char *photo, bool is_file, char *caption, int reply_to_message_id,
        char *reply_markup);
/**
 * @brief This function is used to to send audio files. if you want Telegram
 * clients to display them in the music player. Your audio must be in the .mp3
 * format. Bots can currently send audio files of up to 50 MB in size. For
 * backward compatibility, when the fields title and performer are both empty
 * and the mime-type of the file to be sent is not audio/mpeg, the file will be
 * sent as a playable voice message. For this to work, the audio must be in
 * an .ogg file encoded with OPUS. For sending voice messages, use the
 * telegram_core_send_voice() function instead.
 * @param handler The telebot handler created with telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param audio Audio file to send. It is either a file_id as String to resend an
 * audio that is already on the Telegram servers, or a path to audio file.
 * @param is_file False if audio is file_id, true, if audio is a file path.
 * @param duration Duration of sent audio in seconds.
 * @param performer The performer of the audio.
 * @param title The track name of the audio.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned. Response is placed in
 * handler->resp_data that contains the sent message. It MUST be freed after use.
 */
telebot_error_e telebot_core_send_audio(telebot_core_h *handler, char *chat_id,
        char *audio, bool is_file, int duration, char *performer, char *title,
    int reply_to_message_id, char *reply_markup);

/**
 * @brief This function is used to send general files.
 * @param handler The telebot handler created with telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param document Document file to send. It is either a file_id as String to
 * resend a file that is already on the Telegram servers, or a path to file.
 * @param is_file False if document is file_id, true, if document is a file path.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned. Response is placed in
 * handler->resp_data that contains the sent message. It MUST be freed after use.
 */
telebot_error_e telebot_core_send_document(telebot_core_h *handler, char *chat_id,
        char *document, bool is_file, int reply_to_message_id,
        char *reply_markup);

/**
 * @brief This function is used to to send .webp stickers.
 * @param handler The telebot handler created with telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param sticker Sticker file to send. It is either a file_id as String to
 * resend a sticker that is already on the Telegram servers, or a path to file.
 * @param is_file False if sticker is file_id, true, if sticker is a file path.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned. Response is placed in
 * handler->resp_data that contains the sent message. It MUST be freed after use.
 */

telebot_error_e telebot_core_send_sticker(telebot_core_h *handler, char *chat_id,
        char *sticker, bool is_file, int reply_to_message_id, char *reply_markup);

/**
 * @brief This function is used to send video files, Telegram clients support
 * mp4 videos (other formats may be sent as Document).
 * @param handler The telebot handler created with telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param video Video file to send. It is either a file_id as String to resend
 * a video that is already on the Telegram servers, or a path to video file.
 * @param is_file False if video is file_id, true, if video is a file path.
 * @param duration Duration of sent video in seconds.
 * @param caption Video caption (may also be used when resending videos).
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned. Response is placed in
 * handler->resp_data that contains the sent message. It MUST be freed after use.
 */
telebot_error_e telebot_core_send_video(telebot_core_h *handler, char *chat_id,
        char *video, bool is_file, int duration, char *caption,
        int reply_to_message_id, char *reply_markup);
/**
 * @brief This function is used to send audio files, if you want Telegram
 * clients to display the file as a playable voice message. For this to work,
 * your audio must be in an .ogg file encoded with OPUS (other formats may be
 * sent as Audio or Document). Bots can currently send voice messages of up to
 * 50 MB in size.
 * @param handler The telebot handler created with telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param voice Audio file to send. It is either a file_id as String to resend
 * a audio that is already on the Telegram servers, or a path to audio file.
 * @param is_file False if voice is file_id, true, if voice is a file path.
 * @param duration Duration of sent voice/audio in seconds.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned. Response is placed in
 * handler->resp_data that contains the sent message. It MUST be freed after use.
 */

telebot_error_e telebot_core_send_voice(telebot_core_h *handler, char *chat_id,
        char *voice, bool is_file, int duration, int reply_to_message_id,
        char *reply_markup);

/**
 * @brief This function is used to send point on the map.
 * @param handler The telebot handler created with telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param latitude Latitude of location.
 * @param longitude Longitude of location.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned. Response is placed in
 * handler->resp_data that contains the sent message. It MUST be freed after use.
 */
telebot_error_e telebot_core_send_location(telebot_core_h *handler, char *chat_id,
        float latitude, float longitude, int reply_to_message_id,
        char *reply_markup);

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
 * @param handler The telebot handler created with telebot_core_create().
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param action Type of action to broadcast. Choose one, depending on what the
 * user is about to receive: typing for text messages, upload_photo for photos,
 * record_video or upload_video for videos, record_audio or upload_audio for
 * audio files, upload_document for general files, find_location for location
 * data.
 */
telebot_error_e telebot_core_send_chat_action(telebot_core_h *handler,
        char *chat_id, char *action);

/**
 * @brief This function is used to specify a url and receive incoming updates
 * via an outgoing webhook. Whenever there is an update for the bot, we will
 * send an HTTPS POST request to the specified url, containing a JSON-serialized
 * Update. In case of an unsuccessful request, we will give up after a reasonable
 * amount of attempts.
 * @param handler The telebot handler created with telebot_core_create().
 * @param url HTTPS url to send updates to. Use an empty string to remove webhook
 * integration
 * @param certificate A path to to a public key certificate to upload server.
 */
telebot_error_e telebot_core_set_web_hook(telebot_core_h *handler, char *url,
        char *certificate);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_CORE_API_H__ */
