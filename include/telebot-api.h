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

#ifndef __TELEBOT_API_H__
#define __TELEBOT_API_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        telebot-api.h
 * @ingroup     TELEBOT_API
 * @brief       This file contains simple APIs to create telegram bot
 * @author      Elmurod Talipov
 * @date        2015-12-13
 */

/**
 * @defgroup TELEBOT_API		Simple Telegram Bot API
 * @brief Simplified APIs to create telegram bot.
 *
 *
 * @addtogroup TELEBOT_API
 * @{
 */

/**
 * @brief This object represents a Telegram user or bot.
 */
typedef struct telebot_user {
    /** Unique identifier for this user or bot */
    int id;

    /** User's or bot's first name. */
    char first_name[TELEBOT_FIRST_NAME_SIZE];

    /** Optional. User's or bot's last name */
    char last_name[TELEBOT_LAST_NAME_SIZE];

    /** Optional. User's or bot's username. */
    char username[TELEBOT_USER_NAME_SIZE];
} telebot_user_t;

/**
 * @brief This object represents a chat.
 */
typedef struct telebot_chat {
    /** Unique identifier for this chat, not exceeding 1e13 by absolute value */
    int id;

    /** Type of chat, can be either "private", or "group", or "channel" */
    char type[TELEBOT_CHAT_TYPE_SIZE];

    /** Optional. Title, for channels and group chats */
    char title[TELEBOT_CHAT_TITLE_SIZE];

    /** Optional. Username, for private chats and channels if available */
    char username[TELEBOT_USER_NAME_SIZE];

    /** Optional. First name of the other party in a private chat */
    char first_name[TELEBOT_FIRST_NAME_SIZE];

    /** Optional. Last name of the other party in a private chat */
    char last_name[TELEBOT_LAST_NAME_SIZE];
} telebot_chat_t;

/**
 * @brief This object represents one size of a photo or a file / sticker
 * thumbnail.
 */
typedef struct telebot_photo {
    /** Unique identifier for this file */
    char file_id[TELEBOT_FILE_ID_SIZE];

    /** Photo width */
    int width;

    /** Photo height */
    int height;

    /** Optional. File size */
    int file_size;
} telebot_photo_t;

/**
 * @brief This object represents an audio file to be treated as music by the
 * Telegram clients.
 */
typedef struct telebot_audio {
    /** Unique identifier for this file */
    char file_id[TELEBOT_FILE_ID_SIZE];

    /** Duration of the audio in seconds as defined by sender */
    int duration;

    /** Optional. Performer of the audio as defined by sender or by audio tags */
    char performer[TELEBOT_AUDIO_PERFORMER_SIZE];

    /** Optional. Title of the audio as defined by sender or by audio tags */
    char title[TELEBOT_AUDIO_TITLE_SIZE];

    /** Optional. MIME type of the file as defined by sender */
    char mime_type[TELEBOT_AUDIO_MIME_TYPE_SIZE];

    /** Optional. File size */
    int file_size;
} telebot_audio_t;

/**
 * @brief This object represents a general file (as opposed to photos, voice
 * messages and audio files).
 */
typedef struct telebot_document {
    /** Unique file identifier. */
    char file_id[TELEBOT_FILE_ID_SIZE];

    /** Optional. Document thumbnail as defined by sender. */
    telebot_photo_t thumb;

    /** Optional. Original filename as defined by sender. */
    char file_name[TELEBOT_FILE_NAME_SIZE];

    /** Optional. MIME type of the file as defined by sender. */
    char mime_type[TELEBOT_DOCUMENT_MIME_TYPE_SIZE];

    /** Optional. File size. */
    int file_size;
} telebot_document_t;

/**
 * @brief This object represents a sticker.
 */
typedef struct telebot_sticker {
    /** Unique identifier for this file */
    char file_id[TELEBOT_FILE_ID_SIZE];

    /** Sticker width */
    int width;

    /** Sticker height */
    int height;

    /** Optional. Sticker thumbnail in .webp or .jpg format */
    telebot_photo_t thumb;

    /** Optional. File size */
    int file_size;
} telebot_sticker_t;

/**
 * @brief This object represents a video file.
 */
