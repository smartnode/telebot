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

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "fake_transport.h"

namespace
{

const char *kToken = "123456:TEST-TOKEN";
const std::string kBase = std::string("https://api.telegram.org/bot") + kToken + "/";

// Owns a telebot_core_response_t for the duration of a test.
struct Response
{
    explicit Response(telebot_core_response_t r) : r(r) {}
    ~Response() { telebot_core_put_response(r); }
    telebot_error_e code() const { return telebot_core_get_response_code(r); }
    std::string data() const
    {
        const char *d = telebot_core_get_response_data(r);
        return d ? d : "";
    }
    telebot_core_response_t r;
};

class CoreRequests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_EQ(telebot_core_create(&core_, kToken), TELEBOT_ERROR_NONE);
        fake_ = FakeTransport::install(core_);
    }
    void TearDown() override { telebot_core_destroy(&core_); }

    telebot_core_handler_t core_ = nullptr;
    FakeTransport *fake_ = nullptr;
};

TEST_F(CoreRequests, GetMeIsAGetWithoutFields)
{
    Response r(telebot_core_get_me(core_));
    EXPECT_EQ(r.code(), TELEBOT_ERROR_NONE);
    ASSERT_EQ(fake_->requests().size(), 1u);
    EXPECT_EQ(fake_->last().url, kBase + "getMe");
    EXPECT_TRUE(fake_->last().fields.empty());
}

TEST_F(CoreRequests, SendMessageBuildsExpectedFields)
{
    telebot_send_options_t opts{};
    opts.reply_markup = R"({"inline_keyboard":[]})";
    Response r(telebot_core_send_message(core_, -1001234567890LL, "hi *there*", "MarkdownV2", true, &opts));
    EXPECT_EQ(r.code(), TELEBOT_ERROR_NONE);

    const RecordedRequest &req = fake_->last();
    EXPECT_EQ(req.method(), "sendMessage");
    EXPECT_EQ(req.at("chat_id"), "-1001234567890");
    EXPECT_EQ(req.at("text"), "hi *there*");
    EXPECT_EQ(req.at("parse_mode"), "MarkdownV2");
    EXPECT_EQ(req.at("reply_markup"), R"({"inline_keyboard":[]})");
    EXPECT_FALSE(req.has("reply_to_message_id")); // 0 means "not a reply"
}

TEST_F(CoreRequests, OptionalFieldsAreOmitted)
{
    Response r(telebot_core_send_message(core_, 1, "x", nullptr, false, nullptr));
    EXPECT_FALSE(fake_->last().has("parse_mode"));
    EXPECT_FALSE(fake_->last().has("reply_markup"));
}

TEST_F(CoreRequests, InvalidArgumentsNeverReachTheNetwork)
{
    Response r(telebot_core_send_message(core_, 1, nullptr, nullptr, false, nullptr));
    EXPECT_EQ(r.code(), TELEBOT_ERROR_INVALID_PARAMETER);
    EXPECT_TRUE(fake_->requests().empty());
}

TEST_F(CoreRequests, HttpErrorKeepsTelegramDescription)
{
    fake_->reply(400, R"({"ok":false,"error_code":400,"description":"Bad Request: chat not found"})");
    Response r(telebot_core_send_message(core_, 1, "x", nullptr, false, nullptr));
    EXPECT_EQ(r.code(), TELEBOT_ERROR_OPERATION_FAILED);
    EXPECT_NE(r.data().find("chat not found"), std::string::npos);
}

TEST_F(CoreRequests, NetworkFailureIsReported)
{
    fake_->fail(TELEBOT_ERROR_NO_CONNECTION);
    Response r(telebot_core_get_me(core_));
    EXPECT_EQ(r.code(), TELEBOT_ERROR_NO_CONNECTION);
}

