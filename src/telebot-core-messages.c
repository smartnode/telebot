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
    tb_req_str(req, "caption", caption);
    if (caption != NULL)
        tb_req_str(req, "parse_mode", parse_mode);
}

/* Fields forward/copy accept from the shared options (no reply markup on forwards) */
static void add_forward_options(telebot_request_t *req, const telebot_send_options_t *options)
{
    if (options == NULL)
        return;
    tb_req_id(req, "message_thread_id", options->message_thread_id);
    tb_req_flag(req, "disable_notification", options->disable_notification);
    tb_req_flag(req, "protect_content", options->protect_content);
    tb_req_str(req, "message_effect_id", options->message_effect_id);
}

/* Serialises ids as a JSON array; caller frees the result */
static char *int_array_json(const int values[], int count)
{
    cJSON *array = cJSON_CreateArray();
    for (int i = 0; array && (i < count); i++)
        cJSON_AddItemToArray(array, cJSON_CreateNumber(values[i]));
    char *json = array ? cJSON_PrintUnformatted(array) : NULL;
    tb_json_put(array);
    return json;
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
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_str_required(&req, "text", text);
    tb_req_str(&req, "parse_mode", parse_mode);
    if (disable_link_preview)
        tb_req_str(&req, "link_preview_options", LINK_PREVIEW_DISABLED);
    tb_req_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_MESSAGE, &req);
}

telebot_core_response_t
telebot_core_forward_message(telebot_core_handler_t core_h, long long int chat_id, long long int from_chat_id,
                             int message_id, const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_ll(&req, "from_chat_id", from_chat_id);
    if (message_id <= 0)
        tb_req_str_required(&req, "message_id", NULL);
    tb_req_int(&req, "message_id", message_id);
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

    char *ids = int_array_json(message_ids, count);
    if (ids == NULL)
        return telebot_core_error_response(TELEBOT_ERROR_OUT_OF_MEMORY);

    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_ll(&req, "from_chat_id", from_chat_id);
    tb_req_str(&req, "message_ids", ids);
    add_forward_options(&req, options);
    telebot_core_response_t response = telebot_core_request(core_h, TELEBOT_METHOD_FORWARD_MESSAGES, &req);
    free(ids);
    return response;
}

telebot_core_response_t
telebot_core_copy_message(telebot_core_handler_t core_h, long long int chat_id, long long int from_chat_id,
                          int message_id, const char *caption, const char *parse_mode,
                          const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_ll(&req, "from_chat_id", from_chat_id);
    if (message_id <= 0)
        tb_req_str_required(&req, "message_id", NULL);
    tb_req_int(&req, "message_id", message_id);
    add_caption(&req, caption, parse_mode);
    tb_req_send_options(&req, options);
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

    char *ids = int_array_json(message_ids, count);
    if (ids == NULL)
        return telebot_core_error_response(TELEBOT_ERROR_OUT_OF_MEMORY);

    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_ll(&req, "from_chat_id", from_chat_id);
    tb_req_str(&req, "message_ids", ids);
    tb_req_flag(&req, "remove_caption", remove_caption);
    add_forward_options(&req, options);
    telebot_core_response_t response = telebot_core_request(core_h, TELEBOT_METHOD_COPY_MESSAGES, &req);
    free(ids);
    return response;
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
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_file_required(&req, "photo", photo);
    add_caption(&req, caption, parse_mode);
    tb_req_send_options(&req, options);
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
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_file_required(&req, "audio", audio);
    add_caption(&req, caption, parse_mode);
    tb_req_id(&req, "duration", duration);
    tb_req_str(&req, "performer", performer);
    tb_req_str(&req, "title", title);
    tb_req_upload(&req, "thumbnail", thumbnail);
    tb_req_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_AUDIO, &req);
}

telebot_core_response_t
telebot_core_send_document(telebot_core_handler_t core_h, long long int chat_id,
                           const telebot_input_file_t *document, const telebot_input_file_t *thumbnail,
                           const char *caption, const char *parse_mode, const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_file_required(&req, "document", document);
    tb_req_upload(&req, "thumbnail", thumbnail);
    add_caption(&req, caption, parse_mode);
    tb_req_send_options(&req, options);
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
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_file_required(&req, "video", video);
    tb_req_id(&req, "duration", duration);
    tb_req_id(&req, "width", width);
    tb_req_id(&req, "height", height);
    tb_req_upload(&req, "thumbnail", thumbnail);
    add_caption(&req, caption, parse_mode);
    tb_req_flag(&req, "supports_streaming", supports_streaming);
    tb_req_send_options(&req, options);
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
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_file_required(&req, "animation", animation);
    tb_req_id(&req, "duration", duration);
    tb_req_id(&req, "width", width);
    tb_req_id(&req, "height", height);
    tb_req_upload(&req, "thumbnail", thumbnail);
    add_caption(&req, caption, parse_mode);
    tb_req_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_ANIMATION, &req);
}

