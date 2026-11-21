/*
 * telebot
 *
 * Copyright (c) 2026 Elmurod Talipov.
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

/*
 * Core layer for sending, forwarding, copying and editing messages.
 *
 * Every function here follows one shape:
 *     (core_h, chat_id, <required content>, <method-specific extras>, options)
 * and is built with the request builder, so absent optional values are never
 * sent and invalid input is rejected before anything reaches the network.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "telebot-json.h"
#include "telebot-request.h"

/* {"is_disabled":true} is the only link preview setting exposed directly */
#define LINK_PREVIEW_DISABLED "{\"is_disabled\":true}"

/* Adds caption + parse_mode; parse_mode is meaningless without a caption */
static void add_caption(telebot_request_t *req, const char *caption, const char *parse_mode)
{
    telebot_request_str(req, "caption", caption);
    if (caption != NULL)
        telebot_request_str(req, "parse_mode", parse_mode);
}

/* Fields forward/copy accept from the shared options (no reply markup on forwards) */
static void add_forward_options(telebot_request_t *req, const telebot_send_options_t *options)
{
    if (options == NULL)
        return;
    telebot_request_id(req, "message_thread_id", options->message_thread_id);
    telebot_request_flag(req, "disable_notification", options->disable_notification);
    telebot_request_flag(req, "protect_content", options->protect_content);
    telebot_request_str(req, "message_effect_id", options->message_effect_id);
}

typedef struct int_array
{
    const int *values;
    int count;
} int_array_t;

/* [1,2,3] */
static void compose_int_array(telebot_json_writer_t *w, const void *ctx)
{
    const int_array_t *a = ctx;
    telebot_json_write_raw(w, "[");
    for (int i = 0; i < a->count; i++)
    {
        if (i > 0)
            telebot_json_write_raw(w, ",");
        telebot_json_write_int(w, a->values[i]);
    }
    telebot_json_write_raw(w, "]");
}

/* ------------------------------------------------------------------------ */
/* Text and forwarding                                                      */
/* ------------------------------------------------------------------------ */

telebot_core_response_t
telebot_core_send_message(telebot_core_handler_t core_h, long long int chat_id, const char *text,
                          const char *parse_mode, bool disable_link_preview, const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_str_required(&req, "text", text);
    telebot_request_str(&req, "parse_mode", parse_mode);
    if (disable_link_preview)
        telebot_request_str(&req, "link_preview_options", LINK_PREVIEW_DISABLED);
    telebot_request_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_MESSAGE, &req);
}

telebot_core_response_t
telebot_core_forward_message(telebot_core_handler_t core_h, long long int chat_id, long long int from_chat_id,
                             int message_id, const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_ll(&req, "from_chat_id", from_chat_id);
    if (message_id <= 0)
        telebot_request_str_required(&req, "message_id", NULL);
    telebot_request_int(&req, "message_id", message_id);
    add_forward_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_FORWARD_MESSAGE, &req);
}

telebot_core_response_t
telebot_core_forward_messages(telebot_core_handler_t core_h, long long int chat_id, long long int from_chat_id,
                              const int message_ids[], int count, const telebot_send_options_t *options)
{
    if ((message_ids == NULL) || (count <= 0) || (count > 100))
    {
        ERR("Between 1 and 100 message ids are required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    const int_array_t ids = {message_ids, count};
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_ll(&req, "from_chat_id", from_chat_id);
    telebot_request_json(&req, "message_ids", compose_int_array, &ids);
    add_forward_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_FORWARD_MESSAGES, &req);
}

telebot_core_response_t
telebot_core_copy_message(telebot_core_handler_t core_h, long long int chat_id, long long int from_chat_id,
                          int message_id, const char *caption, const char *parse_mode,
                          const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_ll(&req, "from_chat_id", from_chat_id);
    if (message_id <= 0)
        telebot_request_str_required(&req, "message_id", NULL);
    telebot_request_int(&req, "message_id", message_id);
    add_caption(&req, caption, parse_mode);
    telebot_request_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_COPY_MESSAGE, &req);
}

