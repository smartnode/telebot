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

#include "telebot-types.h"
#include "telebot-methods.h"
#include "telebot-stickers.h"

struct cJSON;

/* Bot API names of update types, indexed by telebot_update_type_e */
extern const char *const telebot_update_type_names[TELEBOT_UPDATE_TYPE_MAX];

/* Update type for a Bot API name, or TELEBOT_UPDATE_TYPE_UNKNOWN */
int telebot_update_type_from_name(const char *name);

struct cJSON *telebot_parser_str_to_obj(const char *data);

/** Get update from Json Object */
telebot_error_e telebot_parser_get_updates(struct cJSON *obj, telebot_update_t **updates, int *count);

/** Parse webhook info object */
telebot_error_e telebot_parser_get_webhook_info(struct cJSON *obj, telebot_webhook_info_t *info);

/** Parse message object */
telebot_error_e telebot_parser_get_message(struct cJSON *obj, telebot_message_t *msg);

/** Parse message entity object */
telebot_error_e telebot_parser_get_message_entity(struct cJSON *obj, telebot_message_entity_t *entity);

/** Parse message entity array object */
telebot_error_e telebot_parser_get_message_entities(struct cJSON *obj, telebot_message_entity_t **entities, int *count);

/** Parse user object */
telebot_error_e telebot_parser_get_user(struct cJSON *obj, telebot_user_t *user);

/** Prase chat object */
telebot_error_e telebot_parser_get_chat(struct cJSON *obj, telebot_chat_t *chat);

/** Prase chat object */
telebot_error_e telebot_parser_get_chat_member(struct cJSON *obj, telebot_chat_member_t *member);

/** Prase chat admins object */
telebot_error_e telebot_parser_get_chat_admins(struct cJSON *obj, telebot_chat_member_t **admins, int *count);

/** Prase chat permissions object */
telebot_error_e telebot_parser_get_chat_permissions(struct cJSON *obj, telebot_chat_permissions_t *permissions);

/** Prase audio object */
telebot_error_e telebot_parser_get_audio(struct cJSON *obj, telebot_audio_t *audio);

/** Prase document object */
telebot_error_e telebot_parser_get_document(struct cJSON *obj, telebot_document_t *document);

/** Prase photo size object */
telebot_error_e telebot_parser_get_photo(struct cJSON *obj, telebot_photo_t *photo);

/** Parse a sticker */
telebot_error_e telebot_parser_get_sticker(struct cJSON *obj, telebot_sticker_t *sticker);

/** Parse a sticker set */
telebot_error_e telebot_parser_get_sticker_set(struct cJSON *obj, telebot_sticker_set_t *set);

/** Prase video object */
telebot_error_e telebot_parser_get_video(struct cJSON *obj, telebot_video_t *video);

/** Prase animation object */
telebot_error_e telebot_parser_get_animation(struct cJSON *obj, telebot_animation_t *animation);

/** Prase video note object */
telebot_error_e telebot_parser_get_video_note(struct cJSON *obj, telebot_video_note_t *video_note);

/** Prase voice object */
telebot_error_e telebot_parser_get_voice(struct cJSON *obj, telebot_voice_t *voice);

/** Prase contact object */
telebot_error_e telebot_parser_get_contact(struct cJSON *obj, telebot_contact_t *contact);

/** Prase location object */
telebot_error_e telebot_parser_get_location(struct cJSON *obj, telebot_location_t *location);

/** Prase location object */
telebot_error_e telebot_parser_get_venue(struct cJSON *obj, telebot_venue_t *venue);

/** Prase poll option object */
telebot_error_e telebot_parser_get_poll_option(struct cJSON *obj, telebot_poll_option_t *option);

/** Prase poll answer object */
telebot_error_e telebot_parser_get_poll_answer(struct cJSON *obj, telebot_poll_answer_t *answer);

/** Prase poll object */
telebot_error_e telebot_parser_get_poll(struct cJSON *obj, telebot_poll_t *poll);

/** Prase dice object */
telebot_error_e telebot_parser_get_dice(struct cJSON *obj, telebot_dice_t *dice);

