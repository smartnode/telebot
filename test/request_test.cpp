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

#include <string>

#include "telebot_c.h"

extern "C" {
#include "telebot-request.h"
}

namespace
{

class RequestBuilder : public ::testing::Test
{
protected:
    void SetUp() override { telebot_request_init(&req_); }

    const telebot_core_mime_t *find(const std::string &name) const
    {
        for (size_t i = 0; i < req_.count; i++)
            if (name == req_.parts[i].name)
                return &req_.parts[i];
        return nullptr;
    }

    telebot_request_t req_;
};

TEST_F(RequestBuilder, OptionalValuesAreSkipped)
{
    tb_req_str(&req_, "caption", nullptr);
    tb_req_id(&req_, "reply_to_message_id", 0);
    tb_req_id(&req_, "message_thread_id", -3);
    tb_req_flag(&req_, "disable_notification", false);
    tb_req_file(&req_, "thumbnail", nullptr);
    EXPECT_EQ(req_.count, 0u);
    EXPECT_EQ(req_.error, TELEBOT_ERROR_NONE);
}

TEST_F(RequestBuilder, PresentValuesKeepTheirTypes)
{
    tb_req_ll(&req_, "chat_id", -1001234567890LL);
    tb_req_id(&req_, "reply_to_message_id", 7);
    tb_req_flag(&req_, "protect_content", true);
    tb_req_bool(&req_, "is_anonymous", false); // always sent
    tb_req_str(&req_, "text", "hi");

    ASSERT_EQ(req_.count, 5u);
    EXPECT_EQ(find("chat_id")->data.lld, -1001234567890LL);
    EXPECT_EQ(find("reply_to_message_id")->data.lld, 7);
    EXPECT_EQ(find("protect_content")->type, TELEBOT_MIME_TYPE_BOOL);
    EXPECT_FALSE(find("is_anonymous")->data.b);
    EXPECT_STREQ(find("text")->data.s, "hi");
}

TEST_F(RequestBuilder, RequiredValuesRecordAnError)
{
    tb_req_str_required(&req_, "text", nullptr);
    EXPECT_EQ(req_.error, TELEBOT_ERROR_INVALID_PARAMETER);

    telebot_request_init(&req_);
    tb_req_file_required(&req_, "photo", nullptr);
    EXPECT_EQ(req_.error, TELEBOT_ERROR_INVALID_PARAMETER);
}

TEST_F(RequestBuilder, OverflowIsAnErrorNotAMemoryCorruption)
{
    for (int i = 0; i < TELEBOT_REQUEST_MAX_PARTS + 3; i++)
        tb_req_int(&req_, "x", i);
    EXPECT_EQ(req_.count, static_cast<size_t>(TELEBOT_REQUEST_MAX_PARTS));
    EXPECT_EQ(req_.error, TELEBOT_ERROR_INVALID_PARAMETER);
}

TEST_F(RequestBuilder, InputFileKindsMapToWireTypes)
{
    const telebot_input_file_t remote = telebot_input_file_remote("AgACAgIAAxkB");
    const telebot_input_file_t path = telebot_input_file_path("/spiffs/cam.jpg");
    const unsigned char bytes[] = {1, 2, 3};
    const telebot_input_file_t mem = telebot_input_file_memory(bytes, sizeof(bytes), "x.bin");

    tb_req_file(&req_, "a", &remote);
    tb_req_file(&req_, "b", &path);
    tb_req_file(&req_, "c", &mem);

    ASSERT_EQ(req_.error, TELEBOT_ERROR_NONE);
    EXPECT_EQ(find("a")->type, TELEBOT_MIME_TYPE_STRING);
    EXPECT_EQ(find("b")->type, TELEBOT_MIME_TYPE_FILE);
    EXPECT_EQ(find("c")->type, TELEBOT_MIME_TYPE_BUFFER);
    EXPECT_EQ(find("c")->data.buf.len, 3u);
    EXPECT_STREQ(find("c")->data.buf.filename, "x.bin");
}

TEST_F(RequestBuilder, EmptyInputFilesAreRejected)
{
    const telebot_input_file_t empty_ref = telebot_input_file_path("");
    tb_req_file(&req_, "doc", &empty_ref);
    EXPECT_EQ(req_.error, TELEBOT_ERROR_INVALID_PARAMETER);

    telebot_request_init(&req_);
    const telebot_input_file_t empty_mem = telebot_input_file_memory("x", 0, "x");
    tb_req_file(&req_, "doc", &empty_mem);
    EXPECT_EQ(req_.error, TELEBOT_ERROR_INVALID_PARAMETER);
}

} // namespace
