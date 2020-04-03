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
#include <telebot-parser.h>

extern const char *telebot_update_type_str[UPDATE_TYPE_MAX];

static telebot_error_e telebot_parser_get_photos(struct json_object *obj,
        telebot_photo_t **photos, int *count);

static telebot_error_e telebot_parser_get_users(struct json_object *obj,
        telebot_user_t **users, int *count);

static char *strdup_s(const char *str)
{
    if (str)
        return strdup(str);
    else
        return NULL;
}

struct json_object *telebot_parser_str_to_obj(char *data)
{
    return json_tokener_parse(data);
}

telebot_error_e telebot_parser_get_updates(struct json_object *obj,
        telebot_update_t **updates, int *count)
{
    if ((obj == NULL) || (updates == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    int ret;
    struct json_object *array = obj;
    int array_len = json_object_array_length(array);
    if (!array_len)
        return TELEBOT_ERROR_OPERATION_FAILED;

    telebot_update_t *result = calloc(array_len, sizeof(telebot_update_t));
    if (result == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    *count = array_len;
    *updates = result;

    int index;
    for (index = 0; index < array_len; index++) {
        struct json_object *item = json_object_array_get_idx(array, index);

        struct json_object *update_id;
        if (json_object_object_get_ex(item, "update_id", &update_id)) {
            result[index].update_id = json_object_get_int(update_id);
        }

        struct json_object *message;
        if (json_object_object_get_ex(item, "message", &message)) {
            ret = telebot_parser_get_message(message, &(result[index].message));
            if (ret != TELEBOT_ERROR_NONE)
                ERR("Failed to parse message of bot update");

            result[index].update_type = UPDATE_TYPE_MESSAGE;
        }

        struct json_object *callback_query;
        if (json_object_object_get_ex(item, "callback_query", &callback_query)) {
            ret = telebot_parser_get_callback_query(callback_query,
                    &(result[index].callback_query));
            if (ret != TELEBOT_ERROR_NONE)
                ERR("Failed to parse callback query of bot update");

            result[index].update_type = UPDATE_TYPE_CALLBACK_QUERY;
        }

    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_webhook_info(struct json_object *obj,
        telebot_webhook_info_t *info)
{
    if ((obj == NULL) || (info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(info, 0, sizeof(telebot_webhook_info_t));
    struct json_object *url;
    if (!json_object_object_get_ex(obj, "url", &url)) {
        ERR("Object is not json webhook info type, url not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    info->url = strdup_s(json_object_get_string(url));

    struct json_object *certificate;
    if (!json_object_object_get_ex(obj, "has_custom_certificate", &certificate)) {
        ERR("Object is not webhook info type, has_custom_certificate not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    info->has_custom_certificate = json_object_get_boolean(certificate);

    struct json_object *update_count;
    if (!json_object_object_get_ex(obj, "pending_update_count", &update_count)) {
        ERR("Object is not webook info type, pending_update_count not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    info->pending_update_count = json_object_get_int(update_count);

    struct json_object *last_error_date;
    if (json_object_object_get_ex(obj, "last_error_date", &last_error_date)) {
        info->last_error_date = json_object_get_double(last_error_date);
    }

    struct json_object *last_error_message;
    if (json_object_object_get_ex(obj, "last_error_message", &last_error_message)) {
        info->last_error_message = strdup_s(json_object_get_string(last_error_message));
    }

    struct json_object *max_connections;
    if (json_object_object_get_ex(obj, "max_connections", &max_connections)) {
        info->max_connections = json_object_get_int(max_connections);
    }

    struct json_object *allowed_updates;
    if (json_object_object_get_ex(obj, "allowed_updates", &allowed_updates)) {
        int i, j, cnt = 0;
        int array_len = json_object_array_length(allowed_updates);
        for (i=0;i<array_len;i++) {
            struct json_object *item = json_object_array_get_idx(allowed_updates, i);
            const char *update_type = json_object_get_string(item);
            for (j=0;j<UPDATE_TYPE_MAX;j++)
                if (strstr(update_type, telebot_update_type_str[j]))
                    info->allowed_updates[cnt++] = j;
        }
        info->allowed_updates_count = cnt;
    } else {
        int i;
        for (i=0;i<UPDATE_TYPE_MAX;i++)
            info->allowed_updates[i] = i;
        info->allowed_updates_count = UPDATE_TYPE_MAX;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user(struct json_object *obj,
        telebot_user_t *user)
{
    if ((obj == NULL) || (user == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(user, 0, sizeof(telebot_user_t));
    struct json_object *id;
    if (!json_object_object_get_ex(obj, "id", &id)) {
        ERR("Object is not json user type, id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    user->id = json_object_get_int(id);

    struct json_object *first_name;
    if (!json_object_object_get_ex(obj, "first_name", &first_name)) {
        ERR("Object is not user type, first_name not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    user->first_name = strdup_s(json_object_get_string(first_name));

    struct json_object *last_name;
    if (json_object_object_get_ex(obj, "last_name", &last_name)) {
        user->last_name = strdup_s(json_object_get_string(last_name));
    }

    struct json_object *username;
    if (json_object_object_get_ex(obj, "username", &username)) {
        user->username = strdup_s(json_object_get_string(username));
    }

    struct json_object *language_code;
    if (json_object_object_get_ex(obj, "language_code", &language_code)) {
        user->language_code = strdup_s(json_object_get_string(language_code));
    }

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_users(struct json_object *obj,
        telebot_user_t **users, int *count)
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
    for (index = 0; index < array_len; index++) {
        struct json_object *item = json_object_array_get_idx(array, index);
        if (telebot_parser_get_user(item, &(result[index])) != TELEBOT_ERROR_NONE)
            ERR("Failed to parse user from users array");
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat(struct json_object *obj,
        telebot_chat_t *chat)
{
    if ((obj == NULL) || (chat == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(chat, 0, sizeof(telebot_chat_t));
    int ret;
    struct json_object *id;
    if (json_object_object_get_ex(obj, "id", &id)) {
        chat->id = json_object_get_int64(id);
    }
    else {
        ERR("Object is not chat type, id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *type;
    if (json_object_object_get_ex(obj, "type", &type)){
        chat->type = strdup_s(json_object_get_string(type));
    }
    else {
        ERR("Object is not chat type, type not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *title;
    if (json_object_object_get_ex(obj, "title", &title)) {
        chat->title = strdup_s(json_object_get_string(title));
    }

    struct json_object *username;
    if (json_object_object_get_ex(obj, "username", &username)) {
        chat->username = strdup_s(json_object_get_string(username));
    }

    struct json_object *first_name;
    if (json_object_object_get_ex(obj, "first_name", &first_name)) {
        chat->first_name = strdup_s(json_object_get_string(first_name));
    }

    struct json_object *last_name;
    if (json_object_object_get_ex(obj, "last_name", &last_name)) {
        chat->last_name = strdup_s(json_object_get_string(last_name));
    }

    struct json_object *chat_photo;
    if (json_object_object_get_ex(obj, "photo", &chat_photo)) {
        chat->photo = malloc(sizeof(telebot_chat_photo_t));
        ret = telebot_parser_get_chat_photo(chat_photo , chat->photo);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <photo> from chat object");
            TELEBOT_SAFE_FREE(chat->photo);
        }
    }

    struct json_object *description;
    if (json_object_object_get_ex(obj, "description", &description)) {
        chat->description = strdup_s(json_object_get_string(description));
    }

    struct json_object *invite_link;
    if (json_object_object_get_ex(obj, "invite_link", &invite_link)) {
        chat->invite_link = strdup_s(json_object_get_string(invite_link));
    }

    struct json_object *pinned_message;
    if (json_object_object_get_ex(obj, "pinned_message", &pinned_message)) {
        chat->pinned_message = malloc(sizeof(telebot_message_t));
        ret = telebot_parser_get_message(pinned_message , chat->pinned_message);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <pinned_message> from chat object");
            TELEBOT_SAFE_FREE(chat->pinned_message);
        }
    }

    struct json_object *sticker_set_name;
    if (json_object_object_get_ex(obj, "sticker_set_name", &sticker_set_name)) {
        chat->sticker_set_name = strdup_s(json_object_get_string(sticker_set_name));
    }

    struct json_object *can_set_sticker_set;
    if (json_object_object_get_ex(obj, "can_set_sticker_set", &can_set_sticker_set)) {
        chat->can_set_sticker_set = json_object_get_boolean(can_set_sticker_set);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_message(struct json_object *obj,
        telebot_message_t *msg)
{
    if ((obj == NULL) || (msg == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(msg, 0, sizeof(telebot_message_t));
    int ret;
    struct json_object *message_id;
    if (!json_object_object_get_ex(obj, "message_id", &message_id)) {
        ERR("Failed to get <message_id> from message object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    msg->message_id = json_object_get_int(message_id);

    struct json_object *from;
    if (json_object_object_get_ex(obj, "from", &from)) {
        msg->from = malloc(sizeof(telebot_user_t));
        ret = telebot_parser_get_user(from , msg->from);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <from user> from message object");
            TELEBOT_SAFE_FREE(msg->from);
        }
    }

    struct json_object *date;
    if (json_object_object_get_ex(obj, "date", &date)) {
        msg->date = json_object_get_int(date);
    }

    struct json_object *chat;
    if (json_object_object_get_ex(obj, "chat", &chat)) {
        msg->chat = malloc(sizeof(telebot_chat_t));
        ret = telebot_parser_get_chat(chat , msg->chat);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <chat> from message object");
            TELEBOT_SAFE_FREE(msg->chat);
        }
    }

    struct json_object *forward_from;
    if (json_object_object_get_ex(obj, "forward_from", &forward_from)) {
        msg->forward_from = malloc(sizeof(telebot_user_t));
        ret = telebot_parser_get_user(forward_from , msg->forward_from);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <forward from> from message object");
            TELEBOT_SAFE_FREE(msg->forward_from);
        }
    }

    struct json_object *forward_date;
    if (json_object_object_get_ex(obj, "forward_date", &forward_date)) {
        msg->forward_date = json_object_get_int(forward_date);
    }

    struct json_object *reply_to_message;
    if (json_object_object_get_ex(obj, "reply_to_message", &reply_to_message)) {
        msg->reply_to_message = malloc(sizeof(telebot_message_t));
        ret = telebot_parser_get_message(reply_to_message , msg->reply_to_message);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <reply_to_message> from message object");
            TELEBOT_SAFE_FREE(msg->reply_to_message);
        }
    }

    struct json_object *edit_date;
    if (json_object_object_get_ex(obj, "edit_date", &edit_date)) {
        msg->edit_date = json_object_get_int(edit_date);
    }

    struct json_object *media_group_id;
    if (json_object_object_get_ex(obj, "media_group_id", &media_group_id)) {
        msg->media_group_id = strdup_s(json_object_get_string(media_group_id));
    }

    struct json_object *author_signature;
    if (json_object_object_get_ex(obj, "author_signature", &author_signature)) {
        msg->author_signature = strdup_s(json_object_get_string(author_signature));
    }

    struct json_object *text;
    if (json_object_object_get_ex(obj, "text", &text)) {
        msg->text = strdup_s(json_object_get_string(text));
    }

    struct json_object *entities;
    if (json_object_object_get_ex(obj, "entities", &entities)) {
        ret = telebot_parser_get_message_entities(entities, &(msg->entities),
                &(msg->count_entities));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <entities> from message object");
    }

    struct json_object *caption_entities;
    if (json_object_object_get_ex(obj, "caption_entities", &caption_entities)) {
        ret = telebot_parser_get_message_entities(caption_entities,
                &(msg->caption_entities), &(msg->count_caption_entities));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <caption_entities> from message object");
    }

    struct json_object *audio;
    if (json_object_object_get_ex(obj, "audio", &audio)) {
        msg->audio = malloc(sizeof(telebot_audio_t));
        ret = telebot_parser_get_audio(audio , msg->audio);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <audio> from message object");
            TELEBOT_SAFE_FREE(msg->audio);
        }
    }

    struct json_object *document;
    if (json_object_object_get_ex(obj, "document", &document)) {
        msg->document = malloc(sizeof(telebot_document_t));
        ret = telebot_parser_get_document(document, msg->document);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <document> from message object");
            TELEBOT_SAFE_FREE(msg->document);
        }
    }

    //TODO: implement game parsing

    struct json_object *photo;
    if (json_object_object_get_ex(obj, "photo", &photo)) {
        ret = telebot_parser_get_photos(photo, &(msg->photos),
                &(msg->count_photos));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <photo> from message object");
    }

    struct json_object *sticker;
    if (json_object_object_get_ex(obj, "sticker", &sticker)) {
        msg->sticker = malloc(sizeof(telebot_sticker_t));
        ret = telebot_parser_get_sticker(sticker , msg->sticker);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <sticker> from message object");
            TELEBOT_SAFE_FREE(msg->sticker);
        }
    }

    struct json_object *video;
    if (json_object_object_get_ex(obj, "video", &video)) {
        msg->video = malloc(sizeof(telebot_video_t));
        ret = telebot_parser_get_video(video , msg->video);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <video> from message object");
            TELEBOT_SAFE_FREE(msg->video);
        }
    }

    struct json_object *video_note;
    if (json_object_object_get_ex(obj, "video_note", &video_note)) {
        msg->video_note = malloc(sizeof(telebot_video_note_t));
        ret = telebot_parser_get_video_note(video_note , msg->video_note);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <video_note> from message object");
            TELEBOT_SAFE_FREE(msg->video_note);
        }
    }

    struct json_object *voice;
    if (json_object_object_get_ex(obj, "voice", &voice)) {
        msg->voice = malloc(sizeof(telebot_voice_t));
        ret = telebot_parser_get_voice(voice , msg->voice);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <voice> from message object");
            TELEBOT_SAFE_FREE(msg->voice);
        }
    }

    struct json_object *caption;
    if (json_object_object_get_ex(obj, "caption", &caption)) {
        msg->new_chat_title = strdup_s(json_object_get_string(caption));
    }

    struct json_object *contact;
    if (json_object_object_get_ex(obj, "contact", &contact)) {
        msg->contact = malloc(sizeof(telebot_contact_t));
        ret = telebot_parser_get_contact(contact , msg->contact);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <contact> from message object");
            TELEBOT_SAFE_FREE(msg->contact);
        }
    }

    struct json_object *location;
    if (json_object_object_get_ex(obj, "location", &location)) {
        msg->location = malloc(sizeof(telebot_location_t));
        ret = telebot_parser_get_location(location , msg->location);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <location> from message object");
            TELEBOT_SAFE_FREE(msg->location);
        }
    }

    struct json_object *venue;
    if (json_object_object_get_ex(obj, "venue", &venue)) {
        msg->venue = malloc(sizeof(telebot_venue_t));
        ret = telebot_parser_get_venue(venue , msg->venue);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <location> from message object");
            TELEBOT_SAFE_FREE(msg->venue);
        }
    }

    struct json_object *ncm;
    if (json_object_object_get_ex(obj, "new_chat_members", &ncm)) {
        ret = telebot_parser_get_users(ncm, &(msg->new_chat_members),
                &(msg->count_new_chat_members));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <new_chat_members> from message object");
    }

    struct json_object *lcm;
    if (json_object_object_get_ex(obj, "left_chat_members", &lcm)) {
        ret = telebot_parser_get_users(lcm, &(msg->left_chat_members),
                &(msg->count_left_chat_members));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <left_chat_members> from message object");
    }

    struct json_object *nct;
    if (json_object_object_get_ex(obj, "new_chat_title", &nct)) {
        msg->new_chat_title = strdup_s(json_object_get_string(nct));
    }

    struct json_object *new_chat_photo;
    if (json_object_object_get_ex(obj, "new_chat_photo", &new_chat_photo)) {
        ret = telebot_parser_get_photos(new_chat_photo, &(msg->new_chat_photos),
                &(msg->count_new_chat_photos));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <new_chat_photo> from message object");
    }

    struct json_object *del_chat_photo;
    if (json_object_object_get_ex(obj, "delete_chat_photo", &del_chat_photo)) {
        msg->delete_chat_photo = json_object_get_boolean(del_chat_photo);
    }

    struct json_object *gcc;
    if (json_object_object_get_ex(obj, "group_chat_created", &gcc)) {
        msg->group_chat_created = json_object_get_boolean(gcc);
    }

    struct json_object *sgcc;
    if (json_object_object_get_ex(obj, "supergroup_chat_created", &sgcc)) {
        msg->supergroup_chat_created = json_object_get_boolean(sgcc);
    }

    struct json_object *cacc;
    if (json_object_object_get_ex(obj, "channel_chat_created", &cacc)) {
        msg->channel_chat_created = json_object_get_boolean(cacc);
    }

    struct json_object *mtci;
    if (json_object_object_get_ex(obj, "migrate_to_chat_id", &mtci)) {
        msg->migrate_to_chat_id = json_object_get_int64(mtci);
    }

    struct json_object *mftci;
    if (json_object_object_get_ex(obj, "migrate_from_chat_id", &mftci)) {
        msg->migrate_from_chat_id = json_object_get_int64(mftci);
    }

    struct json_object *pinned_message;
    if (json_object_object_get_ex(obj, "pinned_message", &pinned_message)) {
        msg->pinned_message = malloc(sizeof(telebot_message_t));
        ret = telebot_parser_get_message(pinned_message , msg->pinned_message);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <pinned_message> from message object");
            TELEBOT_SAFE_FREE(msg->pinned_message);
        }
    }

    //TODO: implement invoce & successful_payment

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_message_entity(struct json_object *obj,
        telebot_message_entity_t *entity)
{
    if ((obj == NULL) || (entity == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(entity, 0, sizeof(telebot_message_entity_t));
    struct json_object *type;
    if (json_object_object_get_ex(obj, "type", &type)) {
        entity->type = strdup_s(json_object_get_string(type));
    }
    else {
        ERR("Object is not message entity type, type not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *offset;
    if (json_object_object_get_ex(obj, "offset", &offset)){
        entity->offset = json_object_get_int(offset);
    }
    else {
        TELEBOT_SAFE_FREE(entity->type);
        ERR("Object is not message entity type, offset not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *length;
    if (json_object_object_get_ex(obj, "length", &length)){
        entity->length = json_object_get_int(length);
    }
    else {
        ERR("Object is not message entity type, length not found");
        TELEBOT_SAFE_FZCNT(entity->type, entity->offset);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *url;
    if (json_object_object_get_ex(obj, "url", &url)) {
        entity->url = strdup_s(json_object_get_string(url));
    }

    struct json_object *user;
    if (json_object_object_get_ex(obj, "user", &user)) {
        entity->user = malloc(sizeof(telebot_user_t));
        int ret = telebot_parser_get_user(user, entity->user);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <user> from message entity object");
            TELEBOT_SAFE_FREE(entity->user);
        }
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_message_entities(struct json_object *obj,
        telebot_message_entity_t **entities, int *count)
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

    int index, ret;
    for (index = 0; index < array_len; index++) {
        struct json_object *item = json_object_array_get_idx(array, index);
        ret = telebot_parser_get_message_entity(item, &(result[index]));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to parse user from users array");
    }

    return TELEBOT_ERROR_NONE;
}


telebot_error_e telebot_parser_get_photo(struct json_object *obj,
        telebot_photo_t *photo)
{
    if ((obj == NULL) || (photo == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(photo, 0, sizeof(telebot_photo_t));
    struct json_object *file_id;
    if (json_object_object_get_ex(obj, "file_id", &file_id)) {
        photo->file_id = strdup_s(json_object_get_string(file_id));
    }
    else {
        ERR("Object is not photo size type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *width;
    if (json_object_object_get_ex(obj, "width", &width)){
        photo->width = json_object_get_int(width);
    }
    else {
        ERR("Object is not photo size type, width not found");
        TELEBOT_SAFE_FREE(photo->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *height;
    if (json_object_object_get_ex(obj, "height", &height)){
        photo->height = json_object_get_int(height);
    }
    else {
        ERR("Object is not photo size type, height not found");
        TELEBOT_SAFE_FZCNT(photo->file_id, photo->width);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *file_size;
    if (json_object_object_get_ex(obj, "file_size", &file_size)) {
        photo->file_size = json_object_get_int(file_size);
    }

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_photos(struct json_object *obj,
        telebot_photo_t **photos, int *count)
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

    int index, ret;
    for (index = 0; index < array_len; index++) {
        struct json_object *item = json_object_array_get_idx(array, index);
        ret = telebot_parser_get_photo(item, &(result[index]));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to parse user from users array");
    }

    return TELEBOT_ERROR_NONE;
}


telebot_error_e telebot_parser_get_audio(struct json_object *obj,
        telebot_audio_t *audio)
{
    if ((obj == NULL) || (audio == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(audio, 0, sizeof(telebot_audio_t));
    struct json_object *file_id;
    if (json_object_object_get_ex(obj, "file_id", &file_id)) {
        audio->file_id = strdup_s(json_object_get_string(file_id));
    }
    else {
        ERR("Object is not audio type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *duration;
    if (json_object_object_get_ex(obj, "duration", &duration)){
        audio->duration = json_object_get_int(duration);
    }
    else {
        ERR("Object is not audio type, duration not found");
        TELEBOT_SAFE_FREE(audio->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *performer;
    if (json_object_object_get_ex(obj, "performer", &performer)) {
        audio->performer = strdup_s(json_object_get_string(performer));
    }

    struct json_object *title;
    if (json_object_object_get_ex(obj, "title", &title)) {
        audio->title = strdup_s(json_object_get_string(title));
    }

    struct json_object *mime_type;
    if (json_object_object_get_ex(obj, "mime_type", &mime_type)) {
        audio->mime_type = strdup_s(json_object_get_string(mime_type));
    }

    struct json_object *file_size;
    if (json_object_object_get_ex(obj, "file_size", &file_size)) {
        audio->file_size = json_object_get_int(file_size);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_document(struct json_object *obj,
        telebot_document_t *document)
{
    if ((obj == NULL) || (document == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(document, 0, sizeof(telebot_document_t));
    struct json_object *file_id;
    if (json_object_object_get_ex(obj, "file_id", &file_id)) {
        document->file_id = strdup_s(json_object_get_string(file_id));
    }
    else {
        ERR("Object is not audio type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *thumb;
    if (json_object_object_get_ex(obj, "thumb", &thumb)) {
        document->thumb = malloc(sizeof(telebot_photo_t));
        int ret = telebot_parser_get_photo(thumb, document->thumb);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <thumb> from document object");
            TELEBOT_SAFE_FREE(document->thumb);
        }
    }

    struct json_object *file_name;
    if (json_object_object_get_ex(obj, "file_name", &file_name)) {
        document->file_name = strdup_s(json_object_get_string(file_name));
    }

    struct json_object *mime_type;
    if (json_object_object_get_ex(obj, "mime_type", &mime_type)) {
        document->mime_type = strdup_s(json_object_get_string(mime_type));
    }

    struct json_object *file_size;
    if (json_object_object_get_ex(obj, "file_size", &file_size)) {
        document->file_size = json_object_get_int(file_size);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_video(struct json_object *obj,
        telebot_video_t *video)
{
    if ((obj == NULL) || (video == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(video, 0, sizeof(telebot_video_t));
    struct json_object *file_id;
    if (json_object_object_get_ex(obj, "file_id", &file_id)) {
        video->file_id = strdup_s(json_object_get_string(file_id));
    }
    else {
        ERR("Object is not video type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *width;
    if (json_object_object_get_ex(obj, "width", &width)){
        video->width = json_object_get_int(width);
    }
    else {
        ERR("Object is not video type, width not found");
        TELEBOT_SAFE_FREE(video->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *height;
    if (json_object_object_get_ex(obj, "height", &height)){
        video->height = json_object_get_int(height);
    }
    else {
        ERR("Object is not video type, height not found");
        TELEBOT_SAFE_FREE(video->file_id);
        video->width = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *duration;
    if (json_object_object_get_ex(obj, "duration", &duration)){
        video->duration = json_object_get_int(duration);
    }
    else {
        ERR("Object is not video type, duration not found");
        TELEBOT_SAFE_FREE(video->file_id);
        video->width = 0;
        video->height = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *thumb;
    if (json_object_object_get_ex(obj, "thumb", &thumb)) {
        video->thumb = malloc(sizeof(telebot_photo_t));
        int ret = telebot_parser_get_photo(thumb, video->thumb);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <thumb> from video object");
            TELEBOT_SAFE_FREE(video->thumb);
        }
    }

    struct json_object *mime_type;
    if (json_object_object_get_ex(obj, "mime_type", &mime_type)) {
        video->mime_type = strdup_s(json_object_get_string(mime_type));
    }

    struct json_object *file_size;
    if (json_object_object_get_ex(obj, "file_size", &file_size)) {
        video->file_size = json_object_get_int(file_size);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_video_note(struct json_object *obj,
        telebot_video_note_t *video_note)
{
    if ((obj == NULL) || (video_note == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(video_note, 0, sizeof(telebot_video_note_t));
    struct json_object *file_id;
    if (json_object_object_get_ex(obj, "file_id", &file_id)) {
        video_note->file_id = strdup_s(json_object_get_string(file_id));
    }
    else {
        ERR("Object is not video note type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *length;
    if (json_object_object_get_ex(obj, "length", &length)){
        video_note->length = json_object_get_int(length);
    }
    else {
        ERR("Object is not video note type, length not found");
        TELEBOT_SAFE_FREE(video_note->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *duration;
    if (json_object_object_get_ex(obj, "duration", &duration)){
        video_note->duration = json_object_get_int(duration);
    }
    else {
        ERR("Object is not video note type, duration not found");
        TELEBOT_SAFE_FZCNT(video_note->file_id, video_note->length);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *thumb;
    if (json_object_object_get_ex(obj, "thumb", &thumb)) {
        video_note->thumb = malloc(sizeof(telebot_photo_t));
        int ret = telebot_parser_get_photo(thumb, video_note->thumb);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <thumb> from video note object");
            TELEBOT_SAFE_FREE(video_note->thumb);
        }
    }

    struct json_object *file_size;
    if (json_object_object_get_ex(obj, "file_size", &file_size)) {
        video_note->file_size = json_object_get_int(file_size);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_voice(struct json_object *obj,
        telebot_voice_t *voice)
{
    if ((obj == NULL) || (voice == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(voice, 0, sizeof(telebot_voice_t));
    struct json_object *file_id;
    if (json_object_object_get_ex(obj, "file_id", &file_id)) {
        voice->file_id = strdup_s(json_object_get_string(file_id));
    }
    else {
        ERR("Object is not voice type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *duration;
    if (json_object_object_get_ex(obj, "duration", &duration)){
        voice->duration = json_object_get_int(duration);
    }
    else {
        ERR("Object is not voice type, voice duration not found");
        TELEBOT_SAFE_FREE(voice->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *mime_type;
    if (json_object_object_get_ex(obj, "mime_type", &mime_type)) {
        voice->mime_type = strdup_s(json_object_get_string(mime_type));
    }

    struct json_object *file_size;
    if (json_object_object_get_ex(obj, "file_size", &file_size)) {
        voice->file_size = json_object_get_int(file_size);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_contact(struct json_object *obj,
        telebot_contact_t *contact)
{
    if ((obj == NULL) || (contact == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(contact, 0, sizeof(telebot_contact_t));
    struct json_object *phone_number;
    if (json_object_object_get_ex(obj, "phone_number", &phone_number)) {
        contact->phone_number = strdup_s(json_object_get_string(phone_number));
    }
    else {
        ERR("Object is not contact type, phone number not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *first_name;
    if (json_object_object_get_ex(obj, "first_name", &first_name)){
        contact->first_name = strdup_s(json_object_get_string(first_name));
    }
    else {
        ERR("Object is not contact type, first name not found");
        TELEBOT_SAFE_FREE(contact->phone_number);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *last_name;
    if (json_object_object_get_ex(obj, "last_name", &last_name)){
        contact->last_name = strdup_s(json_object_get_string(last_name));
    }

    struct json_object *user_id;
    if (json_object_object_get_ex(obj, "user_id", &user_id)) {
        contact->user_id = json_object_get_int(user_id);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_location(struct json_object *obj,
        telebot_location_t *location)
{
    if ((obj == NULL) || (location == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(location, 0, sizeof(telebot_location_t));
    struct json_object *latitude;
    if (json_object_object_get_ex(obj, "latitude", &latitude)) {
        location->latitude = json_object_get_double(latitude);
    }
    else {
        ERR("Object is not location type, latitude not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *longitude;
    if (json_object_object_get_ex(obj, "longitude", &longitude)) {
        location->longitude = json_object_get_double(longitude);
    }
    else {
        ERR("Object is not location type, longitude not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_venue(struct json_object *obj,
        telebot_venue_t *venue)
{
    if ((obj == NULL) || (venue == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(venue, 0, sizeof(telebot_venue_t));
    struct json_object *title;
    if (json_object_object_get_ex(obj, "title", &title)) {
        venue->title = strdup_s(json_object_get_string(title));
    }
    else {
        ERR("Object is not venue type, title not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *address;
    if (json_object_object_get_ex(obj, "address", &address)) {
        venue->address = strdup_s(json_object_get_string(address));
    }
    else {
        ERR("Object is not venue type, address not found");
        TELEBOT_SAFE_FREE(venue->title);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *location;
    if (json_object_object_get_ex(obj, "location", &location)) {
        venue->location = malloc(sizeof(telebot_location_t));
        int ret = telebot_parser_get_location(location, venue->location);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <location> from venue object");
            TELEBOT_SAFE_FREE(venue->location);
            TELEBOT_SAFE_FREE(venue->title);
            TELEBOT_SAFE_FREE(venue->address);
            return TELEBOT_ERROR_OPERATION_FAILED;
        }
    }
    else {
        ERR("Object is not venue type, location not found");
        TELEBOT_SAFE_FREE(venue->title);
        TELEBOT_SAFE_FREE(venue->address);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *foursquare_id;
    if (json_object_object_get_ex(obj, "foursquare_id", &foursquare_id)) {
        venue->foursquare_id = strdup_s(json_object_get_string(foursquare_id));
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user_profile_photos(struct json_object *obj,
        telebot_user_profile_photos_t *photos)
{

    if ((obj == NULL) || (photos == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    int total_count;
    struct json_object *total_count_obj;
    if (json_object_object_get_ex(obj, "total_count", &total_count_obj)) {
        total_count = json_object_get_int(total_count_obj);
    }
    else {
        ERR("Object is not user profile photo type, total_count not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photos->total_count = total_count;
    if (total_count == 0)
        return TELEBOT_ERROR_NONE;

    struct json_object *array;
    if (!json_object_object_get_ex(obj, "photos", &array)) {
        ERR("Failed to get photos from <UserProfilePhotos> object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    int total, subtotal, i, j;
    total = json_object_array_length(array);
    for (i=0;i<4;i++)
        photos->photos[i] = calloc(total, sizeof(telebot_photo_t));
    photos->current_count = total;

    telebot_error_e ret = TELEBOT_ERROR_NONE;
    for (i=0;i<total;i++) {
        struct json_object *item = json_object_array_get_idx(array, i);
        subtotal = json_object_array_length(item);
        if (subtotal > 4) subtotal = 4; /* This MUST not happen */
        for(j=0;j<subtotal;j++) {
            struct json_object *photo = json_object_array_get_idx(item, j);
            ret |= telebot_parser_get_photo(photo, &(photos->photos[j][i]));
        }
    }

    if (ret != TELEBOT_ERROR_NONE) {
        for (i=0;i<4;i++) {
            TELEBOT_SAFE_FREE(photos->photos[i]);
        }
        photos->current_count = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_file_path(struct json_object *obj,
        char **path) {

    if ((obj == NULL) || (path == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *file_path;
    if (json_object_object_get_ex(obj, "file_path", &file_path)) {
        *path = strdup_s(json_object_get_string(file_path));
    }
    else {
        *path = NULL;
        ERR("Object is not file type, file_path not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e  telebot_parser_get_chat_photo(struct json_object *obj,
        telebot_chat_photo_t *photo)
{
    if ((obj == NULL) || (photo == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(photo, 0, sizeof(telebot_chat_photo_t));
    struct json_object *small_file_id;
    if (json_object_object_get_ex(obj, "small_file_id", &small_file_id)) {
        photo->small_file_id = strdup_s(json_object_get_string(small_file_id));
    }
    else {
        ERR("Object is not chat photo type, small_file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *big_file_id;
    if (json_object_object_get_ex(obj, "big_file_id", &big_file_id)) {
        photo->big_file_id = strdup_s(json_object_get_string(big_file_id));
    }
    else {
        ERR("Object is not chat photo type, big_file_id not found");
        TELEBOT_SAFE_FREE(photo->small_file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_sticker(struct json_object *obj,
        telebot_sticker_t *sticker)
{
    if ((obj == NULL) || (sticker == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *file_id;
    if (json_object_object_get_ex(obj, "file_id", &file_id)) {
        sticker->file_id = strdup_s(json_object_get_string(file_id));
    }
    else {
        ERR("Object is not sticker type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *width;
    if (json_object_object_get_ex(obj, "width", &width)){
        sticker->width = json_object_get_int(width);
    }
    else {
        ERR("Object is not sticker type, width not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *height;
    if (json_object_object_get_ex(obj, "height", &height)){
        sticker->height = json_object_get_int(height);
    }
    else {
        ERR("Object is not sticker type, height not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *thumb;
    if (json_object_object_get_ex(obj, "thumb", &thumb)) {
        sticker->thumb = malloc(sizeof(telebot_photo_t));
        int ret = telebot_parser_get_photo(thumb, sticker->thumb);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <thumb> from video note object");
            TELEBOT_SAFE_FREE(sticker->thumb);
        }
    }

    struct json_object *emoji;
    if (json_object_object_get_ex(obj, "emoji", &emoji)) {
        sticker->emoji = strdup_s(json_object_get_string(emoji));
    }

    struct json_object *set_name;
    if (json_object_object_get_ex(obj, "set_name", &set_name)) {
        sticker->set_name = strdup_s(json_object_get_string(set_name));
    }

    struct json_object *mask_position;
    if (json_object_object_get_ex(obj, "mask_position", &mask_position)) {
        sticker->mask_position = malloc(sizeof(telebot_mask_position_t));
        int ret = telebot_parser_get_mask_position(mask_position, sticker->mask_position);
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <mask_position> from video note object");
            TELEBOT_SAFE_FREE(sticker->mask_position);
        }
    }

    struct json_object *file_size;
    if (json_object_object_get_ex(obj, "file_size", &file_size)) {
        sticker->file_size = json_object_get_int(file_size);
    }
    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_mask_position(struct json_object *obj,
        telebot_mask_position_t *mask_position)
{
    if ((obj == NULL) || (mask_position == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *point;
    if (json_object_object_get_ex(obj, "point", &point)) {
        mask_position->point = strdup_s(json_object_get_string(point));
    }
    else {
        ERR("Object is not mask_position type, point not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *x_shift;
    if (json_object_object_get_ex(obj, "x_shift", &x_shift)){
        mask_position->x_shift = json_object_get_double(x_shift);
    }
    else {
        ERR("Object is not mask_position type, x_shift not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *y_shift;
    if (json_object_object_get_ex(obj, "y_shift", &y_shift)){
        mask_position->y_shift = json_object_get_double(y_shift);
    }
    else {
        ERR("Object is not mask_position type, y_shift not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *scale;
    if (json_object_object_get_ex(obj, "scale", &scale)){
        mask_position->scale = json_object_get_double(scale);
    }
    else {
        ERR("Object is not mask_position type, scale not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    return TELEBOT_ERROR_NONE;
}


telebot_error_e telebot_parser_get_callback_query(struct json_object *obj,
        telebot_callback_query_t *cb_query)
{

#if 0
    int ret;
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (cb_query == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *id;
    if (!json_object_object_get_ex(obj, "id", &id)) {
        ERR("Failed to get <id> from callback_query object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    cb_query->id = strdup_s(json_object_get_string(id));


    struct json_object *from;
    if (!json_object_object_get_ex(obj, "from", &from)) {
        ERR("Failed to get <from> from callback_query object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    ret = telebot_parser_get_user(from , &(cb_query->from));

    if (ret != TELEBOT_ERROR_NONE) {
        ERR("Failed to get <from> from callback_query object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *message;
    if (json_object_object_get_ex(obj, "message", &message)) {
        ret = telebot_parser_get_message(message, &(cb_query->message));
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to get <message> from callback_query object");
        }
    }


    struct json_object *inline_message_id;
    if (json_object_object_get_ex(obj, "inline_message_id", &inline_message_id)) {
        int inline_message_id_len = json_object_get_string_len(inline_message_id);

        //TODO(erick): This memory should be allocated with a linear allocator.
        // We will leak for now.
        cb_query->inline_message_id = (char*) malloc(inline_message_id_len + 1);

        const char* tmp_inline_message_id = json_object_get_string(inline_message_id);
        strncpy(cb_query->inline_message_id, tmp_inline_message_id,
                inline_message_id_len + 1);
    }


    struct json_object *chat_instance;
    if (json_object_object_get_ex(obj, "chat_instance", &chat_instance)) {
        int chat_instance_len = json_object_get_string_len(chat_instance);

        //TODO(erick): This memory should be allocated with a linear allocator.
        // We will leak for now.
        cb_query->chat_instance = (char*) malloc(chat_instance_len + 1);

        const char* tmp_chat_instance = json_object_get_string(chat_instance);
        strncpy(cb_query->chat_instance, tmp_chat_instance, chat_instance_len + 1);
    }


    struct json_object *data;
    if (json_object_object_get_ex(obj, "data", &data)) {
        int data_len = json_object_get_string_len(data);

        //TODO(erick): This memory should be allocated with a linear allocator.
        // We will leak for now.
        cb_query->data = (char*) malloc(data_len + 1);

        const char* tmp_data = json_object_get_string(data);
        strncpy(cb_query->data, tmp_data, data_len + 1);
    }


    struct json_object *game_short_name;
    if (json_object_object_get_ex(obj, "game_short_name", &game_short_name)) {
        int game_short_name_len = json_object_get_string_len(game_short_name);

        //TODO(erick): This memory should be allocated with a linear allocator.
        // We will leak for now.
        cb_query->game_short_name = (char*) malloc(game_short_name_len + 1);

        const char* tmp_game_short_name = json_object_get_string(game_short_name);
        strncpy(cb_query->game_short_name, tmp_game_short_name, game_short_name_len + 1);
    }
#endif

    return TELEBOT_ERROR_NONE;
}





