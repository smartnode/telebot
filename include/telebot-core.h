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

#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include "telebot-common.h"
#include "telebot-types.h"
#include "telebot-payments.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @file        telebot-core.h
 * @ingroup     TELEBOT_CORE_API
 * @brief       This file contains core API for the telegram bot interface
 * @author      Elmurod Talipov
 * @date        2026-02-28
 */

/**
 * @defgroup TELEBOT_CORE_API Core Telegram Bot API
 * @brief The APIs for the telegram bot interface, response is unparsed json sting.
 *
 *
 * @addtogroup TELEBOT_CORE_API
 * @{
 */

/**
 * @brief Telebot core handler opaque type.
 */
typedef struct telebot_core_handler *telebot_core_handler_t;

/**
 * @brief Telebot core response opaque type.
 */
typedef struct telebot_core_response *telebot_core_response_t;

/**
 * @brief Get response error code.
 * @param[in] response Response to get its error code.
 * @return on For successful responses TELEBOT_ERROR_NONE is returned,
 * otherwise a negative error value @see #telebot_error_e
 */
telebot_error_e telebot_core_get_response_code(telebot_core_response_t response);

/**
 * @brief Seconds Telegram asks to wait before repeating a rate-limited request.
 *
 * Non-zero only when the response code is #TELEBOT_ERROR_RATE_LIMITED.
 *
 * @param[in] response Response returned by a core function.
 * @return Seconds to wait, or 0.
 */
int telebot_core_get_response_retry_after(telebot_core_response_t response);

/**
 * @brief New identifier of a group that was upgraded to a supergroup.
 *
 * Non-zero when a request failed because the group was migrated; repeat the
 * request with this chat identifier.
 *
 * @param[in] response Response returned by a core function.
 * @return The supergroup identifier, or 0.
 */
long long int telebot_core_get_response_migrate_to_chat_id(telebot_core_response_t response);

/**
 * @brief Seconds to wait after the most recent request made with a core handler.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @return The retry_after value of the last request, or 0 if it was not rate-limited.
 */
int telebot_core_get_retry_after(telebot_core_handler_t core_h);

/**
 * @brief Get response data.
 * @param[in] response Response to get its data.
 * @return on For successful responses null terminated string is returned,
 * otherwise NULL.
 */
const char *telebot_core_get_response_data(telebot_core_response_t response);

/**
 * @brief Release response data obtained with telebot core methods.
 * @param[in] response Response to release.
 */
void telebot_core_put_response(telebot_core_response_t response);

/**
 * @brief Routes the memory used for JSON parsing through custom allocators.
 *
 * Parsing a reply builds a temporary JSON tree whose size depends on the
 * reply. On boards with external RAM this keeps it out of internal memory:
 * @code
 * static void *psram_malloc(size_t size) { return heap_caps_malloc(size, MALLOC_CAP_SPIRAM); }
 * telebot_set_json_allocator(psram_malloc, heap_caps_free);
 * @endcode
 * The setting is global. Call it before creating any handler; pass NULL for
 * both to restore malloc() and free().
 *
 * @param[in] malloc_fn Allocation function, or NULL for malloc().
 * @param[in] free_fn Matching release function, or NULL for free().
 */
void telebot_set_json_allocator(void *(*malloc_fn)(size_t size), void (*free_fn)(void *ptr));

/**
 * @brief Start function to use telebot core APIs.
 *
 * This function must be used first to call, and it creates handler that is used
 * as input to other functions in telebot core interface. This call MUST have
 * corresponding call to #telebot_core_destroy when operation is complete.
 *
 * @param[out] core_h Receives the new core handler; release it with #telebot_core_destroy().
 * @param[in] token Bot token from \@BotFather, e.g. "123456:ABC-DEF..."; it is copied.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_error_e telebot_core_create(telebot_core_handler_t *core_h, const char *token);

/**
 * @brief Final function to use telebot core APIs
 *
 * This function must be the last function to call for a telebot core use.
 * It is the opposite of the telebot_core_create function and MUST be called
 * with the same handler as the input that a telebot_core_create call
 * created.
 * @param[in,out] core_h The A pointer to telebot core handler created with #telebot_core_create().
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_destroy(telebot_core_handler_t *core_h);

/**
 * @brief Set proxy address to use telebot behind proxy
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] addr Proxy address in full.
 * @param[in] auth Proxy authorization informatio.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_set_proxy(telebot_core_handler_t core_h, const char *addr,
        const char *auth);

/**
 * @brief Get currently used proxy address
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[out] addr Current proxy address or NULL, MUST be freed after use.
 * @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative error value.
 */
telebot_error_e telebot_core_get_proxy(telebot_core_handler_t core_h, char **addr);

