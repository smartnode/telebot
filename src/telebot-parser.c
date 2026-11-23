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
#include "telebot-limits.h"
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
    [TELEBOT_UPDATE_TYPE_MANAGED_BOT] = "managed_bot",
    [TELEBOT_UPDATE_TYPE_SUBSCRIPTION] = "subscription",
    [TELEBOT_UPDATE_TYPE_WIDGET_QUERY] = "widget_query",
    [TELEBOT_UPDATE_TYPE_SHARED_WIDGET_QUERY] = "shared_widget_query",
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
static void telebot_parser_get_managed_bot_updated(struct cJSON *obj, telebot_managed_bot_updated_t *updated);
#if TELEBOT_FEATURE_PAYMENTS
static void telebot_parser_get_bot_subscription_updated(struct cJSON *obj, telebot_bot_subscription_updated_t *updated);
#endif
#if TELEBOT_FEATURE_WIDGETS
static void telebot_parser_get_widget_query(struct cJSON *obj, telebot_widget_query_t *query);
#endif
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
    int array_len = telebot_json_array_length(array);
    if (array_len == 0)
        return TELEBOT_ERROR_NONE;

    telebot_update_t *result = telebot_owned_calloc(array_len, sizeof(telebot_update_t));
    if (result == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    *count = array_len;
    *updates = result;

    for (int index = 0; index < array_len; index++)
    {
        struct cJSON *item = telebot_json_array_get_idx(array, index);
        result[index].update_type = TELEBOT_UPDATE_TYPE_UNKNOWN; /* until a known payload is found */

        struct cJSON *update_id = NULL;
        if (telebot_json_get_ex(item, "update_id", &update_id))
        {
            result[index].update_id = telebot_json_get_int(update_id);
        }

        struct cJSON *message = NULL;
        if (telebot_json_get_ex(item, "message", &message))
        {
            if (telebot_parser_get_message(message, &(result[index].message)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse message of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_MESSAGE;
            continue;
        }

        struct cJSON *edited_message = NULL;
        if (telebot_json_get_ex(item, "edited_message", &edited_message))
        {
            if (telebot_parser_get_message(edited_message, &(result[index].edited_message)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse edited message of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_EDITED_MESSAGE;
            continue;
        }

        struct cJSON *channel_post = NULL;
        if (telebot_json_get_ex(item, "channel_post", &channel_post))
        {
            if (telebot_parser_get_message(channel_post, &(result[index].channel_post)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse channel post of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CHANNEL_POST;
            continue;
        }

        struct cJSON *edited_channel_post = NULL;
        if (telebot_json_get_ex(item, "edited_channel_post", &edited_channel_post))
        {
            if (telebot_parser_get_message(edited_channel_post, &(result[index].edited_channel_post)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse edited channel post of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_EDITED_CHANNEL_POST;
            continue;
        }

#if TELEBOT_FEATURE_BUSINESS
        struct cJSON *business_connection = NULL;
        if (telebot_json_get_ex(item, "business_connection", &business_connection))
        {
            if (telebot_parser_get_business_connection(business_connection, &(result[index].business_connection)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse business_connection of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_BUSINESS_CONNECTION;
            continue;
        }
#endif

#if TELEBOT_FEATURE_BUSINESS
        struct cJSON *business_message = NULL;
        if (telebot_json_get_ex(item, "business_message", &business_message))
        {
            if (telebot_parser_get_message(business_message, &(result[index].business_message)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse business_message of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_BUSINESS_MESSAGE;
            continue;
        }
#endif

#if TELEBOT_FEATURE_BUSINESS
        struct cJSON *edited_business_message = NULL;
        if (telebot_json_get_ex(item, "edited_business_message", &edited_business_message))
        {
            if (telebot_parser_get_message(edited_business_message, &(result[index].edited_business_message)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse edited_business_message of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_EDITED_BUSINESS_MESSAGE;
            continue;
        }
#endif

#if TELEBOT_FEATURE_BUSINESS
        struct cJSON *deleted_business_messages = NULL;
        if (telebot_json_get_ex(item, "deleted_business_messages", &deleted_business_messages))
        {
            if (telebot_parser_get_business_messages_deleted(deleted_business_messages, &(result[index].deleted_business_messages)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse deleted_business_messages of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_DELETED_BUSINESS_MESSAGES;
            continue;
        }
#endif

        struct cJSON *guest_message = NULL;
        if (telebot_json_get_ex(item, "guest_message", &guest_message))
        {
            if (telebot_parser_get_message(guest_message, &(result[index].guest_message)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse guest_message of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_GUEST_MESSAGE;
            continue;
        }

        struct cJSON *stopped = NULL;
        if (telebot_json_get_ex(item, "stopped_message_generation", &stopped))
        {
            telebot_message_generation_stopped_t *gen = &(result[index].stopped_message_generation);
            cJSON *tmp = NULL;
            if (telebot_json_get_ex(stopped, "chat", &tmp))
            {
                gen->chat = telebot_mem_calloc(1, sizeof(telebot_chat_t));
                if ((gen->chat != NULL) && (telebot_parser_get_chat(tmp, gen->chat) != TELEBOT_ERROR_NONE))
                    TELEBOT_SAFE_FREE(gen->chat);
            }
            if (telebot_json_get_ex(stopped, "message_thread_id", &tmp))
                gen->message_thread_id = telebot_json_get_int(tmp);
            if (telebot_json_get_ex(stopped, "draft_id", &tmp))
                gen->draft_id = telebot_json_get_int(tmp);
            result[index].update_type = TELEBOT_UPDATE_TYPE_STOPPED_MESSAGE_GENERATION;
            continue;
        }

        struct cJSON *managed_bot = NULL;
        if (telebot_json_get_ex(item, "managed_bot", &managed_bot))
        {
            telebot_parser_get_managed_bot_updated(managed_bot, &(result[index].managed_bot));
            result[index].update_type = TELEBOT_UPDATE_TYPE_MANAGED_BOT;
            continue;
        }

#if TELEBOT_FEATURE_PAYMENTS
        struct cJSON *subscription = NULL;
        if (telebot_json_get_ex(item, "subscription", &subscription))
        {
            telebot_parser_get_bot_subscription_updated(subscription, &(result[index].subscription));
            result[index].update_type = TELEBOT_UPDATE_TYPE_SUBSCRIPTION;
            continue;
        }
#endif

#if TELEBOT_FEATURE_WIDGETS
        struct cJSON *widget_query = NULL;
        if (telebot_json_get_ex(item, "widget_query", &widget_query))
        {
            telebot_parser_get_widget_query(widget_query, &(result[index].widget_query));
            result[index].update_type = TELEBOT_UPDATE_TYPE_WIDGET_QUERY;
            continue;
        }

        struct cJSON *shared_widget_query = NULL;
        if (telebot_json_get_ex(item, "shared_widget_query", &shared_widget_query))
        {
            telebot_parser_get_widget_query(shared_widget_query, &(result[index].shared_widget_query));
            result[index].update_type = TELEBOT_UPDATE_TYPE_SHARED_WIDGET_QUERY;
            continue;
        }
#endif

        struct cJSON *callback_query = NULL;
        if (telebot_json_get_ex(item, "callback_query", &callback_query))
        {
            if (telebot_parser_get_callback_query(callback_query, &(result[index].callback_query)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse callback query of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CALLBACK_QUERY;
            continue;
        }

        struct cJSON *poll = NULL;
        if (telebot_json_get_ex(item, "poll", &poll))
        {
            if (telebot_parser_get_poll(poll, &(result[index].poll)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse poll of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_POLL;
            continue;
        }

        struct cJSON *poll_answer = NULL;
        if (telebot_json_get_ex(item, "poll_answer", &poll_answer))
        {
            if (telebot_parser_get_poll_answer(poll_answer, &(result[index].poll_answer)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse poll answer of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_POLL_ANSWER;
            continue;
        }

        struct cJSON *my_chat_member = NULL;
        if (telebot_json_get_ex(item, "my_chat_member", &my_chat_member))
        {
            if (telebot_parser_get_chat_member_updated(my_chat_member, &(result[index].my_chat_member)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse my_chat_member of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_MY_CHAT_MEMBER;
            continue;
        }

        struct cJSON *chat_member = NULL;
        if (telebot_json_get_ex(item, "chat_member", &chat_member))
        {
            if (telebot_parser_get_chat_member_updated(chat_member, &(result[index].chat_member)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse chat_member of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CHAT_MEMBER;
            continue;
        }

        struct cJSON *chat_join_request = NULL;
        if (telebot_json_get_ex(item, "chat_join_request", &chat_join_request))
        {
            if (telebot_parser_get_chat_join_request(chat_join_request, &(result[index].chat_join_request)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse chat_join_request of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CHAT_JOIN_REQUEST;
            continue;
        }

#if TELEBOT_FEATURE_CHAT_EXTRAS
        struct cJSON *message_reaction = NULL;
        if (telebot_json_get_ex(item, "message_reaction", &message_reaction))
        {
            if (telebot_parser_get_message_reaction_updated(message_reaction, &(result[index].message_reaction)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse message_reaction of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_MESSAGE_REACTION;
            continue;
        }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
        struct cJSON *message_reaction_count = NULL;
        if (telebot_json_get_ex(item, "message_reaction_count", &message_reaction_count))
        {
            if (telebot_parser_get_message_reaction_count_updated(message_reaction_count, &(result[index].message_reaction_count)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse message_reaction_count of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_MESSAGE_REACTION_COUNT;
            continue;
        }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
        struct cJSON *chat_boost = NULL;
        if (telebot_json_get_ex(item, "chat_boost", &chat_boost))
        {
            if (telebot_parser_get_chat_boost_updated(chat_boost, &(result[index].chat_boost)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse chat_boost of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CHAT_BOOST;
            continue;
        }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
        struct cJSON *chat_boost_removed = NULL;
        if (telebot_json_get_ex(item, "removed_chat_boost", &chat_boost_removed))
        {
            if (telebot_parser_get_chat_boost_removed(chat_boost_removed, &(result[index].chat_boost_removed)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse chat_boost_removed of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_REMOVED_CHAT_BOOST;
            continue;
        }
#endif

        struct cJSON *inline_query = NULL;
        if (telebot_json_get_ex(item, "inline_query", &inline_query))
        {
            if (telebot_parser_get_inline_query(inline_query, &(result[index].inline_query)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse inline_query of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_INLINE_QUERY;
            continue;
        }

        struct cJSON *chosen_inline_result = NULL;
        if (telebot_json_get_ex(item, "chosen_inline_result", &chosen_inline_result))
        {
            if (telebot_parser_get_chosen_inline_result(chosen_inline_result, &(result[index].chosen_inline_result)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse chosen_inline_result of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_CHOSEN_INLINE_RESULT;
            continue;
        }

#if TELEBOT_FEATURE_PAYMENTS
        struct cJSON *shipping_query = NULL;
        if (telebot_json_get_ex(item, "shipping_query", &shipping_query))
        {
            if (telebot_parser_get_shipping_query(shipping_query, &(result[index].shipping_query)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse shipping_query of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_SHIPPING_QUERY;
            continue;
        }
#endif

#if TELEBOT_FEATURE_PAYMENTS
        struct cJSON *pre_checkout_query = NULL;
        if (telebot_json_get_ex(item, "pre_checkout_query", &pre_checkout_query))
        {
            if (telebot_parser_get_pre_checkout_query(pre_checkout_query, &(result[index].pre_checkout_query)) != TELEBOT_ERROR_NONE)
                ERR("Failed to parse pre_checkout_query of bot update");
            result[index].update_type = TELEBOT_UPDATE_TYPE_PRE_CHECKOUT_QUERY;
            continue;
        }
#endif

#if TELEBOT_FEATURE_PAYMENTS
        struct cJSON *purchased_paid_media = NULL;
        if (telebot_json_get_ex(item, "purchased_paid_media", &purchased_paid_media))
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

/* Nested object types parsed by table (index into telebot_object_types) */
enum telebot_object_type_index
{
    TELEBOT_OBJECT_ANIMATION,
    TELEBOT_OBJECT_AUDIO,
    TELEBOT_OBJECT_BIRTHDATE,
    TELEBOT_OBJECT_BUSINESS_INTRO,
    TELEBOT_OBJECT_BUSINESS_LOCATION,
    TELEBOT_OBJECT_BUSINESS_OPENING_HOURS,
    TELEBOT_OBJECT_CHAT,
    TELEBOT_OBJECT_CHAT_BOOST,
    TELEBOT_OBJECT_CHAT_BOOST_SOURCE,
    TELEBOT_OBJECT_CHAT_INVITE_LINK,
    TELEBOT_OBJECT_CHAT_LOCATION,
    TELEBOT_OBJECT_CHAT_MEMBER,
    TELEBOT_OBJECT_CHAT_PERMISSIONS,
    TELEBOT_OBJECT_CHAT_PHOTO,
    TELEBOT_OBJECT_CONTACT,
    TELEBOT_OBJECT_DICE,
    TELEBOT_OBJECT_DOCUMENT,
    TELEBOT_OBJECT_ENCRYPTED_CREDENTIALS,
    TELEBOT_OBJECT_EXTERNAL_REPLY_INFO,
    TELEBOT_OBJECT_GIFT,
    TELEBOT_OBJECT_INLINE_KEYBOARD_MARKUP,
    TELEBOT_OBJECT_LINK_PREVIEW_OPTIONS,
    TELEBOT_OBJECT_LOCATION,
    TELEBOT_OBJECT_MESSAGE,
    TELEBOT_OBJECT_MESSAGE_ORIGIN,
    TELEBOT_OBJECT_ORDER_INFO,
    TELEBOT_OBJECT_PASSPORT_FILE,
    TELEBOT_OBJECT_PHOTO,
    TELEBOT_OBJECT_POLL,
    TELEBOT_OBJECT_SHIPPING_ADDRESS,
    TELEBOT_OBJECT_STORY,
    TELEBOT_OBJECT_TEXT_QUOTE,
    TELEBOT_OBJECT_USER,
    TELEBOT_OBJECT_VENUE,
    TELEBOT_OBJECT_VIDEO,
    TELEBOT_OBJECT_VIDEO_NOTE,
    TELEBOT_OBJECT_VOICE,
    TELEBOT_OBJECT_TYPE_COUNT
};

static const telebot_json_field_t telebot_parser_get_webhook_info_fields[] = {
    TELEBOT_JSON_FIELD("last_error_message", telebot_webhook_info_t, last_error_message, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("max_connections", telebot_webhook_info_t, max_connections, TELEBOT_FIELD_INT),
};

telebot_error_e telebot_parser_get_webhook_info(struct cJSON *obj, telebot_webhook_info_t *info)
{
    if ((obj == NULL) || (info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(info, 0, sizeof(telebot_webhook_info_t));
    struct cJSON *url = NULL;
    if (!telebot_json_get_ex(obj, "url", &url))
    {
        ERR("Object is not json webhook info type, url not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    info->url = TELEBOT_SAFE_STRDUP(telebot_json_get_string(url));

    struct cJSON *certificate = NULL;
    if (!telebot_json_get_ex(obj, "has_custom_certificate", &certificate))
    {
        ERR("Object is not webhook info type, has_custom_certificate not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    info->has_custom_certificate = telebot_json_get_bool(certificate);

    struct cJSON *update_count = NULL;
    if (!telebot_json_get_ex(obj, "pending_update_count", &update_count))
    {
        ERR("Object is not webook info type, pending_update_count not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    info->pending_update_count = telebot_json_get_int(update_count);

    struct cJSON *last_error_date = NULL;
    if (telebot_json_get_ex(obj, "last_error_date", &last_error_date))
    {
        info->last_error_date = telebot_json_get_double(last_error_date);
    }

    telebot_parse_fields(obj, info, telebot_parser_get_webhook_info_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_webhook_info_fields));

    struct cJSON *allowed_updates = NULL;
    if (telebot_json_get_ex(obj, "allowed_updates", &allowed_updates))
    {
        int cnt = 0;
        int array_len = telebot_json_array_length(allowed_updates);
        for (int i = 0; i < array_len; i++)
        {
            int type = telebot_update_type_from_name(telebot_json_get_string(telebot_json_array_get_idx(allowed_updates, i)));
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

static const telebot_json_field_t telebot_parser_get_user_fields[] = {
    TELEBOT_JSON_FIELD("last_name", telebot_user_t, last_name, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("username", telebot_user_t, username, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("language_code", telebot_user_t, language_code, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("is_premium", telebot_user_t, is_premium, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("added_to_attachment_menu", telebot_user_t, added_to_attachment_menu, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("can_join_groups", telebot_user_t, can_join_groups, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("can_read_all_group_messages", telebot_user_t, can_read_all_group_messages, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("supports_inline_queries", telebot_user_t, supports_inline_queries, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("can_connect_to_business", telebot_user_t, can_connect_to_business, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("has_main_web_app", telebot_user_t, has_main_web_app, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("has_topics_enabled", telebot_user_t, has_topics_enabled, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("allows_users_to_create_topics", telebot_user_t, allows_users_to_create_topics, TELEBOT_FIELD_BOOL),
};

telebot_error_e telebot_parser_get_user(struct cJSON *obj, telebot_user_t *user)
{
    if ((obj == NULL) || (user == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(user, 0, sizeof(telebot_user_t));
    struct cJSON *id;
    if (!telebot_json_get_ex(obj, "id", &id))
    {
        ERR("Object is not json user type, id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    user->id = telebot_json_get_int(id);

    struct cJSON *is_bot = NULL;
    if (!telebot_json_get_ex(obj, "is_bot", &is_bot))
    {
        ERR("Object is not json user type, is_bot not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    user->is_bot = telebot_json_get_bool(is_bot);

    struct cJSON *first_name = NULL;
    if (!telebot_json_get_ex(obj, "first_name", &first_name))
    {
        ERR("Object is not user type, first_name not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    user->first_name = TELEBOT_SAFE_STRDUP(telebot_json_get_string(first_name));

    telebot_parse_fields(obj, user, telebot_parser_get_user_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_user_fields));

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_users(struct cJSON *obj, telebot_user_t **users, int *count)
{
    struct cJSON *array = obj;
    int array_len = telebot_json_array_length(array);
    if (array_len == 0)
        return TELEBOT_ERROR_OPERATION_FAILED;

    telebot_user_t *result = telebot_mem_calloc(array_len, sizeof(telebot_user_t));
    if (result == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    *count = array_len;
    *users = result;

    int index;
    for (index = 0; index < array_len; index++)
    {
        struct cJSON *item = telebot_json_array_get_idx(array, index);
        if (telebot_parser_get_user(item, &(result[index])) != TELEBOT_ERROR_NONE)
            ERR("Failed to parse user from users array");
    }

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_chat_fields[] = {
    TELEBOT_JSON_FIELD("title", telebot_chat_t, title, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("username", telebot_chat_t, username, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("first_name", telebot_chat_t, first_name, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("last_name", telebot_chat_t, last_name, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("is_forum", telebot_chat_t, is_forum, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("is_direct_messages", telebot_chat_t, is_direct_messages, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("accent_color_id", telebot_chat_t, accent_color_id, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("max_reaction_count", telebot_chat_t, max_reaction_count, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("emoji_status_custom_emoji_id", telebot_chat_t, emoji_status_custom_emoji_id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("emoji_status_expiration_date", telebot_chat_t, emoji_status_expiration_date, TELEBOT_FIELD_LONG),
    TELEBOT_JSON_FIELD("bio", telebot_chat_t, bio, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("has_private_forwards", telebot_chat_t, has_private_forwards, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("has_restricted_voice_and_video_messages", telebot_chat_t, has_restricted_voice_and_video_messages, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("join_to_send_messages", telebot_chat_t, join_to_send_messages, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("join_by_request", telebot_chat_t, join_by_request, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("description", telebot_chat_t, description, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("invite_link", telebot_chat_t, invite_link, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("slow_mode_delay", telebot_chat_t, slow_mode_delay, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("unrestrict_boost_count", telebot_chat_t, unrestrict_boost_count, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("has_aggressive_anti_spam_enabled", telebot_chat_t, has_aggressive_anti_spam_enabled, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("has_hidden_members", telebot_chat_t, has_hidden_members, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("has_protected_content", telebot_chat_t, has_protected_content, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("message_auto_delete_time", telebot_chat_t, message_auto_delete_time, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("sticker_set_name", telebot_chat_t, sticker_set_name, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("can_set_sticker_set", telebot_chat_t, can_set_sticker_set, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("custom_emoji_sticker_set_name", telebot_chat_t, custom_emoji_sticker_set_name, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("linked_chat_id", telebot_chat_t, linked_chat_id, TELEBOT_FIELD_LONG_LONG),
};

static const telebot_json_field_t telebot_parser_get_chat_objects[] = {
    TELEBOT_JSON_OBJECT("photo", telebot_chat_t, photo, TELEBOT_OBJECT_CHAT_PHOTO),
    TELEBOT_JSON_OBJECT("birthdate", telebot_chat_t, birthdate, TELEBOT_OBJECT_BIRTHDATE),
    TELEBOT_JSON_OBJECT("business_intro", telebot_chat_t, business_intro, TELEBOT_OBJECT_BUSINESS_INTRO),
    TELEBOT_JSON_OBJECT("business_location", telebot_chat_t, business_location, TELEBOT_OBJECT_BUSINESS_LOCATION),
    TELEBOT_JSON_OBJECT("business_opening_hours", telebot_chat_t, business_opening_hours, TELEBOT_OBJECT_BUSINESS_OPENING_HOURS),
    TELEBOT_JSON_OBJECT("personal_chat", telebot_chat_t, personal_chat, TELEBOT_OBJECT_CHAT),
    TELEBOT_JSON_OBJECT("parent_chat", telebot_chat_t, parent_chat, TELEBOT_OBJECT_CHAT),
    TELEBOT_JSON_OBJECT("permissions", telebot_chat_t, permissions, TELEBOT_OBJECT_CHAT_PERMISSIONS),
    TELEBOT_JSON_OBJECT("location", telebot_chat_t, location, TELEBOT_OBJECT_CHAT_LOCATION),
};

telebot_error_e telebot_parser_get_chat(struct cJSON *obj, telebot_chat_t *chat)
{
    if ((obj == NULL) || (chat == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(chat, 0, sizeof(telebot_chat_t));

    struct cJSON *id;
    if (!telebot_json_get_ex(obj, "id", &id))
    {
        ERR("Object is not chat type, id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    chat->id = telebot_json_get_int(id);

    struct cJSON *type = NULL;
    if (!telebot_json_get_ex(obj, "type", &type))
    {
        ERR("Object is not chat type, type not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    chat->type = TELEBOT_SAFE_STRDUP(telebot_json_get_string(type));

    telebot_parse_fields(obj, chat, telebot_parser_get_chat_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_chat_fields));

    telebot_parse_fields(obj, chat, telebot_parser_get_chat_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_chat_objects));

    struct cJSON *array_active_usernames = NULL;
    if (telebot_json_get_ex(obj, "active_usernames", &array_active_usernames))
    {
        int count_active_usernames = telebot_json_array_length(array_active_usernames);
        if ((chat->count_active_usernames > 0) &&
            (chat->active_usernames = telebot_mem_calloc(count_active_usernames, sizeof(char *))))
        {
            chat->count_active_usernames = count_active_usernames;
            for (int index = 0; index < count_active_usernames; index++)
            {
                struct cJSON *item = telebot_json_array_get_idx(array_active_usernames, index);
                chat->active_usernames[index] = TELEBOT_SAFE_STRDUP(telebot_json_get_string(item));
            }
        }
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_admins(struct cJSON *obj, telebot_chat_member_t **admins, int *count)
{
    int ret = TELEBOT_ERROR_NONE;
    struct cJSON *array = obj;
    int array_len = telebot_json_array_length(array);
    if (!array_len)
        return TELEBOT_ERROR_OPERATION_FAILED;

    telebot_chat_member_t *result = telebot_owned_calloc(array_len, sizeof(telebot_chat_member_t));
    if (result == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    *count = array_len;
    *admins = result;

    int index = 0;
    for (index = 0; index < array_len; index++)
    {
        struct cJSON *item = telebot_json_array_get_idx(array, index);
        ret = telebot_parser_get_chat_member(item, &result[index]);
        if (ret != TELEBOT_ERROR_NONE)
            break;

    } /* for index */

    if (ret)
    {
        *admins = NULL; /* the arena holding `result` is discarded by the caller */
        *count = 0;
    }

    return ret;
}

static const telebot_json_field_t telebot_parser_get_message_fields[] = {
    TELEBOT_JSON_FIELD("message_thread_id", telebot_message_t, message_thread_id, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("sender_boost_count", telebot_message_t, sender_boost_count, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("sender_tag", telebot_message_t, sender_tag, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("business_connection_id", telebot_message_t, business_connection_id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("forward_from_message_id", telebot_message_t, forward_from_message_id, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("forward_signature", telebot_message_t, forward_signature, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("forward_sender_name", telebot_message_t, forward_sender_name, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("forward_date", telebot_message_t, forward_date, TELEBOT_FIELD_LONG),
    TELEBOT_JSON_FIELD("is_topic_message", telebot_message_t, is_topic_message, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("is_automatic_forward", telebot_message_t, is_automatic_forward, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("edit_date", telebot_message_t, edit_date, TELEBOT_FIELD_LONG),
    TELEBOT_JSON_FIELD("has_protected_content", telebot_message_t, has_protected_content, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("is_paid_post", telebot_message_t, is_paid_post, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("media_group_id", telebot_message_t, media_group_id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("author_signature", telebot_message_t, author_signature, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("text", telebot_message_t, text, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("effect_id", telebot_message_t, effect_id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("caption", telebot_message_t, caption, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("show_caption_above_media", telebot_message_t, show_caption_above_media, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("has_media_spoiler", telebot_message_t, has_media_spoiler, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("new_chat_title", telebot_message_t, new_chat_title, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("delete_chat_photo", telebot_message_t, delete_chat_photo, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("group_chat_created", telebot_message_t, group_chat_created, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("supergroup_chat_created", telebot_message_t, supergroup_chat_created, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("channel_chat_created", telebot_message_t, channel_chat_created, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("migrate_to_chat_id", telebot_message_t, migrate_to_chat_id, TELEBOT_FIELD_LONG_LONG),
    TELEBOT_JSON_FIELD("migrate_from_chat_id", telebot_message_t, migrate_from_chat_id, TELEBOT_FIELD_LONG_LONG),
    TELEBOT_JSON_FIELD("connected_website", telebot_message_t, connected_website, TELEBOT_FIELD_STRING),
};

static const telebot_json_field_t telebot_parser_get_message_objects[] = {
    TELEBOT_JSON_OBJECT("forward_origin", telebot_message_t, forward_origin, TELEBOT_OBJECT_MESSAGE_ORIGIN),
    TELEBOT_JSON_OBJECT("from", telebot_message_t, from, TELEBOT_OBJECT_USER),
    TELEBOT_JSON_OBJECT("sender_chat", telebot_message_t, sender_chat, TELEBOT_OBJECT_CHAT),
    TELEBOT_JSON_OBJECT("sender_business_bot", telebot_message_t, sender_business_bot, TELEBOT_OBJECT_USER),
    TELEBOT_JSON_OBJECT("forward_from", telebot_message_t, forward_from, TELEBOT_OBJECT_USER),
    TELEBOT_JSON_OBJECT("forward_from_chat", telebot_message_t, forward_from_chat, TELEBOT_OBJECT_CHAT),
    TELEBOT_JSON_OBJECT("reply_to_message", telebot_message_t, reply_to_message, TELEBOT_OBJECT_MESSAGE),
    TELEBOT_JSON_OBJECT("external_reply", telebot_message_t, external_reply, TELEBOT_OBJECT_EXTERNAL_REPLY_INFO),
    TELEBOT_JSON_OBJECT("quote", telebot_message_t, quote, TELEBOT_OBJECT_TEXT_QUOTE),
    TELEBOT_JSON_OBJECT("reply_to_story", telebot_message_t, reply_to_story, TELEBOT_OBJECT_STORY),
    TELEBOT_JSON_OBJECT("via_bot", telebot_message_t, via_bot, TELEBOT_OBJECT_USER),
    TELEBOT_JSON_OBJECT("link_preview_options", telebot_message_t, link_preview_options, TELEBOT_OBJECT_LINK_PREVIEW_OPTIONS),
    TELEBOT_JSON_OBJECT("animation", telebot_message_t, animation, TELEBOT_OBJECT_ANIMATION),
    TELEBOT_JSON_OBJECT("audio", telebot_message_t, audio, TELEBOT_OBJECT_AUDIO),
    TELEBOT_JSON_OBJECT("document", telebot_message_t, document, TELEBOT_OBJECT_DOCUMENT),
    TELEBOT_JSON_OBJECT("video", telebot_message_t, video, TELEBOT_OBJECT_VIDEO),
    TELEBOT_JSON_OBJECT("video_note", telebot_message_t, video_note, TELEBOT_OBJECT_VIDEO_NOTE),
    TELEBOT_JSON_OBJECT("voice", telebot_message_t, voice, TELEBOT_OBJECT_VOICE),
    TELEBOT_JSON_OBJECT("contact", telebot_message_t, contact, TELEBOT_OBJECT_CONTACT),
    TELEBOT_JSON_OBJECT("dice", telebot_message_t, dice, TELEBOT_OBJECT_DICE),
    TELEBOT_JSON_OBJECT("poll", telebot_message_t, poll, TELEBOT_OBJECT_POLL),
    TELEBOT_JSON_OBJECT("venue", telebot_message_t, venue, TELEBOT_OBJECT_VENUE),
    TELEBOT_JSON_OBJECT("location", telebot_message_t, location, TELEBOT_OBJECT_LOCATION),
    TELEBOT_JSON_OBJECT("pinned_message", telebot_message_t, pinned_message, TELEBOT_OBJECT_MESSAGE),
    TELEBOT_JSON_OBJECT("reply_markup", telebot_message_t, reply_markup, TELEBOT_OBJECT_INLINE_KEYBOARD_MARKUP),
};

telebot_error_e telebot_parser_get_message(struct cJSON *obj, telebot_message_t *msg)
{
    if ((obj == NULL) || (msg == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(msg, 0, sizeof(telebot_message_t));

    /* Mandatory Fields */
    struct cJSON *message_id = NULL;
    if (!telebot_json_get_ex(obj, "message_id", &message_id))
    {
        ERR("Failed to get <message_id> from message object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    msg->message_id = telebot_json_get_int(message_id);

    struct cJSON *date = NULL;
    if (!telebot_json_get_ex(obj, "date", &date))
    {
        ERR("Failed to get <date> from message object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    msg->date = telebot_json_get_int(date);

    struct cJSON *chat = NULL;
    if (!telebot_json_get_ex(obj, "chat", &chat))
    {
        ERR("Failed to get <date> from message object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    msg->chat = telebot_mem_calloc(1, sizeof(telebot_chat_t));
    if (telebot_parser_get_chat(chat, msg->chat) != TELEBOT_ERROR_NONE)
    {
        ERR("Failed to get <chat> from message object");
        TELEBOT_SAFE_FREE(msg->chat);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    telebot_parse_fields(obj, msg, telebot_parser_get_message_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_message_objects));

    /* Optional Fields */
    telebot_parse_fields(obj, msg, telebot_parser_get_message_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_message_fields));

    struct cJSON *entities = NULL;
    if (telebot_json_get_ex(obj, "entities", &entities))
    {
        if (telebot_parser_get_message_entities(entities, &(msg->entities), &(msg->count_entities)) !=
            TELEBOT_ERROR_NONE)
            ERR("Failed to get <entities> from message object");
    }

    struct cJSON *sticker = NULL;
    if (telebot_json_get_ex(obj, "sticker", &sticker))
    {
        msg->sticker = telebot_mem_calloc(1, sizeof(telebot_sticker_t));
        if ((msg->sticker != NULL) && (telebot_parser_get_sticker(sticker, msg->sticker) != TELEBOT_ERROR_NONE))
        {
            ERR("Failed to get <sticker> from message object");
            msg->sticker = NULL;
        }
    }

#if TELEBOT_FEATURE_PAYMENTS
    struct cJSON *paid_media = NULL;
    if (telebot_json_get_ex(obj, "paid_media", &paid_media))
    {
        msg->paid_media = telebot_mem_calloc(1, sizeof(telebot_paid_media_info_t));
        if (telebot_parser_get_paid_media_info(paid_media, msg->paid_media) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <paid_media> from message object");
            TELEBOT_SAFE_FREE(msg->paid_media);
        }
    }
#endif

    struct cJSON *photo = NULL;
    if (telebot_json_get_ex(obj, "photo", &photo))
    {
        if (telebot_parser_get_photos(photo, &(msg->photos), &(msg->count_photos)) != TELEBOT_ERROR_NONE)
            ERR("Failed to get <photo> from message object");
    }

    // TODO: implement sticker parsing

    struct cJSON *caption_entities = NULL;
    if (telebot_json_get_ex(obj, "caption_entities", &caption_entities))
    {
        if (telebot_parser_get_message_entities(caption_entities, &(msg->caption_entities),
                                                &(msg->count_caption_entities)) != TELEBOT_ERROR_NONE)
            ERR("Failed to get <caption_entities> from message object");
    }

    // TODO: implement game parsing

    struct cJSON *new_chat_members = NULL;
    if (telebot_json_get_ex(obj, "new_chat_members", &new_chat_members))
    {
        int ret = telebot_parser_get_users(new_chat_members, &(msg->new_chat_members), &(msg->count_new_chat_members));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <new_chat_members> from message object");
    }

    struct cJSON *left_chat_members = NULL;
    if (telebot_json_get_ex(obj, "left_chat_members", &left_chat_members))
    {
        int ret = telebot_parser_get_users(left_chat_members, &(msg->left_chat_members), &(msg->count_left_chat_members));
        if (ret != TELEBOT_ERROR_NONE)
            ERR("Failed to get <left_chat_members> from message object");
    }

    struct cJSON *new_chat_photo = NULL;
    if (telebot_json_get_ex(obj, "new_chat_photo", &new_chat_photo))
    {
        if (telebot_parser_get_photos(new_chat_photo, &(msg->new_chat_photos), &(msg->count_new_chat_photos)) !=
            TELEBOT_ERROR_NONE)
            ERR("Failed to get <new_chat_photo> from message object");
    }

    struct cJSON *message_auto_delete_timer_changed = NULL;
    if (telebot_json_get_ex(obj, "message_auto_delete_timer_changed", &message_auto_delete_timer_changed))
    {
        msg->message_auto_delete_timer_changed = telebot_mem_malloc(sizeof(telebot_message_auto_delete_timer_changed_t));
        int ret = telebot_parser_get_message_auto_delete_timer_changed(message_auto_delete_timer_changed,
                                                                       msg->message_auto_delete_timer_changed);
        if (ret != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <message_auto_delete_timer_changed> from message object");
            TELEBOT_SAFE_FREE(msg->message_auto_delete_timer_changed);
        }
    }

#if TELEBOT_FEATURE_PAYMENTS
    struct cJSON *invoice = NULL;
    if (telebot_json_get_ex(obj, "invoice", &invoice))
    {
        msg->invoice = telebot_mem_calloc(1, sizeof(telebot_invoice_t));
        if (telebot_parser_get_invoice(invoice, msg->invoice) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <invoice> from message object");
            TELEBOT_SAFE_FREE(msg->invoice);
        }
    }
#endif

#if TELEBOT_FEATURE_PAYMENTS
    struct cJSON *successful_payment = NULL;
    if (telebot_json_get_ex(obj, "successful_payment", &successful_payment))
    {
        msg->successful_payment = telebot_mem_calloc(1, sizeof(telebot_successful_payment_t));
        if (telebot_parser_get_successful_payment(successful_payment, msg->successful_payment) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <successful_payment> from message object");
            TELEBOT_SAFE_FREE(msg->successful_payment);
        }
    }
#endif

#if TELEBOT_FEATURE_PAYMENTS
    struct cJSON *refunded_payment = NULL;
    if (telebot_json_get_ex(obj, "refunded_payment", &refunded_payment))
    {
        msg->refunded_payment = telebot_mem_calloc(1, sizeof(telebot_refunded_payment_t));
        if (telebot_parser_get_refunded_payment(refunded_payment, msg->refunded_payment) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <refunded_payment> from message object");
            TELEBOT_SAFE_FREE(msg->refunded_payment);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *write_access_allowed = NULL;
    if (telebot_json_get_ex(obj, "write_access_allowed", &write_access_allowed))
    {
        msg->write_access_allowed = telebot_mem_calloc(1, sizeof(telebot_write_access_allowed_t));
        if (telebot_parser_get_write_access_allowed(write_access_allowed, msg->write_access_allowed) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <write_access_allowed> from message object");
            TELEBOT_SAFE_FREE(msg->write_access_allowed);
        }
    }
#endif

#if TELEBOT_FEATURE_PASSPORT
    struct cJSON *passport_data = NULL;
    if (telebot_json_get_ex(obj, "passport_data", &passport_data))
    {
        msg->passport_data = telebot_mem_calloc(1, sizeof(telebot_passport_data_t));
        if (telebot_parser_get_passport_data(passport_data, msg->passport_data) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <passport_data> from message object");
            TELEBOT_SAFE_FREE(msg->passport_data);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *proximity_alert_triggered = NULL;
    if (telebot_json_get_ex(obj, "proximity_alert_triggered", &proximity_alert_triggered))
    {
        msg->proximity_alert_triggered = telebot_mem_calloc(1, sizeof(telebot_proximity_alert_triggered_t));
        if (telebot_parser_get_proximity_alert_triggered(proximity_alert_triggered, msg->proximity_alert_triggered) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <proximity_alert_triggered> from message object");
            TELEBOT_SAFE_FREE(msg->proximity_alert_triggered);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *forum_topic_created = NULL;
    if (telebot_json_get_ex(obj, "forum_topic_created", &forum_topic_created))
    {
        msg->forum_topic_created = telebot_mem_calloc(1, sizeof(telebot_forum_topic_created_t));
        if (telebot_parser_get_forum_topic_created(forum_topic_created, msg->forum_topic_created) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <forum_topic_created> from message object");
            TELEBOT_SAFE_FREE(msg->forum_topic_created);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *forum_topic_edited = NULL;
    if (telebot_json_get_ex(obj, "forum_topic_edited", &forum_topic_edited))
    {
        msg->forum_topic_edited = telebot_mem_calloc(1, sizeof(telebot_forum_topic_edited_t));
        if (telebot_parser_get_forum_topic_edited(forum_topic_edited, msg->forum_topic_edited) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <forum_topic_edited> from message object");
            TELEBOT_SAFE_FREE(msg->forum_topic_edited);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *forum_topic_closed = NULL;
    if (telebot_json_get_ex(obj, "forum_topic_closed", &forum_topic_closed))
    {
        msg->forum_topic_closed = telebot_mem_calloc(1, sizeof(telebot_forum_topic_closed_t));
        msg->forum_topic_closed->dummy = true;
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *forum_topic_reopened = NULL;
    if (telebot_json_get_ex(obj, "forum_topic_reopened", &forum_topic_reopened))
    {
        msg->forum_topic_reopened = telebot_mem_calloc(1, sizeof(telebot_forum_topic_reopened_t));
        msg->forum_topic_reopened->dummy = true;
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *video_chat_scheduled = NULL;
    if (telebot_json_get_ex(obj, "video_chat_scheduled", &video_chat_scheduled))
    {
        msg->video_chat_scheduled = telebot_mem_calloc(1, sizeof(telebot_video_chat_scheduled_t));
        if (telebot_parser_get_video_chat_scheduled(video_chat_scheduled, msg->video_chat_scheduled) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <video_chat_scheduled> from message object");
            TELEBOT_SAFE_FREE(msg->video_chat_scheduled);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *video_chat_started = NULL;
    if (telebot_json_get_ex(obj, "video_chat_started", &video_chat_started))
    {
        msg->video_chat_started = telebot_mem_calloc(1, sizeof(telebot_video_chat_started_t));
        msg->video_chat_started->dummy = true;
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *video_chat_ended = NULL;
    if (telebot_json_get_ex(obj, "video_chat_ended", &video_chat_ended))
    {
        msg->video_chat_ended = telebot_mem_calloc(1, sizeof(telebot_video_chat_ended_t));
        if (telebot_parser_get_video_chat_ended(video_chat_ended, msg->video_chat_ended) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <video_chat_ended> from message object");
            TELEBOT_SAFE_FREE(msg->video_chat_ended);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *video_chat_participants_invited = NULL;
    if (telebot_json_get_ex(obj, "video_chat_participants_invited", &video_chat_participants_invited))
    {
        msg->video_chat_participants_invited = telebot_mem_calloc(1, sizeof(telebot_video_chat_participants_invited_t));
        if (telebot_parser_get_video_chat_participants_invited(video_chat_participants_invited, msg->video_chat_participants_invited) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <video_chat_participants_invited> from message object");
            TELEBOT_SAFE_FREE(msg->video_chat_participants_invited);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *web_app_data = NULL;
    if (telebot_json_get_ex(obj, "web_app_data", &web_app_data))
    {
        msg->web_app_data = telebot_mem_calloc(1, sizeof(telebot_web_app_data_t));
        if (telebot_parser_get_web_app_data(web_app_data, msg->web_app_data) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <web_app_data> from message object");
            TELEBOT_SAFE_FREE(msg->web_app_data);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *users_shared = NULL;
    if (telebot_json_get_ex(obj, "users_shared", &users_shared))
    {
        msg->users_shared = telebot_mem_calloc(1, sizeof(telebot_users_shared_t));
        if (telebot_parser_get_users_shared(users_shared, msg->users_shared) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <users_shared> from message object");
            TELEBOT_SAFE_FREE(msg->users_shared);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *chat_shared = NULL;
    if (telebot_json_get_ex(obj, "chat_shared", &chat_shared))
    {
        msg->chat_shared = telebot_mem_calloc(1, sizeof(telebot_chat_shared_t));
        if (telebot_parser_get_chat_shared(chat_shared, msg->chat_shared) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <chat_shared> from message object");
            TELEBOT_SAFE_FREE(msg->chat_shared);
        }
    }
#endif

#if TELEBOT_FEATURE_GIFTS
    struct cJSON *gift = NULL;
    if (telebot_json_get_ex(obj, "gift", &gift))
    {
        msg->gift = telebot_mem_calloc(1, sizeof(telebot_gift_info_t));
        if (telebot_parser_get_gift_info(gift, msg->gift) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <gift> from message object");
            TELEBOT_SAFE_FREE(msg->gift);
        }
    }
#endif

#if TELEBOT_FEATURE_GIFTS
    struct cJSON *unique_gift = NULL;
    if (telebot_json_get_ex(obj, "unique_gift", &unique_gift))
    {
        msg->unique_gift = telebot_mem_calloc(1, sizeof(telebot_unique_gift_info_t));
        if (telebot_parser_get_unique_gift_info(unique_gift, msg->unique_gift) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <unique_gift> from message object");
            TELEBOT_SAFE_FREE(msg->unique_gift);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *boost_added = NULL;
    if (telebot_json_get_ex(obj, "boost_added", &boost_added))
    {
        msg->boost_added = telebot_mem_calloc(1, sizeof(telebot_chat_boost_added_t));
        if (telebot_parser_get_chat_boost_added(boost_added, msg->boost_added) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <boost_added> from message object");
            TELEBOT_SAFE_FREE(msg->boost_added);
        }
    }
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
    struct cJSON *chat_background_set = NULL;
    if (telebot_json_get_ex(obj, "chat_background_set", &chat_background_set))
    {
        msg->chat_background_set = telebot_mem_calloc(1, sizeof(telebot_chat_background_t));
        if (telebot_parser_get_chat_background(chat_background_set, msg->chat_background_set) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <chat_background_set> from message object");
            TELEBOT_SAFE_FREE(msg->chat_background_set);
        }
    }
#endif

#if TELEBOT_FEATURE_GIFTS
    struct cJSON *giveaway_created = NULL;
    if (telebot_json_get_ex(obj, "giveaway_created", &giveaway_created))
    {
        msg->giveaway_created = telebot_mem_calloc(1, sizeof(telebot_giveaway_created_t));
        if (telebot_parser_get_giveaway_created(giveaway_created, msg->giveaway_created) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <giveaway_created> from message object");
            TELEBOT_SAFE_FREE(msg->giveaway_created);
        }
    }
#endif

#if TELEBOT_FEATURE_GIFTS
    struct cJSON *giveaway_completed = NULL;
    if (telebot_json_get_ex(obj, "giveaway_completed", &giveaway_completed))
    {
        msg->giveaway_completed = telebot_mem_calloc(1, sizeof(telebot_giveaway_completed_t));
        if (telebot_parser_get_giveaway_completed(giveaway_completed, msg->giveaway_completed) != TELEBOT_ERROR_NONE)
        {
            ERR("Failed to get <giveaway_completed> from message object");
            TELEBOT_SAFE_FREE(msg->giveaway_completed);
        }
    }
#endif

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_message_entity_fields[] = {
    TELEBOT_JSON_FIELD("url", telebot_message_entity_t, url, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("language", telebot_message_entity_t, language, TELEBOT_FIELD_STRING),
};

static const telebot_json_field_t telebot_parser_get_message_entity_objects[] = {
    TELEBOT_JSON_OBJECT("user", telebot_message_entity_t, user, TELEBOT_OBJECT_USER),
};

telebot_error_e telebot_parser_get_message_entity(struct cJSON *obj, telebot_message_entity_t *entity)
{
    if ((obj == NULL) || (entity == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(entity, 0, sizeof(telebot_message_entity_t));
    struct cJSON *type = NULL;
    if (!telebot_json_get_ex(obj, "type", &type))
    {
        ERR("Object is not message entity type, type not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    entity->type = TELEBOT_SAFE_STRDUP(telebot_json_get_string(type));

    struct cJSON *offset = NULL;
    if (!telebot_json_get_ex(obj, "offset", &offset))
    {
        TELEBOT_SAFE_FREE(entity->type);
        ERR("Object is not message entity type, offset not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    entity->offset = telebot_json_get_int(offset);

    struct cJSON *length = NULL;
    if (!telebot_json_get_ex(obj, "length", &length))
    {
        ERR("Object is not message entity type, length not found");
        TELEBOT_SAFE_FZCNT(entity->type, entity->offset);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    entity->length = telebot_json_get_int(length);

    telebot_parse_fields(obj, entity, telebot_parser_get_message_entity_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_message_entity_fields));

    telebot_parse_fields(obj, entity, telebot_parser_get_message_entity_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_message_entity_objects));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_message_entities(struct cJSON *obj, telebot_message_entity_t **entities, int *count)
{
    struct cJSON *array = obj;
    int array_len = telebot_json_array_length(array);
    if (array_len == 0)
        return TELEBOT_ERROR_OPERATION_FAILED;

    telebot_message_entity_t *result = telebot_mem_calloc(array_len, sizeof(telebot_message_entity_t));
    if (result == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    *count = array_len;
    *entities = result;

    for (int index = 0; index < array_len; index++)
    {
        struct cJSON *item = telebot_json_array_get_idx(array, index);
        if (telebot_parser_get_message_entity(item, &(result[index])) != TELEBOT_ERROR_NONE)
            ERR("Failed to parse user from users array");
    }

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_photo_fields[] = {
    TELEBOT_JSON_FIELD("file_size", telebot_photo_t, file_size, TELEBOT_FIELD_INT),
};

telebot_error_e telebot_parser_get_photo(struct cJSON *obj, telebot_photo_t *photo)
{
    if ((obj == NULL) || (photo == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(photo, 0, sizeof(telebot_photo_t));
    struct cJSON *file_id = NULL;
    if (!telebot_json_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not photo size type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->file_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(file_id));

    struct cJSON *width = NULL;
    if (!telebot_json_get_ex(obj, "width", &width))
    {
        ERR("Object is not photo size type, width not found");
        TELEBOT_SAFE_FREE(photo->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->width = telebot_json_get_int(width);

    struct cJSON *height = NULL;
    if (!telebot_json_get_ex(obj, "height", &height))
    {
        ERR("Object is not photo size type, height not found");
        TELEBOT_SAFE_FZCNT(photo->file_id, photo->width);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->height = telebot_json_get_int(height);

    telebot_parse_fields(obj, photo, telebot_parser_get_photo_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_photo_fields));

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_photos(struct cJSON *obj, telebot_photo_t **photos, int *count)
{
    struct cJSON *array = obj;
    int array_len = telebot_json_array_length(array);
    if (array_len == 0)
        return TELEBOT_ERROR_OPERATION_FAILED;

    telebot_photo_t *result = telebot_mem_calloc(array_len, sizeof(telebot_photo_t));
    if (result == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    *count = array_len;
    *photos = result;

    for (int index = 0; index < array_len; index++)
    {
        struct cJSON *item = telebot_json_array_get_idx(array, index);
        if (telebot_parser_get_photo(item, &(result[index])) != TELEBOT_ERROR_NONE)
            ERR("Failed to parse user from users array");
    }

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_audio_fields[] = {
    TELEBOT_JSON_FIELD("performer", telebot_audio_t, performer, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("title", telebot_audio_t, title, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("mime_type", telebot_audio_t, mime_type, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("file_size", telebot_audio_t, file_size, TELEBOT_FIELD_INT),
};

static const telebot_json_field_t telebot_parser_get_audio_objects[] = {
    TELEBOT_JSON_OBJECT("thumbnail", telebot_audio_t, thumbnail, TELEBOT_OBJECT_PHOTO),
};

telebot_error_e telebot_parser_get_audio(struct cJSON *obj, telebot_audio_t *audio)
{
    if ((obj == NULL) || (audio == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(audio, 0, sizeof(telebot_audio_t));
    struct cJSON *file_id = NULL;
    if (!telebot_json_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not audio type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    audio->file_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(file_id));

    struct cJSON *file_unique_id = NULL;
    if (!telebot_json_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not audio type, file_unique_id not found");
        TELEBOT_SAFE_FREE(audio->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    audio->file_unique_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(file_unique_id));

    struct cJSON *duration = NULL;
    if (!telebot_json_get_ex(obj, "duration", &duration))
    {
        ERR("Object is not audio type, duration not found");
        TELEBOT_SAFE_FREE(audio->file_id);
        TELEBOT_SAFE_FREE(audio->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    audio->duration = telebot_json_get_int(duration);

    telebot_parse_fields(obj, audio, telebot_parser_get_audio_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_audio_fields));

    telebot_parse_fields(obj, audio, telebot_parser_get_audio_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_audio_objects));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_document_fields[] = {
    TELEBOT_JSON_FIELD("file_name", telebot_document_t, file_name, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("mime_type", telebot_document_t, mime_type, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("file_size", telebot_document_t, file_size, TELEBOT_FIELD_INT),
};

static const telebot_json_field_t telebot_parser_get_document_objects[] = {
    TELEBOT_JSON_OBJECT("thumbnail", telebot_document_t, thumbnail, TELEBOT_OBJECT_PHOTO),
};

telebot_error_e telebot_parser_get_document(struct cJSON *obj, telebot_document_t *document)
{
    if ((obj == NULL) || (document == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(document, 0, sizeof(telebot_document_t));
    struct cJSON *file_id = NULL;
    if (!telebot_json_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not document type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    document->file_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(file_id));

    struct cJSON *file_unique_id = NULL;
    if (!telebot_json_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not document type, file_unique_id not found");
        TELEBOT_SAFE_FREE(document->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    document->file_unique_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(file_unique_id));

    telebot_parse_fields(obj, document, telebot_parser_get_document_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_document_objects));

    telebot_parse_fields(obj, document, telebot_parser_get_document_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_document_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_video_fields[] = {
    TELEBOT_JSON_FIELD("mime_type", telebot_video_t, mime_type, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("file_size", telebot_video_t, file_size, TELEBOT_FIELD_INT),
};

static const telebot_json_field_t telebot_parser_get_video_objects[] = {
    TELEBOT_JSON_OBJECT("thumbnail", telebot_video_t, thumbnail, TELEBOT_OBJECT_PHOTO),
};

telebot_error_e telebot_parser_get_video(struct cJSON *obj, telebot_video_t *video)
{
    if ((obj == NULL) || (video == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(video, 0, sizeof(telebot_video_t));
    struct cJSON *file_id = NULL;
    if (!telebot_json_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not video type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video->file_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(file_id));

    struct cJSON *file_unique_id = NULL;
    if (!telebot_json_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not video type, file_unique_id not found");
        TELEBOT_SAFE_FREE(video->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video->file_unique_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(file_unique_id));

    struct cJSON *width = NULL;
    if (!telebot_json_get_ex(obj, "width", &width))
    {
        ERR("Object is not video type, width not found");
        TELEBOT_SAFE_FREE(video->file_id);
        TELEBOT_SAFE_FREE(video->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video->width = telebot_json_get_int(width);

    struct cJSON *height = NULL;
    if (!telebot_json_get_ex(obj, "height", &height))
    {
        ERR("Object is not video type, height not found");
        TELEBOT_SAFE_FREE(video->file_id);
        TELEBOT_SAFE_FREE(video->file_unique_id);
        video->width = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video->height = telebot_json_get_int(height);

    struct cJSON *duration = NULL;
    if (!telebot_json_get_ex(obj, "duration", &duration))
    {
        ERR("Object is not video type, duration not found");
        TELEBOT_SAFE_FREE(video->file_id);
        TELEBOT_SAFE_FREE(video->file_unique_id);
        video->width = 0;
        video->height = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video->duration = telebot_json_get_int(duration);

    telebot_parse_fields(obj, video, telebot_parser_get_video_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_video_objects));

    telebot_parse_fields(obj, video, telebot_parser_get_video_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_video_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_animation_fields[] = {
    TELEBOT_JSON_FIELD("mime_type", telebot_animation_t, mime_type, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("file_size", telebot_animation_t, file_size, TELEBOT_FIELD_INT),
};

static const telebot_json_field_t telebot_parser_get_animation_objects[] = {
    TELEBOT_JSON_OBJECT("thumbnail", telebot_animation_t, thumbnail, TELEBOT_OBJECT_PHOTO),
};

telebot_error_e telebot_parser_get_animation(struct cJSON *obj, telebot_animation_t *animation)
{
    if ((obj == NULL) || (animation == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(animation, 0, sizeof(telebot_animation_t));
    struct cJSON *file_id = NULL;
    if (!telebot_json_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not animation type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    animation->file_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(file_id));

    struct cJSON *file_unique_id = NULL;
    if (!telebot_json_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not animation type, file_unique_id not found");
        TELEBOT_SAFE_FREE(animation->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    animation->file_unique_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(file_unique_id));

    struct cJSON *width = NULL;
    if (!telebot_json_get_ex(obj, "width", &width))
    {
        ERR("Object is not animation type, width not found");
        TELEBOT_SAFE_FREE(animation->file_id);
        TELEBOT_SAFE_FREE(animation->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    animation->width = telebot_json_get_int(width);

    struct cJSON *height = NULL;
    if (!telebot_json_get_ex(obj, "height", &height))
    {
        ERR("Object is not animation type, height not found");
        TELEBOT_SAFE_FREE(animation->file_id);
        TELEBOT_SAFE_FREE(animation->file_unique_id);
        animation->width = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    animation->height = telebot_json_get_int(height);

    struct cJSON *duration = NULL;
    if (!telebot_json_get_ex(obj, "duration", &duration))
    {
        ERR("Object is not animation type, duration not found");
        TELEBOT_SAFE_FREE(animation->file_id);
        TELEBOT_SAFE_FREE(animation->file_unique_id);
        animation->width = 0;
        animation->height = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    animation->duration = telebot_json_get_int(duration);

    telebot_parse_fields(obj, animation, telebot_parser_get_animation_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_animation_objects));

    telebot_parse_fields(obj, animation, telebot_parser_get_animation_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_animation_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_video_note_fields[] = {
    TELEBOT_JSON_FIELD("file_size", telebot_video_note_t, file_size, TELEBOT_FIELD_INT),
};

static const telebot_json_field_t telebot_parser_get_video_note_objects[] = {
    TELEBOT_JSON_OBJECT("thumbnail", telebot_video_note_t, thumbnail, TELEBOT_OBJECT_PHOTO),
};

telebot_error_e telebot_parser_get_video_note(struct cJSON *obj, telebot_video_note_t *video_note)
{
    if ((obj == NULL) || (video_note == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(video_note, 0, sizeof(telebot_video_note_t));
    struct cJSON *file_id = NULL;
    if (!telebot_json_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not video note type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video_note->file_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(file_id));

    struct cJSON *file_unique_id = NULL;
    if (!telebot_json_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not video_note type, file_unique_id not found");
        TELEBOT_SAFE_FREE(video_note->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video_note->file_unique_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(file_unique_id));

    struct cJSON *length = NULL;
    if (!telebot_json_get_ex(obj, "length", &length))
    {
        ERR("Object is not video note type, length not found");
        TELEBOT_SAFE_FREE(video_note->file_id);
        TELEBOT_SAFE_FREE(video_note->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video_note->length = telebot_json_get_int(length);

    struct cJSON *duration = NULL;
    if (!telebot_json_get_ex(obj, "duration", &duration))
    {
        ERR("Object is not video note type, duration not found");
        TELEBOT_SAFE_FREE(video_note->file_id);
        TELEBOT_SAFE_FREE(video_note->file_unique_id);
        video_note->length = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    video_note->duration = telebot_json_get_int(duration);

    telebot_parse_fields(obj, video_note, telebot_parser_get_video_note_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_video_note_objects));

    telebot_parse_fields(obj, video_note, telebot_parser_get_video_note_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_video_note_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_voice_fields[] = {
    TELEBOT_JSON_FIELD("mime_type", telebot_voice_t, mime_type, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("file_size", telebot_voice_t, file_size, TELEBOT_FIELD_INT),
};

telebot_error_e telebot_parser_get_voice(struct cJSON *obj, telebot_voice_t *voice)
{
    if ((obj == NULL) || (voice == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(voice, 0, sizeof(telebot_voice_t));
    struct cJSON *file_id = NULL;
    if (!telebot_json_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not voice type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    voice->file_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(file_id));

    struct cJSON *file_unique_id = NULL;
    if (!telebot_json_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not voice type, file_unique_id not found");
        TELEBOT_SAFE_FREE(voice->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    voice->file_unique_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(file_unique_id));

    struct cJSON *duration = NULL;
    if (!telebot_json_get_ex(obj, "duration", &duration))
    {
        ERR("Object is not voice type, voice duration not found");
        TELEBOT_SAFE_FREE(voice->file_id);
        TELEBOT_SAFE_FREE(voice->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    voice->duration = telebot_json_get_int(duration);

    telebot_parse_fields(obj, voice, telebot_parser_get_voice_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_voice_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_contact_fields[] = {
    TELEBOT_JSON_FIELD("last_name", telebot_contact_t, last_name, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("user_id", telebot_contact_t, user_id, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("vcard", telebot_contact_t, vcard, TELEBOT_FIELD_STRING),
};

telebot_error_e telebot_parser_get_contact(struct cJSON *obj, telebot_contact_t *contact)
{
    if ((obj == NULL) || (contact == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(contact, 0, sizeof(telebot_contact_t));
    struct cJSON *phone_number = NULL;
    if (!telebot_json_get_ex(obj, "phone_number", &phone_number))
    {
        ERR("Object is not contact type, phone number not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    contact->phone_number = TELEBOT_SAFE_STRDUP(telebot_json_get_string(phone_number));

    struct cJSON *first_name = NULL;
    if (!telebot_json_get_ex(obj, "first_name", &first_name))
    {
        ERR("Object is not contact type, first name not found");
        TELEBOT_SAFE_FREE(contact->phone_number);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    contact->first_name = TELEBOT_SAFE_STRDUP(telebot_json_get_string(first_name));

    telebot_parse_fields(obj, contact, telebot_parser_get_contact_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_contact_fields));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_location(struct cJSON *obj, telebot_location_t *location)
{
    if ((obj == NULL) || (location == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(location, 0, sizeof(telebot_location_t));
    struct cJSON *latitude = NULL;
    if (!telebot_json_get_ex(obj, "latitude", &latitude))
    {
        ERR("Object is not location type, latitude not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    location->latitude = telebot_json_get_double(latitude);

    struct cJSON *longitude = NULL;
    if (!telebot_json_get_ex(obj, "longitude", &longitude))
    {
        ERR("Object is not location type, longitude not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    location->longitude = telebot_json_get_double(longitude);

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_venue_fields[] = {
    TELEBOT_JSON_FIELD("foursquare_id", telebot_venue_t, foursquare_id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("foursquare_type", telebot_venue_t, foursquare_type, TELEBOT_FIELD_STRING),
};

telebot_error_e telebot_parser_get_venue(struct cJSON *obj, telebot_venue_t *venue)
{
    if ((obj == NULL) || (venue == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(venue, 0, sizeof(telebot_venue_t));
    struct cJSON *title = NULL;
    if (!telebot_json_get_ex(obj, "title", &title))
    {
        ERR("Object is not venue type, title not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    venue->title = TELEBOT_SAFE_STRDUP(telebot_json_get_string(title));

    struct cJSON *address = NULL;
    if (!telebot_json_get_ex(obj, "address", &address))
    {
        ERR("Object is not venue type, address not found");
        TELEBOT_SAFE_FREE(venue->title);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    venue->address = TELEBOT_SAFE_STRDUP(telebot_json_get_string(address));

    struct cJSON *location = NULL;
    if (telebot_json_get_ex(obj, "location", &location))
    {
        venue->location = telebot_mem_calloc(1, sizeof(telebot_location_t));
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

    telebot_parse_fields(obj, venue, telebot_parser_get_venue_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_venue_fields));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_poll_option(struct cJSON *obj, telebot_poll_option_t *option)
{
    if ((obj == NULL) || (option == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(option, 0, sizeof(telebot_poll_option_t));
    struct cJSON *text = NULL;
    if (!telebot_json_get_ex(obj, "text", &text))
    {
        ERR("Object is not poll option type, text not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    option->text = TELEBOT_SAFE_STRDUP(telebot_json_get_string(text));

    struct cJSON *voter_count = NULL;
    if (!telebot_json_get_ex(obj, "voter_count", &voter_count))
    {
        ERR("Object is not pol option type, voter_count not found");
        TELEBOT_SAFE_FREE(option->text);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    option->voter_count = telebot_json_get_int(voter_count);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_poll_answer(struct cJSON *obj, telebot_poll_answer_t *answer)
{
    if ((obj == NULL) || (answer == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(answer, 0, sizeof(telebot_poll_answer_t));
    struct cJSON *poll_id = NULL;
    if (!telebot_json_get_ex(obj, "poll_id", &poll_id))
    {
        ERR("Object is not poll answer type, poll_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    answer->poll_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(poll_id));

    struct cJSON *option_ids = NULL;
    if (!telebot_json_get_ex(obj, "option_ids", &option_ids))
    {
        ERR("Object is not pol answer type, option_ids not found");
        TELEBOT_SAFE_FREE(answer->poll_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    answer->count_option_ids = telebot_json_array_length(option_ids);
    if (answer->count_option_ids == 0)
    {
        ERR("Object is not pol answer type, option_ids is not array");
        TELEBOT_SAFE_FREE(answer->poll_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    answer->option_ids = telebot_mem_calloc(answer->count_option_ids, sizeof(*(answer->option_ids)));
    if (answer->option_ids == NULL)
    {
        ERR("Failed to allocate memory for option_ids");
        TELEBOT_SAFE_FREE(answer->poll_id);
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    for (int index = 0; index < answer->count_option_ids; index++)
        answer->option_ids[index] = telebot_json_get_int(telebot_json_array_get_idx(option_ids, index));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_poll(struct cJSON *obj, telebot_poll_t *poll)
{
    if ((obj == NULL) || (poll == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(poll, 0, sizeof(telebot_poll_t));
    struct cJSON *id = NULL;
    if (!telebot_json_get_ex(obj, "id", &id))
    {
        ERR("Object is not poll type, id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(id));

    struct cJSON *question = NULL;
    if (!telebot_json_get_ex(obj, "question", &question))
    {
        ERR("Object is not pol type, question not found");
        TELEBOT_SAFE_FREE(poll->id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->question = TELEBOT_SAFE_STRDUP(telebot_json_get_string(question));

    struct cJSON *total_voter_count = NULL;
    if (!telebot_json_get_ex(obj, "total_voter_count", &total_voter_count))
    {
        ERR("Object is not pol type, total_voter_count not found");
        TELEBOT_SAFE_FREE(poll->id);
        TELEBOT_SAFE_FREE(poll->question);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->total_voter_count = telebot_json_get_int(total_voter_count);

    struct cJSON *is_closed = NULL;
    if (!telebot_json_get_ex(obj, "is_closed", &is_closed))
    {
        ERR("Object is not pol type, is_closed not found");
        TELEBOT_SAFE_FREE(poll->id);
        TELEBOT_SAFE_FREE(poll->question);
        poll->total_voter_count = 0;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->is_closed = telebot_json_get_bool(is_closed);

    struct cJSON *is_anonymous = NULL;
    if (!telebot_json_get_ex(obj, "is_anonymous", &is_anonymous))
    {
        ERR("Object is not pol type, is_anonymous not found");
        TELEBOT_SAFE_FREE(poll->id);
        TELEBOT_SAFE_FREE(poll->question);
        poll->total_voter_count = 0;
        poll->is_closed = false;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->is_anonymous = telebot_json_get_bool(is_anonymous);

    struct cJSON *type = NULL;
    if (!telebot_json_get_ex(obj, "type", &type))
    {
        ERR("Object is not pol type, type not found");
        TELEBOT_SAFE_FREE(poll->id);
        TELEBOT_SAFE_FREE(poll->question);
        poll->total_voter_count = 0;
        poll->is_closed = false;
        poll->is_anonymous = false;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    poll->type = TELEBOT_SAFE_STRDUP(telebot_json_get_string(type));

    struct cJSON *allows_multiple_answers = NULL;
    if (!telebot_json_get_ex(obj, "allows_multiple_answers", &allows_multiple_answers))
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
    poll->allows_multiple_answers = telebot_json_get_bool(allows_multiple_answers);

    struct cJSON *options = NULL;
    if (!telebot_json_get_ex(obj, "options", &options))
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

    poll->count_options = telebot_json_array_length(options);
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

    poll->options = telebot_mem_calloc(poll->count_options, sizeof(telebot_poll_option_t));
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
        telebot_parser_get_poll_option(telebot_json_array_get_idx(options, index), &(poll->options[index]));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_dice(struct cJSON *obj, telebot_dice_t *dice)
{
    if ((obj == NULL) || (dice == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(dice, 0, sizeof(telebot_dice_t));
    struct cJSON *value = NULL;
    if (!telebot_json_get_ex(obj, "value", &value))
    {
        ERR("Object is not dice type, value not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    dice->value = telebot_json_get_int(value);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user_profile_photos(struct cJSON *obj, telebot_user_profile_photos_t *photos)
{
    if ((obj == NULL) || (photos == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct cJSON *total_count_obj;
    if (!telebot_json_get_ex(obj, "total_count", &total_count_obj))
    {
        ERR("Object is not user profile photo type, total_count not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photos->total_count = telebot_json_get_int(total_count_obj);
    if (photos->total_count == 0)
        return TELEBOT_ERROR_NONE;

    struct cJSON *array;
    if (!telebot_json_get_ex(obj, "photos", &array))
    {
        ERR("Failed to get photos from <UserProfilePhotos> object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    int total = telebot_json_array_length(array);
    for (int i = 0; i < 4; i++)
        photos->photos[i] = telebot_mem_calloc(total, sizeof(telebot_photo_t));
    photos->current_count = total;

    telebot_error_e ret = TELEBOT_ERROR_NONE;
    for (int i = 0; i < total; i++)
    {
        struct cJSON *item = telebot_json_array_get_idx(array, i);
        int subtotal = telebot_json_array_length(item);
        if (subtotal > 4)
            subtotal = 4; /* This MUST not happen */
        for (int j = 0; j < subtotal; j++)
        {
            struct cJSON *photo = telebot_json_array_get_idx(item, j);
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
    if (!telebot_json_get_ex(obj, "file_id", &file_id))
    {
        ERR("Object is not file type, file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    file->file_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(file_id));

    struct cJSON *file_unique_id = NULL;
    if (!telebot_json_get_ex(obj, "file_unique_id", &file_unique_id))
    {
        ERR("Object is not file type, file_unique_id not found");
        TELEBOT_SAFE_FREE(file->file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    file->file_unique_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(file_unique_id));

    struct cJSON *file_path = NULL;
    if (!telebot_json_get_ex(obj, "file_path", &file_path))
    {
        ERR("Object is not file type, file_path not found");
        TELEBOT_SAFE_FREE(file->file_id);
        TELEBOT_SAFE_FREE(file->file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    file->file_path = TELEBOT_SAFE_STRDUP(telebot_json_get_string(file_path));

    struct cJSON *file_size;
    if (telebot_json_get_ex(obj, "file_size", &file_size))
        file->file_size = telebot_json_get_int(file_size);

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_callback_query_fields[] = {
    TELEBOT_JSON_FIELD("inline_message_id", telebot_callback_query_t, inline_message_id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("chat_instance", telebot_callback_query_t, chat_instance, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("data", telebot_callback_query_t, data, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("game_short_name", telebot_callback_query_t, game_short_name, TELEBOT_FIELD_STRING),
};

static const telebot_json_field_t telebot_parser_get_callback_query_objects[] = {
    TELEBOT_JSON_OBJECT("message", telebot_callback_query_t, message, TELEBOT_OBJECT_MESSAGE),
};

telebot_error_e telebot_parser_get_callback_query(struct cJSON *obj, telebot_callback_query_t *cb_query)
{
    if ((obj == NULL) || (cb_query == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct cJSON *id = NULL;
    if (!telebot_json_get_ex(obj, "id", &id))
    {
        ERR("Failed to get <id> from callback_query object");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    cb_query->id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(id));

    struct cJSON *from = NULL;
    if (telebot_json_get_ex(obj, "from", &from))
    {
        cb_query->from = telebot_mem_calloc(1, sizeof(telebot_user_t));
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

    telebot_parse_fields(obj, cb_query, telebot_parser_get_callback_query_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_callback_query_objects));

    telebot_parse_fields(obj, cb_query, telebot_parser_get_callback_query_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_callback_query_fields));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_photo(struct cJSON *obj, telebot_chat_photo_t *photo)
{
    if ((obj == NULL) || (photo == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(photo, 0, sizeof(telebot_chat_photo_t));
    struct cJSON *small_file_id = NULL;
    if (!telebot_json_get_ex(obj, "small_file_id", &small_file_id))
    {
        ERR("Object is not chat photo type, small_file_id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->small_file_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(small_file_id));

    struct cJSON *small_file_unique_id = NULL;
    if (!telebot_json_get_ex(obj, "small_file_unique_id", &small_file_unique_id))
    {
        ERR("Object is not chat photo type, small_file_unique_id not found");
        TELEBOT_SAFE_FREE(photo->small_file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->small_file_unique_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(small_file_unique_id));

    struct cJSON *big_file_id = NULL;
    if (!telebot_json_get_ex(obj, "big_file_id", &big_file_id))
    {
        ERR("Object is not chat photo type, big_file_id not found");
        TELEBOT_SAFE_FREE(photo->small_file_id);
        TELEBOT_SAFE_FREE(photo->small_file_unique_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->big_file_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(big_file_id));

    struct cJSON *big_file_unique_id = NULL;
    if (!telebot_json_get_ex(obj, "big_file_unique_id", &big_file_unique_id))
    {
        ERR("Object is not chat photo type, big_file_unique_id not found");
        TELEBOT_SAFE_FREE(photo->small_file_id);
        TELEBOT_SAFE_FREE(photo->small_file_unique_id);
        TELEBOT_SAFE_FREE(photo->big_file_id);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    photo->big_file_unique_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(big_file_unique_id));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat_member(struct cJSON *obj, telebot_chat_member_t *member)
{
    if ((obj == NULL) || (member == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(member, 0, sizeof(telebot_chat_member_t));
    cJSON *tmp = NULL;
    cJSON *user = NULL;
    if (!telebot_json_get_ex(obj, "status", &tmp) || !telebot_json_get_ex(obj, "user", &user))
    {
        ERR("Object is not a chat member: status or user missing");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    member->status = TELEBOT_SAFE_STRDUP(telebot_json_get_string(tmp));

    member->user = telebot_mem_calloc(1, sizeof(telebot_user_t));
    if ((member->user == NULL) || (telebot_parser_get_user(user, member->user) != TELEBOT_ERROR_NONE))
    {
        ERR("Failed to get <user> from chat member object");
        TELEBOT_SAFE_FREE(member->status);
        TELEBOT_SAFE_FREE(member->user);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    if (telebot_json_get_ex(obj, "custom_title", &tmp))
        member->custom_title = TELEBOT_SAFE_STRDUP(telebot_json_get_string(tmp));
    if (telebot_json_get_ex(obj, "tag", &tmp))
        member->tag = TELEBOT_SAFE_STRDUP(telebot_json_get_string(tmp));
    if (telebot_json_get_ex(obj, "until_date", &tmp))
        member->until_date = telebot_json_get_int(tmp);
    if (telebot_json_get_ex(obj, "is_anonymous", &tmp))
        member->is_anonymous = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "can_be_edited", &tmp))
        member->can_be_edited = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "is_member", &tmp))
        member->is_member = telebot_json_get_bool(tmp);

    /* Rights and permissions are flat boolean fields of the ChatMember object */
    for (size_t f = 0; f < TELEBOT_FIELD_COUNT(telebot_chat_administrator_rights_fields); f++)
    {
        if (telebot_json_get_ex(obj, telebot_chat_administrator_rights_fields[f].key, &tmp))
            *telebot_bool_field_ptr(&member->rights, &telebot_chat_administrator_rights_fields[f]) =
                telebot_json_get_bool(tmp);
    }
    for (size_t f = 0; f < TELEBOT_FIELD_COUNT(telebot_chat_permissions_fields); f++)
    {
        if (telebot_json_get_ex(obj, telebot_chat_permissions_fields[f].key, &tmp))
            *telebot_bool_field_ptr(&member->permissions, &telebot_chat_permissions_fields[f]) =
                telebot_json_get_bool(tmp);
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
        if (telebot_json_get_ex(obj, telebot_chat_permissions_fields[i].key, &value))
            *telebot_bool_field_ptr(permissions, &telebot_chat_permissions_fields[i]) = telebot_json_get_bool(value);
    }
    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_bot_command(struct cJSON *obj, telebot_bot_command_t *botcmd)
{
    if ((obj == NULL) || (botcmd == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(botcmd, 0, sizeof(telebot_bot_command_t));
    struct cJSON *command = NULL;
    if (!telebot_json_get_ex(obj, "command", &command))
    {
        ERR("Object is not bot command type, command not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    botcmd->command = TELEBOT_SAFE_STRDUP(telebot_json_get_string(command));

    struct cJSON *description = NULL;
    if (!telebot_json_get_ex(obj, "description", &description))
    {
        ERR("Object is not bot command type, description not found");
        TELEBOT_SAFE_FREE(botcmd->command);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    botcmd->description = TELEBOT_SAFE_STRDUP(telebot_json_get_string(description));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_array_bot_command(struct cJSON *obj, telebot_bot_command_t **cmds, int *count)
{
    int ret = TELEBOT_ERROR_NONE;
    if ((obj == NULL) || (cmds == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct cJSON *array = obj;
    int array_len = telebot_json_array_length(array);
    if (!array_len)
        return TELEBOT_ERROR_OPERATION_FAILED;

    telebot_bot_command_t *result = telebot_owned_calloc(array_len, sizeof(telebot_bot_command_t));
    if (result == NULL)
        return TELEBOT_ERROR_OUT_OF_MEMORY;

    *count = array_len;
    *cmds = result;

    int index = 0;
    for (index = 0; index < array_len; index++)
    {
        struct cJSON *item = telebot_json_array_get_idx(array, index);
        ret = telebot_parser_get_bot_command(item, &result[index]);
        if (ret != TELEBOT_ERROR_NONE)
            break;

    } /* for index */

    if (ret)
    {
        *cmds = NULL; /* the arena holding `result` is discarded by the caller */
        *count = 0;
    }

    return ret;
}

telebot_error_e telebot_parser_get_chat_location(struct cJSON *obj, telebot_chat_location_t *chat_location)
{
    if ((obj == NULL) || (chat_location == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct cJSON *address = NULL;
    if (!telebot_json_get_ex(obj, "address", &address))
    {
        ERR("Object is not chat type, address not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    chat_location->address = TELEBOT_SAFE_STRDUP(telebot_json_get_string(address));

    struct cJSON *location = NULL;
    if (telebot_json_get_ex(obj, "location", &location))
    {
        chat_location->location = telebot_mem_malloc(sizeof(telebot_location_t));
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
    if (!telebot_json_get_ex(obj, "message_auto_delete_time", &message_auto_delete_time))
    {
        ERR("Object is not message auto-delete timer type, message_auto_delete_time not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    timer_changed->message_auto_delete_time = telebot_json_get_int(message_auto_delete_time);

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_invoice_fields[] = {
    TELEBOT_JSON_FIELD("title", telebot_invoice_t, title, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("description", telebot_invoice_t, description, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("start_parameter", telebot_invoice_t, start_parameter, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("currency", telebot_invoice_t, currency, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("total_amount", telebot_invoice_t, total_amount, TELEBOT_FIELD_INT),
};

telebot_error_e telebot_parser_get_invoice(struct cJSON *obj, telebot_invoice_t *invoice)
{
    if ((obj == NULL) || (invoice == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(invoice, 0, sizeof(telebot_invoice_t));
    telebot_parse_fields(obj, invoice, telebot_parser_get_invoice_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_invoice_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_shipping_address_fields[] = {
    TELEBOT_JSON_FIELD("country_code", telebot_shipping_address_t, country_code, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("state", telebot_shipping_address_t, state, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("city", telebot_shipping_address_t, city, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("street_line1", telebot_shipping_address_t, street_line1, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("street_line2", telebot_shipping_address_t, street_line2, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("post_code", telebot_shipping_address_t, post_code, TELEBOT_FIELD_STRING),
};

telebot_error_e telebot_parser_get_shipping_address(struct cJSON *obj, telebot_shipping_address_t *address)
{
    if ((obj == NULL) || (address == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(address, 0, sizeof(telebot_shipping_address_t));
    telebot_parse_fields(obj, address, telebot_parser_get_shipping_address_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_shipping_address_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_order_info_fields[] = {
    TELEBOT_JSON_FIELD("name", telebot_order_info_t, name, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("phone_number", telebot_order_info_t, phone_number, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("email", telebot_order_info_t, email, TELEBOT_FIELD_STRING),
};

static const telebot_json_field_t telebot_parser_get_order_info_objects[] = {
    TELEBOT_JSON_OBJECT("shipping_address", telebot_order_info_t, shipping_address, TELEBOT_OBJECT_SHIPPING_ADDRESS),
};

telebot_error_e telebot_parser_get_order_info(struct cJSON *obj, telebot_order_info_t *info)
{
    if ((obj == NULL) || (info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(info, 0, sizeof(telebot_order_info_t));
    telebot_parse_fields(obj, info, telebot_parser_get_order_info_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_order_info_fields));

    telebot_parse_fields(obj, info, telebot_parser_get_order_info_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_order_info_objects));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_successful_payment_fields[] = {
    TELEBOT_JSON_FIELD("currency", telebot_successful_payment_t, currency, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("total_amount", telebot_successful_payment_t, total_amount, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("invoice_payload", telebot_successful_payment_t, invoice_payload, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("shipping_option_id", telebot_successful_payment_t, shipping_option_id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("telegram_payment_charge_id", telebot_successful_payment_t, telegram_payment_charge_id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("provider_payment_charge_id", telebot_successful_payment_t, provider_payment_charge_id, TELEBOT_FIELD_STRING),
};

static const telebot_json_field_t telebot_parser_get_successful_payment_objects[] = {
    TELEBOT_JSON_OBJECT("order_info", telebot_successful_payment_t, order_info, TELEBOT_OBJECT_ORDER_INFO),
};

telebot_error_e telebot_parser_get_successful_payment(struct cJSON *obj, telebot_successful_payment_t *payment)
{
    if ((obj == NULL) || (payment == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(payment, 0, sizeof(telebot_successful_payment_t));
    telebot_parse_fields(obj, payment, telebot_parser_get_successful_payment_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_successful_payment_fields));

    telebot_parse_fields(obj, payment, telebot_parser_get_successful_payment_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_successful_payment_objects));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_passport_file_fields[] = {
    TELEBOT_JSON_FIELD("file_id", telebot_passport_file_t, file_id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("file_unique_id", telebot_passport_file_t, file_unique_id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("file_size", telebot_passport_file_t, file_size, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("date", telebot_passport_file_t, date, TELEBOT_FIELD_LONG),
};

telebot_error_e telebot_parser_get_passport_file(struct cJSON *obj, telebot_passport_file_t *file)
{
    if ((obj == NULL) || (file == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(file, 0, sizeof(telebot_passport_file_t));
    telebot_parse_fields(obj, file, telebot_parser_get_passport_file_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_passport_file_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_encrypted_passport_element_fields[] = {
    TELEBOT_JSON_FIELD("type", telebot_encrypted_passport_element_t, type, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("data", telebot_encrypted_passport_element_t, data, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("phone_number", telebot_encrypted_passport_element_t, phone_number, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("email", telebot_encrypted_passport_element_t, email, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("hash", telebot_encrypted_passport_element_t, hash, TELEBOT_FIELD_STRING),
};

static const telebot_json_field_t telebot_parser_get_encrypted_passport_element_objects[] = {
    TELEBOT_JSON_OBJECT("front_side", telebot_encrypted_passport_element_t, front_side, TELEBOT_OBJECT_PASSPORT_FILE),
    TELEBOT_JSON_OBJECT("reverse_side", telebot_encrypted_passport_element_t, reverse_side, TELEBOT_OBJECT_PASSPORT_FILE),
    TELEBOT_JSON_OBJECT("selfie", telebot_encrypted_passport_element_t, selfie, TELEBOT_OBJECT_PASSPORT_FILE),
};

telebot_error_e telebot_parser_get_encrypted_passport_element(struct cJSON *obj, telebot_encrypted_passport_element_t *element)
{
    if ((obj == NULL) || (element == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(element, 0, sizeof(telebot_encrypted_passport_element_t));
    telebot_parse_fields(obj, element, telebot_parser_get_encrypted_passport_element_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_encrypted_passport_element_fields));

    struct cJSON *files = NULL;
    if (telebot_json_get_ex(obj, "files", &files))
    {
        int array_len = telebot_json_array_length(files);
        element->count_files = array_len;
        element->files = telebot_mem_calloc(array_len, sizeof(telebot_passport_file_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_passport_file(telebot_json_array_get_idx(files, i), &(element->files[i]));
    }

    telebot_parse_fields(obj, element, telebot_parser_get_encrypted_passport_element_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_encrypted_passport_element_objects));

    struct cJSON *translation = NULL;
    if (telebot_json_get_ex(obj, "translation", &translation))
    {
        int array_len = telebot_json_array_length(translation);
        element->count_translation = array_len;
        element->translation = telebot_mem_calloc(array_len, sizeof(telebot_passport_file_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_passport_file(telebot_json_array_get_idx(translation, i), &(element->translation[i]));
    }

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_encrypted_credentials_fields[] = {
    TELEBOT_JSON_FIELD("data", telebot_encrypted_credentials_t, data, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("hash", telebot_encrypted_credentials_t, hash, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("secret", telebot_encrypted_credentials_t, secret, TELEBOT_FIELD_STRING),
};

telebot_error_e telebot_parser_get_encrypted_credentials(struct cJSON *obj, telebot_encrypted_credentials_t *credentials)
{
    if ((obj == NULL) || (credentials == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(credentials, 0, sizeof(telebot_encrypted_credentials_t));
    telebot_parse_fields(obj, credentials, telebot_parser_get_encrypted_credentials_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_encrypted_credentials_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_passport_data_objects[] = {
    TELEBOT_JSON_OBJECT("credentials", telebot_passport_data_t, credentials, TELEBOT_OBJECT_ENCRYPTED_CREDENTIALS),
};

telebot_error_e telebot_parser_get_passport_data(struct cJSON *obj, telebot_passport_data_t *passport_data)
{
    if ((obj == NULL) || (passport_data == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(passport_data, 0, sizeof(telebot_passport_data_t));
    struct cJSON *data = NULL;
    if (telebot_json_get_ex(obj, "data", &data))
    {
        int array_len = telebot_json_array_length(data);
        passport_data->count_data = array_len;
        passport_data->data = telebot_mem_calloc(array_len, sizeof(telebot_encrypted_passport_element_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_encrypted_passport_element(telebot_json_array_get_idx(data, i), &(passport_data->data[i]));
    }

    telebot_parse_fields(obj, passport_data, telebot_parser_get_passport_data_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_passport_data_objects));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_proximity_alert_triggered_fields[] = {
    TELEBOT_JSON_FIELD("distance", telebot_proximity_alert_triggered_t, distance, TELEBOT_FIELD_INT),
};

static const telebot_json_field_t telebot_parser_get_proximity_alert_triggered_objects[] = {
    TELEBOT_JSON_OBJECT("traveler", telebot_proximity_alert_triggered_t, traveler, TELEBOT_OBJECT_USER),
    TELEBOT_JSON_OBJECT("watcher", telebot_proximity_alert_triggered_t, watcher, TELEBOT_OBJECT_USER),
};

telebot_error_e telebot_parser_get_proximity_alert_triggered(struct cJSON *obj, telebot_proximity_alert_triggered_t *alert)
{
    if ((obj == NULL) || (alert == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(alert, 0, sizeof(telebot_proximity_alert_triggered_t));
    telebot_parse_fields(obj, alert, telebot_parser_get_proximity_alert_triggered_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_proximity_alert_triggered_objects));

    telebot_parse_fields(obj, alert, telebot_parser_get_proximity_alert_triggered_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_proximity_alert_triggered_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_forum_topic_created_fields[] = {
    TELEBOT_JSON_FIELD("name", telebot_forum_topic_created_t, name, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("icon_color", telebot_forum_topic_created_t, icon_color, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("icon_custom_emoji_id", telebot_forum_topic_created_t, icon_custom_emoji_id, TELEBOT_FIELD_STRING),
};

telebot_error_e telebot_parser_get_forum_topic_created(struct cJSON *obj, telebot_forum_topic_created_t *topic)
{
    if ((obj == NULL) || (topic == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(topic, 0, sizeof(telebot_forum_topic_created_t));
    telebot_parse_fields(obj, topic, telebot_parser_get_forum_topic_created_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_forum_topic_created_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_forum_topic_edited_fields[] = {
    TELEBOT_JSON_FIELD("name", telebot_forum_topic_edited_t, name, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("icon_custom_emoji_id", telebot_forum_topic_edited_t, icon_custom_emoji_id, TELEBOT_FIELD_STRING),
};

telebot_error_e telebot_parser_get_forum_topic_edited(struct cJSON *obj, telebot_forum_topic_edited_t *topic)
{
    if ((obj == NULL) || (topic == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(topic, 0, sizeof(telebot_forum_topic_edited_t));
    telebot_parse_fields(obj, topic, telebot_parser_get_forum_topic_edited_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_forum_topic_edited_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_video_chat_scheduled_fields[] = {
    TELEBOT_JSON_FIELD("start_date", telebot_video_chat_scheduled_t, start_date, TELEBOT_FIELD_LONG),
};

telebot_error_e telebot_parser_get_video_chat_scheduled(struct cJSON *obj, telebot_video_chat_scheduled_t *scheduled)
{
    if ((obj == NULL) || (scheduled == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(scheduled, 0, sizeof(telebot_video_chat_scheduled_t));
    telebot_parse_fields(obj, scheduled, telebot_parser_get_video_chat_scheduled_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_video_chat_scheduled_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_video_chat_ended_fields[] = {
    TELEBOT_JSON_FIELD("duration", telebot_video_chat_ended_t, duration, TELEBOT_FIELD_INT),
};

telebot_error_e telebot_parser_get_video_chat_ended(struct cJSON *obj, telebot_video_chat_ended_t *ended)
{
    if ((obj == NULL) || (ended == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(ended, 0, sizeof(telebot_video_chat_ended_t));
    telebot_parse_fields(obj, ended, telebot_parser_get_video_chat_ended_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_video_chat_ended_fields));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_video_chat_participants_invited(struct cJSON *obj, telebot_video_chat_participants_invited_t *invited)
{
    if ((obj == NULL) || (invited == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(invited, 0, sizeof(telebot_video_chat_participants_invited_t));
    struct cJSON *users = NULL;
    if (telebot_json_get_ex(obj, "users", &users))
    {
        int array_len = telebot_json_array_length(users);
        invited->count_users = array_len;
        invited->users = telebot_mem_calloc(array_len, sizeof(telebot_user_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_user(telebot_json_array_get_idx(users, i), &(invited->users[i]));
    }

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_web_app_data_fields[] = {
    TELEBOT_JSON_FIELD("data", telebot_web_app_data_t, data, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("button_text", telebot_web_app_data_t, button_text, TELEBOT_FIELD_STRING),
};

telebot_error_e telebot_parser_get_web_app_data(struct cJSON *obj, telebot_web_app_data_t *data)
{
    if ((obj == NULL) || (data == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(data, 0, sizeof(telebot_web_app_data_t));
    telebot_parse_fields(obj, data, telebot_parser_get_web_app_data_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_web_app_data_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_inline_keyboard_button_fields[] = {
    TELEBOT_JSON_FIELD("text", telebot_inline_keyboard_button_t, text, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("url", telebot_inline_keyboard_button_t, url, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("callback_data", telebot_inline_keyboard_button_t, callback_data, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("switch_inline_query", telebot_inline_keyboard_button_t, switch_inline_query, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("switch_inline_query_current_chat", telebot_inline_keyboard_button_t, switch_inline_query_current_chat, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("pay", telebot_inline_keyboard_button_t, pay, TELEBOT_FIELD_BOOL),
};

telebot_error_e telebot_parser_get_inline_keyboard_button(struct cJSON *obj, telebot_inline_keyboard_button_t *button)
{
    if ((obj == NULL) || (button == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(button, 0, sizeof(telebot_inline_keyboard_button_t));
    telebot_parse_fields(obj, button, telebot_parser_get_inline_keyboard_button_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_inline_keyboard_button_fields));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_inline_keyboard_markup(struct cJSON *obj, telebot_inline_keyboard_markup_t *markup)
{
    if ((obj == NULL) || (markup == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(markup, 0, sizeof(telebot_inline_keyboard_markup_t));
    struct cJSON *inline_keyboard = NULL;
    if (telebot_json_get_ex(obj, "inline_keyboard", &inline_keyboard))
    {
        int rows = telebot_json_array_length(inline_keyboard);
        int cols = 0;
        if (rows > 0)
        {
            struct cJSON *row0 = telebot_json_array_get_idx(inline_keyboard, 0);
            cols = telebot_json_array_length(row0);
        }
        markup->rows = rows;
        markup->cols = cols;
        markup->inline_keyboard = telebot_mem_calloc(rows * cols, sizeof(telebot_inline_keyboard_button_t));
        for (int i = 0; i < rows; i++)
        {
            struct cJSON *row = telebot_json_array_get_idx(inline_keyboard, i);
            int row_cols = telebot_json_array_length(row);
            for (int j = 0; j < row_cols && j < cols; j++)
            {
                telebot_parser_get_inline_keyboard_button(telebot_json_array_get_idx(row, j), &(markup->inline_keyboard[i * cols + j]));
            }
        }
    }

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_chat_invite_link_fields[] = {
    TELEBOT_JSON_FIELD("invite_link", telebot_chat_invite_link_t, invite_link, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("creates_join_request", telebot_chat_invite_link_t, creates_join_request, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("is_primary", telebot_chat_invite_link_t, is_primary, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("is_revoked", telebot_chat_invite_link_t, is_revoked, TELEBOT_FIELD_BOOL),
    TELEBOT_JSON_FIELD("name", telebot_chat_invite_link_t, name, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("expire_date", telebot_chat_invite_link_t, expire_date, TELEBOT_FIELD_LONG),
    TELEBOT_JSON_FIELD("member_limit", telebot_chat_invite_link_t, member_limit, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("pending_join_request_count", telebot_chat_invite_link_t, pending_join_request_count, TELEBOT_FIELD_INT),
};

static const telebot_json_field_t telebot_parser_get_chat_invite_link_objects[] = {
    TELEBOT_JSON_OBJECT("creator", telebot_chat_invite_link_t, creator, TELEBOT_OBJECT_USER),
};

telebot_error_e telebot_parser_get_chat_invite_link(struct cJSON *obj, telebot_chat_invite_link_t *invite_link)
{
    if ((obj == NULL) || (invite_link == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(invite_link, 0, sizeof(telebot_chat_invite_link_t));
    telebot_parse_fields(obj, invite_link, telebot_parser_get_chat_invite_link_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_chat_invite_link_fields));

    telebot_parse_fields(obj, invite_link, telebot_parser_get_chat_invite_link_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_chat_invite_link_objects));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_chat_member_updated_fields[] = {
    TELEBOT_JSON_FIELD("date", telebot_chat_member_updated_t, date, TELEBOT_FIELD_LONG),
    TELEBOT_JSON_FIELD("via_chat_folder_invite_link", telebot_chat_member_updated_t, via_chat_folder_invite_link, TELEBOT_FIELD_BOOL),
};

static const telebot_json_field_t telebot_parser_get_chat_member_updated_objects[] = {
    TELEBOT_JSON_OBJECT("chat", telebot_chat_member_updated_t, chat, TELEBOT_OBJECT_CHAT),
    TELEBOT_JSON_OBJECT("from", telebot_chat_member_updated_t, from, TELEBOT_OBJECT_USER),
    TELEBOT_JSON_OBJECT("old_chat_member", telebot_chat_member_updated_t, old_chat_member, TELEBOT_OBJECT_CHAT_MEMBER),
    TELEBOT_JSON_OBJECT("new_chat_member", telebot_chat_member_updated_t, new_chat_member, TELEBOT_OBJECT_CHAT_MEMBER),
    TELEBOT_JSON_OBJECT("invite_link", telebot_chat_member_updated_t, invite_link, TELEBOT_OBJECT_CHAT_INVITE_LINK),
};

telebot_error_e telebot_parser_get_chat_member_updated(struct cJSON *obj, telebot_chat_member_updated_t *updated)
{
    if ((obj == NULL) || (updated == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(updated, 0, sizeof(telebot_chat_member_updated_t));
    telebot_parse_fields(obj, updated, telebot_parser_get_chat_member_updated_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_chat_member_updated_objects));

    telebot_parse_fields(obj, updated, telebot_parser_get_chat_member_updated_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_chat_member_updated_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_chat_join_request_fields[] = {
    TELEBOT_JSON_FIELD("user_chat_id", telebot_chat_join_request_t, user_chat_id, TELEBOT_FIELD_LONG_LONG),
    TELEBOT_JSON_FIELD("date", telebot_chat_join_request_t, date, TELEBOT_FIELD_LONG),
    TELEBOT_JSON_FIELD("bio", telebot_chat_join_request_t, bio, TELEBOT_FIELD_STRING),
};

static const telebot_json_field_t telebot_parser_get_chat_join_request_objects[] = {
    TELEBOT_JSON_OBJECT("chat", telebot_chat_join_request_t, chat, TELEBOT_OBJECT_CHAT),
    TELEBOT_JSON_OBJECT("from", telebot_chat_join_request_t, from, TELEBOT_OBJECT_USER),
    TELEBOT_JSON_OBJECT("invite_link", telebot_chat_join_request_t, invite_link, TELEBOT_OBJECT_CHAT_INVITE_LINK),
};

telebot_error_e telebot_parser_get_chat_join_request(struct cJSON *obj, telebot_chat_join_request_t *request)
{
    if ((obj == NULL) || (request == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(request, 0, sizeof(telebot_chat_join_request_t));
    telebot_parse_fields(obj, request, telebot_parser_get_chat_join_request_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_chat_join_request_objects));

    telebot_parse_fields(obj, request, telebot_parser_get_chat_join_request_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_chat_join_request_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_reaction_type_fields[] = {
    TELEBOT_JSON_FIELD("type", telebot_reaction_type_t, type, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("emoji", telebot_reaction_type_t, emoji, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("custom_emoji_id", telebot_reaction_type_t, custom_emoji_id, TELEBOT_FIELD_STRING),
};

telebot_error_e telebot_parser_get_reaction_type(struct cJSON *obj, telebot_reaction_type_t *reaction)
{
    if ((obj == NULL) || (reaction == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(reaction, 0, sizeof(telebot_reaction_type_t));
    telebot_parse_fields(obj, reaction, telebot_parser_get_reaction_type_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_reaction_type_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_message_reaction_updated_fields[] = {
    TELEBOT_JSON_FIELD("message_id", telebot_message_reaction_updated_t, message_id, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("date", telebot_message_reaction_updated_t, date, TELEBOT_FIELD_LONG),
};

static const telebot_json_field_t telebot_parser_get_message_reaction_updated_objects[] = {
    TELEBOT_JSON_OBJECT("chat", telebot_message_reaction_updated_t, chat, TELEBOT_OBJECT_CHAT),
    TELEBOT_JSON_OBJECT("user", telebot_message_reaction_updated_t, user, TELEBOT_OBJECT_USER),
    TELEBOT_JSON_OBJECT("actor_chat", telebot_message_reaction_updated_t, actor_chat, TELEBOT_OBJECT_CHAT),
};

telebot_error_e telebot_parser_get_message_reaction_updated(struct cJSON *obj, telebot_message_reaction_updated_t *updated)
{
    if ((obj == NULL) || (updated == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(updated, 0, sizeof(telebot_message_reaction_updated_t));
    telebot_parse_fields(obj, updated, telebot_parser_get_message_reaction_updated_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_message_reaction_updated_objects));

    telebot_parse_fields(obj, updated, telebot_parser_get_message_reaction_updated_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_message_reaction_updated_fields));

    struct cJSON *old_reaction = NULL;
    if (telebot_json_get_ex(obj, "old_reaction", &old_reaction))
    {
        int array_len = telebot_json_array_length(old_reaction);
        updated->count_old_reaction = array_len;
        updated->old_reaction = telebot_mem_calloc(array_len, sizeof(telebot_reaction_type_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_reaction_type(telebot_json_array_get_idx(old_reaction, i), &(updated->old_reaction[i]));
    }

    struct cJSON *new_reaction = NULL;
    if (telebot_json_get_ex(obj, "new_reaction", &new_reaction))
    {
        int array_len = telebot_json_array_length(new_reaction);
        updated->count_new_reaction = array_len;
        updated->new_reaction = telebot_mem_calloc(array_len, sizeof(telebot_reaction_type_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_reaction_type(telebot_json_array_get_idx(new_reaction, i), &(updated->new_reaction[i]));
    }

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_reaction_count_fields[] = {
    TELEBOT_JSON_FIELD("total_count", telebot_reaction_count_t, total_count, TELEBOT_FIELD_INT),
};

telebot_error_e telebot_parser_get_reaction_count(struct cJSON *obj, telebot_reaction_count_t *count)
{
    if ((obj == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(count, 0, sizeof(telebot_reaction_count_t));
    struct cJSON *type = NULL;
    if (telebot_json_get_ex(obj, "type", &type))
        telebot_parser_get_reaction_type(type, &(count->type));

    telebot_parse_fields(obj, count, telebot_parser_get_reaction_count_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_reaction_count_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_message_reaction_count_updated_fields[] = {
    TELEBOT_JSON_FIELD("message_id", telebot_message_reaction_count_updated_t, message_id, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("date", telebot_message_reaction_count_updated_t, date, TELEBOT_FIELD_LONG),
};

static const telebot_json_field_t telebot_parser_get_message_reaction_count_updated_objects[] = {
    TELEBOT_JSON_OBJECT("chat", telebot_message_reaction_count_updated_t, chat, TELEBOT_OBJECT_CHAT),
};

telebot_error_e telebot_parser_get_message_reaction_count_updated(struct cJSON *obj, telebot_message_reaction_count_updated_t *updated)
{
    if ((obj == NULL) || (updated == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(updated, 0, sizeof(telebot_message_reaction_count_updated_t));
    telebot_parse_fields(obj, updated, telebot_parser_get_message_reaction_count_updated_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_message_reaction_count_updated_objects));

    telebot_parse_fields(obj, updated, telebot_parser_get_message_reaction_count_updated_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_message_reaction_count_updated_fields));

    struct cJSON *reactions = NULL;
    if (telebot_json_get_ex(obj, "reactions", &reactions))
    {
        int array_len = telebot_json_array_length(reactions);
        updated->count_reactions = array_len;
        updated->reactions = telebot_mem_calloc(array_len, sizeof(telebot_reaction_count_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_reaction_count(telebot_json_array_get_idx(reactions, i), &(updated->reactions[i]));
    }

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_chat_boost_source_fields[] = {
    TELEBOT_JSON_FIELD("source", telebot_chat_boost_source_t, source, TELEBOT_FIELD_STRING),
};

static const telebot_json_field_t telebot_parser_get_chat_boost_source_objects[] = {
    TELEBOT_JSON_OBJECT("user", telebot_chat_boost_source_t, user, TELEBOT_OBJECT_USER),
};

telebot_error_e telebot_parser_get_chat_boost_source(struct cJSON *obj, telebot_chat_boost_source_t *source)
{
    if ((obj == NULL) || (source == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(source, 0, sizeof(telebot_chat_boost_source_t));
    telebot_parse_fields(obj, source, telebot_parser_get_chat_boost_source_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_chat_boost_source_fields));

    telebot_parse_fields(obj, source, telebot_parser_get_chat_boost_source_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_chat_boost_source_objects));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_chat_boost_fields[] = {
    TELEBOT_JSON_FIELD("boost_id", telebot_chat_boost_t, boost_id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("add_date", telebot_chat_boost_t, add_date, TELEBOT_FIELD_LONG),
    TELEBOT_JSON_FIELD("expiration_date", telebot_chat_boost_t, expiration_date, TELEBOT_FIELD_LONG),
};

static const telebot_json_field_t telebot_parser_get_chat_boost_objects[] = {
    TELEBOT_JSON_OBJECT("source", telebot_chat_boost_t, source, TELEBOT_OBJECT_CHAT_BOOST_SOURCE),
};

telebot_error_e telebot_parser_get_chat_boost(struct cJSON *obj, telebot_chat_boost_t *boost)
{
    if ((obj == NULL) || (boost == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(boost, 0, sizeof(telebot_chat_boost_t));
    telebot_parse_fields(obj, boost, telebot_parser_get_chat_boost_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_chat_boost_fields));

    telebot_parse_fields(obj, boost, telebot_parser_get_chat_boost_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_chat_boost_objects));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_chat_boost_updated_objects[] = {
    TELEBOT_JSON_OBJECT("chat", telebot_chat_boost_updated_t, chat, TELEBOT_OBJECT_CHAT),
    TELEBOT_JSON_OBJECT("boost", telebot_chat_boost_updated_t, boost, TELEBOT_OBJECT_CHAT_BOOST),
};

telebot_error_e telebot_parser_get_chat_boost_updated(struct cJSON *obj, telebot_chat_boost_updated_t *updated)
{
    if ((obj == NULL) || (updated == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(updated, 0, sizeof(telebot_chat_boost_updated_t));
    telebot_parse_fields(obj, updated, telebot_parser_get_chat_boost_updated_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_chat_boost_updated_objects));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_chat_boost_removed_fields[] = {
    TELEBOT_JSON_FIELD("boost_id", telebot_chat_boost_removed_t, boost_id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("remove_date", telebot_chat_boost_removed_t, remove_date, TELEBOT_FIELD_LONG),
};

static const telebot_json_field_t telebot_parser_get_chat_boost_removed_objects[] = {
    TELEBOT_JSON_OBJECT("chat", telebot_chat_boost_removed_t, chat, TELEBOT_OBJECT_CHAT),
    TELEBOT_JSON_OBJECT("source", telebot_chat_boost_removed_t, source, TELEBOT_OBJECT_CHAT_BOOST_SOURCE),
};

telebot_error_e telebot_parser_get_chat_boost_removed(struct cJSON *obj, telebot_chat_boost_removed_t *removed)
{
    if ((obj == NULL) || (removed == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(removed, 0, sizeof(telebot_chat_boost_removed_t));
    telebot_parse_fields(obj, removed, telebot_parser_get_chat_boost_removed_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_chat_boost_removed_objects));

    telebot_parse_fields(obj, removed, telebot_parser_get_chat_boost_removed_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_chat_boost_removed_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_forum_topic_fields[] = {
    TELEBOT_JSON_FIELD("message_thread_id", telebot_forum_topic_t, message_thread_id, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("name", telebot_forum_topic_t, name, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("icon_color", telebot_forum_topic_t, icon_color, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("icon_custom_emoji_id", telebot_forum_topic_t, icon_custom_emoji_id, TELEBOT_FIELD_STRING),
};

telebot_error_e telebot_parser_get_forum_topic(struct cJSON *obj, telebot_forum_topic_t *topic)
{
    if ((obj == NULL) || (topic == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(topic, 0, sizeof(telebot_forum_topic_t));
    telebot_parse_fields(obj, topic, telebot_parser_get_forum_topic_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_forum_topic_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_inline_query_fields[] = {
    TELEBOT_JSON_FIELD("id", telebot_inline_query_t, id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("query", telebot_inline_query_t, query, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("offset", telebot_inline_query_t, offset, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("chat_type", telebot_inline_query_t, chat_type, TELEBOT_FIELD_STRING),
};

static const telebot_json_field_t telebot_parser_get_inline_query_objects[] = {
    TELEBOT_JSON_OBJECT("from", telebot_inline_query_t, from, TELEBOT_OBJECT_USER),
    TELEBOT_JSON_OBJECT("location", telebot_inline_query_t, location, TELEBOT_OBJECT_LOCATION),
};

telebot_error_e telebot_parser_get_inline_query(struct cJSON *obj, telebot_inline_query_t *query)
{
    if ((obj == NULL) || (query == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(query, 0, sizeof(telebot_inline_query_t));
    telebot_parse_fields(obj, query, telebot_parser_get_inline_query_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_inline_query_fields));

    telebot_parse_fields(obj, query, telebot_parser_get_inline_query_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_inline_query_objects));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_chosen_inline_result_fields[] = {
    TELEBOT_JSON_FIELD("result_id", telebot_chosen_inline_result_t, result_id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("inline_message_id", telebot_chosen_inline_result_t, inline_message_id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("query", telebot_chosen_inline_result_t, query, TELEBOT_FIELD_STRING),
};

static const telebot_json_field_t telebot_parser_get_chosen_inline_result_objects[] = {
    TELEBOT_JSON_OBJECT("from", telebot_chosen_inline_result_t, from, TELEBOT_OBJECT_USER),
    TELEBOT_JSON_OBJECT("location", telebot_chosen_inline_result_t, location, TELEBOT_OBJECT_LOCATION),
};

telebot_error_e telebot_parser_get_chosen_inline_result(struct cJSON *obj, telebot_chosen_inline_result_t *result)
{
    if ((obj == NULL) || (result == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(result, 0, sizeof(telebot_chosen_inline_result_t));
    telebot_parse_fields(obj, result, telebot_parser_get_chosen_inline_result_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_chosen_inline_result_fields));

    telebot_parse_fields(obj, result, telebot_parser_get_chosen_inline_result_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_chosen_inline_result_objects));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_shipping_query_fields[] = {
    TELEBOT_JSON_FIELD("id", telebot_shipping_query_t, id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("invoice_payload", telebot_shipping_query_t, invoice_payload, TELEBOT_FIELD_STRING),
};

static const telebot_json_field_t telebot_parser_get_shipping_query_objects[] = {
    TELEBOT_JSON_OBJECT("from", telebot_shipping_query_t, from, TELEBOT_OBJECT_USER),
    TELEBOT_JSON_OBJECT("shipping_address", telebot_shipping_query_t, shipping_address, TELEBOT_OBJECT_SHIPPING_ADDRESS),
};

telebot_error_e telebot_parser_get_shipping_query(struct cJSON *obj, telebot_shipping_query_t *query)
{
    if ((obj == NULL) || (query == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(query, 0, sizeof(telebot_shipping_query_t));
    telebot_parse_fields(obj, query, telebot_parser_get_shipping_query_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_shipping_query_fields));

    telebot_parse_fields(obj, query, telebot_parser_get_shipping_query_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_shipping_query_objects));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_pre_checkout_query_fields[] = {
    TELEBOT_JSON_FIELD("id", telebot_pre_checkout_query_t, id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("currency", telebot_pre_checkout_query_t, currency, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("total_amount", telebot_pre_checkout_query_t, total_amount, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("invoice_payload", telebot_pre_checkout_query_t, invoice_payload, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("shipping_option_id", telebot_pre_checkout_query_t, shipping_option_id, TELEBOT_FIELD_STRING),
};

static const telebot_json_field_t telebot_parser_get_pre_checkout_query_objects[] = {
    TELEBOT_JSON_OBJECT("from", telebot_pre_checkout_query_t, from, TELEBOT_OBJECT_USER),
    TELEBOT_JSON_OBJECT("order_info", telebot_pre_checkout_query_t, order_info, TELEBOT_OBJECT_ORDER_INFO),
};

telebot_error_e telebot_parser_get_pre_checkout_query(struct cJSON *obj, telebot_pre_checkout_query_t *query)
{
    if ((obj == NULL) || (query == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(query, 0, sizeof(telebot_pre_checkout_query_t));
    telebot_parse_fields(obj, query, telebot_parser_get_pre_checkout_query_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_pre_checkout_query_fields));

    telebot_parse_fields(obj, query, telebot_parser_get_pre_checkout_query_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_pre_checkout_query_objects));

    return TELEBOT_ERROR_NONE;
}

static void telebot_parser_get_mask_position(struct cJSON *obj, telebot_mask_position_t *mask)
{
    memset(mask, 0, sizeof(telebot_mask_position_t));
    cJSON *tmp = NULL;
    if (telebot_json_get_ex(obj, "point", &tmp))
        mask->point = TELEBOT_SAFE_STRDUP(telebot_json_get_string(tmp));
    if (telebot_json_get_ex(obj, "x_shift", &tmp))
        mask->x_shift = (float)telebot_json_get_double(tmp);
    if (telebot_json_get_ex(obj, "y_shift", &tmp))
        mask->y_shift = (float)telebot_json_get_double(tmp);
    if (telebot_json_get_ex(obj, "scale", &tmp))
        mask->scale = (float)telebot_json_get_double(tmp);
}

telebot_error_e telebot_parser_get_sticker(struct cJSON *obj, telebot_sticker_t *sticker)
{
    if ((obj == NULL) || (sticker == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(sticker, 0, sizeof(telebot_sticker_t));
    cJSON *tmp = NULL;

    if (!telebot_json_get_ex(obj, "file_id", &tmp))
        return TELEBOT_ERROR_OPERATION_FAILED;
    sticker->file_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(tmp));

    if (telebot_json_get_ex(obj, "file_unique_id", &tmp))
        sticker->file_unique_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(tmp));
    if (telebot_json_get_ex(obj, "type", &tmp))
        sticker->type = TELEBOT_SAFE_STRDUP(telebot_json_get_string(tmp));
    if (telebot_json_get_ex(obj, "width", &tmp))
        sticker->width = telebot_json_get_int(tmp);
    if (telebot_json_get_ex(obj, "height", &tmp))
        sticker->height = telebot_json_get_int(tmp);
    if (telebot_json_get_ex(obj, "is_animated", &tmp))
        sticker->is_animated = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "is_video", &tmp))
        sticker->is_video = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "emoji", &tmp))
        sticker->emoji = TELEBOT_SAFE_STRDUP(telebot_json_get_string(tmp));
    if (telebot_json_get_ex(obj, "set_name", &tmp))
        sticker->set_name = TELEBOT_SAFE_STRDUP(telebot_json_get_string(tmp));
    if (telebot_json_get_ex(obj, "custom_emoji_id", &tmp))
        sticker->custom_emoji_id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(tmp));
    if (telebot_json_get_ex(obj, "needs_repainting", &tmp))
        sticker->needs_repainting = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "file_size", &tmp))
        sticker->file_size = telebot_json_get_int(tmp);

    if (telebot_json_get_ex(obj, "thumbnail", &tmp))
    {
        sticker->thumbnail = telebot_mem_calloc(1, sizeof(telebot_photo_t));
        if ((sticker->thumbnail != NULL) && (telebot_parser_get_photo(tmp, sticker->thumbnail) != TELEBOT_ERROR_NONE))
            TELEBOT_SAFE_FREE(sticker->thumbnail);
    }

    if (telebot_json_get_ex(obj, "mask_position", &tmp))
    {
        sticker->mask_position = telebot_mem_calloc(1, sizeof(telebot_mask_position_t));
        if (sticker->mask_position != NULL)
            telebot_parser_get_mask_position(tmp, sticker->mask_position);
    }

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_sticker_array(struct cJSON *obj, telebot_sticker_t **stickers, int *count,
                                                        bool owned)
{
    if ((obj == NULL) || (stickers == NULL) || (count == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    *stickers = NULL;
    *count = 0;
    int len = telebot_json_array_length(obj);
    if (len == 0)
        return TELEBOT_ERROR_NONE;

    telebot_sticker_t *result = owned ? telebot_owned_calloc(len, sizeof(telebot_sticker_t))
                                      : telebot_mem_calloc(len, sizeof(telebot_sticker_t));
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

telebot_error_e telebot_parser_get_stickers(struct cJSON *obj, telebot_sticker_t **stickers, int *count)
{
    return telebot_parser_get_sticker_array(obj, stickers, count, true);
}

telebot_error_e telebot_parser_get_sticker_set(struct cJSON *obj, telebot_sticker_set_t *set)
{
    if ((obj == NULL) || (set == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(set, 0, sizeof(telebot_sticker_set_t));
    cJSON *tmp = NULL;

    if (!telebot_json_get_ex(obj, "name", &tmp))
        return TELEBOT_ERROR_OPERATION_FAILED;
    set->name = TELEBOT_SAFE_STRDUP(telebot_json_get_string(tmp));

    if (telebot_json_get_ex(obj, "title", &tmp))
        set->title = TELEBOT_SAFE_STRDUP(telebot_json_get_string(tmp));
    if (telebot_json_get_ex(obj, "sticker_type", &tmp))
        set->sticker_type = TELEBOT_SAFE_STRDUP(telebot_json_get_string(tmp));
    if (telebot_json_get_ex(obj, "stickers", &tmp))
    {
        telebot_error_e ret = telebot_parser_get_sticker_array(tmp, &set->stickers, &set->count_stickers, false);
        if (ret != TELEBOT_ERROR_NONE)
            return ret;
    }
    if (telebot_json_get_ex(obj, "thumbnail", &tmp))
    {
        set->thumbnail = telebot_mem_calloc(1, sizeof(telebot_photo_t));
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
    if (telebot_json_get_ex(obj, "can_reply", &tmp))
        rights->can_reply = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "can_read_messages", &tmp))
        rights->can_read_messages = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "can_delete_sent_messages", &tmp))
        rights->can_delete_sent_messages = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "can_delete_all_messages", &tmp))
        rights->can_delete_all_messages = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "can_edit_name", &tmp))
        rights->can_edit_name = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "can_edit_bio", &tmp))
        rights->can_edit_bio = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "can_edit_profile_photo", &tmp))
        rights->can_edit_profile_photo = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "can_edit_username", &tmp))
        rights->can_edit_username = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "can_change_gift_settings", &tmp))
        rights->can_change_gift_settings = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "can_view_gifts_and_stars", &tmp))
        rights->can_view_gifts_and_stars = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "can_convert_gifts_to_stars", &tmp))
        rights->can_convert_gifts_to_stars = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "can_transfer_and_upgrade_gifts", &tmp))
        rights->can_transfer_and_upgrade_gifts = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "can_transfer_stars", &tmp))
        rights->can_transfer_stars = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "can_manage_stories", &tmp))
        rights->can_manage_stories = telebot_json_get_bool(tmp);

    return TELEBOT_ERROR_NONE;
}

#if TELEBOT_FEATURE_BUSINESS
static const telebot_json_field_t telebot_parser_get_business_messages_deleted_fields[] = {
    TELEBOT_JSON_FIELD("business_connection_id", telebot_business_messages_deleted_t, business_connection_id, TELEBOT_FIELD_STRING),
};

static telebot_error_e telebot_parser_get_business_messages_deleted(struct cJSON *obj, telebot_business_messages_deleted_t *deleted)
{
    if ((obj == NULL) || (deleted == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(deleted, 0, sizeof(telebot_business_messages_deleted_t));
    telebot_parse_fields(obj, deleted, telebot_parser_get_business_messages_deleted_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_business_messages_deleted_fields));

    struct cJSON *chat = NULL;
    if (telebot_json_get_ex(obj, "chat", &chat))
    {
        deleted->chat = telebot_mem_calloc(1, sizeof(telebot_chat_t));
        telebot_parser_get_chat(chat, deleted->chat);
    }

    struct cJSON *message_ids = NULL;
    if (telebot_json_get_ex(obj, "message_ids", &message_ids))
    {
        int array_len = telebot_json_array_length(message_ids);
        deleted->count_message_ids = array_len;
        deleted->message_ids = telebot_mem_calloc(array_len, sizeof(int));
        for (int i = 0; i < array_len; i++)
            deleted->message_ids[i] = telebot_json_get_int(telebot_json_array_get_idx(message_ids, i));
    }

    return TELEBOT_ERROR_NONE;
}
#endif

static const telebot_json_field_t telebot_parser_get_message_origin_fields[] = {
    TELEBOT_JSON_FIELD("type", telebot_message_origin_t, type, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("date", telebot_message_origin_t, date, TELEBOT_FIELD_LONG),
    TELEBOT_JSON_FIELD("sender_user_name", telebot_message_origin_t, sender_user_name, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("author_signature", telebot_message_origin_t, author_signature, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("message_id", telebot_message_origin_t, message_id, TELEBOT_FIELD_INT),
};

static const telebot_json_field_t telebot_parser_get_message_origin_objects[] = {
    TELEBOT_JSON_OBJECT("sender_user", telebot_message_origin_t, sender_user, TELEBOT_OBJECT_USER),
    TELEBOT_JSON_OBJECT("sender_chat", telebot_message_origin_t, sender_chat, TELEBOT_OBJECT_CHAT),
};

static telebot_error_e telebot_parser_get_message_origin(struct cJSON *obj, telebot_message_origin_t *origin)
{
    if ((obj == NULL) || (origin == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(origin, 0, sizeof(telebot_message_origin_t));
    telebot_parse_fields(obj, origin, telebot_parser_get_message_origin_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_message_origin_fields));

    telebot_parse_fields(obj, origin, telebot_parser_get_message_origin_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_message_origin_objects));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_text_quote_fields[] = {
    TELEBOT_JSON_FIELD("text", telebot_text_quote_t, text, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("position", telebot_text_quote_t, position, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("is_manual", telebot_text_quote_t, is_manual, TELEBOT_FIELD_BOOL),
};

static telebot_error_e telebot_parser_get_text_quote(struct cJSON *obj, telebot_text_quote_t *quote)
{
    if ((obj == NULL) || (quote == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(quote, 0, sizeof(telebot_text_quote_t));
    telebot_parse_fields(obj, quote, telebot_parser_get_text_quote_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_text_quote_fields));

    struct cJSON *entities = NULL;
    if (telebot_json_get_ex(obj, "entities", &entities))
        telebot_parser_get_message_entities(entities, &(quote->entities), &(quote->count_entities));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_story_fields[] = {
    TELEBOT_JSON_FIELD("id", telebot_story_t, id, TELEBOT_FIELD_INT),
};

static const telebot_json_field_t telebot_parser_get_story_objects[] = {
    TELEBOT_JSON_OBJECT("chat", telebot_story_t, chat, TELEBOT_OBJECT_CHAT),
};

static telebot_error_e telebot_parser_get_story(struct cJSON *obj, telebot_story_t *story)
{
    if ((obj == NULL) || (story == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(story, 0, sizeof(telebot_story_t));
    telebot_parse_fields(obj, story, telebot_parser_get_story_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_story_objects));

    telebot_parse_fields(obj, story, telebot_parser_get_story_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_story_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_external_reply_info_fields[] = {
    TELEBOT_JSON_FIELD("message_id", telebot_external_reply_info_t, message_id, TELEBOT_FIELD_INT),
};

static const telebot_json_field_t telebot_parser_get_external_reply_info_objects[] = {
    TELEBOT_JSON_OBJECT("chat", telebot_external_reply_info_t, chat, TELEBOT_OBJECT_CHAT),
};

static telebot_error_e telebot_parser_get_external_reply_info(struct cJSON *obj, telebot_external_reply_info_t *info)
{
    if ((obj == NULL) || (info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(info, 0, sizeof(telebot_external_reply_info_t));
    struct cJSON *origin = NULL;
    if (telebot_json_get_ex(obj, "origin", &origin))
        telebot_parser_get_message_origin(origin, &(info->origin));

    telebot_parse_fields(obj, info, telebot_parser_get_external_reply_info_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_external_reply_info_objects));

    telebot_parse_fields(obj, info, telebot_parser_get_external_reply_info_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_external_reply_info_fields));

    // ... other optional media fields can be added as needed ...

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_link_preview_options(struct cJSON *obj, telebot_link_preview_options_t *options)
{
    if ((obj == NULL) || (options == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(options, 0, sizeof(telebot_link_preview_options_t));
    struct cJSON *tmp = NULL;
    if (telebot_json_get_ex(obj, "is_disabled", &tmp))
        options->is_disabled = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "url", &tmp))
        options->url = TELEBOT_SAFE_STRDUP(telebot_json_get_string(tmp));
    if (telebot_json_get_ex(obj, "prefer_small_media", &tmp))
        options->prefer_small_media = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "prefer_large_media", &tmp))
        options->prefer_large_media = telebot_json_get_bool(tmp);
    if (telebot_json_get_ex(obj, "show_above_text", &tmp))
        options->show_above_text = telebot_json_get_bool(tmp);

    return TELEBOT_ERROR_NONE;
}

#if TELEBOT_FEATURE_PAYMENTS
static const telebot_json_field_t telebot_parser_get_paid_media_fields[] = {
    TELEBOT_JSON_FIELD("type", telebot_paid_media_t, type, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("width", telebot_paid_media_t, width, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("height", telebot_paid_media_t, height, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("duration", telebot_paid_media_t, duration, TELEBOT_FIELD_INT),
};

static telebot_error_e telebot_parser_get_paid_media(struct cJSON *obj, telebot_paid_media_t *media)
{
    if ((obj == NULL) || (media == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(media, 0, sizeof(telebot_paid_media_t));
    telebot_parse_fields(obj, media, telebot_parser_get_paid_media_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_paid_media_fields));

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_PAYMENTS
static const telebot_json_field_t telebot_parser_get_paid_media_info_fields[] = {
    TELEBOT_JSON_FIELD("star_count", telebot_paid_media_info_t, star_count, TELEBOT_FIELD_INT),
};

static telebot_error_e telebot_parser_get_paid_media_info(struct cJSON *obj, telebot_paid_media_info_t *info)
{
    if ((obj == NULL) || (info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(info, 0, sizeof(telebot_paid_media_info_t));
    telebot_parse_fields(obj, info, telebot_parser_get_paid_media_info_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_paid_media_info_fields));

    struct cJSON *paid_media = NULL;
    if (telebot_json_get_ex(obj, "paid_media", &paid_media))
    {
        int array_len = telebot_json_array_length(paid_media);
        info->count_paid_media = array_len;
        info->paid_media = telebot_mem_calloc(array_len, sizeof(telebot_paid_media_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_paid_media(telebot_json_array_get_idx(paid_media, i), &(info->paid_media[i]));
    }

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_PAYMENTS
static const telebot_json_field_t telebot_parser_get_refunded_payment_fields[] = {
    TELEBOT_JSON_FIELD("currency", telebot_refunded_payment_t, currency, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("total_amount", telebot_refunded_payment_t, total_amount, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("invoice_payload", telebot_refunded_payment_t, invoice_payload, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("telegram_payment_charge_id", telebot_refunded_payment_t, telegram_payment_charge_id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("provider_payment_charge_id", telebot_refunded_payment_t, provider_payment_charge_id, TELEBOT_FIELD_STRING),
};

static telebot_error_e telebot_parser_get_refunded_payment(struct cJSON *obj, telebot_refunded_payment_t *payment)
{
    if ((obj == NULL) || (payment == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(payment, 0, sizeof(telebot_refunded_payment_t));
    telebot_parse_fields(obj, payment, telebot_parser_get_refunded_payment_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_refunded_payment_fields));

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
static const telebot_json_field_t telebot_parser_get_write_access_allowed_fields[] = {
    TELEBOT_JSON_FIELD("web_app_name", telebot_write_access_allowed_t, web_app_name, TELEBOT_FIELD_STRING),
};

static telebot_error_e telebot_parser_get_write_access_allowed(struct cJSON *obj, telebot_write_access_allowed_t *allowed)
{
    if ((obj == NULL) || (allowed == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(allowed, 0, sizeof(telebot_write_access_allowed_t));
    telebot_parse_fields(obj, allowed, telebot_parser_get_write_access_allowed_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_write_access_allowed_fields));

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
static const telebot_json_field_t telebot_parser_get_shared_user_fields[] = {
    TELEBOT_JSON_FIELD("user_id", telebot_shared_user_t, user_id, TELEBOT_FIELD_LONG_LONG),
    TELEBOT_JSON_FIELD("first_name", telebot_shared_user_t, first_name, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("last_name", telebot_shared_user_t, last_name, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("username", telebot_shared_user_t, username, TELEBOT_FIELD_STRING),
};

static telebot_error_e telebot_parser_get_shared_user(struct cJSON *obj, telebot_shared_user_t *user)
{
    if ((obj == NULL) || (user == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(user, 0, sizeof(telebot_shared_user_t));
    telebot_parse_fields(obj, user, telebot_parser_get_shared_user_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_shared_user_fields));

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
static const telebot_json_field_t telebot_parser_get_users_shared_fields[] = {
    TELEBOT_JSON_FIELD("request_id", telebot_users_shared_t, request_id, TELEBOT_FIELD_INT),
};

static telebot_error_e telebot_parser_get_users_shared(struct cJSON *obj, telebot_users_shared_t *shared)
{
    if ((obj == NULL) || (shared == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(shared, 0, sizeof(telebot_users_shared_t));
    telebot_parse_fields(obj, shared, telebot_parser_get_users_shared_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_users_shared_fields));

    struct cJSON *users = NULL;
    if (telebot_json_get_ex(obj, "users", &users))
    {
        int array_len = telebot_json_array_length(users);
        shared->count_users = array_len;
        shared->users = telebot_mem_calloc(array_len, sizeof(telebot_shared_user_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_shared_user(telebot_json_array_get_idx(users, i), &(shared->users[i]));
    }

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
static const telebot_json_field_t telebot_parser_get_chat_shared_fields[] = {
    TELEBOT_JSON_FIELD("request_id", telebot_chat_shared_t, request_id, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("chat_id", telebot_chat_shared_t, chat_id, TELEBOT_FIELD_LONG_LONG),
};

static telebot_error_e telebot_parser_get_chat_shared(struct cJSON *obj, telebot_chat_shared_t *shared)
{
    if ((obj == NULL) || (shared == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(shared, 0, sizeof(telebot_chat_shared_t));
    telebot_parse_fields(obj, shared, telebot_parser_get_chat_shared_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_chat_shared_fields));

    return TELEBOT_ERROR_NONE;
}
#endif

static const telebot_json_field_t telebot_parser_get_gift_fields[] = {
    TELEBOT_JSON_FIELD("id", telebot_gift_t, id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("star_count", telebot_gift_t, star_count, TELEBOT_FIELD_INT),
};

static telebot_error_e telebot_parser_get_gift(struct cJSON *obj, telebot_gift_t *gift)
{
    if ((obj == NULL) || (gift == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gift, 0, sizeof(telebot_gift_t));
    telebot_parse_fields(obj, gift, telebot_parser_get_gift_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_gift_fields));

    return TELEBOT_ERROR_NONE;
}

#if TELEBOT_FEATURE_GIFTS
static const telebot_json_field_t telebot_parser_get_gift_info_fields[] = {
    TELEBOT_JSON_FIELD("text", telebot_gift_info_t, text, TELEBOT_FIELD_STRING),
};

static telebot_error_e telebot_parser_get_gift_info(struct cJSON *obj, telebot_gift_info_t *gift_info)
{
    if ((obj == NULL) || (gift_info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gift_info, 0, sizeof(telebot_gift_info_t));
    struct cJSON *gift = NULL;
    if (telebot_json_get_ex(obj, "gift", &gift))
        telebot_parser_get_gift(gift, &(gift_info->gift));

    telebot_parse_fields(obj, gift_info, telebot_parser_get_gift_info_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_gift_info_fields));

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_GIFTS
static const telebot_json_field_t telebot_parser_get_unique_gift_fields[] = {
    TELEBOT_JSON_FIELD("gift_id", telebot_unique_gift_t, gift_id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("name", telebot_unique_gift_t, name, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("number", telebot_unique_gift_t, number, TELEBOT_FIELD_INT),
};

static telebot_error_e telebot_parser_get_unique_gift(struct cJSON *obj, telebot_unique_gift_t *gift)
{
    if ((obj == NULL) || (gift == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gift, 0, sizeof(telebot_unique_gift_t));
    telebot_parse_fields(obj, gift, telebot_parser_get_unique_gift_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_unique_gift_fields));

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_GIFTS
static const telebot_json_field_t telebot_parser_get_unique_gift_info_fields[] = {
    TELEBOT_JSON_FIELD("origin", telebot_unique_gift_info_t, origin, TELEBOT_FIELD_STRING),
};

static telebot_error_e telebot_parser_get_unique_gift_info(struct cJSON *obj, telebot_unique_gift_info_t *gift_info)
{
    if ((obj == NULL) || (gift_info == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gift_info, 0, sizeof(telebot_unique_gift_info_t));
    struct cJSON *gift = NULL;
    if (telebot_json_get_ex(obj, "gift", &gift))
    {
        gift_info->gift = telebot_mem_calloc(1, sizeof(telebot_unique_gift_t));
        telebot_parser_get_unique_gift(gift, gift_info->gift);
    }

    telebot_parse_fields(obj, gift_info, telebot_parser_get_unique_gift_info_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_unique_gift_info_fields));

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_CHAT_EXTRAS
static const telebot_json_field_t telebot_parser_get_chat_boost_added_fields[] = {
    TELEBOT_JSON_FIELD("boost_count", telebot_chat_boost_added_t, boost_count, TELEBOT_FIELD_INT),
};

static telebot_error_e telebot_parser_get_chat_boost_added(struct cJSON *obj, telebot_chat_boost_added_t *boost)
{
    if ((obj == NULL) || (boost == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(boost, 0, sizeof(telebot_chat_boost_added_t));
    telebot_parse_fields(obj, boost, telebot_parser_get_chat_boost_added_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_chat_boost_added_fields));

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
    if (telebot_json_get_ex(obj, "type", &type))
    {
        struct cJSON *type_str = NULL;
        if (telebot_json_get_ex(type, "type", &type_str))
            background->type = TELEBOT_SAFE_STRDUP(telebot_json_get_string(type_str));
    }

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_GIFTS
static const telebot_json_field_t telebot_parser_get_giveaway_created_fields[] = {
    TELEBOT_JSON_FIELD("prize_star_count", telebot_giveaway_created_t, prize_star_count, TELEBOT_FIELD_INT),
};

static telebot_error_e telebot_parser_get_giveaway_created(struct cJSON *obj, telebot_giveaway_created_t *giveaway)
{
    if ((obj == NULL) || (giveaway == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(giveaway, 0, sizeof(telebot_giveaway_created_t));
    telebot_parse_fields(obj, giveaway, telebot_parser_get_giveaway_created_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_giveaway_created_fields));

    return TELEBOT_ERROR_NONE;
}
#endif

#if TELEBOT_FEATURE_GIFTS
static const telebot_json_field_t telebot_parser_get_giveaway_completed_fields[] = {
    TELEBOT_JSON_FIELD("winner_count", telebot_giveaway_completed_t, winner_count, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("unclaimed_prize_count", telebot_giveaway_completed_t, unclaimed_prize_count, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("is_star_giveaway", telebot_giveaway_completed_t, is_star_giveaway, TELEBOT_FIELD_BOOL),
};

static telebot_error_e telebot_parser_get_giveaway_completed(struct cJSON *obj, telebot_giveaway_completed_t *giveaway)
{
    if ((obj == NULL) || (giveaway == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(giveaway, 0, sizeof(telebot_giveaway_completed_t));
    telebot_parse_fields(obj, giveaway, telebot_parser_get_giveaway_completed_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_giveaway_completed_fields));

    struct cJSON *giveaway_message = NULL;
    if (telebot_json_get_ex(obj, "giveaway_message", &giveaway_message))
    {
        giveaway->giveaway_message = telebot_mem_calloc(1, sizeof(telebot_message_t));
        telebot_parser_get_message(giveaway_message, giveaway->giveaway_message);
    }

    return TELEBOT_ERROR_NONE;
}
#endif

static telebot_error_e telebot_parser_get_birthdate(struct cJSON *obj, telebot_birthdate_t *birthdate)
{
    if ((obj == NULL) || (birthdate == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(birthdate, 0, sizeof(telebot_birthdate_t));
    struct cJSON *tmp = NULL;
    if (telebot_json_get_ex(obj, "day", &tmp))
        birthdate->day = telebot_json_get_int(tmp);
    if (telebot_json_get_ex(obj, "month", &tmp))
        birthdate->month = telebot_json_get_int(tmp);
    if (telebot_json_get_ex(obj, "year", &tmp))
        birthdate->year = telebot_json_get_int(tmp);

    return TELEBOT_ERROR_NONE;
}

static telebot_error_e telebot_parser_get_business_intro(struct cJSON *obj, telebot_business_intro_t *intro)
{
    if ((obj == NULL) || (intro == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(intro, 0, sizeof(telebot_business_intro_t));
    struct cJSON *tmp = NULL;
    if (telebot_json_get_ex(obj, "title", &tmp))
        intro->title = TELEBOT_SAFE_STRDUP(telebot_json_get_string(tmp));
    if (telebot_json_get_ex(obj, "message", &tmp))
        intro->message = TELEBOT_SAFE_STRDUP(telebot_json_get_string(tmp));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_business_location_fields[] = {
    TELEBOT_JSON_FIELD("address", telebot_business_location_t, address, TELEBOT_FIELD_STRING),
};

static const telebot_json_field_t telebot_parser_get_business_location_objects[] = {
    TELEBOT_JSON_OBJECT("location", telebot_business_location_t, location, TELEBOT_OBJECT_LOCATION),
};

static telebot_error_e telebot_parser_get_business_location(struct cJSON *obj, telebot_business_location_t *location)
{
    if ((obj == NULL) || (location == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(location, 0, sizeof(telebot_business_location_t));
    telebot_parse_fields(obj, location, telebot_parser_get_business_location_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_business_location_fields));

    telebot_parse_fields(obj, location, telebot_parser_get_business_location_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_business_location_objects));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_business_opening_hours_fields[] = {
    TELEBOT_JSON_FIELD("time_zone_name", telebot_business_opening_hours_t, time_zone_name, TELEBOT_FIELD_STRING),
};

static telebot_error_e telebot_parser_get_business_opening_hours(struct cJSON *obj, telebot_business_opening_hours_t *hours)
{
    if ((obj == NULL) || (hours == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(hours, 0, sizeof(telebot_business_opening_hours_t));
    telebot_parse_fields(obj, hours, telebot_parser_get_business_opening_hours_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_business_opening_hours_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_star_transaction_fields[] = {
    TELEBOT_JSON_FIELD("id", telebot_star_transaction_t, id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("amount", telebot_star_transaction_t, amount, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("date", telebot_star_transaction_t, date, TELEBOT_FIELD_LONG),
};

static telebot_error_e telebot_parser_get_star_transaction(struct cJSON *obj, telebot_star_transaction_t *transaction)
{
    if ((obj == NULL) || (transaction == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(transaction, 0, sizeof(telebot_star_transaction_t));
    telebot_parse_fields(obj, transaction, telebot_parser_get_star_transaction_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_star_transaction_fields));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_star_transactions(struct cJSON *obj, telebot_star_transactions_t *transactions)
{
    if ((obj == NULL) || (transactions == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(transactions, 0, sizeof(telebot_star_transactions_t));
    struct cJSON *array = NULL;
    if (telebot_json_get_ex(obj, "transactions", &array))
    {
        int array_len = telebot_json_array_length(array);
        transactions->count_transactions = array_len;
        transactions->transactions = telebot_mem_calloc(array_len, sizeof(telebot_star_transaction_t));
        for (int i = 0; i < array_len; i++)
            telebot_parser_get_star_transaction(telebot_json_array_get_idx(array, i), &(transactions->transactions[i]));
    }

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_prepared_inline_message_fields[] = {
    TELEBOT_JSON_FIELD("id", telebot_prepared_inline_message_t, id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("expiration_date", telebot_prepared_inline_message_t, expiration_date, TELEBOT_FIELD_LONG),
};

telebot_error_e telebot_parser_get_prepared_inline_message(struct cJSON *obj, telebot_prepared_inline_message_t *prepared_message)
{
    if ((obj == NULL) || (prepared_message == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(prepared_message, 0, sizeof(telebot_prepared_inline_message_t));
    telebot_parse_fields(obj, prepared_message, telebot_parser_get_prepared_inline_message_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_prepared_inline_message_fields));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_game_high_score_fields[] = {
    TELEBOT_JSON_FIELD("position", telebot_game_high_score_t, position, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("score", telebot_game_high_score_t, score, TELEBOT_FIELD_INT),
};

static const telebot_json_field_t telebot_parser_get_game_high_score_objects[] = {
    TELEBOT_JSON_OBJECT("user", telebot_game_high_score_t, user, TELEBOT_OBJECT_USER),
};

telebot_error_e telebot_parser_get_game_high_score(struct cJSON *obj, telebot_game_high_score_t *high_score)
{
    if ((obj == NULL) || (high_score == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(high_score, 0, sizeof(telebot_game_high_score_t));
    telebot_parse_fields(obj, high_score, telebot_parser_get_game_high_score_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_game_high_score_fields));

    telebot_parse_fields(obj, high_score, telebot_parser_get_game_high_score_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_game_high_score_objects));

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_paid_media_purchased_fields[] = {
    TELEBOT_JSON_FIELD("paid_media_payload", telebot_paid_media_purchased_t, paid_media_payload, TELEBOT_FIELD_STRING),
};

static const telebot_json_field_t telebot_parser_get_paid_media_purchased_objects[] = {
    TELEBOT_JSON_OBJECT("from", telebot_paid_media_purchased_t, from, TELEBOT_OBJECT_USER),
};

telebot_error_e telebot_parser_get_paid_media_purchased(struct cJSON *obj, telebot_paid_media_purchased_t *purchased)
{
    if ((obj == NULL) || (purchased == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(purchased, 0, sizeof(telebot_paid_media_purchased_t));
    telebot_parse_fields(obj, purchased, telebot_parser_get_paid_media_purchased_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_paid_media_purchased_objects));

    telebot_parse_fields(obj, purchased, telebot_parser_get_paid_media_purchased_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_paid_media_purchased_fields));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_gifts(struct cJSON *obj, telebot_gifts_t *gifts)
{
    if ((obj == NULL) || (gifts == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gifts, 0, sizeof(telebot_gifts_t));

    struct cJSON *result = NULL;
    if (!telebot_json_get_ex(obj, "result", &result))
        result = obj;

    if (!cJSON_IsArray(result))
        return TELEBOT_ERROR_OPERATION_FAILED;

    int array_len = telebot_json_array_length(result);
    if (array_len > 0)
    {
        gifts->gifts = telebot_mem_calloc(array_len, sizeof(telebot_gift_t));
        if (gifts->gifts == NULL)
            return TELEBOT_ERROR_OUT_OF_MEMORY;

        gifts->count = array_len;
        for (int i = 0; i < array_len; i++)
        {
            struct cJSON *item = telebot_json_array_get_idx(result, i);
            telebot_parser_get_gift(item, &(gifts->gifts[i]));
        }
    }

    return TELEBOT_ERROR_NONE;
}

static const telebot_json_field_t telebot_parser_get_user_gift_fields[] = {
    TELEBOT_JSON_FIELD("gift_id", telebot_user_gift_t, gift_id, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("text", telebot_user_gift_t, text, TELEBOT_FIELD_STRING),
    TELEBOT_JSON_FIELD("date", telebot_user_gift_t, date, TELEBOT_FIELD_LONG),
    TELEBOT_JSON_FIELD("message_id", telebot_user_gift_t, message_id, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("upgrade_star_count", telebot_user_gift_t, upgrade_star_count, TELEBOT_FIELD_INT),
    TELEBOT_JSON_FIELD("is_upgraded", telebot_user_gift_t, is_upgraded, TELEBOT_FIELD_BOOL),
};

static const telebot_json_field_t telebot_parser_get_user_gift_objects[] = {
    TELEBOT_JSON_OBJECT("sender_user", telebot_user_gift_t, sender_user, TELEBOT_OBJECT_USER),
    TELEBOT_JSON_OBJECT("gift", telebot_user_gift_t, gift, TELEBOT_OBJECT_GIFT),
};

telebot_error_e telebot_parser_get_user_gift(struct cJSON *obj, telebot_user_gift_t *gift)
{
    if ((obj == NULL) || (gift == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gift, 0, sizeof(telebot_user_gift_t));

    telebot_parse_fields(obj, gift, telebot_parser_get_user_gift_fields, TELEBOT_FIELD_COUNT(telebot_parser_get_user_gift_fields));

    telebot_parse_fields(obj, gift, telebot_parser_get_user_gift_objects, TELEBOT_FIELD_COUNT(telebot_parser_get_user_gift_objects));

    struct cJSON *entities = NULL;
    if (telebot_json_get_ex(obj, "entities", &entities))
        telebot_parser_get_message_entities(entities, &(gift->entities), &(gift->count_entities));

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user_gifts(struct cJSON *obj, telebot_user_gifts_t *gifts)
{
    if ((obj == NULL) || (gifts == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(gifts, 0, sizeof(telebot_user_gifts_t));

    struct cJSON *result = NULL;
    if (!telebot_json_get_ex(obj, "result", &result))
        result = obj;

    struct cJSON *total_count = NULL;
    if (telebot_json_get_ex(result, "total_count", &total_count))
        gifts->total_count = telebot_json_get_int(total_count);

    struct cJSON *gifts_array = NULL;
    if (telebot_json_get_ex(result, "gifts", &gifts_array))
    {
        int array_len = telebot_json_array_length(gifts_array);
        if (array_len > 0)
        {
            gifts->gifts = telebot_mem_calloc(array_len, sizeof(telebot_user_gift_t));
            if (gifts->gifts == NULL)
                return TELEBOT_ERROR_OUT_OF_MEMORY;

            gifts->count = array_len;
            for (int i = 0; i < array_len; i++)
            {
                struct cJSON *item = telebot_json_array_get_idx(gifts_array, i);
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
    if (!telebot_json_get_ex(obj, "result", &result))
        result = obj;

    struct cJSON *total_count = NULL;
    if (telebot_json_get_ex(result, "total_count", &total_count))
        audios->total_count = telebot_json_get_int(total_count);

    struct cJSON *audios_array = NULL;
    if (telebot_json_get_ex(result, "audios", &audios_array))
    {
        int array_len = telebot_json_array_length(audios_array);
        if (array_len > 0)
        {
            audios->audios = telebot_mem_calloc(array_len, sizeof(telebot_audio_t));
            if (audios->audios == NULL)
                return TELEBOT_ERROR_OUT_OF_MEMORY;

            audios->count = array_len;
            for (int i = 0; i < array_len; i++)
            {
                struct cJSON *item = telebot_json_array_get_idx(audios_array, i);
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
    if (!telebot_json_get_ex(obj, "result", &result))
        result = obj;

    struct cJSON *id = NULL;
    if (telebot_json_get_ex(result, "id", &id))
        connection->id = TELEBOT_SAFE_STRDUP(telebot_json_get_string(id));

    struct cJSON *user = NULL;
    if (telebot_json_get_ex(result, "user", &user))
    {
        connection->user = telebot_mem_calloc(1, sizeof(telebot_user_t));
        telebot_parser_get_user(user, connection->user);
    }

    struct cJSON *user_chat_id = NULL;
    if (telebot_json_get_ex(result, "user_chat_id", &user_chat_id))
        connection->user_chat_id = telebot_json_get_int(user_chat_id);

    struct cJSON *date = NULL;
    if (telebot_json_get_ex(result, "date", &date))
        connection->date = telebot_json_get_int(date);

    struct cJSON *rights = NULL;
    if (telebot_json_get_ex(result, "rights", &rights))
        telebot_parser_get_business_bot_rights(rights, &connection->rights);

    struct cJSON *is_enabled = NULL;
    if (telebot_json_get_ex(result, "is_enabled", &is_enabled))
        connection->is_enabled = telebot_json_get_bool(is_enabled);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_user_chat_boosts(struct cJSON *obj, telebot_user_chat_boosts_t *boosts)
{
    if ((obj == NULL) || (boosts == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    memset(boosts, 0, sizeof(telebot_user_chat_boosts_t));

    struct cJSON *result = NULL;
    if (!telebot_json_get_ex(obj, "result", &result))
        result = obj;

    struct cJSON *boosts_array = NULL;
    if (telebot_json_get_ex(result, "boosts", &boosts_array))
    {
        int array_len = telebot_json_array_length(boosts_array);
        if (array_len > 0)
        {
            boosts->boosts = telebot_mem_calloc(array_len, sizeof(telebot_chat_boost_t));
            if (boosts->boosts == NULL)
                return TELEBOT_ERROR_OUT_OF_MEMORY;

            boosts->count = array_len;
            for (int i = 0; i < array_len; i++)
            {
                struct cJSON *item = telebot_json_array_get_idx(boosts_array, i);
                telebot_parser_get_chat_boost(item, &(boosts->boosts[i]));
            }
        }
    }

    return TELEBOT_ERROR_NONE;
}

/* Allocates and parses a User, or leaves *out NULL */
static void telebot_parser_get_user_ptr(struct cJSON *obj, const char *key, telebot_user_t **out)
{
    cJSON *tmp = NULL;
    if (!telebot_json_get_ex(obj, key, &tmp))
        return;
    *out = telebot_mem_calloc(1, sizeof(telebot_user_t));
    if ((*out != NULL) && (telebot_parser_get_user(tmp, *out) != TELEBOT_ERROR_NONE))
        TELEBOT_SAFE_FREE(*out);
}

#if TELEBOT_FEATURE_PAYMENTS || TELEBOT_FEATURE_WIDGETS
static char *telebot_parser_dup_string(struct cJSON *obj, const char *key)
{
    cJSON *tmp = NULL;
    return telebot_json_get_ex(obj, key, &tmp) ? TELEBOT_SAFE_STRDUP(telebot_json_get_string(tmp)) : NULL;
}
#endif

static void telebot_parser_get_managed_bot_updated(struct cJSON *obj, telebot_managed_bot_updated_t *updated)
{
    memset(updated, 0, sizeof(*updated));
    telebot_parser_get_user_ptr(obj, "user", &updated->user);
    telebot_parser_get_user_ptr(obj, "bot", &updated->bot);
}

#if TELEBOT_FEATURE_PAYMENTS
static void telebot_parser_get_bot_subscription_updated(struct cJSON *obj, telebot_bot_subscription_updated_t *updated)
{
    memset(updated, 0, sizeof(*updated));
    telebot_parser_get_user_ptr(obj, "user", &updated->user);
    updated->invoice_payload = telebot_parser_dup_string(obj, "invoice_payload");
    updated->state = telebot_parser_dup_string(obj, "state");
}
#endif

#if TELEBOT_FEATURE_WIDGETS
static void telebot_parser_get_widget_query(struct cJSON *obj, telebot_widget_query_t *query)
{
    memset(query, 0, sizeof(*query));
    cJSON *tmp = NULL;

    query->id = telebot_parser_dup_string(obj, "id");
    telebot_parser_get_user_ptr(obj, "from", &query->from);
    if (telebot_json_get_ex(obj, "chat", &tmp))
    {
        query->chat = telebot_mem_calloc(1, sizeof(telebot_chat_t));
        if ((query->chat != NULL) && (telebot_parser_get_chat(tmp, query->chat) != TELEBOT_ERROR_NONE))
            TELEBOT_SAFE_FREE(query->chat);
    }
    if (telebot_json_get_ex(obj, "message_id", &tmp))
        query->message_id = telebot_json_get_int(tmp);
    query->inline_message_id = telebot_parser_dup_string(obj, "inline_message_id");
    query->chat_instance = telebot_parser_dup_string(obj, "chat_instance");
    query->chat_type = telebot_parser_dup_string(obj, "chat_type");
    query->data = telebot_parser_dup_string(obj, "data");
    query->widget_id = telebot_parser_dup_string(obj, "widget_id");

    cJSON *type = NULL;
    if (telebot_json_get_ex(obj, "type", &type))
    {
        query->query_type = telebot_parser_dup_string(type, "type");
        query->sign_data = telebot_parser_dup_string(type, "sign_data");
        query->msg_hash = telebot_parser_dup_string(type, "msg_hash");
        if (telebot_json_get_ex(type, "wallet", &tmp))
        {
            query->wallet = telebot_mem_calloc(1, sizeof(telebot_widget_query_wallet_t));
            if (query->wallet != NULL)
            {
                query->wallet->address = telebot_parser_dup_string(tmp, "address");
                query->wallet->amount = telebot_parser_dup_string(tmp, "amount");
                query->wallet->public_key = telebot_parser_dup_string(tmp, "public_key");
                query->wallet->state_init = telebot_parser_dup_string(tmp, "state_init");
            }
        }
    }
}
#endif

void telebot_parse_fields(const struct cJSON *obj, void *target, const telebot_json_field_t *fields, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        cJSON *value = NULL;
        if (!telebot_json_get_ex(obj, fields[i].key, &value))
            continue;

        void *field = (char *)target + fields[i].offset;
        switch (fields[i].kind)
        {
        case TELEBOT_FIELD_STRING:
            *(char **)field = TELEBOT_SAFE_STRDUP(telebot_json_get_string(value));
            break;
        case TELEBOT_FIELD_BOOL:
            *(bool *)field = telebot_json_get_bool(value);
            break;
        case TELEBOT_FIELD_INT:
            *(int *)field = (int)telebot_json_get_int(value);
            break;
        case TELEBOT_FIELD_LONG:
            *(long *)field = (long)telebot_json_get_int(value);
            break;
        case TELEBOT_FIELD_LONG_LONG:
            *(long long *)field = telebot_json_get_int(value);
            break;
        case TELEBOT_FIELD_OBJECT:
        {
            /* A nested object that fails to parse is left out; its memory belongs to the arena */
            const telebot_object_type_t *type = &telebot_object_types[fields[i].aux];
            void *nested = telebot_mem_calloc(1, type->size);
            if ((nested != NULL) && (type->parse(value, nested) != TELEBOT_ERROR_NONE))
            {
                ERR("Failed to parse <%s>", fields[i].key);
                nested = NULL;
            }
            *(void **)field = nested;
            break;
        }
        default:
            break;
        }
    }
}

/* Adapters giving every nested-object parser the generic table signature */
static telebot_error_e telebot_parser_get_animation_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_animation(obj, target);
}

static telebot_error_e telebot_parser_get_audio_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_audio(obj, target);
}

static telebot_error_e telebot_parser_get_birthdate_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_birthdate(obj, target);
}

static telebot_error_e telebot_parser_get_business_intro_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_business_intro(obj, target);
}

static telebot_error_e telebot_parser_get_business_location_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_business_location(obj, target);
}

static telebot_error_e telebot_parser_get_business_opening_hours_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_business_opening_hours(obj, target);
}

static telebot_error_e telebot_parser_get_chat_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_chat(obj, target);
}

static telebot_error_e telebot_parser_get_chat_boost_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_chat_boost(obj, target);
}

static telebot_error_e telebot_parser_get_chat_boost_source_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_chat_boost_source(obj, target);
}

static telebot_error_e telebot_parser_get_chat_invite_link_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_chat_invite_link(obj, target);
}

static telebot_error_e telebot_parser_get_chat_location_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_chat_location(obj, target);
}

static telebot_error_e telebot_parser_get_chat_member_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_chat_member(obj, target);
}

static telebot_error_e telebot_parser_get_chat_permissions_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_chat_permissions(obj, target);
}

static telebot_error_e telebot_parser_get_chat_photo_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_chat_photo(obj, target);
}

static telebot_error_e telebot_parser_get_contact_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_contact(obj, target);
}

static telebot_error_e telebot_parser_get_dice_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_dice(obj, target);
}

static telebot_error_e telebot_parser_get_document_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_document(obj, target);
}

static telebot_error_e telebot_parser_get_encrypted_credentials_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_encrypted_credentials(obj, target);
}

static telebot_error_e telebot_parser_get_external_reply_info_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_external_reply_info(obj, target);
}

static telebot_error_e telebot_parser_get_gift_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_gift(obj, target);
}

static telebot_error_e telebot_parser_get_inline_keyboard_markup_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_inline_keyboard_markup(obj, target);
}

static telebot_error_e telebot_parser_get_link_preview_options_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_link_preview_options(obj, target);
}

static telebot_error_e telebot_parser_get_location_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_location(obj, target);
}

static telebot_error_e telebot_parser_get_message_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_message(obj, target);
}

static telebot_error_e telebot_parser_get_message_origin_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_message_origin(obj, target);
}

static telebot_error_e telebot_parser_get_order_info_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_order_info(obj, target);
}

static telebot_error_e telebot_parser_get_passport_file_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_passport_file(obj, target);
}

static telebot_error_e telebot_parser_get_photo_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_photo(obj, target);
}

static telebot_error_e telebot_parser_get_poll_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_poll(obj, target);
}

static telebot_error_e telebot_parser_get_shipping_address_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_shipping_address(obj, target);
}

static telebot_error_e telebot_parser_get_story_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_story(obj, target);
}

static telebot_error_e telebot_parser_get_text_quote_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_text_quote(obj, target);
}

static telebot_error_e telebot_parser_get_user_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_user(obj, target);
}

static telebot_error_e telebot_parser_get_venue_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_venue(obj, target);
}

static telebot_error_e telebot_parser_get_video_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_video(obj, target);
}

static telebot_error_e telebot_parser_get_video_note_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_video_note(obj, target);
}

static telebot_error_e telebot_parser_get_voice_object(struct cJSON *obj, void *target)
{
    return telebot_parser_get_voice(obj, target);
}

const telebot_object_type_t telebot_object_types[TELEBOT_OBJECT_TYPE_COUNT] = {
    [TELEBOT_OBJECT_ANIMATION] = {sizeof(telebot_animation_t), telebot_parser_get_animation_object},
    [TELEBOT_OBJECT_AUDIO] = {sizeof(telebot_audio_t), telebot_parser_get_audio_object},
    [TELEBOT_OBJECT_BIRTHDATE] = {sizeof(telebot_birthdate_t), telebot_parser_get_birthdate_object},
    [TELEBOT_OBJECT_BUSINESS_INTRO] = {sizeof(telebot_business_intro_t), telebot_parser_get_business_intro_object},
    [TELEBOT_OBJECT_BUSINESS_LOCATION] = {sizeof(telebot_business_location_t), telebot_parser_get_business_location_object},
    [TELEBOT_OBJECT_BUSINESS_OPENING_HOURS] = {sizeof(telebot_business_opening_hours_t), telebot_parser_get_business_opening_hours_object},
    [TELEBOT_OBJECT_CHAT] = {sizeof(telebot_chat_t), telebot_parser_get_chat_object},
    [TELEBOT_OBJECT_CHAT_BOOST] = {sizeof(telebot_chat_boost_t), telebot_parser_get_chat_boost_object},
    [TELEBOT_OBJECT_CHAT_BOOST_SOURCE] = {sizeof(telebot_chat_boost_source_t), telebot_parser_get_chat_boost_source_object},
    [TELEBOT_OBJECT_CHAT_INVITE_LINK] = {sizeof(telebot_chat_invite_link_t), telebot_parser_get_chat_invite_link_object},
    [TELEBOT_OBJECT_CHAT_LOCATION] = {sizeof(telebot_chat_location_t), telebot_parser_get_chat_location_object},
    [TELEBOT_OBJECT_CHAT_MEMBER] = {sizeof(telebot_chat_member_t), telebot_parser_get_chat_member_object},
    [TELEBOT_OBJECT_CHAT_PERMISSIONS] = {sizeof(telebot_chat_permissions_t), telebot_parser_get_chat_permissions_object},
    [TELEBOT_OBJECT_CHAT_PHOTO] = {sizeof(telebot_chat_photo_t), telebot_parser_get_chat_photo_object},
    [TELEBOT_OBJECT_CONTACT] = {sizeof(telebot_contact_t), telebot_parser_get_contact_object},
    [TELEBOT_OBJECT_DICE] = {sizeof(telebot_dice_t), telebot_parser_get_dice_object},
    [TELEBOT_OBJECT_DOCUMENT] = {sizeof(telebot_document_t), telebot_parser_get_document_object},
    [TELEBOT_OBJECT_ENCRYPTED_CREDENTIALS] = {sizeof(telebot_encrypted_credentials_t), telebot_parser_get_encrypted_credentials_object},
    [TELEBOT_OBJECT_EXTERNAL_REPLY_INFO] = {sizeof(telebot_external_reply_info_t), telebot_parser_get_external_reply_info_object},
    [TELEBOT_OBJECT_GIFT] = {sizeof(telebot_gift_t), telebot_parser_get_gift_object},
    [TELEBOT_OBJECT_INLINE_KEYBOARD_MARKUP] = {sizeof(telebot_inline_keyboard_markup_t), telebot_parser_get_inline_keyboard_markup_object},
    [TELEBOT_OBJECT_LINK_PREVIEW_OPTIONS] = {sizeof(telebot_link_preview_options_t), telebot_parser_get_link_preview_options_object},
    [TELEBOT_OBJECT_LOCATION] = {sizeof(telebot_location_t), telebot_parser_get_location_object},
    [TELEBOT_OBJECT_MESSAGE] = {sizeof(telebot_message_t), telebot_parser_get_message_object},
    [TELEBOT_OBJECT_MESSAGE_ORIGIN] = {sizeof(telebot_message_origin_t), telebot_parser_get_message_origin_object},
    [TELEBOT_OBJECT_ORDER_INFO] = {sizeof(telebot_order_info_t), telebot_parser_get_order_info_object},
    [TELEBOT_OBJECT_PASSPORT_FILE] = {sizeof(telebot_passport_file_t), telebot_parser_get_passport_file_object},
    [TELEBOT_OBJECT_PHOTO] = {sizeof(telebot_photo_t), telebot_parser_get_photo_object},
    [TELEBOT_OBJECT_POLL] = {sizeof(telebot_poll_t), telebot_parser_get_poll_object},
    [TELEBOT_OBJECT_SHIPPING_ADDRESS] = {sizeof(telebot_shipping_address_t), telebot_parser_get_shipping_address_object},
    [TELEBOT_OBJECT_STORY] = {sizeof(telebot_story_t), telebot_parser_get_story_object},
    [TELEBOT_OBJECT_TEXT_QUOTE] = {sizeof(telebot_text_quote_t), telebot_parser_get_text_quote_object},
    [TELEBOT_OBJECT_USER] = {sizeof(telebot_user_t), telebot_parser_get_user_object},
    [TELEBOT_OBJECT_VENUE] = {sizeof(telebot_venue_t), telebot_parser_get_venue_object},
    [TELEBOT_OBJECT_VIDEO] = {sizeof(telebot_video_t), telebot_parser_get_video_object},
    [TELEBOT_OBJECT_VIDEO_NOTE] = {sizeof(telebot_video_note_t), telebot_parser_get_video_note_object},
    [TELEBOT_OBJECT_VOICE] = {sizeof(telebot_voice_t), telebot_parser_get_voice_object},
};
