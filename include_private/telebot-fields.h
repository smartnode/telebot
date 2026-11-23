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

#ifndef __TELEBOT_FIELDS_H__
#define __TELEBOT_FIELDS_H__

/*
 * Field tables (X-macros) for all-boolean Bot API objects. One list drives the
 * parser and the serializer, and the static asserts below fail the build if a
 * field is added to a public struct but not to its table.
 */

#include <stdbool.h>
#include <stddef.h>
#include "telebot-limits.h"
#include "telebot-common.h"
#include "telebot-types.h"

#define TELEBOT_CHAT_PERMISSIONS_FIELDS(X) \
    X(can_send_messages)                   \
    X(can_send_audios)                     \
    X(can_send_documents)                  \
    X(can_send_photos)                     \
    X(can_send_videos)                     \
    X(can_send_video_notes)                \
    X(can_send_voice_notes)                \
    X(can_send_polls)                      \
    X(can_send_other_messages)             \
    X(can_add_web_page_previews)           \
    X(can_react_to_messages)               \
    X(can_edit_tag)                        \
    X(can_change_info)                     \
    X(can_invite_users)                    \
    X(can_pin_messages)                    \
    X(can_manage_topics)

#define TELEBOT_CHAT_ADMINISTRATOR_RIGHTS_FIELDS(X) \
    X(is_anonymous)                                 \
    X(can_manage_chat)                              \
    X(can_delete_messages)                          \
    X(can_manage_video_chats)                       \
    X(can_restrict_members)                         \
    X(can_promote_members)                          \
    X(can_change_info)                              \
    X(can_invite_users)                             \
    X(can_post_stories)                             \
    X(can_edit_stories)                             \
    X(can_delete_stories)                           \
    X(can_post_messages)                            \
    X(can_edit_messages)                            \
    X(can_pin_messages)                             \
    X(can_manage_topics)                            \
    X(can_manage_direct_messages)                   \
    X(can_manage_tags)                              \
    X(can_send_welcome_messages)

/* JSON key + byte offset of a bool member */
typedef struct telebot_bool_field
{
    const char *key;
    size_t offset;
} telebot_bool_field_t;

#define TELEBOT_PERMISSION_ENTRY(name) {#name, offsetof(telebot_chat_permissions_t, name)},
#define TELEBOT_RIGHTS_ENTRY(name) {#name, offsetof(telebot_chat_administrator_rights_t, name)},
#define TELEBOT_COUNT_FIELD(name) +1

/* Each translation unit uses only the tables it needs */
#define TELEBOT_MAYBE_UNUSED __attribute__((unused))

static const telebot_bool_field_t telebot_chat_permissions_fields[] TELEBOT_MAYBE_UNUSED = {
    TELEBOT_CHAT_PERMISSIONS_FIELDS(TELEBOT_PERMISSION_ENTRY)};

static const telebot_bool_field_t telebot_chat_administrator_rights_fields[] TELEBOT_MAYBE_UNUSED = {
    TELEBOT_CHAT_ADMINISTRATOR_RIGHTS_FIELDS(TELEBOT_RIGHTS_ENTRY)};

TELEBOT_STATIC_ASSERT(sizeof(telebot_chat_permissions_t) ==
                          (0 TELEBOT_CHAT_PERMISSIONS_FIELDS(TELEBOT_COUNT_FIELD)) * sizeof(bool),
                      "telebot_chat_permissions_t and TELEBOT_CHAT_PERMISSIONS_FIELDS are out of sync");
TELEBOT_STATIC_ASSERT(sizeof(telebot_chat_administrator_rights_t) ==
                          (0 TELEBOT_CHAT_ADMINISTRATOR_RIGHTS_FIELDS(TELEBOT_COUNT_FIELD)) * sizeof(bool),
                      "telebot_chat_administrator_rights_t and its field table are out of sync");

#define TELEBOT_FIELD_COUNT(table) (sizeof(table) / sizeof((table)[0]))

static inline bool *telebot_bool_field_ptr(void *object, const telebot_bool_field_t *field)
{
    return (bool *)((char *)object + field->offset);
}

/*
 * Scalar JSON fields parsed by table: one loop replaces a block of code per
 * field. Offsets are 16-bit; every parsed struct is far smaller than 64 KiB.
 */
typedef enum telebot_field_kind
{
    TELEBOT_FIELD_STRING,    /* char *, duplicated */
    TELEBOT_FIELD_BOOL,      /* bool */
    TELEBOT_FIELD_INT,       /* int */
    TELEBOT_FIELD_LONG,      /* long */
    TELEBOT_FIELD_LONG_LONG, /* long long */
    TELEBOT_FIELD_OBJECT,    /* pointer to a nested struct, parsed by telebot_object_types[aux] */
} telebot_field_kind_e;

typedef struct telebot_json_field
{
    const char *key;
    unsigned short offset;
    unsigned char kind;
    unsigned char aux; /* TELEBOT_FIELD_OBJECT: index into telebot_object_types */
} telebot_json_field_t;

#define TELEBOT_JSON_FIELD(key, type, field, kind) {key, (unsigned short)offsetof(type, field), kind, 0}
#define TELEBOT_JSON_OBJECT(key, type, field, object_type) \
    {key, (unsigned short)offsetof(type, field), TELEBOT_FIELD_OBJECT, object_type}

struct cJSON;

/* How to allocate and parse one kind of nested object */
typedef struct telebot_object_type
{
    unsigned short size;
    telebot_error_e (*parse)(struct cJSON *obj, void *target);
} telebot_object_type_t;

/* Defined by the parser, indexed by telebot_json_field_t.aux */
extern const telebot_object_type_t telebot_object_types[];

TELEBOT_STATIC_ASSERT(sizeof(telebot_message_t) < 65536, "field offsets must fit in 16 bits");

/* Copies every listed field present in `obj` into the struct at `target` */
void telebot_parse_fields(const struct cJSON *obj, void *target, const telebot_json_field_t *fields, size_t count);

#endif /* __TELEBOT_FIELDS_H__ */
