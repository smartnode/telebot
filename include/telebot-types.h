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
    TELEBOT_UPDATE_TYPE_BUSINESS_CONNECTION,  /**< Business connection */
    TELEBOT_UPDATE_TYPE_BUSINESS_MESSAGE,     /**< Business message */
    TELEBOT_UPDATE_TYPE_EDITED_BUSINESS_MESSAGE, /**< Edited business message */
    TELEBOT_UPDATE_TYPE_DELETED_BUSINESS_MESSAGES, /**< Deleted business messages */
    TELEBOT_UPDATE_TYPE_MESSAGE_REACTION,     /**< Message reaction updated */
    TELEBOT_UPDATE_TYPE_MESSAGE_REACTION_COUNT, /**< Message reaction count updated */
    TELEBOT_UPDATE_TYPE_INLINE_QUERY,         /**< Inline query */
    TELEBOT_UPDATE_TYPE_CHOSEN_INLINE_RESULT, /**< Chosen inline result */
    TELEBOT_UPDATE_TYPE_CALLBACK_QUERY,       /**< Callback query */
    TELEBOT_UPDATE_TYPE_SHIPPING_QUERY,       /**< Shipping query */
    TELEBOT_UPDATE_TYPE_PRE_CHECKOUT_QUERY,   /**< Pre-checkout query */
    TELEBOT_UPDATE_TYPE_PURCHASED_PAID_MEDIA, /**< User purchased paid media */
    TELEBOT_UPDATE_TYPE_POLL,                 /**< Poll */
    TELEBOT_UPDATE_TYPE_POLL_ANSWER,          /**< Poll answer */
    TELEBOT_UPDATE_TYPE_MY_CHAT_MEMBER,       /**< My chat member updated */
    TELEBOT_UPDATE_TYPE_CHAT_MEMBER,          /**< Chat member updated */
    TELEBOT_UPDATE_TYPE_CHAT_JOIN_REQUEST,    /**< Chat join request */
    TELEBOT_UPDATE_TYPE_CHAT_BOOST,           /**< Chat boost updated */
    TELEBOT_UPDATE_TYPE_REMOVED_CHAT_BOOST,   /**< Chat boost removed */
    TELEBOT_UPDATE_TYPE_MAX                   /**< Number of update types */
} telebot_update_type_e;

/**
 * @brief Describes the birthdate of a user.
 */
typedef struct telebot_birthdate {
    /** Day of the user's birth; 1-31 */
    int day;

    /** Month of the user's birth; 1-12 */
    int month;

    /** Optional. Year of the user's birth */
    int year;
} telebot_birthdate_t;

/**
 * @brief Contains information about the start page settings of a Telegram Business account.
 */
typedef struct telebot_business_intro {
    /** Optional. Title text of the business intro */
    char *title;

    /** Optional. Message text of the business intro */
    char *message;

    /** Optional. Sticker of the business intro */
    struct telebot_sticker *sticker;
} telebot_business_intro_t;

/**
 * @brief Contains information about the location of a Telegram Business account.
 */
typedef struct telebot_business_location {
    /** Address of the business */
    char *address;

    /** Optional. Location of the business */
    struct telebot_location *location;
} telebot_business_location_t;

/**
 * @brief Describes an interval of time during which a business is open.
 */
typedef struct telebot_business_opening_hours_interval {
    /** The minute's sequence number in a week, starting on Monday, marking the start of the time interval during which the business is open; 0 - 7 * 24 * 60 */
    int opening_minute;

    /** The minute's sequence number in a week, starting on Monday, marking the end of the time interval during which the business is open; 0 - 8 * 24 * 60 */
    int closing_minute;
} telebot_business_opening_hours_interval_t;

/**
 * @brief Describes the opening hours of a business.
 */
typedef struct telebot_business_opening_hours {
    /** Unique name of the time zone for which the opening hours are defined */
    char *time_zone_name;

    /** List of time intervals describing business opening hours */
    struct telebot_business_opening_hours_interval *opening_hours;
    int count_opening_hours;
} telebot_business_opening_hours_t;

/**
 * @brief Represents the rights of a business bot.
 */
typedef struct telebot_business_bot_rights {
    bool can_reply;
    bool can_read_messages;
    bool can_delete_sent_messages;
    bool can_delete_all_messages;
    bool can_edit_name;
    bool can_edit_bio;
    bool can_edit_profile_photo;
    bool can_edit_username;
    bool can_change_gift_settings;
    bool can_view_gifts_and_stars;
    bool can_convert_gifts_to_stars;
    bool can_transfer_and_upgrade_gifts;
    bool can_transfer_stars;
    bool can_manage_stories;
} telebot_business_bot_rights_t;

/**
 * @brief Describes the connection of the bot with a business account.
 */
typedef struct telebot_business_connection {
    char *id;
    struct telebot_user *user;
    long long int user_chat_id;
    long date;
    bool can_reply;
    bool is_enabled;
} telebot_business_connection_t;

/**
 * @brief This object is received when messages are deleted from a connected business account.
 */
typedef struct telebot_business_messages_deleted {
    char *business_connection_id;
    struct telebot_chat *chat;
    int *message_ids;
    int count_message_ids;
} telebot_business_messages_deleted_t;

/**
 * @brief This object represents a Telegram user or bot.
 */
typedef struct telebot_user {
    /** Unique identifier for this user or bot. */
    long long int id;

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

    /** Optional. True, if this user is a Telegram Premium user. */
    bool is_premium;

    /** Optional. True, if this user added the bot to the attachment menu. */
    bool added_to_attachment_menu;

    /** Optional. Optional. True, if the bot can be invited to groups. Returned only in getMe. */
    bool can_join_groups;

    /** Optional. True, if privacy mode is disabled for the bot. Returned only in getMe. */
    bool can_read_all_group_messages;

    /** Optional. True, if the bot supports inline queries. Returned only in getMe. */
    bool supports_inline_queries;

    /** Optional. True, if the bot can be connected to a Telegram Business account to receive its messages. Returned only in getMe. */
    bool can_connect_to_business;

    /** Optional. True, if the bot has a main Web App. Returned only in getMe. */
    bool has_main_web_app;

    /** Optional. True, if the bot has forum topic mode enabled in private chats. Returned only in getMe. */
    bool has_topics_enabled;

    /** Optional. True, if the bot allows users to create and delete topics in private chats. Returned only in getMe. */
    bool allows_users_to_create_topics;

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

    /** Optional. True, if the supergroup chat is a forum (has topics enabled). */
    bool is_forum;

    /** Optional. True, if the chat is the direct messages chat of a channel. */
    bool is_direct_messages;

    /** Optional. Identifier of the accent color for the chat name and backgrounds. */
    int accent_color_id;

    /** Optional. The maximum number of reactions that can be set on a message in the chat. */
    int max_reaction_count;

    /** Optional. Chat photo. Returned only in getChat. */
    struct telebot_chat_photo *photo;

    /** Optional. If non-empty, the list of all active chat usernames;
     * for private chats, supergroups and channels. Returned only in getChat.
     */
    char **active_usernames;
    int count_active_usernames;

    /** Optional. For private chats, the date of birth of the user. */
    struct telebot_birthdate *birthdate;

    /** Optional. For private chats with business accounts, the intro of the business. */
    struct telebot_business_intro *business_intro;

    /** Optional. For private chats with business accounts, the location of the business. */
    struct telebot_business_location *business_location;

    /** Optional. For private chats with business accounts, the opening hours of the business. */
    struct telebot_business_opening_hours *business_opening_hours;

    /** Optional. For private chats, the personal channel of the user. */
    struct telebot_chat *personal_chat;

    /** Optional. Information about the corresponding channel chat; for direct messages chats only. */
    struct telebot_chat *parent_chat;

    /**
     * Optional. Custom emoji identifier of emoji status of the other party in
     * a private chat. Returned only in getChat.
     */
    char *emoji_status_custom_emoji_id;

    /** Optional. Expiration date of the emoji status of the chat or the other party in a private chat, in Unix time, if any. */
    long emoji_status_expiration_date;

    /** Optional. Bio of the other party in a private chat. Returned only in getChat. */
    char *bio;

    /**
     * Optional. True, if privacy settings of the other party in the private
     * chat allows to use tg://user?id=<user_id> links only in chats with the user.
     * Returned only in getChat.
     */
    bool has_private_forwards;

    /**
     * Optional. True, if the privacy settings of the other party restrict
     * sending voice and video note messages in the private chat.
     * Returned only in getChat.
     */
    bool has_restricted_voice_and_video_messages;

    /**
     * Optional. True, if users need to join the supergroup before they can
     * send messages. Returned only in getChat.
     */
    bool join_to_send_messages;

    /**
     * Optional. True, if all users directly joining the supergroup need
     * to be approved by supergroup administrators. Returned only in getChat.
     */
    bool join_by_request;

    /**
     * Optional. Description, for supergroups and channel chats.
     * Returned only in getChat.
     */
    char *description;

    /**
     * Optional. Primary invite link, for groups, supergroups and channel chats.
     * Returned only in getChat.
     */
    char *invite_link;

    /** Optional. The most recent pinned message (by sending date).
     * Returned only in getChat.
     */
    struct telebot_message *pinned_message;

    /**
     * Optional. Default chat member permissions, for groups and supergroups.
     * Returned only in getChat.
     */
    struct telebot_chat_permissions *permissions;

    /**
     * Optional. For supergroups, the minimum allowed delay between consecutive
     * messages sent by each unpriviledged user. Returned only in getChat.
     */
    int slow_mode_delay;

    /** Optional. For supergroups, the minimum number of boosts that a non-administrator user needs. */
    int unrestrict_boost_count;

    /**
     * Optional. The time after which all messages sent to the chat will be
     * automatically deleted; in seconds. Returned only in getChat.
     */
    int message_auto_delete_time;

    /** Optional. True, if aggressive anti-spam checks are enabled in the supergroup. */
    bool has_aggressive_anti_spam_enabled;

    /** Optional. True, if non-administrators can only get the list of bots and administrators in the chat. */
    bool has_hidden_members;

    /**
     * Optional. True, if messages from the chat can't be forwarded to other chats.
     * Returned only in getChat.
     */
    bool has_protected_content;

    /** Optional. True, if new chat members will have access to old messages. */
    bool has_visible_history;

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

    /** Optional. For supergroups, the name of the group's custom emoji sticker set. */
    char *custom_emoji_sticker_set_name;

    /**
     * Optional. Unique identifier for the linked chat, i.e. the discussion
     * group identifier for a channel and vice versa; for supergroups and
     * channel chats. Returned only in getChat.
     */
    long long int linked_chat_id;

    /**
     * Optional. For supergroups, the location to which the supergroup is connected.
     * Returned only in getChat.
     */
    struct telebot_chat_location *location;
} telebot_chat_t;

