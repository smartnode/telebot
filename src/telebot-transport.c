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

/* Backend-independent parts of the transport layer: sinks and formatting. */

#include <stdlib.h>
#include <string.h>
#include "telebot-transport.h"

/* ---------------------------- memory sink -------------------------------- */

static bool memory_sink_write(telebot_sink_t *self, const void *data, size_t len)
{
    telebot_memory_sink_t *sink = TELEBOT_CONTAINER_OF(self, telebot_memory_sink_t, base);

    if (len == 0)
        return true;

    if ((sink->limit > 0) && (len > sink->limit - sink->size))
    {
        ERR("Response exceeds TELEBOT_MAX_RESPONSE_SIZE (%zu bytes)", sink->limit);
        sink->error = TELEBOT_ERROR_OUT_OF_MEMORY;
        return false;
    }

    char *grown = realloc(sink->data, sink->size + len + 1);
    if (grown == NULL)
    {
        ERR("Failed to allocate %zu bytes for response", sink->size + len + 1);
        sink->error = TELEBOT_ERROR_OUT_OF_MEMORY;
        return false;
    }

    memcpy(grown + sink->size, data, len);
    sink->data = grown;
    sink->size += len;
    sink->data[sink->size] = '\0';
    return true;
}

static const telebot_sink_ops_t memory_sink_ops = {.write = memory_sink_write};

void telebot_memory_sink_init(telebot_memory_sink_t *sink, size_t limit)
{
    *sink = (telebot_memory_sink_t){.base = {.ops = &memory_sink_ops}, .limit = limit, .error = TELEBOT_ERROR_NONE};
}

char *telebot_memory_sink_take(telebot_memory_sink_t *sink, size_t *size)
{
    char *data = sink->data;
    if (size)
        *size = sink->size;
    sink->data = NULL;
    sink->size = 0;
    return data;
}

void telebot_memory_sink_release(telebot_memory_sink_t *sink)
{
    TELEBOT_SAFE_FZCNT(sink->data, sink->size);
}

/* ----------------------------- file sink --------------------------------- */

static bool file_sink_write(telebot_sink_t *self, const void *data, size_t len)
{
    telebot_file_sink_t *sink = TELEBOT_CONTAINER_OF(self, telebot_file_sink_t, base);
    return fwrite(data, 1, len, sink->fp) == len;
}

static const telebot_sink_ops_t file_sink_ops = {.write = file_sink_write};

void telebot_file_sink_init(telebot_file_sink_t *sink, FILE *fp)
{
    *sink = (telebot_file_sink_t){.base = {.ops = &file_sink_ops}, .fp = fp};
}

/* --------------------------- field formatting ---------------------------- */

const char *telebot_mime_format_scalar(const telebot_core_mime_t *mime, char buf[TELEBOT_MIME_SCALAR_MAX])
{
    switch (mime->type)
    {
    case TELEBOT_MIME_TYPE_INT:
        snprintf(buf, TELEBOT_MIME_SCALAR_MAX, "%d", mime->data.d);
        return buf;
    case TELEBOT_MIME_TYPE_LONG_INT:
        snprintf(buf, TELEBOT_MIME_SCALAR_MAX, "%ld", mime->data.ld);
        return buf;
    case TELEBOT_MIME_TYPE_LONG_LONG_INT:
        snprintf(buf, TELEBOT_MIME_SCALAR_MAX, "%lld", mime->data.lld);
        return buf;
    case TELEBOT_MIME_TYPE_FLOAT:
        snprintf(buf, TELEBOT_MIME_SCALAR_MAX, "%.7g", (double)mime->data.f);
        return buf;
    case TELEBOT_MIME_TYPE_DOUBLE:
        snprintf(buf, TELEBOT_MIME_SCALAR_MAX, "%.15g", mime->data.lf);
        return buf;
    case TELEBOT_MIME_TYPE_BOOL:
        snprintf(buf, TELEBOT_MIME_SCALAR_MAX, "%s", mime->data.b ? "true" : "false");
        return buf;
    case TELEBOT_MIME_TYPE_STRING:
    case TELEBOT_MIME_TYPE_FILE:
    case TELEBOT_MIME_TYPE_BUFFER:
    case TELEBOT_MIME_TYPE_MAX:
    default:
        return NULL;
    }
}
