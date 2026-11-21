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

/* Drafts, reactions, member tags, verification and rate limiting. */
#include "core_fixture.h"

namespace
{

using RecentApi = CoreTest;

TEST_F(RecentApi, MessageDraftStreamsText)
{
    CoreResponse r(telebot_core_send_message_draft(core_, 5, 0, 77, "Partial answer", nullptr, true, false));
    ASSERT_EQ(r.code(), TELEBOT_ERROR_NONE);
    EXPECT_EQ(last().method(), "sendMessageDraft");
    EXPECT_EQ(last().at("draft_id"), "77");
    EXPECT_EQ(last().at("text"), "Partial answer");
    EXPECT_EQ(last().at("can_stop"), "true");
    EXPECT_FALSE(last().has("keep_on_stop"));
    EXPECT_FALSE(last().has("message_thread_id"));
}

TEST_F(RecentApi, MessageDraftNeedsNonZeroId)
{
    CoreResponse r(telebot_core_send_message_draft(core_, 5, 0, 0, "x", nullptr, false, false));
    EXPECT_EQ(r.code(), TELEBOT_ERROR_INVALID_PARAMETER);
    EXPECT_TRUE(nothing_sent());
}

TEST_F(RecentApi, ReactionRemovalNeedsExactlyOneActor)
{
    CoreResponse user(telebot_core_delete_message_reaction(core_, -100, 9, 7123456789LL, 0));
    EXPECT_EQ(last().method(), "deleteMessageReaction");
    EXPECT_EQ(last().at("user_id"), "7123456789");
    EXPECT_FALSE(last().has("actor_chat_id"));

    CoreResponse chat(telebot_core_delete_all_message_reactions(core_, -100, 0, -1001234567890LL));
    EXPECT_EQ(last().method(), "deleteAllMessageReactions");
    EXPECT_EQ(last().at("actor_chat_id"), "-1001234567890");

    size_t sent = fake_->requests().size();
    CoreResponse both(telebot_core_delete_all_message_reactions(core_, -100, 1, -2));
    CoreResponse none(telebot_core_delete_message_reaction(core_, -100, 9, 0, 0));
    EXPECT_EQ(both.code(), TELEBOT_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(none.code(), TELEBOT_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(fake_->requests().size(), sent);
}

TEST_F(RecentApi, MemberTagsAndAdmins)
{
    CoreResponse tag(telebot_core_set_chat_member_tag(core_, -100, 42, "mod"));
    EXPECT_EQ(last().method(), "setChatMemberTag");
    EXPECT_EQ(last().at("tag"), "mod");

    CoreResponse clear(telebot_core_set_chat_member_tag(core_, -100, 42, nullptr));
    EXPECT_FALSE(last().has("tag"));

    CoreResponse admins(telebot_core_get_chat_admins(core_, -100, true));
    EXPECT_EQ(last().method(), "getChatAdministrators");
    EXPECT_EQ(last().at("return_bots"), "true");

    CoreResponse unpin(telebot_core_unpin_all_chat_messages(core_, -100));
    EXPECT_EQ(last().method(), "unpinAllChatMessages");
}

TEST_F(RecentApi, Verification)
{
    CoreResponse u(telebot_core_verify_user(core_, 42, "Official account", nullptr));
    EXPECT_EQ(last().method(), "verifyUser");
    EXPECT_EQ(last().at("custom_description"), "Official account");
    CoreResponse c(telebot_core_verify_chat(core_, -100, nullptr, "HTML"));
    EXPECT_FALSE(last().has("custom_description_parse_mode")); // meaningless without a description
    CoreResponse ru(telebot_core_remove_user_verification(core_, 42));
    EXPECT_EQ(last().method(), "removeUserVerification");
    CoreResponse rc(telebot_core_remove_chat_verification(core_, -100));
    EXPECT_EQ(last().method(), "removeChatVerification");
}

TEST_F(RecentApi, FloodControlReportsRetryAfter)
{
    fake_->reply(429, R"({"ok":false,"error_code":429,"description":"Too Many Requests: retry after 17",)"
                      R"("parameters":{"retry_after":17}})");
    CoreResponse r(telebot_core_send_message(core_, 5, "hi", nullptr, false, nullptr));
    EXPECT_EQ(r.code(), TELEBOT_ERROR_RATE_LIMITED);
    EXPECT_EQ(telebot_core_get_response_retry_after(r.r), 17);
    EXPECT_EQ(telebot_core_get_retry_after(core_), 17);

    // The next successful request clears it
    CoreResponse ok(telebot_core_get_me(core_));
    EXPECT_EQ(telebot_core_get_retry_after(core_), 0);
    EXPECT_EQ(telebot_core_get_response_retry_after(ok.r), 0);
}

TEST_F(RecentApi, OtherHttpErrorsAreNotRateLimits)
{
    fake_->reply(400, R"({"ok":false,"error_code":400,"description":"Bad Request"})");
    CoreResponse r(telebot_core_get_me(core_));
    EXPECT_EQ(r.code(), TELEBOT_ERROR_OPERATION_FAILED);
    EXPECT_EQ(telebot_core_get_response_retry_after(r.r), 0);
}

class RecentApiHighLevel : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_EQ(telebot_create(&handle_, "123456:TEST-TOKEN"), TELEBOT_ERROR_NONE);
        fake_ = FakeTransport::install(handle_->core_h);
    }
    void TearDown() override { telebot_destroy(handle_); }

    telebot_handler_t handle_ = nullptr;
    FakeTransport *fake_ = nullptr;
};

TEST_F(RecentApiHighLevel, RateLimitIsVisibleThroughTheHandle)
{
    fake_->reply(429, R"({"ok":false,"error_code":429,"parameters":{"retry_after":3}})");
    EXPECT_EQ(telebot_send_message(handle_, 5, "hi", nullptr, false, nullptr), TELEBOT_ERROR_RATE_LIMITED);
    EXPECT_EQ(telebot_get_retry_after(handle_), 3);
    EXPECT_EQ(telebot_get_retry_after(nullptr), 0);
}

TEST_F(RecentApiHighLevel, ChatMemberCarriesTagRightsAndPermissions)
{
    fake_->reply(200, R"({"ok":true,"result":{"status":"restricted","user":{"id":7123456789,"is_bot":false,"first_name":"R"},
        "tag":"newbie","is_member":true,"until_date":0,"can_send_messages":true,"can_send_photos":false,
        "can_edit_tag":true,"can_react_to_messages":true}})");
    telebot_chat_member_t member;
    ASSERT_EQ(telebot_get_chat_member(handle_, -100, 7123456789LL, &member), TELEBOT_ERROR_NONE);
    EXPECT_STREQ(member.status, "restricted");
    EXPECT_STREQ(member.tag, "newbie");
    EXPECT_TRUE(member.is_member);
    EXPECT_TRUE(member.permissions.can_send_messages);
    EXPECT_FALSE(member.permissions.can_send_photos);
    EXPECT_TRUE(member.permissions.can_edit_tag);
    EXPECT_FALSE(member.rights.can_manage_tags);
    telebot_put_chat_member(&member);

    fake_->reply(200, R"({"ok":true,"result":{"status":"administrator","user":{"id":9,"is_bot":false,"first_name":"A"},
        "custom_title":"Boss","can_be_edited":true,"can_manage_tags":true,"can_delete_messages":true}})");
    ASSERT_EQ(telebot_get_chat_member(handle_, -100, 9, &member), TELEBOT_ERROR_NONE);
    EXPECT_STREQ(member.custom_title, "Boss");
    EXPECT_TRUE(member.rights.can_manage_tags);
    EXPECT_TRUE(member.rights.can_delete_messages);
    telebot_put_chat_member(&member);
}

