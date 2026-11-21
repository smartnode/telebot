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

#include <gtest/gtest.h>

#include <cstring>

#include "telebot_c.h"

namespace
{

const char *kUpdates =
    R"({"ok":true,"result":[
      {"update_id":900000001,"message":{
        "message_id":42,"message_thread_id":7,"is_topic_message":true,"sender_tag":"lab-admin",
        "sticker":{"file_id":"CAAC","file_unique_id":"u","type":"regular","width":512,"height":512,
          "is_animated":false,"is_video":true,"emoji":"👍","thumbnail":{"file_id":"T","file_unique_id":"t","width":128,"height":128}},
        "from":{"id":7123456789,"is_bot":false,"first_name":"Ada","last_name":null,"username":"ada"},
        "chat":{"id":-1001234567890,"type":"supergroup","title":"Lab"},
        "date":1763700000,"text":"/start hello",
        "entities":[{"offset":0,"length":6,"type":"bot_command"}]}},
      {"update_id":900000002,"message":{
        "message_id":43,"chat":{"id":7123456789,"type":"private","first_name":"Ada"},
        "date":1763700001,"photo":[
          {"file_id":"A","file_unique_id":"a","width":90,"height":60,"file_size":1200},
          {"file_id":"B","file_unique_id":"b","width":800,"height":600,"file_size":64000}]}},
      {"update_id":900000003,"widget_query":{"id":"w1"}},
      {"update_id":900000004,"callback_query":{"id":"cbq-1",
        "from":{"id":7123456789,"is_bot":false,"first_name":"Ada"},
        "chat_instance":"ci","data":"btn:1"}}
    ]})";

// Parses kUpdates once per test and releases everything afterwards.
class ParserUpdates : public ::testing::Test
{
protected:
    void SetUp() override
    {
        root_ = telebot_parser_str_to_obj(kUpdates);
        ASSERT_NE(root_, nullptr);
        cJSON *result = nullptr;
        ASSERT_TRUE(tb_json_get_ex(root_, "result", &result));
        ASSERT_EQ(telebot_parser_get_updates(result, &updates_, &count_), TELEBOT_ERROR_NONE);
        ASSERT_EQ(count_, 4);
    }

    void TearDown() override
    {
        if (updates_)
            telebot_put_updates(updates_, count_);
        tb_json_put(root_);
    }

    cJSON *root_ = nullptr;
    telebot_update_t *updates_ = nullptr;
    int count_ = 0;
};

TEST_F(ParserUpdates, TextMessageInForumTopic)
{
    const telebot_update_t &u = updates_[0];
    EXPECT_EQ(u.update_id, 900000001);
    EXPECT_EQ(u.update_type, TELEBOT_UPDATE_TYPE_MESSAGE);

    const telebot_message_t &m = u.message;
    EXPECT_EQ(m.message_id, 42);
    EXPECT_EQ(m.message_thread_id, 7);
    EXPECT_EQ(m.date, 1763700000L);
    ASSERT_NE(m.text, nullptr);
    EXPECT_STREQ(m.text, "/start hello");
    ASSERT_EQ(m.count_entities, 1);
    EXPECT_EQ(m.entities[0].length, 6);
}

TEST_F(ParserUpdates, TopicSenderTagAndSticker)
{
    const telebot_message_t &m = updates_[0].message;
    EXPECT_TRUE(m.is_topic_message);
    ASSERT_NE(m.sender_tag, nullptr);
    EXPECT_STREQ(m.sender_tag, "lab-admin");
    ASSERT_NE(m.sticker, nullptr);
    EXPECT_STREQ(m.sticker->file_id, "CAAC");
    EXPECT_STREQ(m.sticker->type, "regular");
    EXPECT_TRUE(m.sticker->is_video);
    EXPECT_STREQ(m.sticker->emoji, "👍");
    ASSERT_NE(m.sticker->thumbnail, nullptr);
    EXPECT_EQ(m.sticker->thumbnail->width, 128);
}

TEST_F(ParserUpdates, IdentifiersBeyond32BitsAreExact)
{
    const telebot_message_t &m = updates_[0].message;
    ASSERT_NE(m.from, nullptr);
    EXPECT_EQ(m.from->id, 7123456789LL);
    ASSERT_NE(m.chat, nullptr);
    EXPECT_EQ(m.chat->id, -1001234567890LL);
}

