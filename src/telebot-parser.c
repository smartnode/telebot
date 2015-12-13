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
#include <stdbool.h>
#include <string.h>
#include <json.h>
#include <json_object.h>
#include <telebot-private.h>
#include <telebot-parser.h>

struct json_object *telebot_parser_str_to_obj(char *data)
{
    return json_tokener_parse(data);
}

telebot_error_e telebot_parser_get_updates(char *data, telebot_update_t updates[],
        int *count)
{
    if (data == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (updates == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *obj = json_tokener_parse(data);
    struct json_object *array;
    if (!json_object_object_get_ex(obj, "update", &array)) {
        ERR("Failed to get update array from response data");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    int array_len = json_object_array_length(array);
    if (!array_len)
        return TELEBOT_ERROR_OPERATION_FAILED;

    *count = array_len;

    int index;
    for (index=0;index<array_len;index++) {
        obj = json_object_array_get_idx(array, index);

        struct json_object *update_id;
        if (!json_object_object_get_ex(obj, "update_id", &update_id)) {
            ERR("Failed to get update_id from bot updates");
            continue;
        }
        updates[index].update_id = json_object_get_int(update_id);

        struct json_object *message;
        if (!json_object_object_get_ex(obj, "message", &message)) {
            ERR("Failed to get message from bot updates");
            continue;
        }

        int ret = telebot_parser_get_message(message, &(updates[index].message));
        if (ret != TELEBOT_ERROR_NONE) {
            ERR("Failed to parse message of bot update");
            continue;
        }
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_message(struct json_object *obj,
        telebot_message_t *msg)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (msg == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    memset(msg, 0, sizeof(telebot_message_t));

    struct json_object *message_id;
    if (!json_object_object_get_ex(obj, "message_id", &message_id)) {
        ERR("Failed to get <message_id> from message object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    msg->message_id = json_object_get_int(message_id);

    int ret;
    struct json_object *from;
    if (json_object_object_get_ex(obj, "from", &from)) {
        ret = telebot_parser_get_user(from , &(msg->from));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <from user> from message object");
    }

    struct json_object *date;
    if (json_object_object_get_ex(obj, "date", &date))
        msg->date = json_object_get_int(date);

    struct json_object *chat;
    if (json_object_object_get_ex(obj, "chat", &chat)) {
        ret = telebot_parser_get_chat(chat , &(msg->chat));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <chat> from message object");
    }

    struct json_object *forward_from;
    if (json_object_object_get_ex(obj, "forward_from", &forward_from)) {
        ret = telebot_parser_get_user(forward_from , &(msg->forward_from));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <chat> from message object");
    }

    struct json_object *forward_date;
    if (json_object_object_get_ex(obj, "forward_date", &forward_date))
        msg->forward_date = json_object_get_int(forward_date);

    //TODO: Fix following
    /*
       struct json_object *reply_to_message;
       if (json_object_object_get_ex(obj, "reply_to_message", &reply_to_message)) {
       telebot_message_t *reply = alloca(sizeof(telebot_message_t));
       ret = telebot_parser_get_message(reply_to_message , reply);
       if (ret != TELEBOT_ERROR_NONE)
       ERR("Failed to get <reply_to_message> from message object");
       msg->reply_to_message = reply;
       }
       */

    struct json_object *text;
    if (json_object_object_get_ex(obj, "text", &text))
        msg->text = (char *)json_object_get_string(text);

    struct json_object *audio;
    if (json_object_object_get_ex(obj, "audio", &audio)) {
        ret = telebot_parser_get_audio(audio , &(msg->audio));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <audio> from message object");
    }

    struct json_object *document;
    if (json_object_object_get_ex(obj, "document", &document)) {
        ret = telebot_parser_get_document(document , &(msg->document));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <document> from message object");
    }

    struct json_object *photo;
    if (json_object_object_get_ex(obj, "photo", &photo)) {
        ret = telebot_parser_get_photos(photo , msg->photo);
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <photo> from message object");
    }

    struct json_object *video;
    if (json_object_object_get_ex(obj, "video", &video)) {
        ret = telebot_parser_get_video(video , &(msg->video));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <video> from message object");
    }

    struct json_object *voice;
    if (json_object_object_get_ex(obj, "voice", &voice)) {
        ret = telebot_parser_get_voice(voice , &(msg->voice));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <voice> from message object");
    }

    struct json_object *caption;
    if (json_object_object_get_ex(obj, "caption", &caption))
        msg->caption = (char *)json_object_get_string(caption);

    struct json_object *contact;
    if (json_object_object_get_ex(obj, "contact", &contact)) {
        ret = telebot_parser_get_contact(contact , &(msg->contact));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <contact> from message object");
    }

    struct json_object *location;
    if (json_object_object_get_ex(obj, "location", &location)) {
        ret = telebot_parser_get_location(location , &(msg->location));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <location> from message object");
    }

    struct json_object *ncp;
    if (json_object_object_get_ex(obj, "new_chat_participant", &ncp)) {
        ret = telebot_parser_get_user(ncp , &(msg->new_chat_participant));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <new_chat_participant> from message object");
    }

    struct json_object *lcp;
    if (json_object_object_get_ex(obj, "left_chat_participant", &lcp)) {
        ret = telebot_parser_get_user(lcp , &(msg->left_chat_participant));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <left_chat_participant> from message object");
    }

    struct json_object *new_chat_title;
    if (json_object_object_get_ex(obj, "new_chat_title", &new_chat_title))
        msg->new_chat_title = (char *)json_object_get_string(new_chat_title);

    struct json_object *new_chat_photo;
    if (json_object_object_get_ex(obj, "new_chat_photo", &new_chat_photo)) {
        ret = telebot_parser_get_photos(new_chat_photo , msg->new_chat_photo);
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <left_chat_participant> from message object");
    }

    struct json_object *delete_chat_photo;
    if (json_object_object_get_ex(obj, "delete_chat_photo", &delete_chat_photo))
        msg->delete_chat_photo = json_object_get_boolean(delete_chat_photo);

    struct json_object *gcc;
    if (json_object_object_get_ex(obj, "group_chat_created", &gcc))
        msg->group_chat_created = json_object_get_boolean(gcc);

    struct json_object *sgcc;
    if (json_object_object_get_ex(obj, "supergroup_chat_created", &sgcc))
        msg->supergroup_chat_created = json_object_get_boolean(sgcc);

    struct json_object *cacc;
    if (json_object_object_get_ex(obj, "channel_chat_created", &cacc))
        msg->channel_chat_created = json_object_get_boolean(cacc);

    struct json_object *mtci;
    if (json_object_object_get_ex(obj, "migrate_to_chat_id", &mtci))
        msg->migrate_to_chat_id = json_object_get_int(mtci);

    struct json_object *mftci;
    if (json_object_object_get_ex(obj, "migrate_from_chat_id", &mftci))
        msg->migrate_from_chat_id = json_object_get_int(mftci);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user(struct json_object *obj,
        telebot_user_t *user)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (user == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *id;
    if (json_object_object_get_ex(obj, "id", &id)) {
        user->id = json_object_get_int(id);
    }
    else {
        ERR("Object is not json user type, id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *first_name;
    if (json_object_object_get_ex(obj, "first_name", &first_name)){
        user->first_name = (char *)json_object_get_string(first_name);
    }
    else {
        ERR("Object is not json user type, first_name not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *last_name;
    if (json_object_object_get_ex(obj, "last_name", &last_name))
        user->last_name = (char *)json_object_get_string(last_name);

    struct json_object *username;
    if (json_object_object_get_ex(obj, "username", &username))
        user->username = (char *)json_object_get_string(username);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat(struct json_object *obj,
        telebot_chat_t *chat)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (chat == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    
    struct json_object *id;
    if (json_object_object_get_ex(obj, "id", &id)) {
        chat->id = json_object_get_int(id);
    }
    else {
        ERR("Object is not chat type, chat id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    
    struct json_object *type;
    if (json_object_object_get_ex(obj, "type", &type)){
        chat->type = (char *)json_object_get_string(type);
    }
    else {
        ERR("Object is not user type, chat type not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    
    struct json_object *title;
    if (json_object_object_get_ex(obj, "title", &title))
        chat->title = (char *)json_object_get_string(title);
    
    struct json_object *username;
    if (json_object_object_get_ex(obj, "username", &username))
        chat->username = (char *)json_object_get_string(username);
    
    struct json_object *first_name;
    if (json_object_object_get_ex(obj, "first_name", &first_name))
        chat->first_name = (char *)json_object_get_string(first_name);
    
    struct json_object *last_name;
    if (json_object_object_get_ex(obj, "last_name", &last_name))
        chat->last_name = (char *)json_object_get_string(last_name);
    
    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_audio(struct json_object *obj,
                                         telebot_audio_t *audio)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (audio == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *file_id;
    if (json_object_object_get_ex(obj, "file_id", &file_id)) {
        audio->file_id = (char *)json_object_get_string(file_id);
        json_object_put(file_id);
    }
    else {
        ERR("Object is not audio type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    
    struct json_object *duration;
    if (json_object_object_get_ex(obj, "duration", &duration)){
        audio->duration = json_object_get_int(duration);
        json_object_put(duration);
    }
    else {
        ERR("Object is not audio type, audio duration not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    
    struct json_object *performer;
    if (json_object_object_get_ex(obj, "performer", &performer)) {
        audio->performer = (char *)json_object_get_string(performer);
        json_object_put(performer);
    }
    
    struct json_object *title;
    if (json_object_object_get_ex(obj, "title", &title)) {
        audio->title = (char *)json_object_get_string(title);
        json_object_put(title);
    }
    
    struct json_object *mime_type;
    if (json_object_object_get_ex(obj, "mime_type", &mime_type))
        audio->mime_type = (char *)json_object_get_string(mime_type);
    
    struct json_object *file_size;
    if (json_object_object_get_ex(obj, "file_size", &file_size))
        audio->file_size = json_object_get_int(file_size);
    
    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_document(struct json_object *obj,
                                            telebot_document_t *document)
{
    
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    
    if (document == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    
    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_photos(struct json_object *obj,
        telebot_photosize_t photos[])
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (photos == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_sticker(struct json_object *obj,
        telebot_sticker_t *sticker)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (sticker == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_video(struct json_object *obj,
        telebot_video_t *video)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (video == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_voice(struct json_object *obj,
        telebot_voice_t *voice)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (voice == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_contact(struct json_object *obj,
        telebot_contact_t *contact)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (contact == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_location(struct json_object *obj,
        telebot_location_t *location)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (location == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    return TELEBOT_ERROR_NONE;
}


