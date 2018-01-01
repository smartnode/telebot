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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        telebot-methods.h
 * @ingroup     TELEBOT_API
 * @brief       This file contains methods used to create telegram bot
 * @author      Elmurod Talipov
 * @date        2015-12-13
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
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_create(telebot_handler_t *handle, char *token);

/**
 * @brief Final function to use telebo APIs
 *
 * This function must be the last function to call for a telebot use.
 * It is the opposite of the #telebot_create() function.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_destroy(telebot_handler_t handle);

/**
 * @brief This function is used start to receiving Telegram bot updates.
 * Internally, it starts thread for polling updates.
 *
 * This call MUST have corresponding call to telebot_stop() when receiving
 * update is needed to terminate.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] cb Callback function to receive latest telegram update.
 * @param[in] interval Update polling interval in seconds
 * @param[in] detach_thread Set if thread should be detached.
 * @param[out] thread_id Thread identifier obtained at thread creation.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_start(telebot_handler_t handle, telebot_update_cb_f cb,
        int interval, bool detach_thread, pthread_t *thread_id);

/**
 * @brief This function stops receiving updates by stopping internal thread.
 * It is the opposite of the telebot_start() function.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_stop(telebot_handler_t handle);

/**
 * @brief This function is used to get latest updates. It is alternative for
 * telebot_start() function, if you want to poll updates.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] allowed_updates Types of updates you want your bot to receive.
 * Referes #telebot_update_type_e.
 * @param[in] allowed_updates_count Number of update types.
 * @param[out] updates Update objects. It MUST be freed after use.
 * @param[out] count Number of updates received.
 * @return On success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_get_updates(telebot_handler_t handle,
        telebot_update_type_e allowed_updates[], int allowed_updates_count,
        telebot_update_t **updates, int *count);

/**
 * @brief This function is used to get information about telegram bot itself.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[out] me Pointer to telegram user object.
 * after use.
 * @return On success, TELEBOT_ERROR_NONE is returned, and user object is
 * stored in input parameter.
 */
telebot_error_e telebot_get_me(telebot_handler_t handle, telebot_user_t *me);