/**
 * @brief This object represents a game.
 */
typedef struct telebot_game {
    /** Title of the game */
    char *title;

    /** Description of the game */
    char *description;

    /** Photo that will be displayed in the game message in chats. */
    struct telebot_photo *photo;

    /** Number of photos */
    int count_photo;

    /**
     * Optional. Brief description of the game or high scores included in the
     * game message.
     */
    char *text;

    /**
     * Optional. Special entities that appear in text, such as usernames, URLs,
     * bot commands, etc.
     */
    struct telebot_message_entity *text_entities;
    int count_text_entities;

    /**
     * Optional. Animation that will be displayed in the game message in chats.
     */
    struct telebot_animation *animation;
} telebot_game_t;

/**
 * @brief This object represents a shipping address.
 */
typedef struct telebot_shipping_address {
    /** ISO 3166-1 alpha-2 country code */
    char *country_code;

    /** State, if applicable */
    char *state;

    /** City */
    char *city;

    /** First line for the address */
    char *street_line1;

    /** Second line for the address */
    char *street_line2;

    /** Address post code */
    char *post_code;
} telebot_shipping_address_t;

/**
 * @brief This object represents information about an order.
 */
typedef struct telebot_order_info {
    /** Optional. User name */
    char *name;

    /** Optional. User's phone number */
    char *phone_number;

    /** Optional. User's email address */
    char *email;

    /** Optional. User's shipping address */
    struct telebot_shipping_address *shipping_address;
} telebot_order_info_t;

/**
 * @brief This object contains basic information about an invoice.
 */
typedef struct telebot_invoice {
    /** Product name */
    char *title;

    /** Product description */
    char *description;

    /**
     * Unique bot deep-linking parameter that can be used to generate this
     * invoice
     */
    char *start_parameter;

    /** Three-letter ISO 4217 currency code */
    char *currency;

    /**
     * Total amount in the smallest units of the currency (integer, not
     * float/double).
     */
    int total_amount;
} telebot_invoice_t;

/**
 * @brief This object contains basic information about a successful payment.
 */
typedef struct telebot_successful_payment {
    /** Three-letter ISO 4217 currency code */
    char *currency;

    /**
     * Total amount in the smallest units of the currency (integer, not
     * float/double).
     */
    int total_amount;

    /** Bot specified invoice payload */
    char *invoice_payload;

    /** Optional. Identifier of the shipping option chosen by the user */
    char *shipping_option_id;

    /** Optional. Order info provided by the user */
    struct telebot_order_info *order_info;

    /** Telegram payment identifier */
    char *telegram_payment_charge_id;

    /** Provider payment identifier */
    char *provider_payment_charge_id;
} telebot_successful_payment_t;

/**
 * @brief This object represents an internal identifier of a user in Telegram Passport.
 */
typedef struct telebot_passport_file {
    /** Identifier for this file, which can be used to download or reuse the file */
    char *file_id;

    /**
     * Unique identifier for this file, which is supposed to be the same over
     * time and for different bots. Can't be used to download or reuse the file.
     */
    char *file_unique_id;

    /** File size */
    int file_size;

    /** Unix time when the file was uploaded */
    long date;
} telebot_passport_file_t;

/**
 * @brief This object represents one element of the Telegram Passport data.
 */
typedef struct telebot_encrypted_passport_element {
    /**
     * Element type. One of "personal_details", "passport", "internal_passport",
     * "driver_license", "id_card", "internal_passport", "address",
     * "utility_bill", "bank_statement", "rental_agreement",
     * "passport_registration", "temporary_registration", "phone_number", "email".
     */
    char *type;

    /**
     * Optional. Base64-encoded encrypted Telegram Passport element data
     * provided by the user.
     */
    char *data;

    /** Optional. User's verified phone number. */
    char *phone_number;

    /** Optional. User's verified email address. */
    char *email;

    /** Optional. Array of encrypted files with documents provided by the user. */
    struct telebot_passport_file *files;
    int count_files;

    /** Optional. Encrypted file with the front side of the document. */
    struct telebot_passport_file *front_side;

    /** Optional. Encrypted file with the reverse side of the document. */
    struct telebot_passport_file *reverse_side;

    /** Optional. Encrypted file with the selfie of the user holding a document. */
    struct telebot_passport_file *selfie;

    /** Optional. Array of encrypted files with translated versions of documents. */
    struct telebot_passport_file *translation;
    int count_translation;

    /** Base64-encoded element hash for using in PassportElementErrorUnspecified */
    char *hash;
} telebot_encrypted_passport_element_t;

/**
 * @brief This object represents credentials required to decrypt the data.
 */
typedef struct telebot_encrypted_credentials {
    /** Base64-encoded encrypted JSON-serialized data with unique user's payload */
    char *data;

    /** Base64-encoded data hash */
    char *hash;

    /** Base64-encoded secret used for data decryption */
    char *secret;
} telebot_encrypted_credentials_t;

/**
 * @brief Contains information about Telegram Passport data which was shared
 * with the bot by the user.
 */
typedef struct telebot_passport_data {
    /**
     * Array with information about documents and other Telegram Passport
     * elements that were shared with the bot
     */
    struct telebot_encrypted_passport_element *data;
    int count_data;

    /** Encrypted credentials required to decrypt the data */
    struct telebot_encrypted_credentials *credentials;
} telebot_passport_data_t;

/**
 * @brief This object represents a service message about a user in the chat
 * triggered another user's proximity alert while sharing Live Location.
 */
typedef struct telebot_proximity_alert_triggered {
    /** User that triggered the alert */
    struct telebot_user *traveler;

    /** User that set the alert */
    struct telebot_user *watcher;

    /** The distance between the users */
    int distance;
} telebot_proximity_alert_triggered_t;

/**
 * @brief This object represents a service message about a new forum topic
 * created in the chat.
 */