telebot_core_response_t
telebot_core_send_voice(telebot_core_handler_t core_h, long long int chat_id, const telebot_input_file_t *voice,
                        const char *caption, const char *parse_mode, int duration,
                        const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_file_required(&req, "voice", voice);
    add_caption(&req, caption, parse_mode);
    tb_req_id(&req, "duration", duration);
    tb_req_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_VOICE, &req);
}

telebot_core_response_t
telebot_core_send_video_note(telebot_core_handler_t core_h, long long int chat_id,
                             const telebot_input_file_t *video_note, int duration, int length,
                             const telebot_input_file_t *thumbnail, const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_file_required(&req, "video_note", video_note);
    tb_req_id(&req, "duration", duration);
    tb_req_id(&req, "length", length);
    tb_req_upload(&req, "thumbnail", thumbnail);
    tb_req_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_VIDEO_NOTE, &req);
}

telebot_core_response_t
telebot_core_send_sticker(telebot_core_handler_t core_h, long long int chat_id, const telebot_input_file_t *sticker,
                          const char *emoji, const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_file_required(&req, "sticker", sticker);
    tb_req_str(&req, "emoji", emoji);
    tb_req_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_STICKER, &req);
}

/* Upload part names for media group items ("attach://fileN") */
static const char *const media_group_parts[] = {"file0", "file1", "file2", "file3", "file4",
                                                "file5", "file6", "file7", "file8", "file9"};
static const char *const media_type_names[] = {"photo", "video", "audio", "document"};

#define MEDIA_GROUP_MAX ((int)(sizeof(media_group_parts) / sizeof(media_group_parts[0])))

telebot_core_response_t
telebot_core_send_media_group(telebot_core_handler_t core_h, long long int chat_id,
                              const telebot_input_media_t media[], int count, const telebot_send_options_t *options)
{
    if ((media == NULL) || (count < 2) || (count > MEDIA_GROUP_MAX))
    {
        ERR("A media group needs 2 to %d items", MEDIA_GROUP_MAX);
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
    }

    cJSON *array = cJSON_CreateArray();
    if (array == NULL)
        return telebot_core_error_response(TELEBOT_ERROR_OUT_OF_MEMORY);

    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);

    for (int i = 0; i < count; i++)
    {
        const telebot_input_media_t *item = &media[i];
        if (((unsigned)item->type) >= (sizeof(media_type_names) / sizeof(media_type_names[0])))
        {
            ERR("Media group item %d has an invalid type", i);
            tb_json_put(array);
            return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
        }

        cJSON *obj = cJSON_CreateObject();
        cJSON_AddItemToArray(array, obj);
        cJSON_AddStringToObject(obj, "type", media_type_names[item->type]);

        if (item->file.kind == TELEBOT_INPUT_FILE_REMOTE)
        {
            /* Validates and adds nothing to the form: the id/URL goes in the JSON */
            if ((item->file.ref == NULL) || (item->file.ref[0] == '\0'))
            {
                ERR("Media group item %d has an empty file reference", i);
                tb_json_put(array);
                return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
            }
            cJSON_AddStringToObject(obj, "media", item->file.ref);
        }
        else
        {
            char attach[sizeof("attach://file9")];
            snprintf(attach, sizeof(attach), "attach://%s", media_group_parts[i]);
            cJSON_AddStringToObject(obj, "media", attach);
            tb_req_file_required(&req, media_group_parts[i], &item->file);
        }

        if (item->caption != NULL)
        {
            cJSON_AddStringToObject(obj, "caption", item->caption);
            if (item->parse_mode != NULL)
                cJSON_AddStringToObject(obj, "parse_mode", item->parse_mode);
        }
    }

    char *json = cJSON_PrintUnformatted(array);
    tb_json_put(array);
    if (json == NULL)
        return telebot_core_error_response(TELEBOT_ERROR_OUT_OF_MEMORY);

    tb_req_str(&req, "media", json);
    /* sendMediaGroup takes no reply_markup */
    if (options != NULL)
    {
        telebot_send_options_t no_markup = *options;
        no_markup.reply_markup = NULL;
        tb_req_send_options(&req, &no_markup);
    }

    telebot_core_response_t response = telebot_core_request(core_h, TELEBOT_METHOD_SEND_MEDIA_GROUP, &req);
    free(json);
    return response;
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
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_double(&req, "latitude", latitude);
    tb_req_double(&req, "longitude", longitude);
    tb_req_id(&req, "live_period", live_period);
    tb_req_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_LOCATION, &req);
}