typedef struct telebot_video {
    /** Unique identifier for this file */
    char file_id[TELEBOT_FILE_ID_SIZE];

    /** Video width as defined by sender */
    int width;

    /** Video height as defined by sender */
    int height;

    /** Duration of the video in seconds as defined by sender */
    int duration;

    /** Optional. Video thumbnail */
    telebot_photo_t thumb;

    /** Optional. Mime type of a file as defined by sender */
    char mime_type[TELEBOT_VIDEO_MIME_TYPE_SIZE];

    /** Optional. File size */
    int file_size;
} telebot_video_t;

/**
 * @brief This object represents a voice note.
 */
typedef struct telebot_voice {
    /** Unique identifier for this file */
    char file_id[TELEBOT_FILE_ID_SIZE];

    /** Duration of the audio in seconds as defined by sender */
    int duration;

    /** Optional. MIME type of the file as defined by sender */
    char mime_type[TELEBOT_VOICE_MIME_TYPE_SIZE];

    /** Optional. File size */
    int file_size;
} telebot_voice_t;

/**
 * @brief This object represents a phone contact.
 */
typedef struct telebot_contact {
    /** Contact's phone number */
    char phone_number[TELEBOT_PHONE_NUMBER_SIZE];

    /** Contact's first name */
    char first_name[TELEBOT_FIRST_NAME_SIZE];

    /** Optional. Contact's last name */
    char last_name[TELEBOT_LAST_NAME_SIZE];

    /** Optional. Contact's user identifier in Telegram */
    int user_id;
} telebot_contact_t;

/**
 * @brief This object represents a point on the map.
 */

typedef struct telebot_location {
   /** Longitude as defined by sender */
    float longitude;

    /** Latitude as defined by sender */
    float latitude;
} telebot_location_t;

/**
 * @brief This object represents a file ready to be downloaded.
 *
 * The file can be downloaded via the link
 * https://api.telegram.org/file/bot[token]/[file_path].
 * It is guaranteed that the link will be valid for at least 1 hour.
 * When the link expires, a new one can be requested by calling getFile.
 * Maximum file size to download is 20 MB.
 */
typedef struct telebot_file {
    /** Unique identifier for this file */
    char file_id[TELEBOT_FILE_ID_SIZE];

    /** Optional. File size, if known */
    int file_size;

    /**  Optional. File path. */
    char file_path[TELEBOT_FILE_PATH_SIZE];
} telebot_file_t;


/**
 * @brief This object represents a message.
 */
typedef struct telebot_message {
    /** Unique message identifier */
    int message_id;

    /** Optional. Sender, can be empty for messages sent to channels */
    telebot_user_t from;

    /** Date the message was sent in Unix time */
    long date;

    /** Conversation the message belongs to */
    telebot_chat_t chat;

    /** Optional. For forwarded messages, sender of the original message */
    telebot_user_t forward_from;

    /**
     * Optional. For forwarded messages, date the original message was sent
     * in Unix time
     */
    long forward_date;

    /**
     * For replies, the original message. Note that the Message object in this
     * field will not contain further reply_to_message fields even if it itself
     * is a reply.
     */
    struct telebot_message  *reply_to_message;

    /** Optional. For text messages, the actual UTF-8 text of the message */
    char text[TELEBOT_MESSAGE_TEXT_SIZE];

    /** Optional. Message is an audio file, information about the file */
    telebot_audio_t audio;

    /** Optional. Message is a general file, information about the file */
    telebot_document_t document;

    /** Optional. Message is a photo, available sizes of the photo */
    telebot_photo_t photo[TELEBOT_MESSAGE_PHOTO_SIZE];

    /** Optional. Message is a sticker, information about the sticker */
    telebot_sticker_t sticker;

    /** Optional. Message is a video, information about the video */
    telebot_video_t video;

    /** Optional. Message is a voice message, information about the file */
    telebot_voice_t voice;

    /** Optional. Caption for the photo or video */
    char caption[TELEBOT_MESSAGE_CAPTION_SIZE];

    /** Optional. Message is a shared contact, information about the contact */
    telebot_contact_t contact;

    /** Optional. Message is a shared location, information about the location */
    telebot_location_t location;

    /**
     * Optional. A new member was added to the group, information about them
     * (this member may be bot itself)
     */
    telebot_user_t new_chat_participant;

    /**
     * Optional. A member was removed from the group, information about them
     * (this member may be bot itself)
     */
    telebot_user_t left_chat_participant;

    /** Optional. A chat title was changed to this value */
    char new_chat_title[TELEBOT_CHAT_TITLE_SIZE];

    /** Optional. A chat photo was change to this value */
    telebot_photo_t new_chat_photo[TELEBOT_MESSAGE_NEW_CHAT_PHOTO_SIZE];

    /** Optional. Informs that the chat photo was deleted */
    bool delete_chat_photo;

    /** Optional. Informs that the group has been created */
    bool group_chat_created;

    /** Optional. Service message: the supergroup has been created */
    bool supergroup_chat_created;

    /** Optional. Service message: the channel has been created */
    bool channel_chat_created;

    /**
     * Optional. The group has been migrated to a supergroup with the specified
     * identifier, not exceeding 1e13 by absolute value
     */
    int migrate_to_chat_id;

    /**
     * Optional. The supergroup has been migrated from a group with the
     * specified identifier, not exceeding 1e13 by absolute value
     */
    int migrate_from_chat_id;

} telebot_message_t;

