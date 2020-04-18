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

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        telebot-types.h
 * @ingroup     TELEBOT_API
 * @brief       This file contains types used to create telegram bot
 * @author      Elmurod Talipov
 * @date        2020-04-19
 */

/**
 * @addtogroup TELEBOT_API
 * @{
 */

/**
 * @brief Enumerations of telegram update types.
 */
typedef enum telebot_update_type {
    TELEBOT_UPDATE_TYPE_MESSAGE = 0,          /**< Message */
    TELEBOT_UPDATE_TYPE_EDITED_MESSAGE,       /**< Edited message */
    TELEBOT_UPDATE_TYPE_CHANNEL_POST,         /**< Channel post */
    TELEBOT_UPDATE_TYPE_EDITED_CHANNEL_POST,  /**< Edited channel post */
    TELEBOT_UPDATE_TYPE_INLINE_QUERY,         /**< Inline query */
    TELEBOT_UPDATE_TYPE_CHOSEN_INLINE_RESULT, /**< Chosen inline result */
    TELEBOT_UPDATE_TYPE_CALLBACK_QUERY,       /**< Callback query */
    TELEBOT_UPDATE_TYPE_SHIPPING_QUERY,       /**< Shipping query */
    TELEBOT_UPDATE_TYPE_PRE_CHECKOUT_QUERY,   /**< Pre-checkout query */
    TELEBOT_UPDATE_TYPE_POLL,                 /**< Poll */
    TELEBOT_UPDATE_TYPE_POLL_ANSWER,          /**< Poll answer */
    TELEBOT_UPDATE_TYPE_MAX                   /**< Number of update types */
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

    /** Optional. Optional. True, if the bot can be invited to groups. Returned only in getMe. */
    bool can_join_groups;

    /** Optional. True, if privacy mode is disabled for the bot. Returned only in getMe. */
    bool can_read_all_group_messages;

    /** Optional. True, if the bot supports inline queries. Returned only in getMe. */
    bool supports_inline_queries;

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

    /** Optional. Default chat member permissions, for groups and supergroups.
     * Returned only in getChat.
     */
    struct telebot_chat_permissions *permissions;

    /**  Optional. For supergroups, the minimum allowed delay between consecutive
     * messages sent by each unpriviledged user. Returned only in getChat.
     */
    int slow_mode_delay;

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
     * Optional. For messages forwarded from channels, information about the
     * original channel
     */
    struct telebot_chat *forward_from_chat;

    /**
     * Optional. For messages forwarded from channels, identifier of the original
     * message in the channel
     */
    int forward_from_message_id;

    /**
     * Optional. For messages forwarded from channels, signature of the post
     * author if present
     */
    char *forward_signature;

    /**
     * Optional. Sender's name for messages forwarded from users who disallow
     * adding a link to their account in forwarded messages
     */
    char *forward_sender_name;

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

    /** Optional. Message is a animation, information about the animation */
    struct telebot_animation *animation;

    /** Optional. Message is a game, information about the game. */
    struct telebot_game *game; //TODO:define type

    /** Optional. Message is a photo, available sizes of the photo */
    struct telebot_photo *photos;
    int count_photos;

    /** Optional. Message is a sticker, information about the sticker */
    struct telebot_sticker *sticker;

    /** Optional. Message is a video, information about the video */
    struct telebot_video *video;

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

    /** Optional. Message is a native poll, information about the poll */
    struct telebot_poll *poll;

    /** Optional. Message is a dice with random value from 1 to 6 */
    struct telebot_dice *dice;

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

    /** Optional. The domain name of the website on which the user has logged in.*/
    char *connected_website;

    /** Telegram Passport data */
    struct telebot_passport_data *passport_data; //TODO:define type

    /**
     * Inline keyboard attached to the message. login_url buttons are
     * represented as ordinary url buttons.
     */
    struct inline_keyboard_markup *reply_markup; //TODO:define type
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

    /** Optional. For "pre" only, the programming language of the entity text */
    char *language;
} telebot_message_entity_t;


/**
 * @brief This object represents one size of a photo or a file / sticker
 * thumbnail.
 */
