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

#ifndef __TELEBOT_MULTIPART_H__
#define __TELEBOT_MULTIPART_H__

/*
 * Portable multipart/form-data encoder (RFC 7578), used by transports whose
 * HTTP client has no multipart support (esp_http_client).
 *
 * The encoder streams: it never builds the body in memory. Run it once with
 * a counting writer to learn the exact Content-Length, then again with a
 * real writer to send the very same bytes.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "telebot-common.h"
#include "telebot-private.h"

#define TELEBOT_MULTIPART_BOUNDARY_PREFIX "----telebot"
/* Prefix + 16 hex digits + NUL */
#define TELEBOT_MULTIPART_BOUNDARY_SIZE (sizeof(TELEBOT_MULTIPART_BOUNDARY_PREFIX) + 16)

typedef struct telebot_multipart_writer
{
    /* Sends bytes; return false to abort. NULL turns the writer into a byte counter. */
    bool (*write)(void *ctx, const void *data, size_t len);
    void *ctx;
    size_t total;            /* bytes emitted so far */
    telebot_error_e error;   /* first error encountered, TELEBOT_ERROR_NONE otherwise */
} telebot_multipart_writer_t;

/* Builds a boundary from 8 caller-supplied random bytes. */
void telebot_multipart_boundary(char out[TELEBOT_MULTIPART_BOUNDARY_SIZE], const uint8_t random[8]);

/*
 * Encodes all parts. `chunk` is scratch space for file reads (chunk_size
 * bytes); it is only touched when writer->write is not NULL.
 * Returns writer->error.
 */
telebot_error_e telebot_multipart_encode(telebot_multipart_writer_t *writer, const char *boundary,
                                         const telebot_core_mime_t *parts, size_t count, char *chunk,
                                         size_t chunk_size);

/* "multipart/form-data; boundary=<boundary>" into out; false if it does not fit. */
bool telebot_multipart_content_type(char *out, size_t out_size, const char *boundary);

#endif /* __TELEBOT_MULTIPART_H__ */
