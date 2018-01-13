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

#ifndef __TELEBOT_PARSER_H__
#define __TELEBOT_PARSER_H__

struct json_object *telebot_parser_str_to_obj(char *data);

/** Get update from Json Object */
telebot_error_e telebot_parser_get_updates(struct json_object *obj,
        telebot_update_t **updates, int *count);

/** Parse webhook info object */
telebot_error_e telebot_parser_get_webhook_info(struct json_object *obj,
        telebot_webhook_info_t *info);

/** Parse message object */
telebot_error_e telebot_parser_get_message(struct json_object *obj,
        telebot_message_t *msg);

/** Parse message entity object */
telebot_error_e telebot_parser_get_message_entity(struct json_object *obj,
        telebot_message_entity_t *entity);

/** Parse message entity array object */
telebot_error_e telebot_parser_get_message_entities(struct json_object *obj,
        telebot_message_entity_t **entities, int *count);

/** Parse user object */
telebot_error_e telebot_parser_get_user(struct json_object *obj,
        telebot_user_t *user);

/** Prase chat object */
telebot_error_e telebot_parser_get_chat(struct json_object *obj,
        telebot_chat_t *chat);

/** Prase audio object */
telebot_error_e telebot_parser_get_audio(struct json_object *obj,
        telebot_audio_t *audio);

/** Prase document object */
telebot_error_e telebot_parser_get_document(struct json_object *obj,
        telebot_document_t *document);

/** Prase photo size object */
telebot_error_e telebot_parser_get_photo(struct json_object *obj,
        telebot_photo_t *photo);

/** Prase video object */
telebot_error_e telebot_parser_get_video(struct json_object *obj,
        telebot_video_t *video);

/** Prase video note object */
telebot_error_e telebot_parser_get_video_note(struct json_object *obj,
        telebot_video_note_t *video_note);

/** Prase voice object */
telebot_error_e telebot_parser_get_voice(struct json_object *obj,
        telebot_voice_t *voice);

/** Prase contact object */
telebot_error_e telebot_parser_get_contact(struct json_object *obj,
        telebot_contact_t *contact);

/** Prase location object */
telebot_error_e telebot_parser_get_location(struct json_object *obj,
        telebot_location_t *location);

/** Prase location object */
telebot_error_e telebot_parser_get_venue(struct json_object *obj,
        telebot_venue_t *venue);

/** Prase profile photos object */
telebot_error_e telebot_parser_get_user_profile_photos(struct json_object *obj,
        telebot_user_profile_photos_t *photos);

/** Prase file path object */
telebot_error_e telebot_parser_get_file_path(struct json_object *obj,
        char **path);

/** Prase chat photo object */
telebot_error_e  telebot_parser_get_chat_photo(struct json_object *obj,
        telebot_chat_photo_t *photo);

/** Prase sticker object */
telebot_error_e telebot_parser_get_sticker(struct json_object *obj,
        telebot_sticker_t *sticker);

/** Prase sticker mask position object */
telebot_error_e telebot_parser_get_mask_position(struct json_object *obj,
        telebot_mask_position_t *mask_position);

/** Parse callback_query object */
telebot_error_e telebot_parser_get_callback_query(struct json_object *obj,
        telebot_callback_query_t *cb_query);

#endif /* __TELEBOT_PARSER_H__ */