typedef struct telebot_photo {
    /** Identifier for this file, which can be used to download or reuse the file */
    char *file_id;

    /**
     * Unique identifier for this file, which is supposed to be the same over
     * time and for different bots. Can't be used to download or reuse the file.
     */
    char *file_unique_id;

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
    /** Identifier for this file, which can be used to download or reuse the file */
    char *file_id;

    /**
     * Unique identifier for this file, which is supposed to be the same over
     * time and for different bots. Can't be used to download or reuse the file.
     */
    char *file_unique_id;

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

    /** Optional. Thumbnail of the album cover to which the music file belongs */
    struct telebot_photo *thumb;
} telebot_audio_t;


/**
 * @brief This object represents a general file (as opposed to photos, voice
 * messages and audio files).
 */
typedef struct telebot_document {
    /** Identifier for this file, which can be used to download or reuse the file */
    char *file_id;

    /**
     * Unique identifier for this file, which is supposed to be the same over
     * time and for different bots. Can't be used to download or reuse the file.
     */
    char *file_unique_id;

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
    /** Identifier for this file, which can be used to download or reuse the file */
    char *file_id;

    /**
     * Unique identifier for this file, which is supposed to be the same over
     * time and for different bots. Can't be used to download or reuse the file.
     */
    char *file_unique_id;

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
    /** Identifier for this file, which can be used to download or reuse the file */
    char *file_id;

    /**
     * Unique identifier for this file, which is supposed to be the same over
     * time and for different bots. Can't be used to download or reuse the file.
     */
    char *file_unique_id;

    /** Video width as defined by sender */
    int width;

    /** Video height as defined by sender */
    int height;

    /** Duration of the video in seconds as defined by sender */
    int duration;

    /** Optional. Video thumbnail */
    struct telebot_photo *thumb;

    /** Optional. Original animation filename as defined by sender */
    char *file_name;

    /** Optional. Mime type of a file as defined by sender */
    char *mime_type;

    /** Optional. File size */
    int file_size;
} telebot_animation_t;

/**
 * @brief This object represents a voice note.
 */
typedef struct telebot_voice {
    /** Identifier for this file, which can be used to download or reuse the file */
    char *file_id;

    /**
     * Unique identifier for this file, which is supposed to be the same over
     * time and for different bots. Can't be used to download or reuse the file.
     */
    char *file_unique_id;

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
    /** Identifier for this file, which can be used to download or reuse the file */
    char *file_id;

    /**
     * Unique identifier for this file, which is supposed to be the same over
     * time and for different bots. Can't be used to download or reuse the file.
     */
    char *file_unique_id;

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

    /** Optional. Additional data about the contact in the form of a vCard */
    char *vcard;
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

    /**
     * Optional. Foursquare type of the venue. (For example,
     * "arts_entertainment/default", "arts_entertainment/aquarium" or
     * "food/icecream".)
     */
    char *foursquare_type;
} telebot_venue_t;

/**
 * @brief This object contains information about one answer option in a poll.
 */
typedef struct telebot_poll_option {
    /** Option text, 1-100 characters */
    char *text;

    /** Number of users that voted for this option */
    int voter_count;
} telebot_poll_option_t;

/**
 * @brief This object represents an answer of a user in a non-anonymous poll.
 */
typedef struct telebot_poll_answer {
    /** Unique poll identifier */
    char *poll_id;

    /** The user, who changed the answer to the poll */
    struct telebot_user *user;

    /**
     * 0-based identifiers of answer options, chosen by the user.
     * May be empty if the user retracted their vote.
     */
    int *option_ids;

    /* Number of option ids */
    int count_option_ids;
} telebot_poll_answer_t;

/**
 * @brief This object contains information about a poll.
 */