TEST_F(CoreRequests, SetWebhookSendsMaxConnectionsAndAllowedUpdates)
{
    Response r(telebot_core_set_webhook(core_, "https://example.org/hook", nullptr, 40, R"(["message"])", false, nullptr));
    const RecordedRequest &req = fake_->last();
    EXPECT_EQ(req.method(), "setWebhook");
    EXPECT_EQ(req.at("url"), "https://example.org/hook");
    ASSERT_TRUE(req.has("max_connections"));
    EXPECT_EQ(req.at("max_connections"), "40");
    ASSERT_TRUE(req.has("allowed_updates"));
    EXPECT_EQ(req.at("allowed_updates"), R"(["message"])");
}

TEST_F(CoreRequests, ProxyUnsupportedByBackendIsReported)
{
    EXPECT_EQ(telebot_core_set_proxy(core_, "socks5://127.0.0.1:1080", nullptr), TELEBOT_ERROR_NOT_SUPPORTED);
}

TEST_F(CoreRequests, DownloadFileStreamsToDisk)
{
    const std::string out = ::testing::TempDir() + "telebot_download.bin";
    fake_->reply(200, std::string("\x89PNG\r\n\x1a\n", 8) + "payload");

    EXPECT_EQ(telebot_core_download_file(core_, "photos/file_1.png", out.c_str()), TELEBOT_ERROR_NONE);
    EXPECT_EQ(fake_->last().url, std::string("https://api.telegram.org/file/bot") + kToken + "/photos/file_1.png");

    std::ifstream in(out, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    EXPECT_EQ(ss.str(), std::string("\x89PNG\r\n\x1a\n", 8) + "payload");
    std::remove(out.c_str());
}

TEST_F(CoreRequests, FailedDownloadRemovesPartialFile)
{
    const std::string out = ::testing::TempDir() + "telebot_download_404.bin";
    fake_->reply(404, R"({"ok":false,"error_code":404,"description":"Not Found"})");

    EXPECT_EQ(telebot_core_download_file(core_, "missing", out.c_str()), TELEBOT_ERROR_OPERATION_FAILED);
    std::ifstream in(out);
    EXPECT_FALSE(in.good());
}

TEST_F(CoreRequests, SendPhotoFromMemoryStreamsTheBuffer)
{
    static const char jpeg[] = {'\xFF', '\xD8', '\xFF', '\xE0', '\x00', '\x10', '\xFF', '\xD9'};
    const telebot_input_file_t input = telebot_input_file_memory(jpeg, sizeof(jpeg), "photo.jpg");
    telebot_send_options_t opts{};
    opts.disable_notification = true;
    Response r(telebot_core_send_photo(core_, 42, &input, "door camera", nullptr, &opts));
    EXPECT_EQ(r.code(), TELEBOT_ERROR_NONE);

    const RecordedRequest &req = fake_->last();
    EXPECT_EQ(req.method(), "sendPhoto");
    ASSERT_TRUE(req.has("photo"));
    const RecordedField &photo = req.fields.at("photo");
    EXPECT_EQ(photo.type, TELEBOT_MIME_TYPE_BUFFER);
    EXPECT_EQ(photo.value, std::string(jpeg, sizeof(jpeg)));
    EXPECT_EQ(photo.filename, "photo.jpg");
    EXPECT_EQ(req.at("caption"), "door camera");
    EXPECT_EQ(req.at("disable_notification"), "true");
    EXPECT_FALSE(req.has("parse_mode"));
}

TEST_F(CoreRequests, SendDocumentFromMemoryUsesGivenFilename)
{
    const std::string csv = "t,temp\n0,21.5\n";
    const telebot_input_file_t doc = telebot_input_file_memory(csv.data(), csv.size(), "log.csv");
    Response r(telebot_core_send_document(core_, 42, &doc, nullptr, nullptr, nullptr, nullptr));
    const RecordedRequest &req = fake_->last();
    EXPECT_EQ(req.method(), "sendDocument");
    EXPECT_EQ(req.fields.at("document").filename, "log.csv");
    EXPECT_EQ(req.at("document"), csv);
    EXPECT_FALSE(req.has("disable_notification")); // no options: nothing extra is sent
}

TEST_F(CoreRequests, EmptyBufferIsRejectedBeforeSending)
{
    const telebot_input_file_t empty = telebot_input_file_memory(nullptr, 0, "x.jpg");
    Response r(telebot_core_send_photo(core_, 42, &empty, nullptr, nullptr, nullptr));
    EXPECT_EQ(r.code(), TELEBOT_ERROR_INVALID_PARAMETER);
    EXPECT_TRUE(fake_->requests().empty());
}

// ----- High-level API through the same fake -----

class HighLevel : public ::testing::Test
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

TEST_F(HighLevel, GetUpdatesAdvancesOffset)
{
    fake_->reply(200, R"({"ok":true,"result":[
        {"update_id":10,"message":{"message_id":1,"date":1,"chat":{"id":5,"type":"private"},"text":"a"}},
        {"update_id":11,"message":{"message_id":2,"date":2,"chat":{"id":5,"type":"private"},"text":"b"}}]})");

    telebot_update_t *updates = nullptr;
    int count = 0;
    ASSERT_EQ(telebot_get_updates(handle_, 0, 5, 0, nullptr, 0, &updates, &count), TELEBOT_ERROR_NONE);
    ASSERT_EQ(count, 2);
    EXPECT_STREQ(updates[1].message.text, "b");
    EXPECT_EQ(fake_->last().at("limit"), "5");
    telebot_put_updates(updates, count);

    // The next poll must acknowledge everything seen so far.
    fake_->reply(200, R"({"ok":true,"result":[]})");
    telebot_get_updates(handle_, 0, 5, 0, nullptr, 0, &updates, &count);
    EXPECT_EQ(fake_->last().at("offset"), "12");
}