TEST_F(RecentApiHighLevel, WebhookInfoMatchesUpdateNamesExactly)
{
    fake_->reply(200, R"({"ok":true,"result":{"url":"https://example.org","has_custom_certificate":false,
        "pending_update_count":0,"allowed_updates":["edited_message","message_reaction_count","no_such_type"]}})");
    telebot_webhook_info_t info;
    ASSERT_EQ(telebot_get_webhook_info(handle_, &info), TELEBOT_ERROR_NONE);
    ASSERT_EQ(info.allowed_updates_count, 2);
    EXPECT_EQ(info.allowed_updates[0], TELEBOT_UPDATE_TYPE_EDITED_MESSAGE);
    EXPECT_EQ(info.allowed_updates[1], TELEBOT_UPDATE_TYPE_MESSAGE_REACTION_COUNT);
    telebot_put_webhook_info(&info);
}

TEST(UpdateNames, EveryTypeHasAUniqueName)
{
    for (int i = 0; i < TELEBOT_UPDATE_TYPE_MAX; i++)
    {
        ASSERT_NE(telebot_update_type_names[i], nullptr) << "type " << i;
        EXPECT_EQ(telebot_update_type_from_name(telebot_update_type_names[i]), i);
    }
    EXPECT_EQ(telebot_update_type_from_name("message_reactio"), TELEBOT_UPDATE_TYPE_UNKNOWN);
    EXPECT_EQ(telebot_update_type_from_name(nullptr), TELEBOT_UPDATE_TYPE_UNKNOWN);
}