typedef struct telebot_forum_topic_created {
    /** Name of the topic */
    char *name;

    /** Color of the topic icon in RGB format */
    int icon_color;

    /** Optional. Unique identifier of the custom emoji used as the topic icon */
    char *icon_custom_emoji_id;
} telebot_forum_topic_created_t;

/**
 * @brief This object represents a service message about a forum topic closed
 * in the chat.
 */
typedef struct telebot_forum_topic_closed {
    /** Empty struct as per Telegram API */
    bool dummy;
} telebot_forum_topic_closed_t;

/**
 * @brief This object represents a service message about a forum topic reopened
 * in the chat.
 */
typedef struct telebot_forum_topic_reopened {
    /** Empty struct as per Telegram API */
    bool dummy;
} telebot_forum_topic_reopened_t;

/**
 * @brief This object represents a service message about a video chat scheduled
 * in the chat.
 */
typedef struct telebot_video_chat_scheduled {
    /** Point in time (Unix timestamp) when the video chat is supposed to be started by a chat administrator */
    long start_date;
} telebot_video_chat_scheduled_t;

/**
 * @brief This object represents a service message about a video chat started
 * in the chat.
 */
typedef struct telebot_video_chat_started {
    /** Empty struct as per Telegram API */
    bool dummy;
} telebot_video_chat_started_t;

/**
 * @brief This object represents a service message about a video chat ended
 * in the chat.
 */
typedef struct telebot_video_chat_ended {
    /** Video chat duration in seconds */
    int duration;
} telebot_video_chat_ended_t;

/**
 * @brief This object represents a service message about new members invited to
 * a video chat.
 */
typedef struct telebot_video_chat_participants_invited {
    /** New members that were invited to the video chat */
    struct telebot_user *users;
    int count_users;
} telebot_video_chat_participants_invited_t;

/**
 * @brief This object represents data sent by a Web App to the bot.
 */
typedef struct telebot_web_app_data {
    /** The data. Be aware that a bad client can send arbitrary data in this field. */
    char *data;

    /**
     * Text of the web_app keyboard button from which the Web App was opened.
     * Be aware that a bad client can send arbitrary data in this field.
     */
    char *button_text;
} telebot_web_app_data_t;

/**
 * @brief A placeholder, currently holds no information. Use BotFather to set up your game.
 */
typedef struct telebot_callback_game {
    bool dummy;
} telebot_callback_game_t;

/**
 * @brief This object represents a parameter of the inline keyboard button used
 * to automatically authorize a user.
 */
typedef struct telebot_login_url {
    /** An HTTPS URL used to automatically authorize the user. */
    char *url;

    /** Optional. New text of the button in forwarded messages. */
    char *forward_text;

    /** Optional. Username of a bot, which will be used for user authorization. */
    char *bot_username;

    /** Optional. Pass True to request the permission for your bot to send messages to the user. */
    bool request_write_access;
} telebot_login_url_t;

/**
 * @brief This object represents an inline button that switches the current
 * user to inline mode in a chosen chat.
 */
typedef struct telebot_switch_inline_query_chosen_chat {
    /** Optional. The default inline query to be inserted in the input field. */
    char *query;

    /** Optional. True, if private chats with users can be chosen */
    bool allow_user_chats;

    /** Optional. True, if private chats with bots can be chosen */
    bool allow_bot_chats;

    /** Optional. True, if group and supergroup chats can be chosen */
    bool allow_group_chats;

    /** Optional. True, if channel chats can be chosen */
    bool allow_channel_chats;
} telebot_switch_inline_query_chosen_chat_t;

/**
 * @brief This object represents an inline keyboard button that copies
 * specified text to the clipboard.
 */
typedef struct telebot_copy_text_button {
    /** The text to be copied to the clipboard; 1-256 characters */
    char *text;
} telebot_copy_text_button_t;

/**
 * @brief This object represents one button of an inline keyboard. You must use
 * exactly one of the optional fields.
 */
typedef struct telebot_inline_keyboard_button {
    /** Label text on the button */
    char *text;

    /** Optional. HTTP or tg:// url to be opened when button is pressed */
    char *url;

    /**
     * Optional. Data to be sent in a callback query to the bot when button is
     * pressed, 1-64 bytes
     */
    char *callback_data;

    /**
     * Optional. Description of the Web App that will be launched when the user
     * presses the button.
     */
    struct telebot_web_app_info *web_app;

    /**
     * Optional. An HTTPS URL used to automatically authorize the user.
     */
    struct telebot_login_url *login_url;

    /**
     * Optional. If set, pressing the button will prompt the user to select one
     * of their chats, open that chat and insert the bot's username and the
     * specified inline query in the input field. Can be empty, in which case
     * just the bot's username will be inserted.
     */
    char *switch_inline_query;

    /**
     * Optional. If set, pressing the button will insert the bot's username and
     * the specified inline query in the current chat's input field. Can be
     * empty, in which case just the bot's username will be inserted.
     */
    char *switch_inline_query_current_chat;

    /**
     * Optional. If set, pressing the button will prompt the user to select one
     * of their chats of the specified type.
     */
    struct telebot_switch_inline_query_chosen_chat *switch_inline_query_chosen_chat;

    /**
     * Optional. Description of the button that copies the specified text to the clipboard.
     */
    struct telebot_copy_text_button *copy_text;

    /**
     * Optional. Description of the game that will be launched when the user
     * presses the button.
     */
    struct telebot_callback_game *callback_game;

    /**
     * Optional. Specify True, to send a Pay button.
     * NOTE: This type of button must always be the first button in the first row.
     */
    bool pay;
} telebot_inline_keyboard_button_t;

/**
 * @brief This object represents an inline keyboard that appears right next to
 * the message it belongs to.
 */
typedef struct telebot_inline_keyboard_markup {
    /**
     * Array of button rows, each represented by an Array of
     * InlineKeyboardButton objects
     */
    telebot_inline_keyboard_button_t *inline_keyboard;

    /** Number of rows */
    int rows;

    /** Number of columns */
    int cols;
} telebot_inline_keyboard_markup_t;

/**
 * @brief This object represents a service message about a forum topic edited
 * in the chat.
 */
typedef struct telebot_forum_topic_edited {
    /** Optional. New name of the topic, if it was edited */
    char *name;

    /** Optional. New identifier of the custom emoji used as the topic icon, if it was edited */
    char *icon_custom_emoji_id;
} telebot_forum_topic_edited_t;

/**
 * @brief This object represents a service message about a general forum topic
 * hidden in the chat.
 */
typedef struct telebot_general_forum_topic_hidden {
    /** Empty struct as per Telegram API */
    bool dummy;
} telebot_general_forum_topic_hidden_t;

/**
 * @brief This object represents a service message about a general forum topic
 * unhidden in the chat.
 */
typedef struct telebot_general_forum_topic_unhidden {
    /** Empty struct as per Telegram API */
    bool dummy;
} telebot_general_forum_topic_unhidden_t;

/**
 * @brief This object represents a service message about a user allowing a bot
 * to write messages after adding the bot to the attachment menu or launching
 * a Web App from a link.
 */
typedef struct telebot_write_access_allowed {
    /** Optional. Name of the Web App which was launched from a link */
    char *web_app_name;
} telebot_write_access_allowed_t;

/**
 * @brief This object contains information about the user whose identifier was
 * shared with the bot using a KeyboardButtonRequestUser button.
 */
typedef struct telebot_user_shared {
    /** Identifier of the request */
    int request_id;

    /** Identifier of the shared user. */
    long long int user_id;
} telebot_user_shared_t;

/**
 * @brief This object contains information about the chat whose identifier was
 * shared with the bot using a KeyboardButtonRequestChat button.
 */
typedef struct telebot_chat_shared {
    /** Identifier of the request */
    int request_id;

    /** Identifier of the shared chat. */
    long long int chat_id;
} telebot_chat_shared_t;

/**
 * @brief This object represents a chat invite link.
 */
typedef struct telebot_chat_invite_link {
    /** The invite link. */
    char *invite_link;

    /** Creator of the link */
    struct telebot_user *creator;

    /** True, if users joining the chat via the link need to be approved by chat administrators */
    bool creates_join_request;

    /** True, if the link is primary */
    bool is_primary;

    /** True, if the link is revoked */
    bool is_revoked;

    /** Optional. Invite link name */
    char *name;

    /** Optional. Point in time (Unix timestamp) when the link will expire or has expired */
    long expire_date;

    /** Optional. The maximum number of users that can be members of the chat simultaneously after joining the chat via this invite link; 1-99999 */
    int member_limit;

    /** Optional. Number of pending join requests created using this link */
    int pending_join_request_count;
} telebot_chat_invite_link_t;

