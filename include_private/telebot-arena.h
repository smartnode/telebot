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

#ifndef __TELEBOT_ARENA_H__
#define __TELEBOT_ARENA_H__

/*
 * Arena allocator ADT (private).
 *
 * A batch of updates is parsed into one arena: allocations are bump-pointer
 * carves from a short list of blocks, and the whole batch is released with a
 * single telebot_arena_destroy(). This bounds fragmentation on small heaps and
 * means a missing release function can no longer leak part of an update.
 *
 * The parser does not take an arena parameter. Instead an arena is made
 * "current" for the calling thread, and telebot_mem_*() route to it:
 *
 *     telebot_arena_t *arena = telebot_arena_create();
 *     telebot_arena_t *prev = telebot_arena_enter(arena);
 *     ...parse...                       // telebot_mem_calloc() -> arena
 *     telebot_arena_leave(prev);
 *     ...
 *     telebot_arena_destroy(arena);     // frees everything at once
 *
 * While an arena is current, telebot_mem_free() does nothing: anything freed
 * then was carved from that arena.
 */

#include <stddef.h>

typedef struct telebot_arena telebot_arena_t;

/* Returns NULL when out of memory */
telebot_arena_t *telebot_arena_create(void);
void telebot_arena_destroy(telebot_arena_t *arena);

/* Bytes handed out so far (for diagnostics and tests) */
size_t telebot_arena_used(const telebot_arena_t *arena);

/* Makes `arena` current for this thread; returns the previous one to restore */
telebot_arena_t *telebot_arena_enter(telebot_arena_t *arena);
void telebot_arena_leave(telebot_arena_t *previous);

/* Library allocation entry points: the current arena if any, else the C heap */
void *telebot_mem_calloc(size_t count, size_t size);
void *telebot_mem_malloc(size_t size);
char *telebot_mem_strdup(const char *str);
void telebot_mem_free(void *ptr);

#endif /* __TELEBOT_ARENA_H__ */