TEST(RecentApiParser, GuestMessageAndStoppedDraft)
{
    cJSON *root = telebot_parser_str_to_obj(R"([
        {"update_id":1,"guest_message":{"message_id":3,"date":1,"chat":{"id":5,"type":"private"},"text":"hey"}},
        {"update_id":2,"stopped_message_generation":{"chat":{"id":5,"type":"private"},"draft_id":77}}])");
    telebot_update_t *updates = nullptr;
    int count = 0;
    ASSERT_EQ(telebot_parser_get_updates(root, &updates, &count), TELEBOT_ERROR_NONE);
    ASSERT_EQ(count, 2);
    EXPECT_EQ(updates[0].update_type, TELEBOT_UPDATE_TYPE_GUEST_MESSAGE);
    EXPECT_STREQ(updates[0].guest_message.text, "hey");
    EXPECT_EQ(updates[1].update_type, TELEBOT_UPDATE_TYPE_STOPPED_MESSAGE_GENERATION);
    EXPECT_EQ(updates[1].stopped_message_generation.draft_id, 77);
    ASSERT_NE(updates[1].stopped_message_generation.chat, nullptr);
    EXPECT_EQ(updates[1].stopped_message_generation.chat->id, 5);
    telebot_put_updates(updates, count);
    telebot_json_put(root);
}

} // namespace

namespace
{

TEST_F(RecentApi, AnswerWidgetQuery)
{
    CoreResponse r(telebot_core_answer_widget_query(core_, "wq-1", "{\"ok\":1}"));
    EXPECT_EQ(last().method(), "answerWidgetQuery");
    EXPECT_EQ(last().at("widget_query_id"), "wq-1");

    CoreResponse missing(telebot_core_answer_widget_query(core_, nullptr, nullptr));
    EXPECT_EQ(missing.code(), TELEBOT_ERROR_INVALID_PARAMETER);
}

TEST(RecentApiParser, ManagedBotSubscriptionAndWidgets)
{
    cJSON *root = telebot_parser_str_to_obj(R"([
        {"update_id":1,"managed_bot":{"user":{"id":1,"is_bot":false,"first_name":"Owner"},
                                      "bot":{"id":2,"is_bot":true,"first_name":"Child","username":"child_bot"}}},
        {"update_id":2,"subscription":{"user":{"id":3,"is_bot":false,"first_name":"S"},
                                       "invoice_payload":"plan-pro","state":"canceled"}},
        {"update_id":3,"widget_query":{"id":"wq","from":{"id":4,"is_bot":false,"first_name":"W"},
            "chat":{"id":-100,"type":"supergroup","title":"G"},"message_id":12,"chat_instance":"ci",
            "type":{"type":"sign_data","sign_data":"{\"x\":1}",
                    "wallet":{"address":"EQabc","amount":"1000","public_key":"ab12"}},"data":"payload"}},
        {"update_id":4,"shared_widget_query":{"id":"sq","from":{"id":5,"is_bot":false,"first_name":"X"},
            "type":{"type":"data"},"data":"d","widget_id":"wid"}}])");
    telebot_update_t *updates = nullptr;
    int count = 0;
    ASSERT_EQ(telebot_parser_get_updates(root, &updates, &count), TELEBOT_ERROR_NONE);
    ASSERT_EQ(count, 4);