/**
 * @brief This object represents changes in the status of a chat member.
 */
typedef struct telebot_chat_member_updated {
    /** Chat the user belongs to */
    struct telebot_chat *chat;

    /** Performer of the action, which resulted in the change */
    struct telebot_user *from;

    /** Date the change was effected in Unix time */
    long date;

    /** Previous information about the chat member */
    struct telebot_chat_member *old_chat_member;

    /** New information about the chat member */
    struct telebot_chat_member *new_chat_member;

    /** Optional. Chat invite link, which was used by the user to join the chat; for joining by invite link events only. */
    struct telebot_chat_invite_link *invite_link;

    /** Optional. True, if the user joined the chat via a chat folder invite link */
    bool via_chat_folder_invite_link;
} telebot_chat_member_updated_t;

/**
 * @brief This object represents a join request sent to a chat.
 */
typedef struct telebot_chat_join_request {
    /** Chat to which the request was sent */
    struct telebot_chat *chat;

    /** User that sent the join request */
    struct telebot_user *from;

    /** Identifier of a private chat with the user who sent the join request. */
    long long int user_chat_id;

    /** Date the request was sent in Unix time */
    long date;

    /** Optional. Bio of the user. */
    char *bio;

    /** Optional. Chat invite link that was used by the user to send the join request */
    struct telebot_chat_invite_link *invite_link;
} telebot_chat_join_request_t;

/**
 * @brief This object represents a change of a reaction on a message performed
 * by a user.
 */
typedef struct telebot_message_reaction_updated {
    /** The chat where the reaction was changed */
    struct telebot_chat *chat;

    /** Unique identifier of the message inside the chat */
    int message_id;

    /** Optional. The user that changed the reaction, if the reaction was changed on behalf of a user */
    struct telebot_user *user;

    /** Optional. The chat on behalf of which the reaction was changed, if the reaction was changed on behalf of a chat */
    struct telebot_chat *actor_chat;

    /** Date of the change in Unix time */
    long date;

    /** List of reaction types that were set by the user */
    struct telebot_reaction_type *old_reaction;
    int count_old_reaction;

    /** List of reaction types that are set by the user */
    struct telebot_reaction_type *new_reaction;
    int count_new_reaction;
} telebot_message_reaction_updated_t;

/**
 * @brief This object represents reaction changes on a message with anonymous
 * reactions.
 */
typedef struct telebot_message_reaction_count_updated {
    /** The chat where the reactions were changed */
    struct telebot_chat *chat;

    /** Unique identifier of the message inside the chat */
    int message_id;

    /** Date of the change in Unix time */
    long date;

    /** List of reactions that are present on the message */
    struct telebot_reaction_count *reactions;
    int count_reactions;
} telebot_message_reaction_count_updated_t;

/**
 * @brief This object represents a reaction type.
 */
typedef struct telebot_reaction_type {
    /** Type of the reaction, currently can be "emoji" or "custom_emoji" */
    char *type;

    /** Optional. Reaction emoji. */
    char *emoji;

    /** Optional. Custom emoji identifier. */
    char *custom_emoji_id;
} telebot_reaction_type_t;

/**
 * @brief This object represents a reaction count.
 */
typedef struct telebot_reaction_count {
    /** Type of the reaction */
    struct telebot_reaction_type type;

    /** Number of times the reaction was added */
    int total_count;
} telebot_reaction_count_t;

/**
 * @brief This object represents a boost added to a chat.
 */
typedef struct telebot_chat_boost_updated {
    /** Chat which was boosted */
    struct telebot_chat *chat;

    /** Information about the boost */
    struct telebot_chat_boost *boost;
} telebot_chat_boost_updated_t;

/**
 * @brief This object contains information about a chat boost.
 */
typedef struct telebot_chat_boost {
    /** Unique identifier of the boost */
    char *boost_id;

    /** Point in time (Unix timestamp) when the chat was boosted */
    long add_date;

    /** Point in time (Unix timestamp) when the boost will automatically expire, unless the booster's Telegram Premium subscription is prolonged */
    long expiration_date;

    /** Source of the added boost */
    struct telebot_chat_boost_source *source;
} telebot_chat_boost_t;

/**
 * @brief This object represents the source of a chat boost.
 */
typedef struct telebot_chat_boost_source {
    /** Source of the boost, one of "premium", "gift_code", "giveaway" */
    char *source;

    /** Optional. User that boosted the chat */
    struct telebot_user *user;
} telebot_chat_boost_source_t;

/**
 * @brief This object represents a boost removed from a chat.
 */
typedef struct telebot_chat_boost_removed {
    /** Chat from which the boost was removed */
    struct telebot_chat *chat;

    /** Unique identifier of the boost */
    char *boost_id;

    /** Point in time (Unix timestamp) when the boost was removed */
    long remove_date;

    /** Source of the removed boost */
    struct telebot_chat_boost_source *source;
} telebot_chat_boost_removed_t;

/**
 * @brief This object describes the origin of a message.
 */
typedef struct telebot_message_origin {
    /** Type of the message origin, can be "user", "hidden_user", "chat", "channel" */
    char *type;

    /** Date the message was sent originally in Unix time */
    long date;

    /** Optional. User that sent the message originally */
    struct telebot_user *sender_user;

    /** Optional. Name of the user that sent the message originally */
    char *sender_user_name;

    /** Optional. Chat that sent the message originally */
    struct telebot_chat *sender_chat;

    /** Optional. For messages originally sent by an anonymous chat administrator, original message author signature */
    char *author_signature;

    /** Optional. Unique message identifier inside the chat */
    int message_id;
} telebot_message_origin_t;

/**
 * @brief This object contains information about the quoted part of a message
 * that is replied to by the given message.
 */
typedef struct telebot_text_quote {
    /** Text of the quoted part of a message */
    char *text;

    /** Optional. Special entities that appear in the quote. */
    struct telebot_message_entity *entities;
    int count_entities;

    /** Approximate quote position in the original message */
    int position;

    /** Optional. True, if the quote was chosen manually by the message sender. */
    bool is_manual;
} telebot_text_quote_t;

/**
 * @brief This object contains information about a story.
 */
typedef struct telebot_story {
    /** Chat that posted the story */
    struct telebot_chat *chat;

    /** Unique identifier for the story in the chat */
    int id;
} telebot_story_t;

/**
 * @brief This object contains information about the message that is being
 * replied to, which may come from another chat or forum topic.
 */
typedef struct telebot_external_reply_info {
    /** Origin of the message replied to by the given message */
    struct telebot_message_origin origin;

    /** Optional. Chat the original message belongs to. */
    struct telebot_chat *chat;

    /** Optional. Unique message identifier inside the original chat. */
    int message_id;

    /** Optional. Options used for link preview generation for the original message */
    struct telebot_link_preview_options *link_preview_options;

    /** Optional. Message is an animation, information about the animation */
    struct telebot_animation *animation;

    /** Optional. Message is an audio file, information about the file */
    struct telebot_audio *audio;

    /** Optional. Message is a general file, information about the file */
    struct telebot_document *document;

    /** Optional. Message contains paid media; information about the paid media */
    struct telebot_paid_media_info *paid_media;

    /** Optional. Message is a photo, available sizes of the photo */
    struct telebot_photo *photos;
    int count_photos;

    /** Optional. Message is a sticker, information about the sticker */
    struct telebot_sticker *sticker;

    /** Optional. Message is a forwarded story */
    struct telebot_story *story;

    /** Optional. Message is a video, information about the video */
    struct telebot_video *video;

    /** Optional. Message is a video note, information about the video message */
    struct telebot_video_note *video_note;

    /** Optional. Message is a voice message, information about the file */
    struct telebot_voice *voice;

    /** Optional. True, if the message media is covered by a spoiler animation */
    bool has_media_spoiler;

    /** Optional. Message is a shared contact, information about the contact */
    struct telebot_contact *contact;

    /** Optional. Message is a dice with random value */
    struct telebot_dice *dice;

    /** Optional. Message is a game, information about the game. */
    struct telebot_game *game;

    /** Optional. Message is a giveaway, information about the giveaway */
    struct telebot_giveaway *giveaway;

    /** Optional. A giveaway with public winners was completed */
    struct telebot_giveaway_winners *giveaway_winners;

    /** Optional. Message is an invoice for a payment, information about the invoice. */
    struct telebot_invoice *invoice;

    /** Optional. Message is a shared location, information about the location */
    struct telebot_location *location;

    /** Optional. Message is a native poll, information about the poll */
    struct telebot_poll *poll;

    /** Optional. Message is a venue, information about the venue */
    struct telebot_venue *venue;
} telebot_external_reply_info_t;