/**
 * @brief Receive incoming updates (long polling). It will not work if an outgoing
 * webhook is set up. In order to avoid getting duplicate updates, recalculate
 * offset after each server response.
 * @param[in] core_h Telebot core handler creating using telebot_core_create().
 * @param[in] offset Identifier of the first update to be returned. Must be greater
 * by one than the highest among the identifiers of previously received updates.
 * By default, updates starting with the earliest unconfirmed update are returned.
 * An update is considered confirmed as soon as the function is called with an
 * offset higher than its update_id.
 * @param[in] limit Limits the number of updates to be retrieved. Values between
 * 1—100 are accepted. Defaults to 100.
 * @param[in] timeout Timeout in seconds for long polling. Defaults to 0, i.e. usual
 * short polling.
 * @param[in] allowed_updates List the types of update you want your bot to receive.
 * Specify an empty list to receive all updates regardless of type (default).
 * @return #telebot_core_response_t response that contains updates,
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_get_updates(telebot_core_handler_t core_h,
        int offset, int limit, int timeout, const char *allowed_updates);

/**
 * @brief Sets a URL that receives updates through an HTTPS POST request.
 *
 * While a webhook is set, #telebot_get_updates() does not work. Use @p secret_token and check the
 *     X-Telegram-Bot-Api-Secret-Token header to make sure a request really comes from Telegram.
 *
 * Bot API method: setWebhook.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] url HTTPS URL that receives the updates; an empty string removes the webhook.
 * @param[in] certificate Optional public key of a self-signed certificate, uploaded from a path or
 *     memory, or NULL.
 * @param[in] max_connections Maximum simultaneous HTTPS connections, 1-100; 0 for the default of
 *     40.
 * @param[in] allowed_updates Optional JSON array of update type names, or NULL for all.
 * @param[in] drop_pending_updates Drop all updates waiting to be delivered.
 * @param[in] secret_token Optional secret, 1-256 characters of A-Z, a-z, 0-9, _ and -, sent back in
 *     every request; or NULL.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_set_webhook(telebot_core_handler_t core_h, const char *url,
        const telebot_input_file_t *certificate, int max_connections, const char *allowed_updates,
        bool drop_pending_updates, const char *secret_token);

/**
 * @brief Remove webhook integration if you decide to switch back to getUpdates.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @return #telebot_core_response_t response that contains the result (true/false),
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_delete_webhook(telebot_core_handler_t core_h);

/**
 * @brief Get current webhook status.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @return #telebot_core_response_t response that contains webhook information,
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_get_webhook_info(telebot_core_handler_t core_h);

/**
 * @brief Get basic information about the bot.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_get_me(telebot_core_handler_t core_h);

/**
 * @brief Sends a text message.
 *
 * Bot API method: sendMessage.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] text Text of the message, 1-4096 characters after entity parsing.
 * @param[in] parse_mode Optional formatting: "MarkdownV2", "HTML" or "Markdown"; NULL for plain
 *     text.
 * @param[in] disable_link_preview Do not show a preview for links in the text.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_message(telebot_core_handler_t core_h, long long int chat_id,
        const char *text, const char *parse_mode, bool disable_link_preview,
        const telebot_send_options_t *options);

/**
 * @brief Forwards a message of any kind.
 *
 * Only message_thread_id, disable_notification, protect_content and message_effect_id are taken
 *     from @p options.
 *
 * Bot API method: forwardMessage.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] from_chat_id Chat where the original message was sent.
 * @param[in] message_id Identifier of the message in @p from_chat_id.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_forward_message(telebot_core_handler_t core_h, long long int chat_id,
        long long int from_chat_id, int message_id, const telebot_send_options_t *options);

/**
 * @brief Sends a photo.
 *
 * Upload limit is 10 MB; width plus height must not exceed 10000 and the aspect ratio must be at
 *     most 20.
 *
 * Bot API method: sendPhoto.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] photo Photo to send: a file_id or HTTP URL, a local file, or bytes in memory; see
 *     #telebot_input_file_t.
 * @param[in] caption Optional caption, 0-1024 characters after entity parsing, or NULL.
 * @param[in] parse_mode Optional formatting of the caption: "MarkdownV2", "HTML" or "Markdown";
 *     NULL for plain text.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_photo(telebot_core_handler_t core_h, long long int chat_id,
        const telebot_input_file_t *photo, const char *caption, const char *parse_mode,
        const telebot_send_options_t *options);

/**
 * @brief Sends an audio file to be shown in the music player.
 *
 * Use MP3 or M4A, up to 50 MB. For voice messages use #telebot_send_voice().
 *
 * Bot API method: sendAudio.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] audio Audio file to send: a file_id or HTTP URL, a local file, or bytes in memory; see
 *     #telebot_input_file_t.
 * @param[in] caption Optional caption, 0-1024 characters after entity parsing, or NULL.
 * @param[in] parse_mode Optional formatting of the caption: "MarkdownV2", "HTML" or "Markdown";
 *     NULL for plain text.
 * @param[in] duration Duration in seconds, or 0 if unknown.
 * @param[in] performer Optional performer, or NULL.
 * @param[in] title Optional track name, or NULL.
 * @param[in] thumbnail Optional thumbnail (JPEG, under 200 kB, at most 320x320), uploaded from a
 *     path or memory, or NULL. A file_id or URL is rejected because Telegram never reuses
 *     thumbnails.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_audio(telebot_core_handler_t core_h, long long int chat_id,
        const telebot_input_file_t *audio, const char *caption, const char *parse_mode, int duration,
        const char *performer, const char *title, const telebot_input_file_t *thumbnail,
        const telebot_send_options_t *options);

/**
 * @brief Sends a general file.
 *
 * Files of any type, up to 50 MB.
 *
 * Bot API method: sendDocument.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] document File to send: a file_id or HTTP URL, a local file, or bytes in memory; see
 *     #telebot_input_file_t.
 * @param[in] thumbnail Optional thumbnail (JPEG, under 200 kB, at most 320x320), uploaded from a
 *     path or memory, or NULL. A file_id or URL is rejected because Telegram never reuses
 *     thumbnails.
 * @param[in] caption Optional caption, 0-1024 characters after entity parsing, or NULL.
 * @param[in] parse_mode Optional formatting of the caption: "MarkdownV2", "HTML" or "Markdown";
 *     NULL for plain text.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_document(telebot_core_handler_t core_h, long long int chat_id,
        const telebot_input_file_t *document, const telebot_input_file_t *thumbnail, const char *caption,
        const char *parse_mode, const telebot_send_options_t *options);

/**
 * @brief Sends an MPEG4 video.
 *
 * Bot API method: sendVideo.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] video Video to send: a file_id or HTTP URL, a local file, or bytes in memory; see
 *     #telebot_input_file_t.
 * @param[in] duration Duration in seconds, or 0 if unknown.
 * @param[in] width Width in pixels, or 0 if unknown.
 * @param[in] height Height in pixels, or 0 if unknown.
 * @param[in] thumbnail Optional thumbnail (JPEG, under 200 kB, at most 320x320), uploaded from a
 *     path or memory, or NULL. A file_id or URL is rejected because Telegram never reuses
 *     thumbnails.
 * @param[in] caption Optional caption, 0-1024 characters after entity parsing, or NULL.
 * @param[in] parse_mode Optional formatting of the caption: "MarkdownV2", "HTML" or "Markdown";
 *     NULL for plain text.
 * @param[in] supports_streaming The video is suitable for streaming.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_video(telebot_core_handler_t core_h, long long int chat_id,
        const telebot_input_file_t *video, int duration, int width, int height,
        const telebot_input_file_t *thumbnail, const char *caption, const char *parse_mode,
        bool supports_streaming, const telebot_send_options_t *options);

/**
 * @brief Sends a GIF or an H.264/MPEG-4 AVC video without sound.
 *
 * Bot API method: sendAnimation.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] animation Animation to send: a file_id or HTTP URL, a local file, or bytes in memory;
 *     see #telebot_input_file_t.
 * @param[in] duration Duration in seconds, or 0 if unknown.
 * @param[in] width Width in pixels, or 0 if unknown.
 * @param[in] height Height in pixels, or 0 if unknown.
 * @param[in] thumbnail Optional thumbnail (JPEG, under 200 kB, at most 320x320), uploaded from a
 *     path or memory, or NULL. A file_id or URL is rejected because Telegram never reuses
 *     thumbnails.
 * @param[in] caption Optional caption, 0-1024 characters after entity parsing, or NULL.
 * @param[in] parse_mode Optional formatting of the caption: "MarkdownV2", "HTML" or "Markdown";
 *     NULL for plain text.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_animation(telebot_core_handler_t core_h, long long int chat_id,
        const telebot_input_file_t *animation, int duration, int width, int height,
        const telebot_input_file_t *thumbnail, const char *caption, const char *parse_mode,
        const telebot_send_options_t *options);

/**
 * @brief Sends a voice message.
 *
 * Use OGG encoded with OPUS, MP3 or M4A, up to 50 MB.
 *
 * Bot API method: sendVoice.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] voice Audio file to send: a file_id or HTTP URL, a local file, or bytes in memory; see
 *     #telebot_input_file_t.
 * @param[in] caption Optional caption, 0-1024 characters after entity parsing, or NULL.
 * @param[in] parse_mode Optional formatting of the caption: "MarkdownV2", "HTML" or "Markdown";
 *     NULL for plain text.
 * @param[in] duration Duration in seconds, or 0 if unknown.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_voice(telebot_core_handler_t core_h, long long int chat_id,
        const telebot_input_file_t *voice, const char *caption, const char *parse_mode, int duration,
        const telebot_send_options_t *options);

/**
 * @brief Sends a rounded square video message of up to 1 minute.
 *
 * Sending video notes by URL is not supported by Telegram.
 *
 * Bot API method: sendVideoNote.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] video_note Video note to send: a file_id or HTTP URL, a local file, or bytes in
 *     memory; see #telebot_input_file_t.
 * @param[in] duration Duration in seconds, or 0 if unknown.
 * @param[in] length Video width and height (the diameter), or 0 if unknown.
 * @param[in] thumbnail Optional thumbnail (JPEG, under 200 kB, at most 320x320), uploaded from a
 *     path or memory, or NULL. A file_id or URL is rejected because Telegram never reuses
 *     thumbnails.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_video_note(telebot_core_handler_t core_h, long long int chat_id,
        const telebot_input_file_t *video_note, int duration, int length,
        const telebot_input_file_t *thumbnail, const telebot_send_options_t *options);

/**
 * @brief Sends 2-10 photos, videos, documents or audios as an album.
 *
 * Documents and audios can only be grouped with items of the same type. reply_markup in @p options
 *     is ignored, because albums cannot carry a keyboard.
 *
 * Bot API method: sendMediaGroup.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] media Album items, see #telebot_input_media_t.
 * @param[in] count Number of items in @p media, 2-10.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_media_group(telebot_core_handler_t core_h, long long int chat_id,
        const telebot_input_media_t media[], int count, const telebot_send_options_t *options);

/**
 * @brief Sends a point on the map.
 *
 * Bot API method: sendLocation.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] latitude Latitude of the location.
 * @param[in] longitude Longitude of the location.
 * @param[in] live_period Seconds the live location updates (60-86400), or 0 for a static location.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_location(telebot_core_handler_t core_h, long long int chat_id,
        double latitude, double longitude, int live_period, const telebot_send_options_t *options);

/**
 * @brief Edit live location messages sent by the bot or via the bot (for inline
 * bots). A location can be edited until its live_period expires or editing is
 * explicitly disabled by a call to #telebot_core_stop_message_live_location().
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] message_id Required if inline_message_id is not specified.
 * Identifier of the sent message.
 * @param[in] inline_message_id Required if chat_id and message_id are not
 * specified. Identifier of the inline message.
 * @param[in] latitude Latitude of location.
 * @param[in] longitude Longitude of location.
 * @param[in] reply_markup A JSON-serialized object for a new inline keyboard.
 * @return #telebot_core_response_t response that contains the result (true/false),
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_edit_message_live_location(telebot_core_handler_t core_h,
        long long int chat_id, int message_id, const char *inline_message_id,
        float latitude, float longitude, const char *reply_markup);

/**
 * @brief Stop updating a live location message sent by the bot or via the bot
 * (for inline bots) before live_period expires.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] message_id Required if inline_message_id is not specified.
 * Identifier of the sent message.
 * @param[in] inline_message_id Required if chat_id and message_id are not
 * specified. Identifier of the inline message.
 * @param[in] reply_markup A JSON-serialized object for a new inline keyboard.
 * @return #telebot_core_response_t response that contains the result (true/false),
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_stop_message_live_location(telebot_core_handler_t core_h,
        long long int chat_id, int message_id, char *inline_message_id,
        const char *reply_markup);

/**
 * @brief Sends information about a venue.
 *
 * Bot API method: sendVenue.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] latitude Latitude of the venue.
 * @param[in] longitude Longitude of the venue.
 * @param[in] title Name of the venue.
 * @param[in] address Address of the venue.
 * @param[in] foursquare_id Optional Foursquare identifier, or NULL.
 * @param[in] foursquare_type Optional Foursquare type, e.g. "food/icecream", or NULL.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_venue(telebot_core_handler_t core_h, long long int chat_id,
        double latitude, double longitude, const char *title, const char *address, const char *foursquare_id,
        const char *foursquare_type, const telebot_send_options_t *options);

/**
 * @brief Sends a phone contact.
 *
 * Bot API method: sendContact.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] phone_number Contact phone number.
 * @param[in] first_name Contact first name.
 * @param[in] last_name Optional contact last name, or NULL.
 * @param[in] vcard Optional vCard data, 0-2048 bytes, or NULL.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_contact(telebot_core_handler_t core_h, long long int chat_id,
        const char *phone_number, const char *first_name, const char *last_name, const char *vcard,
        const telebot_send_options_t *options);

/**
 * @brief Sends a native poll or quiz.
 *
 * Bot API method: sendPoll.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] question Poll question, 1-300 characters.
 * @param[in] answers Answer options, 1-100 characters each.
 * @param[in] count_answers Number of answer options, 1-12.
 * @param[in] is_anonymous The poll is anonymous (Telegram's default is true, so pass it
 *     explicitly).
 * @param[in] type "regular" or "quiz"; NULL for "regular".
 * @param[in] allows_multiple_answers Voters may choose several answers.
 * @param[in] correct_option_ids Quiz only: zero-based indexes of the correct answers, or NULL.
 * @param[in] count_correct Number of entries in @p correct_option_ids, 0 for none.
 * @param[in] is_closed Send the poll already closed.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_poll(telebot_core_handler_t core_h, long long int chat_id,
        const char *question, const char *const answers[], int count_answers, bool is_anonymous,
        const char *type, bool allows_multiple_answers, const int correct_option_ids[], int count_correct,
        bool is_closed, const telebot_send_options_t *options);

/**
 * @brief Sends an animated emoji showing a random value.
 *
 * Bot API method: sendDice.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] emoji One of the supported emoji ("🎲", "🎯", "🏀", "⚽", "🎳", "🎰"), or NULL for the die.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_dice(telebot_core_handler_t core_h, long long int chat_id,
        const char *emoji, const telebot_send_options_t *options);

/**
 * @brief Shows a status such as "typing" or "uploading photo" for 5 seconds.
 *
 * Bot API method: sendChatAction.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] message_thread_id Forum topic or private-chat topic, or 0.
 * @param[in] action One of "typing", "upload_photo", "record_video", "upload_video",
 *     "record_voice", "upload_voice", "upload_document", "choose_sticker", "find_location",
 *     "record_video_note" or "upload_video_note".
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_chat_action(telebot_core_handler_t core_h, long long int chat_id,
        int message_thread_id, const char *action);

/**
 * @brief Gets a list of profile pictures of a user.
 *
 * Bot API method: getUserProfilePhotos.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] offset Number of the first photo to return; 0 for the newest.
 * @param[in] limit Number of photos to return, 1-100.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_get_user_profile_photos(telebot_core_handler_t core_h,
        long long int user_id, int offset, int limit);

/**
* @brief Get basic info about a file and prepare it for
* downloading. For the moment, bots can download files of up to 20MB in size.
* @param[in] core_h The telebot core handler created with #telebot_core_create().
* @param[in] file_id File identifier to get info about.
* @return #telebot_core_response_t response that contains file object,
* which MUST be released with #telebot_core_put_response(), or null if allocation fails.
* Response code should be checked with #telebot_core_get_response_code(),
* before getting data with #telebot_core_get_response_data().
*/
telebot_core_response_t telebot_core_get_file(telebot_core_handler_t core_h, const char *file_id);

