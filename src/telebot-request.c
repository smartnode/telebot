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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "telebot-fields.h"
#include "telebot-request.h"

void telebot_request_init(telebot_request_t *req)
{
    req->count = 0;
    req->error = TELEBOT_ERROR_NONE;
    req->scratch_used = 0;
    req->owned_count = 0;
}

static void fail(telebot_request_t *req, telebot_error_e error, const char *name, const char *why)
{
    if (req->error == TELEBOT_ERROR_NONE)
    {
        ERR("Field '%s': %s", name, why);
        req->error = error;
    }
}

/* Reserves the next slot, or records an overflow. */
static telebot_core_mime_t *slot(telebot_request_t *req, const char *name, telebot_core_mime_e type)
{
    if (req->count >= TELEBOT_REQUEST_MAX_PARTS)
    {
        fail(req, TELEBOT_ERROR_INVALID_PARAMETER, name, "too many fields (TELEBOT_REQUEST_MAX_PARTS)");
        return NULL;
    }
    telebot_core_mime_t *part = &req->parts[req->count++];
    part->name = name;
    part->type = type;
    return part;
}

void telebot_request_int(telebot_request_t *req, const char *name, int value)
{
    telebot_core_mime_t *p = slot(req, name, TELEBOT_MIME_TYPE_INT);
    if (p)
        p->data.d = value;
}

void telebot_request_ll(telebot_request_t *req, const char *name, long long value)
{
    telebot_core_mime_t *p = slot(req, name, TELEBOT_MIME_TYPE_LONG_LONG_INT);
    if (p)
        p->data.lld = value;
}

void telebot_request_double(telebot_request_t *req, const char *name, double value)
{
    telebot_core_mime_t *p = slot(req, name, TELEBOT_MIME_TYPE_DOUBLE);
    if (p)
        p->data.lf = value;
}

void telebot_request_bool(telebot_request_t *req, const char *name, bool value)
{
    telebot_core_mime_t *p = slot(req, name, TELEBOT_MIME_TYPE_BOOL);
    if (p)
        p->data.b = value;
}

void telebot_request_str(telebot_request_t *req, const char *name, const char *value)
{
    if (value == NULL)
        return;
    telebot_core_mime_t *p = slot(req, name, TELEBOT_MIME_TYPE_STRING);
    if (p)
        p->data.s = value;
}

void telebot_request_id(telebot_request_t *req, const char *name, long long value)
{
    if (value > 0)
        telebot_request_ll(req, name, value);
}

void telebot_request_flag(telebot_request_t *req, const char *name, bool value)
{
    if (value)
        telebot_request_bool(req, name, true);
}

void telebot_request_str_required(telebot_request_t *req, const char *name, const char *value)
{
    if (value == NULL)
        fail(req, TELEBOT_ERROR_INVALID_PARAMETER, name, "is required");
    else
        telebot_request_str(req, name, value);
}

void telebot_request_file(telebot_request_t *req, const char *name, const telebot_input_file_t *file)
{
    if (file == NULL)
        return;

    switch (file->kind)
    {
    case TELEBOT_INPUT_FILE_REMOTE:
    case TELEBOT_INPUT_FILE_PATH:
        if ((file->ref == NULL) || (file->ref[0] == '\0'))
        {
            fail(req, TELEBOT_ERROR_INVALID_PARAMETER, name, "file reference is empty");
            return;
        }
        telebot_core_mime_t *p = slot(req, name, (file->kind == TELEBOT_INPUT_FILE_PATH) ? TELEBOT_MIME_TYPE_FILE
                                                                                        : TELEBOT_MIME_TYPE_STRING);
        if (p)
            p->data.s = file->ref;
        return;

    case TELEBOT_INPUT_FILE_MEMORY:
        if ((file->data == NULL) || (file->size == 0))
        {
            fail(req, TELEBOT_ERROR_INVALID_PARAMETER, name, "memory file is empty");
            return;
        }
        telebot_core_mime_t *m = slot(req, name, TELEBOT_MIME_TYPE_BUFFER);
        if (m)
        {
            m->data.buf.ptr = file->data;
            m->data.buf.len = file->size;
            m->data.buf.filename = file->filename;
        }
        return;

    default:
        fail(req, TELEBOT_ERROR_INVALID_PARAMETER, name, "unknown file kind");
        return;
    }
}

void telebot_request_file_required(telebot_request_t *req, const char *name, const telebot_input_file_t *file)
{
    if (file == NULL)
        fail(req, TELEBOT_ERROR_INVALID_PARAMETER, name, "is required");
    else
        telebot_request_file(req, name, file);
}

void telebot_request_upload(telebot_request_t *req, const char *name, const telebot_input_file_t *file)
{
    if ((file != NULL) && (file->kind == TELEBOT_INPUT_FILE_REMOTE))
    {
        fail(req, TELEBOT_ERROR_INVALID_PARAMETER, name, "must be uploaded (path or memory), not a file_id/URL");
        return;
    }
    telebot_request_file(req, name, file);
}

