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
#include "telebot-json.h"
#include "telebot-config.h"
#include "telebot-fields.h"
#include <telebot-methods.h>
#include <telebot-parser.h>
#include <telebot-private.h>

/* Bot API names of update types, indexed by telebot_update_type_e */
const char *const telebot_update_type_names[TELEBOT_UPDATE_TYPE_MAX] = {
    [TELEBOT_UPDATE_TYPE_MESSAGE] = "message",
    [TELEBOT_UPDATE_TYPE_EDITED_MESSAGE] = "edited_message",
    [TELEBOT_UPDATE_TYPE_CHANNEL_POST] = "channel_post",
    [TELEBOT_UPDATE_TYPE_EDITED_CHANNEL_POST] = "edited_channel_post",
    [TELEBOT_UPDATE_TYPE_BUSINESS_CONNECTION] = "business_connection",
    [TELEBOT_UPDATE_TYPE_BUSINESS_MESSAGE] = "business_message",
    [TELEBOT_UPDATE_TYPE_EDITED_BUSINESS_MESSAGE] = "edited_business_message",
    [TELEBOT_UPDATE_TYPE_DELETED_BUSINESS_MESSAGES] = "deleted_business_messages",
    [TELEBOT_UPDATE_TYPE_MESSAGE_REACTION] = "message_reaction",
    [TELEBOT_UPDATE_TYPE_MESSAGE_REACTION_COUNT] = "message_reaction_count",
    [TELEBOT_UPDATE_TYPE_INLINE_QUERY] = "inline_query",
    [TELEBOT_UPDATE_TYPE_CHOSEN_INLINE_RESULT] = "chosen_inline_result",
    [TELEBOT_UPDATE_TYPE_CALLBACK_QUERY] = "callback_query",
    [TELEBOT_UPDATE_TYPE_SHIPPING_QUERY] = "shipping_query",
    [TELEBOT_UPDATE_TYPE_PRE_CHECKOUT_QUERY] = "pre_checkout_query",
    [TELEBOT_UPDATE_TYPE_PURCHASED_PAID_MEDIA] = "purchased_paid_media",
    [TELEBOT_UPDATE_TYPE_POLL] = "poll",
    [TELEBOT_UPDATE_TYPE_POLL_ANSWER] = "poll_answer",
    [TELEBOT_UPDATE_TYPE_MY_CHAT_MEMBER] = "my_chat_member",
    [TELEBOT_UPDATE_TYPE_CHAT_MEMBER] = "chat_member",
    [TELEBOT_UPDATE_TYPE_CHAT_JOIN_REQUEST] = "chat_join_request",
    [TELEBOT_UPDATE_TYPE_CHAT_BOOST] = "chat_boost",
    [TELEBOT_UPDATE_TYPE_REMOVED_CHAT_BOOST] = "removed_chat_boost",
    [TELEBOT_UPDATE_TYPE_GUEST_MESSAGE] = "guest_message",
    [TELEBOT_UPDATE_TYPE_STOPPED_MESSAGE_GENERATION] = "stopped_message_generation",
};
TELEBOT_STATIC_ASSERT(sizeof(telebot_update_type_names) / sizeof(telebot_update_type_names[0]) == TELEBOT_UPDATE_TYPE_MAX,
                      "telebot_update_type_names must name every update type");

int telebot_update_type_from_name(const char *name)
{
    for (int i = 0; (name != NULL) && (i < TELEBOT_UPDATE_TYPE_MAX); i++)
    {
        if ((telebot_update_type_names[i] != NULL) && (strcmp(name, telebot_update_type_names[i]) == 0))
            return i;
    }
    return TELEBOT_UPDATE_TYPE_UNKNOWN;
}

static telebot_error_e telebot_parser_get_photos(struct cJSON *obj, telebot_photo_t **photos, int *count);
static telebot_error_e telebot_parser_get_users(struct cJSON *obj, telebot_user_t **users, int *count);
#if TELEBOT_FEATURE_BUSINESS
static telebot_error_e telebot_parser_get_business_messages_deleted(struct cJSON *obj, telebot_business_messages_deleted_t *deleted);
#endif
static telebot_error_e telebot_parser_get_message_origin(struct cJSON *obj, telebot_message_origin_t *origin);
static telebot_error_e telebot_parser_get_text_quote(struct cJSON *obj, telebot_text_quote_t *quote);
static telebot_error_e telebot_parser_get_story(struct cJSON *obj, telebot_story_t *story);
static telebot_error_e telebot_parser_get_external_reply_info(struct cJSON *obj, telebot_external_reply_info_t *info);
static telebot_error_e telebot_parser_get_link_preview_options(struct cJSON *obj, telebot_link_preview_options_t *options);
#if TELEBOT_FEATURE_PAYMENTS
static telebot_error_e telebot_parser_get_paid_media_info(struct cJSON *obj, telebot_paid_media_info_t *info);
#endif
#if TELEBOT_FEATURE_PAYMENTS
static telebot_error_e telebot_parser_get_refunded_payment(struct cJSON *obj, telebot_refunded_payment_t *payment);
#endif
#if TELEBOT_FEATURE_CHAT_EXTRAS
static telebot_error_e telebot_parser_get_write_access_allowed(struct cJSON *obj, telebot_write_access_allowed_t *allowed);
#endif
#if TELEBOT_FEATURE_CHAT_EXTRAS
static telebot_error_e telebot_parser_get_users_shared(struct cJSON *obj, telebot_users_shared_t *shared);
#endif
#if TELEBOT_FEATURE_CHAT_EXTRAS
static telebot_error_e telebot_parser_get_chat_shared(struct cJSON *obj, telebot_chat_shared_t *shared);
#endif
#if TELEBOT_FEATURE_GIFTS
static telebot_error_e telebot_parser_get_gift_info(struct cJSON *obj, telebot_gift_info_t *gift);
#endif
#if TELEBOT_FEATURE_GIFTS
static telebot_error_e telebot_parser_get_unique_gift_info(struct cJSON *obj, telebot_unique_gift_info_t *gift);
#endif
#if TELEBOT_FEATURE_CHAT_EXTRAS
static telebot_error_e telebot_parser_get_chat_boost_added(struct cJSON *obj, telebot_chat_boost_added_t *boost);
#endif
#if TELEBOT_FEATURE_CHAT_EXTRAS
static telebot_error_e telebot_parser_get_chat_background(struct cJSON *obj, telebot_chat_background_t *background);
#endif
#if TELEBOT_FEATURE_GIFTS
static telebot_error_e telebot_parser_get_giveaway_created(struct cJSON *obj, telebot_giveaway_created_t *giveaway);
#endif
#if TELEBOT_FEATURE_GIFTS
static telebot_error_e telebot_parser_get_giveaway_completed(struct cJSON *obj, telebot_giveaway_completed_t *giveaway);
#endif
static telebot_error_e telebot_parser_get_birthdate(struct cJSON *obj, telebot_birthdate_t *birthdate);
static telebot_error_e telebot_parser_get_business_intro(struct cJSON *obj, telebot_business_intro_t *intro);
static telebot_error_e telebot_parser_get_business_location(struct cJSON *obj, telebot_business_location_t *location);
static telebot_error_e telebot_parser_get_business_opening_hours(struct cJSON *obj, telebot_business_opening_hours_t *hours);
static telebot_error_e telebot_parser_get_business_bot_rights(struct cJSON *obj, telebot_business_bot_rights_t *rights);
#if TELEBOT_FEATURE_PAYMENTS
static telebot_error_e telebot_parser_get_paid_media(struct cJSON *obj, telebot_paid_media_t *media);
#endif
static telebot_error_e telebot_parser_get_gift(struct cJSON *obj, telebot_gift_t *gift);
#if TELEBOT_FEATURE_GIFTS
static telebot_error_e telebot_parser_get_unique_gift(struct cJSON *obj, telebot_unique_gift_t *gift);
#endif
#if TELEBOT_FEATURE_CHAT_EXTRAS
static telebot_error_e telebot_parser_get_shared_user(struct cJSON *obj, telebot_shared_user_t *user);
#endif

struct cJSON *telebot_parser_str_to_obj(const char *data)
{
    if (data)
        return cJSON_Parse(data);
    else
        return NULL;
}