telebot_core_response_t
telebot_core_copy_messages(telebot_core_handler_t core_h, long long int chat_id, long long int from_chat_id,
                           const int message_ids[], int count, bool remove_caption,
                           const telebot_send_options_t *options)
{
    if ((message_ids == NULL) || (count <= 0) || (count > 100))
    {
        ERR("Between 1 and 100 message ids are required");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    const int_array_t ids = {message_ids, count};
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_ll(&req, "from_chat_id", from_chat_id);
    telebot_request_json(&req, "message_ids", compose_int_array, &ids);
    telebot_request_flag(&req, "remove_caption", remove_caption);
    add_forward_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_COPY_MESSAGES, &req);
}

/* ------------------------------------------------------------------------ */
/* Media                                                                    */
/* ------------------------------------------------------------------------ */

telebot_core_response_t
telebot_core_send_photo(telebot_core_handler_t core_h, long long int chat_id, const telebot_input_file_t *photo,
                        const char *caption, const char *parse_mode, const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_file_required(&req, "photo", photo);
    add_caption(&req, caption, parse_mode);
    telebot_request_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_PHOTO, &req);
}

telebot_core_response_t
telebot_core_send_audio(telebot_core_handler_t core_h, long long int chat_id, const telebot_input_file_t *audio,
                        const char *caption, const char *parse_mode, int duration, const char *performer,
                        const char *title, const telebot_input_file_t *thumbnail,
                        const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_file_required(&req, "audio", audio);
    add_caption(&req, caption, parse_mode);
    telebot_request_id(&req, "duration", duration);
    telebot_request_str(&req, "performer", performer);
    telebot_request_str(&req, "title", title);
    telebot_request_upload(&req, "thumbnail", thumbnail);
    telebot_request_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_AUDIO, &req);
}

telebot_core_response_t
telebot_core_send_document(telebot_core_handler_t core_h, long long int chat_id,
                           const telebot_input_file_t *document, const telebot_input_file_t *thumbnail,
                           const char *caption, const char *parse_mode, const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_file_required(&req, "document", document);
    telebot_request_upload(&req, "thumbnail", thumbnail);
    add_caption(&req, caption, parse_mode);
    telebot_request_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_DOCUMENT, &req);
}

telebot_core_response_t
telebot_core_send_video(telebot_core_handler_t core_h, long long int chat_id, const telebot_input_file_t *video,
                        int duration, int width, int height, const telebot_input_file_t *thumbnail,
                        const char *caption, const char *parse_mode, bool supports_streaming,
                        const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_file_required(&req, "video", video);
    telebot_request_id(&req, "duration", duration);
    telebot_request_id(&req, "width", width);
    telebot_request_id(&req, "height", height);
    telebot_request_upload(&req, "thumbnail", thumbnail);
    add_caption(&req, caption, parse_mode);
    telebot_request_flag(&req, "supports_streaming", supports_streaming);
    telebot_request_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_VIDEO, &req);
}

telebot_core_response_t
telebot_core_send_animation(telebot_core_handler_t core_h, long long int chat_id,
                            const telebot_input_file_t *animation, int duration, int width, int height,
                            const telebot_input_file_t *thumbnail, const char *caption, const char *parse_mode,
                            const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_file_required(&req, "animation", animation);
    telebot_request_id(&req, "duration", duration);
    telebot_request_id(&req, "width", width);
    telebot_request_id(&req, "height", height);
    telebot_request_upload(&req, "thumbnail", thumbnail);
    add_caption(&req, caption, parse_mode);
    telebot_request_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_ANIMATION, &req);
}

telebot_core_response_t
telebot_core_send_voice(telebot_core_handler_t core_h, long long int chat_id, const telebot_input_file_t *voice,
                        const char *caption, const char *parse_mode, int duration,
                        const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_file_required(&req, "voice", voice);
    add_caption(&req, caption, parse_mode);
    telebot_request_id(&req, "duration", duration);
    telebot_request_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_VOICE, &req);
}

telebot_core_response_t
telebot_core_send_video_note(telebot_core_handler_t core_h, long long int chat_id,
                             const telebot_input_file_t *video_note, int duration, int length,
                             const telebot_input_file_t *thumbnail, const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_file_required(&req, "video_note", video_note);
    telebot_request_id(&req, "duration", duration);
    telebot_request_id(&req, "length", length);
    telebot_request_upload(&req, "thumbnail", thumbnail);
    telebot_request_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_VIDEO_NOTE, &req);
}