/**
* @brief Download file using file_path obtained with
* telebot_core_get_file(). It is guaranteed that the link will be valid for
* at least 1 hour. When the link expires, a new one can be requested by
* calling telebot_core_get_file() again.
* @param[in] core_h The telebot core handler created with #telebot_core_create().
* @param[in] file_path Value of the file_path field returned by #telebot_core_get_file().
* @param[in] out_file Full path to download and save file.
* @return on Success, TELEBOT_ERROR_NONE is returned, otherwise a negative
* error value. No response, i.e., core_h->resp_data contains nothing.
*/
telebot_error_e telebot_core_download_file(telebot_core_handler_t core_h,
        const char *file_path, const char *out_file);

/**
 * @brief Unbans a previously banned user.
 *
 * Bot API method: unbanChatMember.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] only_if_banned Do nothing if the user is not banned; otherwise an unban also removes a
 *     member from the chat.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_unban_chat_member(telebot_core_handler_t core_h, long long int chat_id,
        long long int user_id, bool only_if_banned);

/**
 * @brief Restricts a user in a supergroup.
 *
 * Pass all permissions as true to lift restrictions.
 *
 * Bot API method: restrictChatMember.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] permissions Permissions the user keeps; anything left false is denied.
 * @param[in] use_independent_chat_permissions Apply each permission exactly as given, instead of
 *     letting some imply others.
 * @param[in] until_date Unix time when the measure ends; 0 (or more than 366 days ahead) means
 *     forever.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_restrict_chat_member(telebot_core_handler_t core_h,
        long long int chat_id, long long int user_id, const telebot_chat_permissions_t *permissions,
        bool use_independent_chat_permissions, long until_date);

/**
 * @brief Promotes or demotes a user in a supergroup or channel.
 *
 * Pass NULL or all rights false to demote the user.
 *
 * Bot API method: promoteChatMember.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] rights Rights to grant, or NULL to demote.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_promote_chat_member(telebot_core_handler_t core_h,
        long long int chat_id, long long int user_id, const telebot_chat_administrator_rights_t *rights);

/**
 * @brief Sets a custom title for an administrator the bot promoted.
 *
 * Bot API method: setChatAdministratorCustomTitle.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] custom_title New title, 0-16 characters, emoji are not allowed.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_set_chat_admin_custom_title(telebot_core_handler_t core_h,
        long long int chat_id, long long int user_id, const char *custom_title);

/**
 * @brief Sets the default permissions of all members of a group or supergroup.
 *
 * Bot API method: setChatPermissions.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] permissions New default permissions; anything left false is denied.
 * @param[in] use_independent_chat_permissions Apply each permission exactly as given, instead of
 *     letting some imply others.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_set_chat_permissions(telebot_core_handler_t core_h,
        long long int chat_id, const telebot_chat_permissions_t *permissions,
        bool use_independent_chat_permissions);

/**
 * @brief Generate a new invite link for a chat; any previously generated link is
 * revoked. The bot must be an administrator in the chat for this to work and must
 * have the appropriate admin rights.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @return #telebot_core_response_t response that contains new invite link,
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_export_chat_invite_link(telebot_core_handler_t core_h,
        long long int chat_id);

/**
 * @brief Sets a new profile photo for a group, supergroup or channel.
 *
 * Bot API method: setChatPhoto.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] photo New chat photo, uploaded from a path or memory. A file_id or URL is rejected.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_set_chat_photo(telebot_core_handler_t core_h, long long int chat_id,
        const telebot_input_file_t *photo);

/**
 * @brief Delete a chat photo. Photos can't be changed for private chats. The bot
 * must be an administrator in the chat for this to work and must have the
 * appropriate admin rights.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @return #telebot_core_response_t response that contains the result (true/false),
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_delete_chat_photo(telebot_core_handler_t core_h,
        long long int chat_id);

/**
 * @brief Change the title of a chat. Titles can't be
 * changed for private chats. The bot must be an administrator in the chat for
 * this to work and must have the appropriate admin rights.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] title New chat title, 1-255 characters.
 * @return #telebot_core_response_t response that contains the result (true/false),
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_set_chat_title(telebot_core_handler_t core_h,
        long long int chat_id, const char *title);

/**
 * @brief Change the description of a supergroup or a channel. The bot must be
 * an administrator in the chat for this to work and must have the appropriate
 * admin rights.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] description New chat description, 0-255 characters.
 * @return #telebot_core_response_t response that contains the result (true/false),
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_set_chat_description(telebot_core_handler_t core_h,
        long long int chat_id, const char *description);

/**
 * @brief Pin a message in a supergroup or a channel. The bot must be an administrator
 * in the chat for this to work and must have the 'can_pin_messages' admin right
 * in the supergroup or 'can_edit_messages' admin right in the channel.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] message_id Identifier of a message to pin.
 * @param[in] disable_notification  Pass True, if it is not necessary to send
 * a notification to all chat members about the new pinned message.
 * Notifications are always disabled in channels.
 * @return #telebot_core_response_t response that contains the result (true/false),
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_pin_chat_message(telebot_core_handler_t core_h,
        long long int chat_id, int message_id, bool disable_notification);

/**
 * @brief Unpin a message in a supergroup or a channel. The bot must be an administrator
 * in the chat for this to work and must have the 'can_pin_messages' admin right
 * in the supergroup or 'can_edit_messages' admin right in the channel.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @return #telebot_core_response_t response that contains the result (true/false),
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_unpin_chat_message(telebot_core_handler_t core_h,
        long long int chat_id);

/**
 * @brief Leave a group, supergroup or channel.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @return #telebot_core_response_t response that contains the result (true/false),
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_leave_chat(telebot_core_handler_t core_h, long long int chat_id);

/**
 * @brief Get up to date information about the chat (current name of the user for
 * one-on-one conversations, current username of a user, group or channel, etc).
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @return #telebot_core_response_t response that contains chat object,
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_get_chat(telebot_core_handler_t core_h,
        long long int chat_id);

/**
 * @brief Gets the administrators of a chat.
 *
 * Bot API method: getChatAdministrators.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] return_bots Also return other bots that are administrators.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_get_chat_admins(telebot_core_handler_t core_h, long long int chat_id,
        bool return_bots);

/**
 * @brief Get the number of members in a chat.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @return #telebot_core_response_t response that contains the number of chat members,
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_get_chat_members_count(telebot_core_handler_t core_h,
        long long int chat_id);

/**
 * @brief Gets information about a member of a chat.
 *
 * Bot API method: getChatMember.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_get_chat_member(telebot_core_handler_t core_h, long long int chat_id,
        long long int user_id);

/**
 * @brief Set a new group sticker set for a supergroup. The bot must be an administrator
 * in the chat for this to work and must have the appropriate admin rights. Use the field
 * can_set_sticker_set optionally returned in #telebot_core_get_chat requests to check
 * if the bot can use it.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @param[in] sticker_set_name Name of the sticker set to be set as the group
 * sticker set.
 * @return #telebot_core_response_t response that contains the result (true/false),
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_set_chat_sticker_set(telebot_core_handler_t core_h,
        long long int chat_id, const char *sticker_set_name);

/**
 * @brief Delete a group sticker set from a supergroup. The bot must be an administrator
 * in the chat for this to work and must have the appropriate admin rights. Use the
 * field can_set_sticker_set optionally returned in #telebot_core_get_chat requests
 * to check if the bot can use this method.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier for the target chat or username of the
 * target channel (in the format \@channelusername).
 * @return #telebot_core_response_t response that contains the result (true/false),
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_delete_chat_sticker_set(telebot_core_handler_t core_h,
        long long int chat_id);

/**
 * @brief Send answers to callback queries sent from inline keyboards.
 * The answer will be displayed to the user as a notification at the top of
 * the chat screen or as an alert.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] callback_query_id Unique identifier for the query to be answered.
 * @param[in] text Text of the notification. If not specified, nothing will be shown
 * to the user, 0-200 characters
 * @param[in] show_alert If true, an alert will be shown by the client instead of
 * a notification at the top of the chat screen. Defaults to false.
 * @param[in] url URL that will be opened by the user's client. If you have created
 * a Game and accepted the conditions via \@BotFather, specify the URL that opens
 * your game - note that this will only work if the query comes from a
 * callback_game button.
 * @param[in] cache_time The maximum amount of time in seconds that the result of
 * the callback query may be cached client-side. Telegram apps will support
 * caching starting in version 3.14.
 * @return #telebot_core_response_t response that contains the result (true/false),
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_answer_callback_query(telebot_core_handler_t core_h,
        const char *callback_query_id, const char *text, bool show_alert,
        const char *url, int cache_time);

/**
 * @brief Change the list of the bot's commands.
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] commands A JSON-serialized list of bot commands to be set as the
 * list of the bot's commands. At most 100 commands can be specified.
 * @return #telebot_core_response_t response that contains the result (true/false),
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_set_my_commands(telebot_core_handler_t core_h,
        const char *commands);

/**
 * @brief Get the current list of the bot's commands..
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @return #telebot_core_response_t response that contains an array of bot commands,
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_get_my_commands(telebot_core_handler_t core_h);

/**
 * @brief Edits the text of a message.
 *
 * Identify the message either by @p chat_id and @p message_id, or by @p inline_message_id.
 *
 * Bot API method: editMessageText.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] message_id Identifier of the message to edit, or 0 when using @p inline_message_id.
 * @param[in] inline_message_id Identifier of an inline message, or NULL.
 * @param[in] text New text, 1-4096 characters after entity parsing.
 * @param[in] parse_mode Optional formatting: "MarkdownV2", "HTML" or "Markdown"; NULL for plain
 *     text.
 * @param[in] disable_link_preview Do not show a preview for links in the text.
 * @param[in] reply_markup Optional JSON inline keyboard, or NULL.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_edit_message_text(telebot_core_handler_t core_h, long long int chat_id,
        int message_id, const char *inline_message_id, const char *text, const char *parse_mode,
        bool disable_link_preview, const char *reply_markup);

/**
 * @brief Edit captions of messages sent by the bot or via the bot (for inline bots).
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] message_id Identifier of the message, or 0 when using @p inline_message_id.
 * @param[in] inline_message_id Identifier of an inline message, or NULL when using @p chat_id and
 *     @p message_id.
 * @param[in] caption New caption, 0-1024 characters after entity parsing, or NULL to remove it.
 * @param[in] parse_mode Optional formatting: "MarkdownV2", "HTML" or "Markdown"; NULL for plain
 *     text.
 * @param[in] reply_markup Optional JSON inline keyboard, or NULL.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_edit_message_caption(telebot_core_handler_t core_h,
        long long int chat_id, int message_id, const char *inline_message_id,
        const char *caption, const char *parse_mode, const char *reply_markup);

/**
 * @brief Edit only the reply markup of messages sent by the bot or via the
 * bot (for inline bots).
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Required if inline_message_id. Unique identifier for the target
 * chat or username of the target channel (in the format \@channelusername).
 * @param[in] message_id Required if inline_message_id is not specified.
 * Identifier of the sent message.
 * @param[in] inline_message_id Required if chat_id and message_id are not
 * specified. Identifier of the inline message.
 * @param[in] reply_markup A JSON-serialized object for an inline keyboard.
 * @return #telebot_core_response_t response that contains the message edited,
 * which MUST be released with #telebot_core_put_response(), or null if allocation fails.
 * Response code should be checked with #telebot_core_get_response_code(),
 * before getting data with #telebot_core_get_response_data().
 */
