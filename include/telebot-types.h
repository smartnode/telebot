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

#ifndef __TELEBOT_TYPES_H__
#define __TELEBOT_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        telebot-types.h
 * @ingroup     TELEBOT_API
 * @brief       This file contains types used to create telegram bot
 * @author      Elmurod Talipov
 * @date        2015-12-13
 */

/**
 * @addtogroup TELEBOT_API
 * @{
 */

/**
 * @brief Enumerations of telegram update types.
 */
typedef enum telebot_update_type {
    UPDATE_TYPE_MESSAGE = 0,
    UPDATE_TYPE_EDITED_MESSAGE,
    UPDATE_TYPE_CHANNEL_POST,
    UPDATE_TYPE_EDITED_CHANNEL_POST,
    UPDATE_TYPE_INLINE_QUERY,
    UPDATE_TYPE_CHOSEN_INLINE_RESULT,
    UPDATE_TYPE_CALLBACK_QUERY,
    UPDATE_TYPE_SHIPPING_QUERY,
    UPDATE_TYPE_PRE_CHECKOUT_QUERY,
    UPDATE_TYPE_MAX
} telebot_update_type_e;

/**
 * @brief This object represents a Telegram user or bot.
 */
typedef struct telebot_user {
    /** Unique identifier for this user or bot. */
    int id;

    /** True, if this user is bot. */
    bool is_bot;

    /** User's or bot's first name. */
    char *first_name;

    /** Optional. User's or bot's last name. */
    char *last_name;

    /** Optional. User's or bot's username. */
    char *username;

    /** Optional. IETF language tag of the user's language. */
    char *language_code;
} telebot_user_t;


/**
 * @brief This object represents a chat.
 */
typedef struct telebot_chat {
    /** Unique identifier for this chat, it may be greater than 32 bits. */
    long long int id;

    /** Type of chat, can be either "private", or "group", "supergroup", or "channel". */
    char *type;

    /** Optional. Title, for supergroups, channels and group chats. */
    char *title;

    /** Optional. Username, for private chats, supergroups and channels if available. */
    char *username;

    /** Optional. First name of the other party in a private chat. */
    char *first_name;

    /** Optional. Last name of the other party in a private chat. */
    char *last_name;

    /** Optional. True, if a group has 'All Members Are Admins' enabled. */
    bool all_members_are_administrators;

    /** Optional. Chat photo. Returned only in getChat. */
    struct telebot_chat_photo *photo;

    /**
     * Optional. Desription, for supergroups and channel chats.
     * Returned only in getChat.
     */
    char *description;

    /**
     * Optional. Chat invite link, for supergroups and channel chats.
     * Returned only in getChat.
     */
    char *invite_link;

    /** Optional. Pinned message, for supergroups. Returned only in getChat. */
    struct telebot_message *pinned_message;

    /**
     * Optional. For supergroups, name of group sticker set.
     * Returned only in getChat.
     */
    char *sticker_set_name;

    /**
     * Optional. True, if the bot can change the group sticker set.
     * Returned only in getChat.
     */
    bool can_set_sticker_set;

} telebot_chat_t;


/**
 * @brief This object represents a message.
 */
typedef struct telebot_message {
    /** Unique message identifier */
    int message_id;

    /** Optional. Sender, can be empty for messages sent to channels */
    struct telebot_user *from;

    /** Date the message was sent in Unix time */
    long date;

    /** Conversation the message belongs to */
    struct telebot_chat *chat;

    /** Optional. For forwarded messages, sender of the original message */
    struct telebot_user *forward_from;

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
    struct telebot_message *reply_to_message;

    /** Optional. Date the message was last edited in Unix time */
    long edit_date;

    /**
     * Optional. The unique identifier of a media message group this message
     * belongs to
     */
    char *media_group_id;

    /** Optional. Signature of the post author for messages in channels */
    char *author_signature;

    /** Optional. For text messages, the actual UTF-8 text of the message */
    char *text;

    /**
     * Optional. For text messages, special entities like usernames, URLs, bot
     * commands, etc. that appear in the text.
     */
    struct telebot_message_entity *entities;
    int count_entities;

    /**
     * Optional. For messages with a caption, special entities like usernames,
     * URLs, bot commands, etc. that appear in the caption.
     */
    struct telebot_message_entity *caption_entities;
    int count_caption_entities;

    /** Optional. Message is an audio file, information about the file */
    struct telebot_audio *audio;

    /** Optional. Message is a general file, information about the file */
    struct telebot_document *document;

    /** Optional. Message is a game, information about the game. */
    struct telebot_game *game; //TODO:define type

    /** Optional. Message is a photo, available sizes of the photo */
    struct telebot_photo *photos;
    int count_photos;

    /** Optional. Message is a sticker, information about the sticker */
    struct telebot_sticker *sticker;

    /** Optional. Message is a video, information about the video */
    struct telebot_video *video;

    /** Optional. Message is a animation, information about the animation */
    struct telebot_animation *animation;

    /** Optional. Message is a voice message, information about the file */
    struct telebot_voice *voice;

    /** Optional. Message is a video note, information about the video message */
    struct telebot_video_note *video_note;

    /** Optional. Caption for the photo or video */
    char *caption;

    /** Optional. Message is a shared contact, information about the contact */
    struct telebot_contact *contact;

    /** Optional. Message is a shared location, information about the location */
    struct telebot_location *location;

    /** Optional. Message is a venue, information about the venue */
    struct telebot_venue *venue;

    /**
     * Optional. New members that were added to the group or supergroup and
     * information about them (the bot itself may be one of these members)
     */
    struct telebot_user *new_chat_members;
    int count_new_chat_members;

    /**
     * Optional. A member was removed from the group, information about them
     * (this member may be the bot itself)
     */
    struct telebot_user *left_chat_members;
    int count_left_chat_members;

    /** Optional. A chat title was changed to this value */
    char *new_chat_title;

    /** Optional. A chat photo was change to this value */
    struct telebot_photo *new_chat_photos;
    int count_new_chat_photos;

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
    long long int migrate_to_chat_id;

    /**
     * Optional. The supergroup has been migrated from a group with the
     * specified identifier, not exceeding 1e13 by absolute value
     */
    long long int migrate_from_chat_id;

    /**
     * Optional. Specified message was pinned. Note that the Message object in
     * this field will not contain further reply_to_message fields even if it
     * is itself a reply.
     */
    struct telebot_message *pinned_message;

    /**
     * Optional. Message is an invoice for a payment, information about the
     * invoice.
     */
    struct telebot_invoice *invoice; //TODO:define type

    /**
     * Optional. Message is a service message about a successful payment,
     * information about the payment.
     */
    struct telebot_successful_payment *successful_payment; //TODO:define type

} telebot_message_t;


