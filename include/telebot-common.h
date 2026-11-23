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

#ifndef __TELEBOT_COMMON_H__
#define __TELEBOT_COMMON_H__

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        telebot-common.h
 * @ingroup     TELEBOT_COMMON_API
 * @brief       This file contains definitions shared by the simple and core APIs
 * @author      Elmurod Talipov
 * @date        2020-04-19
 */

/**
 * @defgroup TELEBOT_COMMON_API Common Definitions
 * @brief Error codes and request parameters shared by the simple and core APIs.
 *
 * @addtogroup TELEBOT_COMMON_API
 * @{
 */

/**
 * @brief Marks a function that will be removed in the next major version.
 *
 * Calls compile as before but produce a compiler warning that names the replacement.
 */
#if defined(__GNUC__) || defined(__clang__)
#define TELEBOT_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
#define TELEBOT_DEPRECATED(msg) __declspec(deprecated(msg))
#else
#define TELEBOT_DEPRECATED(msg)
#endif

/**
 * @brief Enumerations of error code for telebot programming interface
 */
typedef enum {
    TELEBOT_ERROR_NONE              = 0,    /**< Successful */
    TELEBOT_ERROR_OPERATION_FAILED  = -1,   /**< Operation failed */
    TELEBOT_ERROR_NOT_SUPPORTED     = -2,   /**< Not supported */
    TELEBOT_ERROR_OUT_OF_MEMORY     = -3,   /**< Out of memory */
    TELEBOT_ERROR_NO_CONNECTION     = -4,   /**< No Internet connection */
    TELEBOT_ERROR_INVALID_PARAMETER = -5,   /**< Invalid parameter */
    TELEBOT_ERROR_RATE_LIMITED      = -6,   /**< Flood control (HTTP 429): retry after #telebot_get_retry_after() seconds */
} telebot_error_e;

/*
 * Request parameters shared by the core and simple APIs
 */

/**
 * @brief Options shared by every method that sends a message.
 *
 * Pass NULL wherever a function takes `const telebot_send_options_t *` to use
 * Telegram's defaults. Zero-initialise the struct and set only what you need:
 * @code
 * telebot_send_options_t opts = {0};
 * opts.reply_to_message_id = msg->message_id;
 * opts.reply_markup = "{\"inline_keyboard\":[[{\"text\":\"OK\",\"callback_data\":\"ok\"}]]}";
 * telebot_send_message(bot, chat_id, "Done", NULL, false, &opts);
 * @endcode
 */
typedef struct telebot_send_options {
    int message_thread_id;              /**< Forum topic, or topic in a private chat; 0 for none */
    int reply_to_message_id;            /**< Message to reply to; 0 when not a reply */
    bool allow_sending_without_reply;   /**< Send even if the replied-to message was deleted */
    bool disable_notification;          /**< Deliver silently */
    bool protect_content;               /**< Prevent forwarding and saving */
    const char *reply_markup;           /**< JSON inline keyboard, custom keyboard, or NULL */
    const char *business_connection_id; /**< Send on behalf of a business account, or NULL */
    const char *message_effect_id;      /**< Message effect for private chats, or NULL */
} telebot_send_options_t;

/**
 * @brief Where the bytes of an uploaded file come from.
 */
typedef enum telebot_input_file_kind {
    TELEBOT_INPUT_FILE_REMOTE = 0, /**< file_id already on Telegram's servers, or an HTTP URL */
    TELEBOT_INPUT_FILE_PATH,       /**< Local file, streamed from disk (or SPIFFS/LittleFS/SD on ESP32) */
    TELEBOT_INPUT_FILE_MEMORY,     /**< Bytes in RAM, e.g. a camera frame; never copied */
} telebot_input_file_kind_e;

/**
 * @brief A file to send (Bot API "InputFile or String").
 *
 * Build one with telebot_input_file_remote(), telebot_input_file_path() or
 * telebot_input_file_memory(), or in C with the TELEBOT_FILE_REMOTE(),
 * TELEBOT_FILE_PATH() and TELEBOT_FILE_MEMORY() macros. All pointers are
 * borrowed: they must stay valid until the call that uses the file returns.
 */
typedef struct telebot_input_file {
    telebot_input_file_kind_e kind; /**< Where the bytes of the file come from */
    const char *ref;      /**< REMOTE: file_id or URL; PATH: local file path */
    const void *data;     /**< MEMORY: first byte */
    size_t size;          /**< MEMORY: number of bytes */
    const char *filename; /**< MEMORY: file name shown to the recipient (e.g. "photo.jpg") */
} telebot_input_file_t;