telebot_core_response_t
telebot_core_send_sticker(telebot_core_handler_t core_h, long long int chat_id, const telebot_input_file_t *sticker,
                          const char *emoji, const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_file_required(&req, "sticker", sticker);
    telebot_request_str(&req, "emoji", emoji);
    telebot_request_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_STICKER, &req);
}

/* Upload part names for media group items ("attach://fileN") */
static const char *const media_group_parts[] = {"file0", "file1", "file2", "file3", "file4",
                                                "file5", "file6", "file7", "file8", "file9"};
static const char *const media_type_names[] = {"photo", "video", "audio", "document"};

#define MEDIA_GROUP_MAX ((int)(sizeof(media_group_parts) / sizeof(media_group_parts[0])))

/*
 * InputMedia JSON for one item. A file_id/URL goes into the JSON; an upload is
 * added to the request as `part` and referenced as "attach://<part>".
 * Returns NULL for an invalid item.
 */
/* Validates an item and, for uploads, adds the file as form part `part` */
static bool media_item_prepare(telebot_request_t *req, const telebot_input_media_t *item, const char *part)
{
    if (((unsigned)item->type) >= (sizeof(media_type_names) / sizeof(media_type_names[0])))
        return false;
    if (item->file.kind == TELEBOT_INPUT_FILE_REMOTE)
        return (item->file.ref != NULL) && (item->file.ref[0] != '\0');
    telebot_request_file_required(req, part, &item->file);
    return req->error == TELEBOT_ERROR_NONE;
}

/* {"type":...,"media":...,"caption":...}; uploads are referenced as attach://<part> */
static void write_media_item(telebot_json_writer_t *w, const telebot_input_media_t *item, const char *part)
{
    telebot_json_write_raw(w, "{\"type\":");
    telebot_json_write_string(w, media_type_names[item->type]);
    telebot_json_write_raw(w, ",\"media\":");
    if (item->file.kind == TELEBOT_INPUT_FILE_REMOTE)
    {
        telebot_json_write_string(w, item->file.ref);
    }
    else
    {
        telebot_json_write_raw(w, "\"attach://");
        telebot_json_write_raw(w, part);
        telebot_json_write_raw(w, "\"");
    }
    if (item->caption != NULL)
    {
        telebot_json_write_raw(w, ",\"caption\":");
        telebot_json_write_string(w, item->caption);
        if (item->parse_mode != NULL)
        {
            telebot_json_write_raw(w, ",\"parse_mode\":");
            telebot_json_write_string(w, item->parse_mode);
        }
    }
    telebot_json_write_raw(w, "}");
}

typedef struct media_list
{
    const telebot_input_media_t *items;
    int count;
} media_list_t;

static void compose_media_group(telebot_json_writer_t *w, const void *ctx)
{
    const media_list_t *list = ctx;
    telebot_json_write_raw(w, "[");
    for (int i = 0; i < list->count; i++)
    {
        if (i > 0)
            telebot_json_write_raw(w, ",");
        write_media_item(w, &list->items[i], media_group_parts[i]);
    }
    telebot_json_write_raw(w, "]");
}

static void compose_single_media(telebot_json_writer_t *w, const void *ctx)
{
    write_media_item(w, ctx, "file0");
}

telebot_core_response_t
telebot_core_send_media_group(telebot_core_handler_t core_h, long long int chat_id,
                              const telebot_input_media_t media[], int count, const telebot_send_options_t *options)
{
    if ((media == NULL) || (count < 2) || (count > MEDIA_GROUP_MAX))
    {
        ERR("A media group needs 2 to %d items", MEDIA_GROUP_MAX);
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    for (int i = 0; i < count; i++)
    {
        if (!media_item_prepare(&req, &media[i], media_group_parts[i]))
        {
            ERR("Media group item %d is invalid", i);
            telebot_request_release(&req);
            return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
        }
    }

    const media_list_t list = {media, count};
    telebot_request_json(&req, "media", compose_media_group, &list);
    /* sendMediaGroup takes no reply_markup */
    if (options != NULL)
    {
        telebot_send_options_t no_markup = *options;
        no_markup.reply_markup = NULL;
        telebot_request_send_options(&req, &no_markup);
    }
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_MEDIA_GROUP, &req);
}

/* ------------------------------------------------------------------------ */
/* Location, contact, poll, dice                                            */
/* ------------------------------------------------------------------------ */

