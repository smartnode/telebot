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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "telebot-json.h"

/* Appends n bytes; while measuring (buf == NULL) only the length grows */
static void put(telebot_json_writer_t *w, const char *data, size_t n)
{
    if ((w->buf != NULL) && (w->len + n < w->cap))
        memcpy(w->buf + w->len, data, n);
    w->len += n;
}

void telebot_json_write_raw(telebot_json_writer_t *writer, const char *text)
{
    put(writer, text, strlen(text));
}

void telebot_json_write_string(telebot_json_writer_t *writer, const char *value)
{
    static const char hex[] = "0123456789abcdef";
    const char *run = value ? value : "";

    put(writer, "\"", 1);
    for (const char *p = run; *p != '\0'; p++)
    {
        unsigned char c = (unsigned char)*p;
        if ((c >= 0x20) && (c != '"') && (c != '\\'))
            continue;

        put(writer, run, (size_t)(p - run));
        run = p + 1;
        switch (c)
        {
        case '"':
            put(writer, "\\\"", 2);
            break;
        case '\\':
            put(writer, "\\\\", 2);
            break;
        case '\n':
            put(writer, "\\n", 2);
            break;
        case '\r':
            put(writer, "\\r", 2);
            break;
        case '\t':
            put(writer, "\\t", 2);
            break;
        default:
        {
            const char esc[6] = {'\\', 'u', '0', '0', hex[c >> 4], hex[c & 0x0F]};
            put(writer, esc, sizeof(esc));
            break;
        }
        }
    }
    put(writer, run, strlen(run));
    put(writer, "\"", 1);
}

void telebot_json_write_int(telebot_json_writer_t *writer, long long value)
{
    char text[24];
    int n = snprintf(text, sizeof(text), "%lld", value);
    put(writer, text, (size_t)n);
}

char *telebot_json_compose(telebot_json_compose_fn compose, const void *ctx)
{
    telebot_json_writer_t measure = {.buf = NULL};
    compose(&measure, ctx);

    char *out = malloc(measure.len + 1);
    if (out == NULL)
        return NULL;

    telebot_json_writer_t writer = {.buf = out, .cap = measure.len + 1};
    compose(&writer, ctx);
    out[writer.len] = '\0';
    return out;
}
