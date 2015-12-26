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
#include <telebot-common.h>
#include <telebot-api.h>
#include <telebot-parser.h>

struct json_object *telebot_parser_str_to_obj(char *data)
{
    return json_tokener_parse(data);
}

telebot_error_e telebot_parser_get_updates(struct json_object *obj,
        telebot_update_t **updates, int *count)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (updates == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *array = obj;
    int array_len = json_object_array_length(array);
    if (!array_len)
        return TELEBOT_ERROR_OPERATION_FAILED;

    telebot_update_t *tmp = calloc(array_len, sizeof(telebot_update_t));
    if (tmp == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    *count = array_len;
    *updates = tmp;

    int index;
    for (index=0;index<array_len;index++) {
        struct json_object *item = json_object_array_get_idx(array, index);

        memset(&(tmp[index].message), 0, sizeof(telebot_message_t));
        tmp[index].update_id = 0;

        struct json_object *update_id;
        if (json_object_object_get_ex(item, "update_id", &update_id)) {
            tmp[index].update_id = json_object_get_int(update_id);
            json_object_put(update_id);
        }

        struct json_object *message;
        if (json_object_object_get_ex(item, "message", &message)) {
            if (telebot_parser_get_message(message, &(tmp[index].message)) !=
                    TELEBOT_ERROR_NONE)
                ERR("Failed to parse message of bot update");
            json_object_put(message);
        }

        json_object_put(item);
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
    json_object_put(message_id);

    int ret;
    struct json_object *from;
    if (json_object_object_get_ex(obj, "from", &from)) {
        ret = telebot_parser_get_user(from , &(msg->from));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <from user> from message object");
        json_object_put(from);
    }

    struct json_object *date;
    if (json_object_object_get_ex(obj, "date", &date)) {
        msg->date = json_object_get_int(date);
        json_object_put(date);
    }

    struct json_object *chat;
    if (json_object_object_get_ex(obj, "chat", &chat)) {
        ret = telebot_parser_get_chat(chat , &(msg->chat));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <chat> from message object");
        json_object_put(chat);
    }

    struct json_object *forward_from;
    if (json_object_object_get_ex(obj, "forward_from", &forward_from)) {
        ret = telebot_parser_get_user(forward_from , &(msg->forward_from));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <forward from> from message object");
        json_object_put(forward_from);
    }

    struct json_object *forward_date;
    if (json_object_object_get_ex(obj, "forward_date", &forward_date)) {
        msg->forward_date = json_object_get_int(forward_date);
        json_object_put(forward_date);
    }

    //FIXME: allocate memory, using alloca()
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
    if (json_object_object_get_ex(obj, "text", &text)) {
        strncpy(msg->text, json_object_get_string(text),
                TELEBOT_MESSAGE_TEXT_SIZE);
        json_object_put(text);
    }

    struct json_object *audio;
    if (json_object_object_get_ex(obj, "audio", &audio)) {
        ret = telebot_parser_get_audio(audio , &(msg->audio));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <audio> from message object");
        json_object_put(audio);
    }

    struct json_object *document;
    if (json_object_object_get_ex(obj, "document", &document)) {
        ret = telebot_parser_get_document(document , &(msg->document));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <document> from message object");
        json_object_put(document);
    }

    struct json_object *photo;
    if (json_object_object_get_ex(obj, "photo", &photo)) {
        ret = telebot_parser_get_photos(photo , msg->photo,
                TELEBOT_MESSAGE_PHOTO_SIZE);
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <photo> from message object");
        json_object_put(photo);
    }

    struct json_object *video;
    if (json_object_object_get_ex(obj, "video", &video)) {
        ret = telebot_parser_get_video(video , &(msg->video));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <video> from message object");
        json_object_put(video);
    }

    struct json_object *voice;
    if (json_object_object_get_ex(obj, "voice", &voice)) {
        ret = telebot_parser_get_voice(voice , &(msg->voice));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <voice> from message object");
        json_object_put(voice);
    }

    struct json_object *caption;
    if (json_object_object_get_ex(obj, "caption", &caption)) {
        strncpy(msg->caption, json_object_get_string(caption),
                TELEBOT_MESSAGE_CAPTION_SIZE);
        json_object_put(caption);
    }

    struct json_object *contact;
    if (json_object_object_get_ex(obj, "contact", &contact)) {
        ret = telebot_parser_get_contact(contact , &(msg->contact));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <contact> from message object");
        json_object_put(contact);
    }

    struct json_object *location;
    if (json_object_object_get_ex(obj, "location", &location)) {
        ret = telebot_parser_get_location(location , &(msg->location));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <location> from message object");
        json_object_put(location);
    }

    struct json_object *ncp;
    if (json_object_object_get_ex(obj, "new_chat_participant", &ncp)) {
        ret = telebot_parser_get_user(ncp , &(msg->new_chat_participant));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <new_chat_participant> from message object");
        json_object_put(ncp);
    }

    struct json_object *lcp;
    if (json_object_object_get_ex(obj, "left_chat_participant", &lcp)) {
        ret = telebot_parser_get_user(lcp , &(msg->left_chat_participant));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <left_chat_participant> from message object");
        json_object_put(lcp);
    }

    struct json_object *nct;
    if (json_object_object_get_ex(obj, "new_chat_title", &nct)) {
        strncpy(msg->new_chat_title, json_object_get_string(nct),
                TELEBOT_CHAT_TITLE_SIZE);
        json_object_put(nct);
    }

    struct json_object *new_chat_photo;
    if (json_object_object_get_ex(obj, "new_chat_photo", &new_chat_photo)) {
        ret = telebot_parser_get_photos(new_chat_photo , msg->new_chat_photo,
                TELEBOT_MESSAGE_NEW_CHAT_PHOTO_SIZE);
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <left_chat_participant> from message object");
        json_object_put(new_chat_photo);
    }

    struct json_object *del_chat_photo;
    if (json_object_object_get_ex(obj, "delete_chat_photo", &del_chat_photo)) {
        msg->delete_chat_photo = json_object_get_boolean(del_chat_photo);
        json_object_put(del_chat_photo);
    }

    struct json_object *gcc;
    if (json_object_object_get_ex(obj, "group_chat_created", &gcc)) {
        msg->group_chat_created = json_object_get_boolean(gcc);
        json_object_put(gcc);
    }

    struct json_object *sgcc;
    if (json_object_object_get_ex(obj, "supergroup_chat_created", &sgcc)) {
        msg->supergroup_chat_created = json_object_get_boolean(sgcc);
        json_object_put(sgcc);
    }

    struct json_object *cacc;
    if (json_object_object_get_ex(obj, "channel_chat_created", &cacc)) {
        msg->channel_chat_created = json_object_get_boolean(cacc);
        json_object_put(cacc);
    }

    struct json_object *mtci;
    if (json_object_object_get_ex(obj, "migrate_to_chat_id", &mtci)) {
        msg->migrate_to_chat_id = json_object_get_int(mtci);
        json_object_put(mtci);
    }

    struct json_object *mftci;
    if (json_object_object_get_ex(obj, "migrate_from_chat_id", &mftci)) {
        msg->migrate_from_chat_id = json_object_get_int(mftci);
        json_object_put(mftci);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user(struct json_object *obj,
        telebot_user_t *user)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (user == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    memset(user, 0, sizeof(telebot_user_t));

    struct json_object *id;
    if (!json_object_object_get_ex(obj, "id", &id)) {
        ERR("Object is not json user type, id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    user->id = json_object_get_int(id);
    json_object_put(id);

    struct json_object *first_name;
    if (!json_object_object_get_ex(obj, "first_name", &first_name)) {
        ERR("Object is not user type, first_name not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    strncpy(user->first_name, json_object_get_string(first_name),
            TELEBOT_FIRST_NAME_SIZE);
    json_object_put(first_name);

    struct json_object *last_name;
    if (json_object_object_get_ex(obj, "last_name", &last_name)) {
        strncpy(user->last_name, json_object_get_string(last_name),
                TELEBOT_LAST_NAME_SIZE);
        json_object_put(last_name);
    }

    struct json_object *username;
    if (json_object_object_get_ex(obj, "username", &username)) {
        strncpy(user->username, json_object_get_string(username),
                TELEBOT_USER_NAME_SIZE);
        json_object_put(username);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat(struct json_object *obj,
        telebot_chat_t *chat)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (chat == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    memset(chat, 0, sizeof(telebot_chat_t));

    struct json_object *id;
    if (json_object_object_get_ex(obj, "id", &id)) {
        chat->id = json_object_get_int(id);
        json_object_put(id);
    }
    else {
        ERR("Object is not chat type, id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *type;
    if (json_object_object_get_ex(obj, "type", &type)){
        strncpy(chat->type, json_object_get_string(type),
                TELEBOT_CHAT_TYPE_SIZE);
        json_object_put(type);
    }
    else {
        ERR("Object is not chat type, type not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *title;
    if (json_object_object_get_ex(obj, "title", &title)) {
        strncpy(chat->title, json_object_get_string(title),
                TELEBOT_CHAT_TITLE_SIZE);
        json_object_put(title);
    }

    struct json_object *username;
    if (json_object_object_get_ex(obj, "username", &username)) {
        strncpy(chat->username, json_object_get_string(username),
                TELEBOT_USER_NAME_SIZE);
        json_object_put(username);
    }

    struct json_object *first_name;
    if (json_object_object_get_ex(obj, "first_name", &first_name)) {
        strncpy(chat->first_name, json_object_get_string(first_name),
                TELEBOT_FIRST_NAME_SIZE);
        json_object_put(first_name);
    }

    struct json_object *last_name;
    if (json_object_object_get_ex(obj, "last_name", &last_name)) {
        strncpy(chat->last_name, json_object_get_string(last_name),
                TELEBOT_LAST_NAME_SIZE);
        json_object_put(last_name);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_audio(struct json_object *obj,
        telebot_audio_t *audio)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (audio == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    memset(audio, 0, sizeof(telebot_audio_t));

    struct json_object *file_id;
    if (json_object_object_get_ex(obj, "file_id", &file_id)) {
        strncpy(audio->file_id, json_object_get_string(file_id),
                TELEBOT_FILE_ID_SIZE);
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
        ERR("Object is not audio type, duration not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *performer;
    if (json_object_object_get_ex(obj, "performer", &performer)) {
        strncpy(audio->performer, json_object_get_string(performer),
                TELEBOT_AUDIO_PERFORMER_SIZE);
        json_object_put(performer);
    }

    struct json_object *title;
    if (json_object_object_get_ex(obj, "title", &title)) {
        strncpy(audio->title, json_object_get_string(title),
                TELEBOT_AUDIO_TITLE_SIZE);
        json_object_put(title);
    }

    struct json_object *mime_type;
    if (json_object_object_get_ex(obj, "mime_type", &mime_type)) {
        strncpy(audio->mime_type, json_object_get_string(mime_type),
                TELEBOT_AUDIO_MIME_TYPE_SIZE);
        json_object_put(mime_type);
    }

    struct json_object *file_size;
    if (json_object_object_get_ex(obj, "file_size", &file_size)) {
        audio->file_size = json_object_get_int(file_size);
        json_object_put(file_size);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_document(struct json_object *obj,
        telebot_document_t *document)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (document == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    memset(document, 0, sizeof(telebot_document_t));

    struct json_object *file_id;
    if (json_object_object_get_ex(obj, "file_id", &file_id)) {
        strncpy(document->file_id, json_object_get_string(file_id),
                TELEBOT_FILE_ID_SIZE);
        json_object_put(file_id);
    }
    else {
        ERR("Object is not audio type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *thumb;
    if (json_object_object_get_ex(obj, "thumb", &thumb)) {
        if (telebot_parser_get_photo(thumb, &(document->thumb)) !=
                TELEBOT_ERROR_NONE)
            ERR("Failed to get <thumb> from document object");
        json_object_put(thumb);
    }

    struct json_object *file_name;
    if (json_object_object_get_ex(obj, "file_name", &file_name)) {
        strncpy(document->file_name, json_object_get_string(file_name),
                TELEBOT_FILE_NAME_SIZE);
        json_object_put(file_name);
    }

    struct json_object *mime_type;
    if (json_object_object_get_ex(obj, "mime_type", &mime_type)) {
        strncpy(document->mime_type, json_object_get_string(mime_type),
                TELEBOT_DOCUMENT_MIME_TYPE_SIZE);
        json_object_put(file_name);
    }

    struct json_object *file_size;
    if (json_object_object_get_ex(obj, "file_size", &file_size)) {
        document->file_size = json_object_get_int(file_size);
        json_object_put(file_name);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_profile_photos(struct json_object *obj,
        telebot_photo_t **photos, int *count)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (photos == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    int total_count;
    struct json_object *total_count_obj;
    if (json_object_object_get_ex(obj, "total_count", &total_count_obj)) {
        total_count = json_object_get_int(total_count_obj);
        json_object_put(total_count_obj);
    }
    else {
        ERR("Object is not user profile photo type, total_count not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    if (total_count == 0)
        return TELEBOT_ERROR_NONE;


    telebot_photo_t *tmp = calloc(total_count, sizeof(telebot_photo_t));
    *photos = tmp;
    *count = total_count;

    struct json_object *array;
    if (!json_object_object_get_ex(obj, "photos", &array)) {
        ERR("Failed to get photos from <UserProfilePhotos> object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    //FIXME: not sure below code is correct.
    int index;
    int array_len = json_object_array_length(array);
    telebot_error_e ret = TELEBOT_ERROR_NONE;
    for (index=0;index<array_len;index++) {
        struct json_object *item = json_object_array_get_idx(array, index);
        ret |= telebot_parser_get_photos(item, tmp + index*sizeof(telebot_photo_t), 4);
        json_object_put(item);
    }
    json_object_put(array);

    if (ret != TELEBOT_ERROR_NONE) {
        return TELEBOT_ERROR_OPERATION_FAILED;
        *photos = NULL;
        *count = 0;
        free(tmp);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_photo(struct json_object *obj,
        telebot_photo_t *photo)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (photo == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    memset(photo, 0, sizeof(telebot_photo_t));

    struct json_object *file_id;
    if (json_object_object_get_ex(obj, "file_id", &file_id)) {
        strncpy(photo->file_id, json_object_get_string(file_id),
                TELEBOT_FILE_ID_SIZE);
        json_object_put(file_id);
    }
    else {
        ERR("Object is not photo size type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *width;
    if (json_object_object_get_ex(obj, "width", &width)){
        photo->width = json_object_get_int(width);
        json_object_put(width);
    }
    else {
        ERR("Object is not photo size type, width not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *height;
    if (json_object_object_get_ex(obj, "height", &height)){
        photo->height = json_object_get_int(height);
        json_object_put(height);
    }
    else {
        ERR("Object is not photo size type, height not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *file_size;
    if (json_object_object_get_ex(obj, "file_size", &file_size)) {
        photo->file_size = json_object_get_int(file_size);
        json_object_put(file_size);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_photos(struct json_object *obj,
        telebot_photo_t photo_array[], int array_size)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (photo_array == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *array = obj;
    int array_len = json_object_array_length(array);
    if (!array_len)
        return TELEBOT_ERROR_OPERATION_FAILED;

    /* FIXME: why we don't have enough memory? */
    if (array_len > array_size)
        array_len = array_size;

    int index;
    for (index=0;index<array_len;index++) {
        struct json_object *item = json_object_array_get_idx(array, index);

        if (telebot_parser_get_photo(item, &(photo_array[index])) !=
                TELEBOT_ERROR_NONE)
            ERR("Failed to parse photo object");
        json_object_put(item);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_sticker(struct json_object *obj,
        telebot_sticker_t *sticker)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (sticker == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    memset(sticker, 0, sizeof(telebot_sticker_t));

    struct json_object *file_id;
    if (json_object_object_get_ex(obj, "file_id", &file_id)) {
        strncpy(sticker->file_id, json_object_get_string(file_id),
                TELEBOT_FILE_ID_SIZE);
        json_object_put(file_id);
    }
    else {
        ERR("Object is not sticker type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *width;
    if (json_object_object_get_ex(obj, "width", &width)){
        sticker->width = json_object_get_int(width);
        json_object_put(width);
    }
    else {
        ERR("Object is not sticker type, width not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *height;
    if (json_object_object_get_ex(obj, "height", &height)){
        sticker->height = json_object_get_int(height);
        json_object_put(height);
    }
    else {
        ERR("Object is not sticker type, height not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *thumb;
    if (json_object_object_get_ex(obj, "thumb", &thumb)) {
        if (telebot_parser_get_photo(thumb, &(sticker->thumb)) !=
                TELEBOT_ERROR_NONE)
            ERR("Failed to get <thumb> from sticker object");
        json_object_put(thumb);
    }

    struct json_object *file_size;
    if (json_object_object_get_ex(obj, "file_size", &file_size)) {
        sticker->file_size = json_object_get_int(file_size);
        json_object_put(file_size);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_video(struct json_object *obj,
        telebot_video_t *video)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (video == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    memset(video, 0, sizeof(telebot_video_t));

    struct json_object *file_id;
    if (json_object_object_get_ex(obj, "file_id", &file_id)) {
        strncpy(video->file_id, json_object_get_string(file_id),
                TELEBOT_FILE_ID_SIZE);
        json_object_put(file_id);
    }
    else {
        ERR("Object is not video type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *width;
    if (json_object_object_get_ex(obj, "width", &width)){
        video->width = json_object_get_int(width);
        json_object_put(width);
    }
    else {
        ERR("Object is not video type, width not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *height;
    if (json_object_object_get_ex(obj, "height", &height)){
        video->height = json_object_get_int(height);
        json_object_put(height);
    }
    else {
        ERR("Object is not video type, height not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *duration;
    if (json_object_object_get_ex(obj, "duration", &duration)){
        video->duration = json_object_get_int(duration);
        json_object_put(duration);
    }
    else {
        ERR("Object is not video type, duration not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *thumb;
    if (json_object_object_get_ex(obj, "thumb", &thumb)) {
        if (telebot_parser_get_photo(thumb, &(video->thumb)) !=
                TELEBOT_ERROR_NONE)
            ERR("Failed to get <thumb> from video object");
        json_object_put(thumb);
    }

    struct json_object *mime_type;
    if (json_object_object_get_ex(obj, "mime_type", &mime_type)) {
        strncpy(video->mime_type, json_object_get_string(mime_type),
                TELEBOT_VIDEO_MIME_TYPE_SIZE);
        json_object_put(mime_type);
    }

    struct json_object *file_size;
    if (json_object_object_get_ex(obj, "file_size", &file_size)) {
        video->file_size = json_object_get_int(file_size);
        json_object_put(file_size);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_voice(struct json_object *obj,
        telebot_voice_t *voice)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (voice == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    memset(voice, 0, sizeof(telebot_voice_t));

    struct json_object *file_id;
    if (json_object_object_get_ex(obj, "file_id", &file_id)) {
        strncpy(voice->file_id, json_object_get_string(file_id),
                TELEBOT_FILE_ID_SIZE);
        json_object_put(file_id);
    }
    else {
        ERR("Object is not voice type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *duration;
    if (json_object_object_get_ex(obj, "duration", &duration)){
        voice->duration = json_object_get_int(duration);
        json_object_put(duration);
    }
    else {
        ERR("Object is not voice type, voice duration not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *mime_type;
    if (json_object_object_get_ex(obj, "mime_type", &mime_type)) {
        strncpy(voice->mime_type, json_object_get_string(mime_type),
                TELEBOT_AUDIO_MIME_TYPE_SIZE);
        json_object_put(mime_type);
    }

    struct json_object *file_size;
    if (json_object_object_get_ex(obj, "file_size", &file_size)) {
        voice->file_size = json_object_get_int(file_size);
        json_object_put(file_size);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_contact(struct json_object *obj,
        telebot_contact_t *contact)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (contact == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    memset(contact, 0, sizeof(telebot_contact_t));

    struct json_object *phone_number;
    if (json_object_object_get_ex(obj, "phone_number", &phone_number)) {
        strncpy(contact->phone_number, json_object_get_string(phone_number),
                TELEBOT_PHONE_NUMBER_SIZE);
        json_object_put(phone_number);
    }
    else {
        ERR("Object is not contact type, phone number not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *first_name;
    if (json_object_object_get_ex(obj, "first_name", &first_name)){
        strncpy(contact->first_name, json_object_get_string(first_name),
                TELEBOT_FIRST_NAME_SIZE);
        json_object_put(first_name);
    }
    else {
        ERR("Object is not contact type, first name not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *last_name;
    if (json_object_object_get_ex(obj, "last_name", &last_name)){
        strncpy(contact->last_name, json_object_get_string(last_name),
                TELEBOT_LAST_NAME_SIZE);
        json_object_put(last_name);
    }

    struct json_object *user_id;
    if (json_object_object_get_ex(obj, "user_id", &user_id)) {
        contact->user_id = json_object_get_int(user_id);
        json_object_put(user_id);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_location(struct json_object *obj,
        telebot_location_t *location)
{
    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (location == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;
    memset (location, 0, sizeof(telebot_location_t));

    struct json_object *latitude;
    if (json_object_object_get_ex (obj, "latitude", &latitude)) {
        location->latitude = json_object_get_double(latitude);
        json_object_put (latitude);
    }
    else {
        ERR("Object is not location type, latitude not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *longitude;
    if (json_object_object_get_ex (obj, "longitude", &longitude)) {
        location->longitude = json_object_get_double(longitude);
        json_object_put (longitude);
    }
    else {
        ERR("Object is not location type, latitude not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_file_path(struct json_object *obj,
        char **path) {

    if (obj == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (path == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    return TELEBOT_ERROR_NONE;
}

