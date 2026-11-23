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

#include <cJSON.h>
#include <telebot-config.h>

void telebot_set_json_allocator(void *(*malloc_fn)(size_t size), void (*free_fn)(void *ptr))
{
    /* NULL functions restore the C library defaults */
    cJSON_Hooks hooks = {.malloc_fn = malloc_fn, .free_fn = free_fn};
    cJSON_InitHooks(&hooks);
}