/** Prase profile photos object */
telebot_error_e telebot_parser_get_user_profile_photos(struct cJSON *obj, telebot_user_profile_photos_t *photos);

/** Prase file object */
telebot_error_e telebot_parser_get_file(struct cJSON *obj, telebot_file_t *file);

/** Prase callback query object */
telebot_error_e telebot_parser_get_callback_query(struct cJSON *obj, telebot_callback_query_t *cb_query);

/** Prase chat photo object */
telebot_error_e telebot_parser_get_chat_photo(struct cJSON *obj, telebot_chat_photo_t *photo);

/** Prase bot command object */
telebot_error_e telebot_parser_get_bot_command(struct cJSON *obj, telebot_bot_command_t *command);

/** Prase array of bot command object */
telebot_error_e telebot_parser_get_array_bot_command(struct cJSON *obj, telebot_bot_command_t **cmds, int *count);

/** Prase chat location object object */
telebot_error_e telebot_parser_get_chat_location(struct cJSON *obj, telebot_chat_location_t *chat_location);

/** Parse chat message auto-delete timer changed object object */
telebot_error_e telebot_parser_get_message_auto_delete_timer_changed(struct cJSON *obj, telebot_message_auto_delete_timer_changed_t *timer_changed);

/** Parse invoice object */
telebot_error_e telebot_parser_get_invoice(struct cJSON *obj, telebot_invoice_t *invoice);

/** Parse shipping address object */
telebot_error_e telebot_parser_get_shipping_address(struct cJSON *obj, telebot_shipping_address_t *address);

/** Parse order info object */
telebot_error_e telebot_parser_get_order_info(struct cJSON *obj, telebot_order_info_t *info);

/** Parse successful payment object */
telebot_error_e telebot_parser_get_successful_payment(struct cJSON *obj, telebot_successful_payment_t *payment);

/** Parse passport file object */
telebot_error_e telebot_parser_get_passport_file(struct cJSON *obj, telebot_passport_file_t *file);

/** Parse encrypted passport element object */
telebot_error_e telebot_parser_get_encrypted_passport_element(struct cJSON *obj, telebot_encrypted_passport_element_t *element);

/** Parse encrypted credentials object */
telebot_error_e telebot_parser_get_encrypted_credentials(struct cJSON *obj, telebot_encrypted_credentials_t *credentials);

/** Parse passport data object */
telebot_error_e telebot_parser_get_passport_data(struct cJSON *obj, telebot_passport_data_t *passport_data);

/** Parse proximity alert triggered object */
telebot_error_e telebot_parser_get_proximity_alert_triggered(struct cJSON *obj, telebot_proximity_alert_triggered_t *alert);

/** Parse forum topic created object */
telebot_error_e telebot_parser_get_forum_topic_created(struct cJSON *obj, telebot_forum_topic_created_t *topic);

/** Parse forum topic edited object */
telebot_error_e telebot_parser_get_forum_topic_edited(struct cJSON *obj, telebot_forum_topic_edited_t *topic);

/** Parse video chat scheduled object */
telebot_error_e telebot_parser_get_video_chat_scheduled(struct cJSON *obj, telebot_video_chat_scheduled_t *scheduled);

/** Parse video chat ended object */
telebot_error_e telebot_parser_get_video_chat_ended(struct cJSON *obj, telebot_video_chat_ended_t *ended);

/** Parse video chat participants invited object */
telebot_error_e telebot_parser_get_video_chat_participants_invited(struct cJSON *obj, telebot_video_chat_participants_invited_t *invited);

/** Parse web app data object */
telebot_error_e telebot_parser_get_web_app_data(struct cJSON *obj, telebot_web_app_data_t *data);

/** Parse inline keyboard button object */
telebot_error_e telebot_parser_get_inline_keyboard_button(struct cJSON *obj, telebot_inline_keyboard_button_t *button);

/** Parse inline keyboard markup object */
telebot_error_e telebot_parser_get_inline_keyboard_markup(struct cJSON *obj, telebot_inline_keyboard_markup_t *markup);

/** Parse chat invite link object */
telebot_error_e telebot_parser_get_chat_invite_link(struct cJSON *obj, telebot_chat_invite_link_t *invite_link);