telebot_core_response_t
telebot_core_send_location(telebot_core_handler_t core_h, long long int chat_id, double latitude, double longitude,
                           int live_period, const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_double(&req, "latitude", latitude);
    telebot_request_double(&req, "longitude", longitude);
    telebot_request_id(&req, "live_period", live_period);
    telebot_request_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_LOCATION, &req);
}

telebot_core_response_t
telebot_core_send_venue(telebot_core_handler_t core_h, long long int chat_id, double latitude, double longitude,
                        const char *title, const char *address, const char *foursquare_id,
                        const char *foursquare_type, const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_double(&req, "latitude", latitude);
    telebot_request_double(&req, "longitude", longitude);
    telebot_request_str_required(&req, "title", title);
    telebot_request_str_required(&req, "address", address);
    telebot_request_str(&req, "foursquare_id", foursquare_id);
    telebot_request_str(&req, "foursquare_type", foursquare_type);
    telebot_request_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_VENUE, &req);
}

telebot_core_response_t
telebot_core_send_contact(telebot_core_handler_t core_h, long long int chat_id, const char *phone_number,
                          const char *first_name, const char *last_name, const char *vcard,
                          const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_str_required(&req, "phone_number", phone_number);
    telebot_request_str_required(&req, "first_name", first_name);
    telebot_request_str(&req, "last_name", last_name);
    telebot_request_str(&req, "vcard", vcard);
    telebot_request_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_CONTACT, &req);
}

typedef struct poll_answers
{
    const char *const *texts;
    int count;
} poll_answers_t;

/* Array of InputPollOption: [{"text":...},...] */
static void compose_poll_answers(telebot_json_writer_t *w, const void *ctx)
{
    const poll_answers_t *list = ctx;
    telebot_json_write_raw(w, "[");
    for (int i = 0; i < list->count; i++)
    {
        telebot_json_write_raw(w, (i > 0) ? ",{\"text\":" : "{\"text\":");
        telebot_json_write_string(w, list->texts[i]);
        telebot_json_write_raw(w, "}");
    }
    telebot_json_write_raw(w, "]");
}

telebot_core_response_t
telebot_core_send_poll(telebot_core_handler_t core_h, long long int chat_id, const char *question,
                       const char *const answers[], int count_answers, bool is_anonymous, const char *type,
                       bool allows_multiple_answers, const int correct_option_ids[], int count_correct,
                       bool is_closed, const telebot_send_options_t *options)
{
    if ((answers == NULL) || (count_answers < 1) || (count_answers > 12))
    {
        ERR("A poll needs 1 to 12 answer options");
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }
    for (int i = 0; i < count_answers; i++)
    {
        if (answers[i] == NULL)
            return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }
    if ((count_correct > 0) && (correct_option_ids == NULL))
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);

    const poll_answers_t list = {answers, count_answers};
    const int_array_t correct = {correct_option_ids, count_correct};

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_str_required(&req, "question", question);
    telebot_request_json(&req, "options", compose_poll_answers, &list);
    telebot_request_bool(&req, "is_anonymous", is_anonymous); /* Telegram defaults to true: always explicit */
    telebot_request_str(&req, "type", type);
    telebot_request_flag(&req, "allows_multiple_answers", allows_multiple_answers);
    if (count_correct > 0)
        telebot_request_json(&req, "correct_option_ids", compose_int_array, &correct);
    telebot_request_flag(&req, "is_closed", is_closed);
    telebot_request_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_POLL, &req);
}

telebot_core_response_t
telebot_core_send_dice(telebot_core_handler_t core_h, long long int chat_id, const char *emoji,
                       const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_str(&req, "emoji", emoji);
    telebot_request_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_DICE, &req);
}

telebot_core_response_t
telebot_core_send_chat_action(telebot_core_handler_t core_h, long long int chat_id, int message_thread_id,
                              const char *action)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_id(&req, "message_thread_id", message_thread_id);
    telebot_request_str_required(&req, "action", action);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_CHAT_ACTION, &req);
}

/* ------------------------------------------------------------------------ */
/* Editing and reactions                                                    */
/* ------------------------------------------------------------------------ */

