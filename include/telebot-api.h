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

#include <telebot-core-api.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This object represents a Telegram user or bot.
 */
typedef struct _telebot_user_t_ {
    /** Unique identifier for this user or bot */
    int id;

    /** User's or bot's first name. */
    char *first_name;

    /** Optional. User's or bot's last name */
    char *last_name;

    /** Optional. User's or bot's username. */
    char *username;
} telebot_user_t;

/**
 * @brief This object represents a chat.
 */
typedef struct _telebot_chat_t_ {
    /** Unique identifier for this chat, not exceeding 1e13 by absolute value */
    int id;

    /** Type of chat, can be either "private", or "group", or "channel" */
    char *type;

    /** Optional. Title, for channels and group chats */
    char *title;

    /** Optional. Username, for private chats and channels if available */
    char *username;

    /** Optional. First name of the other party in a private chat */
    char *first_name;

    /** Optional. Last name of the other party in a private chat */
    char *last_name;
} telebot_chat_t;

/**
 * @brief This object represents one size of a photo or a file / sticker 
 * thumbnail.
 */
typedef struct _telebot_photosize_t_ {
    /** Unique identifier for this file */
    char *file_id;

    /** Photo width */
    int width;

    /** Photo height */
    int height;

    /** Optional. File size */
    int file_size;
} telebot_photosize_t;

/**
 * @brief This object represents an audio file to be treated as music by the 
 * Telegram clients.
 */
typedef struct _telebot_audio_t_ {
    /** Unique identifier for this file */
    char *file_id;

    /** Duration of the audio in seconds as defined by sender */
    int duration;

    /** Optional. Performer of the audio as defined by sender or by audio tags */
    char *performer;

    /** Optional. Title of the audio as defined by sender or by audio tags */
    char *title;

    /** Optional. MIME type of the file as defined by sender */
    char *mime_type;

    /** Optional. File size */
    int file_size; 
} telebot_audio_t;

/**
 * @brief This object represents a general file (as opposed to photos, voice 
 * messages and audio files).
 */
typedef struct _telebot_document_t_ {
    /** Unique file identifier. */
    char *file_id;

    /** Optional. Document thumbnail as defined by sender. */
    telebot_photosize_t thumb;

    /** Optional. Original filename as defined by sender. */
    char *file_name;

    /** Optional. MIME type of the file as defined by sender. */
    char *mime_type;

    /** Optional. File size. */
    int file_size;
} telebot_document_t;

/**
 * @brief This object represents a sticker.
 */
typedef struct _telebot_sticker_t_ {
    /** Unique identifier for this file */
    char *file_id;

    /**< Sticker width */
    int width;

    /** Sticker height */
    int height;

    /** Optional. Sticker thumbnail in .webp or .jpg format */
    telebot_photosize_t thumb;

    /** Optional. File size */
    int file_size;
} telebot_sticker_t;

/**
 * @brief This object represents a video file.
 */
typedef struct _telebot_video_t_ {
    /** Unique identifier for this file */
    char *file_id;

    /** Video width as defined by sender */
    int width; 

    /** Video height as defined by sender */
    int height;

    /** Duration of the video in seconds as defined by sender */
    int duration;

    /** Optional. Video thumbnail */
    telebot_photosize_t thumb;

    /** Optional. Mime type of a file as defined by sender */
    char *mime_type;

    /** Optional. File size */
    int file_size;
} telebot_video_t;

/**
 * @brief This object represents a voice note.
 */
typedef struct _telebot_voice_t_ {
    /** Unique identifier for this file */
    char *file_id;

    /** Duration of the audio in seconds as defined by sender */
    int duration;

    /** Optional. MIME type of the file as defined by sender */
    char *mime_type;

    /** Optional. File size */
    int file_size;
} telebot_voice_t;

/**
 * @brief This object represents a phone contact.
 */