TEST_F(ParserUpdates, JsonNullIsTreatedAsAbsent)
{
    const telebot_message_t &m = updates_[0].message;
    ASSERT_NE(m.from, nullptr);
    EXPECT_EQ(m.from->last_name, nullptr);
    EXPECT_STREQ(m.from->username, "ada");
}

TEST_F(ParserUpdates, PhotoSizes)
{
    const telebot_message_t &m = updates_[1].message;
    ASSERT_EQ(m.count_photos, 2);
    EXPECT_EQ(m.photos[1].width, 800);
    EXPECT_EQ(m.photos[1].file_size, 64000);
    EXPECT_STREQ(m.photos[1].file_id, "B");
}

TEST_F(ParserUpdates, UnknownUpdateTypeDoesNotAbortBatch)
{
    EXPECT_EQ(updates_[2].update_id, 900000003);
    EXPECT_EQ(updates_[2].update_type, TELEBOT_UPDATE_TYPE_UNKNOWN);
    EXPECT_EQ(updates_[3].update_type, TELEBOT_UPDATE_TYPE_CALLBACK_QUERY);
}

TEST_F(ParserUpdates, CallbackQuery)
{
    const telebot_callback_query_t &q = updates_[3].callback_query;
    EXPECT_STREQ(q.data, "btn:1");
    ASSERT_NE(q.from, nullptr);
    EXPECT_EQ(q.from->id, 7123456789LL);
}

TEST(Parser, RejectsInvalidJson)
{
    EXPECT_EQ(telebot_parser_str_to_obj(R"({"ok":tru)"), nullptr);
    EXPECT_EQ(telebot_parser_str_to_obj(nullptr), nullptr);
}

TEST(JsonAccessors, TypeMismatchYieldsDefaults)
{
    cJSON *root = cJSON_Parse(R"({"n":"12","s":5,"b":1,"arr":{}})");
    ASSERT_NE(root, nullptr);
    cJSON *v = nullptr;

    ASSERT_TRUE(tb_json_get_ex(root, "n", &v));
    EXPECT_EQ(tb_json_get_int(v), 0); // strings are not coerced to numbers
    ASSERT_TRUE(tb_json_get_ex(root, "s", &v));
    EXPECT_EQ(tb_json_get_string(v), nullptr);
    ASSERT_TRUE(tb_json_get_ex(root, "b", &v));
    EXPECT_FALSE(tb_json_get_bool(v)); // only JSON true is true
    ASSERT_TRUE(tb_json_get_ex(root, "arr", &v));
    EXPECT_EQ(tb_json_array_length(v), 0);
    EXPECT_FALSE(tb_json_get_ex(root, "missing", &v));
    EXPECT_FALSE(tb_json_get_ex(nullptr, "n", &v));

    tb_json_put(root);
}

} // namespace

namespace
{

// A disabled feature group must not produce half-parsed or mislabelled updates.
TEST(ParserFeatures, GatedUpdatesFollowBuildConfiguration)
{
    cJSON *root = telebot_parser_str_to_obj(R"([
        {"update_id":1,"chat_boost":{"chat":{"id":5,"type":"channel"},
         "boost":{"boost_id":"b","add_date":1,"expiration_date":2,"source":{"source":"premium"}}}},
        {"update_id":2,"pre_checkout_query":{"id":"q","from":{"id":9,"is_bot":false,"first_name":"P"},
         "currency":"XTR","total_amount":5,"invoice_payload":"p"}}])");
    ASSERT_NE(root, nullptr);

    telebot_update_t *updates = nullptr;
    int count = 0;
    ASSERT_EQ(telebot_parser_get_updates(root, &updates, &count), TELEBOT_ERROR_NONE);
    ASSERT_EQ(count, 2);

    EXPECT_EQ(updates[0].update_type,
              TELEBOT_FEATURE_CHAT_EXTRAS ? TELEBOT_UPDATE_TYPE_CHAT_BOOST : TELEBOT_UPDATE_TYPE_UNKNOWN);
    EXPECT_EQ(updates[1].update_type,
              TELEBOT_FEATURE_PAYMENTS ? TELEBOT_UPDATE_TYPE_PRE_CHECKOUT_QUERY : TELEBOT_UPDATE_TYPE_UNKNOWN);

