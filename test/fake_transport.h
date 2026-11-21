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
 * FakeTransport: an in-memory implementation of the telebot_transport_t ADT.
 *
 * It records every request the core issues (URL + decoded form fields) and
 * replies with a scripted status/body, so API calls can be tested without a
 * network connection or a bot token.
 */
#pragma once

#include <deque>
#include <map>
#include <string>
#include <vector>

#include "telebot_c.h"

struct RecordedField
{
    telebot_core_mime_e type;
    std::string value;    // scalar/string value, file path, or buffer bytes
    std::string filename; // BUFFER only
};

struct RecordedRequest
{
    std::string url;
    std::vector<std::string> order; // field names in the order they were sent
    std::map<std::string, RecordedField> fields;

    bool has(const std::string &name) const { return fields.count(name) != 0; }
    const std::string &at(const std::string &name) const { return fields.at(name).value; }
    std::string method() const { return url.substr(url.rfind('/') + 1); }
};

struct ScriptedReply
{
    long status = 200;
    std::string body = "{\"ok\":true,\"result\":true}";
    telebot_error_e error = TELEBOT_ERROR_NONE; // simulate a network failure
};

class FakeTransport
{
public:
    // Installs a fake into the core handler, which takes ownership and frees it on destroy.
    // The returned pointer stays valid until the handler destroys it.
    static FakeTransport *install(telebot_core_handler_t core_h);

    void reply(long status, const std::string &body) { replies_.push_back({status, body, TELEBOT_ERROR_NONE}); }
    void fail(telebot_error_e error) { replies_.push_back({0, "", error}); }

    const std::vector<RecordedRequest> &requests() const { return requests_; }
    const RecordedRequest &last() const { return requests_.back(); }

private:
    struct Adapter
    {
        telebot_transport_t base; // must stay first: the core sees only this
        FakeTransport *self;
    };

    static telebot_error_e perform(telebot_transport_t *t, const telebot_http_request_t *req,
                                   telebot_sink_t *sink, long *status);
    static void destroy(telebot_transport_t *t);

    static const telebot_transport_ops_t ops_;

    std::deque<ScriptedReply> replies_;
    std::vector<RecordedRequest> requests_;
};