/* chat_id + message_id, or inline_message_id: exactly one form is required */
static void add_message_target(telebot_request_t *req, long long int chat_id, int message_id,
                               const char *inline_message_id)
{
    if (inline_message_id != NULL)
    {
        telebot_request_str(req, "inline_message_id", inline_message_id);
        return;
    }
    if (message_id <= 0)
        telebot_request_str_required(req, "message_id", NULL);
    telebot_request_ll(req, "chat_id", chat_id);
    telebot_request_int(req, "message_id", message_id);
}

telebot_core_response_t
telebot_core_edit_message_text(telebot_core_handler_t core_h, long long int chat_id, int message_id,
                               const char *inline_message_id, const char *text, const char *parse_mode,
                               bool disable_link_preview, const char *reply_markup)
{
    telebot_request_t req;
    telebot_request_init(&req);
    add_message_target(&req, chat_id, message_id, inline_message_id);
    telebot_request_str_required(&req, "text", text);
    telebot_request_str(&req, "parse_mode", parse_mode);
    if (disable_link_preview)
        telebot_request_str(&req, "link_preview_options", LINK_PREVIEW_DISABLED);
    telebot_request_str(&req, "reply_markup", reply_markup);
    return telebot_core_request(core_h, TELEBOT_METHOD_EDIT_MESSAGE_TEXT, &req);
}

telebot_core_response_t
telebot_core_set_message_reaction(telebot_core_handler_t core_h, long long int chat_id, int message_id,
                                  const char *emoji, bool is_big)
{
    char reaction[64];
    if (emoji != NULL)
    {
        /* Plain emoji only: anything needing escapes is not a reaction emoji */
        if ((strlen(emoji) > 16) || strpbrk(emoji, "\"\\"))
        {
            ERR("Invalid reaction emoji");
            return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
        }
        snprintf(reaction, sizeof(reaction), "[{\"type\":\"emoji\",\"emoji\":\"%s\"}]", emoji);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    if (message_id <= 0)
        telebot_request_str_required(&req, "message_id", NULL);
    telebot_request_int(&req, "message_id", message_id);
    telebot_request_str(&req, "reaction", (emoji != NULL) ? reaction : "[]"); /* [] removes the bot's reaction */
    telebot_request_flag(&req, "is_big", is_big);
    return telebot_core_request(core_h, TELEBOT_METHOD_SET_MESSAGE_REACTION, &req);
}

/* ------------------------------------------------------------------------ */
/* Games and invoices                                                       */
/* ------------------------------------------------------------------------ */

telebot_core_response_t
telebot_core_send_game(telebot_core_handler_t core_h, long long int chat_id, const char *game_short_name,
                       const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_str_required(&req, "game_short_name", game_short_name);
    telebot_request_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_GAME, &req);
}

/* Fields common to sendInvoice and createInvoiceLink */
static void add_invoice(telebot_request_t *req, const telebot_invoice_params_t *invoice)
{
    if (invoice == NULL)
    {
        telebot_request_str_required(req, "title", NULL);
        return;
    }
    telebot_request_str_required(req, "title", invoice->title);
    telebot_request_str_required(req, "description", invoice->description);
    telebot_request_str_required(req, "payload", invoice->payload);
    telebot_request_str(req, "provider_token", invoice->provider_token);
    telebot_request_str_required(req, "currency", invoice->currency);
    telebot_request_str_required(req, "prices", invoice->prices);
    telebot_request_id(req, "max_tip_amount", invoice->max_tip_amount);
    telebot_request_str(req, "suggested_tip_amounts", invoice->suggested_tip_amounts);
    telebot_request_str(req, "provider_data", invoice->provider_data);
    telebot_request_str(req, "photo_url", invoice->photo_url);
    telebot_request_id(req, "photo_size", invoice->photo_size);
    telebot_request_id(req, "photo_width", invoice->photo_width);
    telebot_request_id(req, "photo_height", invoice->photo_height);
    telebot_request_flag(req, "need_name", invoice->need_name);
    telebot_request_flag(req, "need_phone_number", invoice->need_phone_number);
    telebot_request_flag(req, "need_email", invoice->need_email);
    telebot_request_flag(req, "need_shipping_address", invoice->need_shipping_address);
    telebot_request_flag(req, "send_phone_number_to_provider", invoice->send_phone_number_to_provider);
    telebot_request_flag(req, "send_email_to_provider", invoice->send_email_to_provider);
    telebot_request_flag(req, "is_flexible", invoice->is_flexible);
}