TEST_F(HighLevel, EmptyPollIsSuccessWithZeroUpdates)
{
    fake_->reply(200, R"({"ok":true,"result":[]})");
    telebot_update_t *updates = reinterpret_cast<telebot_update_t *>(0x1); // must be reset
    int count = -1;
    EXPECT_EQ(telebot_get_updates(handle_, 0, 0, 30, nullptr, 0, &updates, &count), TELEBOT_ERROR_NONE);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(updates, nullptr);
    EXPECT_EQ(fake_->last().at("timeout"), "30");
}

TEST_F(HighLevel, ZeroLimitUsesBuildDefault)
{
    fake_->reply(200, R"({"ok":true,"result":[]})");
    telebot_update_t *updates = nullptr;
    int count = 0;
    telebot_get_updates(handle_, 0, 0, 0, nullptr, 0, &updates, &count);
    EXPECT_EQ(fake_->last().at("limit"), std::to_string(TELEBOT_DEFAULT_UPDATE_LIMIT));
}

TEST_F(HighLevel, GetUpdatesSerialisesAllowedUpdates)
{
    fake_->reply(200, R"({"ok":true,"result":[]})");
    telebot_update_type_e types[] = {TELEBOT_UPDATE_TYPE_MESSAGE, TELEBOT_UPDATE_TYPE_CALLBACK_QUERY};
    telebot_update_t *updates = nullptr;
    int count = 0;
    telebot_get_updates(handle_, 0, 1, 0, types, 2, &updates, &count);
    EXPECT_EQ(fake_->last().at("allowed_updates"), R"(["message","callback_query"])");
}

TEST_F(HighLevel, SetWebhookSendsValidAllowedUpdatesJson)
{
    char url[] = "https://example.org/hook";
    telebot_update_type_e types[] = {TELEBOT_UPDATE_TYPE_MESSAGE, TELEBOT_UPDATE_TYPE_CALLBACK_QUERY};
    EXPECT_EQ(telebot_set_webhook(handle_, url, nullptr, 0, types, 2, false, nullptr), TELEBOT_ERROR_NONE);
    EXPECT_EQ(fake_->last().at("allowed_updates"), R"(["message","callback_query"])");
}