typedef struct _telebot_contact_t_ {
    /** Contact's phone number */
    char *phone_number;

    /** Contact's first name */
    char *first_name;

    /** Optional. Contact's last name */
    char *last_name;

    /** Optional. Contact's user identifier in Telegram */
    int user_id;
} telebot_contact_t;

/**
 * @brief This object represents a point on the map.
 */

typedef struct _telebot_location_t_ {
   /** Longitude as defined by sender */
    float longitude;

    /** Latitude as defined by sender */
    float latitude;
} telebot_location_t;

/**
 * @brief This object represent a user's profile pictures.
 */
typedef struct _telebot_userphotos_t {
    /** Total number of profile pictures the target user has */
    int total_count;

    /** Requested profile pictures (in up to 4 sizes each) */
    telebot_photosize_t **photos;
} telebot_userphotos_t;

/**
 * @brief This object represents a file ready to be downloaded.
 *
 * The file can be downloaded via the link 
 * https://api.telegram.org/file/bot<token>/<file_path>.
 * It is guaranteed that the link will be valid for at least 1 hour. 
 * When the link expires, a new one can be requested by calling getFile.
 * Maximum file size to download is 20 MB.
 */
typedef struct _telebot_file_t_ {
    /** Unique identifier for this file */
    char *file_id;

    /** Optional. File size, if known */
    int file_size;

    /**  Optional. File path. */
    char *file_path;
} telebot_file_t;


/**
 * @brief This object represents a message.
 */
typedef struct _telebot_message_t_ {
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
    struct _telebot_message_t_  *reply_to_message;

    /** Optional. For text messages, the actual UTF-8 text of the message */
    char *text;

    /** Optional. Message is an audio file, information about the file */
    telebot_audio_t audio;

    /** Optional. Message is a general file, information about the file */
    telebot_document_t document;

    /** Optional. Message is a photo, available sizes of the photo */
    telebot_photosize_t photo[10];

    /** Optional. Message is a sticker, information about the sticker */
    telebot_sticker_t sticker;

    /** Optional. Message is a video, information about the video */
    telebot_video_t video;

    /** Optional. Message is a voice message, information about the file */
    telebot_voice_t voice;

    /** Optional. Caption for the photo or video */
    char *caption;

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
    char *new_chat_title; 

    /** Optional. A chat photo was change to this value */
    telebot_photosize_t new_chat_photo[4];

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
     * Optional. The supergroup has been migrated from a group with the specified
     * identifier, not exceeding 1e13 by absolute value
     */
    int migrate_from_chat_id;

} telebot_message_t;

/**
 * @brief This function type defines callback for receiving updates.
 */
typedef void (*telebot_update_cb_f)(telebot_message_t message);

/**
 * @brief Initial function to use telebot APIs.
 *
 * This function must be used first to call, and it creates handler. This call
 * MUST have  corresponding call to telebot_destroy() when operation is complete.
 * @param token Telegram Bot token to use.
 * @return On success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_create(char *token);

/**
 * @brief Final function to use telebot core APIs
 *
 * This function must be the last function to call for a telebot use.
 * It is the opposite of the telebot_create() function.
 * @return On success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_destory();

/**
 * @brief This function is used start to receiving Telegram bot updates.
 * Internally, it starts thread for polling updates.
 *
 * This call MUST have corresponding call to telebot_stop() when receiving update is
 * needed to terminate.
 * @param update_cb Callback function to receive latest telegram update.
 * @return On success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_start(telebot_update_cb_f update_cb);


/**
 * @brief This function stops receiving Telegram update by stopping internal thread.
 *
 * It is the opposite of the telebot_start() function.
 * @return On success, TELEBOT_ERROR_NONE is returned.
 */
telebot_error_e telebot_stop();

/**
 * @brief This function is used to get information about telegram bot itself.
 *
 * @param me Pointer for putting telegram user object.
 *
 * @return On success, TELEBOT_ERROR_NONE is returned, and user object is
 * stored in input parameter.
 */
telebot_error_e telebot_get_me(telebot_user_t *me);

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_API_H__ */


