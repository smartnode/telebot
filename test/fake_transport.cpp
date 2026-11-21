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

#include "fake_transport.h"

#include <cstring>

const telebot_transport_ops_t FakeTransport::ops_ = {
    "fake",
    &FakeTransport::perform,
    nullptr, // no proxy support, exercises the NOT_SUPPORTED path
    &FakeTransport::destroy,
};

FakeTransport *FakeTransport::install(telebot_core_handler_t core_h)
{
    auto *adapter = new Adapter{};
    adapter->base.ops = &ops_;
    adapter->self = new FakeTransport();
    telebot_core_set_transport(core_h, &adapter->base);
    return adapter->self;
}

telebot_error_e FakeTransport::perform(telebot_transport_t *t, const telebot_http_request_t *req,
                                       telebot_sink_t *sink, long *status)
{
    FakeTransport *self = reinterpret_cast<Adapter *>(t)->self;

    RecordedRequest rec;
    rec.url = req->url;
    for (size_t i = 0; i < req->count; i++)
    {
        const telebot_core_mime_t &part = req->parts[i];
        RecordedField field{part.type, "", ""};
        char scalar[TELEBOT_MIME_SCALAR_MAX];
        switch (part.type)
        {
        case TELEBOT_MIME_TYPE_STRING:
        case TELEBOT_MIME_TYPE_FILE:
            field.value = part.data.s ? part.data.s : "";
            break;
        case TELEBOT_MIME_TYPE_BUFFER:
            field.value.assign(static_cast<const char *>(part.data.buf.ptr), part.data.buf.len);
            field.filename = part.data.buf.filename ? part.data.buf.filename : "";
            break;
        default:
            field.value = telebot_mime_format_scalar(&part, scalar) ? scalar : "<invalid>";
            break;
        }
        rec.order.push_back(part.name);
        rec.fields[part.name] = field;
    }
    self->requests_.push_back(rec);

    ScriptedReply reply;
    if (!self->replies_.empty())
    {
        reply = self->replies_.front();
        self->replies_.pop_front();
    }

    *status = reply.status;
    if (reply.error != TELEBOT_ERROR_NONE)
        return reply.error;

    // Deliver the body in small chunks, like a real network stack would.
    const char *p = reply.body.data();
    size_t left = reply.body.size();
    while (left > 0)
    {
        size_t n = left < 7 ? left : 7;
        if (!telebot_sink_write(sink, p, n))
            return TELEBOT_ERROR_OPERATION_FAILED;
        p += n;
        left -= n;
    }
    return TELEBOT_ERROR_NONE;
}

void FakeTransport::destroy(telebot_transport_t *t)
{
    auto *adapter = reinterpret_cast<Adapter *>(t);
    delete adapter->self;
    delete adapter;
}