/**
 * @brief Describes the options used for link preview generation.
 */
typedef struct telebot_link_preview_options {
    /** Optional. True, if the link preview is disabled */
    bool is_disabled;

    /** Optional. URL to use for the link preview. */
    char *url;

    /** Optional. True, if the media in the link preview is supposed to be shrunk */
    bool prefer_small_media;

    /** Optional. True, if the media in the link preview is supposed to be enlarged */
    bool prefer_large_media;

    /** Optional. True, if the link preview must be shown above the message text */
    bool show_above_text;
} telebot_link_preview_options_t;

/**
 * @brief This object describes paid media.
 */
typedef struct telebot_paid_media {
    /** Type of the paid media, can be "preview", "photo", "video" */
    char *type;

    /** Optional. Media width as defined by the sender */
    int width;

    /** Optional. Media height as defined by the sender */
    int height;

    /** Optional. Duration of the media in seconds as defined by the sender */
    int duration;

    /** Optional. The photo */
    struct telebot_photo *photos;
    int count_photos;

    /** Optional. The video */
    struct telebot_video *video;
} telebot_paid_media_t;

/**
 * @brief Describes the paid media added to a message.
 */
typedef struct telebot_paid_media_info {
    /** The number of Telegram Stars that must be paid to buy access to the media */
    int star_count;

    /** Information about the paid media */
    struct telebot_paid_media *paid_media;
    int count_paid_media;
} telebot_paid_media_info_t;

/**
 * @brief This object represents a message about a scheduled giveaway.
 */
typedef struct telebot_giveaway {
    /** The list of chats which the user must join to participate in the giveaway */
    struct telebot_chat *chats;
    int count_chats;

    /** Point in time (Unix timestamp) when winners of the giveaway will be selected */
    long winners_selection_date;

    /** The number of users which are supposed to be selected as winners of the giveaway */
    int winner_count;

    /** Optional. True, if only users who join the chats after the giveaway started should be eligible to win */
    bool only_new_members;

    /** Optional. True, if the list of giveaway winners will be visible to everyone */
    bool has_public_winners;

    /** Optional. Description of additional giveaway prize */
    char *prize_description;

    /** Optional. A list of two-letter ISO 3166-1 alpha-2 country codes */
    char **country_codes;
    int count_country_codes;

    /** Optional. The number of Telegram Stars to be split between giveaway winners */
    int prize_star_count;

    /** Optional. The number of months the Telegram Premium subscription won from the giveaway will be active for */
    int premium_subscription_month_count;
} telebot_giveaway_t;

/**
 * @brief This object represents a message about the completion of a giveaway
 * with public winners.
 */
typedef struct telebot_giveaway_winners {
    /** The chat that created the giveaway */
    struct telebot_chat *chat;

    /** Identifier of the message with the giveaway in the chat */
    int giveaway_message_id;

    /** Point in time (Unix timestamp) when winners of the giveaway were selected */
    long winners_selection_date;

    /** Total number of winners in the giveaway */
    int winner_count;

    /** List of winners of the giveaway */
    struct telebot_user *winners;
    int count_winners;

    /** Optional. The number of other chats the user had to join in order to be eligible for the giveaway */
    int additional_chat_count;

    /** Optional. The number of Telegram Stars that were split between giveaway winners */
    int prize_star_count;

    /** Optional. The number of months the Telegram Premium subscription won from the giveaway will be active for */
    int premium_subscription_month_count;

    /** Optional. Number of undistributed prizes */
    int unclaimed_prize_count;

    /** Optional. True, if only users who had joined the chats after the giveaway started were eligible to win */
    bool only_new_members;

    /** Optional. True, if the giveaway was canceled because the payment for it was refunded */
    bool was_refunded;

    /** Optional. Description of additional giveaway prize */
    char *prize_description;
} telebot_giveaway_winners_t;

/**
 * @brief This object represents a service message about a refunded payment.
 */
typedef struct telebot_refunded_payment {
    /** Three-letter ISO 4217 currency code */
    char *currency;

    /** Total amount in the smallest units of the currency */
    int total_amount;

    /** Bot specified invoice payload */
    char *invoice_payload;

    /** Telegram payment identifier */
    char *telegram_payment_charge_id;

    /** Optional. Provider payment identifier */
    char *provider_payment_charge_id;
} telebot_refunded_payment_t;

/**
 * @brief This object contains information about the users whose identifiers
 * were shared with the bot using a KeyboardButtonRequestUsers button.
 */
typedef struct telebot_users_shared {
    /** Identifier of the request */
    int request_id;

    /** Information about users shared with the bot. */
    struct telebot_shared_user *users;
    int count_users;
} telebot_users_shared_t;

/**
 * @brief This object contains information about a user that was shared with
 * the bot.
 */
typedef struct telebot_shared_user {
    /** Identifier of the shared user. */
    long long int user_id;

    /** Optional. First name of the user */
    char *first_name;

    /** Optional. Last name of the user */
    char *last_name;

    /** Optional. Username of the user */
    char *username;

    /** Optional. Available sizes of the chat photo */
    struct telebot_photo *photo;
    int count_photo;
} telebot_shared_user_t;

/**
 * @brief This object represents a gift that can be sent by the bot.
 */
typedef struct telebot_gift {
    /** Unique identifier of the gift */
    char *id;

    /** The sticker that represents the gift */
    struct telebot_sticker *sticker;

    /** The number of Telegram Stars that must be paid to send the sticker */
    int star_count;

    /** Optional. The number of Telegram Stars that must be paid to upgrade the gift */
    int upgrade_star_count;

    /** Optional. True, if the gift can only be purchased by Telegram Premium subscribers */
    bool is_premium;

    /** Optional. True, if the gift can be used (after being upgraded) to customize a user's appearance */
    bool has_colors;

    /** Optional. The total number of gifts of this type that can be sent */
    int total_count;

    /** Optional. The number of remaining gifts */
    int remaining_count;
} telebot_gift_t;

/**
 * @brief Describes a regular gift that was sent or received.
 */
typedef struct telebot_gift_info {
    /** Information about the gift */
    struct telebot_gift gift;

    /** Optional. Message text added to the gift */
    char *text;

    /** Optional. Special entities that appear in the text */
    struct telebot_message_entity *entities;
    int count_entities;

    /** Optional. True, if the sender and gift text are shown only to the receiver */
    bool is_private;
} telebot_gift_info_t;

/**
 * @brief Describes a unique gift that was sent or received.
 */
typedef struct telebot_unique_gift_info {
    /** Information about the gift */
    struct telebot_unique_gift *gift;

    /** Origin of the gift */
    char *origin;

    /** Optional. Currency in which the payment for the gift was done */
    char *last_resale_currency;

    /** Optional. Price paid for the gift */
    long long int last_resale_amount;
} telebot_unique_gift_info_t;

/**
 * @brief This object describes a unique gift that was upgraded from a regular gift.
 */
typedef struct telebot_unique_gift {
    /** Identifier of the regular gift */
    char *gift_id;

    /** Unique name of the gift */
    char *name;

    /** Unique number of the upgraded gift */
    int number;

    /** Optional. Model of the gift */
    struct telebot_sticker *sticker;
} telebot_unique_gift_t;

/**
 * @brief This object represents a service message about a user boosting a chat.
 */
typedef struct telebot_chat_boost_added {
    /** Number of boosts added by the user */
    int boost_count;
} telebot_chat_boost_added_t;

/**
 * @brief This object represents a chat background.
 */
typedef struct telebot_chat_background {
    /** Type of the background */
    char *type;
} telebot_chat_background_t;

/**
 * @brief This object represents a service message about the creation of a scheduled giveaway.
 */
typedef struct telebot_giveaway_created {
    /** Optional. The number of Telegram Stars to be split between giveaway winners */
    int prize_star_count;
} telebot_giveaway_created_t;