/* Appends formatted text to the scratch arena; returns its start or NULL when full. */
static char *scratch_vprintf(telebot_request_t *req, const char *name, const char *fmt, va_list ap)
{
    char *start = req->scratch + req->scratch_used;
    size_t room = sizeof(req->scratch) - req->scratch_used;
    int n = vsnprintf(start, room, fmt, ap);
    if ((n < 0) || ((size_t)n >= room))
    {
        fail(req, TELEBOT_ERROR_INVALID_PARAMETER, name, "value too long (TELEBOT_REQUEST_SCRATCH_SIZE)");
        return NULL;
    }
    req->scratch_used += (size_t)n + 1;
    return start;
}

static char *scratch_printf(telebot_request_t *req, const char *name, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char *out = scratch_vprintf(req, name, fmt, ap);
    va_end(ap);
    return out;
}

void telebot_request_printf(telebot_request_t *req, const char *name, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char *value = scratch_vprintf(req, name, fmt, ap);
    va_end(ap);
    if (value)
        telebot_request_str(req, name, value);
}

void telebot_request_send_options(telebot_request_t *req, const telebot_send_options_t *options)
{
    if (options == NULL)
        return;

    telebot_request_str(req, "business_connection_id", options->business_connection_id);
    telebot_request_id(req, "message_thread_id", options->message_thread_id);
    telebot_request_flag(req, "disable_notification", options->disable_notification);
    telebot_request_flag(req, "protect_content", options->protect_content);
    telebot_request_str(req, "message_effect_id", options->message_effect_id);
    if (options->reply_to_message_id > 0)
    {
        telebot_request_printf(req, "reply_parameters", "{\"message_id\":%d%s}", options->reply_to_message_id,
                      options->allow_sending_without_reply ? ",\"allow_sending_without_reply\":true" : "");
    }
    telebot_request_str(req, "reply_markup", options->reply_markup);
}

void telebot_request_permissions(telebot_request_t *req, const char *name, const telebot_chat_permissions_t *permissions)
{
    if (permissions == NULL)
        return;

    /* Only granted permissions are listed: an omitted permission is denied */
    char *json = scratch_printf(req, name, "{");
    if (json == NULL)
        return;
    bool first = true;
    for (size_t i = 0; i < TELEBOT_FIELD_COUNT(telebot_chat_permissions_fields); i++)
    {
        const telebot_bool_field_t *field = &telebot_chat_permissions_fields[i];
        if (!*telebot_bool_field_ptr((void *)permissions, field))
            continue;
        /* Overwrite the previous terminator so the object stays contiguous */
        req->scratch_used--;
        if (scratch_printf(req, name, "%s\"%s\":true", first ? "" : ",", field->key) == NULL)
            return;
        first = false;
    }
    req->scratch_used--;
    if (scratch_printf(req, name, "}") == NULL)
        return;
    telebot_request_str(req, name, json);
}

void telebot_request_rights_fields(telebot_request_t *req, const telebot_chat_administrator_rights_t *rights)
{
    if (rights == NULL)
        return;
    for (size_t i = 0; i < TELEBOT_FIELD_COUNT(telebot_chat_administrator_rights_fields); i++)
    {
        const telebot_bool_field_t *field = &telebot_chat_administrator_rights_fields[i];
        telebot_request_flag(req, field->key, *telebot_bool_field_ptr((void *)rights, field));
    }
}

void telebot_request_json(telebot_request_t *req, const char *name, telebot_json_compose_fn compose, const void *ctx)
{
    if (req->error != TELEBOT_ERROR_NONE)
        return;

    telebot_json_writer_t measure = {.buf = NULL};
    compose(&measure, ctx);
    size_t need = measure.len + 1;

    char *out;
    if (need <= sizeof(req->scratch) - req->scratch_used)
    {
        out = req->scratch + req->scratch_used;
        req->scratch_used += need;
    }
    else
    {
        if (req->owned_count >= sizeof(req->owned) / sizeof(req->owned[0]))
        {
            fail(req, TELEBOT_ERROR_INVALID_PARAMETER, name, "too many large JSON values");
            return;
        }
        out = malloc(need);
        if (out == NULL)
        {
            fail(req, TELEBOT_ERROR_OUT_OF_MEMORY, name, "out of memory");
            return;
        }
        req->owned[req->owned_count++] = out;
    }

    telebot_json_writer_t writer = {.buf = out, .cap = need};
    compose(&writer, ctx);
    out[writer.len] = '\0';
    telebot_request_str(req, name, out);
}

void telebot_request_release(telebot_request_t *req)
{
    for (size_t i = 0; i < req->owned_count; i++)
        free(req->owned[i]);
    req->owned_count = 0;
}