telebot_core_response_t telebot_core_edit_message_reply_markup(telebot_core_handler_t core_h,
        long long int chat_id, int message_id, const char *inline_message_id,
        const char *reply_markup);

/**
 * @brief Replaces the photo, video, audio or document of a message.
 *
 * Identify the message either by @p chat_id and @p message_id, or by @p inline_message_id. New
 *     media can be uploaded from a path or memory, or given as a file_id or URL.
 *
 * Bot API method: editMessageMedia.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] message_id Identifier of the message to edit, or 0 when using @p inline_message_id.
 * @param[in] inline_message_id Identifier of an inline message, or NULL.
 * @param[in] media New content of the message; see #telebot_input_media_t.
 * @param[in] reply_markup Optional JSON inline keyboard, or NULL.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_edit_message_media(telebot_core_handler_t core_h, long long int chat_id,
        int message_id, const char *inline_message_id, const telebot_input_media_t *media,
        const char *reply_markup);

/**
 * @brief Stop a poll which was sent by the bot.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] message_id Identifier of the message with the poll.
 * @param[in] reply_markup Optional JSON inline keyboard, or NULL.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_stop_poll(telebot_core_handler_t core_h,
        long long int chat_id, int message_id, const char *reply_markup);

/**
 * @brief Delete a message, including service messages, with the following
 *  limitations:
 *  - A message can only be deleted if it was sent less than 48 hours ago.
 *  - Bots can delete outgoing messages in groups and supergroups.
 *  - Bots granted can_post_messages permissions can delete outgoing messages
 *    in channels.
 *  - If the bot is an administrator of a group, it can delete any message there.
 *  - If the bot has can_delete_messages permission in a supergroup or a channel,
 *    it can delete any message there.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] message_id Identifier of the message, or 0 when using @p inline_message_id.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_delete_message(telebot_core_handler_t core_h,
        long long int chat_id, int message_id);

/**
 * @brief Use this method to log out from the cloud Bot API server before
 * launching the bot locally.
 * @param[in] core_h The telebot core handler.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_log_out(telebot_core_handler_t core_h);

/**
 * @brief Use this method to close the bot instance before moving it from one
 * local server to another.
 * @param[in] core_h The telebot core handler.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_close(telebot_core_handler_t core_h);

/**
 * @brief Use this method to change the bot's name.
 * @param[in] core_h The telebot core handler.
 * @param[in] name New bot name; 0-64 characters.
 * @param[in] language_code A two-letter ISO 639-1 language code.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_set_my_name(telebot_core_handler_t core_h,
        const char *name, const char *language_code);

/**
 * @brief Use this method to get the current bot name for the given user language.
 * @param[in] core_h The telebot core handler.
 * @param[in] language_code A two-letter ISO 639-1 language code.
 * @return #telebot_core_response_t response that contains BotName.
 */
