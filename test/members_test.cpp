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

/* Request building for moderation (telebot-core-members.c). */
#include <cstring>

#include "core_fixture.h"

namespace
{

using Members = CoreTest;

const long long kBigUser = 7123456789LL; // does not fit in 32 bits

TEST_F(Members, UserIdsAre64Bit)
{
    CoreResponse r(telebot_core_ban_chat_member(core_, -100500, kBigUser, 0, true));
    EXPECT_EQ(last().method(), "banChatMember");
    EXPECT_EQ(last().at("user_id"), "7123456789");
    EXPECT_EQ(last().at("revoke_messages"), "true");
    EXPECT_FALSE(last().has("until_date"));
}

TEST_F(Members, MissingUserIsRejectedLocally)
{
    CoreResponse r(telebot_core_unban_chat_member(core_, -100500, 0, true));
    EXPECT_EQ(r.code(), TELEBOT_ERROR_INVALID_PARAMETER);
    EXPECT_TRUE(nothing_sent());
}

TEST_F(Members, RestrictSerialisesGrantedPermissionsOnly)
{
    telebot_chat_permissions_t perms{};
    perms.can_send_messages = true;
    perms.can_react_to_messages = true;
    CoreResponse r(telebot_core_restrict_chat_member(core_, -100500, kBigUser, &perms, true, 1800000000L));
    ASSERT_EQ(r.code(), TELEBOT_ERROR_NONE);
    EXPECT_EQ(last().at("permissions"), R"({"can_send_messages":true,"can_react_to_messages":true})");
    EXPECT_EQ(last().at("use_independent_chat_permissions"), "true");
    EXPECT_EQ(last().at("until_date"), "1800000000");
}

TEST_F(Members, NoPermissionsMeansMutedNotOmitted)
{
    telebot_chat_permissions_t none{};
    CoreResponse r(telebot_core_set_chat_permissions(core_, -100500, &none, false));
    EXPECT_EQ(last().at("permissions"), "{}");

    CoreResponse missing(telebot_core_set_chat_permissions(core_, -100500, nullptr, false));
    EXPECT_EQ(missing.code(), TELEBOT_ERROR_INVALID_PARAMETER);
}

TEST_F(Members, AllPermissionsFitTheScratchArena)
{
    telebot_chat_permissions_t all;
    memset(&all, 1, sizeof(all));
    CoreResponse r(telebot_core_restrict_chat_member(core_, -100500, kBigUser, &all, false, 0));
    ASSERT_EQ(r.code(), TELEBOT_ERROR_NONE);
    const std::string &json = last().at("permissions");
    EXPECT_NE(json.find("\"can_manage_topics\":true}"), std::string::npos);
    EXPECT_NE(json.find("\"can_send_voice_notes\":true"), std::string::npos);
}

TEST_F(Members, PromoteSendsGrantedRightsAsFields)
{
    telebot_chat_administrator_rights_t rights{};
    rights.can_delete_messages = true;
    rights.can_manage_tags = true;
    CoreResponse r(telebot_core_promote_chat_member(core_, -100500, kBigUser, &rights));
    EXPECT_EQ(last().at("can_delete_messages"), "true");
    EXPECT_EQ(last().at("can_manage_tags"), "true");
    EXPECT_FALSE(last().has("can_promote_members"));

    CoreResponse demote(telebot_core_promote_chat_member(core_, -100500, kBigUser, nullptr));
    EXPECT_EQ(last().fields.size(), 2u); // chat_id + user_id only
}

TEST_F(Members, ChatPhotoMustBeAnUpload)
{
    const telebot_input_file_t remote = telebot_input_file_remote("AgAC");
    CoreResponse r(telebot_core_set_chat_photo(core_, -100500, &remote));
    EXPECT_EQ(r.code(), TELEBOT_ERROR_INVALID_PARAMETER);

    const telebot_input_file_t path = telebot_input_file_path("/sdcard/logo.png");
    CoreResponse ok(telebot_core_set_chat_photo(core_, -100500, &path));
    EXPECT_EQ(last().fields.at("photo").type, TELEBOT_MIME_TYPE_FILE);
}

TEST_F(Members, JoinRequestsAndMemberLookups)
{
    CoreResponse a(telebot_core_approve_chat_join_request(core_, -100500, kBigUser));
    EXPECT_EQ(last().method(), "approveChatJoinRequest");
    CoreResponse d(telebot_core_decline_chat_join_request(core_, -100500, kBigUser));
    EXPECT_EQ(last().method(), "declineChatJoinRequest");
    CoreResponse m(telebot_core_get_chat_member(core_, -100500, kBigUser));
    EXPECT_EQ(last().at("user_id"), "7123456789");
    CoreResponse p(telebot_core_get_user_profile_photos(core_, kBigUser, 0, 5));
    EXPECT_EQ(last().at("limit"), "5");
    EXPECT_FALSE(last().has("offset"));
}

} // namespace

namespace
{

// Groups, supergroups and channels have negative identifiers.
TEST_F(Members, GroupChatIdsAreAccepted)
{
    const long long group = -1001234567890LL;
    CoreResponse a(telebot_core_get_chat(core_, group));
    CoreResponse b(telebot_core_leave_chat(core_, group));
    CoreResponse c(telebot_core_delete_message(core_, group, 5));
    CoreResponse d(telebot_core_set_chat_title(core_, group, "Lab"));
    CoreResponse e(telebot_core_pin_chat_message(core_, group, 5, false));
    for (const CoreResponse *r : {&a, &b, &c, &d, &e})
        EXPECT_EQ(r->code(), TELEBOT_ERROR_NONE);
    ASSERT_EQ(fake_->requests().size(), 5u);
    EXPECT_EQ(fake_->requests()[0].at("chat_id"), "-1001234567890");
}

TEST_F(Members, MissingChatIdIsRejected)
{
    CoreResponse r(telebot_core_get_chat(core_, 0));
    EXPECT_EQ(r.code(), TELEBOT_ERROR_INVALID_PARAMETER);
    EXPECT_TRUE(nothing_sent());
}

} // namespace
