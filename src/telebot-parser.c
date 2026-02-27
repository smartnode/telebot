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
#include <telebot-methods.h>
#include <telebot-parser.h>
#include <telebot-private.h>

static const char *telebot_update_type_str[TELEBOT_UPDATE_TYPE_MAX] = {
    "message", "edited_message", "channel_post",
    "edited_channel_post", "business_connection",
    "business_message", "edited_business_message",
    "deleted_business_messages", "message_reaction",
    "message_reaction_count", "inline_query",
    "chosen_inline_result", "callback_query",
    "shipping_query", "pre_checkout_query",
    "purchased_paid_media",
    "poll", "poll_answer", "my_chat_member",
    "chat_member", "chat_join_request",
    "chat_boost", "removed_chat_boost"};

static telebot_error_e telebot_parser_get_photos(struct json_object *obj, telebot_photo_t **photos, int *count);
static telebot_error_e telebot_parser_get_users(struct json_object *obj, telebot_user_t **users, int *count);
static telebot_error_e telebot_parser_get_business_messages_deleted(struct json_object *obj, telebot_business_messages_deleted_t *deleted);
static telebot_error_e telebot_parser_get_message_origin(struct json_object *obj, telebot_message_origin_t *origin);
static telebot_error_e telebot_parser_get_text_quote(struct json_object *obj, telebot_text_quote_t *quote);
static telebot_error_e telebot_parser_get_story(struct json_object *obj, telebot_story_t *story);
static telebot_error_e telebot_parser_get_external_reply_info(struct json_object *obj, telebot_external_reply_info_t *info);
static telebot_error_e telebot_parser_get_link_preview_options(struct json_object *obj, telebot_link_preview_options_t *options);
static telebot_error_e telebot_parser_get_paid_media_info(struct json_object *obj, telebot_paid_media_info_t *info);
static telebot_error_e telebot_parser_get_refunded_payment(struct json_object *obj, telebot_refunded_payment_t *payment);
static telebot_error_e telebot_parser_get_write_access_allowed(struct json_object *obj, telebot_write_access_allowed_t *allowed);
static telebot_error_e telebot_parser_get_users_shared(struct json_object *obj, telebot_users_shared_t *shared);
static telebot_error_e telebot_parser_get_chat_shared(struct json_object *obj, telebot_chat_shared_t *shared);
static telebot_error_e telebot_parser_get_gift_info(struct json_object *obj, telebot_gift_info_t *gift);
static telebot_error_e telebot_parser_get_unique_gift_info(struct json_object *obj, telebot_unique_gift_info_t *gift);
static telebot_error_e telebot_parser_get_chat_boost_added(struct json_object *obj, telebot_chat_boost_added_t *boost);
static telebot_error_e telebot_parser_get_chat_background(struct json_object *obj, telebot_chat_background_t *background);
static telebot_error_e telebot_parser_get_giveaway_created(struct json_object *obj, telebot_giveaway_created_t *giveaway);
static telebot_error_e telebot_parser_get_giveaway_completed(struct json_object *obj, telebot_giveaway_completed_t *giveaway);
static telebot_error_e telebot_parser_get_birthdate(struct json_object *obj, telebot_birthdate_t *birthdate);
static telebot_error_e telebot_parser_get_business_intro(struct json_object *obj, telebot_business_intro_t *intro);
static telebot_error_e telebot_parser_get_business_location(struct json_object *obj, telebot_business_location_t *location);
static telebot_error_e telebot_parser_get_business_opening_hours(struct json_object *obj, telebot_business_opening_hours_t *hours);
static telebot_error_e telebot_parser_get_business_bot_rights(struct json_object *obj, telebot_business_bot_rights_t *rights);
static telebot_error_e telebot_parser_get_paid_media(struct json_object *obj, telebot_paid_media_t *media);
static telebot_error_e telebot_parser_get_gift(struct json_object *obj, telebot_gift_t *gift);
static telebot_error_e telebot_parser_get_unique_gift(struct json_object *obj, telebot_unique_gift_t *gift);
static telebot_error_e telebot_parser_get_shared_user(struct json_object *obj, telebot_shared_user_t *user);

struct json_object *telebot_parser_str_to_obj(const char *data)
{
    if (data)
        return json_tokener_parse(data);
    else
        return NULL;
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