/**
 * @brief This object represents one special entity in a text message.
 * For example, hashtags, usernames, URLs, etc.
 */
typedef struct telebot_message_entity {
    /**
     * Type of the entity. Can be mention (@username), hashtag, bot_command,
     * url, email, bold (bold text), italic (italic text), code (monowidth
     * string), pre (monowidth block), text_link (for clickable text URLs),
     * text_mention (for users without usernames)
     */
    char *type;

    /** Offset in UTF-16 code units to the start of the entity */
    int offset;

    /** Length of the entity in UTF-16 code units */
    int length;

    /**
     * Optional. For "text_link" only, url that will be opened after user taps
     * on the text
     */
    char *url;

    /** Optional. For "text_mention" only, the mentioned user */
    telebot_user_t *user;
} telebot_message_entity_t;


/**
 * @brief This object represents one size of a photo or a file / sticker
 * thumbnail.
 */
typedef struct telebot_photo {
    /** Unique identifier for this file */
    char *file_id;

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
typedef struct telebot_document {
    /** Unique file identifier. */
    char *file_id;

    /** Optional. Document thumbnail as defined by sender. */
    struct telebot_photo *thumb;

    /** Optional. Original filename as defined by sender. */
    char *file_name;

    /** Optional. MIME type of the file as defined by sender. */
    char *mime_type;

    /** Optional. File size. */
    int file_size;
} telebot_document_t;


/**
 * @brief This object represents a video file.
 */
typedef struct telebot_video {
    /** Unique identifier for this file */
    char *file_id;

    /** Video width as defined by sender */
    int width;

    /** Video height as defined by sender */
    int height;

    /** Duration of the video in seconds as defined by sender */
    int duration;

    /** Optional. Video thumbnail */
    struct telebot_photo *thumb;

    /** Optional. Mime type of a file as defined by sender */
    char *mime_type;

    /** Optional. File size */
    int file_size;
} telebot_video_t;

/**
 * @brief This object represents a video file.
 */
typedef struct telebot_animation {
    /** Unique identifier for this file */
    char *file_id;

    /** Video width as defined by sender */
    int width;

    /** Video height as defined by sender */
    int height;

    /** Duration of the video in seconds as defined by sender */
    int duration;

    /** Optional. Video thumbnail */
    struct telebot_photo *thumb;

    /** Optional. Mime type of a file as defined by sender */
    char *mime_type;

    /** Optional. File size */
    int file_size;
} telebot_animation_t;

/**
 * @brief This object represents a voice note.
 */
typedef struct telebot_voice {
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
 * @brief This object represents a video message
 * (available in Telegram apps as of v.4.0).
 */
typedef struct telebot_video_note {
    /** Unique identifier for this file */
    char *file_id;

    /** Video width and height as defined by sender */
    int length;

    /** Duration of the video in seconds as defined by sender */
    int duration;

    /** Optional. Video thumbnail */
    struct telebot_photo *thumb;

    /** Optional. File size */
    int file_size;
} telebot_video_note_t;


/**
 * @brief This object represents a phone contact.
 */
typedef struct telebot_contact {
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
typedef struct telebot_location {
   /** Longitude as defined by sender */
    float longitude;

    /** Latitude as defined by sender */
    float latitude;
} telebot_location_t;


/**
 * @brief This object represents a venue.
 */
typedef struct telebot_venue {
    /** Venue location */
    struct telebot_location *location;

    /** Name of the venue */
    char *title;

    /** Address of the venue */
    char *address;

    /** Optional. Foursquare identifier of the venue */
    char *foursquare_id;
} telebot_venue_t;


/**
 * @brief This object represent a user's profile pictures.
 */
typedef struct telebot_user_profile_photos {
    /** Total number of profile pictures the target user has */
    int total_count;

    /** Number of profile pictures in this request */
    int current_count;

    /** Requested profile pictures (in up to 4 sizes each) */
    struct telebot_photo *photos[4];
} telebot_user_profile_photos_t;

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
    char *file_id;

    /** Optional. File size, if known */
    int file_size;

    /**  Optional. File path. */
    char *file_path;
} telebot_file_t;

typedef struct telebot_callback_query {
    /** Unique identifier for this query */
    char *id;

    /** Sender */
    struct telebot_user *from;

    /**
     * Optional. Message with the callback button that originated the query.
     * Note that message content and message date will not be available
     * if the message is too old.
     */
    struct telebot_message *message;

    /**
     * Optional. Identifier of the message sent via the bot in inline mode,
     * that originated the query.
     */
    char *inline_message_id;

    /**
     * Global identifier, uniquely corresponding to the chat to which the
     * message with the callback button was sent. Useful for high scores in
     * games.
     */
    char *chat_instance;

    /**
     * Optional. Data associated with the callback button. Be aware that a bad
     * client can send arbitrary data in this field.
     */
    char *data;

    /**
     * Optional. Short name of a Game to be returned, serves as the unique
     * identifier for the game.
     */
    char *game_short_name;
} telebot_callback_query_t;

/**
 * @brief This object represents a chat photo.
 */
typedef struct telebot_chat_photo {
    /**
     * Unique file identifier of small (160x160) chat photo.
     * This file_id can be used only for photo download.
     */
    char *small_file_id;

    /**
     * Unique file identifier of small (640x640) chat photo.
     * This file_id can be used only for photo download.
     */
    char *big_file_id;
} telebot_chat_photo_t;

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
    /** Unique identifier for this file */
    char *file_id;

    /** Sticker width */
    int width;

    /** Sticker height */
    int height;

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
 * @brief This object represents an incoming update.
 */
typedef struct telebot_update {
    /**
     * The update's unique identifier. Update identifiers start from a certain
     * positive number and increase sequentially.
     */
    int update_id;

    /**
     * The type of the update.
     */
    telebot_update_type_e update_type;

    union {
        /** New incoming message of any kind — text, photo, sticker, etc. */
        telebot_message_t message;

        /** New version of a message that is known to the bot and was edited */
        telebot_message_t edited_message;

        /**  New incoming channel post of any kind — text, photo, sticker, etc. */
        telebot_message_t channel_post;

        /** New version of a channel post that is known to the bot and was edited */
        telebot_message_t edited_channel_post;

        /** New incoming inline query */
        //TODO: telebot_inline_query_t inline_query;

        /**
         * The result of an inline query that was chosen by a user and sent to
         * their chat partner. Please see our documentation on the feedback collecting
         * for details on how to enable these updates for your bot.
         */
        //TODO: telebot_inline_query_result_t chosen_inline_result;

        /** New incoming callback query */
        telebot_callback_query_t callback_query;

        /** New incoming shipping query. Only for invoices with flexible price */
        //TODO: telebot_shipping_query_t shipping_query;

        /** New incoming pre-checkout query. Contains full information about checkout */
        //TODO: telebot_pre_checkout_query_t pre_checkout_query;

    };
} telebot_update_t;

/**
 * @brief Thi object represetns information about the current status of a webhook.
 */
typedef struct telebot_webhook_info {
    /** Webhook URL, may be empty if webhook is not set up */
    char *url;

    /** True, if a custom certificate was provided for webhook certificate checks */
    bool has_custom_certificate;

    /** Number of updates awaiting delivery */
    int pending_update_count;

    /**
     * Optional. Unix time for the most recent error that happened when
     * trying to deliver an update via webhook
     */
    long last_error_date;

    /**
     * Optional. Error message in human-readable format for the most recent
     * error that happened when trying to deliver an update via webhook
     */
    char *last_error_message;

    /**
     * Optional. Maximum allowed number of simultaneous HTTPS connections
     * to the webhook for update delivery
     */
    int max_connections;

    /**
     * Optional. A list of update types the bot is subscribed to.
     * Defaults to all update types.
     */
    telebot_update_type_e allowed_updates[UPDATE_TYPE_MAX];

    /**
     * Optional. Number of updates types.
     */
    int allowed_updates_count;

} telebot_webhook_info_t;

/**
 * @brief This is opaque object to represent a telebot handler.
 */
typedef struct telebot_handler_s *telebot_handler_t;

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_TYPES_H__ */
