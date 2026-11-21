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

/* Request building for the message API (telebot-core-messages.c). */
#include "core_fixture.h"

namespace
{

using Messages = CoreTest;

TEST_F(Messages, SendOptionsExpandToTheirWireFields)
{
    telebot_send_options_t opts{};
    opts.message_thread_id = 7;
    opts.reply_to_message_id = 42;
    opts.allow_sending_without_reply = true;
    opts.protect_content = true;
    opts.reply_markup = R"({"inline_keyboard":[]})";
    opts.business_connection_id = "bc1";
    opts.message_effect_id = "5104841245755180586";

    CoreResponse r(telebot_core_send_message(core_, 5, "hi", nullptr, false, &opts));
    ASSERT_EQ(r.code(), TELEBOT_ERROR_NONE);
    EXPECT_EQ(last().at("message_thread_id"), "7");
    EXPECT_EQ(last().at("reply_parameters"), R"({"message_id":42,"allow_sending_without_reply":true})");
    EXPECT_FALSE(last().has("reply_to_message_id"));
    EXPECT_EQ(last().at("protect_content"), "true");
    EXPECT_FALSE(last().has("disable_notification"));
    EXPECT_EQ(last().at("reply_markup"), R"({"inline_keyboard":[]})");
    EXPECT_EQ(last().at("business_connection_id"), "bc1");
    EXPECT_EQ(last().at("message_effect_id"), "5104841245755180586");
}

TEST_F(Messages, LinkPreviewUsesLinkPreviewOptions)
{
    CoreResponse r(telebot_core_send_message(core_, 5, "https://example.org", nullptr, true, nullptr));
    EXPECT_EQ(last().at("link_preview_options"), R"({"is_disabled":true})");
    EXPECT_FALSE(last().has("disable_web_page_preview"));
}

TEST_F(Messages, InputFileKindsReachTheWire)
{
    const telebot_input_file_t remote = telebot_input_file_remote("AgACAgIAAxkBAAI");
    CoreResponse a(telebot_core_send_photo(core_, 5, &remote, nullptr, nullptr, nullptr));
    EXPECT_EQ(last().fields.at("photo").type, TELEBOT_MIME_TYPE_STRING);

    const telebot_input_file_t path = telebot_input_file_path("/spiffs/doc.pdf");
    CoreResponse b(telebot_core_send_document(core_, 5, &path, nullptr, nullptr, nullptr, nullptr));
    EXPECT_EQ(last().fields.at("document").type, TELEBOT_MIME_TYPE_FILE);
    EXPECT_EQ(last().at("document"), "/spiffs/doc.pdf");
}

TEST_F(Messages, ThumbnailUsesCurrentFieldNameAndMustBeUploaded)
{
    const telebot_input_file_t video = telebot_input_file_remote("BAACAgIAAxkB");
    const char jpeg[] = "\xFF\xD8\xFF\xD9";
    const telebot_input_file_t thumb = telebot_input_file_memory(jpeg, 4, "t.jpg");
    CoreResponse r(telebot_core_send_video(core_, 5, &video, 3, 640, 480, &thumb, nullptr, nullptr, true, nullptr));
    ASSERT_EQ(r.code(), TELEBOT_ERROR_NONE);
    EXPECT_TRUE(last().has("thumbnail"));
    EXPECT_FALSE(last().has("thumb"));
    EXPECT_EQ(last().at("supports_streaming"), "true");

    const telebot_input_file_t remote_thumb = telebot_input_file_remote("AAQ");
    size_t sent = fake_->requests().size();
    CoreResponse bad(telebot_core_send_video(core_, 5, &video, 0, 0, 0, &remote_thumb, nullptr, nullptr, false, nullptr));
    EXPECT_EQ(bad.code(), TELEBOT_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(fake_->requests().size(), sent);
}

TEST_F(Messages, SendStickerIsImplemented)
{
    const telebot_input_file_t sticker = telebot_input_file_remote("CAACAgIAAxkB");
    CoreResponse r(telebot_core_send_sticker(core_, 5, &sticker, nullptr, nullptr));
    EXPECT_EQ(last().method(), "sendSticker");
    EXPECT_EQ(last().at("sticker"), "CAACAgIAAxkB");
}

TEST_F(Messages, MediaGroupMixesUploadsAndFileIds)
{
    const char bytes[] = "jpegdata";
    telebot_input_media_t album[2] = {};
    album[0].type = TELEBOT_INPUT_MEDIA_PHOTO;
    album[0].file = telebot_input_file_memory(bytes, 8, "a.jpg");
    album[0].caption = "first";
    album[1].type = TELEBOT_INPUT_MEDIA_VIDEO;
    album[1].file = telebot_input_file_remote("BAACAgIAAxkB");

    telebot_send_options_t opts{};
    opts.reply_markup = "{}"; // albums cannot carry a keyboard: must be dropped
    CoreResponse r(telebot_core_send_media_group(core_, 5, album, 2, &opts));
    ASSERT_EQ(r.code(), TELEBOT_ERROR_NONE);
    EXPECT_EQ(last().at("media"),
              R"([{"type":"photo","media":"attach://file0","caption":"first"},)"
              R"({"type":"video","media":"BAACAgIAAxkB"}])");
    EXPECT_EQ(last().fields.at("file0").type, TELEBOT_MIME_TYPE_BUFFER);
    EXPECT_FALSE(last().has("file1"));
    EXPECT_FALSE(last().has("reply_markup"));
}

TEST_F(Messages, MediaGroupNeedsTwoToTenItems)
{
    telebot_input_media_t one[1] = {};
    one[0].file = telebot_input_file_remote("x");
    CoreResponse r(telebot_core_send_media_group(core_, 5, one, 1, nullptr));
    EXPECT_EQ(r.code(), TELEBOT_ERROR_INVALID_PARAMETER);
    EXPECT_TRUE(nothing_sent());
}

TEST_F(Messages, PollUsesInputPollOptionsAndExplicitAnonymity)
{
    const char *answers[] = {"Red", "Blue \"navy\""};
    const int correct[] = {1};
    CoreResponse r(telebot_core_send_poll(core_, 5, "Colour?", answers, 2, false, "quiz", false, correct, 1,
                                          false, nullptr));
    ASSERT_EQ(r.code(), TELEBOT_ERROR_NONE);
    EXPECT_EQ(last().at("options"), R"([{"text":"Red"},{"text":"Blue \"navy\""}])");
    EXPECT_EQ(last().at("is_anonymous"), "false");
    EXPECT_EQ(last().at("correct_option_ids"), "[1]");
    EXPECT_FALSE(last().has("correct_option_id"));
}

