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

#ifndef __TELEBOT_REQUEST_H__
#define __TELEBOT_REQUEST_H__

/*
 * Request builder ADT (private).
 *
 * Replaces hand-indexed `mimes[N]` arrays and `count++` bookkeeping. Adders
 * are bounds-checked, skip absent values (NULL strings, non-positive ids),
 * and record the first error; telebot_core_request() refuses to send a
 * request that has one. The builder lives on the caller's stack and never
 * allocates; every pointer it stores is borrowed until the request is sent.
 *
 *     telebot_request_t req;
 *     telebot_request_init(&req);
 *     tb_req_add(&req, "chat_id", chat_id);        // _Generic dispatch
 *     tb_req_add(&req, "caption", caption);        // skipped when NULL
 *     tb_req_file(&req, "photo", &photo);          // telebot_input_file_t
 *     return telebot_core_request(core_h, "sendPhoto", &req);
 */

#include <stdbool.h>
#include <stddef.h>
#include "telebot-common.h"
#include "telebot-core.h"
#include "telebot-private.h"
#include "telebot-types.h"

#ifndef TELEBOT_REQUEST_MAX_PARTS
#define TELEBOT_REQUEST_MAX_PARTS 32
#endif

/* Room for small generated JSON values (reply_parameters, permissions, ...) */
#ifndef TELEBOT_REQUEST_SCRATCH_SIZE
#define TELEBOT_REQUEST_SCRATCH_SIZE 640
#endif

typedef struct telebot_request
{
    telebot_core_mime_t parts[TELEBOT_REQUEST_MAX_PARTS];
    size_t count;
    telebot_error_e error; /* first error recorded by an adder */
    char scratch[TELEBOT_REQUEST_SCRATCH_SIZE];
    size_t scratch_used;
} telebot_request_t;

void telebot_request_init(telebot_request_t *req);

/* Always added */
void tb_req_int(telebot_request_t *req, const char *name, int value);
void tb_req_ll(telebot_request_t *req, const char *name, long long value);
void tb_req_double(telebot_request_t *req, const char *name, double value);
void tb_req_bool(telebot_request_t *req, const char *name, bool value);

/* Skipped when absent */
void tb_req_str(telebot_request_t *req, const char *name, const char *value);   /* NULL */
void tb_req_id(telebot_request_t *req, const char *name, long long value);      /* <= 0 */
void tb_req_flag(telebot_request_t *req, const char *name, bool value);         /* false */

/* Required value: records TELEBOT_ERROR_INVALID_PARAMETER when NULL */
void tb_req_str_required(telebot_request_t *req, const char *name, const char *value);

/* InputFile or String; validates the descriptor. NULL is "absent" unless required. */
void tb_req_file(telebot_request_t *req, const char *name, const telebot_input_file_t *file);
void tb_req_file_required(telebot_request_t *req, const char *name, const telebot_input_file_t *file);
/* Same as tb_req_file(), but the file must be uploaded (PATH or MEMORY): file_ids and
 * URLs are rejected. Used for thumbnails and chat photos, which Telegram never reuses. */
void tb_req_upload(telebot_request_t *req, const char *name, const telebot_input_file_t *file);

/* Adds a field whose value is formatted into the request's scratch arena. */
void tb_req_printf(telebot_request_t *req, const char *name, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/* Expands the shared send options; NULL adds nothing. */
void tb_req_send_options(telebot_request_t *req, const telebot_send_options_t *options);

/* ChatPermissions as a JSON object; NULL adds nothing. */
void tb_req_permissions(telebot_request_t *req, const char *name, const telebot_chat_permissions_t *permissions);

/* ChatAdministratorRights as individual boolean fields (promoteChatMember style). */
void tb_req_rights_fields(telebot_request_t *req, const telebot_chat_administrator_rights_t *rights);

/* Sends the request, or returns an error response if the builder recorded one. */
telebot_core_response_t telebot_core_request(telebot_core_handler_t core_h, const char *method,
                                             const telebot_request_t *req);

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && !defined(__cplusplus)
/* Picks the adder from the value's type. Optional strings are skipped when NULL. */
#define tb_req_add(req, name, value)        \
    _Generic((value),                       \
        bool: tb_req_bool,                  \
        int: tb_req_int,                    \
        long long: tb_req_ll,               \
        double: tb_req_double,              \
        char *: tb_req_str,                 \
        const char *: tb_req_str,           \
        telebot_input_file_t *: tb_req_file, \
        const telebot_input_file_t *: tb_req_file)(req, name, value)
#endif

#endif /* __TELEBOT_REQUEST_H__ */
