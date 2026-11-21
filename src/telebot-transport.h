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

#ifndef __TELEBOT_TRANSPORT_H__
#define __TELEBOT_TRANSPORT_H__

/*
 * Transport and sink abstract data types (private).
 *
 * The core never talks to an HTTP library directly. It hands a request
 * (URL + multipart fields) to a telebot_transport_t, and the transport
 * streams the response body into a telebot_sink_t.
 *
 * Both are "interfaces" in the classic C sense: a struct whose first member
 * is a pointer to a const ops table. Concrete implementations embed the base
 * struct as their first member and recover themselves with TELEBOT_CONTAINER_OF.
 *
 *   Transports: curl (POSIX, src/transport/telebot-transport-curl.c)
 *               esp_http_client (ESP-IDF, src/transport/telebot-transport-esp.c)
 *   Sinks:      memory (size-capped, NUL-terminated), file (FILE *)
 */

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include "telebot-common.h"
#include "telebot-private.h"

#define TELEBOT_CONTAINER_OF(ptr, type, member) \
    ((type *)(void *)((char *)(ptr) - offsetof(type, member)))

/* ------------------------------------------------------------------------ */
/* Sink ADT                                                                  */
/* ------------------------------------------------------------------------ */

typedef struct telebot_sink telebot_sink_t;

typedef struct telebot_sink_ops
{
    /* Consume `len` bytes. Returns false to abort the transfer. */
    bool (*write)(telebot_sink_t *self, const void *data, size_t len);
} telebot_sink_ops_t;

struct telebot_sink
{
    const telebot_sink_ops_t *ops;
};

static inline bool telebot_sink_write(telebot_sink_t *sink, const void *data, size_t len)
{
    return sink->ops->write(sink, data, len);
}

/* Growable, NUL-terminated buffer; refuses to grow past `limit` (0 = no limit). */
typedef struct telebot_memory_sink
{
    telebot_sink_t base;
    char *data;
    size_t size;
    size_t limit;
    telebot_error_e error; /* why the sink rejected data, if it did */
} telebot_memory_sink_t;

void telebot_memory_sink_init(telebot_memory_sink_t *sink, size_t limit);
/* Transfers ownership of the buffer to the caller (may be NULL when empty). */
char *telebot_memory_sink_take(telebot_memory_sink_t *sink, size_t *size);
void telebot_memory_sink_release(telebot_memory_sink_t *sink);

typedef struct telebot_file_sink
{
    telebot_sink_t base;
    FILE *fp;
} telebot_file_sink_t;

void telebot_file_sink_init(telebot_file_sink_t *sink, FILE *fp);

/* ------------------------------------------------------------------------ */
/* Transport ADT                                                             */
/* ------------------------------------------------------------------------ */

typedef struct telebot_http_request
{
    const char *url;
    const telebot_core_mime_t *parts; /* may be NULL when count == 0 */
    size_t count;
} telebot_http_request_t;

typedef struct telebot_transport telebot_transport_t;

typedef struct telebot_transport_ops
{
    const char *name;

    /*
     * Perform one request. With count == 0 a GET is issued, otherwise a
     * multipart/form-data POST. The body is streamed into `sink` whatever the
     * status code, so callers can read Telegram's error description.
     * Returns TELEBOT_ERROR_NONE when an HTTP response was received;
     * *http_status receives the status code.
     */
    telebot_error_e (*perform)(telebot_transport_t *self, const telebot_http_request_t *req,
                               telebot_sink_t *sink, long *http_status);

    /* Optional: may be NULL when the backend cannot use a proxy. */
    telebot_error_e (*set_proxy)(telebot_transport_t *self, const char *addr, const char *auth);

    void (*destroy)(telebot_transport_t *self);
} telebot_transport_ops_t;

struct telebot_transport
{
    const telebot_transport_ops_t *ops;
};

/* Creates the backend selected at build time; NULL on allocation failure. */
telebot_transport_t *telebot_transport_create_default(void);

static inline telebot_error_e telebot_transport_perform(telebot_transport_t *t, const telebot_http_request_t *req,
                                                        telebot_sink_t *sink, long *http_status)
{
    return t->ops->perform(t, req, sink, http_status);
}

static inline telebot_error_e telebot_transport_set_proxy(telebot_transport_t *t, const char *addr,
                                                          const char *auth)
{
    return t->ops->set_proxy ? t->ops->set_proxy(t, addr, auth) : TELEBOT_ERROR_NOT_SUPPORTED;
}

static inline void telebot_transport_destroy(telebot_transport_t *t)
{
    if (t)
        t->ops->destroy(t);
}

/* ------------------------------------------------------------------------ */
/* Helpers shared by the backends                                            */
/* ------------------------------------------------------------------------ */

/* Size of the buffer telebot_mime_format_scalar() needs. */
#define TELEBOT_MIME_SCALAR_MAX 48

/*
 * Formats a numeric or boolean field into `buf`. Returns `buf`, or NULL when
 * the field is not a scalar (STRING / FILE / BUFFER).
 */
const char *telebot_mime_format_scalar(const telebot_core_mime_t *mime, char buf[TELEBOT_MIME_SCALAR_MAX]);

#endif /* __TELEBOT_TRANSPORT_H__ */