TEST_F(Messages, LocationKeepsDoublePrecision)
{
    CoreResponse r(telebot_core_send_location(core_, 5, 41.311081, 69.240562, 0, nullptr));
    EXPECT_EQ(last().at("latitude"), "41.311081");
    EXPECT_EQ(last().at("longitude"), "69.240562");
    EXPECT_FALSE(last().has("live_period"));
}

TEST_F(Messages, ForwardMessagesSendsIdArray)
{
    const int ids[] = {10, 11, 12};
    CoreResponse r(telebot_core_forward_messages(core_, 5, 6, ids, 3, nullptr));
    EXPECT_EQ(last().method(), "forwardMessages");
    EXPECT_EQ(last().at("message_ids"), "[10,11,12]");

    CoreResponse none(telebot_core_forward_messages(core_, 5, 6, ids, 0, nullptr));
    EXPECT_EQ(none.code(), TELEBOT_ERROR_INVALID_PARAMETER);
}

TEST_F(Messages, ReactionIsBuiltFromAnEmoji)
{
    CoreResponse r(telebot_core_set_message_reaction(core_, 5, 9, "👍", false));
    EXPECT_EQ(last().at("reaction"), R"([{"type":"emoji","emoji":"👍"}])");

    CoreResponse clear(telebot_core_set_message_reaction(core_, 5, 9, nullptr, false));
    EXPECT_EQ(last().at("reaction"), "[]");

    CoreResponse bad(telebot_core_set_message_reaction(core_, 5, 9, "\"}]", false));
    EXPECT_EQ(bad.code(), TELEBOT_ERROR_INVALID_PARAMETER);
}

TEST_F(Messages, EditTargetsEitherChatMessageOrInlineMessage)
{
    CoreResponse inl(telebot_core_edit_message_text(core_, 0, 0, "inl-1", "new", nullptr, false, nullptr));
    EXPECT_EQ(last().at("inline_message_id"), "inl-1");
    EXPECT_FALSE(last().has("chat_id"));

    CoreResponse bad(telebot_core_edit_message_text(core_, 5, 0, nullptr, "new", nullptr, false, nullptr));
    EXPECT_EQ(bad.code(), TELEBOT_ERROR_INVALID_PARAMETER);
}

} // namespace