telebot_core_response_t telebot_core_get_my_name(telebot_core_handler_t core_h,
        const char *language_code);

/**
 * @brief Use this method to change the bot's description.
 * @param[in] core_h The telebot core handler.
 * @param[in] description New bot description; 0-512 characters.
 * @param[in] language_code A two-letter ISO 639-1 language code.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_set_my_description(telebot_core_handler_t core_h,
        const char *description, const char *language_code);

/**
 * @brief Use this method to get the current bot description for the given user language.
 * @param[in] core_h The telebot core handler.
 * @param[in] language_code A two-letter ISO 639-1 language code.
 * @return #telebot_core_response_t response that contains BotDescription.
 */
telebot_core_response_t telebot_core_get_my_description(telebot_core_handler_t core_h,
        const char *language_code);

/**
 * @brief Use this method to change the bot's short description.
 * @param[in] core_h The telebot core handler.
 * @param[in] short_description New short description; 0-120 characters.
 * @param[in] language_code A two-letter ISO 639-1 language code.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_set_my_short_description(telebot_core_handler_t core_h,
        const char *short_description, const char *language_code);

/**
 * @brief Use this method to get the current bot short description for the given user language.
 * @param[in] core_h The telebot core handler.
 * @param[in] language_code A two-letter ISO 639-1 language code.
 * @return #telebot_core_response_t response that contains BotShortDescription.
 */
telebot_core_response_t telebot_core_get_my_short_description(telebot_core_handler_t core_h,
        const char *language_code);

/**
 * @brief Use this method to change the bot's menu button in a private chat, or the default menu button.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target private chat.
 * @param[in] menu_button A JSON-serialized object for the bot's new menu button.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_set_chat_menu_button(telebot_core_handler_t core_h,
        long long int chat_id, const char *menu_button);

/**
 * @brief Use this method to get the current value of the bot's menu button in a private chat, or the default menu button.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target private chat.
 * @return #telebot_core_response_t response that contains MenuButton.
 */
telebot_core_response_t telebot_core_get_chat_menu_button(telebot_core_handler_t core_h,
        long long int chat_id);

/**
 * @brief Use this method to change the default administrator rights requested by the bot when it's added as an administrator to groups or channels.
 * @param[in] core_h The telebot core handler.
 * @param[in] rights A JSON-serialized object describing new default administrator rights.
 * @param[in] for_channels Pass True to change the default administrator rights of the bot in channels.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_set_my_default_administrator_rights(telebot_core_handler_t core_h,
        const char *rights, bool for_channels);

/**
 * @brief Use this method to get the current default administrator rights of the bot.
 * @param[in] core_h The telebot core handler.
 * @param[in] for_channels Pass True to get default administrator rights of the bot in channels.
 * @return #telebot_core_response_t response that contains ChatAdministratorRights.
 */
telebot_core_response_t telebot_core_get_my_default_administrator_rights(telebot_core_handler_t core_h,
        bool for_channels);

/**
 * @brief Use this method to delete the list of the bot's commands for the given scope and user language.
 * @param[in] core_h The telebot core handler.
 * @param[in] scope A JSON-serialized object, describing scope of users for which the commands are relevant.
 * @param[in] language_code A two-letter ISO 639-1 language code.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_delete_my_commands(telebot_core_handler_t core_h,
        const char *scope, const char *language_code);

/**
 * @brief Copies a message without a link to the original.
 *
 * Bot API method: copyMessage.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] from_chat_id Chat where the original message was sent.
 * @param[in] message_id Identifier of the message in @p from_chat_id.
 * @param[in] caption Optional new caption for media, or NULL to keep the original.
 * @param[in] parse_mode Optional formatting of the caption: "MarkdownV2", "HTML" or "Markdown";
 *     NULL for plain text.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_copy_message(telebot_core_handler_t core_h, long long int chat_id,
        long long int from_chat_id, int message_id, const char *caption, const char *parse_mode,
        const telebot_send_options_t *options);

/**
 * @brief Copies several messages without links to the originals.
 *
 * Only message_thread_id, disable_notification and protect_content are taken from @p options.
 *
 * Bot API method: copyMessages.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] from_chat_id Chat where the original messages were sent.
 * @param[in] message_ids Identifiers of 1-100 messages in @p from_chat_id, in increasing order.
 * @param[in] count Number of identifiers in @p message_ids.
 * @param[in] remove_caption Copy media without their captions.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_copy_messages(telebot_core_handler_t core_h, long long int chat_id,
        long long int from_chat_id, const int message_ids[], int count, bool remove_caption,
        const telebot_send_options_t *options);

/**
 * @brief Forwards several messages at once, keeping album grouping.
 *
 * Only message_thread_id, disable_notification and protect_content are taken from @p options.
 *
 * Bot API method: forwardMessages.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] from_chat_id Chat where the original messages were sent.
 * @param[in] message_ids Identifiers of 1-100 messages in @p from_chat_id, in increasing order.
 * @param[in] count Number of identifiers in @p message_ids.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_forward_messages(telebot_core_handler_t core_h, long long int chat_id,
        long long int from_chat_id, const int message_ids[], int count, const telebot_send_options_t *options);

/**
 * @brief Use this method to delete multiple messages in a chat.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] message_ids A JSON-serialized list of 1-100 identifiers of messages to delete.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_delete_messages(telebot_core_handler_t core_h,
        long long int chat_id, const char *message_ids);

/**
 * @brief Bans a user from a group, supergroup or channel.
 *
 * The bot must be an administrator with the right to restrict members.
 *
 * Bot API method: banChatMember.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] until_date Unix time when the measure ends; 0 (or more than 366 days ahead) means
 *     forever.
 * @param[in] revoke_messages Delete all messages from the chat for the user.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_ban_chat_member(telebot_core_handler_t core_h, long long int chat_id,
        long long int user_id, long until_date, bool revoke_messages);

/**
 * @brief Use this method to ban a channel chat in a supergroup or a channel.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] sender_chat_id Unique identifier of the target sender chat.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_ban_chat_sender_chat(telebot_core_handler_t core_h,
        long long int chat_id, long long int sender_chat_id);

/**
 * @brief Use this method to unban a previously banned channel chat in a supergroup or a channel.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] sender_chat_id Unique identifier of the target sender chat.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_unban_chat_sender_chat(telebot_core_handler_t core_h,
        long long int chat_id, long long int sender_chat_id);

/**
 * @brief Use this method to create an additional invite link for a chat.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] name Invite link name; 0-32 characters.
 * @param[in] expire_date Point in time (Unix timestamp) when the link will expire.
 * @param[in] member_limit The maximum number of users that can be members of the chat simultaneously after joining the chat via this invite link; 1-99999.
 * @param[in] creates_join_request True, if users joining the chat via the link need to be approved by chat administrators.
 * @return #telebot_core_response_t response that contains ChatInviteLink.
 */