/**
 * @brief This object represents an incoming update.
 */
typedef struct telebot_update {
    /**
     * The update's unique identifier. Update identifiers start from a certain
     * positive number and increase sequentially.
     */
    int update_id;

    /** New incoming message of any kind — text, photo, sticker, etc. */
    telebot_message_t message;
} telebot_update_t;

/**
 * @brief This function type defines callback for receiving updates.
 */
typedef void (*telebot_update_cb_f)(const telebot_message_t *message);

/**
 * @brief Initial function to use telebot APIs.
 *
 * This function must be used first to call, and it creates handler. This call
 * MUST have corresponding call to telebot_destroy() when operation is complete.
 * @param token Telegram Bot token to use.
 * @return On success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_create(char *token);

/**
 * @brief Final function to use telebo APIs
 *
 * This function must be the last function to call for a telebot use.
 * It is the opposite of the telebot_create() function.
 * @return On success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_destroy();

/**
 * @brief This function is used start to receiving Telegram bot updates.
 * Internally, it starts thread for polling updates.
 *
 * This call MUST have corresponding call to telebot_stop() when receiving
 * update is needed to terminate.
 * @param update_cb Callback function to receive latest telegram update.
 * @return On success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_start(telebot_update_cb_f update_cb, bool should_deatach_thread, pthread_t* thread_id);


/**
 * @brief This function stops receiving updates by stopping internal thread.
 *
 * It is the opposite of the telebot_start() function.
 * @return On success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_stop();

/**
 * @brief This function is used to get information about telegram bot itself.
 *
 * @param me Pointer to the telegram user object address. This pointer MUST be
 * freed after use.
 * @return On success, TELEBOT_ERROR_NONE is returned, and user object is
 * stored in input parameter.
 */
telebot_error_e telebot_get_me(telebot_user_t **me);