/**
 * @brief This function is used to send text messages.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] text Text of the message to be sent.
 * @param[in] parse_mode Send Markdown, if you want Telegram apps to show bold,
 * italic and inline URLs in your bot's message.
 * @param[in] disable_web_page_preview Disables link previews for links in this message.
 * @param[in] disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom
 * reply keyboard, instructions to hide keyboard or to force a reply from the user.
* @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_send_message(telebot_handler_t handle, int chat_id,
        char *text, char *parse_mode, bool disable_web_page_preview,
        bool disable_notification, int reply_to_message_id, const char *reply_markup);

/**
 * @brief This function is used to forward messages of any kind.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] from_chat_id Unique identifier for the chat where the original
 * message was sent (or channel username in the format \@channelusername).
 * @param[in] disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param[in] message_id Unique message identifier.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_forward_message(telebot_handler_t handle, int chat_id,
        char *from_chat_id, bool disable_notification, int message_id);

/**
 * @brief This functionis used to send photos.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).int chat_id
 * @param[in] photo Photo to send. It is either file_id as String to resend a photo
 * that is already on the Telegram servers, or a path to photo file.
 * @param[in] is_file False if photo is file_id, true, if photo is a file path.
 * @param[in] caption Photo caption. (may also be used when resending photos).
 * @param[in] disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom
 * reply keyboard, instructions to hide keyboard or to force a reply from
 * the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_send_photo(telebot_handler_t handle, int chat_id,
        char *photo, bool is_file, char *caption, bool disable_notification,
        int reply_to_message_id, char *reply_markup);

/**
 * @brief This function is used to to send audio files. if you want Telegram
 * clients to display them in the music player. Your audio must be in the .mp3
 * format. Bots can currently send audio files of up to 50 MB in size. For backward
 * compatibility, when the fields title and performer are both empty and the
 * mime-type of the file to be sent is not audio/mpeg, the file will be sent as
 * a playable voice message. For this to work, the audio must be in an .ogg
 * file encoded with OPUS.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] audio Audio file to send. It is either a file_id as String to resend an
 * audio that is already on the Telegram servers, or a path to audio file.
 * @param[in] is_file False if audio is file_id, true, if audio is a file path.
 * @param[in] duration Duration of sent audio in seconds.
 * @param[in] performer The performer of the audio.
 * @param[in] title The track name of the audio.
 * @param[in] disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_send_audio(telebot_handler_t handle, int chat_id,
        char *audio, bool is_file, int duration, char *performer, char *title,
        bool disable_notification, int reply_to_message_id, char *reply_markup);

/**
 * @brief This function is used to send general files.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] document Document file to send. It is either a file_id as String to
 * resend a file that is already on the Telegram servers, or a path to file.
 * @param[in] is_file False if document is file_id, true, if document is a file path.
 * @param[in] disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_send_document(telebot_handler_t handle, int chat_id,
        char *document, bool is_file, bool disable_notification,
        int reply_to_message_id, char *reply_markup);

/**
 * @brief This function is used to send video files, Telegram clients support
 * mp4 videos (other formats may be sent as Document).
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] video Video file to send. It is either a file_id as String to resend
 * a video that is already on the Telegram servers, or a path to video file.
 * @param[in] is_file False if video is file_id, true, if video is a file path.
 * @param[in] width Video width.
 * @param[in] height Video height.
 * @param[in] duration Duration of sent video in seconds.
 * @param[in] caption Video caption (may also be used when resending videos).
 * @param[in] disable_notification Sends the message silently. Users will
 * receive a notification with no sound.
 * @param[in] reply_to_message_id Isend_videof the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_send_video(telebot_handler_t handle, int chat_id,
        char *video, bool is_file, int duration, int width, int height,
        char *caption, bool disable_notification, int reply_to_message_id,
        char *reply_markup);

/**
 * @brief This function is used to send audio files, if you want Telegram
 * clients to display the file as a playable voice message. For this to work,
 * your audio must be in an .ogg file encoded with OPUS (other formats may be
 * sent as Audio or Document). Bots can currently send voice messages of up to
 * 50 MB in size.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] voice Audio file to send. It is either a file_id as String to resend
 * a audio that is already on the Telegram servers, or a path to audio file.
 * @param[in] is_file False if voice is file_id, true, if voice is a file path.
 * @param[in] caption Voice message caption, 0-200 characters.
 * @param[in] duration Duration of sent audio in seconds.
 * @param[in] disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_send_voice(telebot_handler_t handle, int chat_id,
        char *voice, bool is_file, char *caption, int duration,
        bool disable_notification, int reply_to_message_id, char *reply_markup);

/**
 * @breif As of v.4.0, Telegram clients support rounded square mp4 videos of up
 * to 1 minute long. This function is used to send video messages.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] video_note Video note to send. Pass a file_id as String to send a
 * video note that exists on the Telegram servers or upload a new video.
 * @param[in] is_file False if voice is file_id, true, if voice is a file path.
 * @param[in] caption Voice message caption, 0-200 characters.
 * @param[in] duration Duration of sent video in seconds.
 * @param[in] length Video width and height.
 * @param[in] disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_send_video_note(telebot_handler_t handle, int chat_id,
        char *video_note, bool is_file, int duration, int length,
        bool disable_notification, int reply_to_message_id, char *reply_markup);

/**
 * @breif This function is used to send a group of photos or videos as an album.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] media A JSON-serialized array describing photos and videos to
 * be sent, must include 2–10 items
 * @param[in] is_file False if voice is file_id, true, if voice is a file path.
 * @param[in] disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
/* TODO
 * telebot_error_e telebot_send_media_group(telebot_handler_t handle, int chat_id,
 *        telebot_input_media_t media[], bool is_file, bool disable_notification,
 *        int reply_to_message_id);
 */

