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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <json.h>
#include <json_object.h>
#include <telebot-private.h>
#include <telebot-common.h>
#include <telebot-types.h>
#include <telebot-methods.h>
#include <telebot-stickers.h>
#include <telebot-parser.h>

static const char *telebot_update_type_str[TELEBOT_UPDATE_TYPE_MAX] = {
    "message", "edited_message", "channel_post",
    "edited_channel_post", "inline_query",
    "chonse_inline_result", "callback_query",
    "shipping_query", "pre_checkout_query",
    "poll", "poll_answer"};

static telebot_error_e telebot_parser_get_photos(struct json_object *obj, telebot_photo_t **photos, int *count);
static telebot_error_e telebot_parser_get_users(struct json_object *obj, telebot_user_t **users, int *count);

struct json_object *telebot_parser_str_to_obj(char *data)
{
    return json_tokener_parse(data);
}

telebot_error_e telebot_parser_get_updates(struct json_object *obj, telebot_update_t **updates, int *count)
{
    if ((obj == NULL) || (updates == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *array = obj;
    int array_len = json_object_array_length(array);
    if (!array_len)
        return TELEBOT_ERROR_OPERATION_FAILED;

    telebot_update_t *result = calloc(array_len, sizeof(telebot_update_t));
    if (result == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    *count = array_len;
    *updates = result;

    for (int index = 0; index < array_len; index++)
    {
        struct json_object *item = json_object_array_get_idx(array, index);

        struct json_object *update_id = NULL;
        if (json_object_object_get_ex(item, "update_id", &update_id))
        {
            result[index].update_id = json_object_get_int(update_id);
        }

        struct json_object *message = NULL;
        if (json_object_object_get_ex(item, "message", &message))
        {
            if (telebot_parser_get_message(message, &(result[index].message)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse message of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_MESSAGE;
            continue;
        }

        struct json_object *edited_message = NULL;
        if (json_object_object_get_ex(item, "edited_message", &edited_message))
        {
            if (telebot_parser_get_message(edited_message, &(result[index].edited_message)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse edited message of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_EDITED_MESSAGE;
            continue;
        }

        struct json_object *channel_post = NULL;
        if (json_object_object_get_ex(item, "channel_post", &channel_post))
        {
            if (telebot_parser_get_message(channel_post, &(result[index].channel_post)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse channel post of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CHANNEL_POST;
            continue;
        }

        struct json_object *edited_channel_post = NULL;
        if (json_object_object_get_ex(item, "edited_channel_post", &edited_channel_post))
        {
            if (telebot_parser_get_message(edited_channel_post, &(result[index].edited_channel_post)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse edited channel post of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_EDITED_CHANNEL_POST;
            continue;
        }

        struct json_object *callback_query = NULL;
        if (json_object_object_get_ex(item, "callback_query", &callback_query))
        {
            if (telebot_parser_get_callback_query(callback_query, &(result[index].callback_query)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse callback query of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CALLBACK_QUERY;
            continue;
        }

        struct json_object *poll = NULL;
        if (json_object_object_get_ex(item, "poll", &poll))
        {
            if (telebot_parser_get_poll(poll, &(result[index].poll)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse poll of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_POLL;
            continue;
        }

        struct json_object *poll_answer = NULL;
        if (json_object_object_get_ex(item, "poll_answer", &poll_answer))
        {
            if (telebot_parser_get_poll_answer(poll_answer, &(result[index].poll_anser)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse poll answer of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_POLL_ANSWER;
            continue;
        }

    } /* for index */

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_webhook_info(struct json_object *obj, telebot_webhook_info_t *info)
{
    if ((obj == NULL) || (info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(info, 0, sizeof(telebot_webhook_info_t));
    struct json_object *url = NULL;
    if (!json_object_object_get_ex(obj, "url", &url))
    {
        ERR("Object is not json webhook info type, url not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    info->url = TELEBOT_SAFE_STRDUP(json_object_get_string(url));

    struct json_object *certificate = NULL;
    if (!json_object_object_get_ex(obj, "has_custom_certificate", &certificate))
    {
        ERR("Object is not webhook info type, has_custom_certificate not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    info->has_custom_certificate = json_object_get_boolean(certificate);

    struct json_object *update_count = NULL;
    if (!json_object_object_get_ex(obj, "pending_update_count", &update_count))
    {
        ERR("Object is not webook info type, pending_update_count not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    info->pending_update_count = json_object_get_int(update_count);

    struct json_object *last_error_date = NULL;
    if (json_object_object_get_ex(obj, "last_error_date", &last_error_date))
    {
        info->last_error_date = json_object_get_double(last_error_date);
    }

    struct json_object *last_error_message = NULL;
    if (json_object_object_get_ex(obj, "last_error_message", &last_error_message))
        info->last_error_message = TELEBOT_SAFE_STRDUP(json_object_get_string(last_error_message));

    struct json_object *max_connections = NULL;
    if (json_object_object_get_ex(obj, "max_connections", &max_connections))
        info->max_connections = json_object_get_int(max_connections);

    struct json_object *allowed_updates = NULL;
    if (json_object_object_get_ex(obj, "allowed_updates", &allowed_updates))
    {
        int cnt = 0;
        int array_len = json_object_array_length(allowed_updates);
        for (int i = 0; i < array_len; i++)
        {
            struct json_object *item = json_object_array_get_idx(allowed_updates, i);
            const char *update_type = json_object_get_string(item);
            for (int j = 0; j < TELEBOT_UPDATE_TYPE_MAX; j++)
                if (strstr(update_type, telebot_update_type_str[j]))
                    info->allowed_updates[cnt++] = j;
        }
        info->allowed_updates_count = cnt;
    }
    else
    {
        for (int i = 0; i < TELEBOT_UPDATE_TYPE_MAX; i++)
            info->allowed_updates[i] = i;
        info->allowed_updates_count = TELEBOT_UPDATE_TYPE_MAX;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user(struct json_object *obj, telebot_user_t *user)
{
    if ((obj == NULL) || (user == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(user, 0, sizeof(telebot_user_t));
    struct json_object *id;
    if (!json_object_object_get_ex(obj, "id", &id))
    {
        ERR("Object is not json user type, id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    user->id = json_object_get_int(id);

    struct json_object *is_bot = NULL;
    if (!json_object_object_get_ex(obj, "is_bot", &is_bot))
    {
        ERR("Object is not json user type, is_bot not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    user->is_bot = json_object_get_boolean(is_bot);

    struct json_object *first_name = NULL;
    if (!json_object_object_get_ex(obj, "first_name", &first_name))
    {
        ERR("Object is not user type, first_name not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    user->first_name = TELEBOT_SAFE_STRDUP(json_object_get_string(first_name));

    struct json_object *last_name = NULL;
    if (json_object_object_get_ex(obj, "last_name", &last_name))
        user->last_name = TELEBOT_SAFE_STRDUP(json_object_get_string(last_name));

    struct json_object *username = NULL;
    if (json_object_object_get_ex(obj, "username", &username))
        user->username = TELEBOT_SAFE_STRDUP(json_object_get_string(username));

    struct json_object *language_code = NULL;
    if (json_object_object_get_ex(obj, "language_code", &language_code))
        user->language_code = TELEBOT_SAFE_STRDUP(json_object_get_string(language_code));

    struct json_object *can_join_groups = NULL;
    if (json_object_object_get_ex(obj, "can_join_groups", &can_join_groups))
        user->can_join_groups = json_object_get_boolean(can_join_groups);

    struct json_object *can_read_all_group_messages = NULL;
    if (json_object_object_get_ex(obj, "can_read_all_group_messages", &can_read_all_group_messages))
        user->can_read_all_group_messages = json_object_get_boolean(can_read_all_group_messages);

    struct json_object *supports_inline_queries = NULL;
    if (json_object_object_get_ex(obj, "supports_inline_queries", &supports_inline_queries))
        user->supports_inline_queries = json_object_get_boolean(supports_inline_queries);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_users(struct json_object *obj, telebot_user_t **users, int *count)
{
    struct json_object *array = obj;
    int array_len = json_object_array_length(array);
    if (array_len == 0)
        return TELEBOT_ERROR_OPERATION_FAILED;

    telebot_user_t *result = calloc(array_len, sizeof(telebot_user_t));
    if (result == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    *count = array_len;
    *users = result;

    int index;
    for (index = 0; index < array_len; index++)
    {
        struct json_object *item = json_object_array_get_idx(array, index);
        if (telebot_parser_get_user(item, &(result[index])) != TELEBOT_ERROR_NONE)
            ERR("Failed to parse user from users array");
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat(struct json_object *obj, telebot_chat_t *chat)
{
    if ((obj == NULL) || (chat == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(chat, 0, sizeof(telebot_chat_t));

    struct json_object *id;
    if (!json_object_object_get_ex(obj, "id", &id))
    {
        ERR("Object is not chat type, id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    chat->id = json_object_get_int64(id);

    struct json_object *type = NULL;
    if (!json_object_object_get_ex(obj, "type", &type))
    {
        ERR("Object is not chat type, type not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    chat->type = TELEBOT_SAFE_STRDUP(json_object_get_string(type));

    struct json_object *title = NULL;
    if (json_object_object_get_ex(obj, "title", &title))
        chat->title = TELEBOT_SAFE_STRDUP(json_object_get_string(title));

    struct json_object *username = NULL;
    if (json_object_object_get_ex(obj, "username", &username))
        chat->username = TELEBOT_SAFE_STRDUP(json_object_get_string(username));

    struct json_object *first_name = NULL;
    if (json_object_object_get_ex(obj, "first_name", &first_name))
        chat->first_name = TELEBOT_SAFE_STRDUP(json_object_get_string(first_name));

    struct json_object *last_name = NULL;
    if (json_object_object_get_ex(obj, "last_name", &last_name))
        chat->last_name = TELEBOT_SAFE_STRDUP(json_object_get_string(last_name));

    struct json_object *chat_photo = NULL;
    if (json_object_object_get_ex(obj, "photo", &chat_photo))
    {
        chat->photo = malloc(sizeof(telebot_chat_photo_t));
        if (telebot_parser_get_chat_photo(chat_photo, chat->photo) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <photo> from chat object");
            TELEBOT_SAFE_FREE(chat->photo);
        }
    }

    struct json_object *description = NULL;
    if (json_object_object_get_ex(obj, "description", &description))
        chat->description = TELEBOT_SAFE_STRDUP(json_object_get_string(description));

    struct json_object *invite_link = NULL;
    if (json_object_object_get_ex(obj, "invite_link", &invite_link))
        chat->invite_link = TELEBOT_SAFE_STRDUP(json_object_get_string(invite_link));

    struct json_object *pinned_message = NULL;
    if (json_object_object_get_ex(obj, "pinned_message", &pinned_message))
    {
        chat->pinned_message = malloc(sizeof(telebot_message_t));
        if (telebot_parser_get_message(pinned_message, chat->pinned_message) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <pinned_message> from chat object");
            TELEBOT_SAFE_FREE(chat->pinned_message);
        }
    }

    struct json_object *permissions = NULL;
    if (json_object_object_get_ex(obj, "permissions", &permissions))
    {
        chat->permissions = malloc(sizeof(telebot_chat_permissions_t));
        if (telebot_parser_get_chat_permissions(permissions, chat->permissions) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <permissions> from chat object");
            TELEBOT_SAFE_FREE(chat->permissions);
        }
    }

    struct json_object *slow_mode_delay = NULL;
    if (json_object_object_get_ex(obj, "slow_mode_delay", &slow_mode_delay))
    {
        chat->slow_mode_delay = json_object_get_int(slow_mode_delay);
    }

    struct json_object *sticker_set_name = NULL;
    if (json_object_object_get_ex(obj, "sticker_set_name", &sticker_set_name))
        chat->sticker_set_name = TELEBOT_SAFE_STRDUP(json_object_get_string(sticker_set_name));

    struct json_object *can_set_sticker_set = NULL;
    if (json_object_object_get_ex(obj, "can_set_sticker_set", &can_set_sticker_set))
        chat->can_set_sticker_set = json_object_get_boolean(can_set_sticker_set);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_admins(struct json_object *obj, telebot_chat_member_t **admins, int *count)
{
    int ret = TELEBOT_ERROR_NONE;
    struct json_object *array = obj;
    int array_len = json_object_array_length(array);
    if (!array_len)
        return TELEBOT_ERROR_OPERATION_FAILED;

    telebot_chat_member_t *result = calloc(array_len, sizeof(telebot_chat_member_t));
    if (result == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    *count = array_len;
    *admins = result;

    int index = 0;
    for (index = 0; index < array_len; index++)
    {
        struct json_object *item = json_object_array_get_idx(array, index);
        ret = telebot_parser_get_chat_member(item, &result[index]);
        if (ret != TELEBOT_ERROR_NONE)
            break;

    } /* for index */

    if (ret)
    {
        telebot_put_chat_admins(result, index + 1);
        *admins = NULL;
        *count = 0;
    }

    return ret;
}

telebot_error_e telebot_parser_get_message(struct json_object *obj, telebot_message_t *msg)
{
    if ((obj == NULL) || (msg == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(msg, 0, sizeof(telebot_message_t));

    /* Mandatory Fields */
    struct json_object *message_id = NULL;
    if (!json_object_object_get_ex(obj, "message_id", &message_id))
    {
        ERR("Failed to get <message_id> from message object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    msg->message_id = json_object_get_int(message_id);

    struct json_object *date = NULL;
    if (!json_object_object_get_ex(obj, "date", &date))
    {
        ERR("Failed to get <date> from message object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    msg->date = json_object_get_int(date);

    struct json_object *chat = NULL;
    if (!json_object_object_get_ex(obj, "chat", &chat))
    {
        ERR("Failed to get <date> from message object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    msg->chat = malloc(sizeof(telebot_chat_t));
    if (telebot_parser_get_chat(chat, msg->chat) != TELEBOT_ERROR_NONE)
    {
        ERR("Failed to get <chat> from message object");
        TELEBOT_SAFE_FREE(msg->chat);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    /* Optional Fields */
    struct json_object *from = NULL;
    if (json_object_object_get_ex(obj, "from", &from))
    {
        msg->from = malloc(sizeof(telebot_user_t));
        if (telebot_parser_get_user(from, msg->from) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <from user> from message object");
            TELEBOT_SAFE_FREE(msg->from);
        }
    }

    struct json_object *forward_from = NULL;
    if (json_object_object_get_ex(obj, "forward_from", &forward_from))
    {
        msg->forward_from = malloc(sizeof(telebot_user_t));
        if (telebot_parser_get_user(forward_from, msg->forward_from) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <forward from> from message object");
            TELEBOT_SAFE_FREE(msg->forward_from);
        }
    }

    struct json_object *forward_from_chat = NULL;
    if (json_object_object_get_ex(obj, "forward_from_chat", &forward_from_chat))
    {
        msg->forward_from = malloc(sizeof(telebot_user_t));
        if (telebot_parser_get_chat(forward_from_chat, msg->forward_from_chat) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <forward from> from message object");
            TELEBOT_SAFE_FREE(msg->forward_from_chat);
        }
    }

    struct json_object *forward_from_message_id = NULL;
    if (json_object_object_get_ex(obj, "forward_from_message_id", &forward_from_message_id))
        msg->forward_from_message_id = json_object_get_int(forward_from_message_id);

    struct json_object *forward_signature = NULL;
    if (json_object_object_get_ex(obj, "forward_signature", &forward_signature))
        msg->forward_signature = TELEBOT_SAFE_STRDUP(json_object_get_string(forward_signature));

    struct json_object *forward_sender_name = NULL;
    if (json_object_object_get_ex(obj, "forward_sender_name", &forward_sender_name))
        msg->forward_sender_name = TELEBOT_SAFE_STRDUP(json_object_get_string(forward_sender_name));

    struct json_object *forward_date = NULL;
    if (json_object_object_get_ex(obj, "forward_date", &forward_date))
        msg->forward_date = json_object_get_int(forward_date);

    struct json_object *reply_to_message = NULL;
    if (json_object_object_get_ex(obj, "reply_to_message", &reply_to_message))
    {
        msg->reply_to_message = malloc(sizeof(telebot_message_t));
        if (telebot_parser_get_message(reply_to_message, msg->reply_to_message) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <reply_to_message> from message object");
            TELEBOT_SAFE_FREE(msg->reply_to_message);
        }
    }

    struct json_object *edit_date = NULL;
    if (json_object_object_get_ex(obj, "edit_date", &edit_date))
        msg->edit_date = json_object_get_int(edit_date);

    struct json_object *media_group_id = NULL;
    if (json_object_object_get_ex(obj, "media_group_id", &media_group_id))
        msg->media_group_id = TELEBOT_SAFE_STRDUP(json_object_get_string(media_group_id));

    struct json_object *author_signature = NULL;
    if (json_object_object_get_ex(obj, "author_signature", &author_signature))
        msg->author_signature = TELEBOT_SAFE_STRDUP(json_object_get_string(author_signature));

    struct json_object *text = NULL;
    if (json_object_object_get_ex(obj, "text", &text))
        msg->text = TELEBOT_SAFE_STRDUP(json_object_get_string(text));

    struct json_object *entities = NULL;
    if (json_object_object_get_ex(obj, "entities", &entities))
    {
        if (telebot_parser_get_message_entities(entities, &(msg->entities), &(msg->count_entities)) !=
            TELEBOT_ERROR_NONE)
            ERR("Failed to get <entities> from message object");
    }

    struct json_object *caption_entities = NULL;
    if (json_object_object_get_ex(obj, "caption_entities", &caption_entities))
    {
        if (telebot_parser_get_message_entities(caption_entities, &(msg->caption_entities),
                                                &(msg->count_caption_entities)) != TELEBOT_ERROR_NONE)
            ERR("Failed to get <caption_entities> from message object");
    }

    struct json_object *audio = NULL;
    if (json_object_object_get_ex(obj, "audio", &audio))
    {
        msg->audio = malloc(sizeof(telebot_audio_t));
        if (telebot_parser_get_audio(audio, msg->audio) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <audio> from message object");
            TELEBOT_SAFE_FREE(msg->audio);
        }
    }

    struct json_object *document = NULL;
    if (json_object_object_get_ex(obj, "document", &document))
    {
        msg->document = malloc(sizeof(telebot_document_t));
        if (telebot_parser_get_document(document, msg->document) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <document> from message object");
            TELEBOT_SAFE_FREE(msg->document);
        }
    }

    //TODO: implement game parsing

    struct json_object *photo = NULL;
    if (json_object_object_get_ex(obj, "photo", &photo))
    {
        if (telebot_parser_get_photos(photo, &(msg->photos), &(msg->count_photos)) != TELEBOT_ERROR_NONE)
            ERR("Failed to get <photo> from message object");
    }

    //TODO: implement sticker parsing
    struct json_object *video = NULL;
    if (json_object_object_get_ex(obj, "video", &video))
    {
        msg->video = malloc(sizeof(telebot_video_t));
        if (telebot_parser_get_video(video, msg->video) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <video> from message object");
            TELEBOT_SAFE_FREE(msg->video);
        }
    }

    struct json_object *voice = NULL;
    if (json_object_object_get_ex(obj, "voice", &voice))
    {
        msg->voice = malloc(sizeof(telebot_voice_t));
        if (telebot_parser_get_voice(voice, msg->voice) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <voice> from message object");
            TELEBOT_SAFE_FREE(msg->voice);
        }
    }

    struct json_object *video_note = NULL;
    if (json_object_object_get_ex(obj, "video_note", &video_note))
    {
        msg->video_note = malloc(sizeof(telebot_video_note_t));
        if (telebot_parser_get_video_note(video_note, msg->video_note) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <video_note> from message object");
            TELEBOT_SAFE_FREE(msg->video_note);
        }
    }

    struct json_object *caption = NULL;
    if (json_object_object_get_ex(obj, "caption", &caption))
        msg->caption = TELEBOT_SAFE_STRDUP(json_object_get_string(caption));

    struct json_object *contact = NULL;
    if (json_object_object_get_ex(obj, "contact", &contact))
    {
        msg->contact = malloc(sizeof(telebot_contact_t));
        if (telebot_parser_get_contact(contact, msg->contact) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <contact> from message object");
            TELEBOT_SAFE_FREE(msg->contact);
        }
    }

    struct json_object *location = NULL;
    if (json_object_object_get_ex(obj, "location", &location))
    {
        msg->location = malloc(sizeof(telebot_location_t));
        if (telebot_parser_get_location(location, msg->location) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <location> from message object");
            TELEBOT_SAFE_FREE(msg->location);
        }
    }

    struct json_object *venue = NULL;
    if (json_object_object_get_ex(obj, "venue", &venue))
    {
        msg->venue = malloc(sizeof(telebot_venue_t));
        if (telebot_parser_get_venue(venue, msg->venue) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <venue> from message object");
            TELEBOT_SAFE_FREE(msg->venue);
        }
    }

    struct json_object *poll = NULL;
    if (json_object_object_get_ex(obj, "poll", &poll))
    {
        msg->poll = malloc(sizeof(telebot_poll_t));
        if (telebot_parser_get_poll(poll, msg->poll) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <poll> from message object");
            TELEBOT_SAFE_FREE(msg->poll);
        }
    }

    struct json_object *dice = NULL;
    if (json_object_object_get_ex(obj, "dice", &dice))
    {
        msg->dice = malloc(sizeof(telebot_dice_t));
        if (telebot_parser_get_dice(dice, msg->dice) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <dice> from message object");
            TELEBOT_SAFE_FREE(msg->dice);
        }
    }

    struct json_object *ncm = NULL;
    if (json_object_object_get_ex(obj, "new_chat_members", &ncm))
    {
        if (telebot_parser_get_users(ncm, &(msg->new_chat_members), &(msg->count_new_chat_members)) != TELEBOT_ERROR_NONE)
            ERR("Failed to get <new_chat_members> from message object");
    }

    struct json_object *lcm = NULL;
    if (json_object_object_get_ex(obj, "left_chat_members", &lcm))
    {
        if (telebot_parser_get_users(lcm, &(msg->left_chat_members), &(msg->count_left_chat_members)) !=
            TELEBOT_ERROR_NONE)
            ERR("Failed to get <left_chat_members> from message object");
    }

    struct json_object *nct = NULL;
    if (json_object_object_get_ex(obj, "new_chat_title", &nct))
        msg->new_chat_title = TELEBOT_SAFE_STRDUP(json_object_get_string(nct));

    struct json_object *new_chat_photo = NULL;
    if (json_object_object_get_ex(obj, "new_chat_photo", &new_chat_photo))
    {
        if (telebot_parser_get_photos(new_chat_photo, &(msg->new_chat_photos), &(msg->count_new_chat_photos)) !=
            TELEBOT_ERROR_NONE)
            ERR("Failed to get <new_chat_photo> from message object");
    }

    struct json_object *del_chat_photo = NULL;
    if (json_object_object_get_ex(obj, "delete_chat_photo", &del_chat_photo))
        msg->delete_chat_photo = json_object_get_boolean(del_chat_photo);

    struct json_object *gcc = NULL;
    if (json_object_object_get_ex(obj, "group_chat_created", &gcc))
        msg->group_chat_created = json_object_get_boolean(gcc);

    struct json_object *sgcc = NULL;
    if (json_object_object_get_ex(obj, "supergroup_chat_created", &sgcc))
        msg->supergroup_chat_created = json_object_get_boolean(sgcc);

    struct json_object *cacc = NULL;
    if (json_object_object_get_ex(obj, "channel_chat_created", &cacc))
        msg->channel_chat_created = json_object_get_boolean(cacc);

    struct json_object *mtci = NULL;
    if (json_object_object_get_ex(obj, "migrate_to_chat_id", &mtci))
        msg->migrate_to_chat_id = json_object_get_int64(mtci);

    struct json_object *mftci = NULL;
    if (json_object_object_get_ex(obj, "migrate_from_chat_id", &mftci))
        msg->migrate_from_chat_id = json_object_get_int64(mftci);

    struct json_object *pinned_message = NULL;
    if (json_object_object_get_ex(obj, "pinned_message", &pinned_message))
    {
        msg->pinned_message = malloc(sizeof(telebot_message_t));
        if (telebot_parser_get_message(pinned_message, msg->pinned_message) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <pinned_message> from message object");
            TELEBOT_SAFE_FREE(msg->pinned_message);
        }
    }

    struct json_object *connected_website = NULL;
    if (json_object_object_get_ex(obj, "connected_website", &connected_website))
        msg->connected_website = TELEBOT_SAFE_STRDUP(json_object_get_string(connected_website));

    //TODO: implement invoce, successful_payment, passport_data, reply_markup

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_message_entity(struct json_object *obj, telebot_message_entity_t *entity)
{
    if ((obj == NULL) || (entity == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(entity, 0, sizeof(telebot_message_entity_t));
    struct json_object *type = NULL;
    if (!json_object_object_get_ex(obj, "type", &type))
    {
        ERR("Object is not message entity type, type not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    entity->type = TELEBOT_SAFE_STRDUP(json_object_get_string(type));

    struct json_object *offset = NULL;
    if (!json_object_object_get_ex(obj, "offset", &offset))
    {
        TELEBOT_SAFE_FREE(entity->type);
        ERR("Object is not message entity type, offset not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    entity->offset = json_object_get_int(offset);

    struct json_object *length = NULL;
    if (!json_object_object_get_ex(obj, "length", &length))
    {
        ERR("Object is not message entity type, length not found");
        TELEBOT_SAFE_FZCNT(entity->type, entity->offset);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    entity->length = json_object_get_int(length);

    struct json_object *url = NULL;
    if (json_object_object_get_ex(obj, "url", &url))
        entity->url = TELEBOT_SAFE_STRDUP(json_object_get_string(url));

    struct json_object *user = NULL;
    if (json_object_object_get_ex(obj, "user", &user))
    {
        entity->user = malloc(sizeof(telebot_user_t));
        if (telebot_parser_get_user(user, entity->user) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <user> from message entity object");
            TELEBOT_SAFE_FREE(entity->user);
        }
    }

    struct json_object *language = NULL;
    if (json_object_object_get_ex(obj, "language", &language))
        entity->language = TELEBOT_SAFE_STRDUP(json_object_get_string(language));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_message_entities(struct json_object *obj, telebot_message_entity_t **entities, int *count)
{
    struct json_object *array = obj;
    int array_len = json_object_array_length(array);
    if (array_len == 0)
        return TELEBOT_ERROR_OPERATION_FAILED;

    telebot_message_entity_t *result = calloc(array_len, sizeof(telebot_message_entity_t));
    if (result == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    *count = array_len;
    *entities = result;

    for (int index = 0; index < array_len; index++)
    {
        struct json_object *item = json_object_array_get_idx(array, index);
        if (telebot_parser_get_message_entity(item, &(result[index])) != TELEBOT_ERROR_NONE)
            ERR("Failed to parse user from users array");
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_photo(struct json_object *obj, telebot_photo_t *photo)
{
    if ((obj == NULL) || (photo == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(photo, 0, sizeof(telebot_photo_t));
    struct json_object *file_id = NULL;
    if (!json_object_object_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not photo size type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->file_id = TELEBOT_SAFE_STRDUP(json_object_get_string(file_id));

    struct json_object *width = NULL;
    if (!json_object_object_get_ex(obj, "width", &width))
    {
        ERR("Object is not photo size type, width not found");
        TELEBOT_SAFE_FREE(photo->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->width = json_object_get_int(width);

    struct json_object *height = NULL;
    if (!json_object_object_get_ex(obj, "height", &height))
    {
        ERR("Object is not photo size type, height not found");
        TELEBOT_SAFE_FZCNT(photo->file_id, photo->width);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->height = json_object_get_int(height);

    struct json_object *file_size = NULL;
    if (json_object_object_get_ex(obj, "file_size", &file_size))
        photo->file_size = json_object_get_int(file_size);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_photos(struct json_object *obj, telebot_photo_t **photos, int *count)
{
    struct json_object *array = obj;
    int array_len = json_object_array_length(array);
    if (array_len == 0)
        return TELEBOT_ERROR_OPERATION_FAILED;

    telebot_photo_t *result = calloc(array_len, sizeof(telebot_photo_t));
    if (result == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    *count = array_len;
    *photos = result;

    for (int index = 0; index < array_len; index++)
    {
        struct json_object *item = json_object_array_get_idx(array, index);
        if (telebot_parser_get_photo(item, &(result[index])) != TELEBOT_ERROR_NONE)
            ERR("Failed to parse user from users array");
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_audio(struct json_object *obj, telebot_audio_t *audio)
{
    if ((obj == NULL) || (audio == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(audio, 0, sizeof(telebot_audio_t));
    struct json_object *file_id = NULL;
    if (!json_object_object_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not audio type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    audio->file_id = TELEBOT_SAFE_STRDUP(json_object_get_string(file_id));

    struct json_object *file_unique_id = NULL;
    if (!json_object_object_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not audio type, file_unique_id not found");
        TELEBOT_SAFE_FREE(audio->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    audio->file_unique_id = TELEBOT_SAFE_STRDUP(json_object_get_string(file_unique_id));

    struct json_object *duration = NULL;
    if (!json_object_object_get_ex(obj, "duration", &duration))
    {
        ERR("Object is not audio type, duration not found");
        TELEBOT_SAFE_FREE(audio->file_id);
        TELEBOT_SAFE_FREE(audio->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    audio->duration = json_object_get_int(duration);

    struct json_object *performer = NULL;
    if (json_object_object_get_ex(obj, "performer", &performer))
        audio->performer = TELEBOT_SAFE_STRDUP(json_object_get_string(performer));

    struct json_object *title = NULL;
    if (json_object_object_get_ex(obj, "title", &title))
        audio->title = TELEBOT_SAFE_STRDUP(json_object_get_string(title));

    struct json_object *mime_type = NULL;
    if (json_object_object_get_ex(obj, "mime_type", &mime_type))
        audio->mime_type = TELEBOT_SAFE_STRDUP(json_object_get_string(mime_type));

    struct json_object *file_size = NULL;
    if (json_object_object_get_ex(obj, "file_size", &file_size))
        audio->file_size = json_object_get_int(file_size);

    struct json_object *thumb;
    if (json_object_object_get_ex(obj, "thumb", &thumb))
    {
        audio->thumb = malloc(sizeof(telebot_photo_t));
        if (telebot_parser_get_photo(thumb, audio->thumb) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <thumb> from audio object");
            TELEBOT_SAFE_FREE(audio->thumb);
        }
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_document(struct json_object *obj, telebot_document_t *document)
{
    if ((obj == NULL) || (document == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(document, 0, sizeof(telebot_document_t));
    struct json_object *file_id = NULL;
    if (!json_object_object_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not document type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    document->file_id = TELEBOT_SAFE_STRDUP(json_object_get_string(file_id));

    struct json_object *file_unique_id = NULL;
    if (!json_object_object_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not document type, file_unique_id not found");
        TELEBOT_SAFE_FREE(document->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    document->file_unique_id = TELEBOT_SAFE_STRDUP(json_object_get_string(file_unique_id));

    struct json_object *thumb = NULL;
    if (json_object_object_get_ex(obj, "thumb", &thumb))
    {
        document->thumb = malloc(sizeof(telebot_photo_t));
        if (telebot_parser_get_photo(thumb, document->thumb) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <thumb> from document object");
            TELEBOT_SAFE_FREE(document->thumb);
        }
    }

    struct json_object *file_name = NULL;
    if (json_object_object_get_ex(obj, "file_name", &file_name))
        document->file_name = TELEBOT_SAFE_STRDUP(json_object_get_string(file_name));

    struct json_object *mime_type = NULL;
    if (json_object_object_get_ex(obj, "mime_type", &mime_type))
        document->mime_type = TELEBOT_SAFE_STRDUP(json_object_get_string(mime_type));

    struct json_object *file_size = NULL;
    if (json_object_object_get_ex(obj, "file_size", &file_size))
        document->file_size = json_object_get_int(file_size);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_video(struct json_object *obj, telebot_video_t *video)
{
    if ((obj == NULL) || (video == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(video, 0, sizeof(telebot_video_t));
    struct json_object *file_id = NULL;
    if (!json_object_object_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not video type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video->file_id = TELEBOT_SAFE_STRDUP(json_object_get_string(file_id));

    struct json_object *file_unique_id = NULL;
    if (!json_object_object_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not video type, file_unique_id not found");
        TELEBOT_SAFE_FREE(video->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video->file_unique_id = TELEBOT_SAFE_STRDUP(json_object_get_string(file_unique_id));

    struct json_object *width = NULL;
    if (!json_object_object_get_ex(obj, "width", &width))
    {
        ERR("Object is not video type, width not found");
        TELEBOT_SAFE_FREE(video->file_id);
        TELEBOT_SAFE_FREE(video->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video->width = json_object_get_int(width);

    struct json_object *height = NULL;
    if (!json_object_object_get_ex(obj, "height", &height))
    {
        ERR("Object is not video type, height not found");
        TELEBOT_SAFE_FREE(video->file_id);
        TELEBOT_SAFE_FREE(video->file_unique_id);
        video->width = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video->height = json_object_get_int(height);

    struct json_object *duration = NULL;
    if (!json_object_object_get_ex(obj, "duration", &duration))
    {
        ERR("Object is not video type, duration not found");
        TELEBOT_SAFE_FREE(video->file_id);
        TELEBOT_SAFE_FREE(video->file_unique_id);
        video->width = 0;
        video->height = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video->duration = json_object_get_int(duration);

    struct json_object *thumb = NULL;
    if (json_object_object_get_ex(obj, "thumb", &thumb))
    {
        video->thumb = malloc(sizeof(telebot_photo_t));
        if (telebot_parser_get_photo(thumb, video->thumb) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <thumb> from video object");
            TELEBOT_SAFE_FREE(video->thumb);
        }
    }

    struct json_object *mime_type = NULL;
    if (json_object_object_get_ex(obj, "mime_type", &mime_type))
        video->mime_type = TELEBOT_SAFE_STRDUP(json_object_get_string(mime_type));

    struct json_object *file_size = NULL;
    if (json_object_object_get_ex(obj, "file_size", &file_size))
        video->file_size = json_object_get_int(file_size);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_animation(struct json_object *obj, telebot_animation_t *animation)
{
    if ((obj == NULL) || (animation == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(animation, 0, sizeof(telebot_animation_t));
    struct json_object *file_id = NULL;
    if (!json_object_object_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not animation type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    animation->file_id = TELEBOT_SAFE_STRDUP(json_object_get_string(file_id));

    struct json_object *file_unique_id = NULL;
    if (!json_object_object_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not animation type, file_unique_id not found");
        TELEBOT_SAFE_FREE(animation->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    animation->file_unique_id = TELEBOT_SAFE_STRDUP(json_object_get_string(file_unique_id));

    struct json_object *width = NULL;
    if (!json_object_object_get_ex(obj, "width", &width))
    {
        ERR("Object is not animation type, width not found");
        TELEBOT_SAFE_FREE(animation->file_id);
        TELEBOT_SAFE_FREE(animation->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    animation->width = json_object_get_int(width);

    struct json_object *height = NULL;
    if (!json_object_object_get_ex(obj, "height", &height))
    {
        ERR("Object is not animation type, height not found");
        TELEBOT_SAFE_FREE(animation->file_id);
        TELEBOT_SAFE_FREE(animation->file_unique_id);
        animation->width = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    animation->height = json_object_get_int(height);

    struct json_object *duration = NULL;
    if (!json_object_object_get_ex(obj, "duration", &duration))
    {
        ERR("Object is not animation type, duration not found");
        TELEBOT_SAFE_FREE(animation->file_id);
        TELEBOT_SAFE_FREE(animation->file_unique_id);
        animation->width = 0;
        animation->height = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    animation->duration = json_object_get_int(duration);

    struct json_object *thumb = NULL;
    if (json_object_object_get_ex(obj, "thumb", &thumb))
    {
        animation->thumb = malloc(sizeof(telebot_photo_t));
        if (telebot_parser_get_photo(thumb, animation->thumb) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <thumb> from animation object");
            TELEBOT_SAFE_FREE(animation->thumb);
        }
    }

    struct json_object *mime_type = NULL;
    if (json_object_object_get_ex(obj, "mime_type", &mime_type))
        animation->mime_type = TELEBOT_SAFE_STRDUP(json_object_get_string(mime_type));

    struct json_object *file_size = NULL;
    if (json_object_object_get_ex(obj, "file_size", &file_size))
        animation->file_size = json_object_get_int(file_size);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_video_note(struct json_object *obj, telebot_video_note_t *video_note)
{
    if ((obj == NULL) || (video_note == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(video_note, 0, sizeof(telebot_video_note_t));
    struct json_object *file_id = NULL;
    if (!json_object_object_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not video note type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video_note->file_id = TELEBOT_SAFE_STRDUP(json_object_get_string(file_id));

    struct json_object *file_unique_id = NULL;
    if (!json_object_object_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not video_note type, file_unique_id not found");
        TELEBOT_SAFE_FREE(video_note->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video_note->file_unique_id = TELEBOT_SAFE_STRDUP(json_object_get_string(file_unique_id));

    struct json_object *length = NULL;
    if (!json_object_object_get_ex(obj, "length", &length))
    {
        ERR("Object is not video note type, length not found");
        TELEBOT_SAFE_FREE(video_note->file_id);
        TELEBOT_SAFE_FREE(video_note->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video_note->length = json_object_get_int(length);

    struct json_object *duration = NULL;
    if (!json_object_object_get_ex(obj, "duration", &duration))
    {
        ERR("Object is not video note type, duration not found");
        TELEBOT_SAFE_FREE(video_note->file_id);
        TELEBOT_SAFE_FREE(video_note->file_unique_id);
        video_note->length = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video_note->duration = json_object_get_int(duration);

    struct json_object *thumb;
    if (json_object_object_get_ex(obj, "thumb", &thumb))
    {
        video_note->thumb = malloc(sizeof(telebot_photo_t));
        if (telebot_parser_get_photo(thumb, video_note->thumb) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <thumb> from video note object");
            TELEBOT_SAFE_FREE(video_note->thumb);
        }
    }

    struct json_object *file_size = NULL;
    if (json_object_object_get_ex(obj, "file_size", &file_size))
        video_note->file_size = json_object_get_int(file_size);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_voice(struct json_object *obj, telebot_voice_t *voice)
{
    if ((obj == NULL) || (voice == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(voice, 0, sizeof(telebot_voice_t));
    struct json_object *file_id = NULL;
    if (!json_object_object_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not voice type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    voice->file_id = TELEBOT_SAFE_STRDUP(json_object_get_string(file_id));

    struct json_object *file_unique_id = NULL;
    if (!json_object_object_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not voice type, file_unique_id not found");
        TELEBOT_SAFE_FREE(voice->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    voice->file_unique_id = TELEBOT_SAFE_STRDUP(json_object_get_string(file_unique_id));

    struct json_object *duration = NULL;
    if (!json_object_object_get_ex(obj, "duration", &duration))
    {
        ERR("Object is not voice type, voice duration not found");
        TELEBOT_SAFE_FREE(voice->file_id);
        TELEBOT_SAFE_FREE(voice->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    voice->duration = json_object_get_int(duration);

    struct json_object *mime_type = NULL;
    if (json_object_object_get_ex(obj, "mime_type", &mime_type))
        voice->mime_type = TELEBOT_SAFE_STRDUP(json_object_get_string(mime_type));

    struct json_object *file_size = NULL;
    if (json_object_object_get_ex(obj, "file_size", &file_size))
        voice->file_size = json_object_get_int(file_size);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_contact(struct json_object *obj, telebot_contact_t *contact)
{
    if ((obj == NULL) || (contact == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(contact, 0, sizeof(telebot_contact_t));
    struct json_object *phone_number = NULL;
    if (!json_object_object_get_ex(obj, "phone_number", &phone_number))
    {
        ERR("Object is not contact type, phone number not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    contact->phone_number = TELEBOT_SAFE_STRDUP(json_object_get_string(phone_number));

    struct json_object *first_name = NULL;
    if (!json_object_object_get_ex(obj, "first_name", &first_name))
    {
        ERR("Object is not contact type, first name not found");
        TELEBOT_SAFE_FREE(contact->phone_number);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    contact->first_name = TELEBOT_SAFE_STRDUP(json_object_get_string(first_name));

    struct json_object *last_name = NULL;
    if (json_object_object_get_ex(obj, "last_name", &last_name))
        contact->last_name = TELEBOT_SAFE_STRDUP(json_object_get_string(last_name));

    struct json_object *user_id = NULL;
    if (json_object_object_get_ex(obj, "user_id", &user_id))
        contact->user_id = json_object_get_int(user_id);

    struct json_object *vcard = NULL;
    if (json_object_object_get_ex(obj, "vcard", &vcard))
        contact->vcard = TELEBOT_SAFE_STRDUP(json_object_get_string(vcard));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_location(struct json_object *obj, telebot_location_t *location)
{
    if ((obj == NULL) || (location == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(location, 0, sizeof(telebot_location_t));
    struct json_object *latitude = NULL;
    if (!json_object_object_get_ex(obj, "latitude", &latitude))
    {
        ERR("Object is not location type, latitude not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    location->latitude = json_object_get_double(latitude);

    struct json_object *longitude = NULL;
    if (!json_object_object_get_ex(obj, "longitude", &longitude))
    {
        ERR("Object is not location type, longitude not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    location->longitude = json_object_get_double(longitude);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_venue(struct json_object *obj, telebot_venue_t *venue)
{
    if ((obj == NULL) || (venue == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(venue, 0, sizeof(telebot_venue_t));
    struct json_object *title = NULL;
    if (!json_object_object_get_ex(obj, "title", &title))
    {
        ERR("Object is not venue type, title not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    venue->title = TELEBOT_SAFE_STRDUP(json_object_get_string(title));

    struct json_object *address = NULL;
    if (!json_object_object_get_ex(obj, "address", &address))
    {
        ERR("Object is not venue type, address not found");
        TELEBOT_SAFE_FREE(venue->title);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    venue->address = TELEBOT_SAFE_STRDUP(json_object_get_string(address));

    struct json_object *location = NULL;
    if (json_object_object_get_ex(obj, "location", &location))
    {
        venue->location = malloc(sizeof(telebot_location_t));
        if (telebot_parser_get_location(location, venue->location) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <location> from venue object");
            TELEBOT_SAFE_FREE(venue->location);
            TELEBOT_SAFE_FREE(venue->title);
            TELEBOT_SAFE_FREE(venue->address);
            return TELEBOT_ERROR_OPERATION_FAILED;
        }
    }
    else
    {
        ERR("Object is not venue type, location not found");
        TELEBOT_SAFE_FREE(venue->title);
        TELEBOT_SAFE_FREE(venue->address);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *foursquare_id = NULL;
    if (json_object_object_get_ex(obj, "foursquare_id", &foursquare_id))
        venue->foursquare_id = TELEBOT_SAFE_STRDUP(json_object_get_string(foursquare_id));

    struct json_object *foursquare_type = NULL;
    if (json_object_object_get_ex(obj, "foursquare_type", &foursquare_type))
        venue->foursquare_type = TELEBOT_SAFE_STRDUP(json_object_get_string(foursquare_type));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_poll_option(struct json_object *obj, telebot_poll_option_t *option)
{
    if ((obj == NULL) || (option == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(option, 0, sizeof(telebot_poll_option_t));
    struct json_object *text = NULL;
    if (!json_object_object_get_ex(obj, "text", &text))
    {
        ERR("Object is not poll option type, text not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    option->text = TELEBOT_SAFE_STRDUP(json_object_get_string(text));

    struct json_object *voter_count = NULL;
    if (!json_object_object_get_ex(obj, "voter_count", &voter_count))
    {
        ERR("Object is not pol option type, voter_count not found");
        TELEBOT_SAFE_FREE(option->text);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    option->voter_count = json_object_get_int(voter_count);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_poll_answer(struct json_object *obj, telebot_poll_answer_t *answer)
{
    if ((obj == NULL) || (answer == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(answer, 0, sizeof(telebot_poll_answer_t));
    struct json_object *poll_id = NULL;
    if (!json_object_object_get_ex(obj, "poll_id", &poll_id))
    {
        ERR("Object is not poll answer type, poll_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    answer->poll_id = TELEBOT_SAFE_STRDUP(json_object_get_string(poll_id));

    struct json_object *option_ids = NULL;
    if (!json_object_object_get_ex(obj, "option_ids", &option_ids))
    {
        ERR("Object is not pol answer type, option_ids not found");
        TELEBOT_SAFE_FREE(answer->poll_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    answer->count_option_ids = json_object_array_length(option_ids);
    if (answer->count_option_ids == 0)
    {
        ERR("Object is not pol answer type, option_ids is not array");
        TELEBOT_SAFE_FREE(answer->poll_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    answer->option_ids = calloc(answer->count_option_ids, sizeof(*(answer->option_ids)));
    if (answer->option_ids == NULL)
    {
        ERR("Failed to allocate memory for option_ids");
        TELEBOT_SAFE_FREE(answer->poll_id);
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    for (int index = 0; index < answer->count_option_ids; index++)
        answer->option_ids[index] = json_object_get_int(json_object_array_get_idx(option_ids, index));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_poll(struct json_object *obj, telebot_poll_t *poll)
{
    if ((obj == NULL) || (poll == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(poll, 0, sizeof(telebot_poll_t));
    struct json_object *id = NULL;
    if (!json_object_object_get_ex(obj, "id", &id))
    {
        ERR("Object is not poll type, id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->id = TELEBOT_SAFE_STRDUP(json_object_get_string(id));

    struct json_object *question = NULL;
    if (!json_object_object_get_ex(obj, "question", &question))
    {
        ERR("Object is not pol type, question not found");
        TELEBOT_SAFE_FREE(poll->id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->question = TELEBOT_SAFE_STRDUP(json_object_get_string(question));

    struct json_object *total_voter_count = NULL;
    if (!json_object_object_get_ex(obj, "total_voter_count", &total_voter_count))
    {
        ERR("Object is not pol type, total_voter_count not found");
        TELEBOT_SAFE_FREE(poll->id);
        TELEBOT_SAFE_FREE(poll->question);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->total_voter_count = json_object_get_int(total_voter_count);

    struct json_object *is_closed = NULL;
    if (!json_object_object_get_ex(obj, "is_closed", &is_closed))
    {
        ERR("Object is not pol type, is_closed not found");
        TELEBOT_SAFE_FREE(poll->id);
        TELEBOT_SAFE_FREE(poll->question);
        poll->total_voter_count = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->is_closed = json_object_get_boolean(is_closed);

    struct json_object *is_anonymous = NULL;
    if (!json_object_object_get_ex(obj, "is_anonymous", &is_anonymous))
    {
        ERR("Object is not pol type, is_anonymous not found");
        TELEBOT_SAFE_FREE(poll->id);
        TELEBOT_SAFE_FREE(poll->question);
        poll->total_voter_count = 0;
        poll->is_closed = false;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->is_anonymous = json_object_get_boolean(is_anonymous);

    struct json_object *type = NULL;
    if (!json_object_object_get_ex(obj, "type", &type))
    {
        ERR("Object is not pol type, type not found");
        TELEBOT_SAFE_FREE(poll->id);
        TELEBOT_SAFE_FREE(poll->question);
        poll->total_voter_count = 0;
        poll->is_closed = false;
        poll->is_anonymous = false;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->type = TELEBOT_SAFE_STRDUP(json_object_get_string(type));

    struct json_object *allows_multiple_answers = NULL;
    if (!json_object_object_get_ex(obj, "allows_multiple_answers", &allows_multiple_answers))
    {
        ERR("Object is not pol type, allows_multiple_answers not found");
        TELEBOT_SAFE_FREE(poll->id);
        TELEBOT_SAFE_FREE(poll->question);
        poll->total_voter_count = 0;
        poll->is_closed = false;
        poll->is_anonymous = false;
        TELEBOT_SAFE_FREE(poll->type);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->allows_multiple_answers = json_object_get_boolean(allows_multiple_answers);

    struct json_object *options = NULL;
    if (!json_object_object_get_ex(obj, "options", &options))
    {
        ERR("Object is not pol type, options not found");
        TELEBOT_SAFE_FREE(poll->id);
        TELEBOT_SAFE_FREE(poll->question);
        poll->total_voter_count = 0;
        poll->is_closed = false;
        poll->is_anonymous = false;
        TELEBOT_SAFE_FREE(poll->type);
        poll->allows_multiple_answers = false;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    poll->count_options = json_object_array_length(options);
    if (poll->count_options == 0)
    {
        ERR("Object is not pol type, options is not array");
        TELEBOT_SAFE_FREE(poll->id);
        TELEBOT_SAFE_FREE(poll->question);
        poll->total_voter_count = 0;
        poll->is_closed = false;
        poll->is_anonymous = false;
        TELEBOT_SAFE_FREE(poll->type);
        poll->allows_multiple_answers = false;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    poll->options = calloc(poll->count_options, sizeof(telebot_poll_option_t));
    if (poll->options == NULL)
    {
        ERR("Failed to allocate memory for option_ids");
        TELEBOT_SAFE_FREE(poll->id);
        TELEBOT_SAFE_FREE(poll->question);
        poll->total_voter_count = 0;
        poll->is_closed = false;
        poll->is_anonymous = false;
        poll->allows_multiple_answers = false;
        TELEBOT_SAFE_FREE(poll->type);
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    for (int index = 0; index < poll->count_options; index++)
        telebot_parser_get_poll_option(json_object_array_get_idx(options, index), &(poll->options[index]));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_dice(struct json_object *obj, telebot_dice_t *dice)
{
    if ((obj == NULL) || (dice == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(dice, 0, sizeof(telebot_location_t));
    struct json_object *value = NULL;
    if (!json_object_object_get_ex(obj, "value", &value))
    {
        ERR("Object is not dice type, value not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    dice->value = json_object_get_int(value);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user_profile_photos(struct json_object *obj, telebot_user_profile_photos_t *photos)
{
    if ((obj == NULL) || (photos == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *total_count_obj;
    if (!json_object_object_get_ex(obj, "total_count", &total_count_obj))
    {
        ERR("Object is not user profile photo type, total_count not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photos->total_count = json_object_get_int(total_count_obj);
    if (photos->total_count == 0)
        return TELEBOT_ERROR_NONE;

    struct json_object *array;
    if (!json_object_object_get_ex(obj, "photos", &array))
    {
        ERR("Failed to get photos from <UserProfilePhotos> object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    int total = json_object_array_length(array);
    for (int i = 0; i < 4; i++)
        photos->photos[i] = calloc(total, sizeof(telebot_photo_t));
    photos->current_count = total;

    telebot_error_e ret = TELEBOT_ERROR_NONE;
    for (int i = 0; i < total; i++)
    {
        struct json_object *item = json_object_array_get_idx(array, i);
        int subtotal = json_object_array_length(item);
        if (subtotal > 4)
            subtotal = 4; /* This MUST not happen */
        for (int j = 0; j < subtotal; j++)
        {
            struct json_object *photo = json_object_array_get_idx(item, j);
            ret |= telebot_parser_get_photo(photo, &(photos->photos[j][i]));
        }
    }

    if (ret != TELEBOT_ERROR_NONE)
    {
        for (int i = 0; i < 4; i++)
        {
            TELEBOT_SAFE_FREE(photos->photos[i]);
        }
        photos->current_count = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_file(struct json_object *obj, telebot_file_t *file)
{
    if ((obj == NULL) || (file == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *file_id = NULL;
    if (!json_object_object_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not file type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    file->file_id = TELEBOT_SAFE_STRDUP(json_object_get_string(file_id));

    struct json_object *file_unique_id = NULL;
    if (!json_object_object_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not file type, file_unique_id not found");
        TELEBOT_SAFE_FREE(file->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    file->file_unique_id = TELEBOT_SAFE_STRDUP(json_object_get_string(file_unique_id));

    struct json_object *file_path = NULL;
    if (!json_object_object_get_ex(obj, "file_path", &file_path))
    {
        ERR("Object is not file type, file_path not found");
        TELEBOT_SAFE_FREE(file->file_id);
        TELEBOT_SAFE_FREE(file->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    file->file_path = TELEBOT_SAFE_STRDUP(json_object_get_string(file_path));

    struct json_object *file_size;
    if (json_object_object_get_ex(obj, "file_size", &file_size))
        file->file_size = json_object_get_int(file_size);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_callback_query(struct json_object *obj, telebot_callback_query_t *cb_query)
{
    if ((obj == NULL) || (cb_query == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *id = NULL;
    if (!json_object_object_get_ex(obj, "id", &id))
    {
        ERR("Failed to get <id> from callback_query object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    cb_query->id = TELEBOT_SAFE_STRDUP(json_object_get_string(id));

    struct json_object *from = NULL;
    if (json_object_object_get_ex(obj, "from", &from))
    {
        cb_query->from = malloc(sizeof(telebot_user_t));
        if (telebot_parser_get_user(from, cb_query->from) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <from> from callback_query object");
            TELEBOT_SAFE_FREE(cb_query->id);
            TELEBOT_SAFE_FREE(cb_query->from);
            return TELEBOT_ERROR_OPERATION_FAILED;
        }
    }
    else
    {
        ERR("Object is not chat member type, user not found");
        TELEBOT_SAFE_FREE(cb_query->id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *message = NULL;
    if (json_object_object_get_ex(obj, "message", &message))
    {
        cb_query->message = malloc(sizeof(telebot_message_t));
        if (telebot_parser_get_message(message, cb_query->message) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <message> from callback_query object");
            TELEBOT_SAFE_FREE(cb_query->message);
        }
    }

    struct json_object *inline_message_id = NULL;
    if (json_object_object_get_ex(obj, "inline_message_id", &inline_message_id))
        cb_query->inline_message_id = TELEBOT_SAFE_STRDUP(json_object_get_string(inline_message_id));

    struct json_object *chat_instance = NULL;
    if (json_object_object_get_ex(obj, "chat_instance", &chat_instance))
        cb_query->chat_instance = TELEBOT_SAFE_STRDUP(json_object_get_string(chat_instance));

    struct json_object *data = NULL;
    if (json_object_object_get_ex(obj, "data", &data))
        cb_query->data = TELEBOT_SAFE_STRDUP(json_object_get_string(data));

    struct json_object *game_short_name = NULL;
    if (json_object_object_get_ex(obj, "game_short_name", &game_short_name))
        cb_query->game_short_name = TELEBOT_SAFE_STRDUP(json_object_get_string(game_short_name));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_photo(struct json_object *obj, telebot_chat_photo_t *photo)
{
    if ((obj == NULL) || (photo == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(photo, 0, sizeof(telebot_chat_photo_t));
    struct json_object *small_file_id = NULL;
    if (!json_object_object_get_ex(obj, "small_file_id", &small_file_id))
    {
        ERR("Object is not chat photo type, small_file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->small_file_id = TELEBOT_SAFE_STRDUP(json_object_get_string(small_file_id));

    struct json_object *small_file_unique_id = NULL;
    if (!json_object_object_get_ex(obj, "small_file_unique_id", &small_file_unique_id))
    {
        ERR("Object is not chat photo type, small_file_unique_id not found");
        TELEBOT_SAFE_FREE(photo->small_file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->small_file_unique_id = TELEBOT_SAFE_STRDUP(json_object_get_string(small_file_unique_id));

    struct json_object *big_file_id = NULL;
    if (!json_object_object_get_ex(obj, "big_file_id", &big_file_id))
    {
        ERR("Object is not chat photo type, big_file_id not found");
        TELEBOT_SAFE_FREE(photo->small_file_id);
        TELEBOT_SAFE_FREE(photo->small_file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->big_file_id = TELEBOT_SAFE_STRDUP(json_object_get_string(big_file_id));

    struct json_object *big_file_unique_id = NULL;
    if (!json_object_object_get_ex(obj, "big_file_unique_id", &big_file_unique_id))
    {
        ERR("Object is not chat photo type, big_file_unique_id not found");
        TELEBOT_SAFE_FREE(photo->small_file_id);
        TELEBOT_SAFE_FREE(photo->small_file_unique_id);
        TELEBOT_SAFE_FREE(photo->big_file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->big_file_unique_id = TELEBOT_SAFE_STRDUP(json_object_get_string(big_file_unique_id));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_member(struct json_object *obj, telebot_chat_member_t *member)
{
    if ((obj == NULL) || (member == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(member, 0, sizeof(telebot_chat_member_t));
    struct json_object *status = NULL;
    if (!json_object_object_get_ex(obj, "status", &status))
    {
        ERR("Object is not chat member type, status not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    member->status = TELEBOT_SAFE_STRDUP(json_object_get_string(status));

    struct json_object *user = NULL;
    if (json_object_object_get_ex(obj, "user", &user))
    {
        member->user = malloc(sizeof(telebot_user_t));
        if (telebot_parser_get_user(user, member->user) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <user> from chat member object");
            TELEBOT_SAFE_FREE(member->status);
            TELEBOT_SAFE_FREE(member->user);
            return TELEBOT_ERROR_OPERATION_FAILED;
        }
    }
    else
    {
        ERR("Object is not chat member type, user not found");
        TELEBOT_SAFE_FREE(member->status);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *custom_title = NULL;
    if (json_object_object_get_ex(obj, "custom_title", &custom_title))
        member->custom_title = TELEBOT_SAFE_STRDUP(json_object_get_string(custom_title));

    struct json_object *until_date = NULL;
    if (json_object_object_get_ex(obj, "until_date", &until_date))
        member->until_date = json_object_get_int(until_date);

    struct json_object *can_be_edited = NULL;
    if (json_object_object_get_ex(obj, "can_be_edited", &can_be_edited))
        member->can_be_edited = json_object_get_boolean(can_be_edited);

    struct json_object *can_post_messages = NULL;
    if (json_object_object_get_ex(obj, "can_post_messages", &can_post_messages))
        member->can_post_messages = json_object_get_boolean(can_post_messages);

    struct json_object *can_edit_messages = NULL;
    if (json_object_object_get_ex(obj, "can_edit_messages", &can_edit_messages))
        member->can_edit_messages = json_object_get_boolean(can_edit_messages);

    struct json_object *can_delete_messages = NULL;
    if (json_object_object_get_ex(obj, "can_delete_messages", &can_delete_messages))
        member->can_delete_messages = json_object_get_boolean(can_delete_messages);

    struct json_object *can_restrict_members = NULL;
    if (json_object_object_get_ex(obj, "can_restrict_members", &can_restrict_members))
        member->can_restrict_members = json_object_get_boolean(can_restrict_members);

    struct json_object *can_promote_members = NULL;
    if (json_object_object_get_ex(obj, "can_promote_members", &can_promote_members))
        member->can_promote_members = json_object_get_boolean(can_promote_members);

    struct json_object *can_change_info = NULL;
    if (json_object_object_get_ex(obj, "can_change_info", &can_change_info))
        member->can_change_info = json_object_get_boolean(can_change_info);

    struct json_object *can_invite_users = NULL;
    if (json_object_object_get_ex(obj, "can_invite_users", &can_invite_users))
        member->can_invite_users = json_object_get_boolean(can_invite_users);

    struct json_object *can_pin_messages = NULL;
    if (json_object_object_get_ex(obj, "can_pin_messages", &can_pin_messages))
        member->can_pin_messages = json_object_get_boolean(can_pin_messages);

    struct json_object *is_member = NULL;
    if (json_object_object_get_ex(obj, "is_member", &is_member))
        member->is_member = json_object_get_boolean(is_member);

    struct json_object *can_send_media_messages = NULL;
    if (json_object_object_get_ex(obj, "can_send_media_messages", &can_send_media_messages))
        member->can_send_media_messages = json_object_get_boolean(can_send_media_messages);

    struct json_object *can_send_polls = NULL;
    if (json_object_object_get_ex(obj, "can_send_polls", &can_send_polls))
        member->can_send_polls = json_object_get_boolean(can_send_polls);

    struct json_object *can_send_other_messages = NULL;
    if (json_object_object_get_ex(obj, "can_send_other_messages", &can_send_other_messages))
        member->can_send_other_messages = json_object_get_boolean(can_send_other_messages);

    struct json_object *can_add_web_page_previews = NULL;
    if (json_object_object_get_ex(obj, "can_add_web_page_previews", &can_add_web_page_previews))
        member->can_add_web_page_previews = json_object_get_boolean(can_add_web_page_previews);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_permissions(struct json_object *obj, telebot_chat_permissions_t *permissions)
{
    if ((obj == NULL) || (permissions == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(permissions, 0, sizeof(telebot_chat_permissions_t));
    struct json_object *can_send_messages = NULL;
    if (json_object_object_get_ex(obj, "can_send_messages", &can_send_messages))
        permissions->can_send_messages = json_object_get_boolean(can_send_messages);

    struct json_object *can_send_media_messages = NULL;
    if (json_object_object_get_ex(obj, "can_send_media_messages", &can_send_media_messages))
        permissions->can_send_media_messages = json_object_get_boolean(can_send_media_messages);

    struct json_object *can_send_polls = NULL;
    if (json_object_object_get_ex(obj, "can_send_polls", &can_send_polls))
        permissions->can_send_polls = json_object_get_boolean(can_send_polls);

    struct json_object *can_send_other_messages = NULL;
    if (json_object_object_get_ex(obj, "can_send_other_messages", &can_send_other_messages))
        permissions->can_send_other_messages = json_object_get_boolean(can_send_other_messages);

    struct json_object *can_add_web_page_previews = NULL;
    if (json_object_object_get_ex(obj, "can_add_web_page_previews", &can_add_web_page_previews))
        permissions->can_add_web_page_previews = json_object_get_boolean(can_add_web_page_previews);

    struct json_object *can_change_info = NULL;
    if (json_object_object_get_ex(obj, "can_change_info", &can_change_info))
        permissions->can_change_info = json_object_get_boolean(can_change_info);

    struct json_object *can_invite_users = NULL;
    if (json_object_object_get_ex(obj, "can_invite_users", &can_invite_users))
        permissions->can_invite_users = json_object_get_boolean(can_invite_users);

    struct json_object *can_pin_messages = NULL;
    if (json_object_object_get_ex(obj, "can_pin_messages", &can_pin_messages))
        permissions->can_pin_messages = json_object_get_boolean(can_pin_messages);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_bot_command(struct json_object *obj, telebot_bot_command_t *botcmd)
{
    if ((obj == NULL) || (botcmd == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(botcmd, 0, sizeof(telebot_bot_command_t));
    struct json_object *command = NULL;
    if (!json_object_object_get_ex(obj, "command", &command))
    {
        ERR("Object is not bot command type, command not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    botcmd->command = TELEBOT_SAFE_STRDUP(json_object_get_string(command));

    struct json_object *description = NULL;
    if (!json_object_object_get_ex(obj, "description", &description))
    {
        ERR("Object is not bot command type, description not found");
        TELEBOT_SAFE_FREE(botcmd->command);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    botcmd->description = TELEBOT_SAFE_STRDUP(json_object_get_string(description));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_array_bot_command(struct json_object *obj, telebot_bot_command_t **cmds, int *count)
{
    int ret = TELEBOT_ERROR_NONE;
    if ((obj == NULL) || (cmds == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *array = obj;
    int array_len = json_object_array_length(array);
    if (!array_len)
        return TELEBOT_ERROR_OPERATION_FAILED;

    telebot_bot_command_t *result = calloc(array_len, sizeof(telebot_bot_command_t));
    if (result == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    *count = array_len;
    *cmds = result;

    int index = 0;
    for (index = 0; index < array_len; index++)
    {
        struct json_object *item = json_object_array_get_idx(array, index);
        ret = telebot_parser_get_bot_command(item, &result[index]);
        if (ret != TELEBOT_ERROR_NONE)
            break;

    } /* for index */

    if (ret)
    {
        telebot_put_my_commands(result, index + 1);
        *cmds = NULL;
        *count = 0;
    }

    return ret;
}

telebot_error_e telebot_parser_get_response_parameters(struct json_object *obj, telebot_response_paramters_t *resp_param)
{
    if ((obj == NULL) || (resp_param == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(resp_param, 0, sizeof(telebot_response_paramters_t));
    struct json_object *migrate_to_chat_id = NULL;
    if (json_object_object_get_ex(obj, "migrate_to_chat_id", &migrate_to_chat_id))
        resp_param->migrate_to_chat_id = json_object_get_int(migrate_to_chat_id);

    struct json_object *retry_after = NULL;
    if (json_object_object_get_ex(obj, "retry_after", &retry_after))
        resp_param->retry_after = json_object_get_int(retry_after);

    return TELEBOT_ERROR_NONE;
}