/**
 * @brief This object represents a service message about the completion of a giveaway
 * without public winners.
 */
typedef struct telebot_giveaway_completed {
    /** Number of winners in the giveaway */
    int winner_count;

    /** Optional. Number of undistributed prizes */
    int unclaimed_prize_count;

    /** Optional. Message with the giveaway that was completed */
    struct telebot_message *giveaway_message;

    /** Optional. True, if the giveaway is a Telegram Star giveaway */
    bool is_star_giveaway;
} telebot_giveaway_completed_t;

/**
 * @brief Describes an amount of Telegram Stars.
 */
typedef struct telebot_star_amount {
    /** Integer amount of Telegram Stars, rounded to 0; can be negative */
    int amount;

    /** Optional. The number of 1/1000000000 shares of Telegram Stars */
    int nanostar_amount;
} telebot_star_amount_t;

/**
 * @brief Contains information about the affiliate that received a commission.
 */
typedef struct telebot_affiliate_info {
    /** Optional. The bot or the user that received an affiliate commission */
    struct telebot_user *affiliate_user;

    /** Optional. The chat that received an affiliate commission */
    struct telebot_chat *affiliate_chat;

    /** The number of Telegram Stars received by the affiliate for each 1000 Stars */
    int commission_per_mille;

    /** Integer amount of Telegram Stars received by the affiliate */
    int amount;

    /** Optional. The number of 1/1000000000 shares of Telegram Stars */
    int nanostar_amount;
} telebot_affiliate_info_t;

/**
 * @brief This object describes the state of a revenue withdrawal operation.
 */
typedef struct telebot_revenue_withdrawal_state {
    /** Type of the state, currently one of "pending", "succeeded", "failed" */
    char *type;

    /** Optional. Date the withdrawal was completed in Unix time */
    long date;

    /** Optional. An HTTPS URL that can be used to see transaction details */
    char *url;
} telebot_revenue_withdrawal_state_t;

/**
 * @brief This object describes the source of a transaction.
 */
typedef struct telebot_transaction_partner {
    /** Type of the transaction partner */
    char *type;

    /** Optional. Type of the transaction */
    char *transaction_type;

    /** Optional. Information about the user */
    struct telebot_user *user;

    /** Optional. Information about the chat */
    struct telebot_chat *chat;

    /** Optional. Information about the affiliate */
    struct telebot_affiliate_info *affiliate;

    /** Optional. Bot-specified invoice payload */
    char *invoice_payload;

    /** Optional. The duration of the paid subscription */
    int subscription_period;

    /** Optional. Information about the paid media bought by the user */
    struct telebot_paid_media *paid_media;
    int count_paid_media;

    /** Optional. Bot-specified paid media payload */
    char *paid_media_payload;

    /** Optional. The gift sent to the user by the bot */
    struct telebot_gift *gift;

    /** Optional. Number of months the gifted Telegram Premium subscription will be active for */
    int premium_subscription_duration;

    /** Optional. State of the transaction if the transaction is outgoing */
    struct telebot_revenue_withdrawal_state *withdrawal_state;

    /** Optional. Information about the bot that sponsored the affiliate program */
    struct telebot_user *sponsor_user;

    /** Optional. The number of Telegram Stars received by the bot for each 1000 Stars */
    int commission_per_mille;

    /** Optional. The number of successful requests that exceeded regular limits */
    int request_count;
} telebot_transaction_partner_t;

/**
 * @brief Describes a Telegram Star transaction.
 */
typedef struct telebot_star_transaction {
    /** Unique identifier of the transaction. */
    char *id;

    /** Integer amount of Telegram Stars transferred by the transaction */
    int amount;

    /** Optional. The number of 1/1000000000 shares of Telegram Stars */
    int nanostar_amount;

    /** Date the transaction was created in Unix time */
    long date;

    /** Optional. Source of an incoming transaction */
    struct telebot_transaction_partner *source;

    /** Optional. Receiver of an outgoing transaction */
    struct telebot_transaction_partner *receiver;
} telebot_star_transaction_t;

/**
 * @brief Contains a list of Telegram Star transactions.
 */
typedef struct telebot_star_transactions {
    /** The list of transactions */
    struct telebot_star_transaction *transactions;
    int count_transactions;
} telebot_star_transactions_t;

/**
 * @brief This object contains information about a paid media purchase.
 */
typedef struct telebot_paid_media_purchased {
    /** User who purchased the media */
    struct telebot_user *from;

    /** Bot-specified paid media payload */
    char *paid_media_payload;
} telebot_paid_media_purchased_t;

/**
 * @brief This object represents an error in the Telegram Passport element.
 */
typedef struct telebot_passport_element_error {
    /** Error source */
    char *source;

    /** The section of the user's Telegram Passport which has the error */
    char *type;

    /** Name of the data field which has the error */
    char *field_name;

    /** Base64-encoded data hash */
    char *data_hash;

    /** Error message */
    char *message;

    /** Base64-encoded file hash */
    char *file_hash;

    /** List of base64-encoded file hashes */
    char **file_hashes;
    int count_file_hashes;

    /** Base64-encoded element hash */
    char *element_hash;
} telebot_passport_element_error_t;

/**
 * @brief This object represents a game high score row.
 */
typedef struct telebot_game_high_score {
    /** Position in high score table for the game */
    int position;

    /** User */
    struct telebot_user *user;

    /** Score */
    int score;
} telebot_game_high_score_t;

/**
 * @brief This object represents a message.
 */