telebot_error_e telebot_parser_get_updates(struct cJSON *obj, telebot_update_t **updates, int *count)
{
    if ((obj == NULL) || (updates == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct cJSON *array = obj;
    if (!cJSON_IsArray(array))
        return TELEBOT_ERROR_OPERATION_FAILED;

    /* An idle long-poll returns an empty array */
    *updates = NULL;
    *count = 0;
    int array_len = tb_json_array_length(array);
    if (array_len == 0)
        return TELEBOT_ERROR_NONE;

    telebot_update_t *result = calloc(array_len, sizeof(telebot_update_t));
    if (result == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    *count = array_len;
    *updates = result;

    for (int index = 0; index < array_len; index++)
    {
        struct cJSON *item = tb_json_array_get_idx(array, index);
        result[index].update_type = TELEBOT_UPDATE_TYPE_UNKNOWN; /* until a known payload is found */

        struct cJSON *update_id = NULL;
        if (tb_json_get_ex(item, "update_id", &update_id))
        {
            result[index].update_id = tb_json_get_int(update_id);
        }

        struct cJSON *message = NULL;
        if (tb_json_get_ex(item, "message", &message))
        {
            if (telebot_parser_get_message(message, &(result[index].message)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse message of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_MESSAGE;
            continue;
        }

        struct cJSON *edited_message = NULL;
        if (tb_json_get_ex(item, "edited_message", &edited_message))
        {
            if (telebot_parser_get_message(edited_message, &(result[index].edited_message)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse edited message of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_EDITED_MESSAGE;
            continue;
        }

        struct cJSON *channel_post = NULL;
        if (tb_json_get_ex(item, "channel_post", &channel_post))
        {
            if (telebot_parser_get_message(channel_post, &(result[index].channel_post)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse channel post of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CHANNEL_POST;
            continue;
        }

        struct cJSON *edited_channel_post = NULL;
        if (tb_json_get_ex(item, "edited_channel_post", &edited_channel_post))
        {
            if (telebot_parser_get_message(edited_channel_post, &(result[index].edited_channel_post)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse edited channel post of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_EDITED_CHANNEL_POST;
            continue;
        }

#if TELEBOT_FEATURE_BUSINESS
        struct cJSON *business_connection = NULL;
        if (tb_json_get_ex(item, "business_connection", &business_connection))
        {
            if (telebot_parser_get_business_connection(business_connection, &(result[index].business_connection)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse business_connection of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_BUSINESS_CONNECTION;
            continue;
        }
#endif

#if TELEBOT_FEATURE_BUSINESS
        struct cJSON *business_message = NULL;
        if (tb_json_get_ex(item, "business_message", &business_message))
        {
            if (telebot_parser_get_message(business_message, &(result[index].business_message)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse business_message of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_BUSINESS_MESSAGE;
            continue;
        }
#endif

#if TELEBOT_FEATURE_BUSINESS
        struct cJSON *edited_business_message = NULL;
        if (tb_json_get_ex(item, "edited_business_message", &edited_business_message))
        {
            if (telebot_parser_get_message(edited_business_message, &(result[index].edited_business_message)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse edited_business_message of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_EDITED_BUSINESS_MESSAGE;
            continue;
        }
#endif

#if TELEBOT_FEATURE_BUSINESS
        struct cJSON *deleted_business_messages = NULL;
        if (tb_json_get_ex(item, "deleted_business_messages", &deleted_business_messages))
        {
            if (telebot_parser_get_business_messages_deleted(deleted_business_messages, &(result[index].deleted_business_messages)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse deleted_business_messages of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_DELETED_BUSINESS_MESSAGES;
            continue;
        }
#endif

        struct cJSON *guest_message = NULL;
        if (tb_json_get_ex(item, "guest_message", &guest_message))
        {
            if (telebot_parser_get_message(guest_message, &(result[index].guest_message)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse guest_message of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_GUEST_MESSAGE;
            continue;
        }

        struct cJSON *stopped = NULL;
        if (tb_json_get_ex(item, "stopped_message_generation", &stopped))
        {
            telebot_message_generation_stopped_t *gen = &(result[index].stopped_message_generation);
            cJSON *tmp = NULL;
            if (tb_json_get_ex(stopped, "chat", &tmp))
            {
                gen->chat = calloc(1, sizeof(telebot_chat_t));
                if ((gen->chat != NULL) && (telebot_parser_get_chat(tmp, gen->chat) != TELEBOT_ERROR_NONE))
                    TELEBOT_SAFE_FREE(gen->chat);
            }
            if (tb_json_get_ex(stopped, "message_thread_id", &tmp))
                gen->message_thread_id = tb_json_get_int(tmp);
            if (tb_json_get_ex(stopped, "draft_id", &tmp))
                gen->draft_id = tb_json_get_int(tmp);
            result[index].update_type = TELEBOT_UPDATE_TYPE_STOPPED_MESSAGE_GENERATION;
            continue;
        }

        struct cJSON *callback_query = NULL;
        if (tb_json_get_ex(item, "callback_query", &callback_query))
        {
            if (telebot_parser_get_callback_query(callback_query, &(result[index].callback_query)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse callback query of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CALLBACK_QUERY;
            continue;
        }

        struct cJSON *poll = NULL;
        if (tb_json_get_ex(item, "poll", &poll))
        {
            if (telebot_parser_get_poll(poll, &(result[index].poll)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse poll of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_POLL;
            continue;
        }

        struct cJSON *poll_answer = NULL;
        if (tb_json_get_ex(item, "poll_answer", &poll_answer))
        {
            if (telebot_parser_get_poll_answer(poll_answer, &(result[index].poll_answer)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse poll answer of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_POLL_ANSWER;
            continue;
        }

        struct cJSON *my_chat_member = NULL;
        if (tb_json_get_ex(item, "my_chat_member", &my_chat_member))
        {
            if (telebot_parser_get_chat_member_updated(my_chat_member, &(result[index].my_chat_member)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse my_chat_member of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_MY_CHAT_MEMBER;
            continue;
        }

        struct cJSON *chat_member = NULL;
        if (tb_json_get_ex(item, "chat_member", &chat_member))
        {
            if (telebot_parser_get_chat_member_updated(chat_member, &(result[index].chat_member)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse chat_member of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CHAT_MEMBER;
            continue;
        }

        struct cJSON *chat_join_request = NULL;
        if (tb_json_get_ex(item, "chat_join_request", &chat_join_request))
        {
            if (telebot_parser_get_chat_join_request(chat_join_request, &(result[index].chat_join_request)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse chat_join_request of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CHAT_JOIN_REQUEST;
            continue;
        }

#if TELEBOT_FEATURE_CHAT_EXTRAS
        struct cJSON *message_reaction = NULL;
        if (tb_json_get_ex(item, "message_reaction", &message_reaction))
        {
            if (telebot_parser_get_message_reaction_updated(message_reaction, &(result[index].message_reaction)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse message_reaction of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_MESSAGE_REACTION;
            continue;
        }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
        struct cJSON *message_reaction_count = NULL;
        if (tb_json_get_ex(item, "message_reaction_count", &message_reaction_count))
        {
            if (telebot_parser_get_message_reaction_count_updated(message_reaction_count, &(result[index].message_reaction_count)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse message_reaction_count of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_MESSAGE_REACTION_COUNT;
            continue;
        }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
        struct cJSON *chat_boost = NULL;
        if (tb_json_get_ex(item, "chat_boost", &chat_boost))
        {
            if (telebot_parser_get_chat_boost_updated(chat_boost, &(result[index].chat_boost)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse chat_boost of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CHAT_BOOST;
            continue;
        }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
        struct cJSON *chat_boost_removed = NULL;
        if (tb_json_get_ex(item, "removed_chat_boost", &chat_boost_removed))
        {
            if (telebot_parser_get_chat_boost_removed(chat_boost_removed, &(result[index].chat_boost_removed)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse chat_boost_removed of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_REMOVED_CHAT_BOOST;
            continue;
        }
#endif

        struct cJSON *inline_query = NULL;
        if (tb_json_get_ex(item, "inline_query", &inline_query))
        {
            if (telebot_parser_get_inline_query(inline_query, &(result[index].inline_query)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse inline_query of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_INLINE_QUERY;
            continue;
        }

        struct cJSON *chosen_inline_result = NULL;
        if (tb_json_get_ex(item, "chosen_inline_result", &chosen_inline_result))
        {
            if (telebot_parser_get_chosen_inline_result(chosen_inline_result, &(result[index].chosen_inline_result)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse chosen_inline_result of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CHOSEN_INLINE_RESULT;
            continue;
        }

#if TELEBOT_FEATURE_PAYMENTS
        struct cJSON *shipping_query = NULL;
        if (tb_json_get_ex(item, "shipping_query", &shipping_query))
        {
            if (telebot_parser_get_shipping_query(shipping_query, &(result[index].shipping_query)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse shipping_query of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_SHIPPING_QUERY;
            continue;
        }
#endif

#if TELEBOT_FEATURE_PAYMENTS
        struct cJSON *pre_checkout_query = NULL;
        if (tb_json_get_ex(item, "pre_checkout_query", &pre_checkout_query))
        {
            if (telebot_parser_get_pre_checkout_query(pre_checkout_query, &(result[index].pre_checkout_query)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse pre_checkout_query of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_PRE_CHECKOUT_QUERY;
            continue;
        }
#endif

#if TELEBOT_FEATURE_PAYMENTS
        struct cJSON *purchased_paid_media = NULL;
        if (tb_json_get_ex(item, "purchased_paid_media", &purchased_paid_media))
        {
            if (telebot_parser_get_paid_media_purchased(purchased_paid_media, &(result[index].purchased_paid_media)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse purchased_paid_media of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_PURCHASED_PAID_MEDIA;
            continue;
        }
#endif

    } /* for index */

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_webhook_info(struct cJSON *obj, telebot_webhook_info_t *info)
{
    if ((obj == NULL) || (info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(info, 0, sizeof(telebot_webhook_info_t));
    struct cJSON *url = NULL;
    if (!tb_json_get_ex(obj, "url", &url))
    {
        ERR("Object is not json webhook info type, url not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    info->url = TELEBOT_SAFE_STRDUP(tb_json_get_string(url));

    struct cJSON *certificate = NULL;
    if (!tb_json_get_ex(obj, "has_custom_certificate", &certificate))
    {
        ERR("Object is not webhook info type, has_custom_certificate not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    info->has_custom_certificate = tb_json_get_bool(certificate);

    struct cJSON *update_count = NULL;
    if (!tb_json_get_ex(obj, "pending_update_count", &update_count))
    {
        ERR("Object is not webook info type, pending_update_count not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    info->pending_update_count = tb_json_get_int(update_count);

    struct cJSON *last_error_date = NULL;
    if (tb_json_get_ex(obj, "last_error_date", &last_error_date))
    {
        info->last_error_date = tb_json_get_double(last_error_date);
    }

    struct cJSON *last_error_message = NULL;
    if (tb_json_get_ex(obj, "last_error_message", &last_error_message))
        info->last_error_message = TELEBOT_SAFE_STRDUP(tb_json_get_string(last_error_message));

    struct cJSON *max_connections = NULL;
    if (tb_json_get_ex(obj, "max_connections", &max_connections))
        info->max_connections = tb_json_get_int(max_connections);

    struct cJSON *allowed_updates = NULL;
    if (tb_json_get_ex(obj, "allowed_updates", &allowed_updates))
    {
        int cnt = 0;
        int array_len = tb_json_array_length(allowed_updates);
        for (int i = 0; i < array_len; i++)
        {
            int type = telebot_update_type_from_name(tb_json_get_string(tb_json_array_get_idx(allowed_updates, i)));
            if ((type != TELEBOT_UPDATE_TYPE_UNKNOWN) && (cnt < TELEBOT_UPDATE_TYPE_MAX))
                info->allowed_updates[cnt++] = type;
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

telebot_error_e telebot_parser_get_user(struct cJSON *obj, telebot_user_t *user)
{
    if ((obj == NULL) || (user == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(user, 0, sizeof(telebot_user_t));
    struct cJSON *id;
    if (!tb_json_get_ex(obj, "id", &id))
    {
        ERR("Object is not json user type, id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    user->id = tb_json_get_int(id);

    struct cJSON *is_bot = NULL;
    if (!tb_json_get_ex(obj, "is_bot", &is_bot))
    {
        ERR("Object is not json user type, is_bot not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    user->is_bot = tb_json_get_bool(is_bot);

    struct cJSON *first_name = NULL;
    if (!tb_json_get_ex(obj, "first_name", &first_name))
    {
        ERR("Object is not user type, first_name not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    user->first_name = TELEBOT_SAFE_STRDUP(tb_json_get_string(first_name));

    struct cJSON *last_name = NULL;
    if (tb_json_get_ex(obj, "last_name", &last_name))
        user->last_name = TELEBOT_SAFE_STRDUP(tb_json_get_string(last_name));

    struct cJSON *username = NULL;
    if (tb_json_get_ex(obj, "username", &username))
        user->username = TELEBOT_SAFE_STRDUP(tb_json_get_string(username));

    struct cJSON *language_code = NULL;
    if (tb_json_get_ex(obj, "language_code", &language_code))
        user->language_code = TELEBOT_SAFE_STRDUP(tb_json_get_string(language_code));

    struct cJSON *is_premium = NULL;
    if (tb_json_get_ex(obj, "is_premium", &is_premium))
        user->is_premium = tb_json_get_bool(is_premium);

    struct cJSON *added_to_attachment_menu = NULL;
    if (tb_json_get_ex(obj, "added_to_attachment_menu", &added_to_attachment_menu))
        user->added_to_attachment_menu = tb_json_get_bool(added_to_attachment_menu);

    struct cJSON *can_join_groups = NULL;
    if (tb_json_get_ex(obj, "can_join_groups", &can_join_groups))
        user->can_join_groups = tb_json_get_bool(can_join_groups);

    struct cJSON *can_read_all_group_messages = NULL;
    if (tb_json_get_ex(obj, "can_read_all_group_messages", &can_read_all_group_messages))
        user->can_read_all_group_messages = tb_json_get_bool(can_read_all_group_messages);

    struct cJSON *supports_inline_queries = NULL;
    if (tb_json_get_ex(obj, "supports_inline_queries", &supports_inline_queries))
        user->supports_inline_queries = tb_json_get_bool(supports_inline_queries);

    struct cJSON *can_connect_to_business = NULL;
    if (tb_json_get_ex(obj, "can_connect_to_business", &can_connect_to_business))
        user->can_connect_to_business = tb_json_get_bool(can_connect_to_business);

    struct cJSON *has_main_web_app = NULL;
    if (tb_json_get_ex(obj, "has_main_web_app", &has_main_web_app))
        user->has_main_web_app = tb_json_get_bool(has_main_web_app);

    struct cJSON *has_topics_enabled = NULL;
    if (tb_json_get_ex(obj, "has_topics_enabled", &has_topics_enabled))
        user->has_topics_enabled = tb_json_get_bool(has_topics_enabled);

    struct cJSON *allows_users_to_create_topics = NULL;
    if (tb_json_get_ex(obj, "allows_users_to_create_topics", &allows_users_to_create_topics))
        user->allows_users_to_create_topics = tb_json_get_bool(allows_users_to_create_topics);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_users(struct cJSON *obj, telebot_user_t **users, int *count)
{
    struct cJSON *array = obj;
    int array_len = tb_json_array_length(array);
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
        struct cJSON *item = tb_json_array_get_idx(array, index);
        if (telebot_parser_get_user(item, &(result[index])) != TELEBOT_ERROR_NONE)
            ERR("Failed to parse user from users array");
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat(struct cJSON *obj, telebot_chat_t *chat)
{
    if ((obj == NULL) || (chat == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(chat, 0, sizeof(telebot_chat_t));

    struct cJSON *id;
    if (!tb_json_get_ex(obj, "id", &id))
    {
        ERR("Object is not chat type, id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    chat->id = tb_json_get_int(id);

    struct cJSON *type = NULL;
    if (!tb_json_get_ex(obj, "type", &type))
    {
        ERR("Object is not chat type, type not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    chat->type = TELEBOT_SAFE_STRDUP(tb_json_get_string(type));

    struct cJSON *title = NULL;
    if (tb_json_get_ex(obj, "title", &title))
        chat->title = TELEBOT_SAFE_STRDUP(tb_json_get_string(title));

    struct cJSON *username = NULL;
    if (tb_json_get_ex(obj, "username", &username))
        chat->username = TELEBOT_SAFE_STRDUP(tb_json_get_string(username));

    struct cJSON *first_name = NULL;
    if (tb_json_get_ex(obj, "first_name", &first_name))
        chat->first_name = TELEBOT_SAFE_STRDUP(tb_json_get_string(first_name));

    struct cJSON *last_name = NULL;
    if (tb_json_get_ex(obj, "last_name", &last_name))
        chat->last_name = TELEBOT_SAFE_STRDUP(tb_json_get_string(last_name));

    struct cJSON *is_forum = NULL;
    if (tb_json_get_ex(obj, "is_forum", &is_forum))
        chat->is_forum = tb_json_get_bool(is_forum);

    struct cJSON *is_direct_messages = NULL;
    if (tb_json_get_ex(obj, "is_direct_messages", &is_direct_messages))
        chat->is_direct_messages = tb_json_get_bool(is_direct_messages);

    struct cJSON *accent_color_id = NULL;
    if (tb_json_get_ex(obj, "accent_color_id", &accent_color_id))
        chat->accent_color_id = tb_json_get_int(accent_color_id);

    struct cJSON *max_reaction_count = NULL;
    if (tb_json_get_ex(obj, "max_reaction_count", &max_reaction_count))
        chat->max_reaction_count = tb_json_get_int(max_reaction_count);

    struct cJSON *chat_photo = NULL;
    if (tb_json_get_ex(obj, "photo", &chat_photo))
    {
        chat->photo = calloc(1, sizeof(telebot_chat_photo_t));
        if (telebot_parser_get_chat_photo(chat_photo, chat->photo) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <photo> from chat object");
            TELEBOT_SAFE_FREE(chat->photo);
        }
    }

    struct cJSON *array_active_usernames = NULL;
    if (tb_json_get_ex(obj, "active_usernames", &array_active_usernames))
    {
        int count_active_usernames = tb_json_array_length(array_active_usernames);
        if ((chat->count_active_usernames > 0) &&
            (chat->active_usernames = calloc(count_active_usernames, sizeof(char *))))
        {
            chat->count_active_usernames = count_active_usernames;
            for (int index = 0; index < count_active_usernames; index++)
            {
                struct cJSON *item = tb_json_array_get_idx(array_active_usernames, index);
                chat->active_usernames[index] = TELEBOT_SAFE_STRDUP(tb_json_get_string(item));
            }
        }
    }

    struct cJSON *birthdate = NULL;
    if (tb_json_get_ex(obj, "birthdate", &birthdate))
    {
        chat->birthdate = calloc(1, sizeof(telebot_birthdate_t));
        telebot_parser_get_birthdate(birthdate, chat->birthdate);
    }

    struct cJSON *business_intro = NULL;
    if (tb_json_get_ex(obj, "business_intro", &business_intro))
    {
        chat->business_intro = calloc(1, sizeof(telebot_business_intro_t));
        telebot_parser_get_business_intro(business_intro, chat->business_intro);
    }

    struct cJSON *business_location = NULL;
    if (tb_json_get_ex(obj, "business_location", &business_location))
    {
        chat->business_location = calloc(1, sizeof(telebot_business_location_t));
        telebot_parser_get_business_location(business_location, chat->business_location);
    }

    struct cJSON *business_opening_hours = NULL;
    if (tb_json_get_ex(obj, "business_opening_hours", &business_opening_hours))
    {
        chat->business_opening_hours = calloc(1, sizeof(telebot_business_opening_hours_t));
        telebot_parser_get_business_opening_hours(business_opening_hours, chat->business_opening_hours);
    }

    struct cJSON *personal_chat = NULL;
    if (tb_json_get_ex(obj, "personal_chat", &personal_chat))
    {
        chat->personal_chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(personal_chat, chat->personal_chat);
    }

    struct cJSON *parent_chat = NULL;
    if (tb_json_get_ex(obj, "parent_chat", &parent_chat))
    {
        chat->parent_chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(parent_chat, chat->parent_chat);
    }

    struct cJSON *emoji_status_custom_emoji_id = NULL;
    if (tb_json_get_ex(obj, "emoji_status_custom_emoji_id", &emoji_status_custom_emoji_id))
        chat->emoji_status_custom_emoji_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(emoji_status_custom_emoji_id));

    struct cJSON *emoji_status_expiration_date = NULL;
    if (tb_json_get_ex(obj, "emoji_status_expiration_date", &emoji_status_expiration_date))
        chat->emoji_status_expiration_date = tb_json_get_int(emoji_status_expiration_date);

    struct cJSON *bio = NULL;
    if (tb_json_get_ex(obj, "bio", &bio))
        chat->bio = TELEBOT_SAFE_STRDUP(tb_json_get_string(bio));

    struct cJSON *has_private_forwards = NULL;
    if (tb_json_get_ex(obj, "has_private_forwards", &has_private_forwards))
        chat->has_private_forwards = tb_json_get_bool(has_private_forwards);

    struct cJSON *has_restricted_voice_and_video_messages = NULL;
    if (tb_json_get_ex(obj, "has_restricted_voice_and_video_messages", &has_restricted_voice_and_video_messages))
        chat->has_restricted_voice_and_video_messages = tb_json_get_bool(has_restricted_voice_and_video_messages);

    struct cJSON *join_to_send_messages = NULL;
    if (tb_json_get_ex(obj, "join_to_send_messages", &join_to_send_messages))
        chat->join_to_send_messages = tb_json_get_bool(join_to_send_messages);

    struct cJSON *join_by_request = NULL;
    if (tb_json_get_ex(obj, "join_by_request", &join_by_request))
        chat->join_by_request = tb_json_get_bool(join_by_request);

    struct cJSON *description = NULL;
    if (tb_json_get_ex(obj, "description", &description))
        chat->description = TELEBOT_SAFE_STRDUP(tb_json_get_string(description));

    struct cJSON *invite_link = NULL;
    if (tb_json_get_ex(obj, "invite_link", &invite_link))
        chat->invite_link = TELEBOT_SAFE_STRDUP(tb_json_get_string(invite_link));


    struct cJSON *permissions = NULL;
    if (tb_json_get_ex(obj, "permissions", &permissions))
    {
        chat->permissions = calloc(1, sizeof(telebot_chat_permissions_t));
        if (telebot_parser_get_chat_permissions(permissions, chat->permissions) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <permissions> from chat object");
            TELEBOT_SAFE_FREE(chat->permissions);
        }
    }

    struct cJSON *slow_mode_delay = NULL;
    if (tb_json_get_ex(obj, "slow_mode_delay", &slow_mode_delay))
        chat->slow_mode_delay = tb_json_get_int(slow_mode_delay);

    struct cJSON *unrestrict_boost_count = NULL;
    if (tb_json_get_ex(obj, "unrestrict_boost_count", &unrestrict_boost_count))
        chat->unrestrict_boost_count = tb_json_get_int(unrestrict_boost_count);

    struct cJSON *has_aggressive_anti_spam_enabled = NULL;
    if (tb_json_get_ex(obj, "has_aggressive_anti_spam_enabled", &has_aggressive_anti_spam_enabled))
        chat->has_aggressive_anti_spam_enabled = tb_json_get_bool(has_aggressive_anti_spam_enabled);

    struct cJSON *has_hidden_members = NULL;
    if (tb_json_get_ex(obj, "has_hidden_members", &has_hidden_members))
        chat->has_hidden_members = tb_json_get_bool(has_hidden_members);

    struct cJSON *has_protected_content = NULL;
    if (tb_json_get_ex(obj, "has_protected_content", &has_protected_content))
        chat->has_protected_content = tb_json_get_bool(has_protected_content);

    struct cJSON *message_auto_delete_time = NULL;
    if (tb_json_get_ex(obj, "message_auto_delete_time", &message_auto_delete_time))
        chat->message_auto_delete_time = tb_json_get_int(message_auto_delete_time);

    struct cJSON *sticker_set_name = NULL;
    if (tb_json_get_ex(obj, "sticker_set_name", &sticker_set_name))
        chat->sticker_set_name = TELEBOT_SAFE_STRDUP(tb_json_get_string(sticker_set_name));

    struct cJSON *can_set_sticker_set = NULL;
    if (tb_json_get_ex(obj, "can_set_sticker_set", &can_set_sticker_set))
        chat->can_set_sticker_set = tb_json_get_bool(can_set_sticker_set);

    struct cJSON *custom_emoji_sticker_set_name = NULL;
    if (tb_json_get_ex(obj, "custom_emoji_sticker_set_name", &custom_emoji_sticker_set_name))
        chat->custom_emoji_sticker_set_name = TELEBOT_SAFE_STRDUP(tb_json_get_string(custom_emoji_sticker_set_name));

    struct cJSON *linked_chat_id = NULL;
    if (tb_json_get_ex(obj, "linked_chat_id", &linked_chat_id))
        chat->linked_chat_id = tb_json_get_int(linked_chat_id);

    struct cJSON *location = NULL;
    if (tb_json_get_ex(obj, "location", &location))
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

telebot_error_e telebot_parser_get_chat_admins(struct cJSON *obj, telebot_chat_member_t **admins, int *count)
{
    int ret = TELEBOT_ERROR_NONE;
    struct cJSON *array = obj;
    int array_len = tb_json_array_length(array);
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
        struct cJSON *item = tb_json_array_get_idx(array, index);
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

telebot_error_e telebot_parser_get_message(struct cJSON *obj, telebot_message_t *msg)
{
    if ((obj == NULL) || (msg == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(msg, 0, sizeof(telebot_message_t));

    /* Mandatory Fields */
    struct cJSON *message_id = NULL;
    if (!tb_json_get_ex(obj, "message_id", &message_id))
    {
        ERR("Failed to get <message_id> from message object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    msg->message_id = tb_json_get_int(message_id);

    struct cJSON *date = NULL;
    if (!tb_json_get_ex(obj, "date", &date))
    {
        ERR("Failed to get <date> from message object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    msg->date = tb_json_get_int(date);

    struct cJSON *chat = NULL;
    if (!tb_json_get_ex(obj, "chat", &chat))
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

    struct cJSON *forward_origin = NULL;
    if (tb_json_get_ex(obj, "forward_origin", &forward_origin))
    {
        msg->forward_origin = calloc(1, sizeof(telebot_message_origin_t));
        if (telebot_parser_get_message_origin(forward_origin, msg->forward_origin) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <forward_origin> from message object");
            TELEBOT_SAFE_FREE(msg->forward_origin);
        }
    }

    /* Optional Fields */
    struct cJSON *message_thread_id = NULL;
    if (tb_json_get_ex(obj, "message_thread_id", &message_thread_id))
        msg->message_thread_id = tb_json_get_int(message_thread_id);

    struct cJSON *from = NULL;
    if (tb_json_get_ex(obj, "from", &from))
    {
        msg->from = calloc(1, sizeof(telebot_user_t));
        if (telebot_parser_get_user(from, msg->from) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <from user> from message object");
            TELEBOT_SAFE_FREE(msg->from);
        }
    }

    struct cJSON *sender_chat = NULL;
    if (tb_json_get_ex(obj, "sender_chat", &sender_chat))
    {
        msg->sender_chat = malloc(sizeof(telebot_chat_t));
        if (telebot_parser_get_chat(sender_chat, msg->sender_chat) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <sender_chat> from message object");
            TELEBOT_SAFE_FREE(msg->sender_chat);
        }
    }

    struct cJSON *sender_boost_count = NULL;
    if (tb_json_get_ex(obj, "sender_boost_count", &sender_boost_count))
        msg->sender_boost_count = tb_json_get_int(sender_boost_count);

    struct cJSON *sender_business_bot = NULL;
    if (tb_json_get_ex(obj, "sender_business_bot", &sender_business_bot))
    {
        msg->sender_business_bot = calloc(1, sizeof(telebot_user_t));
        if (telebot_parser_get_user(sender_business_bot, msg->sender_business_bot) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <sender_business_bot> from message object");
            TELEBOT_SAFE_FREE(msg->sender_business_bot);
        }
    }

struct cJSON *sender_tag = NULL;
    if (tb_json_get_ex(obj, "sender_tag", &sender_tag))
        msg->sender_tag = TELEBOT_SAFE_STRDUP(tb_json_get_string(sender_tag));

        struct cJSON *business_connection_id = NULL;
    if (tb_json_get_ex(obj, "business_connection_id", &business_connection_id))
        msg->business_connection_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(business_connection_id));

    struct cJSON *forward_from = NULL;
    if (tb_json_get_ex(obj, "forward_from", &forward_from))
    {
        msg->forward_from = calloc(1, sizeof(telebot_user_t));
        if (telebot_parser_get_user(forward_from, msg->forward_from) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <forward from> from message object");
            TELEBOT_SAFE_FREE(msg->forward_from);
        }
    }

    struct cJSON *forward_from_chat = NULL;
    if (tb_json_get_ex(obj, "forward_from_chat", &forward_from_chat))
    {
        msg->forward_from_chat = calloc(1, sizeof(telebot_chat_t));
        if (telebot_parser_get_chat(forward_from_chat, msg->forward_from_chat) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <forward from> from message object");
            TELEBOT_SAFE_FREE(msg->forward_from_chat);
        }
    }

    struct cJSON *forward_from_message_id = NULL;
    if (tb_json_get_ex(obj, "forward_from_message_id", &forward_from_message_id))
        msg->forward_from_message_id = tb_json_get_int(forward_from_message_id);

    struct cJSON *forward_signature = NULL;
    if (tb_json_get_ex(obj, "forward_signature", &forward_signature))
        msg->forward_signature = TELEBOT_SAFE_STRDUP(tb_json_get_string(forward_signature));

    struct cJSON *forward_sender_name = NULL;
    if (tb_json_get_ex(obj, "forward_sender_name", &forward_sender_name))
        msg->forward_sender_name = TELEBOT_SAFE_STRDUP(tb_json_get_string(forward_sender_name));

    struct cJSON *forward_date = NULL;
    if (tb_json_get_ex(obj, "forward_date", &forward_date))
        msg->forward_date = tb_json_get_int(forward_date);

    struct cJSON *is_topic_message = NULL;
    if (tb_json_get_ex(obj, "is_topic_message", &is_topic_message))
        msg->is_topic_message = tb_json_get_bool(is_topic_message);

    struct cJSON *is_automatic_forward = NULL;
    if (tb_json_get_ex(obj, "is_automatic_forward", &is_automatic_forward))
        msg->is_automatic_forward = tb_json_get_bool(is_automatic_forward);

    struct cJSON *reply_to_message = NULL;
    if (tb_json_get_ex(obj, "reply_to_message", &reply_to_message))
    {
        msg->reply_to_message = calloc(1, sizeof(telebot_message_t));
        if (telebot_parser_get_message(reply_to_message, msg->reply_to_message) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <reply_to_message> from message object");
            TELEBOT_SAFE_FREE(msg->reply_to_message);
        }
    }

    struct cJSON *external_reply = NULL;
    if (tb_json_get_ex(obj, "external_reply", &external_reply))
    {
        msg->external_reply = calloc(1, sizeof(telebot_external_reply_info_t));
        if (telebot_parser_get_external_reply_info(external_reply, msg->external_reply) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <external_reply> from message object");
            TELEBOT_SAFE_FREE(msg->external_reply);
        }
    }

    struct cJSON *quote = NULL;
    if (tb_json_get_ex(obj, "quote", &quote))
    {
        msg->quote = calloc(1, sizeof(telebot_text_quote_t));
        if (telebot_parser_get_text_quote(quote, msg->quote) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <quote> from message object");
            TELEBOT_SAFE_FREE(msg->quote);
        }
    }

    struct cJSON *reply_to_story = NULL;
    if (tb_json_get_ex(obj, "reply_to_story", &reply_to_story))
    {
        msg->reply_to_story = calloc(1, sizeof(telebot_story_t));
        if (telebot_parser_get_story(reply_to_story, msg->reply_to_story) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <reply_to_story> from message object");
            TELEBOT_SAFE_FREE(msg->reply_to_story);
        }
    }

    struct cJSON *via_bot = NULL;
    if (tb_json_get_ex(obj, "via_bot", &via_bot))
    {
        msg->via_bot = malloc(sizeof(telebot_user_t));
        if (telebot_parser_get_user(via_bot, msg->via_bot) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <via_bot> from message object");
            TELEBOT_SAFE_FREE(msg->via_bot);
        }
    }

    struct cJSON *edit_date = NULL;
    if (tb_json_get_ex(obj, "edit_date", &edit_date))
        msg->edit_date = tb_json_get_int(edit_date);

    struct cJSON *has_protected_content = NULL;
    if (tb_json_get_ex(obj, "has_protected_content", &has_protected_content))
        msg->has_protected_content = tb_json_get_bool(has_protected_content);

    struct cJSON *is_paid_post = NULL;
    if (tb_json_get_ex(obj, "is_paid_post", &is_paid_post))
        msg->is_paid_post = tb_json_get_bool(is_paid_post);

    struct cJSON *media_group_id = NULL;
    if (tb_json_get_ex(obj, "media_group_id", &media_group_id))
        msg->media_group_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(media_group_id));

    struct cJSON *author_signature = NULL;
    if (tb_json_get_ex(obj, "author_signature", &author_signature))
        msg->author_signature = TELEBOT_SAFE_STRDUP(tb_json_get_string(author_signature));

    struct cJSON *text = NULL;
    if (tb_json_get_ex(obj, "text", &text))
        msg->text = TELEBOT_SAFE_STRDUP(tb_json_get_string(text));

    struct cJSON *entities = NULL;
    if (tb_json_get_ex(obj, "entities", &entities))
    {
        if (telebot_parser_get_message_entities(entities, &(msg->entities), &(msg->count_entities)) !=
            TELEBOT_ERROR_NONE)
            ERR("Failed to get <entities> from message object");
    }

    struct cJSON *link_preview_options = NULL;
    if (tb_json_get_ex(obj, "link_preview_options", &link_preview_options))
    {
        msg->link_preview_options = calloc(1, sizeof(telebot_link_preview_options_t));
        if (telebot_parser_get_link_preview_options(link_preview_options, msg->link_preview_options) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <link_preview_options> from message object");
            TELEBOT_SAFE_FREE(msg->link_preview_options);
        }
    }

    struct cJSON *effect_id = NULL;
    if (tb_json_get_ex(obj, "effect_id", &effect_id))
        msg->effect_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(effect_id));

    struct cJSON *animation = NULL;
    if (tb_json_get_ex(obj, "animation", &animation))
    {
        msg->animation = malloc(sizeof(telebot_animation_t));
        if (telebot_parser_get_animation(animation, msg->animation) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <animation> from message object");
            TELEBOT_SAFE_FREE(msg->animation);
        }
    }

    struct cJSON *audio = NULL;
    if (tb_json_get_ex(obj, "audio", &audio))
    {
        msg->audio = calloc(1, sizeof(telebot_audio_t));
        if (telebot_parser_get_audio(audio, msg->audio) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <audio> from message object");
            TELEBOT_SAFE_FREE(msg->audio);
        }
    }

    struct cJSON *document = NULL;
    if (tb_json_get_ex(obj, "document", &document))
    {
        msg->document = calloc(1, sizeof(telebot_document_t));
        if (telebot_parser_get_document(document, msg->document) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <document> from message object");
            TELEBOT_SAFE_FREE(msg->document);
        }
    }

    struct cJSON *sticker = NULL;
    if (tb_json_get_ex(obj, "sticker", &sticker))
    {
        msg->sticker = calloc(1, sizeof(telebot_sticker_t));
        if ((msg->sticker != NULL) && (telebot_parser_get_sticker(sticker, msg->sticker) != TELEBOT_ERROR_NONE))
        {
            ERR("Failed to get <sticker> from message object");
            telebot_put_sticker(msg->sticker);
            TELEBOT_SAFE_FREE(msg->sticker);
        }
    }

#if TELEBOT_FEATURE_PAYMENTS
    struct cJSON *paid_media = NULL;
    if (tb_json_get_ex(obj, "paid_media", &paid_media))
    {
        msg->paid_media = calloc(1, sizeof(telebot_paid_media_info_t));
        if (telebot_parser_get_paid_media_info(paid_media, msg->paid_media) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <paid_media> from message object");
            TELEBOT_SAFE_FREE(msg->paid_media);
        }
    }
#endif

    struct cJSON *photo = NULL;
    if (tb_json_get_ex(obj, "photo", &photo))
    {
        if (telebot_parser_get_photos(photo, &(msg->photos), &(msg->count_photos)) != TELEBOT_ERROR_NONE)
            ERR("Failed to get <photo> from message object");
    }

    // TODO: implement sticker parsing

    struct cJSON *video = NULL;
    if (tb_json_get_ex(obj, "video", &video))
    {
        msg->video = calloc(1, sizeof(telebot_video_t));
        if (telebot_parser_get_video(video, msg->video) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <video> from message object");
            TELEBOT_SAFE_FREE(msg->video);
        }
    }

    struct cJSON *video_note = NULL;
    if (tb_json_get_ex(obj, "video_note", &video_note))
    {
        msg->video_note = calloc(1, sizeof(telebot_video_note_t));
        if (telebot_parser_get_video_note(video_note, msg->video_note) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <video_note> from message object");
            TELEBOT_SAFE_FREE(msg->video_note);
        }
    }

    struct cJSON *voice = NULL;
    if (tb_json_get_ex(obj, "voice", &voice))
    {
        msg->voice = malloc(sizeof(telebot_voice_t));
        if (telebot_parser_get_voice(voice, msg->voice) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <voice> from message object");
            TELEBOT_SAFE_FREE(msg->voice);
        }
    }

    struct cJSON *caption = NULL;
    if (tb_json_get_ex(obj, "caption", &caption))
        msg->caption = TELEBOT_SAFE_STRDUP(tb_json_get_string(caption));

    struct cJSON *caption_entities = NULL;
    if (tb_json_get_ex(obj, "caption_entities", &caption_entities))
    {
        if (telebot_parser_get_message_entities(caption_entities, &(msg->caption_entities),
                                                &(msg->count_caption_entities)) != TELEBOT_ERROR_NONE)
            ERR("Failed to get <caption_entities> from message object");
    }

    struct cJSON *show_caption_above_media = NULL;
    if (tb_json_get_ex(obj, "show_caption_above_media", &show_caption_above_media))
        msg->show_caption_above_media = tb_json_get_bool(show_caption_above_media);

    struct cJSON *has_media_spoiler = NULL;
    if (tb_json_get_ex(obj, "has_media_spoiler", &has_media_spoiler))
        msg->has_media_spoiler = tb_json_get_bool(has_media_spoiler);

    struct cJSON *contact = NULL;
    if (tb_json_get_ex(obj, "contact", &contact))
    {
        msg->contact = calloc(1, sizeof(telebot_contact_t));
        if (telebot_parser_get_contact(contact, msg->contact) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <contact> from message object");
            TELEBOT_SAFE_FREE(msg->contact);
        }
    }

    struct cJSON *dice = NULL;
    if (tb_json_get_ex(obj, "dice", &dice))
    {
        msg->dice = calloc(1, sizeof(telebot_dice_t));
        if (telebot_parser_get_dice(dice, msg->dice) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <dice> from message object");
            TELEBOT_SAFE_FREE(msg->dice);
        }
    }

    // TODO: implement game parsing

    struct cJSON *poll = NULL;
    if (tb_json_get_ex(obj, "poll", &poll))
    {
        msg->poll = malloc(sizeof(telebot_poll_t));
        if (telebot_parser_get_poll(poll, msg->poll) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <poll> from message object");
            TELEBOT_SAFE_FREE(msg->poll);
        }
    }

    struct cJSON *venue = NULL;
    if (tb_json_get_ex(obj, "venue", &venue))
    {
        msg->venue = malloc(sizeof(telebot_venue_t));
        if (telebot_parser_get_venue(venue, msg->venue) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <venue> from message object");
            TELEBOT_SAFE_FREE(msg->venue);
        }
    }

    struct cJSON *location = NULL;
    if (tb_json_get_ex(obj, "location", &location))
    {
        msg->location = malloc(sizeof(telebot_location_t));
        if (telebot_parser_get_location(location, msg->location) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <location> from message object");
            TELEBOT_SAFE_FREE(msg->location);
        }
    }

    struct cJSON *new_chat_members = NULL;
    if (tb_json_get_ex(obj, "new_chat_members", &new_chat_members))
    {
        int ret = telebot_parser_get_users(new_chat_members, &(msg->new_chat_members), &(msg->count_new_chat_members));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <new_chat_members> from message object");
    }

    struct cJSON *left_chat_members = NULL;
    if (tb_json_get_ex(obj, "left_chat_members", &left_chat_members))
    {
        int ret = telebot_parser_get_users(left_chat_members, &(msg->left_chat_members), &(msg->count_left_chat_members));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <left_chat_members> from message object");
    }

    struct cJSON *new_chat_title = NULL;
    if (tb_json_get_ex(obj, "new_chat_title", &new_chat_title))
        msg->new_chat_title = TELEBOT_SAFE_STRDUP(tb_json_get_string(new_chat_title));

    struct cJSON *new_chat_photo = NULL;
    if (tb_json_get_ex(obj, "new_chat_photo", &new_chat_photo))
    {
        if (telebot_parser_get_photos(new_chat_photo, &(msg->new_chat_photos), &(msg->count_new_chat_photos)) !=
            TELEBOT_ERROR_NONE)
            ERR("Failed to get <new_chat_photo> from message object");
    }

    struct cJSON *del_chat_photo = NULL;
    if (tb_json_get_ex(obj, "delete_chat_photo", &del_chat_photo))
        msg->delete_chat_photo = tb_json_get_bool(del_chat_photo);

    struct cJSON *group_chat_created = NULL;
    if (tb_json_get_ex(obj, "group_chat_created", &group_chat_created))
        msg->group_chat_created = tb_json_get_bool(group_chat_created);

    struct cJSON *supergroup_chat_created = NULL;
    if (tb_json_get_ex(obj, "supergroup_chat_created", &supergroup_chat_created))
        msg->supergroup_chat_created = tb_json_get_bool(supergroup_chat_created);

    struct cJSON *channel_chat_created = NULL;
    if (tb_json_get_ex(obj, "channel_chat_created", &channel_chat_created))
        msg->channel_chat_created = tb_json_get_bool(channel_chat_created);

    struct cJSON *message_auto_delete_timer_changed = NULL;
    if (tb_json_get_ex(obj, "message_auto_delete_timer_changed", &message_auto_delete_timer_changed))
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

    struct cJSON *migrate_to_chat_id = NULL;
    if (tb_json_get_ex(obj, "migrate_to_chat_id", &migrate_to_chat_id))
        msg->migrate_to_chat_id = tb_json_get_int(migrate_to_chat_id);

    struct cJSON *migrate_from_chat_id = NULL;
    if (tb_json_get_ex(obj, "migrate_from_chat_id", &migrate_from_chat_id))
        msg->migrate_from_chat_id = tb_json_get_int(migrate_from_chat_id);

    struct cJSON *pinned_message = NULL;
    if (tb_json_get_ex(obj, "pinned_message", &pinned_message))
    {
        msg->pinned_message = calloc(1, sizeof(telebot_message_t));
        if (telebot_parser_get_message(pinned_message, msg->pinned_message) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <pinned_message> from message object");
            TELEBOT_SAFE_FREE(msg->pinned_message);
        }
    }

    struct cJSON *connected_website = NULL;
    if (tb_json_get_ex(obj, "connected_website", &connected_website))
        msg->connected_website = TELEBOT_SAFE_STRDUP(tb_json_get_string(connected_website));

#if TELEBOT_FEATURE_PAYMENTS
    struct cJSON *invoice = NULL;
    if (tb_json_get_ex(obj, "invoice", &invoice))
    {
        msg->invoice = calloc(1, sizeof(telebot_invoice_t));
        if (telebot_parser_get_invoice(invoice, msg->invoice) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <invoice> from message object");
            TELEBOT_SAFE_FREE(msg->invoice);
        }
    }
#endif

#if TELEBOT_FEATURE_PAYMENTS
    struct cJSON *successful_payment = NULL;
    if (tb_json_get_ex(obj, "successful_payment", &successful_payment))
    {
        msg->successful_payment = calloc(1, sizeof(telebot_successful_payment_t));
        if (telebot_parser_get_successful_payment(successful_payment, msg->successful_payment) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <successful_payment> from message object");
            TELEBOT_SAFE_FREE(msg->successful_payment);
        }
    }
#endif

#if TELEBOT_FEATURE_PAYMENTS
    struct cJSON *refunded_payment = NULL;
    if (tb_json_get_ex(obj, "refunded_payment", &refunded_payment))
    {
        msg->refunded_payment = calloc(1, sizeof(telebot_refunded_payment_t));
        if (telebot_parser_get_refunded_payment(refunded_payment, msg->refunded_payment) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <refunded_payment> from message object");
            TELEBOT_SAFE_FREE(msg->refunded_payment);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *write_access_allowed = NULL;
    if (tb_json_get_ex(obj, "write_access_allowed", &write_access_allowed))
    {
        msg->write_access_allowed = calloc(1, sizeof(telebot_write_access_allowed_t));
        if (telebot_parser_get_write_access_allowed(write_access_allowed, msg->write_access_allowed) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <write_access_allowed> from message object");
            TELEBOT_SAFE_FREE(msg->write_access_allowed);
        }
    }
#endif

#if TELEBOT_FEATURE_PASSPORT
    struct cJSON *passport_data = NULL;
    if (tb_json_get_ex(obj, "passport_data", &passport_data))
    {
        msg->passport_data = calloc(1, sizeof(telebot_passport_data_t));
        if (telebot_parser_get_passport_data(passport_data, msg->passport_data) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <passport_data> from message object");
            TELEBOT_SAFE_FREE(msg->passport_data);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *proximity_alert_triggered = NULL;
    if (tb_json_get_ex(obj, "proximity_alert_triggered", &proximity_alert_triggered))
    {
        msg->proximity_alert_triggered = calloc(1, sizeof(telebot_proximity_alert_triggered_t));
        if (telebot_parser_get_proximity_alert_triggered(proximity_alert_triggered, msg->proximity_alert_triggered) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <proximity_alert_triggered> from message object");
            TELEBOT_SAFE_FREE(msg->proximity_alert_triggered);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *forum_topic_created = NULL;
    if (tb_json_get_ex(obj, "forum_topic_created", &forum_topic_created))
    {
        msg->forum_topic_created = calloc(1, sizeof(telebot_forum_topic_created_t));
        if (telebot_parser_get_forum_topic_created(forum_topic_created, msg->forum_topic_created) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <forum_topic_created> from message object");
            TELEBOT_SAFE_FREE(msg->forum_topic_created);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *forum_topic_edited = NULL;
    if (tb_json_get_ex(obj, "forum_topic_edited", &forum_topic_edited))
    {
        msg->forum_topic_edited = calloc(1, sizeof(telebot_forum_topic_edited_t));
        if (telebot_parser_get_forum_topic_edited(forum_topic_edited, msg->forum_topic_edited) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <forum_topic_edited> from message object");
            TELEBOT_SAFE_FREE(msg->forum_topic_edited);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *forum_topic_closed = NULL;
    if (tb_json_get_ex(obj, "forum_topic_closed", &forum_topic_closed))
    {
        msg->forum_topic_closed = calloc(1, sizeof(telebot_forum_topic_closed_t));
        msg->forum_topic_closed->dummy = true;
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *forum_topic_reopened = NULL;
    if (tb_json_get_ex(obj, "forum_topic_reopened", &forum_topic_reopened))
    {
        msg->forum_topic_reopened = calloc(1, sizeof(telebot_forum_topic_reopened_t));
        msg->forum_topic_reopened->dummy = true;
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *video_chat_scheduled = NULL;
    if (tb_json_get_ex(obj, "video_chat_scheduled", &video_chat_scheduled))
    {
        msg->video_chat_scheduled = calloc(1, sizeof(telebot_video_chat_scheduled_t));
        if (telebot_parser_get_video_chat_scheduled(video_chat_scheduled, msg->video_chat_scheduled) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <video_chat_scheduled> from message object");
            TELEBOT_SAFE_FREE(msg->video_chat_scheduled);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *video_chat_started = NULL;
    if (tb_json_get_ex(obj, "video_chat_started", &video_chat_started))
    {
        msg->video_chat_started = calloc(1, sizeof(telebot_video_chat_started_t));
        msg->video_chat_started->dummy = true;
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *video_chat_ended = NULL;
    if (tb_json_get_ex(obj, "video_chat_ended", &video_chat_ended))
    {
        msg->video_chat_ended = calloc(1, sizeof(telebot_video_chat_ended_t));
        if (telebot_parser_get_video_chat_ended(video_chat_ended, msg->video_chat_ended) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <video_chat_ended> from message object");
            TELEBOT_SAFE_FREE(msg->video_chat_ended);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *video_chat_participants_invited = NULL;
    if (tb_json_get_ex(obj, "video_chat_participants_invited", &video_chat_participants_invited))
    {
        msg->video_chat_participants_invited = calloc(1, sizeof(telebot_video_chat_participants_invited_t));
        if (telebot_parser_get_video_chat_participants_invited(video_chat_participants_invited, msg->video_chat_participants_invited) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <video_chat_participants_invited> from message object");
            TELEBOT_SAFE_FREE(msg->video_chat_participants_invited);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *web_app_data = NULL;
    if (tb_json_get_ex(obj, "web_app_data", &web_app_data))
    {
        msg->web_app_data = calloc(1, sizeof(telebot_web_app_data_t));
        if (telebot_parser_get_web_app_data(web_app_data, msg->web_app_data) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <web_app_data> from message object");
            TELEBOT_SAFE_FREE(msg->web_app_data);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *users_shared = NULL;
    if (tb_json_get_ex(obj, "users_shared", &users_shared))
    {
        msg->users_shared = calloc(1, sizeof(telebot_users_shared_t));
        if (telebot_parser_get_users_shared(users_shared, msg->users_shared) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <users_shared> from message object");
            TELEBOT_SAFE_FREE(msg->users_shared);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *chat_shared = NULL;
    if (tb_json_get_ex(obj, "chat_shared", &chat_shared))
    {
        msg->chat_shared = calloc(1, sizeof(telebot_chat_shared_t));
        if (telebot_parser_get_chat_shared(chat_shared, msg->chat_shared) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <chat_shared> from message object");
            TELEBOT_SAFE_FREE(msg->chat_shared);
        }
    }
#endif

#if TELEBOT_FEATURE_GIFTS
    struct cJSON *gift = NULL;
    if (tb_json_get_ex(obj, "gift", &gift))
    {
        msg->gift = calloc(1, sizeof(telebot_gift_info_t));
        if (telebot_parser_get_gift_info(gift, msg->gift) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <gift> from message object");
            TELEBOT_SAFE_FREE(msg->gift);
        }
    }
#endif

#if TELEBOT_FEATURE_GIFTS
    struct cJSON *unique_gift = NULL;
    if (tb_json_get_ex(obj, "unique_gift", &unique_gift))
    {
        msg->unique_gift = calloc(1, sizeof(telebot_unique_gift_info_t));
        if (telebot_parser_get_unique_gift_info(unique_gift, msg->unique_gift) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <unique_gift> from message object");
            TELEBOT_SAFE_FREE(msg->unique_gift);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *boost_added = NULL;
    if (tb_json_get_ex(obj, "boost_added", &boost_added))
    {
        msg->boost_added = calloc(1, sizeof(telebot_chat_boost_added_t));
        if (telebot_parser_get_chat_boost_added(boost_added, msg->boost_added) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <boost_added> from message object");
            TELEBOT_SAFE_FREE(msg->boost_added);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *chat_background_set = NULL;
    if (tb_json_get_ex(obj, "chat_background_set", &chat_background_set))
    {
        msg->chat_background_set = calloc(1, sizeof(telebot_chat_background_t));
        if (telebot_parser_get_chat_background(chat_background_set, msg->chat_background_set) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <chat_background_set> from message object");
            TELEBOT_SAFE_FREE(msg->chat_background_set);
        }
    }
#endif

#if TELEBOT_FEATURE_GIFTS
    struct cJSON *giveaway_created = NULL;
    if (tb_json_get_ex(obj, "giveaway_created", &giveaway_created))
    {
        msg->giveaway_created = calloc(1, sizeof(telebot_giveaway_created_t));
        if (telebot_parser_get_giveaway_created(giveaway_created, msg->giveaway_created) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <giveaway_created> from message object");
            TELEBOT_SAFE_FREE(msg->giveaway_created);
        }
    }
#endif

#if TELEBOT_FEATURE_GIFTS
    struct cJSON *giveaway_completed = NULL;
    if (tb_json_get_ex(obj, "giveaway_completed", &giveaway_completed))
    {
        msg->giveaway_completed = calloc(1, sizeof(telebot_giveaway_completed_t));
        if (telebot_parser_get_giveaway_completed(giveaway_completed, msg->giveaway_completed) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <giveaway_completed> from message object");
            TELEBOT_SAFE_FREE(msg->giveaway_completed);
        }
    }
#endif

    struct cJSON *reply_markup = NULL;
    if (tb_json_get_ex(obj, "reply_markup", &reply_markup))
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

telebot_error_e telebot_parser_get_message_entity(struct cJSON *obj, telebot_message_entity_t *entity)
{
    if ((obj == NULL) || (entity == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(entity, 0, sizeof(telebot_message_entity_t));
    struct cJSON *type = NULL;
    if (!tb_json_get_ex(obj, "type", &type))
    {
        ERR("Object is not message entity type, type not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    entity->type = TELEBOT_SAFE_STRDUP(tb_json_get_string(type));

    struct cJSON *offset = NULL;
    if (!tb_json_get_ex(obj, "offset", &offset))
    {
        TELEBOT_SAFE_FREE(entity->type);
        ERR("Object is not message entity type, offset not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    entity->offset = tb_json_get_int(offset);

    struct cJSON *length = NULL;
    if (!tb_json_get_ex(obj, "length", &length))
    {
        ERR("Object is not message entity type, length not found");
        TELEBOT_SAFE_FZCNT(entity->type, entity->offset);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    entity->length = tb_json_get_int(length);

    struct cJSON *url = NULL;
    if (tb_json_get_ex(obj, "url", &url))
        entity->url = TELEBOT_SAFE_STRDUP(tb_json_get_string(url));

    struct cJSON *user = NULL;
    if (tb_json_get_ex(obj, "user", &user))
    {
        entity->user = calloc(1, sizeof(telebot_user_t));
        if (telebot_parser_get_user(user, entity->user) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <user> from message entity object");
            TELEBOT_SAFE_FREE(entity->user);
        }
    }

    struct cJSON *language = NULL;
    if (tb_json_get_ex(obj, "language", &language))
        entity->language = TELEBOT_SAFE_STRDUP(tb_json_get_string(language));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_message_entities(struct cJSON *obj, telebot_message_entity_t **entities, int *count)
{
    struct cJSON *array = obj;
    int array_len = tb_json_array_length(array);
    if (array_len == 0)
        return TELEBOT_ERROR_OPERATION_FAILED;

    telebot_message_entity_t *result = calloc(array_len, sizeof(telebot_message_entity_t));
    if (result == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    *count = array_len;
    *entities = result;

    for (int index = 0; index < array_len; index++)
    {
        struct cJSON *item = tb_json_array_get_idx(array, index);
        if (telebot_parser_get_message_entity(item, &(result[index])) != TELEBOT_ERROR_NONE)
            ERR("Failed to parse user from users array");
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_photo(struct cJSON *obj, telebot_photo_t *photo)
{
    if ((obj == NULL) || (photo == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(photo, 0, sizeof(telebot_photo_t));
    struct cJSON *file_id = NULL;
    if (!tb_json_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not photo size type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->file_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_id));

    struct cJSON *width = NULL;
    if (!tb_json_get_ex(obj, "width", &width))
    {
        ERR("Object is not photo size type, width not found");
        TELEBOT_SAFE_FREE(photo->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->width = tb_json_get_int(width);

    struct cJSON *height = NULL;
    if (!tb_json_get_ex(obj, "height", &height))
    {
        ERR("Object is not photo size type, height not found");
        TELEBOT_SAFE_FZCNT(photo->file_id, photo->width);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->height = tb_json_get_int(height);

    struct cJSON *file_size = NULL;
    if (tb_json_get_ex(obj, "file_size", &file_size))
        photo->file_size = tb_json_get_int(file_size);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_photos(struct cJSON *obj, telebot_photo_t **photos, int *count)
{
    struct cJSON *array = obj;
    int array_len = tb_json_array_length(array);
    if (array_len == 0)
        return TELEBOT_ERROR_OPERATION_FAILED;

    telebot_photo_t *result = calloc(array_len, sizeof(telebot_photo_t));
    if (result == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    *count = array_len;
    *photos = result;

    for (int index = 0; index < array_len; index++)
    {
        struct cJSON *item = tb_json_array_get_idx(array, index);
        if (telebot_parser_get_photo(item, &(result[index])) != TELEBOT_ERROR_NONE)
            ERR("Failed to parse user from users array");
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_audio(struct cJSON *obj, telebot_audio_t *audio)
{
    if ((obj == NULL) || (audio == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(audio, 0, sizeof(telebot_audio_t));
    struct cJSON *file_id = NULL;
    if (!tb_json_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not audio type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    audio->file_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_id));

    struct cJSON *file_unique_id = NULL;
    if (!tb_json_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not audio type, file_unique_id not found");
        TELEBOT_SAFE_FREE(audio->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    audio->file_unique_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_unique_id));

    struct cJSON *duration = NULL;
    if (!tb_json_get_ex(obj, "duration", &duration))
    {
        ERR("Object is not audio type, duration not found");
        TELEBOT_SAFE_FREE(audio->file_id);
        TELEBOT_SAFE_FREE(audio->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    audio->duration = tb_json_get_int(duration);

    struct cJSON *performer = NULL;
    if (tb_json_get_ex(obj, "performer", &performer))
        audio->performer = TELEBOT_SAFE_STRDUP(tb_json_get_string(performer));

    struct cJSON *title = NULL;
    if (tb_json_get_ex(obj, "title", &title))
        audio->title = TELEBOT_SAFE_STRDUP(tb_json_get_string(title));

    struct cJSON *mime_type = NULL;
    if (tb_json_get_ex(obj, "mime_type", &mime_type))
        audio->mime_type = TELEBOT_SAFE_STRDUP(tb_json_get_string(mime_type));

    struct cJSON *file_size = NULL;
    if (tb_json_get_ex(obj, "file_size", &file_size))
        audio->file_size = tb_json_get_int(file_size);

    struct cJSON *thumb;
    if (tb_json_get_ex(obj, "thumbnail", &thumb))
    {
        audio->thumbnail = calloc(1, sizeof(telebot_photo_t));
        if (telebot_parser_get_photo(thumb, audio->thumbnail) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <thumbnail> from audio object");
            TELEBOT_SAFE_FREE(audio->thumbnail);
        }
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_document(struct cJSON *obj, telebot_document_t *document)
{
    if ((obj == NULL) || (document == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(document, 0, sizeof(telebot_document_t));
    struct cJSON *file_id = NULL;
    if (!tb_json_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not document type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    document->file_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_id));

    struct cJSON *file_unique_id = NULL;
    if (!tb_json_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not document type, file_unique_id not found");
        TELEBOT_SAFE_FREE(document->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    document->file_unique_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_unique_id));

    struct cJSON *thumb = NULL;
    if (tb_json_get_ex(obj, "thumbnail", &thumb))
    {
        document->thumbnail = calloc(1, sizeof(telebot_photo_t));
        if (telebot_parser_get_photo(thumb, document->thumbnail) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <thumbnail> from document object");
            TELEBOT_SAFE_FREE(document->thumbnail);
        }
    }

    struct cJSON *file_name = NULL;
    if (tb_json_get_ex(obj, "file_name", &file_name))
        document->file_name = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_name));

    struct cJSON *mime_type = NULL;
    if (tb_json_get_ex(obj, "mime_type", &mime_type))
        document->mime_type = TELEBOT_SAFE_STRDUP(tb_json_get_string(mime_type));

    struct cJSON *file_size = NULL;
    if (tb_json_get_ex(obj, "file_size", &file_size))
        document->file_size = tb_json_get_int(file_size);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_video(struct cJSON *obj, telebot_video_t *video)
{
    if ((obj == NULL) || (video == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(video, 0, sizeof(telebot_video_t));
    struct cJSON *file_id = NULL;
    if (!tb_json_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not video type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video->file_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_id));

    struct cJSON *file_unique_id = NULL;
    if (!tb_json_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not video type, file_unique_id not found");
        TELEBOT_SAFE_FREE(video->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video->file_unique_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_unique_id));

    struct cJSON *width = NULL;
    if (!tb_json_get_ex(obj, "width", &width))
    {
        ERR("Object is not video type, width not found");
        TELEBOT_SAFE_FREE(video->file_id);
        TELEBOT_SAFE_FREE(video->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video->width = tb_json_get_int(width);

    struct cJSON *height = NULL;
    if (!tb_json_get_ex(obj, "height", &height))
    {
        ERR("Object is not video type, height not found");
        TELEBOT_SAFE_FREE(video->file_id);
        TELEBOT_SAFE_FREE(video->file_unique_id);
        video->width = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video->height = tb_json_get_int(height);

    struct cJSON *duration = NULL;
    if (!tb_json_get_ex(obj, "duration", &duration))
    {
        ERR("Object is not video type, duration not found");
        TELEBOT_SAFE_FREE(video->file_id);
        TELEBOT_SAFE_FREE(video->file_unique_id);
        video->width = 0;
        video->height = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video->duration = tb_json_get_int(duration);

    struct cJSON *thumb = NULL;
    if (tb_json_get_ex(obj, "thumbnail", &thumb))
    {
        video->thumbnail = calloc(1, sizeof(telebot_photo_t));
        if (telebot_parser_get_photo(thumb, video->thumbnail) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <thumbnail> from video object");
            TELEBOT_SAFE_FREE(video->thumbnail);
        }
    }

    struct cJSON *mime_type = NULL;
    if (tb_json_get_ex(obj, "mime_type", &mime_type))
        video->mime_type = TELEBOT_SAFE_STRDUP(tb_json_get_string(mime_type));

    struct cJSON *file_size = NULL;
    if (tb_json_get_ex(obj, "file_size", &file_size))
        video->file_size = tb_json_get_int(file_size);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_animation(struct cJSON *obj, telebot_animation_t *animation)
{
    if ((obj == NULL) || (animation == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(animation, 0, sizeof(telebot_animation_t));
    struct cJSON *file_id = NULL;
    if (!tb_json_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not animation type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    animation->file_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_id));

    struct cJSON *file_unique_id = NULL;
    if (!tb_json_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not animation type, file_unique_id not found");
        TELEBOT_SAFE_FREE(animation->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    animation->file_unique_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_unique_id));

    struct cJSON *width = NULL;
    if (!tb_json_get_ex(obj, "width", &width))
    {
        ERR("Object is not animation type, width not found");
        TELEBOT_SAFE_FREE(animation->file_id);
        TELEBOT_SAFE_FREE(animation->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    animation->width = tb_json_get_int(width);

    struct cJSON *height = NULL;
    if (!tb_json_get_ex(obj, "height", &height))
    {
        ERR("Object is not animation type, height not found");
        TELEBOT_SAFE_FREE(animation->file_id);
        TELEBOT_SAFE_FREE(animation->file_unique_id);
        animation->width = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    animation->height = tb_json_get_int(height);

    struct cJSON *duration = NULL;
    if (!tb_json_get_ex(obj, "duration", &duration))
    {
        ERR("Object is not animation type, duration not found");
        TELEBOT_SAFE_FREE(animation->file_id);
        TELEBOT_SAFE_FREE(animation->file_unique_id);
        animation->width = 0;
        animation->height = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    animation->duration = tb_json_get_int(duration);

    struct cJSON *thumb = NULL;
    if (tb_json_get_ex(obj, "thumbnail", &thumb))
    {
        animation->thumbnail = calloc(1, sizeof(telebot_photo_t));
        if (telebot_parser_get_photo(thumb, animation->thumbnail) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <thumbnail> from animation object");
            TELEBOT_SAFE_FREE(animation->thumbnail);
        }
    }

    struct cJSON *mime_type = NULL;
    if (tb_json_get_ex(obj, "mime_type", &mime_type))
        animation->mime_type = TELEBOT_SAFE_STRDUP(tb_json_get_string(mime_type));

    struct cJSON *file_size = NULL;
    if (tb_json_get_ex(obj, "file_size", &file_size))
        animation->file_size = tb_json_get_int(file_size);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_video_note(struct cJSON *obj, telebot_video_note_t *video_note)
{
    if ((obj == NULL) || (video_note == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(video_note, 0, sizeof(telebot_video_note_t));
    struct cJSON *file_id = NULL;
    if (!tb_json_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not video note type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video_note->file_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_id));

    struct cJSON *file_unique_id = NULL;
    if (!tb_json_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not video_note type, file_unique_id not found");
        TELEBOT_SAFE_FREE(video_note->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video_note->file_unique_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_unique_id));

    struct cJSON *length = NULL;
    if (!tb_json_get_ex(obj, "length", &length))
    {
        ERR("Object is not video note type, length not found");
        TELEBOT_SAFE_FREE(video_note->file_id);
        TELEBOT_SAFE_FREE(video_note->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video_note->length = tb_json_get_int(length);

    struct cJSON *duration = NULL;
    if (!tb_json_get_ex(obj, "duration", &duration))
    {
        ERR("Object is not video note type, duration not found");
        TELEBOT_SAFE_FREE(video_note->file_id);
        TELEBOT_SAFE_FREE(video_note->file_unique_id);
        video_note->length = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video_note->duration = tb_json_get_int(duration);

    struct cJSON *thumb;
    if (tb_json_get_ex(obj, "thumbnail", &thumb))
    {
        video_note->thumbnail = calloc(1, sizeof(telebot_photo_t));
        if (telebot_parser_get_photo(thumb, video_note->thumbnail) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <thumbnail> from video note object");
            TELEBOT_SAFE_FREE(video_note->thumbnail);
        }
    }

    struct cJSON *file_size = NULL;
    if (tb_json_get_ex(obj, "file_size", &file_size))
        video_note->file_size = tb_json_get_int(file_size);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_voice(struct cJSON *obj, telebot_voice_t *voice)
{
    if ((obj == NULL) || (voice == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(voice, 0, sizeof(telebot_voice_t));
    struct cJSON *file_id = NULL;
    if (!tb_json_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not voice type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    voice->file_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_id));

    struct cJSON *file_unique_id = NULL;
    if (!tb_json_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not voice type, file_unique_id not found");
        TELEBOT_SAFE_FREE(voice->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    voice->file_unique_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_unique_id));

    struct cJSON *duration = NULL;
    if (!tb_json_get_ex(obj, "duration", &duration))
    {
        ERR("Object is not voice type, voice duration not found");
        TELEBOT_SAFE_FREE(voice->file_id);
        TELEBOT_SAFE_FREE(voice->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    voice->duration = tb_json_get_int(duration);

    struct cJSON *mime_type = NULL;
    if (tb_json_get_ex(obj, "mime_type", &mime_type))
        voice->mime_type = TELEBOT_SAFE_STRDUP(tb_json_get_string(mime_type));

    struct cJSON *file_size = NULL;
    if (tb_json_get_ex(obj, "file_size", &file_size))
        voice->file_size = tb_json_get_int(file_size);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_contact(struct cJSON *obj, telebot_contact_t *contact)
{
    if ((obj == NULL) || (contact == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(contact, 0, sizeof(telebot_contact_t));
    struct cJSON *phone_number = NULL;
    if (!tb_json_get_ex(obj, "phone_number", &phone_number))
    {
        ERR("Object is not contact type, phone number not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    contact->phone_number = TELEBOT_SAFE_STRDUP(tb_json_get_string(phone_number));

    struct cJSON *first_name = NULL;
    if (!tb_json_get_ex(obj, "first_name", &first_name))
    {
        ERR("Object is not contact type, first name not found");
        TELEBOT_SAFE_FREE(contact->phone_number);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    contact->first_name = TELEBOT_SAFE_STRDUP(tb_json_get_string(first_name));

    struct cJSON *last_name = NULL;
    if (tb_json_get_ex(obj, "last_name", &last_name))
        contact->last_name = TELEBOT_SAFE_STRDUP(tb_json_get_string(last_name));

    struct cJSON *user_id = NULL;
    if (tb_json_get_ex(obj, "user_id", &user_id))
        contact->user_id = tb_json_get_int(user_id);

    struct cJSON *vcard = NULL;
    if (tb_json_get_ex(obj, "vcard", &vcard))
        contact->vcard = TELEBOT_SAFE_STRDUP(tb_json_get_string(vcard));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_location(struct cJSON *obj, telebot_location_t *location)
{
    if ((obj == NULL) || (location == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(location, 0, sizeof(telebot_location_t));
    struct cJSON *latitude = NULL;
    if (!tb_json_get_ex(obj, "latitude", &latitude))
    {
        ERR("Object is not location type, latitude not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    location->latitude = tb_json_get_double(latitude);

    struct cJSON *longitude = NULL;
    if (!tb_json_get_ex(obj, "longitude", &longitude))
    {
        ERR("Object is not location type, longitude not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    location->longitude = tb_json_get_double(longitude);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_venue(struct cJSON *obj, telebot_venue_t *venue)
{
    if ((obj == NULL) || (venue == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(venue, 0, sizeof(telebot_venue_t));
    struct cJSON *title = NULL;
    if (!tb_json_get_ex(obj, "title", &title))
    {
        ERR("Object is not venue type, title not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    venue->title = TELEBOT_SAFE_STRDUP(tb_json_get_string(title));

    struct cJSON *address = NULL;
    if (!tb_json_get_ex(obj, "address", &address))
    {
        ERR("Object is not venue type, address not found");
        TELEBOT_SAFE_FREE(venue->title);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    venue->address = TELEBOT_SAFE_STRDUP(tb_json_get_string(address));

    struct cJSON *location = NULL;
    if (tb_json_get_ex(obj, "location", &location))
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

    struct cJSON *foursquare_id = NULL;
    if (tb_json_get_ex(obj, "foursquare_id", &foursquare_id))
        venue->foursquare_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(foursquare_id));

    struct cJSON *foursquare_type = NULL;
    if (tb_json_get_ex(obj, "foursquare_type", &foursquare_type))
        venue->foursquare_type = TELEBOT_SAFE_STRDUP(tb_json_get_string(foursquare_type));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_poll_option(struct cJSON *obj, telebot_poll_option_t *option)
{
    if ((obj == NULL) || (option == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(option, 0, sizeof(telebot_poll_option_t));
    struct cJSON *text = NULL;
    if (!tb_json_get_ex(obj, "text", &text))
    {
        ERR("Object is not poll option type, text not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    option->text = TELEBOT_SAFE_STRDUP(tb_json_get_string(text));

    struct cJSON *voter_count = NULL;
    if (!tb_json_get_ex(obj, "voter_count", &voter_count))
    {
        ERR("Object is not pol option type, voter_count not found");
        TELEBOT_SAFE_FREE(option->text);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    option->voter_count = tb_json_get_int(voter_count);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_poll_answer(struct cJSON *obj, telebot_poll_answer_t *answer)
{
    if ((obj == NULL) || (answer == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(answer, 0, sizeof(telebot_poll_answer_t));
    struct cJSON *poll_id = NULL;
    if (!tb_json_get_ex(obj, "poll_id", &poll_id))
    {
        ERR("Object is not poll answer type, poll_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    answer->poll_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(poll_id));

    struct cJSON *option_ids = NULL;
    if (!tb_json_get_ex(obj, "option_ids", &option_ids))
    {
        ERR("Object is not pol answer type, option_ids not found");
        TELEBOT_SAFE_FREE(answer->poll_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    answer->count_option_ids = tb_json_array_length(option_ids);
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
        answer->option_ids[index] = tb_json_get_int(tb_json_array_get_idx(option_ids, index));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_poll(struct cJSON *obj, telebot_poll_t *poll)
{
    if ((obj == NULL) || (poll == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(poll, 0, sizeof(telebot_poll_t));
    struct cJSON *id = NULL;
    if (!tb_json_get_ex(obj, "id", &id))
    {
        ERR("Object is not poll type, id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->id = TELEBOT_SAFE_STRDUP(tb_json_get_string(id));

    struct cJSON *question = NULL;
    if (!tb_json_get_ex(obj, "question", &question))
    {
        ERR("Object is not pol type, question not found");
        TELEBOT_SAFE_FREE(poll->id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->question = TELEBOT_SAFE_STRDUP(tb_json_get_string(question));

    struct cJSON *total_voter_count = NULL;
    if (!tb_json_get_ex(obj, "total_voter_count", &total_voter_count))
    {
        ERR("Object is not pol type, total_voter_count not found");
        TELEBOT_SAFE_FREE(poll->id);
        TELEBOT_SAFE_FREE(poll->question);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->total_voter_count = tb_json_get_int(total_voter_count);

    struct cJSON *is_closed = NULL;
    if (!tb_json_get_ex(obj, "is_closed", &is_closed))
    {
        ERR("Object is not pol type, is_closed not found");
        TELEBOT_SAFE_FREE(poll->id);
        TELEBOT_SAFE_FREE(poll->question);
        poll->total_voter_count = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->is_closed = tb_json_get_bool(is_closed);

    struct cJSON *is_anonymous = NULL;
    if (!tb_json_get_ex(obj, "is_anonymous", &is_anonymous))
    {
        ERR("Object is not pol type, is_anonymous not found");
        TELEBOT_SAFE_FREE(poll->id);
        TELEBOT_SAFE_FREE(poll->question);
        poll->total_voter_count = 0;
        poll->is_closed = false;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->is_anonymous = tb_json_get_bool(is_anonymous);

    struct cJSON *type = NULL;
    if (!tb_json_get_ex(obj, "type", &type))
    {
        ERR("Object is not pol type, type not found");
        TELEBOT_SAFE_FREE(poll->id);
        TELEBOT_SAFE_FREE(poll->question);
        poll->total_voter_count = 0;
        poll->is_closed = false;
        poll->is_anonymous = false;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->type = TELEBOT_SAFE_STRDUP(tb_json_get_string(type));

    struct cJSON *allows_multiple_answers = NULL;
    if (!tb_json_get_ex(obj, "allows_multiple_answers", &allows_multiple_answers))
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
    poll->allows_multiple_answers = tb_json_get_bool(allows_multiple_answers);

    struct cJSON *options = NULL;
    if (!tb_json_get_ex(obj, "options", &options))
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

    poll->count_options = tb_json_array_length(options);
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
        telebot_parser_get_poll_option(tb_json_array_get_idx(options, index), &(poll->options[index]));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_dice(struct cJSON *obj, telebot_dice_t *dice)
{
    if ((obj == NULL) || (dice == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(dice, 0, sizeof(telebot_dice_t));
    struct cJSON *value = NULL;
    if (!tb_json_get_ex(obj, "value", &value))
    {
        ERR("Object is not dice type, value not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    dice->value = tb_json_get_int(value);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user_profile_photos(struct cJSON *obj, telebot_user_profile_photos_t *photos)
{
    if ((obj == NULL) || (photos == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct cJSON *total_count_obj;
    if (!tb_json_get_ex(obj, "total_count", &total_count_obj))
    {
        ERR("Object is not user profile photo type, total_count not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photos->total_count = tb_json_get_int(total_count_obj);
    if (photos->total_count == 0)
        return TELEBOT_ERROR_NONE;

    struct cJSON *array;
    if (!tb_json_get_ex(obj, "photos", &array))
    {
        ERR("Failed to get photos from <UserProfilePhotos> object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    int total = tb_json_array_length(array);
    for (int i = 0; i < 4; i++)
        photos->photos[i] = calloc(total, sizeof(telebot_photo_t));
    photos->current_count = total;

    telebot_error_e ret = TELEBOT_ERROR_NONE;
    for (int i = 0; i < total; i++)
    {
        struct cJSON *item = tb_json_array_get_idx(array, i);
        int subtotal = tb_json_array_length(item);
        if (subtotal > 4)
            subtotal = 4; /* This MUST not happen */
        for (int j = 0; j < subtotal; j++)
        {
            struct cJSON *photo = tb_json_array_get_idx(item, j);
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

telebot_error_e telebot_parser_get_file(struct cJSON *obj, telebot_file_t *file)
{
    if ((obj == NULL) || (file == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct cJSON *file_id = NULL;
    if (!tb_json_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not file type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    file->file_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_id));

    struct cJSON *file_unique_id = NULL;
    if (!tb_json_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not file type, file_unique_id not found");
        TELEBOT_SAFE_FREE(file->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    file->file_unique_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_unique_id));

    struct cJSON *file_path = NULL;
    if (!tb_json_get_ex(obj, "file_path", &file_path))
    {
        ERR("Object is not file type, file_path not found");
        TELEBOT_SAFE_FREE(file->file_id);
        TELEBOT_SAFE_FREE(file->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    file->file_path = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_path));

    struct cJSON *file_size;
    if (tb_json_get_ex(obj, "file_size", &file_size))
        file->file_size = tb_json_get_int(file_size);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_callback_query(struct cJSON *obj, telebot_callback_query_t *cb_query)
{
    if ((obj == NULL) || (cb_query == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct cJSON *id = NULL;
    if (!tb_json_get_ex(obj, "id", &id))
    {
        ERR("Failed to get <id> from callback_query object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    cb_query->id = TELEBOT_SAFE_STRDUP(tb_json_get_string(id));

    struct cJSON *from = NULL;
    if (tb_json_get_ex(obj, "from", &from))
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

    struct cJSON *message = NULL;
    if (tb_json_get_ex(obj, "message", &message))
    {
        cb_query->message = calloc(1, sizeof(telebot_message_t));
        if (telebot_parser_get_message(message, cb_query->message) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <message> from callback_query object");
            TELEBOT_SAFE_FREE(cb_query->message);
        }
    }

    struct cJSON *inline_message_id = NULL;
    if (tb_json_get_ex(obj, "inline_message_id", &inline_message_id))
        cb_query->inline_message_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(inline_message_id));

    struct cJSON *chat_instance = NULL;
    if (tb_json_get_ex(obj, "chat_instance", &chat_instance))
        cb_query->chat_instance = TELEBOT_SAFE_STRDUP(tb_json_get_string(chat_instance));

    struct cJSON *data = NULL;
    if (tb_json_get_ex(obj, "data", &data))
        cb_query->data = TELEBOT_SAFE_STRDUP(tb_json_get_string(data));

    struct cJSON *game_short_name = NULL;
    if (tb_json_get_ex(obj, "game_short_name", &game_short_name))
        cb_query->game_short_name = TELEBOT_SAFE_STRDUP(tb_json_get_string(game_short_name));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_photo(struct cJSON *obj, telebot_chat_photo_t *photo)
{
    if ((obj == NULL) || (photo == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(photo, 0, sizeof(telebot_chat_photo_t));
    struct cJSON *small_file_id = NULL;
    if (!tb_json_get_ex(obj, "small_file_id", &small_file_id))
    {
        ERR("Object is not chat photo type, small_file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->small_file_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(small_file_id));

    struct cJSON *small_file_unique_id = NULL;
    if (!tb_json_get_ex(obj, "small_file_unique_id", &small_file_unique_id))
    {
        ERR("Object is not chat photo type, small_file_unique_id not found");
        TELEBOT_SAFE_FREE(photo->small_file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->small_file_unique_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(small_file_unique_id));

    struct cJSON *big_file_id = NULL;
    if (!tb_json_get_ex(obj, "big_file_id", &big_file_id))
    {
        ERR("Object is not chat photo type, big_file_id not found");
        TELEBOT_SAFE_FREE(photo->small_file_id);
        TELEBOT_SAFE_FREE(photo->small_file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->big_file_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(big_file_id));

    struct cJSON *big_file_unique_id = NULL;
    if (!tb_json_get_ex(obj, "big_file_unique_id", &big_file_unique_id))
    {
        ERR("Object is not chat photo type, big_file_unique_id not found");
        TELEBOT_SAFE_FREE(photo->small_file_id);
        TELEBOT_SAFE_FREE(photo->small_file_unique_id);
        TELEBOT_SAFE_FREE(photo->big_file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->big_file_unique_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(big_file_unique_id));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_member(struct cJSON *obj, telebot_chat_member_t *member)
{
    if ((obj == NULL) || (member == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(member, 0, sizeof(telebot_chat_member_t));
    cJSON *tmp = NULL;
    cJSON *user = NULL;
    if (!tb_json_get_ex(obj, "status", &tmp) || !tb_json_get_ex(obj, "user", &user))
    {
        ERR("Object is not a chat member: status or user missing");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    member->status = TELEBOT_SAFE_STRDUP(tb_json_get_string(tmp));

    member->user = calloc(1, sizeof(telebot_user_t));
    if ((member->user == NULL) || (telebot_parser_get_user(user, member->user) != TELEBOT_ERROR_NONE))
    {
        ERR("Failed to get <user> from chat member object");
        TELEBOT_SAFE_FREE(member->status);
        TELEBOT_SAFE_FREE(member->user);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    if (tb_json_get_ex(obj, "custom_title", &tmp))
        member->custom_title = TELEBOT_SAFE_STRDUP(tb_json_get_string(tmp));
    if (tb_json_get_ex(obj, "tag", &tmp))
        member->tag = TELEBOT_SAFE_STRDUP(tb_json_get_string(tmp));
    if (tb_json_get_ex(obj, "until_date", &tmp))
        member->until_date = tb_json_get_int(tmp);
    if (tb_json_get_ex(obj, "is_anonymous", &tmp))
        member->is_anonymous = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "can_be_edited", &tmp))
        member->can_be_edited = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "is_member", &tmp))
        member->is_member = tb_json_get_bool(tmp);

    /* Rights and permissions are flat boolean fields of the ChatMember object */
    for (size_t f = 0; f < TELEBOT_FIELD_COUNT(telebot_chat_administrator_rights_fields); f++)
    {
        if (tb_json_get_ex(obj, telebot_chat_administrator_rights_fields[f].key, &tmp))
            *telebot_bool_field_ptr(&member->rights, &telebot_chat_administrator_rights_fields[f]) =
                tb_json_get_bool(tmp);
    }
    for (size_t f = 0; f < TELEBOT_FIELD_COUNT(telebot_chat_permissions_fields); f++)
    {
        if (tb_json_get_ex(obj, telebot_chat_permissions_fields[f].key, &tmp))
            *telebot_bool_field_ptr(&member->permissions, &telebot_chat_permissions_fields[f]) =
                tb_json_get_bool(tmp);
    }
    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_permissions(struct cJSON *obj, telebot_chat_permissions_t *permissions)
{
    if ((obj == NULL) || (permissions == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(permissions, 0, sizeof(telebot_chat_permissions_t));
    for (size_t i = 0; i < TELEBOT_FIELD_COUNT(telebot_chat_permissions_fields); i++)
    {
        cJSON *value = NULL;
        if (tb_json_get_ex(obj, telebot_chat_permissions_fields[i].key, &value))
            *telebot_bool_field_ptr(permissions, &telebot_chat_permissions_fields[i]) = tb_json_get_bool(value);
    }
    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_bot_command(struct cJSON *obj, telebot_bot_command_t *botcmd)
{
    if ((obj == NULL) || (botcmd == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(botcmd, 0, sizeof(telebot_bot_command_t));
    struct cJSON *command = NULL;
    if (!tb_json_get_ex(obj, "command", &command))
    {
        ERR("Object is not bot command type, command not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    botcmd->command = TELEBOT_SAFE_STRDUP(tb_json_get_string(command));

    struct cJSON *description = NULL;
    if (!tb_json_get_ex(obj, "description", &description))
    {
        ERR("Object is not bot command type, description not found");
        TELEBOT_SAFE_FREE(botcmd->command);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    botcmd->description = TELEBOT_SAFE_STRDUP(tb_json_get_string(description));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_array_bot_command(struct cJSON *obj, telebot_bot_command_t **cmds, int *count)
{
    int ret = TELEBOT_ERROR_NONE;
    if ((obj == NULL) || (cmds == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct cJSON *array = obj;
    int array_len = tb_json_array_length(array);
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
        struct cJSON *item = tb_json_array_get_idx(array, index);
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

telebot_error_e telebot_parser_get_chat_location(struct cJSON *obj, telebot_chat_location_t *chat_location)
{
    if ((obj == NULL) || (chat_location == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct cJSON *address = NULL;
    if (!tb_json_get_ex(obj, "address", &address))
    {
        ERR("Object is not chat type, address not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    chat_location->address = TELEBOT_SAFE_STRDUP(tb_json_get_string(address));

    struct cJSON *location = NULL;
    if (tb_json_get_ex(obj, "location", &location))
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
telebot_parser_get_message_auto_delete_timer_changed(struct cJSON *obj,
                                                     telebot_message_auto_delete_timer_changed_t *timer_changed)
{
    if ((obj == NULL) || (timer_changed == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct cJSON *message_auto_delete_time = NULL;
    if (!tb_json_get_ex(obj, "message_auto_delete_time", &message_auto_delete_time))
    {
        ERR("Object is not message auto-delete timer type, message_auto_delete_time not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    timer_changed->message_auto_delete_time = tb_json_get_int(message_auto_delete_time);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_invoice(struct cJSON *obj, telebot_invoice_t *invoice)
{
    if ((obj == NULL) || (invoice == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(invoice, 0, sizeof(telebot_invoice_t));
    struct cJSON *title = NULL;
    if (tb_json_get_ex(obj, "title", &title))
        invoice->title = TELEBOT_SAFE_STRDUP(tb_json_get_string(title));

    struct cJSON *description = NULL;
    if (tb_json_get_ex(obj, "description", &description))
        invoice->description = TELEBOT_SAFE_STRDUP(tb_json_get_string(description));

    struct cJSON *start_parameter = NULL;
    if (tb_json_get_ex(obj, "start_parameter", &start_parameter))
        invoice->start_parameter = TELEBOT_SAFE_STRDUP(tb_json_get_string(start_parameter));

    struct cJSON *currency = NULL;
    if (tb_json_get_ex(obj, "currency", &currency))
        invoice->currency = TELEBOT_SAFE_STRDUP(tb_json_get_string(currency));

    struct cJSON *total_amount = NULL;
    if (tb_json_get_ex(obj, "total_amount", &total_amount))
        invoice->total_amount = tb_json_get_int(total_amount);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_shipping_address(struct cJSON *obj, telebot_shipping_address_t *address)
{
    if ((obj == NULL) || (address == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(address, 0, sizeof(telebot_shipping_address_t));
    struct cJSON *country_code = NULL;
    if (tb_json_get_ex(obj, "country_code", &country_code))
        address->country_code = TELEBOT_SAFE_STRDUP(tb_json_get_string(country_code));

    struct cJSON *state = NULL;
    if (tb_json_get_ex(obj, "state", &state))
        address->state = TELEBOT_SAFE_STRDUP(tb_json_get_string(state));

    struct cJSON *city = NULL;
    if (tb_json_get_ex(obj, "city", &city))
        address->city = TELEBOT_SAFE_STRDUP(tb_json_get_string(city));

    struct cJSON *street_line1 = NULL;
    if (tb_json_get_ex(obj, "street_line1", &street_line1))
        address->street_line1 = TELEBOT_SAFE_STRDUP(tb_json_get_string(street_line1));

    struct cJSON *street_line2 = NULL;
    if (tb_json_get_ex(obj, "street_line2", &street_line2))
        address->street_line2 = TELEBOT_SAFE_STRDUP(tb_json_get_string(street_line2));

    struct cJSON *post_code = NULL;
    if (tb_json_get_ex(obj, "post_code", &post_code))
        address->post_code = TELEBOT_SAFE_STRDUP(tb_json_get_string(post_code));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_order_info(struct cJSON *obj, telebot_order_info_t *info)
{
    if ((obj == NULL) || (info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(info, 0, sizeof(telebot_order_info_t));
    struct cJSON *name = NULL;
    if (tb_json_get_ex(obj, "name", &name))
        info->name = TELEBOT_SAFE_STRDUP(tb_json_get_string(name));

    struct cJSON *phone_number = NULL;
    if (tb_json_get_ex(obj, "phone_number", &phone_number))
        info->phone_number = TELEBOT_SAFE_STRDUP(tb_json_get_string(phone_number));

    struct cJSON *email = NULL;
    if (tb_json_get_ex(obj, "email", &email))
        info->email = TELEBOT_SAFE_STRDUP(tb_json_get_string(email));

    struct cJSON *shipping_address = NULL;
    if (tb_json_get_ex(obj, "shipping_address", &shipping_address))
    {
        info->shipping_address = calloc(1, sizeof(telebot_shipping_address_t));
        telebot_parser_get_shipping_address(shipping_address, info->shipping_address);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_successful_payment(struct cJSON *obj, telebot_successful_payment_t *payment)
{
    if ((obj == NULL) || (payment == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(payment, 0, sizeof(telebot_successful_payment_t));
    struct cJSON *currency = NULL;
    if (tb_json_get_ex(obj, "currency", &currency))
        payment->currency = TELEBOT_SAFE_STRDUP(tb_json_get_string(currency));

    struct cJSON *total_amount = NULL;
    if (tb_json_get_ex(obj, "total_amount", &total_amount))
        payment->total_amount = tb_json_get_int(total_amount);

    struct cJSON *invoice_payload = NULL;
    if (tb_json_get_ex(obj, "invoice_payload", &invoice_payload))
        payment->invoice_payload = TELEBOT_SAFE_STRDUP(tb_json_get_string(invoice_payload));

    struct cJSON *shipping_option_id = NULL;
    if (tb_json_get_ex(obj, "shipping_option_id", &shipping_option_id))
        payment->shipping_option_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(shipping_option_id));

    struct cJSON *order_info = NULL;
    if (tb_json_get_ex(obj, "order_info", &order_info))
    {
        payment->order_info = calloc(1, sizeof(telebot_order_info_t));
        telebot_parser_get_order_info(order_info, payment->order_info);
    }

    struct cJSON *telegram_payment_charge_id = NULL;
    if (tb_json_get_ex(obj, "telegram_payment_charge_id", &telegram_payment_charge_id))
        payment->telegram_payment_charge_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(telegram_payment_charge_id));

    struct cJSON *provider_payment_charge_id = NULL;
    if (tb_json_get_ex(obj, "provider_payment_charge_id", &provider_payment_charge_id))
        payment->provider_payment_charge_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(provider_payment_charge_id));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_passport_file(struct cJSON *obj, telebot_passport_file_t *file)
{
    if ((obj == NULL) || (file == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(file, 0, sizeof(telebot_passport_file_t));
    struct cJSON *file_id = NULL;
    if (tb_json_get_ex(obj, "file_id", &file_id))
        file->file_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_id));

    struct cJSON *file_unique_id = NULL;
    if (tb_json_get_ex(obj, "file_unique_id", &file_unique_id))
        file->file_unique_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(file_unique_id));

    struct cJSON *file_size = NULL;
    if (tb_json_get_ex(obj, "file_size", &file_size))
        file->file_size = tb_json_get_int(file_size);

    struct cJSON *date = NULL;
    if (tb_json_get_ex(obj, "date", &date))
        file->date = tb_json_get_int(date);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_encrypted_passport_element(struct cJSON *obj, telebot_encrypted_passport_element_t *element)
{
    if ((obj == NULL) || (element == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(element, 0, sizeof(telebot_encrypted_passport_element_t));
    struct cJSON *type = NULL;
    if (tb_json_get_ex(obj, "type", &type))
        element->type = TELEBOT_SAFE_STRDUP(tb_json_get_string(type));

    struct cJSON *data = NULL;
    if (tb_json_get_ex(obj, "data", &data))
        element->data = TELEBOT_SAFE_STRDUP(tb_json_get_string(data));

    struct cJSON *phone_number = NULL;
    if (tb_json_get_ex(obj, "phone_number", &phone_number))
        element->phone_number = TELEBOT_SAFE_STRDUP(tb_json_get_string(phone_number));

    struct cJSON *email = NULL;
    if (tb_json_get_ex(obj, "email", &email))
        element->email = TELEBOT_SAFE_STRDUP(tb_json_get_string(email));

    struct cJSON *files = NULL;
    if (tb_json_get_ex(obj, "files", &files))
    {
        int array_len = tb_json_array_length(files);
        element->count_files = array_len;
        element->files = calloc(array_len, sizeof(telebot_passport_file_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_passport_file(tb_json_array_get_idx(files, i), &(element->files[i]));
    }

    struct cJSON *front_side = NULL;
    if (tb_json_get_ex(obj, "front_side", &front_side))
    {
        element->front_side = calloc(1, sizeof(telebot_passport_file_t));
        telebot_parser_get_passport_file(front_side, element->front_side);
    }

    struct cJSON *reverse_side = NULL;
    if (tb_json_get_ex(obj, "reverse_side", &reverse_side))
    {
        element->reverse_side = calloc(1, sizeof(telebot_passport_file_t));
        telebot_parser_get_passport_file(reverse_side, element->reverse_side);
    }

    struct cJSON *selfie = NULL;
    if (tb_json_get_ex(obj, "selfie", &selfie))
    {
        element->selfie = calloc(1, sizeof(telebot_passport_file_t));
        telebot_parser_get_passport_file(selfie, element->selfie);
    }

    struct cJSON *translation = NULL;
    if (tb_json_get_ex(obj, "translation", &translation))
    {
        int array_len = tb_json_array_length(translation);
        element->count_translation = array_len;
        element->translation = calloc(array_len, sizeof(telebot_passport_file_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_passport_file(tb_json_array_get_idx(translation, i), &(element->translation[i]));
    }

    struct cJSON *hash = NULL;
    if (tb_json_get_ex(obj, "hash", &hash))
        element->hash = TELEBOT_SAFE_STRDUP(tb_json_get_string(hash));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_encrypted_credentials(struct cJSON *obj, telebot_encrypted_credentials_t *credentials)
{
    if ((obj == NULL) || (credentials == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(credentials, 0, sizeof(telebot_encrypted_credentials_t));
    struct cJSON *data = NULL;
    if (tb_json_get_ex(obj, "data", &data))
        credentials->data = TELEBOT_SAFE_STRDUP(tb_json_get_string(data));

    struct cJSON *hash = NULL;
    if (tb_json_get_ex(obj, "hash", &hash))
        credentials->hash = TELEBOT_SAFE_STRDUP(tb_json_get_string(hash));

    struct cJSON *secret = NULL;
    if (tb_json_get_ex(obj, "secret", &secret))
        credentials->secret = TELEBOT_SAFE_STRDUP(tb_json_get_string(secret));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_passport_data(struct cJSON *obj, telebot_passport_data_t *passport_data)
{
    if ((obj == NULL) || (passport_data == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(passport_data, 0, sizeof(telebot_passport_data_t));
    struct cJSON *data = NULL;
    if (tb_json_get_ex(obj, "data", &data))
    {
        int array_len = tb_json_array_length(data);
        passport_data->count_data = array_len;
        passport_data->data = calloc(array_len, sizeof(telebot_encrypted_passport_element_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_encrypted_passport_element(tb_json_array_get_idx(data, i), &(passport_data->data[i]));
    }

    struct cJSON *credentials = NULL;
    if (tb_json_get_ex(obj, "credentials", &credentials))
    {
        passport_data->credentials = calloc(1, sizeof(telebot_encrypted_credentials_t));
        telebot_parser_get_encrypted_credentials(credentials, passport_data->credentials);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_proximity_alert_triggered(struct cJSON *obj, telebot_proximity_alert_triggered_t *alert)
{
    if ((obj == NULL) || (alert == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(alert, 0, sizeof(telebot_proximity_alert_triggered_t));
    struct cJSON *traveler = NULL;
    if (tb_json_get_ex(obj, "traveler", &traveler))
    {
        alert->traveler = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(traveler, alert->traveler);
    }

    struct cJSON *watcher = NULL;
    if (tb_json_get_ex(obj, "watcher", &watcher))
    {
        alert->watcher = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(watcher, alert->watcher);
    }

    struct cJSON *distance = NULL;
    if (tb_json_get_ex(obj, "distance", &distance))
        alert->distance = tb_json_get_int(distance);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_forum_topic_created(struct cJSON *obj, telebot_forum_topic_created_t *topic)
{
    if ((obj == NULL) || (topic == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(topic, 0, sizeof(telebot_forum_topic_created_t));
    struct cJSON *name = NULL;
    if (tb_json_get_ex(obj, "name", &name))
        topic->name = TELEBOT_SAFE_STRDUP(tb_json_get_string(name));

    struct cJSON *icon_color = NULL;
    if (tb_json_get_ex(obj, "icon_color", &icon_color))
        topic->icon_color = tb_json_get_int(icon_color);

    struct cJSON *icon_custom_emoji_id = NULL;
    if (tb_json_get_ex(obj, "icon_custom_emoji_id", &icon_custom_emoji_id))
        topic->icon_custom_emoji_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(icon_custom_emoji_id));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_forum_topic_edited(struct cJSON *obj, telebot_forum_topic_edited_t *topic)
{
    if ((obj == NULL) || (topic == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(topic, 0, sizeof(telebot_forum_topic_edited_t));
    struct cJSON *name = NULL;
    if (tb_json_get_ex(obj, "name", &name))
        topic->name = TELEBOT_SAFE_STRDUP(tb_json_get_string(name));

    struct cJSON *icon_custom_emoji_id = NULL;
    if (tb_json_get_ex(obj, "icon_custom_emoji_id", &icon_custom_emoji_id))
        topic->icon_custom_emoji_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(icon_custom_emoji_id));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_video_chat_scheduled(struct cJSON *obj, telebot_video_chat_scheduled_t *scheduled)
{
    if ((obj == NULL) || (scheduled == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(scheduled, 0, sizeof(telebot_video_chat_scheduled_t));
    struct cJSON *start_date = NULL;
    if (tb_json_get_ex(obj, "start_date", &start_date))
        scheduled->start_date = tb_json_get_int(start_date);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_video_chat_ended(struct cJSON *obj, telebot_video_chat_ended_t *ended)
{
    if ((obj == NULL) || (ended == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(ended, 0, sizeof(telebot_video_chat_ended_t));
    struct cJSON *duration = NULL;
    if (tb_json_get_ex(obj, "duration", &duration))
        ended->duration = tb_json_get_int(duration);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_video_chat_participants_invited(struct cJSON *obj, telebot_video_chat_participants_invited_t *invited)
{
    if ((obj == NULL) || (invited == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(invited, 0, sizeof(telebot_video_chat_participants_invited_t));
    struct cJSON *users = NULL;
    if (tb_json_get_ex(obj, "users", &users))
    {
        int array_len = tb_json_array_length(users);
        invited->count_users = array_len;
        invited->users = calloc(array_len, sizeof(telebot_user_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_user(tb_json_array_get_idx(users, i), &(invited->users[i]));
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_web_app_data(struct cJSON *obj, telebot_web_app_data_t *data)
{
    if ((obj == NULL) || (data == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(data, 0, sizeof(telebot_web_app_data_t));
    struct cJSON *data_obj = NULL;
    if (tb_json_get_ex(obj, "data", &data_obj))
        data->data = TELEBOT_SAFE_STRDUP(tb_json_get_string(data_obj));

    struct cJSON *button_text = NULL;
    if (tb_json_get_ex(obj, "button_text", &button_text))
        data->button_text = TELEBOT_SAFE_STRDUP(tb_json_get_string(button_text));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_inline_keyboard_button(struct cJSON *obj, telebot_inline_keyboard_button_t *button)
{
    if ((obj == NULL) || (button == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(button, 0, sizeof(telebot_inline_keyboard_button_t));
    struct cJSON *text = NULL;
    if (tb_json_get_ex(obj, "text", &text))
        button->text = TELEBOT_SAFE_STRDUP(tb_json_get_string(text));

    struct cJSON *url = NULL;
    if (tb_json_get_ex(obj, "url", &url))
        button->url = TELEBOT_SAFE_STRDUP(tb_json_get_string(url));

    struct cJSON *callback_data = NULL;
    if (tb_json_get_ex(obj, "callback_data", &callback_data))
        button->callback_data = TELEBOT_SAFE_STRDUP(tb_json_get_string(callback_data));

    struct cJSON *switch_inline_query = NULL;
    if (tb_json_get_ex(obj, "switch_inline_query", &switch_inline_query))
        button->switch_inline_query = TELEBOT_SAFE_STRDUP(tb_json_get_string(switch_inline_query));

    struct cJSON *switch_inline_query_current_chat = NULL;
    if (tb_json_get_ex(obj, "switch_inline_query_current_chat", &switch_inline_query_current_chat))
        button->switch_inline_query_current_chat = TELEBOT_SAFE_STRDUP(tb_json_get_string(switch_inline_query_current_chat));

    struct cJSON *pay = NULL;
    if (tb_json_get_ex(obj, "pay", &pay))
        button->pay = tb_json_get_bool(pay);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_inline_keyboard_markup(struct cJSON *obj, telebot_inline_keyboard_markup_t *markup)
{
    if ((obj == NULL) || (markup == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(markup, 0, sizeof(telebot_inline_keyboard_markup_t));
    struct cJSON *inline_keyboard = NULL;
    if (tb_json_get_ex(obj, "inline_keyboard", &inline_keyboard))
    {
        int rows = tb_json_array_length(inline_keyboard);
        int cols = 0;
        if (rows > 0)
        {
            struct cJSON *row0 = tb_json_array_get_idx(inline_keyboard, 0);
            cols = tb_json_array_length(row0);
        }
        markup->rows = rows;
        markup->cols = cols;
        markup->inline_keyboard = calloc(rows * cols, sizeof(telebot_inline_keyboard_button_t));
        for (int i = 0; i < rows; i++)
        {
            struct cJSON *row = tb_json_array_get_idx(inline_keyboard, i);
            int row_cols = tb_json_array_length(row);
            for (int j = 0; j < row_cols && j < cols; j++)
            {
                telebot_parser_get_inline_keyboard_button(tb_json_array_get_idx(row, j), &(markup->inline_keyboard[i * cols + j]));
            }
        }
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_invite_link(struct cJSON *obj, telebot_chat_invite_link_t *invite_link)
{
    if ((obj == NULL) || (invite_link == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(invite_link, 0, sizeof(telebot_chat_invite_link_t));
    struct cJSON *invite_link_obj = NULL;
    if (tb_json_get_ex(obj, "invite_link", &invite_link_obj))
        invite_link->invite_link = TELEBOT_SAFE_STRDUP(tb_json_get_string(invite_link_obj));

    struct cJSON *creator = NULL;
    if (tb_json_get_ex(obj, "creator", &creator))
    {
        invite_link->creator = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(creator, invite_link->creator);
    }

    struct cJSON *creates_join_request = NULL;
    if (tb_json_get_ex(obj, "creates_join_request", &creates_join_request))
        invite_link->creates_join_request = tb_json_get_bool(creates_join_request);

    struct cJSON *is_primary = NULL;
    if (tb_json_get_ex(obj, "is_primary", &is_primary))
        invite_link->is_primary = tb_json_get_bool(is_primary);

    struct cJSON *is_revoked = NULL;
    if (tb_json_get_ex(obj, "is_revoked", &is_revoked))
        invite_link->is_revoked = tb_json_get_bool(is_revoked);

    struct cJSON *name = NULL;
    if (tb_json_get_ex(obj, "name", &name))
        invite_link->name = TELEBOT_SAFE_STRDUP(tb_json_get_string(name));

    struct cJSON *expire_date = NULL;
    if (tb_json_get_ex(obj, "expire_date", &expire_date))
        invite_link->expire_date = tb_json_get_int(expire_date);

    struct cJSON *member_limit = NULL;
    if (tb_json_get_ex(obj, "member_limit", &member_limit))
        invite_link->member_limit = tb_json_get_int(member_limit);

    struct cJSON *pending_join_request_count = NULL;
    if (tb_json_get_ex(obj, "pending_join_request_count", &pending_join_request_count))
        invite_link->pending_join_request_count = tb_json_get_int(pending_join_request_count);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_member_updated(struct cJSON *obj, telebot_chat_member_updated_t *updated)
{
    if ((obj == NULL) || (updated == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(updated, 0, sizeof(telebot_chat_member_updated_t));
    struct cJSON *chat = NULL;
    if (tb_json_get_ex(obj, "chat", &chat))
    {
        updated->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, updated->chat);
    }

    struct cJSON *from = NULL;
    if (tb_json_get_ex(obj, "from", &from))
    {
        updated->from = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(from, updated->from);
    }

    struct cJSON *date = NULL;
    if (tb_json_get_ex(obj, "date", &date))
        updated->date = tb_json_get_int(date);

    struct cJSON *old_chat_member = NULL;
    if (tb_json_get_ex(obj, "old_chat_member", &old_chat_member))
    {
        updated->old_chat_member = calloc(1, sizeof(telebot_chat_member_t));
        telebot_parser_get_chat_member(old_chat_member, updated->old_chat_member);
    }

    struct cJSON *new_chat_member = NULL;
    if (tb_json_get_ex(obj, "new_chat_member", &new_chat_member))
    {
        updated->new_chat_member = calloc(1, sizeof(telebot_chat_member_t));
        telebot_parser_get_chat_member(new_chat_member, updated->new_chat_member);
    }

    struct cJSON *invite_link = NULL;
    if (tb_json_get_ex(obj, "invite_link", &invite_link))
    {
        updated->invite_link = calloc(1, sizeof(telebot_chat_invite_link_t));
        telebot_parser_get_chat_invite_link(invite_link, updated->invite_link);
    }

    struct cJSON *via_chat_folder_invite_link = NULL;
    if (tb_json_get_ex(obj, "via_chat_folder_invite_link", &via_chat_folder_invite_link))
        updated->via_chat_folder_invite_link = tb_json_get_bool(via_chat_folder_invite_link);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_join_request(struct cJSON *obj, telebot_chat_join_request_t *request)
{
    if ((obj == NULL) || (request == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(request, 0, sizeof(telebot_chat_join_request_t));
    struct cJSON *chat = NULL;
    if (tb_json_get_ex(obj, "chat", &chat))
    {
        request->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, request->chat);
    }

    struct cJSON *from = NULL;
    if (tb_json_get_ex(obj, "from", &from))
    {
        request->from = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(from, request->from);
    }

    struct cJSON *user_chat_id = NULL;
    if (tb_json_get_ex(obj, "user_chat_id", &user_chat_id))
        request->user_chat_id = tb_json_get_int(user_chat_id);

    struct cJSON *date = NULL;
    if (tb_json_get_ex(obj, "date", &date))
        request->date = tb_json_get_int(date);

    struct cJSON *bio = NULL;
    if (tb_json_get_ex(obj, "bio", &bio))
        request->bio = TELEBOT_SAFE_STRDUP(tb_json_get_string(bio));

    struct cJSON *invite_link = NULL;
    if (tb_json_get_ex(obj, "invite_link", &invite_link))
    {
        request->invite_link = calloc(1, sizeof(telebot_chat_invite_link_t));
        telebot_parser_get_chat_invite_link(invite_link, request->invite_link);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_reaction_type(struct cJSON *obj, telebot_reaction_type_t *reaction)
{
    if ((obj == NULL) || (reaction == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(reaction, 0, sizeof(telebot_reaction_type_t));
    struct cJSON *type = NULL;
    if (tb_json_get_ex(obj, "type", &type))
        reaction->type = TELEBOT_SAFE_STRDUP(tb_json_get_string(type));

    struct cJSON *emoji = NULL;
    if (tb_json_get_ex(obj, "emoji", &emoji))
        reaction->emoji = TELEBOT_SAFE_STRDUP(tb_json_get_string(emoji));

    struct cJSON *custom_emoji_id = NULL;
    if (tb_json_get_ex(obj, "custom_emoji_id", &custom_emoji_id))
        reaction->custom_emoji_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(custom_emoji_id));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_message_reaction_updated(struct cJSON *obj, telebot_message_reaction_updated_t *updated)
{
    if ((obj == NULL) || (updated == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(updated, 0, sizeof(telebot_message_reaction_updated_t));
    struct cJSON *chat = NULL;
    if (tb_json_get_ex(obj, "chat", &chat))
    {
        updated->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, updated->chat);
    }

    struct cJSON *message_id = NULL;
    if (tb_json_get_ex(obj, "message_id", &message_id))
        updated->message_id = tb_json_get_int(message_id);

    struct cJSON *user = NULL;
    if (tb_json_get_ex(obj, "user", &user))
    {
        updated->user = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(user, updated->user);
    }

    struct cJSON *actor_chat = NULL;
    if (tb_json_get_ex(obj, "actor_chat", &actor_chat))
    {
        updated->actor_chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(actor_chat, updated->actor_chat);
    }

    struct cJSON *date = NULL;
    if (tb_json_get_ex(obj, "date", &date))
        updated->date = tb_json_get_int(date);

    struct cJSON *old_reaction = NULL;
    if (tb_json_get_ex(obj, "old_reaction", &old_reaction))
    {
        int array_len = tb_json_array_length(old_reaction);
        updated->count_old_reaction = array_len;
        updated->old_reaction = calloc(array_len, sizeof(telebot_reaction_type_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_reaction_type(tb_json_array_get_idx(old_reaction, i), &(updated->old_reaction[i]));
    }

    struct cJSON *new_reaction = NULL;
    if (tb_json_get_ex(obj, "new_reaction", &new_reaction))
    {
        int array_len = tb_json_array_length(new_reaction);
        updated->count_new_reaction = array_len;
        updated->new_reaction = calloc(array_len, sizeof(telebot_reaction_type_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_reaction_type(tb_json_array_get_idx(new_reaction, i), &(updated->new_reaction[i]));
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_reaction_count(struct cJSON *obj, telebot_reaction_count_t *count)
{
    if ((obj == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(count, 0, sizeof(telebot_reaction_count_t));
    struct cJSON *type = NULL;
    if (tb_json_get_ex(obj, "type", &type))
        telebot_parser_get_reaction_type(type, &(count->type));

    struct cJSON *total_count = NULL;
    if (tb_json_get_ex(obj, "total_count", &total_count))
        count->total_count = tb_json_get_int(total_count);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_message_reaction_count_updated(struct cJSON *obj, telebot_message_reaction_count_updated_t *updated)
{
    if ((obj == NULL) || (updated == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(updated, 0, sizeof(telebot_message_reaction_count_updated_t));
    struct cJSON *chat = NULL;
    if (tb_json_get_ex(obj, "chat", &chat))
    {
        updated->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, updated->chat);
    }

    struct cJSON *message_id = NULL;
    if (tb_json_get_ex(obj, "message_id", &message_id))
        updated->message_id = tb_json_get_int(message_id);

    struct cJSON *date = NULL;
    if (tb_json_get_ex(obj, "date", &date))
        updated->date = tb_json_get_int(date);

    struct cJSON *reactions = NULL;
    if (tb_json_get_ex(obj, "reactions", &reactions))
    {
        int array_len = tb_json_array_length(reactions);
        updated->count_reactions = array_len;
        updated->reactions = calloc(array_len, sizeof(telebot_reaction_count_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_reaction_count(tb_json_array_get_idx(reactions, i), &(updated->reactions[i]));
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_boost_source(struct cJSON *obj, telebot_chat_boost_source_t *source)
{
    if ((obj == NULL) || (source == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(source, 0, sizeof(telebot_chat_boost_source_t));
    struct cJSON *source_obj = NULL;
    if (tb_json_get_ex(obj, "source", &source_obj))
        source->source = TELEBOT_SAFE_STRDUP(tb_json_get_string(source_obj));

    struct cJSON *user = NULL;
    if (tb_json_get_ex(obj, "user", &user))
    {
        source->user = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(user, source->user);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_boost(struct cJSON *obj, telebot_chat_boost_t *boost)
{
    if ((obj == NULL) || (boost == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(boost, 0, sizeof(telebot_chat_boost_t));
    struct cJSON *boost_id = NULL;
    if (tb_json_get_ex(obj, "boost_id", &boost_id))
        boost->boost_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(boost_id));

    struct cJSON *add_date = NULL;
    if (tb_json_get_ex(obj, "add_date", &add_date))
        boost->add_date = tb_json_get_int(add_date);

    struct cJSON *expiration_date = NULL;
    if (tb_json_get_ex(obj, "expiration_date", &expiration_date))
        boost->expiration_date = tb_json_get_int(expiration_date);

    struct cJSON *source = NULL;
    if (tb_json_get_ex(obj, "source", &source))
    {
        boost->source = calloc(1, sizeof(telebot_chat_boost_source_t));
        telebot_parser_get_chat_boost_source(source, boost->source);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_boost_updated(struct cJSON *obj, telebot_chat_boost_updated_t *updated)
{
    if ((obj == NULL) || (updated == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(updated, 0, sizeof(telebot_chat_boost_updated_t));
    struct cJSON *chat = NULL;
    if (tb_json_get_ex(obj, "chat", &chat))
    {
        updated->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, updated->chat);
    }

    struct cJSON *boost = NULL;
    if (tb_json_get_ex(obj, "boost", &boost))
    {
        updated->boost = calloc(1, sizeof(telebot_chat_boost_t));
        telebot_parser_get_chat_boost(boost, updated->boost);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_boost_removed(struct cJSON *obj, telebot_chat_boost_removed_t *removed)
{
    if ((obj == NULL) || (removed == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(removed, 0, sizeof(telebot_chat_boost_removed_t));
    struct cJSON *chat = NULL;
    if (tb_json_get_ex(obj, "chat", &chat))
    {
        removed->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, removed->chat);
    }

    struct cJSON *boost_id = NULL;
    if (tb_json_get_ex(obj, "boost_id", &boost_id))
        removed->boost_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(boost_id));

    struct cJSON *remove_date = NULL;
    if (tb_json_get_ex(obj, "remove_date", &remove_date))
        removed->remove_date = tb_json_get_int(remove_date);

    struct cJSON *source = NULL;
    if (tb_json_get_ex(obj, "source", &source))
    {
        removed->source = calloc(1, sizeof(telebot_chat_boost_source_t));
        telebot_parser_get_chat_boost_source(source, removed->source);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_forum_topic(struct cJSON *obj, telebot_forum_topic_t *topic)
{
    if ((obj == NULL) || (topic == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(topic, 0, sizeof(telebot_forum_topic_t));
    struct cJSON *message_thread_id = NULL;
    if (tb_json_get_ex(obj, "message_thread_id", &message_thread_id))
        topic->message_thread_id = tb_json_get_int(message_thread_id);

    struct cJSON *name = NULL;
    if (tb_json_get_ex(obj, "name", &name))
        topic->name = TELEBOT_SAFE_STRDUP(tb_json_get_string(name));

    struct cJSON *icon_color = NULL;
    if (tb_json_get_ex(obj, "icon_color", &icon_color))
        topic->icon_color = tb_json_get_int(icon_color);

    struct cJSON *icon_custom_emoji_id = NULL;
    if (tb_json_get_ex(obj, "icon_custom_emoji_id", &icon_custom_emoji_id))
        topic->icon_custom_emoji_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(icon_custom_emoji_id));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_inline_query(struct cJSON *obj, telebot_inline_query_t *query)
{
    if ((obj == NULL) || (query == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(query, 0, sizeof(telebot_inline_query_t));
    struct cJSON *id = NULL;
    if (tb_json_get_ex(obj, "id", &id))
        query->id = TELEBOT_SAFE_STRDUP(tb_json_get_string(id));

    struct cJSON *from = NULL;
    if (tb_json_get_ex(obj, "from", &from))
    {
        query->from = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(from, query->from);
    }

    struct cJSON *query_obj = NULL;
    if (tb_json_get_ex(obj, "query", &query_obj))
        query->query = TELEBOT_SAFE_STRDUP(tb_json_get_string(query_obj));

    struct cJSON *offset = NULL;
    if (tb_json_get_ex(obj, "offset", &offset))
        query->offset = TELEBOT_SAFE_STRDUP(tb_json_get_string(offset));

    struct cJSON *chat_type = NULL;
    if (tb_json_get_ex(obj, "chat_type", &chat_type))
        query->chat_type = TELEBOT_SAFE_STRDUP(tb_json_get_string(chat_type));

    struct cJSON *location = NULL;
    if (tb_json_get_ex(obj, "location", &location))
    {
        query->location = calloc(1, sizeof(telebot_location_t));
        telebot_parser_get_location(location, query->location);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chosen_inline_result(struct cJSON *obj, telebot_chosen_inline_result_t *result)
{
    if ((obj == NULL) || (result == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(result, 0, sizeof(telebot_chosen_inline_result_t));
    struct cJSON *result_id = NULL;
    if (tb_json_get_ex(obj, "result_id", &result_id))
        result->result_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(result_id));

    struct cJSON *from = NULL;
    if (tb_json_get_ex(obj, "from", &from))
    {
        result->from = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(from, result->from);
    }

    struct cJSON *location = NULL;
    if (tb_json_get_ex(obj, "location", &location))
    {
        result->location = calloc(1, sizeof(telebot_location_t));
        telebot_parser_get_location(location, result->location);
    }

    struct cJSON *inline_message_id = NULL;
    if (tb_json_get_ex(obj, "inline_message_id", &inline_message_id))
        result->inline_message_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(inline_message_id));

    struct cJSON *query = NULL;
    if (tb_json_get_ex(obj, "query", &query))
        result->query = TELEBOT_SAFE_STRDUP(tb_json_get_string(query));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_shipping_query(struct cJSON *obj, telebot_shipping_query_t *query)
{
    if ((obj == NULL) || (query == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(query, 0, sizeof(telebot_shipping_query_t));
    struct cJSON *id = NULL;
    if (tb_json_get_ex(obj, "id", &id))
        query->id = TELEBOT_SAFE_STRDUP(tb_json_get_string(id));

    struct cJSON *from = NULL;
    if (tb_json_get_ex(obj, "from", &from))
    {
        query->from = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(from, query->from);
    }

    struct cJSON *invoice_payload = NULL;
    if (tb_json_get_ex(obj, "invoice_payload", &invoice_payload))
        query->invoice_payload = TELEBOT_SAFE_STRDUP(tb_json_get_string(invoice_payload));

    struct cJSON *shipping_address = NULL;
    if (tb_json_get_ex(obj, "shipping_address", &shipping_address))
    {
        query->shipping_address = calloc(1, sizeof(telebot_shipping_address_t));
        telebot_parser_get_shipping_address(shipping_address, query->shipping_address);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_pre_checkout_query(struct cJSON *obj, telebot_pre_checkout_query_t *query)
{
    if ((obj == NULL) || (query == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(query, 0, sizeof(telebot_pre_checkout_query_t));
    struct cJSON *id = NULL;
    if (tb_json_get_ex(obj, "id", &id))
        query->id = TELEBOT_SAFE_STRDUP(tb_json_get_string(id));

    struct cJSON *from = NULL;
    if (tb_json_get_ex(obj, "from", &from))
    {
        query->from = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(from, query->from);
    }

    struct cJSON *currency = NULL;
    if (tb_json_get_ex(obj, "currency", &currency))
        query->currency = TELEBOT_SAFE_STRDUP(tb_json_get_string(currency));

    struct cJSON *total_amount = NULL;
    if (tb_json_get_ex(obj, "total_amount", &total_amount))
        query->total_amount = tb_json_get_int(total_amount);

    struct cJSON *invoice_payload = NULL;
    if (tb_json_get_ex(obj, "invoice_payload", &invoice_payload))
        query->invoice_payload = TELEBOT_SAFE_STRDUP(tb_json_get_string(invoice_payload));

    struct cJSON *shipping_option_id = NULL;
    if (tb_json_get_ex(obj, "shipping_option_id", &shipping_option_id))
        query->shipping_option_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(shipping_option_id));

    struct cJSON *order_info = NULL;
    if (tb_json_get_ex(obj, "order_info", &order_info))
    {
        query->order_info = calloc(1, sizeof(telebot_order_info_t));
        telebot_parser_get_order_info(order_info, query->order_info);
    }

    return TELEBOT_ERROR_NONE;
}

static void telebot_parser_get_mask_position(struct cJSON *obj, telebot_mask_position_t *mask)
{
    memset(mask, 0, sizeof(telebot_mask_position_t));
    cJSON *tmp = NULL;
    if (tb_json_get_ex(obj, "point", &tmp))
        mask->point = TELEBOT_SAFE_STRDUP(tb_json_get_string(tmp));
    if (tb_json_get_ex(obj, "x_shift", &tmp))
        mask->x_shift = (float)tb_json_get_double(tmp);
    if (tb_json_get_ex(obj, "y_shift", &tmp))
        mask->y_shift = (float)tb_json_get_double(tmp);
    if (tb_json_get_ex(obj, "scale", &tmp))
        mask->scale = (float)tb_json_get_double(tmp);
}

telebot_error_e telebot_parser_get_sticker(struct cJSON *obj, telebot_sticker_t *sticker)
{
    if ((obj == NULL) || (sticker == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(sticker, 0, sizeof(telebot_sticker_t));
    cJSON *tmp = NULL;

    if (!tb_json_get_ex(obj, "file_id", &tmp))
        return TELEBOT_ERROR_OPERATION_FAILED;
    sticker->file_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(tmp));

    if (tb_json_get_ex(obj, "file_unique_id", &tmp))
        sticker->file_unique_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(tmp));
    if (tb_json_get_ex(obj, "type", &tmp))
        sticker->type = TELEBOT_SAFE_STRDUP(tb_json_get_string(tmp));
    if (tb_json_get_ex(obj, "width", &tmp))
        sticker->width = tb_json_get_int(tmp);
    if (tb_json_get_ex(obj, "height", &tmp))
        sticker->height = tb_json_get_int(tmp);
    if (tb_json_get_ex(obj, "is_animated", &tmp))
        sticker->is_animated = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "is_video", &tmp))
        sticker->is_video = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "emoji", &tmp))
        sticker->emoji = TELEBOT_SAFE_STRDUP(tb_json_get_string(tmp));
    if (tb_json_get_ex(obj, "set_name", &tmp))
        sticker->set_name = TELEBOT_SAFE_STRDUP(tb_json_get_string(tmp));
    if (tb_json_get_ex(obj, "custom_emoji_id", &tmp))
        sticker->custom_emoji_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(tmp));
    if (tb_json_get_ex(obj, "needs_repainting", &tmp))
        sticker->needs_repainting = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "file_size", &tmp))
        sticker->file_size = tb_json_get_int(tmp);

    if (tb_json_get_ex(obj, "thumbnail", &tmp))
    {
        sticker->thumbnail = calloc(1, sizeof(telebot_photo_t));
        if ((sticker->thumbnail != NULL) && (telebot_parser_get_photo(tmp, sticker->thumbnail) != TELEBOT_ERROR_NONE))
            TELEBOT_SAFE_FREE(sticker->thumbnail);
    }

    if (tb_json_get_ex(obj, "mask_position", &tmp))
    {
        sticker->mask_position = calloc(1, sizeof(telebot_mask_position_t));
        if (sticker->mask_position != NULL)
            telebot_parser_get_mask_position(tmp, sticker->mask_position);
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_stickers(struct cJSON *obj, telebot_sticker_t **stickers, int *count)
{
    if ((obj == NULL) || (stickers == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    *stickers = NULL;
    *count = 0;
    int len = tb_json_array_length(obj);
    if (len == 0)
        return TELEBOT_ERROR_NONE;

    telebot_sticker_t *result = calloc(len, sizeof(telebot_sticker_t));
    if (result == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    int index = 0;
    for (cJSON *item = obj->child; item != NULL; item = item->next)
    {
        if (telebot_parser_get_sticker(item, &result[index]) == TELEBOT_ERROR_NONE)
            index++;
    }

    *stickers = result;
    *count = index;
    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_sticker_set(struct cJSON *obj, telebot_sticker_set_t *set)
{
    if ((obj == NULL) || (set == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(set, 0, sizeof(telebot_sticker_set_t));
    cJSON *tmp = NULL;

    if (!tb_json_get_ex(obj, "name", &tmp))
        return TELEBOT_ERROR_OPERATION_FAILED;
    set->name = TELEBOT_SAFE_STRDUP(tb_json_get_string(tmp));

    if (tb_json_get_ex(obj, "title", &tmp))
        set->title = TELEBOT_SAFE_STRDUP(tb_json_get_string(tmp));
    if (tb_json_get_ex(obj, "sticker_type", &tmp))
        set->sticker_type = TELEBOT_SAFE_STRDUP(tb_json_get_string(tmp));
    if (tb_json_get_ex(obj, "stickers", &tmp))
    {
        telebot_error_e ret = telebot_parser_get_stickers(tmp, &set->stickers, &set->count_stickers);
        if (ret != TELEBOT_ERROR_NONE)
            return ret;
    }
    if (tb_json_get_ex(obj, "thumbnail", &tmp))
    {
        set->thumbnail = calloc(1, sizeof(telebot_photo_t));
        if ((set->thumbnail != NULL) && (telebot_parser_get_photo(tmp, set->thumbnail) != TELEBOT_ERROR_NONE))
            TELEBOT_SAFE_FREE(set->thumbnail);
    }
    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_business_bot_rights(struct cJSON *obj, telebot_business_bot_rights_t *rights)
{
    if ((obj == NULL) || (rights == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(rights, 0, sizeof(telebot_business_bot_rights_t));
    struct cJSON *tmp = NULL;
    if (tb_json_get_ex(obj, "can_reply", &tmp))
        rights->can_reply = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "can_read_messages", &tmp))
        rights->can_read_messages = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "can_delete_sent_messages", &tmp))
        rights->can_delete_sent_messages = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "can_delete_all_messages", &tmp))
        rights->can_delete_all_messages = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "can_edit_name", &tmp))
        rights->can_edit_name = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "can_edit_bio", &tmp))
        rights->can_edit_bio = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "can_edit_profile_photo", &tmp))
        rights->can_edit_profile_photo = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "can_edit_username", &tmp))
        rights->can_edit_username = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "can_change_gift_settings", &tmp))
        rights->can_change_gift_settings = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "can_view_gifts_and_stars", &tmp))
        rights->can_view_gifts_and_stars = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "can_convert_gifts_to_stars", &tmp))
        rights->can_convert_gifts_to_stars = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "can_transfer_and_upgrade_gifts", &tmp))
        rights->can_transfer_and_upgrade_gifts = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "can_transfer_stars", &tmp))
        rights->can_transfer_stars = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "can_manage_stories", &tmp))
        rights->can_manage_stories = tb_json_get_bool(tmp);

    return TELEBOT_ERROR_NONE;
}

#if TELEBOT_FEATURE_BUSINESS
static telebot_error_e telebot_parser_get_business_messages_deleted(struct cJSON *obj, telebot_business_messages_deleted_t *deleted)
{
    if ((obj == NULL) || (deleted == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(deleted, 0, sizeof(telebot_business_messages_deleted_t));
    struct cJSON *id = NULL;
    if (tb_json_get_ex(obj, "business_connection_id", &id))
        deleted->business_connection_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(id));

    struct cJSON *chat = NULL;
    if (tb_json_get_ex(obj, "chat", &chat))
    {
        deleted->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, deleted->chat);
    }

    struct cJSON *message_ids = NULL;
    if (tb_json_get_ex(obj, "message_ids", &message_ids))
    {
        int array_len = tb_json_array_length(message_ids);
        deleted->count_message_ids = array_len;
        deleted->message_ids = calloc(array_len, sizeof(int));
        for (int i = 0; i < array_len; i++)
            deleted->message_ids[i] = tb_json_get_int(tb_json_array_get_idx(message_ids, i));
    }

    return TELEBOT_ERROR_NONE;
}
#endif

static telebot_error_e telebot_parser_get_message_origin(struct cJSON *obj, telebot_message_origin_t *origin)
{
    if ((obj == NULL) || (origin == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(origin, 0, sizeof(telebot_message_origin_t));
    struct cJSON *type = NULL;
    if (tb_json_get_ex(obj, "type", &type))
        origin->type = TELEBOT_SAFE_STRDUP(tb_json_get_string(type));

    struct cJSON *date = NULL;
    if (tb_json_get_ex(obj, "date", &date))
        origin->date = tb_json_get_int(date);

    struct cJSON *sender_user = NULL;
    if (tb_json_get_ex(obj, "sender_user", &sender_user))
    {
        origin->sender_user = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(sender_user, origin->sender_user);
    }

    struct cJSON *sender_user_name = NULL;
    if (tb_json_get_ex(obj, "sender_user_name", &sender_user_name))
        origin->sender_user_name = TELEBOT_SAFE_STRDUP(tb_json_get_string(sender_user_name));

    struct cJSON *sender_chat = NULL;
    if (tb_json_get_ex(obj, "sender_chat", &sender_chat))
    {
        origin->sender_chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(sender_chat, origin->sender_chat);
    }

    struct cJSON *author_signature = NULL;
    if (tb_json_get_ex(obj, "author_signature", &author_signature))
        origin->author_signature = TELEBOT_SAFE_STRDUP(tb_json_get_string(author_signature));

    struct cJSON *message_id = NULL;
    if (tb_json_get_ex(obj, "message_id", &message_id))
        origin->message_id = tb_json_get_int(message_id);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_text_quote(struct cJSON *obj, telebot_text_quote_t *quote)
{
    if ((obj == NULL) || (quote == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(quote, 0, sizeof(telebot_text_quote_t));
    struct cJSON *text = NULL;
    if (tb_json_get_ex(obj, "text", &text))
        quote->text = TELEBOT_SAFE_STRDUP(tb_json_get_string(text));

    struct cJSON *entities = NULL;
    if (tb_json_get_ex(obj, "entities", &entities))
        telebot_parser_get_message_entities(entities, &(quote->entities), &(quote->count_entities));

    struct cJSON *position = NULL;
    if (tb_json_get_ex(obj, "position", &position))
        quote->position = tb_json_get_int(position);

    struct cJSON *is_manual = NULL;
    if (tb_json_get_ex(obj, "is_manual", &is_manual))
        quote->is_manual = tb_json_get_bool(is_manual);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_story(struct cJSON *obj, telebot_story_t *story)
{
    if ((obj == NULL) || (story == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(story, 0, sizeof(telebot_story_t));
    struct cJSON *chat = NULL;
    if (tb_json_get_ex(obj, "chat", &chat))
    {
        story->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, story->chat);
    }

    struct cJSON *id = NULL;
    if (tb_json_get_ex(obj, "id", &id))
        story->id = tb_json_get_int(id);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_external_reply_info(struct cJSON *obj, telebot_external_reply_info_t *info)
{
    if ((obj == NULL) || (info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(info, 0, sizeof(telebot_external_reply_info_t));
    struct cJSON *origin = NULL;
    if (tb_json_get_ex(obj, "origin", &origin))
        telebot_parser_get_message_origin(origin, &(info->origin));

    struct cJSON *chat = NULL;
    if (tb_json_get_ex(obj, "chat", &chat))
    {
        info->chat = calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, info->chat);
    }

    struct cJSON *message_id = NULL;
    if (tb_json_get_ex(obj, "message_id", &message_id))
        info->message_id = tb_json_get_int(message_id);

    // ... other optional media fields can be added as needed ...

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_link_preview_options(struct cJSON *obj, telebot_link_preview_options_t *options)
{
    if ((obj == NULL) || (options == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(options, 0, sizeof(telebot_link_preview_options_t));
    struct cJSON *tmp = NULL;
    if (tb_json_get_ex(obj, "is_disabled", &tmp))
        options->is_disabled = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "url", &tmp))
        options->url = TELEBOT_SAFE_STRDUP(tb_json_get_string(tmp));
    if (tb_json_get_ex(obj, "prefer_small_media", &tmp))
        options->prefer_small_media = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "prefer_large_media", &tmp))
        options->prefer_large_media = tb_json_get_bool(tmp);
    if (tb_json_get_ex(obj, "show_above_text", &tmp))
        options->show_above_text = tb_json_get_bool(tmp);

    return TELEBOT_ERROR_NONE;
}

#if TELEBOT_FEATURE_PAYMENTS
static telebot_error_e telebot_parser_get_paid_media(struct cJSON *obj, telebot_paid_media_t *media)
{
    if ((obj == NULL) || (media == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(media, 0, sizeof(telebot_paid_media_t));
    struct cJSON *type = NULL;
    if (tb_json_get_ex(obj, "type", &type))
        media->type = TELEBOT_SAFE_STRDUP(tb_json_get_string(type));

    struct cJSON *width = NULL;
    if (tb_json_get_ex(obj, "width", &width))
        media->width = tb_json_get_int(width);

    struct cJSON *height = NULL;
    if (tb_json_get_ex(obj, "height", &height))
        media->height = tb_json_get_int(height);

    struct cJSON *duration = NULL;
    if (tb_json_get_ex(obj, "duration", &duration))
        media->duration = tb_json_get_int(duration);

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_PAYMENTS
static telebot_error_e telebot_parser_get_paid_media_info(struct cJSON *obj, telebot_paid_media_info_t *info)
{
    if ((obj == NULL) || (info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(info, 0, sizeof(telebot_paid_media_info_t));
    struct cJSON *star_count = NULL;
    if (tb_json_get_ex(obj, "star_count", &star_count))
        info->star_count = tb_json_get_int(star_count);

    struct cJSON *paid_media = NULL;
    if (tb_json_get_ex(obj, "paid_media", &paid_media))
    {
        int array_len = tb_json_array_length(paid_media);
        info->count_paid_media = array_len;
        info->paid_media = calloc(array_len, sizeof(telebot_paid_media_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_paid_media(tb_json_array_get_idx(paid_media, i), &(info->paid_media[i]));
    }

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_PAYMENTS
static telebot_error_e telebot_parser_get_refunded_payment(struct cJSON *obj, telebot_refunded_payment_t *payment)
{
    if ((obj == NULL) || (payment == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(payment, 0, sizeof(telebot_refunded_payment_t));
    struct cJSON *currency = NULL;
    if (tb_json_get_ex(obj, "currency", &currency))
        payment->currency = TELEBOT_SAFE_STRDUP(tb_json_get_string(currency));

    struct cJSON *total_amount = NULL;
    if (tb_json_get_ex(obj, "total_amount", &total_amount))
        payment->total_amount = tb_json_get_int(total_amount);

    struct cJSON *invoice_payload = NULL;
    if (tb_json_get_ex(obj, "invoice_payload", &invoice_payload))
        payment->invoice_payload = TELEBOT_SAFE_STRDUP(tb_json_get_string(invoice_payload));

    struct cJSON *telegram_payment_charge_id = NULL;
    if (tb_json_get_ex(obj, "telegram_payment_charge_id", &telegram_payment_charge_id))
        payment->telegram_payment_charge_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(telegram_payment_charge_id));

    struct cJSON *provider_payment_charge_id = NULL;
    if (tb_json_get_ex(obj, "provider_payment_charge_id", &provider_payment_charge_id))
        payment->provider_payment_charge_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(provider_payment_charge_id));

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
static telebot_error_e telebot_parser_get_write_access_allowed(struct cJSON *obj, telebot_write_access_allowed_t *allowed)
{
    if ((obj == NULL) || (allowed == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(allowed, 0, sizeof(telebot_write_access_allowed_t));
    struct cJSON *web_app_name = NULL;
    if (tb_json_get_ex(obj, "web_app_name", &web_app_name))
        allowed->web_app_name = TELEBOT_SAFE_STRDUP(tb_json_get_string(web_app_name));

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
static telebot_error_e telebot_parser_get_shared_user(struct cJSON *obj, telebot_shared_user_t *user)
{
    if ((obj == NULL) || (user == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(user, 0, sizeof(telebot_shared_user_t));
    struct cJSON *user_id = NULL;
    if (tb_json_get_ex(obj, "user_id", &user_id))
        user->user_id = tb_json_get_int(user_id);

    struct cJSON *first_name = NULL;
    if (tb_json_get_ex(obj, "first_name", &first_name))
        user->first_name = TELEBOT_SAFE_STRDUP(tb_json_get_string(first_name));

    struct cJSON *last_name = NULL;
    if (tb_json_get_ex(obj, "last_name", &last_name))
        user->last_name = TELEBOT_SAFE_STRDUP(tb_json_get_string(last_name));

    struct cJSON *username = NULL;
    if (tb_json_get_ex(obj, "username", &username))
        user->username = TELEBOT_SAFE_STRDUP(tb_json_get_string(username));

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
static telebot_error_e telebot_parser_get_users_shared(struct cJSON *obj, telebot_users_shared_t *shared)
{
    if ((obj == NULL) || (shared == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(shared, 0, sizeof(telebot_users_shared_t));
    struct cJSON *request_id = NULL;
    if (tb_json_get_ex(obj, "request_id", &request_id))
        shared->request_id = tb_json_get_int(request_id);

    struct cJSON *users = NULL;
    if (tb_json_get_ex(obj, "users", &users))
    {
        int array_len = tb_json_array_length(users);
        shared->count_users = array_len;
        shared->users = calloc(array_len, sizeof(telebot_shared_user_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_shared_user(tb_json_array_get_idx(users, i), &(shared->users[i]));
    }

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
static telebot_error_e telebot_parser_get_chat_shared(struct cJSON *obj, telebot_chat_shared_t *shared)
{
    if ((obj == NULL) || (shared == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(shared, 0, sizeof(telebot_chat_shared_t));
    struct cJSON *request_id = NULL;
    if (tb_json_get_ex(obj, "request_id", &request_id))
        shared->request_id = tb_json_get_int(request_id);

    struct cJSON *chat_id = NULL;
    if (tb_json_get_ex(obj, "chat_id", &chat_id))
        shared->chat_id = tb_json_get_int(chat_id);

    return TELEBOT_ERROR_NONE;
}
#endif

static telebot_error_e telebot_parser_get_gift(struct cJSON *obj, telebot_gift_t *gift)
{
    if ((obj == NULL) || (gift == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gift, 0, sizeof(telebot_gift_t));
    struct cJSON *id = NULL;
    if (tb_json_get_ex(obj, "id", &id))
        gift->id = TELEBOT_SAFE_STRDUP(tb_json_get_string(id));

    struct cJSON *star_count = NULL;
    if (tb_json_get_ex(obj, "star_count", &star_count))
        gift->star_count = tb_json_get_int(star_count);

    return TELEBOT_ERROR_NONE;
}

#if TELEBOT_FEATURE_GIFTS
static telebot_error_e telebot_parser_get_gift_info(struct cJSON *obj, telebot_gift_info_t *gift_info)
{
    if ((obj == NULL) || (gift_info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gift_info, 0, sizeof(telebot_gift_info_t));
    struct cJSON *gift = NULL;
    if (tb_json_get_ex(obj, "gift", &gift))
        telebot_parser_get_gift(gift, &(gift_info->gift));

    struct cJSON *text = NULL;
    if (tb_json_get_ex(obj, "text", &text))
        gift_info->text = TELEBOT_SAFE_STRDUP(tb_json_get_string(text));

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_GIFTS
static telebot_error_e telebot_parser_get_unique_gift(struct cJSON *obj, telebot_unique_gift_t *gift)
{
    if ((obj == NULL) || (gift == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gift, 0, sizeof(telebot_unique_gift_t));
    struct cJSON *gift_id = NULL;
    if (tb_json_get_ex(obj, "gift_id", &gift_id))
        gift->gift_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(gift_id));

    struct cJSON *name = NULL;
    if (tb_json_get_ex(obj, "name", &name))
        gift->name = TELEBOT_SAFE_STRDUP(tb_json_get_string(name));

    struct cJSON *number = NULL;
    if (tb_json_get_ex(obj, "number", &number))
        gift->number = tb_json_get_int(number);

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_GIFTS
static telebot_error_e telebot_parser_get_unique_gift_info(struct cJSON *obj, telebot_unique_gift_info_t *gift_info)
{
    if ((obj == NULL) || (gift_info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gift_info, 0, sizeof(telebot_unique_gift_info_t));
    struct cJSON *gift = NULL;
    if (tb_json_get_ex(obj, "gift", &gift))
    {
        gift_info->gift = calloc(1, sizeof(telebot_unique_gift_t));
        telebot_parser_get_unique_gift(gift, gift_info->gift);
    }

    struct cJSON *origin = NULL;
    if (tb_json_get_ex(obj, "origin", &origin))
        gift_info->origin = TELEBOT_SAFE_STRDUP(tb_json_get_string(origin));

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
static telebot_error_e telebot_parser_get_chat_boost_added(struct cJSON *obj, telebot_chat_boost_added_t *boost)
{
    if ((obj == NULL) || (boost == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(boost, 0, sizeof(telebot_chat_boost_added_t));
    struct cJSON *boost_count = NULL;
    if (tb_json_get_ex(obj, "boost_count", &boost_count))
        boost->boost_count = tb_json_get_int(boost_count);

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
static telebot_error_e telebot_parser_get_chat_background(struct cJSON *obj, telebot_chat_background_t *background)
{
    if ((obj == NULL) || (background == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(background, 0, sizeof(telebot_chat_background_t));
    struct cJSON *type = NULL;
    if (tb_json_get_ex(obj, "type", &type))
    {
        struct cJSON *type_str = NULL;
        if (tb_json_get_ex(type, "type", &type_str))
            background->type = TELEBOT_SAFE_STRDUP(tb_json_get_string(type_str));
    }

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_GIFTS
static telebot_error_e telebot_parser_get_giveaway_created(struct cJSON *obj, telebot_giveaway_created_t *giveaway)
{
    if ((obj == NULL) || (giveaway == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(giveaway, 0, sizeof(telebot_giveaway_created_t));
    struct cJSON *prize_star_count = NULL;
    if (tb_json_get_ex(obj, "prize_star_count", &prize_star_count))
        giveaway->prize_star_count = tb_json_get_int(prize_star_count);

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_GIFTS
static telebot_error_e telebot_parser_get_giveaway_completed(struct cJSON *obj, telebot_giveaway_completed_t *giveaway)
{
    if ((obj == NULL) || (giveaway == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(giveaway, 0, sizeof(telebot_giveaway_completed_t));
    struct cJSON *winner_count = NULL;
    if (tb_json_get_ex(obj, "winner_count", &winner_count))
        giveaway->winner_count = tb_json_get_int(winner_count);

    struct cJSON *unclaimed_prize_count = NULL;
    if (tb_json_get_ex(obj, "unclaimed_prize_count", &unclaimed_prize_count))
        giveaway->unclaimed_prize_count = tb_json_get_int(unclaimed_prize_count);

    struct cJSON *giveaway_message = NULL;
    if (tb_json_get_ex(obj, "giveaway_message", &giveaway_message))
    {
        giveaway->giveaway_message = calloc(1, sizeof(telebot_message_t));
        telebot_parser_get_message(giveaway_message, giveaway->giveaway_message);
    }

    struct cJSON *is_star_giveaway = NULL;
    if (tb_json_get_ex(obj, "is_star_giveaway", &is_star_giveaway))
        giveaway->is_star_giveaway = tb_json_get_bool(is_star_giveaway);

    return TELEBOT_ERROR_NONE;
}
#endif

static telebot_error_e telebot_parser_get_birthdate(struct cJSON *obj, telebot_birthdate_t *birthdate)
{
    if ((obj == NULL) || (birthdate == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(birthdate, 0, sizeof(telebot_birthdate_t));
    struct cJSON *tmp = NULL;
    if (tb_json_get_ex(obj, "day", &tmp))
        birthdate->day = tb_json_get_int(tmp);
    if (tb_json_get_ex(obj, "month", &tmp))
        birthdate->month = tb_json_get_int(tmp);
    if (tb_json_get_ex(obj, "year", &tmp))
        birthdate->year = tb_json_get_int(tmp);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_business_intro(struct cJSON *obj, telebot_business_intro_t *intro)
{
    if ((obj == NULL) || (intro == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(intro, 0, sizeof(telebot_business_intro_t));
    struct cJSON *tmp = NULL;
    if (tb_json_get_ex(obj, "title", &tmp))
        intro->title = TELEBOT_SAFE_STRDUP(tb_json_get_string(tmp));
    if (tb_json_get_ex(obj, "message", &tmp))
        intro->message = TELEBOT_SAFE_STRDUP(tb_json_get_string(tmp));

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_business_location(struct cJSON *obj, telebot_business_location_t *location)
{
    if ((obj == NULL) || (location == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(location, 0, sizeof(telebot_business_location_t));
    struct cJSON *address = NULL;
    if (tb_json_get_ex(obj, "address", &address))
        location->address = TELEBOT_SAFE_STRDUP(tb_json_get_string(address));

    struct cJSON *loc = NULL;
    if (tb_json_get_ex(obj, "location", &loc))
    {
        location->location = calloc(1, sizeof(telebot_location_t));
        telebot_parser_get_location(loc, location->location);
    }

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_business_opening_hours(struct cJSON *obj, telebot_business_opening_hours_t *hours)
{
    if ((obj == NULL) || (hours == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(hours, 0, sizeof(telebot_business_opening_hours_t));
    struct cJSON *time_zone_name = NULL;
    if (tb_json_get_ex(obj, "time_zone_name", &time_zone_name))
        hours->time_zone_name = TELEBOT_SAFE_STRDUP(tb_json_get_string(time_zone_name));

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_star_transaction(struct cJSON *obj, telebot_star_transaction_t *transaction)
{
    if ((obj == NULL) || (transaction == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(transaction, 0, sizeof(telebot_star_transaction_t));
    struct cJSON *id = NULL;
    if (tb_json_get_ex(obj, "id", &id))
        transaction->id = TELEBOT_SAFE_STRDUP(tb_json_get_string(id));

    struct cJSON *amount = NULL;
    if (tb_json_get_ex(obj, "amount", &amount))
        transaction->amount = tb_json_get_int(amount);

    struct cJSON *date = NULL;
    if (tb_json_get_ex(obj, "date", &date))
        transaction->date = tb_json_get_int(date);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_star_transactions(struct cJSON *obj, telebot_star_transactions_t *transactions)
{
    if ((obj == NULL) || (transactions == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(transactions, 0, sizeof(telebot_star_transactions_t));
    struct cJSON *array = NULL;
    if (tb_json_get_ex(obj, "transactions", &array))
    {
        int array_len = tb_json_array_length(array);
        transactions->count_transactions = array_len;
        transactions->transactions = calloc(array_len, sizeof(telebot_star_transaction_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_star_transaction(tb_json_array_get_idx(array, i), &(transactions->transactions[i]));
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_prepared_inline_message(struct cJSON *obj, telebot_prepared_inline_message_t *prepared_message)
{
    if ((obj == NULL) || (prepared_message == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(prepared_message, 0, sizeof(telebot_prepared_inline_message_t));
    struct cJSON *id = NULL;
    if (tb_json_get_ex(obj, "id", &id))
        prepared_message->id = TELEBOT_SAFE_STRDUP(tb_json_get_string(id));

    struct cJSON *expiration_date = NULL;
    if (tb_json_get_ex(obj, "expiration_date", &expiration_date))
        prepared_message->expiration_date = tb_json_get_int(expiration_date);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_game_high_score(struct cJSON *obj, telebot_game_high_score_t *high_score)
{
    if ((obj == NULL) || (high_score == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(high_score, 0, sizeof(telebot_game_high_score_t));
    struct cJSON *position = NULL;
    if (tb_json_get_ex(obj, "position", &position))
        high_score->position = tb_json_get_int(position);

    struct cJSON *user = NULL;
    if (tb_json_get_ex(obj, "user", &user))
    {
        high_score->user = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(user, high_score->user);
    }

    struct cJSON *score = NULL;
    if (tb_json_get_ex(obj, "score", &score))
        high_score->score = tb_json_get_int(score);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_paid_media_purchased(struct cJSON *obj, telebot_paid_media_purchased_t *purchased)
{
    if ((obj == NULL) || (purchased == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(purchased, 0, sizeof(telebot_paid_media_purchased_t));
    struct cJSON *from = NULL;
    if (tb_json_get_ex(obj, "from", &from))
    {
        purchased->from = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(from, purchased->from);
    }

    struct cJSON *paid_media_payload = NULL;
    if (tb_json_get_ex(obj, "paid_media_payload", &paid_media_payload))
        purchased->paid_media_payload = TELEBOT_SAFE_STRDUP(tb_json_get_string(paid_media_payload));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_gifts(struct cJSON *obj, telebot_gifts_t *gifts)
{
    if ((obj == NULL) || (gifts == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gifts, 0, sizeof(telebot_gifts_t));

    struct cJSON *result = NULL;
    if (!tb_json_get_ex(obj, "result", &result))
        result = obj;

    if (!cJSON_IsArray(result))
        return TELEBOT_ERROR_OPERATION_FAILED;

    int array_len = tb_json_array_length(result);
    if (array_len > 0)
    {
        gifts->gifts = calloc(array_len, sizeof(telebot_gift_t));
        if (gifts->gifts == NULL)
            return TELEBOT_ERROR_OUT_OF_MEMORY;

        gifts->count = array_len;
        for (int i = 0; i < array_len; i++)
        {
            struct cJSON *item = tb_json_array_get_idx(result, i);
            telebot_parser_get_gift(item, &(gifts->gifts[i]));
        }
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user_gift(struct cJSON *obj, telebot_user_gift_t *gift)
{
    if ((obj == NULL) || (gift == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gift, 0, sizeof(telebot_user_gift_t));

    struct cJSON *gift_id = NULL;
    if (tb_json_get_ex(obj, "gift_id", &gift_id))
        gift->gift_id = TELEBOT_SAFE_STRDUP(tb_json_get_string(gift_id));

    struct cJSON *sender_user = NULL;
    if (tb_json_get_ex(obj, "sender_user", &sender_user))
    {
        gift->sender_user = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(sender_user, gift->sender_user);
    }

    struct cJSON *text = NULL;
    if (tb_json_get_ex(obj, "text", &text))
        gift->text = TELEBOT_SAFE_STRDUP(tb_json_get_string(text));

    struct cJSON *entities = NULL;
    if (tb_json_get_ex(obj, "entities", &entities))
        telebot_parser_get_message_entities(entities, &(gift->entities), &(gift->count_entities));

    struct cJSON *gift_obj = NULL;
    if (tb_json_get_ex(obj, "gift", &gift_obj))
    {
        gift->gift = calloc(1, sizeof(telebot_gift_t));
        telebot_parser_get_gift(gift_obj, gift->gift);
    }

    struct cJSON *date = NULL;
    if (tb_json_get_ex(obj, "date", &date))
        gift->date = tb_json_get_int(date);

    struct cJSON *message_id = NULL;
    if (tb_json_get_ex(obj, "message_id", &message_id))
        gift->message_id = tb_json_get_int(message_id);

    struct cJSON *upgrade_star_count = NULL;
    if (tb_json_get_ex(obj, "upgrade_star_count", &upgrade_star_count))
        gift->upgrade_star_count = tb_json_get_int(upgrade_star_count);

    struct cJSON *is_upgraded = NULL;
    if (tb_json_get_ex(obj, "is_upgraded", &is_upgraded))
        gift->is_upgraded = tb_json_get_bool(is_upgraded);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user_gifts(struct cJSON *obj, telebot_user_gifts_t *gifts)
{
    if ((obj == NULL) || (gifts == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gifts, 0, sizeof(telebot_user_gifts_t));

    struct cJSON *result = NULL;
    if (!tb_json_get_ex(obj, "result", &result))
        result = obj;

    struct cJSON *total_count = NULL;
    if (tb_json_get_ex(result, "total_count", &total_count))
        gifts->total_count = tb_json_get_int(total_count);

    struct cJSON *gifts_array = NULL;
    if (tb_json_get_ex(result, "gifts", &gifts_array))
    {
        int array_len = tb_json_array_length(gifts_array);
        if (array_len > 0)
        {
            gifts->gifts = calloc(array_len, sizeof(telebot_user_gift_t));
            if (gifts->gifts == NULL)
                return TELEBOT_ERROR_OUT_OF_MEMORY;

            gifts->count = array_len;
            for (int i = 0; i < array_len; i++)
            {
                struct cJSON *item = tb_json_array_get_idx(gifts_array, i);
                telebot_parser_get_user_gift(item, &(gifts->gifts[i]));
            }
        }
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user_profile_audios(struct cJSON *obj, telebot_user_profile_audios_t *audios)
{
    if ((obj == NULL) || (audios == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(audios, 0, sizeof(telebot_user_profile_audios_t));

    struct cJSON *result = NULL;
    if (!tb_json_get_ex(obj, "result", &result))
        result = obj;

    struct cJSON *total_count = NULL;
    if (tb_json_get_ex(result, "total_count", &total_count))
        audios->total_count = tb_json_get_int(total_count);

    struct cJSON *audios_array = NULL;
    if (tb_json_get_ex(result, "audios", &audios_array))
    {
        int array_len = tb_json_array_length(audios_array);
        if (array_len > 0)
        {
            audios->audios = calloc(array_len, sizeof(telebot_audio_t));
            if (audios->audios == NULL)
                return TELEBOT_ERROR_OUT_OF_MEMORY;

            audios->count = array_len;
            for (int i = 0; i < array_len; i++)
            {
                struct cJSON *item = tb_json_array_get_idx(audios_array, i);
                telebot_parser_get_audio(item, &(audios->audios[i]));
            }
        }
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_business_connection(struct cJSON *obj, telebot_business_connection_t *connection)
{
    if ((obj == NULL) || (connection == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(connection, 0, sizeof(telebot_business_connection_t));

    struct cJSON *result = NULL;
    if (!tb_json_get_ex(obj, "result", &result))
        result = obj;

    struct cJSON *id = NULL;
    if (tb_json_get_ex(result, "id", &id))
        connection->id = TELEBOT_SAFE_STRDUP(tb_json_get_string(id));

    struct cJSON *user = NULL;
    if (tb_json_get_ex(result, "user", &user))
    {
        connection->user = calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(user, connection->user);
    }

    struct cJSON *user_chat_id = NULL;
    if (tb_json_get_ex(result, "user_chat_id", &user_chat_id))
        connection->user_chat_id = tb_json_get_int(user_chat_id);

    struct cJSON *date = NULL;
    if (tb_json_get_ex(result, "date", &date))
        connection->date = tb_json_get_int(date);

    struct cJSON *rights = NULL;
    if (tb_json_get_ex(result, "rights", &rights))
        telebot_parser_get_business_bot_rights(rights, &connection->rights);

    struct cJSON *is_enabled = NULL;
    if (tb_json_get_ex(result, "is_enabled", &is_enabled))
        connection->is_enabled = tb_json_get_bool(is_enabled);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user_chat_boosts(struct cJSON *obj, telebot_user_chat_boosts_t *boosts)
{
    if ((obj == NULL) || (boosts == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(boosts, 0, sizeof(telebot_user_chat_boosts_t));

    struct cJSON *result = NULL;
    if (!tb_json_get_ex(obj, "result", &result))
        result = obj;

    struct cJSON *boosts_array = NULL;
    if (tb_json_get_ex(result, "boosts", &boosts_array))
    {
        int array_len = tb_json_array_length(boosts_array);
        if (array_len > 0)
        {
            boosts->boosts = calloc(array_len, sizeof(telebot_chat_boost_t));
            if (boosts->boosts == NULL)
                return TELEBOT_ERROR_OUT_OF_MEMORY;

            boosts->count = array_len;
            for (int i = 0; i < array_len; i++)
            {
                struct cJSON *item = tb_json_array_get_idx(boosts_array, i);
                telebot_parser_get_chat_boost(item, &(boosts->boosts[i]));
            }
        }
    }

    return TELEBOT_ERROR_NONE;
}