typedef struct telebot_poll {
    /** Unique poll identifier */
    char *id;

    /** Poll question, 1-255 characters */
    char *question;

    /** List of poll options */
    telebot_poll_option_t *options;

    /* Number of options */
    int count_options;

    /** Total number of users that voted in the poll */
    int total_voter_count;

    /** True, if the poll is closed */
    bool is_closed;

    /** True, if the poll is anonymous */
    bool is_anonymous;

    /** Poll type, currently can be "regular" or "quiz" */
    char *type;

    /** True, if the poll allows multiple answers */
    bool allows_multiple_answers;

    /**
     * Optional. 0-based identifier of the correct answer option.
     * Available only for polls in the quiz mode, which are closed, or was sent
     * (not forwarded) by the bot or to the private chat with the bot.
     */
    int correct_option_id;
} telebot_poll_t;

/**
 * @brief This object represents a dice with random value from 1 to 6.
 */
typedef struct telebot_dice {
    /** Value of the dice, 1-6*/
    int value;
} telebot_dice_t;

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
    /** Identifier for this file, which can be used to download or reuse the file */
    char *file_id;

    /**
     * Unique identifier for this file, which is supposed to be the same over
     * time and for different bots. Can't be used to download or reuse the file.
     */
    char *file_unique_id;

    /** Optional. File size, if known */
    int file_size;

    /**  Optional. File path. */
    char *file_path;
} telebot_file_t;

/**
 * @brief This object represents type of a poll, which is allowed to be created
 * and sent when the corresponding button is pressed.
 */
typedef struct telebot_keyboard_button_poll_type {

    /**
     * Optional. If quiz is passed, the user will be allowed to create only polls
     * in the quiz mode. If regular is passed, only regular polls will be allowed.
     * Otherwise, the user will be allowed to create a poll of any type.
     */
    char *type;
} telebot_keyboard_button_poll_type_t;

/**
 * @brief This object represents one button of the reply keyboard. For simple
 * text buttons String can be used instead of this object to specify text of
 * the button. Optional fields request_contact, request_location, and request_poll
 * are mutually exclusive.
 */
typedef struct telebot_keyboard_button {
    /**
     * Text of the button. If none of the optional fields are used, it will be
     * sent as a message when the button is pressed.
     */
    char *text;

    /**
     * Optional. If True, the user's phone number will be sent as a contact
     * when the button is pressed. Available in private chats only.
     */
    bool request_contact;
    /**
     * Optional. If True, the user's current location will be sent when the
     * button is pressed. Available in private chats only.
     */
    bool request_location;

    /**
     * Optional. If specified, the user will be asked to create a poll and send
     * it to the bot when the button is pressed. Available in private chats only
     */
    telebot_keyboard_button_poll_type_t *request_poll;
} telebot_keyboard_button_t;

/**
 * @brief This object represents a custom keyboard with reply options
 */
typedef struct telebot_reply_keyboard_markup {
    /**
     * Array of button rows, each represented by an Array of KeyboardButton
     * objects
     */
    telebot_keyboard_button_t *keyboard;

    /* Number of keyboard rows */
    int keyboard_rows;

    /* Number of keyboard columns */
    int keyboard_cols;

    /**
     * Optional. Requests clients to resize the keyboard vertically for optimal
     * fit (e.g., make the keyboard smaller if there are just two rows of buttons).
     * Defaults to false, in which case the custom keyboard is always of the
     * same height as the app's standard keyboard.
     */
    bool resize_keyboard;

    /**
     * Optional. Requests clients to hide the keyboard as soon as it's been used.
     * The keyboard will still be available, but clients will automatically
     * display the usual letter-keyboard in the chat – the user can press
     * a special button in the input field to see the custom keyboard again.
     * Defaults to false.
     */
    bool one_time_keyboard;

    /**
     * Optional. Use this parameter if you want to show the keyboard to specific
     * users only. Targets: 1) users that are @mentioned in the text of
     * the Message object; 2) if the bot's message is a reply
     * (has reply_to_message_id), sender of the original message.
     */
    bool selective;
} telebot_reply_keyboard_markup_t;

/** @brief Upon receiving a message with this object, Telegram clients will
 * remove the current custom keyboard and display the default letter-keyboard.
 * By default, custom keyboards are displayed until a new keyboard is sent
 * by a bot. An exception is made for one-time keyboards that are hidden
 * immediately after the user presses a button (see #telebot_reply_keyboard_markup_t).
 */