telebot_core_response_t telebot_core_create_chat_invite_link(telebot_core_handler_t core_h,
        long long int chat_id, const char *name, long expire_date, int member_limit,
        bool creates_join_request);

/**
 * @brief Use this method to edit a non-primary invite link created by the bot.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] invite_link The invite link to edit.
 * @param[in] name Invite link name; 0-32 characters.
 * @param[in] expire_date Point in time (Unix timestamp) when the link will expire.
 * @param[in] member_limit The maximum number of users that can be members of the chat simultaneously after joining the chat via this invite link; 1-99999.
 * @param[in] creates_join_request True, if users joining the chat via the link need to be approved by chat administrators.
 * @return #telebot_core_response_t response that contains ChatInviteLink.
 */
telebot_core_response_t telebot_core_edit_chat_invite_link(telebot_core_handler_t core_h,
        long long int chat_id, const char *invite_link, const char *name,
        long expire_date, int member_limit, bool creates_join_request);

/**
 * @brief Use this method to revoke an invite link created by the bot.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] invite_link The invite link to revoke.
 * @return #telebot_core_response_t response that contains ChatInviteLink.
 */
telebot_core_response_t telebot_core_revoke_chat_invite_link(telebot_core_handler_t core_h,
        long long int chat_id, const char *invite_link);

/**
 * @brief Approves a request to join a chat.
 *
 * Bot API method: approveChatJoinRequest.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_approve_chat_join_request(telebot_core_handler_t core_h,
        long long int chat_id, long long int user_id);

/**
 * @brief Declines a request to join a chat.
 *
 * Bot API method: declineChatJoinRequest.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_decline_chat_join_request(telebot_core_handler_t core_h,
        long long int chat_id, long long int user_id);

/**
 * @brief Sets or removes the bot's reaction to a message.
 *
 * Bot API method: setMessageReaction.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] message_id Identifier of the message to react to.
 * @param[in] emoji Reaction emoji such as "👍", or NULL to remove the bot's reaction.
 * @param[in] is_big Show the reaction with a big animation.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_set_message_reaction(telebot_core_handler_t core_h,
        long long int chat_id, int message_id, const char *emoji, bool is_big);

/**
 * @brief Use this method to create a topic in a forum supergroup chat.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] name Topic name, 1-128 characters.
 * @param[in] icon_color Color of the topic icon in RGB format.
 * @param[in] icon_custom_emoji_id Unique identifier of the custom emoji used as the topic icon.
 * @return #telebot_core_response_t response that contains ForumTopic.
 */
telebot_core_response_t telebot_core_create_forum_topic(telebot_core_handler_t core_h,
        long long int chat_id, const char *name, int icon_color, const char *icon_custom_emoji_id);

/**
 * @brief Use this method to edit name and icon of a topic in a forum supergroup chat.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] message_thread_id Unique identifier for the target message thread of the forum topic.
 * @param[in] name New topic name, 0-128 characters.
 * @param[in] icon_custom_emoji_id New unique identifier of the custom emoji used as the topic icon.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_edit_forum_topic(telebot_core_handler_t core_h,
        long long int chat_id, int message_thread_id, const char *name, const char *icon_custom_emoji_id);

/**
 * @brief Use this method to close an open topic in a forum supergroup chat.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] message_thread_id Unique identifier for the target message thread of the forum topic.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_close_forum_topic(telebot_core_handler_t core_h,
        long long int chat_id, int message_thread_id);

/**
 * @brief Use this method to reopen a closed topic in a forum supergroup chat.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] message_thread_id Unique identifier for the target message thread of the forum topic.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_reopen_forum_topic(telebot_core_handler_t core_h,
        long long int chat_id, int message_thread_id);

/**
 * @brief Use this method to delete a forum topic along with all its messages in a forum supergroup chat.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] message_thread_id Unique identifier for the target message thread of the forum topic.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_delete_forum_topic(telebot_core_handler_t core_h,
        long long int chat_id, int message_thread_id);

/**
 * @brief Use this method to unpin all messages in a forum topic.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] message_thread_id Unique identifier for the target message thread of the forum topic.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_unpin_all_forum_topic_messages(telebot_core_handler_t core_h,
        long long int chat_id, int message_thread_id);

/**
 * @brief Use this method to get custom emoji stickers, which can be used as a forum topic icon by any user.
 * @param[in] core_h The telebot core handler.
 * @return #telebot_core_response_t response that contains an array of Sticker objects.
 */
telebot_core_response_t telebot_core_get_forum_topic_icon_stickers(telebot_core_handler_t core_h);

/**
 * @brief Use this method to edit the name of the 'General' topic in a forum supergroup chat.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] name New topic name, 1-128 characters.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_edit_general_forum_topic(telebot_core_handler_t core_h,
        long long int chat_id, const char *name);

/**
 * @brief Use this method to close an open 'General' topic in a forum supergroup chat.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_close_general_forum_topic(telebot_core_handler_t core_h,
        long long int chat_id);

/**
 * @brief Use this method to reopen a closed 'General' topic in a forum supergroup chat.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_reopen_general_forum_topic(telebot_core_handler_t core_h,
        long long int chat_id);

/**
 * @brief Use this method to hide the 'General' topic in a forum supergroup chat.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_hide_general_forum_topic(telebot_core_handler_t core_h,
        long long int chat_id);

/**
 * @brief Use this method to unhide the 'General' topic in a forum supergroup chat.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_unhide_general_forum_topic(telebot_core_handler_t core_h,
        long long int chat_id);

/**
 * @brief Use this method to unpin all messages in a General forum topic.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @return #telebot_core_response_t response that contains the result (true/false).
 */
telebot_core_response_t telebot_core_unpin_all_general_forum_topic_messages(telebot_core_handler_t core_h,
        long long int chat_id);

/**
 * @brief Use this method to get information about custom emoji stickers by their identifiers.
 * @param[in] core_h The telebot core handler.
 * @param[in] custom_emoji_ids List of custom emoji identifiers.
 * @return #telebot_core_response_t response that contains an array of Sticker objects.
 */
telebot_core_response_t telebot_core_get_custom_emoji_stickers(telebot_core_handler_t core_h,
        const char *custom_emoji_ids);

/**
 * @brief Use this method to send answers to an inline query to a user from a Web App.
 * @param[in] core_h The telebot core handler.
 * @param[in] web_app_query_id Unique identifier for the query to be answered.
 * @param[in] result A JSON-serialized object describing the message to be sent.
 * @return #telebot_core_response_t response that contains SentWebAppMessage.
 */
telebot_core_response_t telebot_core_answer_web_app_query(telebot_core_handler_t core_h,
        const char *web_app_query_id, const char *result);