telebot_core_response_t
telebot_core_send_invoice(telebot_core_handler_t core_h, long long int chat_id,
                          const telebot_invoice_params_t *invoice, const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    add_invoice(&req, invoice);
    if (invoice != NULL)
        telebot_request_str(&req, "start_parameter", invoice->start_parameter);
    telebot_request_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_INVOICE, &req);
}

telebot_core_response_t
telebot_core_create_invoice_link(telebot_core_handler_t core_h, const telebot_invoice_params_t *invoice)
{
    telebot_request_t req;
    telebot_request_init(&req);
    add_invoice(&req, invoice);
    return telebot_core_request(core_h, TELEBOT_METHOD_CREATE_INVOICE_LINK, &req);
}

/* ------------------------------------------------------------------------ */
/* Drafts, pins and reaction moderation                                     */
/* ------------------------------------------------------------------------ */

telebot_core_response_t
telebot_core_send_message_draft(telebot_core_handler_t core_h, long long int chat_id, int message_thread_id,
                                int draft_id, const char *text, const char *parse_mode, bool can_stop,
                                bool keep_on_stop)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_id(&req, "message_thread_id", message_thread_id);
    if (draft_id == 0)
        telebot_request_str_required(&req, "draft_id", NULL);
    telebot_request_int(&req, "draft_id", draft_id);
    telebot_request_str(&req, "text", text); /* NULL or "" shows a "Thinking" placeholder */
    if (text != NULL)
        telebot_request_str(&req, "parse_mode", parse_mode);
    telebot_request_flag(&req, "can_stop", can_stop);
    telebot_request_flag(&req, "keep_on_stop", keep_on_stop);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_MESSAGE_DRAFT, &req);
}

telebot_core_response_t telebot_core_unpin_all_chat_messages(telebot_core_handler_t core_h, long long int chat_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_UNPIN_ALL_CHAT_MESSAGES, &req);
}

/* A reaction belongs either to a user or to a chat: exactly one id must be given */
static void add_reaction_actor(telebot_request_t *req, long long int user_id, long long int actor_chat_id)
{
    if ((user_id != 0) == (actor_chat_id != 0))
    {
        telebot_request_str_required(req, "user_id", NULL);
        return;
    }
    telebot_request_id(req, "user_id", user_id);
    if (actor_chat_id != 0)
        telebot_request_ll(req, "actor_chat_id", actor_chat_id);
}

telebot_core_response_t
telebot_core_delete_message_reaction(telebot_core_handler_t core_h, long long int chat_id, int message_id,
                                     long long int user_id, long long int actor_chat_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    if (message_id <= 0)
        telebot_request_str_required(&req, "message_id", NULL);
    telebot_request_int(&req, "message_id", message_id);
    add_reaction_actor(&req, user_id, actor_chat_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_DELETE_MESSAGE_REACTION, &req);
}

telebot_core_response_t
telebot_core_delete_all_message_reactions(telebot_core_handler_t core_h, long long int chat_id,
                                          long long int user_id, long long int actor_chat_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    add_reaction_actor(&req, user_id, actor_chat_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_DELETE_ALL_MESSAGE_REACTIONS, &req);
}

telebot_core_response_t
telebot_core_edit_message_media(telebot_core_handler_t core_h, long long int chat_id, int message_id,
                                const char *inline_message_id, const telebot_input_media_t *media,
                                const char *reply_markup)
{
    telebot_request_t req;
    telebot_request_init(&req);
    add_message_target(&req, chat_id, message_id, inline_message_id);
    if ((media == NULL) || !media_item_prepare(&req, media, "file0"))
    {
        ERR("A valid media item is required");
        telebot_request_release(&req);
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }
    telebot_request_json(&req, "media", compose_single_media, media);
    telebot_request_str(&req, "reply_markup", reply_markup);
    return telebot_core_request(core_h, TELEBOT_METHOD_EDIT_MESSAGE_MEDIA, &req);
}

telebot_core_response_t
telebot_core_answer_widget_query(telebot_core_handler_t core_h, const char *widget_query_id, const char *data)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_str_required(&req, "widget_query_id", widget_query_id);
    telebot_request_str(&req, "data", data);
    return telebot_core_request(core_h, TELEBOT_METHOD_ANSWER_WIDGET_QUERY, &req);
}
