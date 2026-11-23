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

#ifndef __TELEBOT_CONFIG_H__
#define __TELEBOT_CONFIG_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @file        telebot-config.h
 * @ingroup     TELEBOT_CONFIG_API
 * @brief       This file contains library-wide runtime settings
 * @author      Elmurod Talipov
 * @date        2026-11-24
 */

/**
 * @defgroup TELEBOT_CONFIG_API Library Configuration
 * @brief Global settings that apply to every handler, for both the simple and core APIs.
 *
 * Build-time limits (response size, arena block size, ...) are set with `-D` or, on ESP-IDF,
 * with `idf.py menuconfig`; this group covers what can be changed at runtime.
 *
 * @addtogroup TELEBOT_CONFIG_API
 * @{
 */

/**
 * @brief Routes the memory used for JSON parsing through custom allocators.
 *
 * Parsing a reply builds a temporary JSON tree whose size depends on the
 * reply. On boards with external RAM this keeps it out of internal memory:
 * @code
 * static void *psram_malloc(size_t size) { return heap_caps_malloc(size, MALLOC_CAP_SPIRAM); }
 * telebot_set_json_allocator(psram_malloc, heap_caps_free);
 * @endcode
 * The setting is global. Call it before creating any handler; pass NULL for
 * both to restore malloc() and free().
 *
 * @param[in] malloc_fn Allocation function, or NULL for malloc().
 * @param[in] free_fn Matching release function, or NULL for free().
 */
void telebot_set_json_allocator(void *(*malloc_fn)(size_t size), void (*free_fn)(void *ptr));

/**
 * @} // end of TELEBOT_CONFIG_API
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_CONFIG_H__ */