/**
 * @brief This function is used to get latest updates. It is alternative for
 * telebot_start() function, if you want to poll updates.
 * @param updates Pointer to the updates object address. It MUST be freed.
 * @param count Pointer to put number of updates received.
 * @return on Success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_get_updates(telebot_update_t **updates, int *count);

/**
 * @brief This function is used to get user profile pictures object
 * @param user_id Unique identifier of the target user.
 * @param offset Sequential number of the first photo to be returned.
 * By default, up to 10 photos are returned.
 * @param photos Pointer to the photos object address. This pointer MUST
 * be freed after use.
 * @param count Pointer to the number of photos object.
 * @return on Success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_get_user_profile_photos(int user_id, int offset,
        telebot_photo_t **photos, int *count);

/**
 * @brief This function is used to download file.
 * @param file_id File identifier to get info about.
 * @param path A path where the file is downloaded
 * @return on Success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_download_file(char *file_id, char *path);

/**
 * @brief This function is used to send text messages.
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param text Text of the message to be sent.
 * @param parse_mode Send Markdown, if you want Telegram apps to show bold,
 * italic and inline URLs in your bot's message.
 * @param disable_web_page_preview Disables link previews for links in this message.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom
 * reply keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_send_message(char *chat_id, char *text, char *parse_mode,
        bool disable_web_page_preview, int reply_to_message_id, const char *reply_markup);

/**
 * @brief This function is used to forward messages of any kind.
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param from_chat_id Unique identifier for the chat where the original
 * message was sent (or channel username in the format \@channelusername).
 * @param message_id Unique message identifier.
 * @return on Success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_forward_message(char *chat_id, char *from_chat_id,
        int message_id);

/**
 * @brief This functionis used to send photos.
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
 * @return on Success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_send_photo(char *chat_id, char *photo, bool is_file,
        char *caption, int reply_to_message_id, char *reply_markup);
/**
 * @brief This function is used to to send audio files. if you want Telegram
 * clients to display them in the music player. Your audio must be in the .mp3
 * format. Bots can currently send audio files of up to 50 MB in size. For backward
 * compatibility, when the fields title and performer are both empty and the
 * mime-type of the file to be sent is not audio/mpeg, the file will be sent as
 * a playable voice message. For this to work, the audio must be in an .ogg
 * file encoded with OPUS.
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
 * @return on Success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_send_audio(char *chat_id, char *audio, bool is_file,
        int duration, char *performer, char *title, int reply_to_message_id,
        char *reply_markup);

/**
 * @brief This function is used to send general files.
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param document Document file to send. It is either a file_id as String to
 * resend a file that is already on the Telegram servers, or a path to file.
 * @param is_file False if document is file_id, true, if document is a file path.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_send_document(char *chat_id, char *document,
        bool is_file, int reply_to_message_id, char *reply_markup);

/**
 * @brief This function is used to to send .webp stickers.
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param sticker Sticker file to send. It is either a file_id as String to
 * resend a sticker that is already on the Telegram servers, or a path to file.
 * @param is_file False if sticker is file_id, true, if sticker is a file path.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_send_sticker(char *chat_id, char *sticker, bool is_file,
        int reply_to_message_id, char *reply_markup);

/**
 * @brief This function is used to send video files, Telegram clients support
 * mp4 videos (other formats may be sent as Document).
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param video Video file to send. It is either a file_id as String to resend
 * a video that is already on the Telegram servers, or a path to video file.
 * @param is_file False if video is file_id, true, if video is a file path.
 * @param duration Duration of sent video in seconds.
 * @param caption Video caption (may also be used when resending videos).
 * @param reply_to_message_id Isend_videof the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_send_video(char *chat_id, char *video, bool is_file,
        int duration, char *caption, int reply_to_message_id, char *reply_markup);

/**
 * @brief This function is used to send audio files, if you want Telegram
 * clients to display the file as a playable voice message. For this to work,
 * your audio must be in an .ogg file encoded with OPUS (other formats may be
 * sent as Audio or Document). Bots can currently send voice messages of up to
 * 50 MB in size.
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param voice Audio file to send. It is either a file_id as String to resend
 * a audio that is already on the Telegram servers, or a path to audio file.
 * @param is_file False if voice is file_id, true, if voice is a file path.
 * @param duration Duration of sent audio in seconds.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_send_voice(char *chat_id, char *voice, bool is_file,
        int duration, int reply_to_message_id, char *reply_markup);

/**
 * @brief This function is used to send point on the map.
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param latitude Latitude of location.
 * @param longitude Longitude of location.
 * @param reply_to_message_id If the message is a reply, ID of the original message.
 * @param reply_markup Additional interface options. An object for a custom reply
 * keyboard, instructions to hide keyboard or to force a reply from the user.
 * @return on Success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_send_location(char *chat_id, float latitude,
        float longitude, int reply_to_message_id, char *reply_markup);

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
 * @param chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername)
 * @param action Type of action to broadcast. Choose one, depending on what the
 * user is about to receive: typing for text messages, upload_photo for photos,
 * record_video or upload_video for videos, record_audio or upload_audio for
 * audio files, upload_document for general files, find_location for location
 * data.
 * @return on Success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_send_chat_action(char *chat_id, char *action);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_API_H__ */
