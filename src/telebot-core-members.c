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
 * Core layer for chat membership and moderation. User identifiers are
 * 64-bit throughout, and permissions/rights are passed as structs instead of
 * long lists of positional booleans.
 */

#include "telebot-request.h"

/* Every method here needs a real user */
static void add_user(telebot_request_t *req, long long int user_id)
{
    if (user_id <= 0)
        telebot_request_str_required(req, "user_id", NULL);
    telebot_request_ll(req, "user_id", user_id);
}

telebot_core_response_t
telebot_core_ban_chat_member(telebot_core_handler_t core_h, long long int chat_id, long long int user_id,
                             long until_date, bool revoke_messages)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    add_user(&req, user_id);
    telebot_request_id(&req, "until_date", until_date);
    telebot_request_flag(&req, "revoke_messages", revoke_messages);
    return telebot_core_request(core_h, TELEBOT_METHOD_BAN_CHAT_MEMBER, &req);
}

telebot_core_response_t
telebot_core_unban_chat_member(telebot_core_handler_t core_h, long long int chat_id, long long int user_id,
                               bool only_if_banned)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    add_user(&req, user_id);
    telebot_request_flag(&req, "only_if_banned", only_if_banned);
    return telebot_core_request(core_h, TELEBOT_METHOD_UNBAN_CHAT_MEMBER, &req);
}

telebot_core_response_t
telebot_core_restrict_chat_member(telebot_core_handler_t core_h, long long int chat_id, long long int user_id,
                                  const telebot_chat_permissions_t *permissions,
                                  bool use_independent_chat_permissions, long until_date)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    add_user(&req, user_id);
    if (permissions == NULL)
        telebot_request_str_required(&req, "permissions", NULL);
    telebot_request_permissions(&req, "permissions", permissions);
    telebot_request_flag(&req, "use_independent_chat_permissions", use_independent_chat_permissions);
    telebot_request_id(&req, "until_date", until_date);
    return telebot_core_request(core_h, TELEBOT_METHOD_RESTRICT_CHAT_MEMBER, &req);
}

telebot_core_response_t
telebot_core_promote_chat_member(telebot_core_handler_t core_h, long long int chat_id, long long int user_id,
                                 const telebot_chat_administrator_rights_t *rights)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    add_user(&req, user_id);
    /* NULL or all-false rights demote the user */
    telebot_request_rights_fields(&req, rights);
    return telebot_core_request(core_h, TELEBOT_METHOD_PROMOTE_CHAT_MEMBER, &req);
}

telebot_core_response_t
telebot_core_set_chat_admin_custom_title(telebot_core_handler_t core_h, long long int chat_id,
                                         long long int user_id, const char *custom_title)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    add_user(&req, user_id);
    telebot_request_str_required(&req, "custom_title", custom_title);
    return telebot_core_request(core_h, TELEBOT_METHOD_SET_CHAT_ADMIN_TITLE, &req);
}

telebot_core_response_t
telebot_core_set_chat_permissions(telebot_core_handler_t core_h, long long int chat_id,
                                  const telebot_chat_permissions_t *permissions,
                                  bool use_independent_chat_permissions)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    if (permissions == NULL)
        telebot_request_str_required(&req, "permissions", NULL);
    telebot_request_permissions(&req, "permissions", permissions);
    telebot_request_flag(&req, "use_independent_chat_permissions", use_independent_chat_permissions);
    return telebot_core_request(core_h, TELEBOT_METHOD_SET_CHAT_PERMISSIONS, &req);
}

telebot_core_response_t
telebot_core_get_chat_member(telebot_core_handler_t core_h, long long int chat_id, long long int user_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    add_user(&req, user_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_CHAT_MEMBER, &req);
}

telebot_core_response_t
telebot_core_get_user_profile_photos(telebot_core_handler_t core_h, long long int user_id, int offset, int limit)
{
    telebot_request_t req;
    telebot_request_init(&req);
    add_user(&req, user_id);
    telebot_request_id(&req, "offset", offset);
    telebot_request_id(&req, "limit", limit);
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_USER_PHOTOS, &req);
}

telebot_core_response_t
telebot_core_approve_chat_join_request(telebot_core_handler_t core_h, long long int chat_id, long long int user_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    add_user(&req, user_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_APPROVE_CHAT_JOIN_REQUEST, &req);
}

telebot_core_response_t
telebot_core_decline_chat_join_request(telebot_core_handler_t core_h, long long int chat_id, long long int user_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    add_user(&req, user_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_DECLINE_CHAT_JOIN_REQUEST, &req);
}

telebot_core_response_t
telebot_core_set_chat_photo(telebot_core_handler_t core_h, long long int chat_id, const telebot_input_file_t *photo)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    if (photo == NULL)
        telebot_request_str_required(&req, "photo", NULL);
    telebot_request_upload(&req, "photo", photo); /* chat photos are always new uploads */
    return telebot_core_request(core_h, TELEBOT_METHOD_SET_CHAT_PHOTO, &req);
}

telebot_core_response_t
telebot_core_get_chat_admins(telebot_core_handler_t core_h, long long int chat_id, bool return_bots)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    telebot_request_flag(&req, "return_bots", return_bots);
    return telebot_core_request(core_h, TELEBOT_METHOD_GET_CHAT_ADMINS, &req);
}

telebot_core_response_t
telebot_core_set_chat_member_tag(telebot_core_handler_t core_h, long long int chat_id, long long int user_id,
                                 const char *tag)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    add_user(&req, user_id);
    telebot_request_str(&req, "tag", tag); /* omitted tag removes it */
    return telebot_core_request(core_h, TELEBOT_METHOD_SET_CHAT_MEMBER_TAG, &req);
}

/* Optional verification text, at most 70 characters, only text_link entities */
static void add_custom_description(telebot_request_t *req, const char *custom_description, const char *parse_mode)
{
    telebot_request_str(req, "custom_description", custom_description);
    if (custom_description != NULL)
        telebot_request_str(req, "custom_description_parse_mode", parse_mode);
}

telebot_core_response_t
telebot_core_verify_user(telebot_core_handler_t core_h, long long int user_id, const char *custom_description,
                         const char *parse_mode)
{
    telebot_request_t req;
    telebot_request_init(&req);
    add_user(&req, user_id);
    add_custom_description(&req, custom_description, parse_mode);
    return telebot_core_request(core_h, TELEBOT_METHOD_VERIFY_USER, &req);
}

telebot_core_response_t
telebot_core_verify_chat(telebot_core_handler_t core_h, long long int chat_id, const char *custom_description,
                         const char *parse_mode)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    add_custom_description(&req, custom_description, parse_mode);
    return telebot_core_request(core_h, TELEBOT_METHOD_VERIFY_CHAT, &req);
}

telebot_core_response_t telebot_core_remove_user_verification(telebot_core_handler_t core_h, long long int user_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    add_user(&req, user_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_REMOVE_USER_VERIFICATION, &req);
}

telebot_core_response_t telebot_core_remove_chat_verification(telebot_core_handler_t core_h, long long int chat_id)
{
    telebot_request_t req;
    telebot_request_init(&req);
    telebot_request_ll(&req, "chat_id", chat_id);
    return telebot_core_request(core_h, TELEBOT_METHOD_REMOVE_CHAT_VERIFICATION, &req);
}