/**
 * @brief This function is used to send point on the map.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] latitude Latitude of location.
 * @param[in] longitude Longitude of location.
 * @param[in] disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_send_location(telebot_handler_t handle, int chat_id,
        float latitude, float longitude, bool disable_notification,
        int reply_to_message_id, char *reply_markup);


/**
 * @brief Use this method to edit live location messages sent by the bot or via
 * the bot (for inline bots). A location can be edited until its live_period
 * expires or editing is explicitly disabled by a call to
 * #telebot_stop_message_live_location().
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] message_id Required if inline_message_id is not specified.
 * Identifier of the sent message.
 * @param[in] inline_message_id Required if chat_id and message_id are not
 * specified. Identifier of the inline message.
 * @param[in] latitude Latitude of location.
 * @param[in] longitude Longitude of location.
 * @param[in] disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param[in] reply_markup A JSON-serialized object for a new inline keyboard.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_edit_message_live_location(telebot_handler_t handle,
        int chat_id, int message_id, char *inline_message_id, float latitude,
        float longitude, bool disable_notification, char *reply_markup);

/**
 * @brief Use this method to stop updating a live location message sent by the
 * bot or via the bot (for inline bots) before live_period expires.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] message_id Required if inline_message_id is not specified.
 * Identifier of the sent message.
 * @param[in] inline_message_id Required if chat_id and message_id are not
 * specified. Identifier of the inline message.
 * @param[in] reply_markup A JSON-serialized object for a new inline keyboard.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_stop_message_live_location(telebot_handler_t handle,
        int chat_id, int message_id, char *inline_message_id, char *reply_markup);

/**
 * @brief Use this method to send information about a venue.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] latitude Latitude of venue.
 * @param[in] longitude Longitude of venue.
 * @param[in] title Name of the venue
 * @param[in] address Address of the venue.
 * @param[in] foursquare_id Foursquare identifier of the venue.
 * @param[in] disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. A JSON-serialized
 * object for an inline keyboard, custom reply keyboard, instructions to remove
 * reply keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_send_venue(telebot_handler_t handle, int chat_id,
        float latitude, float longitude, char *title, char *foursquare_id,
        bool disable_notification, int reply_to_message_id, char *reply_markup);

/**
 * @brief Use this method to send phone contacts.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] phone_number Contact's phone numbers.
 * @param[in] first_name Contact's first name.
 * @param[in] last_name Contact's last name.
 * @param[in] disable_notification Sends the message silently. Users will receive
 * a notification with no sound.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. A JSON-serialized
 * object for an inline keyboard, custom reply keyboard, instructions to remove
 * reply keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_send_contact(telebot_handler_t handle, int chat_id,
        char *phone_number, char *first_name, char *last_name,
        bool disable_notification, int reply_to_message_id, char *reply_markup);

/**
 * @brief This function is used to tell the user that something is happening on
 * the bot's side. The status is set for 5 seconds or less (when a message
 * arrives from your bot, Telegram clients clear its typing status).
 * Example: The ImageBot needs some time to process a request and upload the
 * image. Instead of sending a text message along the lines of "Retrieving image,
 * please wait...", the bot may use sendChatAction with action = upload_photo.
 * The user will see a "sending photo" status for the bot.
 * It is only recommended to use when a response from the bot will take a
 * noticeable amount of time to arrive.
 *
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param[in] action Type of action to broadcast. Choose one, depending on what
 * the user is about to receive: typing for text messages, upload_photo for
 * photos, record_video or upload_video for videos, record_audio or
 * upload_audio for audio files, upload_document for general files,
 * find_location for location data, record_video_note or upload_video_note for
 * video notes.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_send_chat_action(telebot_handler_t handle, int chat_id,
        char *action);

/**
 * @brief This function is used to get user profile pictures object
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] offset Sequential number of the first photo to be returned.
 * By default, up to 10 photos are returned.
 * @param[in] limit Limits the number of photos to be retrieved.
 * Values between 1—100 are accepted. Defaults to 100.
 * @param[out] photos Photo objects. It MUST be freed after use.
 * @param[out] count The number of photo objects.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_get_user_profile_photos(telebot_handler_t handle,
        int user_id, int offset, int limit, telebot_user_profile_photos_t *photos);

/**
 * @brief This function is used to download file.
 * @param[in] file_id File identifier to get info about.
 * @param[in] path A path where the file is downloaded
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_download_file(telebot_handler_t handle, char *file_id,
        char *path);

/**
 * @brief This function is used to delete messages.
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] message_id Identifier of the message to delete
 * @return on Success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_delete_message(telebot_handler_t handle, int chat_id,
        int message_id);


/**
 * @brief Use this method to send answers to callback queries sent from
 * inline keyboards. The answer will be displayed to the user as a notification
 * at the top of the chat screen or as an alert.
 * @param[in] callback_query_id Unique identifier for the query to be answered.
 * @param[in] text Optional (i.e. can be NULL). Text of the notification. If not
 * specified, nothing will be shown to the user, 0-200 characters.
 * @param[in] show_alert Optional (i.e. can be NULL). If true, an alert will be
 * shown by the client instead of a notification at the top of the chat screen.
 * @param[in] url Optional (i.e. can be NULL). URL that will be opened by the user's
 *  client. If you have created a Game and accepted the conditions via @Botfather,
 * specify the URL that opens your game – note that this will only work if the
 * query comes from a callback_game button.
 * @param[in] cache_time Optional (i.e. can be NULL). The maximum amount of time in
 * seconds that the result of the callback query may be cached client-side.
 * Telegram apps will support caching starting in version 3.14.
 * @return on Success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_answer_callback_query(telebot_handler_t handle,
        const char *callback_query_id, char *text, bool show_alert, char *url,
        int cache_time);

/**
 * @brief This function is used to to send .webp stickers.
 * @param[in] handle The telebot handler created with #telebot_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param[in] sticker Sticker file to send. It is either a file_id as String to
 * resend a sticker that is already on the Telegram servers, or a path to file.
 * @param[in] is_file False if sticker is file_id, true, if sticker is a file path.
 * @param[in] reply_to_message_id If the message is a reply, ID of the original message.
 * @param[in] reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_send_sticker(telebot_handler_t handle, int chat_id,
        char *sticker, bool is_file, bool disable_notification,
        int reply_to_message_id, char *reply_markup);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_METHODS_H__ */