    EXPECT_EQ(updates[0].update_type, TELEBOT_UPDATE_TYPE_MANAGED_BOT);
    ASSERT_NE(updates[0].managed_bot.bot, nullptr);
    EXPECT_STREQ(updates[0].managed_bot.bot->username, "child_bot");

    if (TELEBOT_FEATURE_PAYMENTS)
    {
        EXPECT_EQ(updates[1].update_type, TELEBOT_UPDATE_TYPE_SUBSCRIPTION);
        EXPECT_STREQ(updates[1].subscription.state, "canceled");
        EXPECT_STREQ(updates[1].subscription.invoice_payload, "plan-pro");
    }
    else
    {
        EXPECT_EQ(updates[1].update_type, TELEBOT_UPDATE_TYPE_UNKNOWN);
    }

    if (TELEBOT_FEATURE_WIDGETS)
    {
        const telebot_widget_query_t &q = updates[2].widget_query;
        EXPECT_EQ(updates[2].update_type, TELEBOT_UPDATE_TYPE_WIDGET_QUERY);
        EXPECT_STREQ(q.query_type, "sign_data");
        EXPECT_STREQ(q.sign_data, "{\"x\":1}");
        ASSERT_NE(q.wallet, nullptr);
        EXPECT_STREQ(q.wallet->address, "EQabc");
        EXPECT_EQ(q.wallet->state_init, nullptr);
        EXPECT_EQ(q.message_id, 12);

        EXPECT_EQ(updates[3].update_type, TELEBOT_UPDATE_TYPE_SHARED_WIDGET_QUERY);
        EXPECT_STREQ(updates[3].shared_widget_query.widget_id, "wid");
        EXPECT_EQ(updates[3].shared_widget_query.wallet, nullptr);
    }
    else
    {
        EXPECT_EQ(updates[2].update_type, TELEBOT_UPDATE_TYPE_UNKNOWN);
    }

    telebot_put_updates(updates, count);
    telebot_json_put(root);
}

} // namespace

namespace
{

TEST_F(RecentApi, ProfilePhotoIsSentAsInputProfilePhoto)
{
    const char jpeg[] = "\xFF\xD8\xFF\xD9";
    const telebot_input_file_t photo = telebot_input_file_memory(jpeg, 4, "me.jpg");
    CoreResponse r(telebot_core_set_my_profile_photo(core_, &photo));
    ASSERT_EQ(r.code(), TELEBOT_ERROR_NONE);
    EXPECT_EQ(last().method(), "setMyProfilePhoto");
    EXPECT_EQ(last().at("photo"), R"({"type":"static","photo":"attach://file0"})");
    EXPECT_EQ(last().fields.at("file0").type, TELEBOT_MIME_TYPE_BUFFER);

    const telebot_input_file_t remote = telebot_input_file_remote("AgAC");
    CoreResponse bad(telebot_core_set_my_profile_photo(core_, &remote));
    EXPECT_EQ(bad.code(), TELEBOT_ERROR_INVALID_PARAMETER);
}

} // namespace