typedef struct telebot_reply_keyboard_remove
{
    /**
     * Requests clients to remove the custom keyboard (user will not be able
     * to summon this keyboard; if you want to hide the keyboard from sight
     * but keep it accessible, use one_time_keyboard in ReplyKeyboardMarkup)
     */
    bool remove_keyboard;

    /**
     * Optional. Use this parameter if you want to remove the keyboard for
     * specific users only. Targets: 1) users that are @mentioned in the
     * text of the Message object; 2) if the bot's message is a reply
     * (has reply_to_message_id), sender of the original message.
     */
    bool selective;
} telebot_reply_keyboard_remove_t;

/**
 * @brief This object represents an incoming callback query from a callback
 * button in an inline keyboard. If the button that originated the query was
 * attached to a message sent by the bot, the field message will be present.
 * If the button was attached to a message sent via the bot (in inline mode),
 * the field inline_message_id will be present. Exactly one of the fields data
 * or game_short_name will be present.
 */
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
 * @brief Upon receiving a message with this object, Telegram clients will
 * display a reply interface to the user (act as if the user has selected the
 * bot's message and tapped ’Reply'). This can be extremely useful if you want
 * to create user-friendly step-by-step interfaces without having to sacrifice
 * privacy mode.
 */
typedef struct telebot_force_reply
{
    /**
     * Shows reply interface to the user, as if they manually selected the
     * bot's message and tapped 'Reply'
     */
    bool force_reply;

    /**
     * Optional. Use this parameter if you want to force reply from specific
     * users only. Targets:
     * 1) users that are @mentioned in the text of the Message object;
     * 2) if the bot's message is a reply (has reply_to_message_id),
     * sender of the original message.
     */
    bool selective;
} telebot_force_reply_t;


/**
 * @brief This object represents a chat photo.
 */
typedef struct telebot_chat_photo {
    /**
     * File identifier of small (160x160) chat photo. This file_id can be used
     * only for photo download and only for as long as the photo is not changed.
     */
    char *small_file_id;

    /**
     * Unique file identifier of small (160x160) chat photo, which is supposed
     * to be the same over time and for different bots. Can't be used to
     * download or reuse the file.
     */
    char *small_file_unique_id;

    /**
     * File identifier of big (640x640) chat photo. This file_id can be used
     * only for photo download and only for as long as the photo is not changed.
     */
    char *big_file_id;

    /** Unique file identifier of big (640x640) chat photo, which is supposed
     * to be the same over time and for different bots. Can't be used to
     * download or reuse the file.
     */
    char *big_file_unique_id;
} telebot_chat_photo_t;

/**
 * @brief This object contains information about one member of a chat.
 */
typedef struct telebot_chat_member {
    /** Information about the user. */
    struct telebot_user *user;

    /**
     * The member's status in the chat. Can be "creator", "administrator"”,
     * "member", "restricted", "left" or "kicked".
     */
    char *status;

    /** Optional. Owner and administrators only. Custom title for this user. */
    char *custom_title;

    /**
     * Optional. Restricted and kicked only. Date when restrictions will be
     * lifted for this user; unix time.
     */
    long until_date;

    /**
     * Optional. Administrators only. True, if the bot is allowed to edit
     * administrator privileges of that user.
     */
    bool can_be_edited;

    /**
     * Optional. Administrators only. True, if the administrator can post in
     * the channel; channels only.
     */
    bool can_post_messages;

    /**
     * Optional. Administrators only. True, if the administrator can edit
     * messages of other users and can pin messages; channels only.
     */
    bool can_edit_messages;

    /**
     * Optional. Administrators only. True, if the administrator can delete
     * messages of other users.
     */
    bool can_delete_messages;

    /**
     * Optional. Administrators only. True, if the administrator can restrict,
     * ban or unban chat members.
     */
    bool can_restrict_members;

    /**
     * Optional. Administrators only. True, if the administrator can add new
     * administrators with a subset of his own privileges or demote
     * administrators that he has promoted, directly or indirectly
     * (promoted by administrators that were appointed by the user).
     */
    bool can_promote_members;

    /**
     * Optional. Administrators and restricted only. True, if the user is
     * allowed to change the chat title, photo and other settings.
     */
    bool can_change_info;

    /**
     * Optional. Administrators and restricted only. True, if the user is
     * allowed to invite new users to the chat.
     */
    bool can_invite_users;

    /**
     * Optional. Administrators and restricted only. True, if the user is
     * allowed to pin messages; groups and supergroups only.
     */
    bool can_pin_messages;

    /**
     * Optional. Restricted only. True, if the user is a member of the chat at
     * the moment of the request
     */
    bool is_member;

    /**
     * Optional. Restricted only. True, if the user is allowed to send text
     * messages, contacts, locations and venues
     */
    bool can_send_messages;

    /**
     * Optional. Restricted only. True, if the user is allowed to send audios,
     * documents, photos, videos, video notes and voice notes
     */
    bool can_send_media_messages;

    /** Optional. Restricted only. True, if the user is allowed to send polls. */
    bool can_send_polls;

    /**
     * Optional. Restricted only. True, if the user is allowed to send animations,
     * games, stickers and use inline bots
     */
    bool can_send_other_messages;

    /**
     * Optional. Restricted only. True, if the user is allowed to add web page
     * previews to their messages
     */
    bool can_add_web_page_previews;
} telebot_chat_member_t;

