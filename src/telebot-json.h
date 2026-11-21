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

#ifndef __TELEBOT_JSON_H__
#define __TELEBOT_JSON_H__

/*
 * Private JSON accessors on top of cJSON.
 *
 * cJSON is either the vendored copy in third_party/cjson or, on ESP-IDF,
 * the built-in "json" component. Both expose <cJSON.h>.
 *
 * All values returned by these helpers are borrowed from the root object
 * and stay valid until the root is released with tb_json_put().
 */

#include <stdbool.h>
#include <cJSON.h>

/* Returns true and sets *out if `key` exists in `obj` and is not JSON null. */
static inline bool tb_json_get_ex(const cJSON *obj, const char *key, cJSON **out)
{
    cJSON *item = cJSON_IsObject(obj) ? cJSON_GetObjectItemCaseSensitive(obj, key) : NULL;
    if (out)
        *out = item;
    return (item != NULL) && !cJSON_IsNull(item);
}

static inline const char *tb_json_get_string(const cJSON *item)
{
    return cJSON_IsString(item) ? item->valuestring : NULL;
}

/*
 * Integers are read from valuedouble so that 64-bit identifiers (Telegram
 * guarantees they fit in 52 bits) survive intact. Narrowing to `int` happens
 * at the assignment site for fields that are declared as int.
 */
static inline long long tb_json_get_int(const cJSON *item)
{
    if (cJSON_IsNumber(item))
        return (long long)item->valuedouble;
    if (cJSON_IsBool(item))
        return cJSON_IsTrue(item) ? 1 : 0;
    return 0;
}

static inline double tb_json_get_double(const cJSON *item)
{
    return cJSON_IsNumber(item) ? item->valuedouble : 0.0;
}

static inline bool tb_json_get_bool(const cJSON *item)
{
    return cJSON_IsTrue(item);
}

static inline int tb_json_array_length(const cJSON *array)
{
    return cJSON_IsArray(array) ? cJSON_GetArraySize(array) : 0;
}

static inline cJSON *tb_json_array_get_idx(const cJSON *array, int index)
{
    return cJSON_GetArrayItem(array, index);
}

/* Releases a root object returned by telebot_parser_str_to_obj() or cJSON_Create*(). */
static inline void tb_json_put(cJSON *root)
{
    if (root)
        cJSON_Delete(root);
}

#endif /* __TELEBOT_JSON_H__ */