/**
 * @brief Use this method to send paid media to a user.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] star_count The number of Telegram Stars that must be paid.
 * @param[in] media A JSON-serialized array of InputPaidMedia objects.
 * @param[in] caption Media caption.
 * @param[in] parse_mode Mode for parsing entities in the caption.
 * @param[in] caption_entities A JSON-serialized list of special entities.
 * @param[in] show_caption_above_media Pass True, if the caption must be shown above the message media.
 * @param[in] disable_notification Sends the message silently.
 * @param[in] protect_content Protects the contents of the sent message.
 * @param[in] reply_parameters Description of the message to reply to.
 * @param[in] reply_markup Additional interface options.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_send_paid_media(telebot_core_handler_t core_h,
        long long int chat_id, int star_count, const char *media, const char *caption,
        const char *parse_mode, const char *caption_entities, bool show_caption_above_media,
        bool disable_notification, bool protect_content, const char *reply_parameters,
        const char *reply_markup);

/**
 * @brief Use this method to send answers to an inline query.
 * @param[in] core_h The telebot core handler.
 * @param[in] inline_query_id Unique identifier for the answered query.
 * @param[in] results A JSON-serialized array of results for the inline query.
 * @param[in] cache_time The maximum amount of time in seconds that the result
 * of the inline query may be cached on the server.
 * @param[in] is_personal Pass True, if results may be cached only for the user
 * that sent the query.
 * @param[in] next_offset Pass the offset that a client should send in the next
 * query with the same text to receive more results.
 * @param[in] button A JSON-serialized object describing a button to be shown
 * above inline query results.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_answer_inline_query(telebot_core_handler_t core_h,
        const char *inline_query_id, const char *results, int cache_time,
        bool is_personal, const char *next_offset, const char *button);

/**
 * @brief Use this method to save a prepared inline message.
 * @param[in] core_h The telebot core handler.
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] result A JSON-serialized object describing the message to be sent.
 * @param[in] allow_user_chats Pass True, if the message can be sent to user chats.
 * @param[in] allow_bot_chats Pass True, if the message can be sent to bot chats.
 * @param[in] allow_group_chats Pass True, if the message can be sent to group chats.
 * @param[in] allow_channel_chats Pass True, if the message can be sent to channel chats.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_save_prepared_inline_message(telebot_core_handler_t core_h,
        long long int user_id, const char *result, bool allow_user_chats,
        bool allow_bot_chats, bool allow_group_chats, bool allow_channel_chats);

/**
 * @brief Use this method to send a gift.
 * @param[in] core_h The telebot core handler.
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] gift_id Identifier of the gift.
 * @param[in] pay_for_upgrade Pass True to pay for the gift upgrade.
 * @param[in] text Text that will be shown along with the gift.
 * @param[in] text_parse_mode Mode for parsing entities in the text.
 * @param[in] text_entities A JSON-serialized list of special entities.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_send_gift(telebot_core_handler_t core_h,
        long long int user_id, long long int chat_id, const char *gift_id,
        bool pay_for_upgrade, const char *text, const char *text_parse_mode,
        const char *text_entities);

/**
 * @brief Use this method to gift a Telegram Premium subscription to a user.
 * @param[in] core_h The telebot core handler.
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] month_count Number of months the Telegram Premium subscription will be active.
 * @param[in] star_count Number of Telegram Stars to pay.
 * @param[in] text Text that will be shown along with the service message.
 * @param[in] text_parse_mode Mode for parsing entities in the text.
 * @param[in] text_entities A JSON-serialized list of special entities.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_gift_premium_subscription(telebot_core_handler_t core_h,
        long long int user_id, int month_count, int star_count, const char *text,
        const char *text_parse_mode, const char *text_entities);

/**
 * @brief Use this method to get the current Telegram Stars balance of the bot.
 * @param[in] core_h The telebot core handler.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_get_my_star_balance(telebot_core_handler_t core_h);

/**
 * @brief Use this method to get the bot's Telegram Star transactions.
 * @param[in] core_h The telebot core handler.
 * @param[in] offset Number of transactions to skip.
 * @param[in] limit Maximum number of transactions to be retrieved.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_get_star_transactions(telebot_core_handler_t core_h,
        int offset, int limit);

/**
 * @brief Use this method to refund a successful payment in Telegram Stars.
 * @param[in] core_h The telebot core handler.
 * @param[in] user_id Identifier of the user whose payment will be refunded.
 * @param[in] telegram_payment_charge_id Telegram payment identifier.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_refund_star_payment(telebot_core_handler_t core_h,
        long long int user_id, const char *telegram_payment_charge_id);

/**
 * @brief Use this method to get a list of gifts that can be sent by the bot to users.
 * @param[in] core_h The telebot core handler.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_get_available_gifts(telebot_core_handler_t core_h);

/**
 * @brief Use this method to get a list of gifts received by a user.
 * @param[in] core_h The telebot core handler.
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] offset Number of gifts to skip.
 * @param[in] limit Maximum number of gifts to be retrieved.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_get_user_gifts(telebot_core_handler_t core_h,
        long long int user_id, int offset, int limit);

/**
 * @brief Use this method to get a list of gifts received by a chat.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier of the target chat.
 * @param[in] offset Number of gifts to skip.
 * @param[in] limit Maximum number of gifts to be retrieved.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_get_chat_gifts(telebot_core_handler_t core_h,
        long long int chat_id, int offset, int limit);

/**
 * @brief Use this method to upgrade a gift.
 * @param[in] core_h The telebot core handler.
 * @param[in] gift_id Identifier of the gift.
 * @param[in] pay_for_upgrade Pass True to pay for the gift upgrade.
 * @param[in] text Text that will be shown along with the gift.
 * @param[in] text_parse_mode Mode for parsing entities in the text.
 * @param[in] text_entities A JSON-serialized list of special entities.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_upgrade_gift(telebot_core_handler_t core_h,
        const char *gift_id, bool pay_for_upgrade, const char *text,
        const char *text_parse_mode, const char *text_entities);

/**
 * @brief Use this method to transfer a gift.
 * @param[in] core_h The telebot core handler.
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] gift_id Identifier of the gift.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_transfer_gift(telebot_core_handler_t core_h,
        long long int user_id, long long int chat_id, const char *gift_id);

/**
 * @brief Use this method to convert a gift to Telegram Stars.
 * @param[in] core_h The telebot core handler.
 * @param[in] gift_id Identifier of the gift.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_convert_gift_to_stars(telebot_core_handler_t core_h,
        const char *gift_id);

/**
 * @brief Use this method to get user profile audios.
 * @param[in] core_h The telebot core handler.
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] offset Number of audios to skip.
 * @param[in] limit Maximum number of audios to be retrieved.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_get_user_profile_audios(telebot_core_handler_t core_h,
        long long int user_id, int offset, int limit);

/**
 * @brief Changes the profile photo of the bot.
 *
 * Bot API method: setMyProfilePhoto.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] photo New photo, uploaded from a path or memory. A file_id or URL is rejected.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_set_my_profile_photo(telebot_core_handler_t core_h,
        const telebot_input_file_t *photo);

/**
 * @brief Use this method to remove the bot's profile photo.
 * @param[in] core_h The telebot core handler.
 * @param[in] photo_id Identifier of the photo to remove.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_remove_my_profile_photo(telebot_core_handler_t core_h,
        const char *photo_id);

/**
 * @brief Use this method to get information about the business connection.
 * @param[in] core_h The telebot core handler.
 * @param[in] business_connection_id Unique identifier of the business connection.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_get_business_connection(telebot_core_handler_t core_h,
        const char *business_connection_id);

/**
 * @brief Use this method to get user chat boosts.
 * @param[in] core_h The telebot core handler.
 * @param[in] chat_id Unique identifier for the target chat.
 * @param[in] user_id Unique identifier of the target user.
 * @return #telebot_core_response_t response.
 */
telebot_core_response_t telebot_core_get_user_chat_boosts(telebot_core_handler_t core_h,
        long long int chat_id, long long int user_id);