/**
 * @brief Describes actions that a non-administrator user is allowed to take in a chat.
 */
typedef struct telebot_chat_permissions {
    /**
     * Optional. True, if the user is allowed to send text messages, contacts,
     * locations and venues.
     */
    bool can_send_messages;

    /**
     * Optional. True, if the user is allowed to send audios, documents,
     * photos, videos, video notes and voice notes, implies can_send_messages.
     */
    bool can_send_media_messages;

    /**
     * Optional. True, if the user is allowed to send polls, implies
     * can_send_messages.
     */
    bool can_send_polls;

    /**
     * Optional. True, if the user is allowed to send animations, games,
     * stickers and use inline bots, implies can_send_media_messages.
     */
    bool can_send_other_messages;

    /**
     * Optional. True, if the user is allowed to add web page previews to their
     * messages, implies can_send_media_messages.
     */
    bool can_add_web_page_previews;

    /**
     * Optional. True, if the user is allowed to change the chat title, photo
     * and other settings. Ignored in public supergroups
     */
    bool can_change_info;

    /** Optional. True, if the user is allowed to invite new users to the chat. */
    bool can_invite_users;

    /**
     * Optional. True, if the user is allowed to pin messages. Ignored in
     * public supergroups.
     */
    bool can_pin_messages;
} telebot_chat_permissions_t;

/** @brief This object represents a bot command. */
typedef struct telebot_bot_command {
    /**
     * Text of the command, 1-32 characters. Can contain only lowercase English
     * letters, digits and underscores.
     */
    char *command;

    /** Description of the command, 3-256 characters. */
    char *description;
} telebot_bot_command_t;

/**
 * @brief Contains information about why a request was unsuccessful.
 */
typedef struct telebot_response_paramters {
    /**
     * Optional. The group has been migrated to a supergroup with the specified
     * identifier. This number may be greater than 32 bits and some programming
     * languages may have difficulty/silent defects in interpreting it.
     * But it is smaller than 52 bits, so a signed 64 bit integer or
     * double-precision float type are safe for storing this identifier.
     */
    int migrate_to_chat_id;

    /**
     * Optional. In case of exceeding flood control, the number of seconds
     * left to wait before the request can be repeated.
     */
    int retry_after;
} telebot_response_paramters_t;

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

        /**
         * New poll state. Bots receive only updates about stopped polls and
         * polls, which are sent by the bot
         */
        telebot_poll_t poll;

        /**
         * A user changed their answer in a non-anonymous poll. Bots receive
         * new votes only in polls that were sent by the bot itself.
         */
        telebot_poll_answer_t poll_anser;
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
    telebot_update_type_e allowed_updates[TELEBOT_UPDATE_TYPE_MAX];

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