TEST_F(HighLevel, SetWebhookWithoutFilterOmitsAllowedUpdates)
{
    char url[] = "https://example.org/hook";
    EXPECT_EQ(telebot_set_webhook(handle_, url, nullptr, 0, nullptr, 0, false, nullptr), TELEBOT_ERROR_NONE);
    EXPECT_FALSE(fake_->last().has("allowed_updates"));
}

TEST_F(HighLevel, OutOfRangeUpdateTypeIsRejected)
{
    char url[] = "https://example.org/hook";
    telebot_update_type_e bad[] = {static_cast<telebot_update_type_e>(TELEBOT_UPDATE_TYPE_MAX + 5)};
    EXPECT_EQ(telebot_set_webhook(handle_, url, nullptr, 0, bad, 1, false, nullptr), TELEBOT_ERROR_INVALID_PARAMETER);

    telebot_update_t *updates = nullptr;
    int count = 0;
    EXPECT_EQ(telebot_get_updates(handle_, 0, 1, 0, bad, 1, &updates, &count), TELEBOT_ERROR_INVALID_PARAMETER);
    EXPECT_TRUE(fake_->requests().empty());
}

TEST_F(HighLevel, OkFalseIsAnError)
{
    fake_->reply(200, R"({"ok":false,"description":"nope"})");
    telebot_user_t me;
    EXPECT_NE(telebot_get_me(handle_, &me), TELEBOT_ERROR_NONE);
}

TEST_F(HighLevel, GetMeParsesUser)
{
    fake_->reply(200, R"({"ok":true,"result":{"id":8123456789,"is_bot":true,"first_name":"Probe","username":"probe_bot"}})");
    telebot_user_t me;
    ASSERT_EQ(telebot_get_me(handle_, &me), TELEBOT_ERROR_NONE);
    EXPECT_EQ(me.id, 8123456789LL);
    EXPECT_TRUE(me.is_bot);
    EXPECT_STREQ(me.username, "probe_bot");
    telebot_put_me(&me);
}

} // namespace

namespace
{

class Webhook : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_EQ(telebot_core_create(&core_, kToken), TELEBOT_ERROR_NONE);
        fake_ = FakeTransport::install(core_);
    }
    void TearDown() override { telebot_core_destroy(&core_); }

    telebot_core_handler_t core_ = nullptr;
    FakeTransport *fake_ = nullptr;
};

TEST_F(Webhook, CertificateIsUploadedAsFile)
{
    const telebot_input_file_t cert = telebot_input_file_path("/etc/bot/cert.pem");
    Response r(telebot_core_set_webhook(core_, "https://example.org/hook", &cert, 0, nullptr, true, "s3cr3t_token"));
    const RecordedRequest &req = fake_->last();
    ASSERT_TRUE(req.has("certificate"));
    EXPECT_EQ(req.fields.at("certificate").type, TELEBOT_MIME_TYPE_FILE);
    EXPECT_EQ(req.at("certificate"), "/etc/bot/cert.pem");
    EXPECT_FALSE(req.has("max_connections")); // 0 = use Telegram's default
    EXPECT_EQ(req.at("drop_pending_updates"), "true");
    EXPECT_EQ(req.at("secret_token"), "s3cr3t_token");
}

} // namespace

namespace
{

class RawCall : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_EQ(telebot_core_create(&core_, kToken), TELEBOT_ERROR_NONE);
        fake_ = FakeTransport::install(core_);
    }
    void TearDown() override { telebot_core_destroy(&core_); }

    telebot_core_handler_t core_ = nullptr;
    FakeTransport *fake_ = nullptr;
};

TEST_F(RawCall, SendsAnyMethodWithTextAndFileParameters)
{
    const char png[] = "\x89PNG";
    const telebot_input_file_t photo = telebot_input_file_memory(png, 4, "a.png");
    const telebot_param_t params[] = {
        {"chat_id", "-1001234567890", nullptr},
        {"tag", "moderator", nullptr},
        {"photo", nullptr, &photo},
    };
    Response r(telebot_core_call(core_, "setChatMemberTag", params, 3));
    ASSERT_EQ(r.code(), TELEBOT_ERROR_NONE);
    EXPECT_EQ(fake_->last().method(), "setChatMemberTag");
    EXPECT_EQ(fake_->last().at("tag"), "moderator");
    EXPECT_EQ(fake_->last().fields.at("photo").type, TELEBOT_MIME_TYPE_BUFFER);
}

