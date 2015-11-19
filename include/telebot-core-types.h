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

#ifndef __TELEBOT_CORE_TYPES_H__
#define __TELEBOT_CORE_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This object represents a Telegram user or bot.
 */
typedef struct _telebot_user_t_ {
    int id;	/**< Unique identifier for this user or bot */
    char *first_name; /**< User's or bot's first name. */
    char *last_name; /**< Optional. User's or bot's last name */
    char *username;	/**< Optional. User's or bot's username. */
} telebot_user_t;

/**
 * @brief This object represents a chat.
 */
typedef struct _telebot_chat_t_ {
    int id;	/**< Unique identifier for this chat, not exceeding 1e13 by absolute value */
    char *type;	/**< Type of chat, can be either “private”, or “group”, or “channel” */
    char *title; /**< Optional. Title, for channels and group chats */
    char *username;	/**< Optional. Username, for private chats and channels if available */
    char *first_name; /**< Optional. First name of the other party in a private chat */
    char *last_name; /**< Optional. Last name of the other party in a private chat */
} telebot_chat_t;

/**
 * @brief This object represents one size of a photo or a file / sticker thumbnail.
 */
typedef struct _telebot_photosize_t_ {
    char *file_id; /**< Unique identifier for this file */
    int width; /**< Photo width */
    int height;	 /**< Photo height */
    int file_size; /**< Optional. File size */
} telebot_photosize_t;

/**
 * @brief This object represents an audio file to be treated as music by the 
 * Telegram clients.
 */
typedef struct _telebot_audio_t_ {
    char *file_id; /**< Unique identifier for this file */
    int duration; /**< Duration of the audio in seconds as defined by sender */
    char *performer; /**< Optional. Performer of the audio as defined by sender or by audio tags */
    char *title; /**< Optional. Title of the audio as defined by sender or by audio tags */
    char *mime_type; /**< Optional. MIME type of the file as defined by sender */
    char *file_size; /**< Optional. File size */
} telebot_audio_t;

/**
 * @brief This object represents a general file (as opposed to photos, voice 
 * messages and audio files).
 */
typedef struct _telebot_document_t_ {
    char *file_id; /**< Unique file identifier. */
    telebot_photosize_t thumb; /**< Optional. Document thumbnail as defined by sender. */
    char *file_name; /**< Optional. Original filename as defined by sender. */
    char *mime_type; /**< Optional. MIME type of the file as defined by sender. */
    int file_size; /**< Optional. File size. */
} telebot_document_t;

/**
 * @brief This object represents a sticker.
 */
typedef struct _telebot_sticker_t_ {
    char *file_id; /**< Unique identifier for this file */
    int width; /**< Sticker width */
    int height;  /**< Sticker height */
    telebot_photosize_t thumb; /**< Optional. Sticker thumbnail in .webp or .jpg format */
    int file_size; /**< Optional. File size */
} telebot_sticker_t;

/**
 * @brief This object represents a video file.
 */
typedef struct _telebot_video_t_ {
    char *file_id; /**<  Unique identifier for this file */
    int width; /**< Video width as defined by sender */
    int height;	 /**< Video height as defined by sender */
    int duration; /**< Duration of the video in seconds as defined by sender */
    telebot_photosize_t thumb; /**< Optional. Video thumbnail */
    char *mime_type; /**< Optional. Mime type of a file as defined by sender */
    char *file_size; /**< Optional. File size */
} telebot_video_t;

/**
 * @brief This object represents a voice note.
 */
typedef struct _telebot_voice_t_ {
    char *file_id; /**< Unique identifier for this file */
    int duration; /**< Duration of the audio in seconds as defined by sender */
    char *mime_type; /**< Optional. MIME type of the file as defined by sender */
    int file_size; /**< Optional. File size */
} telebot_voice_t;

/**
 * @brief This object represents a phone contact.
 */
typedef struct _telebot_contact_t_ {
    char *phone_number; /**< Contact's phone number */
    char *first_name; /**< Contact's first name */
    char *last_name; /**< Optional. Contact's last name */
    int user_id; /**< Optional. Contact's user identifier in Telegram */
} telebot_contact_t;

/**
 * @brief This object represents a point on the map.
 */

typedef struct _telebot_location_t_ {
    float longitude;	/**< Longitude as defined by sender */
    float latitude; 	/**< Latitude as defined by sender */
} telebot_location_t;

/**
 * @brief This object represent a user's profile pictures.
 */
typedef struct _telebot_userphotos_t {
    int total_count; /**< Total number of profile pictures the target user has */
    telebot_photosize_t **photos; /**< Requested profile pictures (in up to 4 sizes each) */
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
    char * file_id; /**< Unique identifier for this file */
    int file_size; /**< Optional. File size, if known */
    char *file_path; /**<  Optional. File path. */
} telebot_file_t;


/**
 * @brief This object represents a message.
 */
typedef struct _telebot_message_t_ {
    int message_id; /**< Unique message identifier */
    telebot_user_t from; /**< Optional. Sender, can be empty for messages sent to channels */
    long date; /**< Date the message was sent in Unix time */
    telebot_chat_t chat; /**< Conversation the message belongs to */
    telebot_user_t forward_from; /**< Optional. For forwarded messages, sender of the original message */
    long forward_date; /**< Optional. For forwarded messages, date the original message was sent in Unix time */
    struct _telebot_message_t_  *reply_to_message; /**< For replies, the original message. Note that the Message object in this field will not contain further reply_to_message fields even if it itself is a reply. */
    char *text; /**< Optional. For text messages, the actual UTF-8 text of the message */
    telebot_audio_t *audio;	 /**< Optional. Message is an audio file, information about the file */
    telebot_document_t *document; /**< Optional. Message is a general file, information about the file */
    telebot_photosize_t *photo; /**< Optional. Message is a photo, available sizes of the photo */
    telebot_sticker_t *sticker; /**< Optional. Message is a sticker, information about the sticker */
    telebot_video_t *video; /**< Optional. Message is a video, information about the video */
    telebot_voice_t *voice; /**< Optional. Message is a voice message, information about the file */
    char *caption; /**< Optional. Caption for the photo or video */
    telebot_contact_t *contact;	/**< Optional. Message is a shared contact, information about the contact */
    telebot_location_t *location; /**< Optional. Message is a shared location, information about the location */
    telebot_user_t *new_chat_participant; /**< Optional. A new member was added to the group, information about them (this member may be bot itself) */
    telebot_user_t *left_chat_participant; /**< Optional. A member was removed from the group, information about them (this member may be bot itself) */
    char *new_chat_title; /**< Optional. A chat title was changed to this value */
    telebot_photosize_t *new_chat_photo; /**< Optional. A chat photo was change to this value */
    bool delete_chat_photo; /** Optional. Informs that the chat photo was deleted */
    bool group_chat_created; /** Optional. Informs that the group has been created */
} telebot_message_t;

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_CORE_TYPES_H__ */