telebot_core_response_t
telebot_core_send_venue(telebot_core_handler_t core_h, long long int chat_id, double latitude, double longitude,
                        const char *title, const char *address, const char *foursquare_id,
                        const char *foursquare_type, const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_double(&req, "latitude", latitude);
    tb_req_double(&req, "longitude", longitude);
    tb_req_str_required(&req, "title", title);
    tb_req_str_required(&req, "address", address);
    tb_req_str(&req, "foursquare_id", foursquare_id);
    tb_req_str(&req, "foursquare_type", foursquare_type);
    tb_req_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_VENUE, &req);
}

telebot_core_response_t
telebot_core_send_contact(telebot_core_handler_t core_h, long long int chat_id, const char *phone_number,
                          const char *first_name, const char *last_name, const char *vcard,
                          const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_str_required(&req, "phone_number", phone_number);
    tb_req_str_required(&req, "first_name", first_name);
    tb_req_str(&req, "last_name", last_name);
    tb_req_str(&req, "vcard", vcard);
    tb_req_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_CONTACT, &req);
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
    if ((count_correct > 0) && (correct_option_ids == NULL))
        return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);

    /* options: array of InputPollOption {"text": ...} */
    cJSON *array = cJSON_CreateArray();
    for (int i = 0; array && (i < count_answers); i++)
    {
        if (answers[i] == NULL)
        {
            tb_json_put(array);
            return telebot_core_error_response(TELEBOT_ERROR_INVALID_PARAMETER);
        }
        cJSON *opt = cJSON_CreateObject();
        cJSON_AddStringToObject(opt, "text", answers[i]);
        cJSON_AddItemToArray(array, opt);
    }
    char *answers_json = array ? cJSON_PrintUnformatted(array) : NULL;
    tb_json_put(array);
    char *correct_json = (count_correct > 0) ? int_array_json(correct_option_ids, count_correct) : NULL;
    if ((answers_json == NULL) || ((count_correct > 0) && (correct_json == NULL)))
    {
        free(answers_json);
        free(correct_json);
        return telebot_core_error_response(TELEBOT_ERROR_OUT_OF_MEMORY);
    }

    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_str_required(&req, "question", question);
    tb_req_str(&req, "options", answers_json);
    tb_req_bool(&req, "is_anonymous", is_anonymous); /* Telegram defaults to true: always explicit */
    tb_req_str(&req, "type", type);
    tb_req_flag(&req, "allows_multiple_answers", allows_multiple_answers);
    tb_req_str(&req, "correct_option_ids", correct_json);
    tb_req_flag(&req, "is_closed", is_closed);
    tb_req_send_options(&req, options);
    telebot_core_response_t response = telebot_core_request(core_h, TELEBOT_METHOD_SEND_POLL, &req);
    free(answers_json);
    free(correct_json);
    return response;
}

telebot_core_response_t
telebot_core_send_dice(telebot_core_handler_t core_h, long long int chat_id, const char *emoji,
                       const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_str(&req, "emoji", emoji);
    tb_req_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_DICE, &req);
}

telebot_core_response_t
telebot_core_send_chat_action(telebot_core_handler_t core_h, long long int chat_id, int message_thread_id,
                              const char *action)
{
    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_id(&req, "message_thread_id", message_thread_id);
    tb_req_str_required(&req, "action", action);
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
        tb_req_str(req, "inline_message_id", inline_message_id);
        return;
    }
    if (message_id <= 0)
        tb_req_str_required(req, "message_id", NULL);
    tb_req_ll(req, "chat_id", chat_id);
    tb_req_int(req, "message_id", message_id);
}

telebot_core_response_t
telebot_core_edit_message_text(telebot_core_handler_t core_h, long long int chat_id, int message_id,
                               const char *inline_message_id, const char *text, const char *parse_mode,
                               bool disable_link_preview, const char *reply_markup)
{
    telebot_request_t req;
    telebot_request_init(&req);
    add_message_target(&req, chat_id, message_id, inline_message_id);
    tb_req_str_required(&req, "text", text);
    tb_req_str(&req, "parse_mode", parse_mode);
    if (disable_link_preview)
        tb_req_str(&req, "link_preview_options", LINK_PREVIEW_DISABLED);
    tb_req_str(&req, "reply_markup", reply_markup);
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
    tb_req_ll(&req, "chat_id", chat_id);
    if (message_id <= 0)
        tb_req_str_required(&req, "message_id", NULL);
    tb_req_int(&req, "message_id", message_id);
    tb_req_str(&req, "reaction", (emoji != NULL) ? reaction : "[]"); /* [] removes the bot's reaction */
    tb_req_flag(&req, "is_big", is_big);
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
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_str_required(&req, "game_short_name", game_short_name);
    tb_req_send_options(&req, options);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_GAME, &req);
}