        struct json_object *business_connection = NULL;
        if (json_object_object_get_ex(item, "business_connection", &business_connection))
        {
            if (telebot_parser_get_business_connection(business_connection, &(result[index].business_connection)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse business_connection of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_BUSINESS_CONNECTION;
            continue;
        }

        struct json_object *business_message = NULL;
        if (json_object_object_get_ex(item, "business_message", &business_message))
        {
            if (telebot_parser_get_message(business_message, &(result[index].business_message)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse business_message of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_BUSINESS_MESSAGE;
            continue;
        }

        struct json_object *edited_business_message = NULL;
        if (json_object_object_get_ex(item, "edited_business_message", &edited_business_message))
        {
            if (telebot_parser_get_message(edited_business_message, &(result[index].edited_business_message)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse edited_business_message of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_EDITED_BUSINESS_MESSAGE;
            continue;
        }

        struct json_object *deleted_business_messages = NULL;
        if (json_object_object_get_ex(item, "deleted_business_messages", &deleted_business_messages))
        {
            if (telebot_parser_get_business_messages_deleted(deleted_business_messages, &(result[index].deleted_business_messages)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse deleted_business_messages of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_DELETED_BUSINESS_MESSAGES;
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
            if (telebot_parser_get_poll_answer(poll_answer, &(result[index].poll_answer)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse poll answer of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_POLL_ANSWER;
            continue;
        }

        struct json_object *my_chat_member = NULL;
        if (json_object_object_get_ex(item, "my_chat_member", &my_chat_member))
        {
            if (telebot_parser_get_chat_member_updated(my_chat_member, &(result[index].my_chat_member)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse my_chat_member of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_MY_CHAT_MEMBER;
            continue;
        }

        struct json_object *chat_member = NULL;
        if (json_object_object_get_ex(item, "chat_member", &chat_member))
        {
            if (telebot_parser_get_chat_member_updated(chat_member, &(result[index].chat_member)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse chat_member of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CHAT_MEMBER;
            continue;
        }

        struct json_object *chat_join_request = NULL;
        if (json_object_object_get_ex(item, "chat_join_request", &chat_join_request))
        {
            if (telebot_parser_get_chat_join_request(chat_join_request, &(result[index].chat_join_request)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse chat_join_request of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CHAT_JOIN_REQUEST;
            continue;
        }

        struct json_object *message_reaction = NULL;
        if (json_object_object_get_ex(item, "message_reaction", &message_reaction))
        {
            if (telebot_parser_get_message_reaction_updated(message_reaction, &(result[index].message_reaction)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse message_reaction of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_MESSAGE_REACTION;
            continue;
        }

        struct json_object *message_reaction_count = NULL;
        if (json_object_object_get_ex(item, "message_reaction_count", &message_reaction_count))
        {
            if (telebot_parser_get_message_reaction_count_updated(message_reaction_count, &(result[index].message_reaction_count)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse message_reaction_count of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_MESSAGE_REACTION_COUNT;
            continue;
        }

        struct json_object *chat_boost = NULL;
        if (json_object_object_get_ex(item, "chat_boost", &chat_boost))
        {
            if (telebot_parser_get_chat_boost_updated(chat_boost, &(result[index].chat_boost)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse chat_boost of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CHAT_BOOST;
            continue;
        }

        struct json_object *chat_boost_removed = NULL;
        if (json_object_object_get_ex(item, "removed_chat_boost", &chat_boost_removed))
        {
            if (telebot_parser_get_chat_boost_removed(chat_boost_removed, &(result[index].chat_boost_removed)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse chat_boost_removed of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_REMOVED_CHAT_BOOST;
            continue;
        }

        struct json_object *inline_query = NULL;
        if (json_object_object_get_ex(item, "inline_query", &inline_query))
        {
            if (telebot_parser_get_inline_query(inline_query, &(result[index].inline_query)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse inline_query of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_INLINE_QUERY;
            continue;
        }

        struct json_object *chosen_inline_result = NULL;
        if (json_object_object_get_ex(item, "chosen_inline_result", &chosen_inline_result))
        {
            if (telebot_parser_get_chosen_inline_result(chosen_inline_result, &(result[index].chosen_inline_result)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse chosen_inline_result of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CHOSEN_INLINE_RESULT;
            continue;
        }

        struct json_object *shipping_query = NULL;
        if (json_object_object_get_ex(item, "shipping_query", &shipping_query))
        {
            if (telebot_parser_get_shipping_query(shipping_query, &(result[index].shipping_query)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse shipping_query of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_SHIPPING_QUERY;
            continue;
        }

        struct json_object *pre_checkout_query = NULL;
        if (json_object_object_get_ex(item, "pre_checkout_query", &pre_checkout_query))
        {
            if (telebot_parser_get_pre_checkout_query(pre_checkout_query, &(result[index].pre_checkout_query)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse pre_checkout_query of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_PRE_CHECKOUT_QUERY;
            continue;
        }

        struct json_object *purchased_paid_media = NULL;
        if (json_object_object_get_ex(item, "purchased_paid_media", &purchased_paid_media))
        {
            if (telebot_parser_get_paid_media_purchased(purchased_paid_media, &(result[index].purchased_paid_media)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse purchased_paid_media of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_PURCHASED_PAID_MEDIA;
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
    user->id = json_object_get_int64(id);

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

    struct json_object *is_premium = NULL;
    if (json_object_object_get_ex(obj, "is_premium", &is_premium))
        user->is_premium = json_object_get_boolean(is_premium);

    struct json_object *added_to_attachment_menu = NULL;
    if (json_object_object_get_ex(obj, "added_to_attachment_menu", &added_to_attachment_menu))
        user->added_to_attachment_menu = json_object_get_boolean(added_to_attachment_menu);

    struct json_object *can_join_groups = NULL;
    if (json_object_object_get_ex(obj, "can_join_groups", &can_join_groups))
        user->can_join_groups = json_object_get_boolean(can_join_groups);

    struct json_object *can_read_all_group_messages = NULL;
    if (json_object_object_get_ex(obj, "can_read_all_group_messages", &can_read_all_group_messages))
        user->can_read_all_group_messages = json_object_get_boolean(can_read_all_group_messages);

    struct json_object *supports_inline_queries = NULL;
    if (json_object_object_get_ex(obj, "supports_inline_queries", &supports_inline_queries))
        user->supports_inline_queries = json_object_get_boolean(supports_inline_queries);

    struct json_object *can_connect_to_business = NULL;
    if (json_object_object_get_ex(obj, "can_connect_to_business", &can_connect_to_business))
        user->can_connect_to_business = json_object_get_boolean(can_connect_to_business);

    struct json_object *has_main_web_app = NULL;
    if (json_object_object_get_ex(obj, "has_main_web_app", &has_main_web_app))
        user->has_main_web_app = json_object_get_boolean(has_main_web_app);

    struct json_object *has_topics_enabled = NULL;
    if (json_object_object_get_ex(obj, "has_topics_enabled", &has_topics_enabled))
        user->has_topics_enabled = json_object_get_boolean(has_topics_enabled);

    struct json_object *allows_users_to_create_topics = NULL;
    if (json_object_object_get_ex(obj, "allows_users_to_create_topics", &allows_users_to_create_topics))
        user->allows_users_to_create_topics = json_object_get_boolean(allows_users_to_create_topics);

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

    struct json_object *is_forum = NULL;
    if (json_object_object_get_ex(obj, "is_forum", &is_forum))
        chat->is_forum = json_object_get_boolean(is_forum);

    struct json_object *is_direct_messages = NULL;
    if (json_object_object_get_ex(obj, "is_direct_messages", &is_direct_messages))
        chat->is_direct_messages = json_object_get_boolean(is_direct_messages);

    struct json_object *accent_color_id = NULL;
    if (json_object_object_get_ex(obj, "accent_color_id", &accent_color_id))
        chat->accent_color_id = json_object_get_int(accent_color_id);

    struct json_object *max_reaction_count = NULL;
    if (json_object_object_get_ex(obj, "max_reaction_count", &max_reaction_count))
        chat->max_reaction_count = json_object_get_int(max_reaction_count);

    struct json_object *chat_photo = NULL;
    if (json_object_object_get_ex(obj, "photo", &chat_photo))
    {
        chat->photo = calloc(1, sizeof(telebot_chat_photo_t));
        if (telebot_parser_get_chat_photo(chat_photo, chat->photo) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <photo> from chat object");
            TELEBOT_SAFE_FREE(chat->photo);
        }
    }

    struct json_object *array_active_usernames = NULL;
    if (json_object_object_get_ex(obj, "active_usernames", &array_active_usernames))
    {
        int count_active_usernames = json_object_array_length(array_active_usernames);
        if ((chat->count_active_usernames > 0) &&
            (chat->active_usernames = calloc(count_active_usernames, sizeof(char *))))
        {
            chat->count_active_usernames = count_active_usernames;
            for (int index = 0; index < count_active_usernames; index++)
            {
                struct json_object *item = json_object_array_get_idx(array_active_usernames, index);
                chat->active_usernames[index] = TELEBOT_SAFE_STRDUP(json_object_get_string(item));
            }
        }
    }

    struct json_object *birthdate = NULL;
    if (json_object_object_get_ex(obj, "birthdate", &birthdate))
    {
        chat->birthdate = calloc(1, sizeof(telebot_birthdate_t));
        telebot_parser_get_birthdate(birthdate, chat->birthdate);
    }

    struct json_object *business_intro = NULL;
    if (json_object_object_get_ex(obj, "business_intro", &business_intro))
    {
        chat->business_intro = calloc(1, sizeof(telebot_business_intro_t));
        telebot_parser_get_business_intro(business_intro, chat->business_intro);
    }

    struct json_object *business_location = NULL;
    if (json_object_object_get_ex(obj, "business_location", &business_location))
    {
        chat->business_location = calloc(1, sizeof(telebot_business_location_t));
        telebot_parser_get_business_location(business_location, chat->business_location);
    }

    struct json_object *business_opening_hours = NULL;
    if (json_object_object_get_ex(obj, "business_opening_hours", &business_opening_hours))
    {
        chat->business_opening_hours = calloc(1, sizeof(telebot_business_opening_hours_t));
        telebot_parser_get_business_opening_hours(business_opening_hours, chat->business_opening_hours);
    }

    struct json_object *personal_chat = NULL;
    if (json_object_object_get_ex(obj, "personal_chat", &personal_chat))
    {
        chat->personal_chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(personal_chat, chat->personal_chat);
    }

    struct json_object *parent_chat = NULL;
    if (json_object_object_get_ex(obj, "parent_chat", &parent_chat))
    {
        chat->parent_chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(parent_chat, chat->parent_chat);
    }

    struct json_object *emoji_status_custom_emoji_id = NULL;
    if (json_object_object_get_ex(obj, "emoji_status_custom_emoji_id", &emoji_status_custom_emoji_id))
        chat->emoji_status_custom_emoji_id = TELEBOT_SAFE_STRDUP(json_object_get_string(emoji_status_custom_emoji_id));

    struct json_object *emoji_status_expiration_date = NULL;
    if (json_object_object_get_ex(obj, "emoji_status_expiration_date", &emoji_status_expiration_date))
        chat->emoji_status_expiration_date = json_object_get_int(emoji_status_expiration_date);

    struct json_object *bio = NULL;
    if (json_object_object_get_ex(obj, "bio", &bio))
        chat->bio = TELEBOT_SAFE_STRDUP(json_object_get_string(bio));

    struct json_object *has_private_forwards = NULL;
    if (json_object_object_get_ex(obj, "has_private_forwards", &has_private_forwards))
        chat->has_private_forwards = json_object_get_boolean(has_private_forwards);

    struct json_object *has_restricted_voice_and_video_messages = NULL;
    if (json_object_object_get_ex(obj, "has_restricted_voice_and_video_messages", &has_restricted_voice_and_video_messages))
        chat->has_restricted_voice_and_video_messages = json_object_get_boolean(has_restricted_voice_and_video_messages);

    struct json_object *join_to_send_messages = NULL;
    if (json_object_object_get_ex(obj, "join_to_send_messages", &join_to_send_messages))
        chat->join_to_send_messages = json_object_get_boolean(join_to_send_messages);

    struct json_object *join_by_request = NULL;
    if (json_object_object_get_ex(obj, "join_by_request", &join_by_request))
        chat->join_by_request = json_object_get_boolean(join_by_request);

    struct json_object *description = NULL;
    if (json_object_object_get_ex(obj, "description", &description))
        chat->description = TELEBOT_SAFE_STRDUP(json_object_get_string(description));

    struct json_object *invite_link = NULL;
    if (json_object_object_get_ex(obj, "invite_link", &invite_link))
        chat->invite_link = TELEBOT_SAFE_STRDUP(json_object_get_string(invite_link));


    struct json_object *permissions = NULL;
    if (json_object_object_get_ex(obj, "permissions", &permissions))
    {
        chat->permissions = calloc(1, sizeof(telebot_chat_permissions_t));
        if (telebot_parser_get_chat_permissions(permissions, chat->permissions) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <permissions> from chat object");
            TELEBOT_SAFE_FREE(chat->permissions);
        }
    }

    struct json_object *slow_mode_delay = NULL;
    if (json_object_object_get_ex(obj, "slow_mode_delay", &slow_mode_delay))
        chat->slow_mode_delay = json_object_get_int(slow_mode_delay);

    struct json_object *unrestrict_boost_count = NULL;
    if (json_object_object_get_ex(obj, "unrestrict_boost_count", &unrestrict_boost_count))
        chat->unrestrict_boost_count = json_object_get_int(unrestrict_boost_count);

    struct json_object *has_aggressive_anti_spam_enabled = NULL;
    if (json_object_object_get_ex(obj, "has_aggressive_anti_spam_enabled", &has_aggressive_anti_spam_enabled))
        chat->has_aggressive_anti_spam_enabled = json_object_get_boolean(has_aggressive_anti_spam_enabled);

    struct json_object *has_hidden_members = NULL;
    if (json_object_object_get_ex(obj, "has_hidden_members", &has_hidden_members))
        chat->has_hidden_members = json_object_get_boolean(has_hidden_members);

    struct json_object *has_protected_content = NULL;
    if (json_object_object_get_ex(obj, "has_protected_content", &has_protected_content))
        chat->has_protected_content = json_object_get_boolean(has_protected_content);

    struct json_object *message_auto_delete_time = NULL;
    if (json_object_object_get_ex(obj, "message_auto_delete_time", &message_auto_delete_time))
        chat->message_auto_delete_time = json_object_get_int(message_auto_delete_time);

    struct json_object *sticker_set_name = NULL;
    if (json_object_object_get_ex(obj, "sticker_set_name", &sticker_set_name))
        chat->sticker_set_name = TELEBOT_SAFE_STRDUP(json_object_get_string(sticker_set_name));

    struct json_object *can_set_sticker_set = NULL;
    if (json_object_object_get_ex(obj, "can_set_sticker_set", &can_set_sticker_set))
        chat->can_set_sticker_set = json_object_get_boolean(can_set_sticker_set);

    struct json_object *custom_emoji_sticker_set_name = NULL;
    if (json_object_object_get_ex(obj, "custom_emoji_sticker_set_name", &custom_emoji_sticker_set_name))
        chat->custom_emoji_sticker_set_name = TELEBOT_SAFE_STRDUP(json_object_get_string(custom_emoji_sticker_set_name));

    struct json_object *linked_chat_id = NULL;
    if (json_object_object_get_ex(obj, "linked_chat_id", &linked_chat_id))
        chat->linked_chat_id = json_object_get_int64(linked_chat_id);

    struct json_object *location = NULL;
    if (json_object_object_get_ex(obj, "location", &location))
    {
        chat->location = calloc(1, sizeof(telebot_chat_location_t));
        if (telebot_parser_get_chat_location(location, chat->location) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get chat location from chat object");
            TELEBOT_SAFE_FREE(chat->location);
        }
    }

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
    msg->chat = calloc(1, sizeof(telebot_chat_t));
    if (telebot_parser_get_chat(chat, msg->chat) != TELEBOT_ERROR_NONE)
    {
        ERR("Failed to get <chat> from message object");
        TELEBOT_SAFE_FREE(msg->chat);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    struct json_object *forward_origin = NULL;
    if (json_object_object_get_ex(obj, "forward_origin", &forward_origin))
    {
        msg->forward_origin = calloc(1, sizeof(telebot_message_origin_t));
        if (telebot_parser_get_message_origin(forward_origin, msg->forward_origin) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <forward_origin> from message object");
            TELEBOT_SAFE_FREE(msg->forward_origin);
        }
    }

    /* Optional Fields */
    struct json_object *message_thread_id = NULL;
    if (json_object_object_get_ex(obj, "message_thread_id", &message_thread_id))
        msg->message_thread_id = json_object_get_int(message_thread_id);

    struct json_object *from = NULL;
    if (json_object_object_get_ex(obj, "from", &from))
    {
        msg->from = calloc(1, sizeof(telebot_user_t));
        if (telebot_parser_get_user(from, msg->from) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <from user> from message object");
            TELEBOT_SAFE_FREE(msg->from);
        }
    }

    struct json_object *sender_chat = NULL;
    if (json_object_object_get_ex(obj, "sender_chat", &sender_chat))
    {
        msg->sender_chat = malloc(sizeof(telebot_chat_t));
        if (telebot_parser_get_chat(sender_chat, msg->sender_chat) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <sender_chat> from message object");
            TELEBOT_SAFE_FREE(msg->sender_chat);
        }
    }

    struct json_object *sender_boost_count = NULL;
    if (json_object_object_get_ex(obj, "sender_boost_count", &sender_boost_count))
        msg->sender_boost_count = json_object_get_int(sender_boost_count);

    struct json_object *sender_business_bot = NULL;
    if (json_object_object_get_ex(obj, "sender_business_bot", &sender_business_bot))
    {
        msg->sender_business_bot = calloc(1, sizeof(telebot_user_t));
        if (telebot_parser_get_user(sender_business_bot, msg->sender_business_bot) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <sender_business_bot> from message object");
            TELEBOT_SAFE_FREE(msg->sender_business_bot);
        }
    }

    struct json_object *business_connection_id = NULL;
    if (json_object_object_get_ex(obj, "business_connection_id", &business_connection_id))
        msg->business_connection_id = TELEBOT_SAFE_STRDUP(json_object_get_string(business_connection_id));

    struct json_object *forward_from = NULL;
    if (json_object_object_get_ex(obj, "forward_from", &forward_from))
    {
        msg->forward_from = calloc(1, sizeof(telebot_user_t));
        if (telebot_parser_get_user(forward_from, msg->forward_from) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <forward from> from message object");
            TELEBOT_SAFE_FREE(msg->forward_from);
        }
    }

    struct json_object *forward_from_chat = NULL;
    if (json_object_object_get_ex(obj, "forward_from_chat", &forward_from_chat))
    {
        msg->forward_from_chat = calloc(1, sizeof(telebot_chat_t));
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

    struct json_object *is_topic_message = NULL;
    if (json_object_object_get_ex(obj, "is_topic_message", &is_topic_message))
        msg->is_topic_message = json_object_get_boolean(is_topic_message);

    struct json_object *is_automatic_forward = NULL;
    if (json_object_object_get_ex(obj, "is_automatic_forward", &is_automatic_forward))
        msg->is_automatic_forward = json_object_get_boolean(is_automatic_forward);

    struct json_object *reply_to_message = NULL;
    if (json_object_object_get_ex(obj, "reply_to_message", &reply_to_message))
    {
        msg->reply_to_message = calloc(1, sizeof(telebot_message_t));
        if (telebot_parser_get_message(reply_to_message, msg->reply_to_message) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <reply_to_message> from message object");
            TELEBOT_SAFE_FREE(msg->reply_to_message);
        }
    }

    struct json_object *external_reply = NULL;
    if (json_object_object_get_ex(obj, "external_reply", &external_reply))
    {
        msg->external_reply = calloc(1, sizeof(telebot_external_reply_info_t));
        if (telebot_parser_get_external_reply_info(external_reply, msg->external_reply) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <external_reply> from message object");
            TELEBOT_SAFE_FREE(msg->external_reply);
        }
    }

    struct json_object *quote = NULL;
    if (json_object_object_get_ex(obj, "quote", &quote))
    {
        msg->quote = calloc(1, sizeof(telebot_text_quote_t));
        if (telebot_parser_get_text_quote(quote, msg->quote) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <quote> from message object");
            TELEBOT_SAFE_FREE(msg->quote);
        }
    }

    struct json_object *reply_to_story = NULL;
    if (json_object_object_get_ex(obj, "reply_to_story", &reply_to_story))
    {
        msg->reply_to_story = calloc(1, sizeof(telebot_story_t));
        if (telebot_parser_get_story(reply_to_story, msg->reply_to_story) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <reply_to_story> from message object");
            TELEBOT_SAFE_FREE(msg->reply_to_story);
        }
    }

    struct json_object *via_bot = NULL;
    if (json_object_object_get_ex(obj, "via_bot", &via_bot))
    {
        msg->via_bot = malloc(sizeof(telebot_user_t));
        if (telebot_parser_get_user(via_bot, msg->via_bot) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <via_bot> from message object");
            TELEBOT_SAFE_FREE(msg->via_bot);
        }
    }

    struct json_object *edit_date = NULL;
    if (json_object_object_get_ex(obj, "edit_date", &edit_date))
        msg->edit_date = json_object_get_int(edit_date);

    struct json_object *has_protected_content = NULL;
    if (json_object_object_get_ex(obj, "has_protected_content", &has_protected_content))
        msg->has_protected_content = json_object_get_boolean(has_protected_content);

    struct json_object *is_paid_post = NULL;
    if (json_object_object_get_ex(obj, "is_paid_post", &is_paid_post))
        msg->is_paid_post = json_object_get_boolean(is_paid_post);

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

    struct json_object *link_preview_options = NULL;
    if (json_object_object_get_ex(obj, "link_preview_options", &link_preview_options))
    {
        msg->link_preview_options = calloc(1, sizeof(telebot_link_preview_options_t));
        if (telebot_parser_get_link_preview_options(link_preview_options, msg->link_preview_options) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <link_preview_options> from message object");
            TELEBOT_SAFE_FREE(msg->link_preview_options);
        }
    }

    struct json_object *effect_id = NULL;
    if (json_object_object_get_ex(obj, "effect_id", &effect_id))
        msg->effect_id = TELEBOT_SAFE_STRDUP(json_object_get_string(effect_id));

    struct json_object *animation = NULL;
    if (json_object_object_get_ex(obj, "animation", &animation))
    {
        msg->animation = malloc(sizeof(telebot_animation_t));
        if (telebot_parser_get_animation(animation, msg->animation) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <animation> from message object");
            TELEBOT_SAFE_FREE(msg->animation);
        }
    }

    struct json_object *audio = NULL;
    if (json_object_object_get_ex(obj, "audio", &audio))
    {
        msg->audio = calloc(1, sizeof(telebot_audio_t));
        if (telebot_parser_get_audio(audio, msg->audio) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <audio> from message object");
            TELEBOT_SAFE_FREE(msg->audio);
        }
    }

    struct json_object *document = NULL;
    if (json_object_object_get_ex(obj, "document", &document))
    {
        msg->document = calloc(1, sizeof(telebot_document_t));
        if (telebot_parser_get_document(document, msg->document) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <document> from message object");
            TELEBOT_SAFE_FREE(msg->document);
        }
    }

    struct json_object *paid_media = NULL;
    if (json_object_object_get_ex(obj, "paid_media", &paid_media))
    {
        msg->paid_media = calloc(1, sizeof(telebot_paid_media_info_t));
        if (telebot_parser_get_paid_media_info(paid_media, msg->paid_media) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <paid_media> from message object");
            TELEBOT_SAFE_FREE(msg->paid_media);
        }
    }

    struct json_object *photo = NULL;
    if (json_object_object_get_ex(obj, "photo", &photo))
    {
        if (telebot_parser_get_photos(photo, &(msg->photos), &(msg->count_photos)) != TELEBOT_ERROR_NONE)
            ERR("Failed to get <photo> from message object");
    }

    // TODO: implement sticker parsing

    struct json_object *video = NULL;
    if (json_object_object_get_ex(obj, "video", &video))
    {
        msg->video = calloc(1, sizeof(telebot_video_t));
        if (telebot_parser_get_video(video, msg->video) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <video> from message object");
            TELEBOT_SAFE_FREE(msg->video);
        }
    }

    struct json_object *video_note = NULL;
    if (json_object_object_get_ex(obj, "video_note", &video_note))
    {
        msg->video_note = calloc(1, sizeof(telebot_video_note_t));
        if (telebot_parser_get_video_note(video_note, msg->video_note) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <video_note> from message object");
            TELEBOT_SAFE_FREE(msg->video_note);
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

    struct json_object *caption = NULL;
    if (json_object_object_get_ex(obj, "caption", &caption))
        msg->caption = TELEBOT_SAFE_STRDUP(json_object_get_string(caption));

    struct json_object *caption_entities = NULL;
    if (json_object_object_get_ex(obj, "caption_entities", &caption_entities))
    {
        if (telebot_parser_get_message_entities(caption_entities, &(msg->caption_entities),
                                                &(msg->count_caption_entities)) != TELEBOT_ERROR_NONE)
            ERR("Failed to get <caption_entities> from message object");
    }

    struct json_object *show_caption_above_media = NULL;
    if (json_object_object_get_ex(obj, "show_caption_above_media", &show_caption_above_media))
        msg->show_caption_above_media = json_object_get_boolean(show_caption_above_media);

    struct json_object *has_media_spoiler = NULL;
    if (json_object_object_get_ex(obj, "has_media_spoiler", &has_media_spoiler))
        msg->has_media_spoiler = json_object_get_boolean(has_media_spoiler);

    struct json_object *contact = NULL;
    if (json_object_object_get_ex(obj, "contact", &contact))
    {
        msg->contact = calloc(1, sizeof(telebot_contact_t));
        if (telebot_parser_get_contact(contact, msg->contact) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <contact> from message object");
            TELEBOT_SAFE_FREE(msg->contact);
        }
    }

    struct json_object *dice = NULL;
    if (json_object_object_get_ex(obj, "dice", &dice))
    {
        msg->dice = calloc(1, sizeof(telebot_dice_t));
        if (telebot_parser_get_dice(dice, msg->dice) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <dice> from message object");
            TELEBOT_SAFE_FREE(msg->dice);
        }
    }

    // TODO: implement game parsing

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

    struct json_object *new_chat_members = NULL;
    if (json_object_object_get_ex(obj, "new_chat_members", &new_chat_members))
    {
        int ret = telebot_parser_get_users(new_chat_members, &(msg->new_chat_members), &(msg->count_new_chat_members));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <new_chat_members> from message object");
    }

    struct json_object *left_chat_members = NULL;
    if (json_object_object_get_ex(obj, "left_chat_members", &left_chat_members))
    {
        int ret = telebot_parser_get_users(left_chat_members, &(msg->left_chat_members), &(msg->count_left_chat_members));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <left_chat_members> from message object");
    }

    struct json_object *new_chat_title = NULL;
    if (json_object_object_get_ex(obj, "new_chat_title", &new_chat_title))
        msg->new_chat_title = TELEBOT_SAFE_STRDUP(json_object_get_string(new_chat_title));

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

    struct json_object *group_chat_created = NULL;
    if (json_object_object_get_ex(obj, "group_chat_created", &group_chat_created))
        msg->group_chat_created = json_object_get_boolean(group_chat_created);

    struct json_object *supergroup_chat_created = NULL;
    if (json_object_object_get_ex(obj, "supergroup_chat_created", &supergroup_chat_created))
        msg->supergroup_chat_created = json_object_get_boolean(supergroup_chat_created);

    struct json_object *channel_chat_created = NULL;
    if (json_object_object_get_ex(obj, "channel_chat_created", &channel_chat_created))
        msg->channel_chat_created = json_object_get_boolean(channel_chat_created);

    struct json_object *message_auto_delete_timer_changed = NULL;
    if (json_object_object_get_ex(obj, "message_auto_delete_timer_changed", &message_auto_delete_timer_changed))
    {
        msg->message_auto_delete_timer_changed = malloc(sizeof(telebot_message_auto_delete_timer_changed_t));
        int ret = telebot_parser_get_message_auto_delete_timer_changed(message_auto_delete_timer_changed,
                                                                       msg->message_auto_delete_timer_changed);
        if (ret != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <message_auto_delete_timer_changed> from message object");
            TELEBOT_SAFE_FREE(msg->message_auto_delete_timer_changed);
        }
    }

    struct json_object *migrate_to_chat_id = NULL;
    if (json_object_object_get_ex(obj, "migrate_to_chat_id", &migrate_to_chat_id))
        msg->migrate_to_chat_id = json_object_get_int64(migrate_to_chat_id);

    struct json_object *migrate_from_chat_id = NULL;
    if (json_object_object_get_ex(obj, "migrate_from_chat_id", &migrate_from_chat_id))
        msg->migrate_from_chat_id = json_object_get_int64(migrate_from_chat_id);

    struct json_object *pinned_message = NULL;
    if (json_object_object_get_ex(obj, "pinned_message", &pinned_message))
    {
        msg->pinned_message = calloc(1, sizeof(telebot_message_t));
        if (telebot_parser_get_message(pinned_message, msg->pinned_message) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <pinned_message> from message object");
            TELEBOT_SAFE_FREE(msg->pinned_message);
        }
    }

    struct json_object *connected_website = NULL;
    if (json_object_object_get_ex(obj, "connected_website", &connected_website))
        msg->connected_website = TELEBOT_SAFE_STRDUP(json_object_get_string(connected_website));

    struct json_object *invoice = NULL;
    if (json_object_object_get_ex(obj, "invoice", &invoice))
    {
        msg->invoice = calloc(1, sizeof(telebot_invoice_t));
        if (telebot_parser_get_invoice(invoice, msg->invoice) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <invoice> from message object");
            TELEBOT_SAFE_FREE(msg->invoice);
        }
    }

    struct json_object *successful_payment = NULL;
    if (json_object_object_get_ex(obj, "successful_payment", &successful_payment))
    {
        msg->successful_payment = calloc(1, sizeof(telebot_successful_payment_t));
        if (telebot_parser_get_successful_payment(successful_payment, msg->successful_payment) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <successful_payment> from message object");
            TELEBOT_SAFE_FREE(msg->successful_payment);
        }
    }

    struct json_object *refunded_payment = NULL;
    if (json_object_object_get_ex(obj, "refunded_payment", &refunded_payment))
    {
        msg->refunded_payment = calloc(1, sizeof(telebot_refunded_payment_t));
        if (telebot_parser_get_refunded_payment(refunded_payment, msg->refunded_payment) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <refunded_payment> from message object");
            TELEBOT_SAFE_FREE(msg->refunded_payment);
        }
    }

    struct json_object *write_access_allowed = NULL;
    if (json_object_object_get_ex(obj, "write_access_allowed", &write_access_allowed))
    {
        msg->write_access_allowed = calloc(1, sizeof(telebot_write_access_allowed_t));
        if (telebot_parser_get_write_access_allowed(write_access_allowed, msg->write_access_allowed) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <write_access_allowed> from message object");
            TELEBOT_SAFE_FREE(msg->write_access_allowed);
        }
    }

    struct json_object *passport_data = NULL;
    if (json_object_object_get_ex(obj, "passport_data", &passport_data))
    {
        msg->passport_data = calloc(1, sizeof(telebot_passport_data_t));
        if (telebot_parser_get_passport_data(passport_data, msg->passport_data) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <passport_data> from message object");
            TELEBOT_SAFE_FREE(msg->passport_data);
        }
    }

    struct json_object *proximity_alert_triggered = NULL;
    if (json_object_object_get_ex(obj, "proximity_alert_triggered", &proximity_alert_triggered))
    {
        msg->proximity_alert_triggered = calloc(1, sizeof(telebot_proximity_alert_triggered_t));
        if (telebot_parser_get_proximity_alert_triggered(proximity_alert_triggered, msg->proximity_alert_triggered) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <proximity_alert_triggered> from message object");
            TELEBOT_SAFE_FREE(msg->proximity_alert_triggered);
        }
    }

    struct json_object *forum_topic_created = NULL;
    if (json_object_object_get_ex(obj, "forum_topic_created", &forum_topic_created))
    {
        msg->forum_topic_created = calloc(1, sizeof(telebot_forum_topic_created_t));
        if (telebot_parser_get_forum_topic_created(forum_topic_created, msg->forum_topic_created) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <forum_topic_created> from message object");
            TELEBOT_SAFE_FREE(msg->forum_topic_created);
        }
    }

    struct json_object *forum_topic_edited = NULL;
    if (json_object_object_get_ex(obj, "forum_topic_edited", &forum_topic_edited))
    {
        msg->forum_topic_edited = calloc(1, sizeof(telebot_forum_topic_edited_t));
        if (telebot_parser_get_forum_topic_edited(forum_topic_edited, msg->forum_topic_edited) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <forum_topic_edited> from message object");
            TELEBOT_SAFE_FREE(msg->forum_topic_edited);
        }
    }

    struct json_object *forum_topic_closed = NULL;
    if (json_object_object_get_ex(obj, "forum_topic_closed", &forum_topic_closed))
    {
        msg->forum_topic_closed = calloc(1, sizeof(telebot_forum_topic_closed_t));
        msg->forum_topic_closed->dummy = true;
    }

    struct json_object *forum_topic_reopened = NULL;
    if (json_object_object_get_ex(obj, "forum_topic_reopened", &forum_topic_reopened))
    {
        msg->forum_topic_reopened = calloc(1, sizeof(telebot_forum_topic_reopened_t));
        msg->forum_topic_reopened->dummy = true;
    }

    struct json_object *video_chat_scheduled = NULL;
    if (json_object_object_get_ex(obj, "video_chat_scheduled", &video_chat_scheduled))
    {
        msg->video_chat_scheduled = calloc(1, sizeof(telebot_video_chat_scheduled_t));
        if (telebot_parser_get_video_chat_scheduled(video_chat_scheduled, msg->video_chat_scheduled) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <video_chat_scheduled> from message object");
            TELEBOT_SAFE_FREE(msg->video_chat_scheduled);
        }
    }

    struct json_object *video_chat_started = NULL;
    if (json_object_object_get_ex(obj, "video_chat_started", &video_chat_started))
    {
        msg->video_chat_started = calloc(1, sizeof(telebot_video_chat_started_t));
        msg->video_chat_started->dummy = true;
    }

    struct json_object *video_chat_ended = NULL;
    if (json_object_object_get_ex(obj, "video_chat_ended", &video_chat_ended))
    {
        msg->video_chat_ended = calloc(1, sizeof(telebot_video_chat_ended_t));
        if (telebot_parser_get_video_chat_ended(video_chat_ended, msg->video_chat_ended) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <video_chat_ended> from message object");
            TELEBOT_SAFE_FREE(msg->video_chat_ended);
        }
    }

    struct json_object *video_chat_participants_invited = NULL;
    if (json_object_object_get_ex(obj, "video_chat_participants_invited", &video_chat_participants_invited))
    {
        msg->video_chat_participants_invited = calloc(1, sizeof(telebot_video_chat_participants_invited_t));
        if (telebot_parser_get_video_chat_participants_invited(video_chat_participants_invited, msg->video_chat_participants_invited) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <video_chat_participants_invited> from message object");
            TELEBOT_SAFE_FREE(msg->video_chat_participants_invited);
        }
    }

    struct json_object *web_app_data = NULL;
    if (json_object_object_get_ex(obj, "web_app_data", &web_app_data))
    {
        msg->web_app_data = calloc(1, sizeof(telebot_web_app_data_t));
        if (telebot_parser_get_web_app_data(web_app_data, msg->web_app_data) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <web_app_data> from message object");
            TELEBOT_SAFE_FREE(msg->web_app_data);
        }
    }

    struct json_object *users_shared = NULL;
    if (json_object_object_get_ex(obj, "users_shared", &users_shared))
    {
        msg->users_shared = calloc(1, sizeof(telebot_users_shared_t));
        if (telebot_parser_get_users_shared(users_shared, msg->users_shared) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <users_shared> from message object");
            TELEBOT_SAFE_FREE(msg->users_shared);
        }
    }

    struct json_object *chat_shared = NULL;
    if (json_object_object_get_ex(obj, "chat_shared", &chat_shared))
    {
        msg->chat_shared = calloc(1, sizeof(telebot_chat_shared_t));
        if (telebot_parser_get_chat_shared(chat_shared, msg->chat_shared) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <chat_shared> from message object");
            TELEBOT_SAFE_FREE(msg->chat_shared);
        }
    }

    struct json_object *gift = NULL;
    if (json_object_object_get_ex(obj, "gift", &gift))
    {
        msg->gift = calloc(1, sizeof(telebot_gift_info_t));
        if (telebot_parser_get_gift_info(gift, msg->gift) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <gift> from message object");
            TELEBOT_SAFE_FREE(msg->gift);
        }
    }

    struct json_object *unique_gift = NULL;
    if (json_object_object_get_ex(obj, "unique_gift", &unique_gift))
    {
        msg->unique_gift = calloc(1, sizeof(telebot_unique_gift_info_t));
        if (telebot_parser_get_unique_gift_info(unique_gift, msg->unique_gift) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <unique_gift> from message object");
            TELEBOT_SAFE_FREE(msg->unique_gift);
        }
    }

    struct json_object *boost_added = NULL;
    if (json_object_object_get_ex(obj, "boost_added", &boost_added))
    {
        msg->boost_added = calloc(1, sizeof(telebot_chat_boost_added_t));
        if (telebot_parser_get_chat_boost_added(boost_added, msg->boost_added) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <boost_added> from message object");
            TELEBOT_SAFE_FREE(msg->boost_added);
        }
    }

    struct json_object *chat_background_set = NULL;
    if (json_object_object_get_ex(obj, "chat_background_set", &chat_background_set))
    {
        msg->chat_background_set = calloc(1, sizeof(telebot_chat_background_t));
        if (telebot_parser_get_chat_background(chat_background_set, msg->chat_background_set) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <chat_background_set> from message object");
            TELEBOT_SAFE_FREE(msg->chat_background_set);
        }
    }

    struct json_object *giveaway_created = NULL;
    if (json_object_object_get_ex(obj, "giveaway_created", &giveaway_created))
    {
        msg->giveaway_created = calloc(1, sizeof(telebot_giveaway_created_t));
        if (telebot_parser_get_giveaway_created(giveaway_created, msg->giveaway_created) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <giveaway_created> from message object");
            TELEBOT_SAFE_FREE(msg->giveaway_created);
        }
    }

    struct json_object *giveaway_completed = NULL;
    if (json_object_object_get_ex(obj, "giveaway_completed", &giveaway_completed))
    {
        msg->giveaway_completed = calloc(1, sizeof(telebot_giveaway_completed_t));
        if (telebot_parser_get_giveaway_completed(giveaway_completed, msg->giveaway_completed) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <giveaway_completed> from message object");
            TELEBOT_SAFE_FREE(msg->giveaway_completed);
        }
    }

    struct json_object *reply_markup = NULL;
    if (json_object_object_get_ex(obj, "reply_markup", &reply_markup))
    {
        msg->reply_markup = calloc(1, sizeof(telebot_inline_keyboard_markup_t));
        if (telebot_parser_get_inline_keyboard_markup(reply_markup, msg->reply_markup) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <reply_markup> from message object");
            TELEBOT_SAFE_FREE(msg->reply_markup);
        }
    }

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
        entity->user = calloc(1, sizeof(telebot_user_t));
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
        audio->thumb = calloc(1, sizeof(telebot_photo_t));
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
        document->thumb = calloc(1, sizeof(telebot_photo_t));
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
        video->thumb = calloc(1, sizeof(telebot_photo_t));
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
        animation->thumb = calloc(1, sizeof(telebot_photo_t));
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
        video_note->thumb = calloc(1, sizeof(telebot_photo_t));
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
        venue->location = calloc(1, sizeof(telebot_location_t));
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

    memset(dice, 0, sizeof(telebot_dice_t));
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
        cb_query->from = calloc(1, sizeof(telebot_user_t));
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
        cb_query->message = calloc(1, sizeof(telebot_message_t));
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
        member->user = calloc(1, sizeof(telebot_user_t));
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
        resp_param->migrate_to_chat_id = json_object_get_int64(migrate_to_chat_id);

    struct json_object *retry_after = NULL;
    if (json_object_object_get_ex(obj, "retry_after", &retry_after))
        resp_param->retry_after = json_object_get_int(retry_after);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_location(struct json_object *obj, telebot_chat_location_t *chat_location)
{
    if ((obj == NULL) || (chat_location == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *address = NULL;
    if (!json_object_object_get_ex(obj, "address", &address))
    {
        ERR("Object is not chat type, address not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    chat_location->address = TELEBOT_SAFE_STRDUP(json_object_get_string(address));

    struct json_object *location = NULL;
    if (json_object_object_get_ex(obj, "location", &location))
    {
        chat_location->location = malloc(sizeof(telebot_location_t));
        if (telebot_parser_get_location(location, chat_location->location) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <location> from chat object");
            TELEBOT_SAFE_FREE(chat_location->location);
            TELEBOT_SAFE_FREE(chat_location->address);
            return TELEBOT_ERROR_OPERATION_FAILED;
        }
    }
    else
    {
        ERR("Object is not chat location type, location not found");
        TELEBOT_SAFE_FREE(chat_location->address);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e
telebot_parser_get_message_auto_delete_timer_changed(struct json_object *obj,
                                                     telebot_message_auto_delete_timer_changed_t *timer_changed)
{
    if ((obj == NULL) || (timer_changed == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct json_object *message_auto_delete_time = NULL;
    if (!json_object_object_get_ex(obj, "message_auto_delete_time", &message_auto_delete_time))
    {
        ERR("Object is not message auto-delete timer type, message_auto_delete_time not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    timer_changed->message_auto_delete_time = json_object_get_int(message_auto_delete_time);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_invoice(struct json_object *obj, telebot_invoice_t *invoice)
{
    if ((obj == NULL) || (invoice == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(invoice, 0, sizeof(telebot_invoice_t));
    struct json_object *title = NULL;
    if (json_object_object_get_ex(obj, "title", &title))
        invoice->title = TELEBOT_SAFE_STRDUP(json_object_get_string(title));

    struct json_object *description = NULL;
    if (json_object_object_get_ex(obj, "description", &description))
        invoice->description = TELEBOT_SAFE_STRDUP(json_object_get_string(description));

    struct json_object *start_parameter = NULL;
    if (json_object_object_get_ex(obj, "start_parameter", &start_parameter))
        invoice->start_parameter = TELEBOT_SAFE_STRDUP(json_object_get_string(start_parameter));

    struct json_object *currency = NULL;
    if (json_object_object_get_ex(obj, "currency", &currency))
        invoice->currency = TELEBOT_SAFE_STRDUP(json_object_get_string(currency));

    struct json_object *total_amount = NULL;
    if (json_object_object_get_ex(obj, "total_amount", &total_amount))
        invoice->total_amount = json_object_get_int(total_amount);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_shipping_address(struct json_object *obj, telebot_shipping_address_t *address)
{
    if ((obj == NULL) || (address == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(address, 0, sizeof(telebot_shipping_address_t));
    struct json_object *country_code = NULL;
    if (json_object_object_get_ex(obj, "country_code", &country_code))
        address->country_code = TELEBOT_SAFE_STRDUP(json_object_get_string(country_code));

    struct json_object *state = NULL;
    if (json_object_object_get_ex(obj, "state", &state))
        address->state = TELEBOT_SAFE_STRDUP(json_object_get_string(state));

    struct json_object *city = NULL;
    if (json_object_object_get_ex(obj, "city", &city))
        address->city = TELEBOT_SAFE_STRDUP(json_object_get_string(city));

    struct json_object *street_line1 = NULL;
    if (json_object_object_get_ex(obj, "street_line1", &street_line1))
        address->street_line1 = TELEBOT_SAFE_STRDUP(json_object_get_string(street_line1));

    struct json_object *street_line2 = NULL;
    if (json_object_object_get_ex(obj, "street_line2", &street_line2))
        address->street_line2 = TELEBOT_SAFE_STRDUP(json_object_get_string(street_line2));

    struct json_object *post_code = NULL;
    if (json_object_object_get_ex(obj, "post_code", &post_code))
        address->post_code = TELEBOT_SAFE_STRDUP(json_object_get_string(post_code));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_order_info(struct json_object *obj, telebot_order_info_t *info)
{
    if ((obj == NULL) || (info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(info, 0, sizeof(telebot_order_info_t));
    struct json_object *name = NULL;
    if (json_object_object_get_ex(obj, "name", &name))
        info->name = TELEBOT_SAFE_STRDUP(json_object_get_string(name));

    struct json_object *phone_number = NULL;
    if (json_object_object_get_ex(obj, "phone_number", &phone_number))
        info->phone_number = TELEBOT_SAFE_STRDUP(json_object_get_string(phone_number));

    struct json_object *email = NULL;
    if (json_object_object_get_ex(obj, "email", &email))
        info->email = TELEBOT_SAFE_STRDUP(json_object_get_string(email));

    struct json_object *shipping_address = NULL;
    if (json_object_object_get_ex(obj, "shipping_address", &shipping_address))
    {
        info->shipping_address = calloc(1, sizeof(telebot_shipping_address_t));
        telebot_parser_get_shipping_address(shipping_address, info->shipping_address);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_successful_payment(struct json_object *obj, telebot_successful_payment_t *payment)
{
    if ((obj == NULL) || (payment == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(payment, 0, sizeof(telebot_successful_payment_t));
    struct json_object *currency = NULL;
    if (json_object_object_get_ex(obj, "currency", &currency))
        payment->currency = TELEBOT_SAFE_STRDUP(json_object_get_string(currency));

    struct json_object *total_amount = NULL;
    if (json_object_object_get_ex(obj, "total_amount", &total_amount))
        payment->total_amount = json_object_get_int(total_amount);

    struct json_object *invoice_payload = NULL;
    if (json_object_object_get_ex(obj, "invoice_payload", &invoice_payload))
        payment->invoice_payload = TELEBOT_SAFE_STRDUP(json_object_get_string(invoice_payload));

    struct json_object *shipping_option_id = NULL;
    if (json_object_object_get_ex(obj, "shipping_option_id", &shipping_option_id))
        payment->shipping_option_id = TELEBOT_SAFE_STRDUP(json_object_get_string(shipping_option_id));

    struct json_object *order_info = NULL;
    if (json_object_object_get_ex(obj, "order_info", &order_info))
    {
        payment->order_info = calloc(1, sizeof(telebot_order_info_t));
        telebot_parser_get_order_info(order_info, payment->order_info);
    }

    struct json_object *telegram_payment_charge_id = NULL;
    if (json_object_object_get_ex(obj, "telegram_payment_charge_id", &telegram_payment_charge_id))
        payment->telegram_payment_charge_id = TELEBOT_SAFE_STRDUP(json_object_get_string(telegram_payment_charge_id));

    struct json_object *provider_payment_charge_id = NULL;
    if (json_object_object_get_ex(obj, "provider_payment_charge_id", &provider_payment_charge_id))
        payment->provider_payment_charge_id = TELEBOT_SAFE_STRDUP(json_object_get_string(provider_payment_charge_id));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_passport_file(struct json_object *obj, telebot_passport_file_t *file)
{
    if ((obj == NULL) || (file == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(file, 0, sizeof(telebot_passport_file_t));
    struct json_object *file_id = NULL;
    if (json_object_object_get_ex(obj, "file_id", &file_id))
        file->file_id = TELEBOT_SAFE_STRDUP(json_object_get_string(file_id));

    struct json_object *file_unique_id = NULL;
    if (json_object_object_get_ex(obj, "file_unique_id", &file_unique_id))
        file->file_unique_id = TELEBOT_SAFE_STRDUP(json_object_get_string(file_unique_id));

    struct json_object *file_size = NULL;
    if (json_object_object_get_ex(obj, "file_size", &file_size))
        file->file_size = json_object_get_int(file_size);

    struct json_object *date = NULL;
    if (json_object_object_get_ex(obj, "date", &date))
        file->date = json_object_get_int(date);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_encrypted_passport_element(struct json_object *obj, telebot_encrypted_passport_element_t *element)
{
    if ((obj == NULL) || (element == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(element, 0, sizeof(telebot_encrypted_passport_element_t));
    struct json_object *type = NULL;
    if (json_object_object_get_ex(obj, "type", &type))
        element->type = TELEBOT_SAFE_STRDUP(json_object_get_string(type));

    struct json_object *data = NULL;
    if (json_object_object_get_ex(obj, "data", &data))
        element->data = TELEBOT_SAFE_STRDUP(json_object_get_string(data));

    struct json_object *phone_number = NULL;
    if (json_object_object_get_ex(obj, "phone_number", &phone_number))
        element->phone_number = TELEBOT_SAFE_STRDUP(json_object_get_string(phone_number));

    struct json_object *email = NULL;
    if (json_object_object_get_ex(obj, "email", &email))
        element->email = TELEBOT_SAFE_STRDUP(json_object_get_string(email));

    struct json_object *files = NULL;
    if (json_object_object_get_ex(obj, "files", &files))
    {
        int array_len = json_object_array_length(files);
        element->count_files = array_len;
        element->files = calloc(array_len, sizeof(telebot_passport_file_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_passport_file(json_object_array_get_idx(files, i), &(element->files[i]));
    }

    struct json_object *front_side = NULL;
    if (json_object_object_get_ex(obj, "front_side", &front_side))
    {
        element->front_side = calloc(1, sizeof(telebot_passport_file_t));
        telebot_parser_get_passport_file(front_side, element->front_side);
    }

    struct json_object *reverse_side = NULL;
    if (json_object_object_get_ex(obj, "reverse_side", &reverse_side))
    {
        element->reverse_side = calloc(1, sizeof(telebot_passport_file_t));
        telebot_parser_get_passport_file(reverse_side, element->reverse_side);
    }

    struct json_object *selfie = NULL;
    if (json_object_object_get_ex(obj, "selfie", &selfie))
    {
        element->selfie = calloc(1, sizeof(telebot_passport_file_t));
        telebot_parser_get_passport_file(selfie, element->selfie);
    }

    struct json_object *translation = NULL;
    if (json_object_object_get_ex(obj, "translation", &translation))
    {
        int array_len = json_object_array_length(translation);
        element->count_translation = array_len;
        element->translation = calloc(array_len, sizeof(telebot_passport_file_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_passport_file(json_object_array_get_idx(translation, i), &(element->translation[i]));
    }

    struct json_object *hash = NULL;
    if (json_object_object_get_ex(obj, "hash", &hash))
        element->hash = TELEBOT_SAFE_STRDUP(json_object_get_string(hash));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_encrypted_credentials(struct json_object *obj, telebot_encrypted_credentials_t *credentials)
{
    if ((obj == NULL) || (credentials == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(credentials, 0, sizeof(telebot_encrypted_credentials_t));
    struct json_object *data = NULL;
    if (json_object_object_get_ex(obj, "data", &data))
        credentials->data = TELEBOT_SAFE_STRDUP(json_object_get_string(data));

    struct json_object *hash = NULL;
    if (json_object_object_get_ex(obj, "hash", &hash))
        credentials->hash = TELEBOT_SAFE_STRDUP(json_object_get_string(hash));

    struct json_object *secret = NULL;
    if (json_object_object_get_ex(obj, "secret", &secret))
        credentials->secret = TELEBOT_SAFE_STRDUP(json_object_get_string(secret));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_passport_data(struct json_object *obj, telebot_passport_data_t *passport_data)
{
    if ((obj == NULL) || (passport_data == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(passport_data, 0, sizeof(telebot_passport_data_t));
    struct json_object *data = NULL;
    if (json_object_object_get_ex(obj, "data", &data))
    {
        int array_len = json_object_array_length(data);
        passport_data->count_data = array_len;
        passport_data->data = calloc(array_len, sizeof(telebot_encrypted_passport_element_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_encrypted_passport_element(json_object_array_get_idx(data, i), &(passport_data->data[i]));
    }

    struct json_object *credentials = NULL;
    if (json_object_object_get_ex(obj, "credentials", &credentials))
    {
        passport_data->credentials = calloc(1, sizeof(telebot_encrypted_credentials_t));
        telebot_parser_get_encrypted_credentials(credentials, passport_data->credentials);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_proximity_alert_triggered(struct json_object *obj, telebot_proximity_alert_triggered_t *alert)
{
    if ((obj == NULL) || (alert == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(alert, 0, sizeof(telebot_proximity_alert_triggered_t));
    struct json_object *traveler = NULL;
    if (json_object_object_get_ex(obj, "traveler", &traveler))
    {
        alert->traveler = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(traveler, alert->traveler);
    }

    struct json_object *watcher = NULL;
    if (json_object_object_get_ex(obj, "watcher", &watcher))
    {
        alert->watcher = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(watcher, alert->watcher);
    }

    struct json_object *distance = NULL;
    if (json_object_object_get_ex(obj, "distance", &distance))
        alert->distance = json_object_get_int(distance);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_forum_topic_created(struct json_object *obj, telebot_forum_topic_created_t *topic)
{
    if ((obj == NULL) || (topic == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(topic, 0, sizeof(telebot_forum_topic_created_t));
    struct json_object *name = NULL;
    if (json_object_object_get_ex(obj, "name", &name))
        topic->name = TELEBOT_SAFE_STRDUP(json_object_get_string(name));

    struct json_object *icon_color = NULL;
    if (json_object_object_get_ex(obj, "icon_color", &icon_color))
        topic->icon_color = json_object_get_int(icon_color);

    struct json_object *icon_custom_emoji_id = NULL;
    if (json_object_object_get_ex(obj, "icon_custom_emoji_id", &icon_custom_emoji_id))
        topic->icon_custom_emoji_id = TELEBOT_SAFE_STRDUP(json_object_get_string(icon_custom_emoji_id));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_forum_topic_edited(struct json_object *obj, telebot_forum_topic_edited_t *topic)
{
    if ((obj == NULL) || (topic == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(topic, 0, sizeof(telebot_forum_topic_edited_t));
    struct json_object *name = NULL;
    if (json_object_object_get_ex(obj, "name", &name))
        topic->name = TELEBOT_SAFE_STRDUP(json_object_get_string(name));

    struct json_object *icon_custom_emoji_id = NULL;
    if (json_object_object_get_ex(obj, "icon_custom_emoji_id", &icon_custom_emoji_id))
        topic->icon_custom_emoji_id = TELEBOT_SAFE_STRDUP(json_object_get_string(icon_custom_emoji_id));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_video_chat_scheduled(struct json_object *obj, telebot_video_chat_scheduled_t *scheduled)
{
    if ((obj == NULL) || (scheduled == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(scheduled, 0, sizeof(telebot_video_chat_scheduled_t));
    struct json_object *start_date = NULL;
    if (json_object_object_get_ex(obj, "start_date", &start_date))
        scheduled->start_date = json_object_get_int(start_date);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_video_chat_ended(struct json_object *obj, telebot_video_chat_ended_t *ended)
{
    if ((obj == NULL) || (ended == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(ended, 0, sizeof(telebot_video_chat_ended_t));
    struct json_object *duration = NULL;
    if (json_object_object_get_ex(obj, "duration", &duration))
        ended->duration = json_object_get_int(duration);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_video_chat_participants_invited(struct json_object *obj, telebot_video_chat_participants_invited_t *invited)
{
    if ((obj == NULL) || (invited == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(invited, 0, sizeof(telebot_video_chat_participants_invited_t));
    struct json_object *users = NULL;
    if (json_object_object_get_ex(obj, "users", &users))
    {
        int array_len = json_object_array_length(users);
        invited->count_users = array_len;
        invited->users = calloc(array_len, sizeof(telebot_user_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_user(json_object_array_get_idx(users, i), &(invited->users[i]));
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_web_app_data(struct json_object *obj, telebot_web_app_data_t *data)
{
    if ((obj == NULL) || (data == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(data, 0, sizeof(telebot_web_app_data_t));
    struct json_object *data_obj = NULL;
    if (json_object_object_get_ex(obj, "data", &data_obj))
        data->data = TELEBOT_SAFE_STRDUP(json_object_get_string(data_obj));

    struct json_object *button_text = NULL;
    if (json_object_object_get_ex(obj, "button_text", &button_text))
        data->button_text = TELEBOT_SAFE_STRDUP(json_object_get_string(button_text));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_inline_keyboard_button(struct json_object *obj, telebot_inline_keyboard_button_t *button)
{
    if ((obj == NULL) || (button == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(button, 0, sizeof(telebot_inline_keyboard_button_t));
    struct json_object *text = NULL;
    if (json_object_object_get_ex(obj, "text", &text))
        button->text = TELEBOT_SAFE_STRDUP(json_object_get_string(text));

    struct json_object *url = NULL;
    if (json_object_object_get_ex(obj, "url", &url))
        button->url = TELEBOT_SAFE_STRDUP(json_object_get_string(url));

    struct json_object *callback_data = NULL;
    if (json_object_object_get_ex(obj, "callback_data", &callback_data))
        button->callback_data = TELEBOT_SAFE_STRDUP(json_object_get_string(callback_data));

    struct json_object *switch_inline_query = NULL;
    if (json_object_object_get_ex(obj, "switch_inline_query", &switch_inline_query))
        button->switch_inline_query = TELEBOT_SAFE_STRDUP(json_object_get_string(switch_inline_query));

    struct json_object *switch_inline_query_current_chat = NULL;
    if (json_object_object_get_ex(obj, "switch_inline_query_current_chat", &switch_inline_query_current_chat))
        button->switch_inline_query_current_chat = TELEBOT_SAFE_STRDUP(json_object_get_string(switch_inline_query_current_chat));

    struct json_object *pay = NULL;
    if (json_object_object_get_ex(obj, "pay", &pay))
        button->pay = json_object_get_boolean(pay);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_inline_keyboard_markup(struct json_object *obj, telebot_inline_keyboard_markup_t *markup)
{
    if ((obj == NULL) || (markup == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(markup, 0, sizeof(telebot_inline_keyboard_markup_t));
    struct json_object *inline_keyboard = NULL;
    if (json_object_object_get_ex(obj, "inline_keyboard", &inline_keyboard))
    {
        int rows = json_object_array_length(inline_keyboard);
        int cols = 0;
        if (rows > 0)
        {
            struct json_object *row0 = json_object_array_get_idx(inline_keyboard, 0);
            cols = json_object_array_length(row0);
        }
        markup->rows = rows;
        markup->cols = cols;
        markup->inline_keyboard = calloc(rows * cols, sizeof(telebot_inline_keyboard_button_t));
        for (int i = 0; i < rows; i++)
        {
            struct json_object *row = json_object_array_get_idx(inline_keyboard, i);
            int row_cols = json_object_array_length(row);
            for (int j = 0; j < row_cols && j < cols; j++)
            {
                telebot_parser_get_inline_keyboard_button(json_object_array_get_idx(row, j), &(markup->inline_keyboard[i * cols + j]));
            }
        }
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_invite_link(struct json_object *obj, telebot_chat_invite_link_t *invite_link)
{
    if ((obj == NULL) || (invite_link == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(invite_link, 0, sizeof(telebot_chat_invite_link_t));
    struct json_object *invite_link_obj = NULL;
    if (json_object_object_get_ex(obj, "invite_link", &invite_link_obj))
        invite_link->invite_link = TELEBOT_SAFE_STRDUP(json_object_get_string(invite_link_obj));

    struct json_object *creator = NULL;
    if (json_object_object_get_ex(obj, "creator", &creator))
    {
        invite_link->creator = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(creator, invite_link->creator);
    }

    struct json_object *creates_join_request = NULL;
    if (json_object_object_get_ex(obj, "creates_join_request", &creates_join_request))
        invite_link->creates_join_request = json_object_get_boolean(creates_join_request);

    struct json_object *is_primary = NULL;
    if (json_object_object_get_ex(obj, "is_primary", &is_primary))
        invite_link->is_primary = json_object_get_boolean(is_primary);

    struct json_object *is_revoked = NULL;
    if (json_object_object_get_ex(obj, "is_revoked", &is_revoked))
        invite_link->is_revoked = json_object_get_boolean(is_revoked);

    struct json_object *name = NULL;
    if (json_object_object_get_ex(obj, "name", &name))
        invite_link->name = TELEBOT_SAFE_STRDUP(json_object_get_string(name));

    struct json_object *expire_date = NULL;
    if (json_object_object_get_ex(obj, "expire_date", &expire_date))
        invite_link->expire_date = json_object_get_int(expire_date);

    struct json_object *member_limit = NULL;
    if (json_object_object_get_ex(obj, "member_limit", &member_limit))
        invite_link->member_limit = json_object_get_int(member_limit);

    struct json_object *pending_join_request_count = NULL;
    if (json_object_object_get_ex(obj, "pending_join_request_count", &pending_join_request_count))
        invite_link->pending_join_request_count = json_object_get_int(pending_join_request_count);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_member_updated(struct json_object *obj, telebot_chat_member_updated_t *updated)
{
    if ((obj == NULL) || (updated == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(updated, 0, sizeof(telebot_chat_member_updated_t));
    struct json_object *chat = NULL;
    if (json_object_object_get_ex(obj, "chat", &chat))
    {
        updated->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, updated->chat);
    }

    struct json_object *from = NULL;
    if (json_object_object_get_ex(obj, "from", &from))
    {
        updated->from = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(from, updated->from);
    }

    struct json_object *date = NULL;
    if (json_object_object_get_ex(obj, "date", &date))
        updated->date = json_object_get_int(date);

    struct json_object *old_chat_member = NULL;
    if (json_object_object_get_ex(obj, "old_chat_member", &old_chat_member))
    {
        updated->old_chat_member = calloc(1, sizeof(telebot_chat_member_t));
        telebot_parser_get_chat_member(old_chat_member, updated->old_chat_member);
    }

    struct json_object *new_chat_member = NULL;
    if (json_object_object_get_ex(obj, "new_chat_member", &new_chat_member))
    {
        updated->new_chat_member = calloc(1, sizeof(telebot_chat_member_t));
        telebot_parser_get_chat_member(new_chat_member, updated->new_chat_member);
    }

    struct json_object *invite_link = NULL;
    if (json_object_object_get_ex(obj, "invite_link", &invite_link))
    {
        updated->invite_link = calloc(1, sizeof(telebot_chat_invite_link_t));
        telebot_parser_get_chat_invite_link(invite_link, updated->invite_link);
    }

    struct json_object *via_chat_folder_invite_link = NULL;
    if (json_object_object_get_ex(obj, "via_chat_folder_invite_link", &via_chat_folder_invite_link))
        updated->via_chat_folder_invite_link = json_object_get_boolean(via_chat_folder_invite_link);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_join_request(struct json_object *obj, telebot_chat_join_request_t *request)
{
    if ((obj == NULL) || (request == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(request, 0, sizeof(telebot_chat_join_request_t));
    struct json_object *chat = NULL;
    if (json_object_object_get_ex(obj, "chat", &chat))
    {
        request->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, request->chat);
    }

    struct json_object *from = NULL;
    if (json_object_object_get_ex(obj, "from", &from))
    {
        request->from = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(from, request->from);
    }

    struct json_object *user_chat_id = NULL;
    if (json_object_object_get_ex(obj, "user_chat_id", &user_chat_id))
        request->user_chat_id = json_object_get_int64(user_chat_id);

    struct json_object *date = NULL;
    if (json_object_object_get_ex(obj, "date", &date))
        request->date = json_object_get_int(date);

    struct json_object *bio = NULL;
    if (json_object_object_get_ex(obj, "bio", &bio))
        request->bio = TELEBOT_SAFE_STRDUP(json_object_get_string(bio));

    struct json_object *invite_link = NULL;
    if (json_object_object_get_ex(obj, "invite_link", &invite_link))
    {
        request->invite_link = calloc(1, sizeof(telebot_chat_invite_link_t));
        telebot_parser_get_chat_invite_link(invite_link, request->invite_link);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_reaction_type(struct json_object *obj, telebot_reaction_type_t *reaction)
{
    if ((obj == NULL) || (reaction == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(reaction, 0, sizeof(telebot_reaction_type_t));
    struct json_object *type = NULL;
    if (json_object_object_get_ex(obj, "type", &type))
        reaction->type = TELEBOT_SAFE_STRDUP(json_object_get_string(type));

    struct json_object *emoji = NULL;
    if (json_object_object_get_ex(obj, "emoji", &emoji))
        reaction->emoji = TELEBOT_SAFE_STRDUP(json_object_get_string(emoji));

    struct json_object *custom_emoji_id = NULL;
    if (json_object_object_get_ex(obj, "custom_emoji_id", &custom_emoji_id))
        reaction->custom_emoji_id = TELEBOT_SAFE_STRDUP(json_object_get_string(custom_emoji_id));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_message_reaction_updated(struct json_object *obj, telebot_message_reaction_updated_t *updated)
{
    if ((obj == NULL) || (updated == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(updated, 0, sizeof(telebot_message_reaction_updated_t));
    struct json_object *chat = NULL;
    if (json_object_object_get_ex(obj, "chat", &chat))
    {
        updated->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, updated->chat);
    }

    struct json_object *message_id = NULL;
    if (json_object_object_get_ex(obj, "message_id", &message_id))
        updated->message_id = json_object_get_int(message_id);

    struct json_object *user = NULL;
    if (json_object_object_get_ex(obj, "user", &user))
    {
        updated->user = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(user, updated->user);
    }

    struct json_object *actor_chat = NULL;
    if (json_object_object_get_ex(obj, "actor_chat", &actor_chat))
    {
        updated->actor_chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(actor_chat, updated->actor_chat);
    }

    struct json_object *date = NULL;
    if (json_object_object_get_ex(obj, "date", &date))
        updated->date = json_object_get_int(date);

    struct json_object *old_reaction = NULL;
    if (json_object_object_get_ex(obj, "old_reaction", &old_reaction))
    {
        int array_len = json_object_array_length(old_reaction);
        updated->count_old_reaction = array_len;
        updated->old_reaction = calloc(array_len, sizeof(telebot_reaction_type_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_reaction_type(json_object_array_get_idx(old_reaction, i), &(updated->old_reaction[i]));
    }

    struct json_object *new_reaction = NULL;
    if (json_object_object_get_ex(obj, "new_reaction", &new_reaction))
    {
        int array_len = json_object_array_length(new_reaction);
        updated->count_new_reaction = array_len;
        updated->new_reaction = calloc(array_len, sizeof(telebot_reaction_type_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_reaction_type(json_object_array_get_idx(new_reaction, i), &(updated->new_reaction[i]));
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_reaction_count(struct json_object *obj, telebot_reaction_count_t *count)
{
    if ((obj == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(count, 0, sizeof(telebot_reaction_count_t));
    struct json_object *type = NULL;
    if (json_object_object_get_ex(obj, "type", &type))
        telebot_parser_get_reaction_type(type, &(count->type));

    struct json_object *total_count = NULL;
    if (json_object_object_get_ex(obj, "total_count", &total_count))
        count->total_count = json_object_get_int(total_count);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_message_reaction_count_updated(struct json_object *obj, telebot_message_reaction_count_updated_t *updated)
{
    if ((obj == NULL) || (updated == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(updated, 0, sizeof(telebot_message_reaction_count_updated_t));
    struct json_object *chat = NULL;
    if (json_object_object_get_ex(obj, "chat", &chat))
    {
        updated->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, updated->chat);
    }

    struct json_object *message_id = NULL;
    if (json_object_object_get_ex(obj, "message_id", &message_id))
        updated->message_id = json_object_get_int(message_id);

    struct json_object *date = NULL;
    if (json_object_object_get_ex(obj, "date", &date))
        updated->date = json_object_get_int(date);

    struct json_object *reactions = NULL;
    if (json_object_object_get_ex(obj, "reactions", &reactions))
    {
        int array_len = json_object_array_length(reactions);
        updated->count_reactions = array_len;
        updated->reactions = calloc(array_len, sizeof(telebot_reaction_count_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_reaction_count(json_object_array_get_idx(reactions, i), &(updated->reactions[i]));
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_boost_source(struct json_object *obj, telebot_chat_boost_source_t *source)
{
    if ((obj == NULL) || (source == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(source, 0, sizeof(telebot_chat_boost_source_t));
    struct json_object *source_obj = NULL;
    if (json_object_object_get_ex(obj, "source", &source_obj))
        source->source = TELEBOT_SAFE_STRDUP(json_object_get_string(source_obj));

    struct json_object *user = NULL;
    if (json_object_object_get_ex(obj, "user", &user))
    {
        source->user = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(user, source->user);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_boost(struct json_object *obj, telebot_chat_boost_t *boost)
{
    if ((obj == NULL) || (boost == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(boost, 0, sizeof(telebot_chat_boost_t));
    struct json_object *boost_id = NULL;
    if (json_object_object_get_ex(obj, "boost_id", &boost_id))
        boost->boost_id = TELEBOT_SAFE_STRDUP(json_object_get_string(boost_id));

    struct json_object *add_date = NULL;
    if (json_object_object_get_ex(obj, "add_date", &add_date))
        boost->add_date = json_object_get_int(add_date);

    struct json_object *expiration_date = NULL;
    if (json_object_object_get_ex(obj, "expiration_date", &expiration_date))
        boost->expiration_date = json_object_get_int(expiration_date);

    struct json_object *source = NULL;
    if (json_object_object_get_ex(obj, "source", &source))
    {
        boost->source = calloc(1, sizeof(telebot_chat_boost_source_t));
        telebot_parser_get_chat_boost_source(source, boost->source);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_boost_updated(struct json_object *obj, telebot_chat_boost_updated_t *updated)
{
    if ((obj == NULL) || (updated == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(updated, 0, sizeof(telebot_chat_boost_updated_t));
    struct json_object *chat = NULL;
    if (json_object_object_get_ex(obj, "chat", &chat))
    {
        updated->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, updated->chat);
    }

    struct json_object *boost = NULL;
    if (json_object_object_get_ex(obj, "boost", &boost))
    {
        updated->boost = calloc(1, sizeof(telebot_chat_boost_t));
        telebot_parser_get_chat_boost(boost, updated->boost);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_boost_removed(struct json_object *obj, telebot_chat_boost_removed_t *removed)
{
    if ((obj == NULL) || (removed == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(removed, 0, sizeof(telebot_chat_boost_removed_t));
    struct json_object *chat = NULL;
    if (json_object_object_get_ex(obj, "chat", &chat))
    {
        removed->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, removed->chat);
    }

    struct json_object *boost_id = NULL;
    if (json_object_object_get_ex(obj, "boost_id", &boost_id))
        removed->boost_id = TELEBOT_SAFE_STRDUP(json_object_get_string(boost_id));

    struct json_object *remove_date = NULL;
    if (json_object_object_get_ex(obj, "remove_date", &remove_date))
        removed->remove_date = json_object_get_int(remove_date);

    struct json_object *source = NULL;
    if (json_object_object_get_ex(obj, "source", &source))
    {
        removed->source = calloc(1, sizeof(telebot_chat_boost_source_t));
        telebot_parser_get_chat_boost_source(source, removed->source);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_forum_topic(struct json_object *obj, telebot_forum_topic_t *topic)
{
    if ((obj == NULL) || (topic == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(topic, 0, sizeof(telebot_forum_topic_t));
    struct json_object *message_thread_id = NULL;
    if (json_object_object_get_ex(obj, "message_thread_id", &message_thread_id))
        topic->message_thread_id = json_object_get_int(message_thread_id);

    struct json_object *name = NULL;
    if (json_object_object_get_ex(obj, "name", &name))
        topic->name = TELEBOT_SAFE_STRDUP(json_object_get_string(name));

    struct json_object *icon_color = NULL;
    if (json_object_object_get_ex(obj, "icon_color", &icon_color))
        topic->icon_color = json_object_get_int(icon_color);

    struct json_object *icon_custom_emoji_id = NULL;
    if (json_object_object_get_ex(obj, "icon_custom_emoji_id", &icon_custom_emoji_id))
        topic->icon_custom_emoji_id = TELEBOT_SAFE_STRDUP(json_object_get_string(icon_custom_emoji_id));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_inline_query(struct json_object *obj, telebot_inline_query_t *query)
{
    if ((obj == NULL) || (query == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(query, 0, sizeof(telebot_inline_query_t));
    struct json_object *id = NULL;
    if (json_object_object_get_ex(obj, "id", &id))
        query->id = TELEBOT_SAFE_STRDUP(json_object_get_string(id));

    struct json_object *from = NULL;
    if (json_object_object_get_ex(obj, "from", &from))
    {
        query->from = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(from, query->from);
    }

    struct json_object *query_obj = NULL;
    if (json_object_object_get_ex(obj, "query", &query_obj))
        query->query = TELEBOT_SAFE_STRDUP(json_object_get_string(query_obj));

    struct json_object *offset = NULL;
    if (json_object_object_get_ex(obj, "offset", &offset))
        query->offset = TELEBOT_SAFE_STRDUP(json_object_get_string(offset));

    struct json_object *chat_type = NULL;
    if (json_object_object_get_ex(obj, "chat_type", &chat_type))
        query->chat_type = TELEBOT_SAFE_STRDUP(json_object_get_string(chat_type));

    struct json_object *location = NULL;
    if (json_object_object_get_ex(obj, "location", &location))
    {
        query->location = calloc(1, sizeof(telebot_location_t));
        telebot_parser_get_location(location, query->location);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chosen_inline_result(struct json_object *obj, telebot_chosen_inline_result_t *result)
{
    if ((obj == NULL) || (result == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(result, 0, sizeof(telebot_chosen_inline_result_t));
    struct json_object *result_id = NULL;
    if (json_object_object_get_ex(obj, "result_id", &result_id))
        result->result_id = TELEBOT_SAFE_STRDUP(json_object_get_string(result_id));

    struct json_object *from = NULL;
    if (json_object_object_get_ex(obj, "from", &from))
    {
        result->from = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(from, result->from);
    }

    struct json_object *location = NULL;
    if (json_object_object_get_ex(obj, "location", &location))
    {
        result->location = calloc(1, sizeof(telebot_location_t));
        telebot_parser_get_location(location, result->location);
    }

    struct json_object *inline_message_id = NULL;
    if (json_object_object_get_ex(obj, "inline_message_id", &inline_message_id))
        result->inline_message_id = TELEBOT_SAFE_STRDUP(json_object_get_string(inline_message_id));

    struct json_object *query = NULL;
    if (json_object_object_get_ex(obj, "query", &query))
        result->query = TELEBOT_SAFE_STRDUP(json_object_get_string(query));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_shipping_query(struct json_object *obj, telebot_shipping_query_t *query)
{
    if ((obj == NULL) || (query == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(query, 0, sizeof(telebot_shipping_query_t));
    struct json_object *id = NULL;
    if (json_object_object_get_ex(obj, "id", &id))
        query->id = TELEBOT_SAFE_STRDUP(json_object_get_string(id));

    struct json_object *from = NULL;
    if (json_object_object_get_ex(obj, "from", &from))
    {
        query->from = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(from, query->from);
    }

    struct json_object *invoice_payload = NULL;
    if (json_object_object_get_ex(obj, "invoice_payload", &invoice_payload))
        query->invoice_payload = TELEBOT_SAFE_STRDUP(json_object_get_string(invoice_payload));

    struct json_object *shipping_address = NULL;
    if (json_object_object_get_ex(obj, "shipping_address", &shipping_address))
    {
        query->shipping_address = calloc(1, sizeof(telebot_shipping_address_t));
        telebot_parser_get_shipping_address(shipping_address, query->shipping_address);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_pre_checkout_query(struct json_object *obj, telebot_pre_checkout_query_t *query)
{
    if ((obj == NULL) || (query == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(query, 0, sizeof(telebot_pre_checkout_query_t));
    struct json_object *id = NULL;
    if (json_object_object_get_ex(obj, "id", &id))
        query->id = TELEBOT_SAFE_STRDUP(json_object_get_string(id));

    struct json_object *from = NULL;
    if (json_object_object_get_ex(obj, "from", &from))
    {
        query->from = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(from, query->from);
    }

    struct json_object *currency = NULL;
    if (json_object_object_get_ex(obj, "currency", &currency))
        query->currency = TELEBOT_SAFE_STRDUP(json_object_get_string(currency));

    struct json_object *total_amount = NULL;
    if (json_object_object_get_ex(obj, "total_amount", &total_amount))
        query->total_amount = json_object_get_int(total_amount);

    struct json_object *invoice_payload = NULL;
    if (json_object_object_get_ex(obj, "invoice_payload", &invoice_payload))
        query->invoice_payload = TELEBOT_SAFE_STRDUP(json_object_get_string(invoice_payload));

    struct json_object *shipping_option_id = NULL;
    if (json_object_object_get_ex(obj, "shipping_option_id", &shipping_option_id))
        query->shipping_option_id = TELEBOT_SAFE_STRDUP(json_object_get_string(shipping_option_id));

    struct json_object *order_info = NULL;
    if (json_object_object_get_ex(obj, "order_info", &order_info))
    {
        query->order_info = calloc(1, sizeof(telebot_order_info_t));
        telebot_parser_get_order_info(order_info, query->order_info);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_stickers(struct json_object *obj, telebot_sticker_t **stickers, int *count)
{
    // Stub: implement if needed, usually in telebot-stickers.c
    return TELEBOT_ERROR_OPERATION_FAILED;
}

static telebot_error_e telebot_parser_get_business_bot_rights(struct json_object *obj, telebot_business_bot_rights_t *rights)
{
    if ((obj == NULL) || (rights == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(rights, 0, sizeof(telebot_business_bot_rights_t));
    struct json_object *tmp = NULL;
    if (json_object_object_get_ex(obj, "can_reply", &tmp))
        rights->can_reply = json_object_get_boolean(tmp);
    if (json_object_object_get_ex(obj, "can_read_messages", &tmp))
        rights->can_read_messages = json_object_get_boolean(tmp);
    if (json_object_object_get_ex(obj, "can_delete_sent_messages", &tmp))
        rights->can_delete_sent_messages = json_object_get_boolean(tmp);
    if (json_object_object_get_ex(obj, "can_delete_all_messages", &tmp))
        rights->can_delete_all_messages = json_object_get_boolean(tmp);
    if (json_object_object_get_ex(obj, "can_edit_name", &tmp))
        rights->can_edit_name = json_object_get_boolean(tmp);
    if (json_object_object_get_ex(obj, "can_edit_bio", &tmp))
        rights->can_edit_bio = json_object_get_boolean(tmp);
    if (json_object_object_get_ex(obj, "can_edit_profile_photo", &tmp))
        rights->can_edit_profile_photo = json_object_get_boolean(tmp);
    if (json_object_object_get_ex(obj, "can_edit_username", &tmp))
        rights->can_edit_username = json_object_get_boolean(tmp);
    if (json_object_object_get_ex(obj, "can_change_gift_settings", &tmp))
        rights->can_change_gift_settings = json_object_get_boolean(tmp);
    if (json_object_object_get_ex(obj, "can_view_gifts_and_stars", &tmp))
        rights->can_view_gifts_and_stars = json_object_get_boolean(tmp);
    if (json_object_object_get_ex(obj, "can_convert_gifts_to_stars", &tmp))
        rights->can_convert_gifts_to_stars = json_object_get_boolean(tmp);
    if (json_object_object_get_ex(obj, "can_transfer_and_upgrade_gifts", &tmp))
        rights->can_transfer_and_upgrade_gifts = json_object_get_boolean(tmp);
    if (json_object_object_get_ex(obj, "can_transfer_stars", &tmp))
        rights->can_transfer_stars = json_object_get_boolean(tmp);
    if (json_object_object_get_ex(obj, "can_manage_stories", &tmp))
        rights->can_manage_stories = json_object_get_boolean(tmp);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_business_messages_deleted(struct json_object *obj, telebot_business_messages_deleted_t *deleted)
{
    if ((obj == NULL) || (deleted == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(deleted, 0, sizeof(telebot_business_messages_deleted_t));
    struct json_object *id = NULL;
    if (json_object_object_get_ex(obj, "business_connection_id", &id))
        deleted->business_connection_id = TELEBOT_SAFE_STRDUP(json_object_get_string(id));

    struct json_object *chat = NULL;
    if (json_object_object_get_ex(obj, "chat", &chat))
    {
        deleted->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, deleted->chat);
    }

    struct json_object *message_ids = NULL;
    if (json_object_object_get_ex(obj, "message_ids", &message_ids))
    {
        int array_len = json_object_array_length(message_ids);
        deleted->count_message_ids = array_len;
        deleted->message_ids = calloc(array_len, sizeof(int));
        for (int i = 0; i < array_len; i++)
            deleted->message_ids[i] = json_object_get_int(json_object_array_get_idx(message_ids, i));
    }

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_message_origin(struct json_object *obj, telebot_message_origin_t *origin)
{
    if ((obj == NULL) || (origin == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(origin, 0, sizeof(telebot_message_origin_t));
    struct json_object *type = NULL;
    if (json_object_object_get_ex(obj, "type", &type))
        origin->type = TELEBOT_SAFE_STRDUP(json_object_get_string(type));

    struct json_object *date = NULL;
    if (json_object_object_get_ex(obj, "date", &date))
        origin->date = json_object_get_int(date);

    struct json_object *sender_user = NULL;
    if (json_object_object_get_ex(obj, "sender_user", &sender_user))
    {
        origin->sender_user = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(sender_user, origin->sender_user);
    }

    struct json_object *sender_user_name = NULL;
    if (json_object_object_get_ex(obj, "sender_user_name", &sender_user_name))
        origin->sender_user_name = TELEBOT_SAFE_STRDUP(json_object_get_string(sender_user_name));

    struct json_object *sender_chat = NULL;
    if (json_object_object_get_ex(obj, "sender_chat", &sender_chat))
    {
        origin->sender_chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(sender_chat, origin->sender_chat);
    }

    struct json_object *author_signature = NULL;
    if (json_object_object_get_ex(obj, "author_signature", &author_signature))
        origin->author_signature = TELEBOT_SAFE_STRDUP(json_object_get_string(author_signature));

    struct json_object *message_id = NULL;
    if (json_object_object_get_ex(obj, "message_id", &message_id))
        origin->message_id = json_object_get_int(message_id);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_text_quote(struct json_object *obj, telebot_text_quote_t *quote)
{
    if ((obj == NULL) || (quote == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(quote, 0, sizeof(telebot_text_quote_t));
    struct json_object *text = NULL;
    if (json_object_object_get_ex(obj, "text", &text))
        quote->text = TELEBOT_SAFE_STRDUP(json_object_get_string(text));

    struct json_object *entities = NULL;
    if (json_object_object_get_ex(obj, "entities", &entities))
        telebot_parser_get_message_entities(entities, &(quote->entities), &(quote->count_entities));

    struct json_object *position = NULL;
    if (json_object_object_get_ex(obj, "position", &position))
        quote->position = json_object_get_int(position);

    struct json_object *is_manual = NULL;
    if (json_object_object_get_ex(obj, "is_manual", &is_manual))
        quote->is_manual = json_object_get_boolean(is_manual);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_story(struct json_object *obj, telebot_story_t *story)
{
    if ((obj == NULL) || (story == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(story, 0, sizeof(telebot_story_t));
    struct json_object *chat = NULL;
    if (json_object_object_get_ex(obj, "chat", &chat))
    {
        story->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, story->chat);
    }

    struct json_object *id = NULL;
    if (json_object_object_get_ex(obj, "id", &id))
        story->id = json_object_get_int(id);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_external_reply_info(struct json_object *obj, telebot_external_reply_info_t *info)
{
    if ((obj == NULL) || (info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(info, 0, sizeof(telebot_external_reply_info_t));
    struct json_object *origin = NULL;
    if (json_object_object_get_ex(obj, "origin", &origin))
        telebot_parser_get_message_origin(origin, &(info->origin));

    struct json_object *chat = NULL;
    if (json_object_object_get_ex(obj, "chat", &chat))
    {
        info->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, info->chat);
    }

    struct json_object *message_id = NULL;
    if (json_object_object_get_ex(obj, "message_id", &message_id))
        info->message_id = json_object_get_int(message_id);

    // ... other optional media fields can be added as needed ...

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_link_preview_options(struct json_object *obj, telebot_link_preview_options_t *options)
{
    if ((obj == NULL) || (options == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(options, 0, sizeof(telebot_link_preview_options_t));
    struct json_object *tmp = NULL;
    if (json_object_object_get_ex(obj, "is_disabled", &tmp))
        options->is_disabled = json_object_get_boolean(tmp);
    if (json_object_object_get_ex(obj, "url", &tmp))
        options->url = TELEBOT_SAFE_STRDUP(json_object_get_string(tmp));
    if (json_object_object_get_ex(obj, "prefer_small_media", &tmp))
        options->prefer_small_media = json_object_get_boolean(tmp);
    if (json_object_object_get_ex(obj, "prefer_large_media", &tmp))
        options->prefer_large_media = json_object_get_boolean(tmp);
    if (json_object_object_get_ex(obj, "show_above_text", &tmp))
        options->show_above_text = json_object_get_boolean(tmp);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_paid_media(struct json_object *obj, telebot_paid_media_t *media)
{
    if ((obj == NULL) || (media == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(media, 0, sizeof(telebot_paid_media_t));
    struct json_object *type = NULL;
    if (json_object_object_get_ex(obj, "type", &type))
        media->type = TELEBOT_SAFE_STRDUP(json_object_get_string(type));

    struct json_object *width = NULL;
    if (json_object_object_get_ex(obj, "width", &width))
        media->width = json_object_get_int(width);

    struct json_object *height = NULL;
    if (json_object_object_get_ex(obj, "height", &height))
        media->height = json_object_get_int(height);

    struct json_object *duration = NULL;
    if (json_object_object_get_ex(obj, "duration", &duration))
        media->duration = json_object_get_int(duration);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_paid_media_info(struct json_object *obj, telebot_paid_media_info_t *info)
{
    if ((obj == NULL) || (info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(info, 0, sizeof(telebot_paid_media_info_t));
    struct json_object *star_count = NULL;
    if (json_object_object_get_ex(obj, "star_count", &star_count))
        info->star_count = json_object_get_int(star_count);

    struct json_object *paid_media = NULL;
    if (json_object_object_get_ex(obj, "paid_media", &paid_media))
    {
        int array_len = json_object_array_length(paid_media);
        info->count_paid_media = array_len;
        info->paid_media = calloc(array_len, sizeof(telebot_paid_media_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_paid_media(json_object_array_get_idx(paid_media, i), &(info->paid_media[i]));
    }

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_refunded_payment(struct json_object *obj, telebot_refunded_payment_t *payment)
{
    if ((obj == NULL) || (payment == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(payment, 0, sizeof(telebot_refunded_payment_t));
    struct json_object *currency = NULL;
    if (json_object_object_get_ex(obj, "currency", &currency))
        payment->currency = TELEBOT_SAFE_STRDUP(json_object_get_string(currency));

    struct json_object *total_amount = NULL;
    if (json_object_object_get_ex(obj, "total_amount", &total_amount))
        payment->total_amount = json_object_get_int(total_amount);

    struct json_object *invoice_payload = NULL;
    if (json_object_object_get_ex(obj, "invoice_payload", &invoice_payload))
        payment->invoice_payload = TELEBOT_SAFE_STRDUP(json_object_get_string(invoice_payload));

    struct json_object *telegram_payment_charge_id = NULL;
    if (json_object_object_get_ex(obj, "telegram_payment_charge_id", &telegram_payment_charge_id))
        payment->telegram_payment_charge_id = TELEBOT_SAFE_STRDUP(json_object_get_string(telegram_payment_charge_id));

    struct json_object *provider_payment_charge_id = NULL;
    if (json_object_object_get_ex(obj, "provider_payment_charge_id", &provider_payment_charge_id))
        payment->provider_payment_charge_id = TELEBOT_SAFE_STRDUP(json_object_get_string(provider_payment_charge_id));

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_write_access_allowed(struct json_object *obj, telebot_write_access_allowed_t *allowed)
{
    if ((obj == NULL) || (allowed == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(allowed, 0, sizeof(telebot_write_access_allowed_t));
    struct json_object *web_app_name = NULL;
    if (json_object_object_get_ex(obj, "web_app_name", &web_app_name))
        allowed->web_app_name = TELEBOT_SAFE_STRDUP(json_object_get_string(web_app_name));

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_shared_user(struct json_object *obj, telebot_shared_user_t *user)
{
    if ((obj == NULL) || (user == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(user, 0, sizeof(telebot_shared_user_t));
    struct json_object *user_id = NULL;
    if (json_object_object_get_ex(obj, "user_id", &user_id))
        user->user_id = json_object_get_int64(user_id);

    struct json_object *first_name = NULL;
    if (json_object_object_get_ex(obj, "first_name", &first_name))
        user->first_name = TELEBOT_SAFE_STRDUP(json_object_get_string(first_name));

    struct json_object *last_name = NULL;
    if (json_object_object_get_ex(obj, "last_name", &last_name))
        user->last_name = TELEBOT_SAFE_STRDUP(json_object_get_string(last_name));

    struct json_object *username = NULL;
    if (json_object_object_get_ex(obj, "username", &username))
        user->username = TELEBOT_SAFE_STRDUP(json_object_get_string(username));

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_users_shared(struct json_object *obj, telebot_users_shared_t *shared)
{
    if ((obj == NULL) || (shared == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(shared, 0, sizeof(telebot_users_shared_t));
    struct json_object *request_id = NULL;
    if (json_object_object_get_ex(obj, "request_id", &request_id))
        shared->request_id = json_object_get_int(request_id);

    struct json_object *users = NULL;
    if (json_object_object_get_ex(obj, "users", &users))
    {
        int array_len = json_object_array_length(users);
        shared->count_users = array_len;
        shared->users = calloc(array_len, sizeof(telebot_shared_user_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_shared_user(json_object_array_get_idx(users, i), &(shared->users[i]));
    }

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_chat_shared(struct json_object *obj, telebot_chat_shared_t *shared)
{
    if ((obj == NULL) || (shared == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(shared, 0, sizeof(telebot_chat_shared_t));
    struct json_object *request_id = NULL;
    if (json_object_object_get_ex(obj, "request_id", &request_id))
        shared->request_id = json_object_get_int(request_id);

    struct json_object *chat_id = NULL;
    if (json_object_object_get_ex(obj, "chat_id", &chat_id))
        shared->chat_id = json_object_get_int64(chat_id);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_gift(struct json_object *obj, telebot_gift_t *gift)
{
    if ((obj == NULL) || (gift == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gift, 0, sizeof(telebot_gift_t));
    struct json_object *id = NULL;
    if (json_object_object_get_ex(obj, "id", &id))
        gift->id = TELEBOT_SAFE_STRDUP(json_object_get_string(id));

    struct json_object *star_count = NULL;
    if (json_object_object_get_ex(obj, "star_count", &star_count))
        gift->star_count = json_object_get_int(star_count);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_gift_info(struct json_object *obj, telebot_gift_info_t *gift_info)
{
    if ((obj == NULL) || (gift_info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gift_info, 0, sizeof(telebot_gift_info_t));
    struct json_object *gift = NULL;
    if (json_object_object_get_ex(obj, "gift", &gift))
        telebot_parser_get_gift(gift, &(gift_info->gift));

    struct json_object *text = NULL;
    if (json_object_object_get_ex(obj, "text", &text))
        gift_info->text = TELEBOT_SAFE_STRDUP(json_object_get_string(text));

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_unique_gift(struct json_object *obj, telebot_unique_gift_t *gift)
{
    if ((obj == NULL) || (gift == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gift, 0, sizeof(telebot_unique_gift_t));
    struct json_object *gift_id = NULL;
    if (json_object_object_get_ex(obj, "gift_id", &gift_id))
        gift->gift_id = TELEBOT_SAFE_STRDUP(json_object_get_string(gift_id));

    struct json_object *name = NULL;
    if (json_object_object_get_ex(obj, "name", &name))
        gift->name = TELEBOT_SAFE_STRDUP(json_object_get_string(name));

    struct json_object *number = NULL;
    if (json_object_object_get_ex(obj, "number", &number))
        gift->number = json_object_get_int(number);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_unique_gift_info(struct json_object *obj, telebot_unique_gift_info_t *gift_info)
{
    if ((obj == NULL) || (gift_info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gift_info, 0, sizeof(telebot_unique_gift_info_t));
    struct json_object *gift = NULL;
    if (json_object_object_get_ex(obj, "gift", &gift))
    {
        gift_info->gift = calloc(1, sizeof(telebot_unique_gift_t));
        telebot_parser_get_unique_gift(gift, gift_info->gift);
    }

    struct json_object *origin = NULL;
    if (json_object_object_get_ex(obj, "origin", &origin))
        gift_info->origin = TELEBOT_SAFE_STRDUP(json_object_get_string(origin));

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_chat_boost_added(struct json_object *obj, telebot_chat_boost_added_t *boost)
{
    if ((obj == NULL) || (boost == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(boost, 0, sizeof(telebot_chat_boost_added_t));
    struct json_object *boost_count = NULL;
    if (json_object_object_get_ex(obj, "boost_count", &boost_count))
        boost->boost_count = json_object_get_int(boost_count);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_chat_background(struct json_object *obj, telebot_chat_background_t *background)
{
    if ((obj == NULL) || (background == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(background, 0, sizeof(telebot_chat_background_t));
    struct json_object *type = NULL;
    if (json_object_object_get_ex(obj, "type", &type))
    {
        struct json_object *type_str = NULL;
        if (json_object_object_get_ex(type, "type", &type_str))
            background->type = TELEBOT_SAFE_STRDUP(json_object_get_string(type_str));
    }

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_giveaway_created(struct json_object *obj, telebot_giveaway_created_t *giveaway)
{
    if ((obj == NULL) || (giveaway == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(giveaway, 0, sizeof(telebot_giveaway_created_t));
    struct json_object *prize_star_count = NULL;
    if (json_object_object_get_ex(obj, "prize_star_count", &prize_star_count))
        giveaway->prize_star_count = json_object_get_int(prize_star_count);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_giveaway_completed(struct json_object *obj, telebot_giveaway_completed_t *giveaway)
{
    if ((obj == NULL) || (giveaway == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(giveaway, 0, sizeof(telebot_giveaway_completed_t));
    struct json_object *winner_count = NULL;
    if (json_object_object_get_ex(obj, "winner_count", &winner_count))
        giveaway->winner_count = json_object_get_int(winner_count);

    struct json_object *unclaimed_prize_count = NULL;
    if (json_object_object_get_ex(obj, "unclaimed_prize_count", &unclaimed_prize_count))
        giveaway->unclaimed_prize_count = json_object_get_int(unclaimed_prize_count);

    struct json_object *giveaway_message = NULL;
    if (json_object_object_get_ex(obj, "giveaway_message", &giveaway_message))
    {
        giveaway->giveaway_message = calloc(1, sizeof(telebot_message_t));
        telebot_parser_get_message(giveaway_message, giveaway->giveaway_message);
    }

    struct json_object *is_star_giveaway = NULL;
    if (json_object_object_get_ex(obj, "is_star_giveaway", &is_star_giveaway))
        giveaway->is_star_giveaway = json_object_get_boolean(is_star_giveaway);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_birthdate(struct json_object *obj, telebot_birthdate_t *birthdate)
{
    if ((obj == NULL) || (birthdate == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(birthdate, 0, sizeof(telebot_birthdate_t));
    struct json_object *tmp = NULL;
    if (json_object_object_get_ex(obj, "day", &tmp))
        birthdate->day = json_object_get_int(tmp);
    if (json_object_object_get_ex(obj, "month", &tmp))
        birthdate->month = json_object_get_int(tmp);
    if (json_object_object_get_ex(obj, "year", &tmp))
        birthdate->year = json_object_get_int(tmp);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_business_intro(struct json_object *obj, telebot_business_intro_t *intro)
{
    if ((obj == NULL) || (intro == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(intro, 0, sizeof(telebot_business_intro_t));
    struct json_object *tmp = NULL;
    if (json_object_object_get_ex(obj, "title", &tmp))
        intro->title = TELEBOT_SAFE_STRDUP(json_object_get_string(tmp));
    if (json_object_object_get_ex(obj, "message", &tmp))
        intro->message = TELEBOT_SAFE_STRDUP(json_object_get_string(tmp));

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_business_location(struct json_object *obj, telebot_business_location_t *location)
{
    if ((obj == NULL) || (location == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(location, 0, sizeof(telebot_business_location_t));
    struct json_object *address = NULL;
    if (json_object_object_get_ex(obj, "address", &address))
        location->address = TELEBOT_SAFE_STRDUP(json_object_get_string(address));

    struct json_object *loc = NULL;
    if (json_object_object_get_ex(obj, "location", &loc))
    {
        location->location = calloc(1, sizeof(telebot_location_t));
        telebot_parser_get_location(loc, location->location);
    }

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_business_opening_hours(struct json_object *obj, telebot_business_opening_hours_t *hours)
{
    if ((obj == NULL) || (hours == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(hours, 0, sizeof(telebot_business_opening_hours_t));
    struct json_object *time_zone_name = NULL;
    if (json_object_object_get_ex(obj, "time_zone_name", &time_zone_name))
        hours->time_zone_name = TELEBOT_SAFE_STRDUP(json_object_get_string(time_zone_name));

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_star_transaction(struct json_object *obj, telebot_star_transaction_t *transaction)
{
    if ((obj == NULL) || (transaction == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(transaction, 0, sizeof(telebot_star_transaction_t));
    struct json_object *id = NULL;
    if (json_object_object_get_ex(obj, "id", &id))
        transaction->id = TELEBOT_SAFE_STRDUP(json_object_get_string(id));

    struct json_object *amount = NULL;
    if (json_object_object_get_ex(obj, "amount", &amount))
        transaction->amount = json_object_get_int(amount);

    struct json_object *date = NULL;
    if (json_object_object_get_ex(obj, "date", &date))
        transaction->date = json_object_get_int(date);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_star_transactions(struct json_object *obj, telebot_star_transactions_t *transactions)
{
    if ((obj == NULL) || (transactions == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(transactions, 0, sizeof(telebot_star_transactions_t));
    struct json_object *array = NULL;
    if (json_object_object_get_ex(obj, "transactions", &array))
    {
        int array_len = json_object_array_length(array);
        transactions->count_transactions = array_len;
        transactions->transactions = calloc(array_len, sizeof(telebot_star_transaction_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_star_transaction(json_object_array_get_idx(array, i), &(transactions->transactions[i]));
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_prepared_inline_message(struct json_object *obj, telebot_prepared_inline_message_t *prepared_message)
{
    if ((obj == NULL) || (prepared_message == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(prepared_message, 0, sizeof(telebot_prepared_inline_message_t));
    struct json_object *id = NULL;
    if (json_object_object_get_ex(obj, "id", &id))
        prepared_message->id = TELEBOT_SAFE_STRDUP(json_object_get_string(id));

    struct json_object *expiration_date = NULL;
    if (json_object_object_get_ex(obj, "expiration_date", &expiration_date))
        prepared_message->expiration_date = json_object_get_int(expiration_date);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_game_high_score(struct json_object *obj, telebot_game_high_score_t *high_score)
{
    if ((obj == NULL) || (high_score == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(high_score, 0, sizeof(telebot_game_high_score_t));
    struct json_object *position = NULL;
    if (json_object_object_get_ex(obj, "position", &position))
        high_score->position = json_object_get_int(position);

    struct json_object *user = NULL;
    if (json_object_object_get_ex(obj, "user", &user))
    {
        high_score->user = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(user, high_score->user);
    }

    struct json_object *score = NULL;
    if (json_object_object_get_ex(obj, "score", &score))
        high_score->score = json_object_get_int(score);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_paid_media_purchased(struct json_object *obj, telebot_paid_media_purchased_t *purchased)
{
    if ((obj == NULL) || (purchased == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(purchased, 0, sizeof(telebot_paid_media_purchased_t));
    struct json_object *from = NULL;
    if (json_object_object_get_ex(obj, "from", &from))
    {
        purchased->from = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(from, purchased->from);
    }

    struct json_object *paid_media_payload = NULL;
    if (json_object_object_get_ex(obj, "paid_media_payload", &paid_media_payload))
        purchased->paid_media_payload = TELEBOT_SAFE_STRDUP(json_object_get_string(paid_media_payload));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_gifts(struct json_object *obj, telebot_gifts_t *gifts)
{
    if ((obj == NULL) || (gifts == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gifts, 0, sizeof(telebot_gifts_t));

    struct json_object *result = NULL;
    if (!json_object_object_get_ex(obj, "result", &result))
        result = obj;

    if (json_object_get_type(result) != json_type_array)
        return TELEBOT_ERROR_OPERATION_FAILED;

    int array_len = json_object_array_length(result);
    if (array_len > 0)
    {
        gifts->gifts = calloc(array_len, sizeof(telebot_gift_t));
        if (gifts->gifts == NULL)
            return TELEBOT_ERROR_OUT_OF_MEMORY;

        gifts->count = array_len;
        for (int i = 0; i < array_len; i++)
        {
            struct json_object *item = json_object_array_get_idx(result, i);
            telebot_parser_get_gift(item, &(gifts->gifts[i]));
        }
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user_gift(struct json_object *obj, telebot_user_gift_t *gift)
{
    if ((obj == NULL) || (gift == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gift, 0, sizeof(telebot_user_gift_t));

    struct json_object *gift_id = NULL;
    if (json_object_object_get_ex(obj, "gift_id", &gift_id))
        gift->gift_id = TELEBOT_SAFE_STRDUP(json_object_get_string(gift_id));

    struct json_object *sender_user = NULL;
    if (json_object_object_get_ex(obj, "sender_user", &sender_user))
    {
        gift->sender_user = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(sender_user, gift->sender_user);
    }

    struct json_object *text = NULL;
    if (json_object_object_get_ex(obj, "text", &text))
        gift->text = TELEBOT_SAFE_STRDUP(json_object_get_string(text));

    struct json_object *entities = NULL;
    if (json_object_object_get_ex(obj, "entities", &entities))
        telebot_parser_get_message_entities(entities, &(gift->entities), &(gift->count_entities));

    struct json_object *gift_obj = NULL;
    if (json_object_object_get_ex(obj, "gift", &gift_obj))
    {
        gift->gift = calloc(1, sizeof(telebot_gift_t));
        telebot_parser_get_gift(gift_obj, gift->gift);
    }

    struct json_object *date = NULL;
    if (json_object_object_get_ex(obj, "date", &date))
        gift->date = json_object_get_int(date);

    struct json_object *message_id = NULL;
    if (json_object_object_get_ex(obj, "message_id", &message_id))
        gift->message_id = json_object_get_int(message_id);

    struct json_object *upgrade_star_count = NULL;
    if (json_object_object_get_ex(obj, "upgrade_star_count", &upgrade_star_count))
        gift->upgrade_star_count = json_object_get_int(upgrade_star_count);

    struct json_object *is_upgraded = NULL;
    if (json_object_object_get_ex(obj, "is_upgraded", &is_upgraded))
        gift->is_upgraded = json_object_get_boolean(is_upgraded);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user_gifts(struct json_object *obj, telebot_user_gifts_t *gifts)
{
    if ((obj == NULL) || (gifts == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gifts, 0, sizeof(telebot_user_gifts_t));

    struct json_object *result = NULL;
    if (!json_object_object_get_ex(obj, "result", &result))
        result = obj;

    struct json_object *total_count = NULL;
    if (json_object_object_get_ex(result, "total_count", &total_count))
        gifts->total_count = json_object_get_int(total_count);

    struct json_object *gifts_array = NULL;
    if (json_object_object_get_ex(result, "gifts", &gifts_array))
    {
        int array_len = json_object_array_length(gifts_array);
        if (array_len > 0)
        {
            gifts->gifts = calloc(array_len, sizeof(telebot_user_gift_t));
            if (gifts->gifts == NULL)
                return TELEBOT_ERROR_OUT_OF_MEMORY;

            gifts->count = array_len;
            for (int i = 0; i < array_len; i++)
            {
                struct json_object *item = json_object_array_get_idx(gifts_array, i);
                telebot_parser_get_user_gift(item, &(gifts->gifts[i]));
            }
        }
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user_profile_audios(struct json_object *obj, telebot_user_profile_audios_t *audios)
{
    if ((obj == NULL) || (audios == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(audios, 0, sizeof(telebot_user_profile_audios_t));

    struct json_object *result = NULL;
    if (!json_object_object_get_ex(obj, "result", &result))
        result = obj;

    struct json_object *total_count = NULL;
    if (json_object_object_get_ex(result, "total_count", &total_count))
        audios->total_count = json_object_get_int(total_count);

    struct json_object *audios_array = NULL;
    if (json_object_object_get_ex(result, "audios", &audios_array))
    {
        int array_len = json_object_array_length(audios_array);
        if (array_len > 0)
        {
            audios->audios = calloc(array_len, sizeof(telebot_audio_t));
            if (audios->audios == NULL)
                return TELEBOT_ERROR_OUT_OF_MEMORY;

            audios->count = array_len;
            for (int i = 0; i < array_len; i++)
            {
                struct json_object *item = json_object_array_get_idx(audios_array, i);
                telebot_parser_get_audio(item, &(audios->audios[i]));
            }
        }
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_business_connection(struct json_object *obj, telebot_business_connection_t *connection)
{
    if ((obj == NULL) || (connection == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(connection, 0, sizeof(telebot_business_connection_t));

    struct json_object *result = NULL;
    if (!json_object_object_get_ex(obj, "result", &result))
        result = obj;

    struct json_object *id = NULL;
    if (json_object_object_get_ex(result, "id", &id))
        connection->id = TELEBOT_SAFE_STRDUP(json_object_get_string(id));

    struct json_object *user = NULL;
    if (json_object_object_get_ex(result, "user", &user))
    {
        connection->user = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(user, connection->user);
    }

    struct json_object *user_chat_id = NULL;
    if (json_object_object_get_ex(result, "user_chat_id", &user_chat_id))
        connection->user_chat_id = json_object_get_int64(user_chat_id);

    struct json_object *date = NULL;
    if (json_object_object_get_ex(result, "date", &date))
        connection->date = json_object_get_int(date);

    struct json_object *can_reply = NULL;
    if (json_object_object_get_ex(result, "can_reply", &can_reply))
        connection->can_reply = json_object_get_boolean(can_reply);

    struct json_object *is_enabled = NULL;
    if (json_object_object_get_ex(result, "is_enabled", &is_enabled))
        connection->is_enabled = json_object_get_boolean(is_enabled);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user_chat_boosts(struct json_object *obj, telebot_user_chat_boosts_t *boosts)
{
    if ((obj == NULL) || (boosts == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(boosts, 0, sizeof(telebot_user_chat_boosts_t));

    struct json_object *result = NULL;
    if (!json_object_object_get_ex(obj, "result", &result))
        result = obj;

    struct json_object *boosts_array = NULL;
    if (json_object_object_get_ex(result, "boosts", &boosts_array))
    {
        int array_len = json_object_array_length(boosts_array);
        if (array_len > 0)
        {
            boosts->boosts = calloc(array_len, sizeof(telebot_chat_boost_t));
            if (boosts->boosts == NULL)
                return TELEBOT_ERROR_OUT_OF_MEMORY;

            boosts->count = array_len;
            for (int i = 0; i < array_len; i++)
            {
                struct json_object *item = json_object_array_get_idx(boosts_array, i);
                telebot_parser_get_chat_boost(item, &(boosts->boosts[i]));
            }
        }
    }

    return TELEBOT_ERROR_NONE;
}