TEST_F(RawCall, MalformedParametersAreRejectedLocally)
{
    const telebot_param_t both[] = {{"x", "1", nullptr}};
    const telebot_input_file_t f = telebot_input_file_remote("id");
    const telebot_param_t ambiguous[] = {{"x", "1", &f}};
    const telebot_param_t unnamed[] = {{nullptr, "1", nullptr}};

    Response ok(telebot_core_call(core_, "getMe", nullptr, 0));
    EXPECT_EQ(ok.code(), TELEBOT_ERROR_NONE);
    size_t sent = fake_->requests().size();
    Response a(telebot_core_call(core_, "x", ambiguous, 1));
    Response b(telebot_core_call(core_, "x", unnamed, 1));
    Response c(telebot_core_call(core_, "", both, 1));
    Response d(telebot_core_call(core_, "x", nullptr, 2));
    EXPECT_EQ(a.code(), TELEBOT_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(b.code(), TELEBOT_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(c.code(), TELEBOT_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(d.code(), TELEBOT_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(fake_->requests().size(), sent);
}

} // namespace

namespace
{

TEST_F(Webhook, CertificateMustBeAnUpload)
{
    const telebot_input_file_t remote = telebot_input_file_remote("https://example.org/cert.pem");
    Response r(telebot_core_set_webhook(core_, "https://example.org/hook", &remote, 0, nullptr, false, nullptr));
    EXPECT_EQ(r.code(), TELEBOT_ERROR_INVALID_PARAMETER);
    EXPECT_TRUE(fake_->requests().empty());
}

TEST_F(Webhook, EditMessageMediaUploadsNewContent)
{
    const char jpeg[] = "\xFF\xD8\xFF\xD9";
    telebot_input_media_t media{};
    media.type = TELEBOT_INPUT_MEDIA_PHOTO;
    media.file = telebot_input_file_memory(jpeg, 4, "new.jpg");
    media.caption = "updated";
    Response r(telebot_core_edit_message_media(core_, 5, 10, nullptr, &media, nullptr));
    ASSERT_EQ(r.code(), TELEBOT_ERROR_NONE);
    const RecordedRequest &req = fake_->last();
    EXPECT_EQ(req.method(), "editMessageMedia");
    EXPECT_EQ(req.at("media"), R"({"type":"photo","media":"attach://file0","caption":"updated"})");
    EXPECT_EQ(req.fields.at("file0").type, TELEBOT_MIME_TYPE_BUFFER);
    EXPECT_EQ(req.at("message_id"), "10");

    media.file = telebot_input_file_remote("AgACAgIAAxkB");
    Response remote(telebot_core_edit_message_media(core_, 0, 0, "inl", &media, nullptr));
    EXPECT_EQ(fake_->last().at("media"), R"({"type":"photo","media":"AgACAgIAAxkB","caption":"updated"})");
    EXPECT_FALSE(fake_->last().has("file0"));

    Response none(telebot_core_edit_message_media(core_, 5, 10, nullptr, nullptr, nullptr));
    EXPECT_EQ(none.code(), TELEBOT_ERROR_INVALID_PARAMETER);
}

TEST_F(Webhook, MigratedGroupReportsNewChatId)
{
    fake_->reply(400, R"({"ok":false,"error_code":400,"description":"Bad Request: group chat was upgraded",)"
                      R"("parameters":{"migrate_to_chat_id":-1001234567890}})");
    Response r(telebot_core_get_me(core_));
    EXPECT_EQ(r.code(), TELEBOT_ERROR_OPERATION_FAILED);
    EXPECT_EQ(telebot_core_get_response_migrate_to_chat_id(r.r), -1001234567890LL);
    EXPECT_EQ(telebot_core_get_response_retry_after(r.r), 0);
}

} // namespace