/* Fields common to sendInvoice and createInvoiceLink */
static void add_invoice(telebot_request_t *req, const telebot_invoice_params_t *invoice)
{
    if (invoice == NULL)
    {
        tb_req_str_required(req, "title", NULL);
        return;
    }
    tb_req_str_required(req, "title", invoice->title);
    tb_req_str_required(req, "description", invoice->description);
    tb_req_str_required(req, "payload", invoice->payload);
    tb_req_str(req, "provider_token", invoice->provider_token);
    tb_req_str_required(req, "currency", invoice->currency);
    tb_req_str_required(req, "prices", invoice->prices);
    tb_req_id(req, "max_tip_amount", invoice->max_tip_amount);
    tb_req_str(req, "suggested_tip_amounts", invoice->suggested_tip_amounts);
    tb_req_str(req, "provider_data", invoice->provider_data);
    tb_req_str(req, "photo_url", invoice->photo_url);
    tb_req_id(req, "photo_size", invoice->photo_size);
    tb_req_id(req, "photo_width", invoice->photo_width);
    tb_req_id(req, "photo_height", invoice->photo_height);
    tb_req_flag(req, "need_name", invoice->need_name);
    tb_req_flag(req, "need_phone_number", invoice->need_phone_number);
    tb_req_flag(req, "need_email", invoice->need_email);
    tb_req_flag(req, "need_shipping_address", invoice->need_shipping_address);
    tb_req_flag(req, "send_phone_number_to_provider", invoice->send_phone_number_to_provider);
    tb_req_flag(req, "send_email_to_provider", invoice->send_email_to_provider);
    tb_req_flag(req, "is_flexible", invoice->is_flexible);
}

telebot_core_response_t
telebot_core_send_invoice(telebot_core_handler_t core_h, long long int chat_id,
                          const telebot_invoice_params_t *invoice, const telebot_send_options_t *options)
{
    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);
    add_invoice(&req, invoice);
    if (invoice != NULL)
        tb_req_str(&req, "start_parameter", invoice->start_parameter);
    tb_req_send_options(&req, options);
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
    tb_req_ll(&req, "chat_id", chat_id);
    tb_req_id(&req, "message_thread_id", message_thread_id);
    if (draft_id == 0)
        tb_req_str_required(&req, "draft_id", NULL);
    tb_req_int(&req, "draft_id", draft_id);
    tb_req_str(&req, "text", text); /* NULL or "" shows a "Thinking" placeholder */
    if (text != NULL)
        tb_req_str(&req, "parse_mode", parse_mode);
    tb_req_flag(&req, "can_stop", can_stop);
    tb_req_flag(&req, "keep_on_stop", keep_on_stop);
    return telebot_core_request(core_h, TELEBOT_METHOD_SEND_MESSAGE_DRAFT, &req);
}

telebot_core_response_t telebot_core_unpin_all_chat_messages(telebot_core_handler_t core_h, long long int chat_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_UNPIN_ALL_CHAT_MESSAGES, &req);
}

/* A reaction belongs either to a user or to a chat: exactly one id must be given */
static void add_reaction_actor(telebot_request_t *req, long long int user_id, long long int actor_chat_id)
{
    if ((user_id != 0) == (actor_chat_id != 0))
    {
        tb_req_str_required(req, "user_id", NULL);
        return;
    }
    tb_req_id(req, "user_id", user_id);
    if (actor_chat_id != 0)
        tb_req_ll(req, "actor_chat_id", actor_chat_id);
}

telebot_core_response_t
telebot_core_delete_message_reaction(telebot_core_handler_t core_h, long long int chat_id, int message_id,
                                     long long int user_id, long long int actor_chat_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);
    if (message_id <= 0)
        tb_req_str_required(&req, "message_id", NULL);
    tb_req_int(&req, "message_id", message_id);
    add_reaction_actor(&req, user_id, actor_chat_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_DELETE_MESSAGE_REACTION, &req);
}

telebot_core_response_t
telebot_core_delete_all_message_reactions(telebot_core_handler_t core_h, long long int chat_id,
                                          long long int user_id, long long int actor_chat_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    tb_req_ll(&req, "chat_id", chat_id);
    add_reaction_actor(&req, user_id, actor_chat_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_DELETE_ALL_MESSAGE_REACTIONS, &req);
}