#ifndef __cplusplus
/**
 * @brief Pointer to a file_id/URL descriptor, for one-line calls in C, e.g.
 * telebot_send_sticker(bot, chat_id, TELEBOT_FILE_REMOTE("CAACAgIAAxkB..."), NULL, NULL).
 * The pointer is valid until the end of the enclosing block.
 */
#define TELEBOT_FILE_REMOTE(file_id_or_url) \
    (&(telebot_input_file_t){TELEBOT_INPUT_FILE_REMOTE, (file_id_or_url), NULL, 0, NULL})
/** @brief Pointer to a local-file descriptor; see TELEBOT_FILE_REMOTE(). */
#define TELEBOT_FILE_PATH(path) \
    (&(telebot_input_file_t){TELEBOT_INPUT_FILE_PATH, (path), NULL, 0, NULL})
/** @brief Pointer to an in-memory descriptor; see TELEBOT_FILE_REMOTE(). */
#define TELEBOT_FILE_MEMORY(data, size, filename) \
    (&(telebot_input_file_t){TELEBOT_INPUT_FILE_MEMORY, NULL, (data), (size), (filename)})
#endif

/**
 * @brief A file_id or URL of a file Telegram can fetch itself.
 * @param[in] file_id_or_url file_id of a file already on Telegram's servers, or an HTTP(S) URL.
 * @return The file descriptor.
 */
static inline telebot_input_file_t telebot_input_file_remote(const char *file_id_or_url)
{
    telebot_input_file_t f = {TELEBOT_INPUT_FILE_REMOTE, file_id_or_url, NULL, 0, NULL};
    return f;
}

/**
 * @brief A local file, uploaded in chunks without loading it into memory.
 * @param[in] path Path of the file; it must stay valid until the call using it returns.
 * @return The file descriptor.
 */
static inline telebot_input_file_t telebot_input_file_path(const char *path)
{
    telebot_input_file_t f = {TELEBOT_INPUT_FILE_PATH, path, NULL, 0, NULL};
    return f;
}

/**
 * @brief Bytes already in RAM; they are not copied.
 * @param[in] data First byte; it must stay valid until the call using it returns.
 * @param[in] size Number of bytes.
 * @param[in] filename File name shown to the recipient, with the right extension (e.g. "cam.jpg").
 * @return The file descriptor.
 */
static inline telebot_input_file_t telebot_input_file_memory(const void *data, size_t size, const char *filename)
{
    telebot_input_file_t f = {TELEBOT_INPUT_FILE_MEMORY, NULL, data, size, filename};
    return f;
}

/**
 * @brief Kind of an item in a media group.
 */
typedef enum telebot_input_media_type {
    TELEBOT_INPUT_MEDIA_PHOTO = 0, /**< Photo; may be mixed with videos */
    TELEBOT_INPUT_MEDIA_VIDEO,     /**< Video; may be mixed with photos */
    TELEBOT_INPUT_MEDIA_AUDIO,     /**< Audio; a group must contain only audios */
    TELEBOT_INPUT_MEDIA_DOCUMENT,  /**< Document; a group must contain only documents */
} telebot_input_media_type_e;

/**
 * @brief One item of an album sent with #telebot_send_media_group().
 */
typedef struct telebot_input_media {
    telebot_input_media_type_e type; /**< What the item is */
    telebot_input_file_t file;       /**< Where its bytes come from */
    const char *caption;             /**< Optional caption, or NULL */
    const char *parse_mode;          /**< Optional caption parse mode, or NULL */
} telebot_input_media_t;

/**
 * @brief One parameter of a Bot API call made with #telebot_call_method() or
 *     #telebot_core_call_method().
 *
 * Set either @p value (numbers, booleans and JSON objects are passed as text,
 * e.g. "123", "true", "{\"type\":\"emoji\"}") or @p file.
 */
typedef struct telebot_param {
    const char *name;                 /**< Parameter name as in the Bot API docs */
    const char *value;                /**< Text value, or NULL when @p file is set */
    const telebot_input_file_t *file; /**< File to send, or NULL when @p value is set */
} telebot_param_t;

/**
 * @brief Description of an invoice, shared by #telebot_send_invoice() and
 * #telebot_create_invoice_link().
 *
 * Zero-initialise it and set what you need; title, description, payload,
 * currency and prices are required. For payments in Telegram Stars use
 * currency "XTR" and leave provider_token NULL.
 */
