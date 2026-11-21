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
#include <string.h>
#include <sys/stat.h>
#include "telebot-multipart.h"
#include "telebot-transport.h"

static void emit(telebot_multipart_writer_t *w, const void *data, size_t len)
{
    if ((w->error != TELEBOT_ERROR_NONE) || (len == 0))
        return;

    if ((w->write != NULL) && !w->write(w->ctx, data, len))
    {
        w->error = TELEBOT_ERROR_NO_CONNECTION;
        return;
    }
    w->total += len;
}

static void emit_str(telebot_multipart_writer_t *w, const char *s)
{
    emit(w, s, strlen(s));
}

/* Header parameter value: characters that would break the quoted-string become '_'. */
static void emit_quoted(telebot_multipart_writer_t *w, const char *s)
{
    const char *run = s;
    for (const char *p = s; *p; p++)
    {
        if ((*p == '"') || (*p == '\r') || (*p == '\n') || (*p == '\\'))
        {
            emit(w, run, (size_t)(p - run));
            emit(w, "_", 1);
            run = p + 1;
        }
    }
    emit_str(w, run);
}

static const char *path_basename(const char *path)
{
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

static void emit_part_header(telebot_multipart_writer_t *w, const char *boundary, const char *name,
                             const char *filename)
{
    emit_str(w, "--");
    emit_str(w, boundary);
    emit_str(w, "\r\nContent-Disposition: form-data; name=\"");
    emit_quoted(w, name);
    emit_str(w, "\"");
    if (filename != NULL)
    {
        emit_str(w, "; filename=\"");
        emit_quoted(w, filename);
        emit_str(w, "\"\r\nContent-Type: application/octet-stream");
    }
    emit_str(w, "\r\n\r\n");
}

static void emit_file(telebot_multipart_writer_t *w, const char *path, char *chunk, size_t chunk_size)
{
    if (w->error != TELEBOT_ERROR_NONE)
        return;

    if (w->write == NULL)
    {
        struct stat st;
        if ((stat(path, &st) != 0) || !S_ISREG(st.st_mode))
        {
            ERR("Cannot upload '%s': not a readable file", path);
            w->error = TELEBOT_ERROR_INVALID_PARAMETER;
            return;
        }
        w->total += (size_t)st.st_size;
        return;
    }

    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
    {
        ERR("Cannot open '%s'", path);
        w->error = TELEBOT_ERROR_INVALID_PARAMETER;
        return;
    }

    size_t n;
    while ((w->error == TELEBOT_ERROR_NONE) && ((n = fread(chunk, 1, chunk_size, fp)) > 0))
        emit(w, chunk, n);

    if (ferror(fp) && (w->error == TELEBOT_ERROR_NONE))
    {
        ERR("Read error on '%s'", path);
        w->error = TELEBOT_ERROR_OPERATION_FAILED;
    }
    fclose(fp);
}

telebot_error_e telebot_multipart_encode(telebot_multipart_writer_t *w, const char *boundary,
                                         const telebot_core_mime_t *parts, size_t count, char *chunk,
                                         size_t chunk_size)
{
    char scalar[TELEBOT_MIME_SCALAR_MAX];

    for (size_t i = 0; (i < count) && (w->error == TELEBOT_ERROR_NONE); i++)
    {
        const telebot_core_mime_t *part = &parts[i];
        switch (part->type)
        {
        case TELEBOT_MIME_TYPE_STRING:
            emit_part_header(w, boundary, part->name, NULL);
            emit_str(w, part->data.s ? part->data.s : "");
            break;
        case TELEBOT_MIME_TYPE_FILE:
            emit_part_header(w, boundary, part->name, path_basename(part->data.s));
            emit_file(w, part->data.s, chunk, chunk_size);
            break;
        case TELEBOT_MIME_TYPE_BUFFER:
            emit_part_header(w, boundary, part->name, part->data.buf.filename ? part->data.buf.filename : "file");
            emit(w, part->data.buf.ptr, part->data.buf.len);
            break;
        default:
            if (telebot_mime_format_scalar(part, scalar) == NULL)
            {
                ERR("Invalid field type %d for '%s'", part->type, part->name);
                w->error = TELEBOT_ERROR_INVALID_PARAMETER;
                break;
            }
            emit_part_header(w, boundary, part->name, NULL);
            emit_str(w, scalar);
            break;
        }
        emit_str(w, "\r\n");
    }

    emit_str(w, "--");
    emit_str(w, boundary);
    emit_str(w, "--\r\n");
    return w->error;
}

void telebot_multipart_boundary(char out[TELEBOT_MULTIPART_BOUNDARY_SIZE], const uint8_t random[8])
{
    static const char hex[] = "0123456789abcdef";
    const size_t prefix = sizeof(TELEBOT_MULTIPART_BOUNDARY_PREFIX) - 1;

    memcpy(out, TELEBOT_MULTIPART_BOUNDARY_PREFIX, prefix);
    char *p = out + prefix;
    for (size_t i = 0; i < 8; i++)
    {
        *p++ = hex[random[i] >> 4];
        *p++ = hex[random[i] & 0x0F];
    }
    *p = '\0';
}

bool telebot_multipart_content_type(char *out, size_t out_size, const char *boundary)
{
    int n = snprintf(out, out_size, "multipart/form-data; boundary=%s", boundary);
    return (n > 0) && ((size_t)n < out_size);
}