    telebot_put_updates(updates, count);
    tb_json_put(root);
}

} // namespace

namespace
{

TEST(ParserStickers, StickerSet)
{
    cJSON *root = telebot_parser_str_to_obj(R"({"name":"pack_by_bot","title":"Pack","sticker_type":"regular",
        "stickers":[{"file_id":"A","file_unique_id":"a","type":"regular","width":512,"height":512,"is_animated":false,"is_video":false},
                    {"file_id":"B","file_unique_id":"b","type":"regular","width":512,"height":512,"is_animated":true,"is_video":false}],
        "thumbnail":{"file_id":"T","file_unique_id":"t","width":100,"height":100}})");
    ASSERT_NE(root, nullptr);
    telebot_sticker_set_t set;
    ASSERT_EQ(telebot_parser_get_sticker_set(root, &set), TELEBOT_ERROR_NONE);
    EXPECT_STREQ(set.name, "pack_by_bot");
    EXPECT_STREQ(set.sticker_type, "regular");
    ASSERT_EQ(set.count_stickers, 2);
    EXPECT_TRUE(set.stickers[1].is_animated);
    ASSERT_NE(set.thumbnail, nullptr);
    telebot_put_sticker_set(&set);
    EXPECT_EQ(set.stickers, nullptr);
    tb_json_put(root);
}

TEST(ParserPermissions, AllCurrentFieldsAreRead)
{
    cJSON *root = telebot_parser_str_to_obj(
        R"({"can_send_messages":true,"can_send_voice_notes":true,"can_react_to_messages":true,"can_manage_topics":false})");
    telebot_chat_permissions_t p;
    ASSERT_EQ(telebot_parser_get_chat_permissions(root, &p), TELEBOT_ERROR_NONE);
    EXPECT_TRUE(p.can_send_messages);
    EXPECT_TRUE(p.can_send_voice_notes);
    EXPECT_TRUE(p.can_react_to_messages);
    EXPECT_FALSE(p.can_manage_topics);
    EXPECT_FALSE(p.can_send_photos);
    tb_json_put(root);
}

} // namespace

namespace
{

// A message carrying most optional sub-objects; run under ASan/leaks to check
// that telebot_put_updates() releases everything the parser allocates.
TEST(ParserOwnership, RichMessageIsFullyReleased)
{
    cJSON *root = telebot_parser_str_to_obj(R"([{"update_id":5,"message":{
        "message_id":1,"date":1,"chat":{"id":-100,"type":"supergroup","title":"T"},
        "business_connection_id":"bc","effect_id":"e1","sender_tag":"tag",
        "sender_business_bot":{"id":2,"is_bot":true,"first_name":"B"},
        "forward_origin":{"type":"user","date":1,"sender_user":{"id":3,"is_bot":false,"first_name":"F"}},
        "external_reply":{"origin":{"type":"hidden_user","date":1,"sender_user_name":"H"},
                          "chat":{"id":-200,"type":"channel","title":"C"},"message_id":9},
        "quote":{"text":"quoted","position":0,"is_manual":true},
        "reply_to_story":{"chat":{"id":-300,"type":"channel","title":"S"},"id":4},
        "link_preview_options":{"is_disabled":false,"url":"https://example.org"},
        "write_access_allowed":{"web_app_name":"app"},
        "users_shared":{"request_id":1,"users":[{"user_id":7123456789,"first_name":"U","username":"u"}]},
        "chat_background_set":{"type":{"type":"fill"}},
        "text":"hello"}}])");
    ASSERT_NE(root, nullptr);

    telebot_update_t *updates = nullptr;
    int count = 0;
    ASSERT_EQ(telebot_parser_get_updates(root, &updates, &count), TELEBOT_ERROR_NONE);
    ASSERT_EQ(count, 1);
    const telebot_message_t &m = updates[0].message;
    ASSERT_NE(m.forward_origin, nullptr);
    EXPECT_STREQ(m.forward_origin->type, "user");
    ASSERT_NE(m.quote, nullptr);
    EXPECT_STREQ(m.quote->text, "quoted");
    ASSERT_NE(m.link_preview_options, nullptr);
    EXPECT_STREQ(m.link_preview_options->url, "https://example.org");

    telebot_put_updates(updates, count);
    tb_json_put(root);
}

} // namespace