typedef struct telebot_invoice_params {
    const char *title;                  /**< Product name, 1-32 characters */
    const char *description;            /**< Product description, 1-255 characters */
    const char *payload;                /**< Bot-defined payload, 1-128 bytes, not shown to the user */
    const char *provider_token;         /**< Payment provider token; NULL for Telegram Stars */
    const char *currency;               /**< Three-letter ISO 4217 code, or "XTR" for Telegram Stars */
    const char *prices;                 /**< JSON array of LabeledPrice, e.g. [{"label":"Cup","amount":50}] */
    int max_tip_amount;                 /**< Maximum tip in the smallest currency units; 0 for no tips */
    const char *suggested_tip_amounts;  /**< JSON array of up to 4 suggested tips, or NULL */
    const char *start_parameter;        /**< sendInvoice only: deep-linking parameter, or NULL */
    const char *provider_data;          /**< JSON data for the payment provider, or NULL */
    const char *photo_url;              /**< URL of a product photo, or NULL */
    int photo_size;                     /**< Photo size in bytes, or 0 */
    int photo_width;                    /**< Photo width, or 0 */
    int photo_height;                   /**< Photo height, or 0 */
    bool need_name;                     /**< Require the user's full name */
    bool need_phone_number;             /**< Require the user's phone number */
    bool need_email;                    /**< Require the user's email address */
    bool need_shipping_address;         /**< Require the user's shipping address */
    bool send_phone_number_to_provider; /**< Pass the phone number to the provider */
    bool send_email_to_provider;        /**< Pass the email address to the provider */
    bool is_flexible;                   /**< The final price depends on the shipping method */
} telebot_invoice_params_t;

/**
 * @brief Actions a non-administrator user is allowed to take in a chat (Bot API ChatPermissions).
 *
 * Used when reading a chat and when restricting members. A field left false
 * denies the permission.
 */
typedef struct telebot_chat_permissions {
    bool can_send_messages;          /**< Send text messages, contacts, giveaways, invoices, locations and venues */
    bool can_send_audios;            /**< Send audios */
    bool can_send_documents;         /**< Send documents */
    bool can_send_photos;            /**< Send photos */
    bool can_send_videos;            /**< Send videos */
    bool can_send_video_notes;       /**< Send video notes */
    bool can_send_voice_notes;       /**< Send voice notes */
    bool can_send_polls;             /**< Send polls and checklists */
    bool can_send_other_messages;    /**< Send animations, games, stickers and use inline bots */
    bool can_add_web_page_previews;  /**< Add web page previews to messages */
    bool can_react_to_messages;      /**< React to messages (Bot API 10.0) */
    bool can_edit_tag;               /**< Edit their own member tag (Bot API 9.5) */
    bool can_change_info;            /**< Change the chat title, photo and other settings */
    bool can_invite_users;           /**< Invite new users to the chat */
    bool can_pin_messages;           /**< Pin messages */
    bool can_manage_topics;          /**< Create forum topics */
} telebot_chat_permissions_t;

/**
 * @brief Rights of an administrator in a chat (Bot API ChatAdministratorRights).
 *
 * Used to promote members. A field left false does not grant the right.
 */
typedef struct telebot_chat_administrator_rights {
    bool is_anonymous;               /**< The administrator's presence in the chat is hidden */
    bool can_manage_chat;            /**< Access the event log, boost list, member list and more */
    bool can_delete_messages;        /**< Delete messages of other users */
    bool can_manage_video_chats;     /**< Manage video chats */
    bool can_restrict_members;       /**< Restrict, ban or unban members, or access statistics */
    bool can_promote_members;        /**< Add new administrators with a subset of their own rights */
    bool can_change_info;            /**< Change the chat title, photo and other settings */
    bool can_invite_users;           /**< Invite new users to the chat */
    bool can_post_stories;           /**< Post stories to the chat */
    bool can_edit_stories;           /**< Edit stories posted by other users */
    bool can_delete_stories;         /**< Delete stories posted by other users */
    bool can_post_messages;          /**< Channels only: post messages */
    bool can_edit_messages;          /**< Channels only: edit messages of other users and pin messages */
    bool can_pin_messages;           /**< Groups and supergroups only: pin messages */
    bool can_manage_topics;          /**< Supergroups only: manage forum topics */
    bool can_manage_direct_messages; /**< Channels only: manage direct messages (Bot API 9.2) */
    bool can_manage_tags;            /**< Edit member tags (Bot API 9.5) */
    bool can_send_welcome_messages;  /**< Send welcome messages (Bot API 10.3) */
} telebot_chat_administrator_rights_t;

/**
 * @} // end of TELEBOT_COMMON_API
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_COMMON_H__ */