/**
 * @brief Use this method to set the score of the specified user in a game.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] score New score; must be non-negative.
 * @param[in] force Allow the score to decrease, e.g. to fix a mistake or ban a cheater.
 * @param[in] disable_edit_message Do not edit the game message to show the new scoreboard.
 * @param[in] chat_id Chat of the game message, or 0 when using @p inline_message_id.
 * @param[in] message_id Identifier of the message, or 0 when using @p inline_message_id.
 * @param[in] inline_message_id Identifier of an inline message, or NULL when using @p chat_id and
 *     @p message_id.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_set_game_score(telebot_core_handler_t core_h,
        long long int user_id, int score, bool force, bool disable_edit_message,
        long long int chat_id, int message_id, const char *inline_message_id);

/**
 * @brief Use this method to get data for high score tables.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] chat_id Chat of the game message, or 0 when using @p inline_message_id.
 * @param[in] message_id Identifier of the message, or 0 when using @p inline_message_id.
 * @param[in] inline_message_id Identifier of an inline message, or NULL when using @p chat_id and
 *     @p message_id.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_get_game_high_scores(telebot_core_handler_t core_h,
        long long int user_id, long long int chat_id, int message_id, const char *inline_message_id);

/**
 * @brief Sends a game.
 *
 * Bot API method: sendGame.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] game_short_name Short name of the game, set up via \@BotFather.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_game(telebot_core_handler_t core_h, long long int chat_id,
        const char *game_short_name, const telebot_send_options_t *options);

/**
 * @brief Use this method to set passport data errors.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] errors JSON array of PassportElementError describing the problems.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_set_passport_data_errors(telebot_core_handler_t core_h,
        long long int user_id, const char *errors);

/**
 * @brief Sends an invoice.
 *
 * Bot API method: sendInvoice.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] invoice What is being sold and how to pay; see #telebot_invoice_params_t.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_invoice(telebot_core_handler_t core_h, long long int chat_id,
        const telebot_invoice_params_t *invoice, const telebot_send_options_t *options);

/**
 * @brief Creates a link for an invoice that can be shared anywhere.
 *
 * start_parameter in @p invoice is ignored.
 *
 * Bot API method: createInvoiceLink.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] invoice What is being sold and how to pay; see #telebot_invoice_params_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_create_invoice_link(telebot_core_handler_t core_h,
        const telebot_invoice_params_t *invoice);

/**
 * @brief Use this method to reply to shipping queries.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] shipping_query_id Identifier of the shipping query to answer.
 * @param[in] ok True if the order can be processed; false to reject it with @p error_message.
 * @param[in] shipping_options JSON array of available ShippingOption when @p ok is true, or NULL.
 * @param[in] error_message Reason shown to the user when @p ok is false, or NULL.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_answer_shipping_query(telebot_core_handler_t core_h,
        const char *shipping_query_id, bool ok, const char *shipping_options,
        const char *error_message);

/**
 * @brief Use this method to respond to pre-checkout queries.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] pre_checkout_query_id Identifier of the pre-checkout query to answer.
 * @param[in] ok True if the order can be processed; false to reject it with @p error_message.
 * @param[in] error_message Reason shown to the user when @p ok is false, or NULL.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_answer_pre_checkout_query(telebot_core_handler_t core_h,
        const char *pre_checkout_query_id, bool ok, const char *error_message);

/**
 * @brief Sends a static .WEBP, animated .TGS or video .WEBM sticker.
 *
 * Bot API method: sendSticker.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] sticker Sticker to send: a file_id or HTTP URL, a local file, or bytes in memory; see
 *     #telebot_input_file_t.
 * @param[in] emoji Optional emoji associated with an uploaded sticker, or NULL.
 * @param[in] options Shared send options (reply, forum topic, keyboard, silent delivery...), or
 *     NULL for defaults; see #telebot_send_options_t.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_sticker(telebot_core_handler_t core_h, long long int chat_id,
        const telebot_input_file_t *sticker, const char *emoji, const telebot_send_options_t *options);

/**
 * @brief Gets a sticker set by name.
 *
 * Bot API method: getStickerSet.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] name Name of the sticker set, as used in t.me/addstickers/ links.
 * @return A response; read it with #telebot_core_get_response_code() and #telebot_core_get_response_data(),
 *     then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_get_sticker_set(telebot_core_handler_t core_h, const char *name);

/**
 * @brief Calls any Bot API method by name.
 *
 * Use it for methods this library does not wrap. Nothing is validated except
 * the parameter list itself; the response carries Telegram's raw JSON reply.
 * @code
 * telebot_param_t params[] = {
 *     {"chat_id", "-1001234567890", NULL},
 *     {"user_id", "123456789", NULL},
 *     {"tag", "moderator", NULL},
 * };
 * telebot_core_response_t r = telebot_core_call(core_h, "setChatMemberTag", params, 3);
 * @endcode
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] method Bot API method name, e.g. "setChatMemberTag".
 * @param[in] params Parameters; each sets exactly one of value or file. May be NULL when @p count is 0.
 * @param[in] count Number of entries in @p params.
 * @return A response; read it with #telebot_core_get_response_code() and #telebot_core_get_response_data(),
 *     then release it with #telebot_core_put_response(). #TELEBOT_ERROR_INVALID_PARAMETER is returned for
 *     a malformed parameter list without contacting Telegram.
 */
telebot_core_response_t telebot_core_call(telebot_core_handler_t core_h, const char *method,
        const telebot_param_t params[], size_t count);

/**
 * @brief Streams a partial message to a private chat while it is being generated.
 *
 * The draft is a temporary preview that disappears after about 30 seconds or when the bot sends a
 *     message. Call it repeatedly with the same @p draft_id to update the text with an animation,
 *     then send the final text with #telebot_send_message(). With @p can_stop, a stop button is
 *     shown and pressing it delivers a #TELEBOT_UPDATE_TYPE_STOPPED_MESSAGE_GENERATION update.
 *
 * Bot API method: sendMessageDraft.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target private chat.
 * @param[in] message_thread_id Topic in the private chat, or 0.
 * @param[in] draft_id Non-zero identifier of the draft; reuse it for every update of the same
 *     draft.
 * @param[in] text Current text, 0-4096 characters after entity parsing; NULL or "" shows a
 *     "Thinking" placeholder.
 * @param[in] parse_mode Optional formatting: "MarkdownV2", "HTML" or "Markdown"; NULL for plain
 *     text.
 * @param[in] can_stop Show the user a button that stops further drafts.
 * @param[in] keep_on_stop Keep the draft visible for a while after the stop button is pressed.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_send_message_draft(telebot_core_handler_t core_h, long long int chat_id,
        int message_thread_id, int draft_id, const char *text, const char *parse_mode, bool can_stop,
        bool keep_on_stop);

/**
 * @brief Unpins all pinned messages in a chat.
 *
 * In groups and channels the bot needs the can_pin_messages or can_edit_messages right.
 *
 * Bot API method: unpinAllChatMessages.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_unpin_all_chat_messages(telebot_core_handler_t core_h,
        long long int chat_id);

/**
 * @brief Removes the reaction a user or chat added to a message.
 *
 * Pass exactly one of @p user_id and @p actor_chat_id. The bot needs the can_delete_messages right.
 *
 * Bot API method: deleteMessageReaction.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] message_id Identifier of the message.
 * @param[in] user_id User whose reaction is removed, or 0.
 * @param[in] actor_chat_id Chat whose reaction is removed, or 0.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_delete_message_reaction(telebot_core_handler_t core_h,
        long long int chat_id, int message_id, long long int user_id, long long int actor_chat_id);

/**
 * @brief Removes up to 10000 recent reactions added by a user or chat.
 *
 * Pass exactly one of @p user_id and @p actor_chat_id. The bot needs the can_delete_messages right.
 *
 * Bot API method: deleteAllMessageReactions.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id User whose reactions are removed, or 0.
 * @param[in] actor_chat_id Chat whose reactions are removed, or 0.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_delete_all_message_reactions(telebot_core_handler_t core_h,
        long long int chat_id, long long int user_id, long long int actor_chat_id);

/**
 * @brief Sets or removes the tag of a member of a group or supergroup.
 *
 * The bot needs the can_manage_tags right.
 *
 * Bot API method: setChatMemberTag.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] tag New tag, 0-16 characters without emoji, or NULL to remove the tag.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_set_chat_member_tag(telebot_core_handler_t core_h,
        long long int chat_id, long long int user_id, const char *tag);

/**
 * @brief Verifies a user on behalf of the organization the bot represents.
 *
 * Bot API method: verifyUser.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] user_id Unique identifier of the target user.
 * @param[in] custom_description Optional verification description, 0-70 characters, or NULL.
 * @param[in] parse_mode Optional formatting of @p custom_description (only text links are
 *     supported), or NULL.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_verify_user(telebot_core_handler_t core_h, long long int user_id,
        const char *custom_description, const char *parse_mode);

/**
 * @brief Verifies a chat on behalf of the organization the bot represents.
 *
 * Bot API method: verifyChat.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @param[in] custom_description Optional verification description, 0-70 characters, or NULL.
 * @param[in] parse_mode Optional formatting of @p custom_description (only text links are
 *     supported), or NULL.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_verify_chat(telebot_core_handler_t core_h, long long int chat_id,
        const char *custom_description, const char *parse_mode);

/**
 * @brief Removes the verification of a user that the organization added.
 *
 * Bot API method: removeUserVerification.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] user_id Unique identifier of the target user.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_remove_user_verification(telebot_core_handler_t core_h,
        long long int user_id);

/**
 * @brief Removes the verification of a chat that the organization added.
 *
 * Bot API method: removeChatVerification.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] chat_id Unique identifier of the target chat (negative for groups, supergroups and
 *     channels).
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_remove_chat_verification(telebot_core_handler_t core_h,
        long long int chat_id);

/**
 * @brief Answers a query from a widget.
 *
 * Answer within 15 seconds. The answer goes only to the widget that sent the query and does not
 *     change the data of the widget.
 *
 * Bot API method: answerWidgetQuery.
 *
 * @param[in] core_h The telebot core handler created with #telebot_core_create().
 * @param[in] widget_query_id Identifier of the query, from #telebot_widget_query_t.
 * @param[in] data Optional data returned to the widget, 0-1024 bytes, or NULL.
 * @return A response; read it with #telebot_core_get_response_code() and
 *     #telebot_core_get_response_data(), then release it with #telebot_core_put_response().
 */
telebot_core_response_t telebot_core_answer_widget_query(telebot_core_handler_t core_h,
        const char *widget_query_id, const char *data);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_CORE_API_H__ */
