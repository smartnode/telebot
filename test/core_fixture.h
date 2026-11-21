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

/* Shared fixture: a core handler whose HTTP backend is a FakeTransport. */
#pragma once

#include <gtest/gtest.h>

#include <string>

#include "fake_transport.h"

// Owns a telebot_core_response_t for the duration of a test.
struct CoreResponse
{
    explicit CoreResponse(telebot_core_response_t r) : r(r) {}
    ~CoreResponse() { telebot_core_put_response(r); }
    CoreResponse(const CoreResponse &) = delete;
    CoreResponse &operator=(const CoreResponse &) = delete;
    telebot_error_e code() const { return telebot_core_get_response_code(r); }
    telebot_core_response_t r;
};

class CoreTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_EQ(telebot_core_create(&core_, "123456:TEST-TOKEN"), TELEBOT_ERROR_NONE);
        fake_ = FakeTransport::install(core_);
    }
    void TearDown() override { telebot_core_destroy(&core_); }

    const RecordedRequest &last() const { return fake_->last(); }
    bool nothing_sent() const { return fake_->requests().empty(); }

    telebot_core_handler_t core_ = nullptr;
    FakeTransport *fake_ = nullptr;
};