typedef struct telebot_message {
    /** Unique message identifier */
    int message_id;

    /**
     * Unique identifier of a message thread to which the message belongs;
     * for supergroups only
     */
    int message_thread_id;

    /** Optional. Sender, can be empty for messages sent to channels */
    struct telebot_user *from;

    /**
     * Optional. Sender of the message, sent on behalf of a chat. For example,
     * the channel itself for channel posts, the supergroup itself for messages
     * from anonymous group administrators, the linked channel for messages
     * automatically forwarded to the discussion group. For backward compatibility,
     * the field from contains a fake sender user in non-channel chats,
     * if the message was sent on behalf of a chat.
     */
    struct telebot_chat *sender_chat;

    /** Optional. If the sender of the message boosted the chat, the number of boosts */
    int sender_boost_count;

    /** Optional. The bot that actually sent the message on behalf of the business account. */
    struct telebot_user *sender_business_bot;

    /** Optional. Unique identifier of the business connection */
    char *business_connection_id;

    /** Date the message was sent in Unix time */
    long date;

    /** Conversation the message belongs to */
    struct telebot_chat *chat;

    /** Optional. Information about the original message for forwarded messages */
    struct telebot_message_origin *forward_origin;

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

    /** Optional. True, if the message is sent to a forum topic */
    bool is_topic_message;

    /**
     * Optional. True, if the message is a channel post that was automatically
     * forwarded to the connected discussion group
     */
    bool is_automatic_forward;

    /**
     * For replies, the original message. Note that the Message object in this
     * field will not contain further reply_to_message fields even if it itself
     * is a reply.
     */
    struct telebot_message *reply_to_message;

    /** Optional. Information about the message that is being replied to */
    struct telebot_external_reply_info *external_reply;

    /** Optional. For replies that quote part of the original message, the quoted part */
    struct telebot_text_quote *quote;

    /** Optional. For replies to a story, the original story */
    struct telebot_story *reply_to_story;

    /** Optional. Bot through which the message was sent */
    struct telebot_user *via_bot;

    /** Optional. Date the message was last edited in Unix time */
    long edit_date;

    /** Optional. True, if the message can't be forwarded */
    bool has_protected_content;

    /** Optional. True, if the message is a paid post */
    bool is_paid_post;

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

    /** Optional. Options used for link preview generation for the message */
    struct telebot_link_preview_options *link_preview_options;

    /** Optional. Unique identifier of the message effect added to the message */
    char *effect_id;

    /**
     * Optional. Message is an animation, information about the animation.
     * For backward compatibility, when this field is set, the document field
     * will also be set
     */
    struct telebot_animation *animation;

    /** Optional. Message is an audio file, information about the file */
    struct telebot_audio *audio;

    /** Optional. Message is a general file, information about the file */
    struct telebot_document *document;

    /** Optional. Message contains paid media; information about the paid media */
    struct telebot_paid_media_info *paid_media;

    /** Optional. Message is a photo, available sizes of the photo */
    struct telebot_photo *photos;
    int count_photos;

    /** Optional. Message is a sticker, information about the sticker */
    struct telebot_sticker *sticker;

    /** Optional. Message is a video, information about the video */
    struct telebot_video *video;

    /** Optional. Message is a video note, information about the video message */
    struct telebot_video_note *video_note;

    /** Optional. Message is a voice message, information about the file */
    struct telebot_voice *voice;

    /** Optional. Caption for the photo or video */
    char *caption;

    /**
     * Optional. For messages with a caption, special entities like usernames,
     * URLs, bot commands, etc. that appear in the caption.
     */
    struct telebot_message_entity *caption_entities;
    int count_caption_entities;

    /** Optional. True, if the caption must be shown above the message media */
    bool show_caption_above_media;

    /** Optional. Message is a shared contact, information about the contact */
    struct telebot_contact *contact;

    /** Optional. Message is a dice with random value from 1 to 6 */
    struct telebot_dice *dice;

    /** Optional. Message is a game, information about the game. */
    struct telebot_game *game;

    /** Optional. Message is a native poll, information about the poll */
    struct telebot_poll *poll;

    /** Optional. Message is a venue, information about the venue */
    struct telebot_venue *venue;

    /** Optional. Message is a shared location, information about the location */
    struct telebot_location *location;

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

    /** Optional. Service message: auto-delete timer settings changed in the chat */
    struct telebot_message_auto_delete_timer_changed *message_auto_delete_timer_changed;

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
    struct telebot_invoice *invoice;

    /**
     * Optional. Message is a service message about a successful payment,
     * information about the payment.
     */
    struct telebot_successful_payment *successful_payment;

    /** Optional. Message is a service message about a refunded payment */
    struct telebot_refunded_payment *refunded_payment;

    /** Optional. Service message: the user allowed the bot to write messages */
    struct telebot_write_access_allowed *write_access_allowed;

    /** Optional. The domain name of the website on which the user has logged in.*/
    char *connected_website;

    /** Telegram Passport data */
    struct telebot_passport_data *passport_data;

    /**
     * Optional. Service message. A user in the chat triggered another user's
     * proximity alert while sharing Live Location.
     */
    struct telebot_proximity_alert_triggered *proximity_alert_triggered;

    /** Optional. Service message: forum topic created. */
    struct telebot_forum_topic_created *forum_topic_created;

    /** Optional. Service message: forum topic edited. */
    struct telebot_forum_topic_edited *forum_topic_edited;

    /** Optional. Service message: forum topic closed. */
    struct telebot_forum_topic_closed *forum_topic_closed;

    /** Optional. Service message: forum topic reopened. */
    struct telebot_forum_topic_reopened *forum_topic_reopened;

    /** Optional. Service message: general forum topic hidden. */
    struct telebot_general_forum_topic_hidden *general_forum_topic_hidden;

    /** Optional. Service message: general forum topic unhidden. */
    struct telebot_general_forum_topic_unhidden *general_forum_topic_unhidden;

    /** Optional. Service message: video chat scheduled. */
    struct telebot_video_chat_scheduled *video_chat_scheduled;

    /** Optional. Service message: video chat started. */
    struct telebot_video_chat_started *video_chat_started;

    /** Optional. Service message: video chat ended. */
    struct telebot_video_chat_ended *video_chat_ended;

    /** Optional. Service message: new participants invited to a video chat. */
    struct telebot_video_chat_participants_invited *video_chat_participants_invited;

    /** Optional. Service message: data sent by a Web App. */
    struct telebot_web_app_data *web_app_data;

    /** Optional. Service message: users were shared with the bot */
    struct telebot_users_shared *users_shared;

    /** Optional. Service message: a chat was shared with the bot */
    struct telebot_chat_shared *chat_shared;

    /** Optional. Service message: a regular gift was sent or received */
    struct telebot_gift_info *gift;

    /** Optional. Service message: a unique gift was sent or received */
    struct telebot_unique_gift_info *unique_gift;

    /** Optional. Service message: user boosted the chat */
    struct telebot_chat_boost_added *boost_added;

    /** Optional. Service message: chat background set */
    struct telebot_chat_background *chat_background_set;

    /** Optional. Service message: giveaway created */
    struct telebot_giveaway_created *giveaway_created;

    /** Optional. Service message: giveaway completed */
    struct telebot_giveaway_completed *giveaway_completed;

    /** Optional. True, if the message media is covered by a spoiler animation */
    bool has_media_spoiler;

    /**
     * Inline keyboard attached to the message. login_url buttons are
     * represented as ordinary url buttons.
     */
    struct telebot_inline_keyboard_markup *reply_markup;
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
 * @brief This object represent a list of gifts.
 */
typedef struct telebot_gifts {
    /** The list of gifts */
    telebot_gift_t *gifts;
    /** Number of gifts */
    int count;
} telebot_gifts_t;

/**
 * @brief This object represents a list of user profile audios.
 */
typedef struct telebot_user_profile_audios {
    /** Total number of audios in the list */
    int total_count;
    /** The list of audios */
    telebot_audio_t *audios;
    /** Number of audios in the list */
    int count;
} telebot_user_profile_audios_t;

/**
 * @brief This object represents a gift received by a user.
 */
typedef struct telebot_user_gift {
    /** Unique identifier of the gift */
    char *gift_id;

    /** Optional. The user that sent the gift */
    struct telebot_user *sender_user;

    /** Optional. Message text added to the gift */
    char *text;

    /** Optional. Special entities that appear in the text */
    struct telebot_message_entity *entities;
    /** Number of entities */
    int count_entities;

    /** Information about the gift */
    telebot_gift_t *gift;

    /** Date the gift was sent */
    long date;

    /** Optional. Message identifier of the message with the gift */
    int message_id;

    /** Optional. The number of Telegram Stars that must be paid to upgrade the gift */
    int upgrade_star_count;

    /** Optional. True, if the gift is upgraded */
    bool is_upgraded;
} telebot_user_gift_t;

/**
 * @brief This object represents a list of gifts received by a user.
 */
typedef struct telebot_user_gifts {
    /** Total number of gifts in the list */
    int total_count;
    /** The list of gifts */
    telebot_user_gift_t *gifts;
    /** Number of gifts in the list */
    int count;
} telebot_user_gifts_t;

/**
 * @brief This object represents a list of user chat boosts.
 */
typedef struct telebot_user_chat_boosts {
    /** The list of boosts */
    telebot_chat_boost_t *boosts;
    /** Number of boosts */
    int count;
} telebot_user_chat_boosts_t;

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
 * @brief This object defines the criteria used to request suitable users.
 */
typedef struct telebot_keyboard_button_request_users {
    /** Identifier of the request */
    int request_id;

    /** Optional. Pass True to request bots, pass False to request regular users. */
    bool user_is_bot;

    /** Optional. Pass True to request premium users, pass False to request non-premium users. */
    bool user_is_premium;

    /** Optional. The maximum number of users to be selected; 1-10. */
    int max_quantity;

    /** Optional. Pass True to request the users' first and last names */
    bool request_name;

    /** Optional. Pass True to request the users' usernames */
    bool request_username;

    /** Optional. Pass True to request the users' photos */
    bool request_photo;
} telebot_keyboard_button_request_users_t;

/**
 * @brief This object defines the criteria used to request a suitable chat.
 */
typedef struct telebot_keyboard_button_request_chat {
    /** Identifier of the request */
    int request_id;

    /** Pass True to request a channel chat, pass False to request a group or a supergroup chat. */
    bool chat_is_channel;

    /** Optional. Pass True to request a forum supergroup, pass False to request a non-forum chat. */
    bool chat_is_forum;

    /** Optional. Pass True to request a supergroup or a channel with a username, pass False to request a chat without a username. */
    bool chat_has_username;

    /** Optional. Pass True to request a chat owned by the user. */
    bool chat_is_created;

    /** Optional. A JSON-serialized object listing the required administrator rights of the user in the chat. */
    struct telebot_chat_administrator_rights *user_administrator_rights;

    /** Optional. A JSON-serialized object listing the required administrator rights of the bot in the chat. */
    struct telebot_chat_administrator_rights *bot_administrator_rights;

    /** Optional. Pass True to request a chat with the bot as a member. */
    bool bot_is_member;

    /** Optional. Pass True to request the chat's title */
    bool request_title;

    /** Optional. Pass True to request the chat's username */
    bool request_username;

    /** Optional. Pass True to request the chat's photo */
    bool request_photo;
} telebot_keyboard_button_request_chat_t;

/**
 * @brief Describes a Web App.
 */
typedef struct telebot_web_app_info {
    /** An HTTPS URL of a Web App to be opened */
    char *url;
} telebot_web_app_info_t;

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
     * Optional. If specified, pressing the button will open a list of suitable
     * users. Identifiers of selected users will be sent to the bot in a
     * "users_shared" service message. Available in private chats only.
     */
    struct telebot_keyboard_button_request_users *request_users;

    /**
     * Optional. If specified, pressing the button will open a list of suitable
     * chats. Tapping on a chat will send its identifier to the bot in a
     * "chat_shared" service message. Available in private chats only.
     */
    struct telebot_keyboard_button_request_chat *request_chat;

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

    /**
     * Optional. If specified, the described Web App will be launched when the
     * button is pressed. The Web App will be able to send a "web_app_data"
     * service message. Available in private chats only.
     */
    struct telebot_web_app_info *web_app;
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
     * Optional. Requests clients to always show the keyboard when the regular
     * keyboard is hidden. Defaults to false.
     */
    bool is_persistent;

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
 * @brief This object represents an incoming inline query. When the user sends
 * an empty query, your bot could return some default or trending results.
 */
typedef struct telebot_inline_query {
    /** Unique identifier for this query */
    char *id;

    /** Sender */
    struct telebot_user *from;

    /** Text of the query (up to 256 characters) */
    char *query;

    /** Offset of the results to be returned, can be controlled by the bot */
    char *offset;

    /** Optional. Type of the chat from which the inline query was sent */
    char *chat_type;

    /** Optional. Sender location, only for bots that request user location */
    struct telebot_location *location;
} telebot_inline_query_t;

/**
 * @brief Represents a result of an inline query that was chosen by the user
 * and sent to their chat partner.
 */
typedef struct telebot_chosen_inline_result {
    /** The unique identifier for the result that was chosen */
    char *result_id;

    /** The user who chose the result */
    struct telebot_user *from;

    /** Optional. Sender location, only for bots that request user location */
    struct telebot_location *location;

    /**
     * Optional. Identifier of the sent inline message. Available only if there
     * is an inline keyboard attached to the message. Will be also received in
     * callback queries and can be used to edit the message.
     */
    char *inline_message_id;

    /** The query that was used to obtain the result */
    char *query;
} telebot_chosen_inline_result_t;

/**
 * @brief This object contains information about an incoming shipping query.
 */
typedef struct telebot_shipping_query {
    /** Unique query identifier */
    char *id;

    /** User who sent the query */
    struct telebot_user *from;

    /** Bot specified invoice payload */
    char *invoice_payload;

    /** User specified shipping address */
    struct telebot_shipping_address *shipping_address;
} telebot_shipping_query_t;

/**
 * @brief This object contains information about an incoming pre-checkout query.
 */
typedef struct telebot_pre_checkout_query {
    /** Unique query identifier */
    char *id;

    /** User who sent the query */
    struct telebot_user *from;

    /** Three-letter ISO 4217 currency code */
    char *currency;

    /**
     * Total amount in the smallest units of the currency (integer, not
     * float/double).
     */
    int total_amount;

    /** Bot specified invoice payload */
    char *invoice_payload;

    /** Optional. Identifier of the shipping option chosen by the user */
    char *shipping_option_id;

    /** Optional. Order info provided by the user */
    struct telebot_order_info *order_info;
} telebot_pre_checkout_query_t;

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
    /** Identifier for this file, which can be used to download or reuse the file */
    char *file_id;

    /**
     * Unique identifier for this file, which is supposed to be the same over time
     * and for different bots. Can't be used to download or reuse the file.
     */
    char *file_unique_id;

    /** Sticker width */
    int width;

    /** Sticker height */
    int height;

    /** True, if the sticker is animated */
    bool is_animated;

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
 * @brief This object represents a forum topic.
 */
typedef struct telebot_forum_topic {
    /** Unique identifier of the forum topic */
    int message_thread_id;

    /** Name of the topic */
    char *name;

    /** Color of the topic icon in RGB format */
    int icon_color;

    /** Optional. Unique identifier of the custom emoji used as the topic icon */
    char *icon_custom_emoji_id;
} telebot_forum_topic_t;

/**
 * @brief This object represents a prepared inline message.
 */
typedef struct telebot_prepared_inline_message {
    /** Unique identifier of the prepared message */
    char *id;

    /** Expiration date of the prepared message */
    long expiration_date;
} telebot_prepared_inline_message_t;

/**
 * @brief This object represents a message sent via a Web App.
 */
typedef struct telebot_sent_web_app_message {
    /** Optional. Identifier of the sent inline message. */
    char *inline_message_id;
} telebot_sent_web_app_message_t;

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

        /** The bot was connected to or disconnected from a business account */
        telebot_business_connection_t business_connection;

        /** New message from a connected business account */
        telebot_message_t business_message;

        /** New version of a message from a connected business account */
        telebot_message_t edited_business_message;

        /** Messages were deleted from a connected business account */
        telebot_business_messages_deleted_t deleted_business_messages;

        /** New incoming inline query */
        telebot_inline_query_t inline_query;

        /**
         * The result of an inline query that was chosen by a user and sent to
         * their chat partner. Please see our documentation on the feedback collecting
         * for details on how to enable these updates for your bot.
         */
        telebot_chosen_inline_result_t chosen_inline_result;

        /** New incoming callback query */
        telebot_callback_query_t callback_query;

        /** New incoming shipping query. Only for invoices with flexible price */
        telebot_shipping_query_t shipping_query;

        /** New incoming pre-checkout query. Contains full information about checkout */
        telebot_pre_checkout_query_t pre_checkout_query;

        /** A user purchased paid media */
        telebot_paid_media_purchased_t purchased_paid_media;

        /**
         * New poll state. Bots receive only updates about stopped polls and
         * polls, which are sent by the bot
         */
        telebot_poll_t poll;

        /**
         * A user changed their answer in a non-anonymous poll. Bots receive
         * new votes only in polls that were sent by the bot itself.
         */
        telebot_poll_answer_t poll_answer;

        /** The bot's chat member status was updated in a chat. For private chats, this update is received only when the bot is blocked or unblocked by the user. */
        telebot_chat_member_updated_t my_chat_member;

        /** A chat member's status was updated in a chat. The bot must be an administrator in the chat and must explicitly specify "chat_member" in the list of allowed_updates to receive these updates. */
        telebot_chat_member_updated_t chat_member;

        /** A request to join the chat has been sent. The bot must have the can_invite_users administrator right in the chat to receive these updates. */
        telebot_chat_join_request_t chat_join_request;

        /** A reaction to a message was changed by a user. The bot must be an administrator in the chat and must explicitly specify "message_reaction" in the list of allowed_updates to receive these updates. */
        telebot_message_reaction_updated_t message_reaction;

        /** Reactions to a message with anonymous reactions were changed. The bot must be an administrator in the chat and must explicitly specify "message_reaction_count" in the list of allowed_updates to receive these updates. */
        telebot_message_reaction_count_updated_t message_reaction_count;

        /** A chat boost was added or changed. The bot must be an administrator in the chat to receive these updates. */
        telebot_chat_boost_updated_t chat_boost;

        /** A boost was removed from a chat. The bot must be an administrator in the chat to receive these updates. */
        telebot_chat_boost_removed_t chat_boost_removed;
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
 * @brief Thi object represetns information about the current status of a webhook.
 */
typedef struct telebot_chat_location {
    /** The location to which the supergroup is connected. Can't be a live location.*/
    struct telebot_location *location;

    /** Location address; 1-64 characters, as defined by the chat owner */
    char *address;
} telebot_chat_location_t;

/**
 * @brief This object represents a service message about a change in
 * auto-delete timer settings.
 */
typedef struct telebot_message_auto_delete_timer_changed {
    /** New auto-delete time for messages in the chat; in seconds */
    int message_auto_delete_time;
} telebot_message_auto_delete_timer_changed_t;

/**
 * @brief This is opaque object to represent a telebot handler.
 */
typedef struct telebot_handler *telebot_handler_t;

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_TYPES_H__ */