/** Parse chat member updated object */
telebot_error_e telebot_parser_get_chat_member_updated(struct cJSON *obj, telebot_chat_member_updated_t *updated);

/** Parse chat join request object */
telebot_error_e telebot_parser_get_chat_join_request(struct cJSON *obj, telebot_chat_join_request_t *request);

/** Parse reaction type object */
telebot_error_e telebot_parser_get_reaction_type(struct cJSON *obj, telebot_reaction_type_t *reaction);

/** Parse message reaction updated object */
telebot_error_e telebot_parser_get_message_reaction_updated(struct cJSON *obj, telebot_message_reaction_updated_t *updated);

/** Parse reaction count object */
telebot_error_e telebot_parser_get_reaction_count(struct cJSON *obj, telebot_reaction_count_t *count);

/** Parse message reaction count updated object */
telebot_error_e telebot_parser_get_message_reaction_count_updated(struct cJSON *obj, telebot_message_reaction_count_updated_t *updated);

/** Parse chat boost source object */
telebot_error_e telebot_parser_get_chat_boost_source(struct cJSON *obj, telebot_chat_boost_source_t *source);

/** Parse chat boost object */
telebot_error_e telebot_parser_get_chat_boost(struct cJSON *obj, telebot_chat_boost_t *boost);

/** Parse chat boost updated object */
telebot_error_e telebot_parser_get_chat_boost_updated(struct cJSON *obj, telebot_chat_boost_updated_t *updated);

/** Parse chat boost removed object */
telebot_error_e telebot_parser_get_chat_boost_removed(struct cJSON *obj, telebot_chat_boost_removed_t *removed);

/** Parse forum topic object */
telebot_error_e telebot_parser_get_forum_topic(struct cJSON *obj, telebot_forum_topic_t *topic);

/** Parse inline query object */
telebot_error_e telebot_parser_get_inline_query(struct cJSON *obj, telebot_inline_query_t *query);

/** Parse chosen inline result object */
telebot_error_e telebot_parser_get_chosen_inline_result(struct cJSON *obj, telebot_chosen_inline_result_t *result);

/** Parse shipping query object */
telebot_error_e telebot_parser_get_shipping_query(struct cJSON *obj, telebot_shipping_query_t *query);

/** Parse pre-checkout query object */
telebot_error_e telebot_parser_get_pre_checkout_query(struct cJSON *obj, telebot_pre_checkout_query_t *query);

/** Parse stickers array */
telebot_error_e telebot_parser_get_stickers(struct cJSON *obj, telebot_sticker_t **stickers, int *count);

/** Parse star transactions */
telebot_error_e telebot_parser_get_star_transactions(struct cJSON *obj, telebot_star_transactions_t *transactions);

/** Parse prepared inline message */
telebot_error_e telebot_parser_get_prepared_inline_message(struct cJSON *obj, telebot_prepared_inline_message_t *prepared_message);

/** Parse game high score */
telebot_error_e telebot_parser_get_game_high_score(struct cJSON *obj, telebot_game_high_score_t *high_score);

/** Parse gifts */
telebot_error_e telebot_parser_get_gifts(struct cJSON *obj, telebot_gifts_t *gifts);

/** Parse user gift */
telebot_error_e telebot_parser_get_user_gift(struct cJSON *obj, telebot_user_gift_t *gift);

/** Parse user gifts */
telebot_error_e telebot_parser_get_user_gifts(struct cJSON *obj, telebot_user_gifts_t *gifts);

/** Parse user profile audios */
telebot_error_e telebot_parser_get_user_profile_audios(struct cJSON *obj, telebot_user_profile_audios_t *audios);

/** Parse business connection */
telebot_error_e telebot_parser_get_business_connection(struct cJSON *obj, telebot_business_connection_t *connection);

/** Parse user chat boosts */
telebot_error_e telebot_parser_get_user_chat_boosts(struct cJSON *obj, telebot_user_chat_boosts_t *boosts);

/** Parse paid media purchased */
telebot_error_e telebot_parser_get_paid_media_purchased(struct cJSON *obj, telebot_paid_media_purchased_t *purchased);

#endif /* __TELEBOT_PARSER_H__ */
