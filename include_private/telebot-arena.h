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
 * Memory ownership for parsed results (private).
 *
 * Every parsed result lives in one arena: allocations are bump-pointer carves
 * from a few blocks, and the whole result is released at once. Released blocks
 * go to a small cache, so a steady polling loop needs no malloc() or free().
 *
 * Who owns the arena is never stored in a public struct:
 *
 *  - Results the library allocates (the update array, admin lists, ...) carry a
 *    hidden header just before the returned pointer. telebot_owned_release()
 *    finds the arena from the pointer and checks a magic value.
 *
 *  - Results written into a caller's struct (telebot_get_me(bot, &me)) are
 *    recorded in a registry keyed by the struct's address.
 *    telebot_owner_release() looks the address up; a copy of the struct is not
 *    found and is rejected instead of being freed twice.
 *
 * The parser does not take an arena parameter. An arena is made current for
 * the calling thread, and telebot_mem_*() allocate from it:
 *
 *     telebot_arena_t *arena = telebot_arena_begin();       // create, make current
 *     ret = arena ? parse(...) : TELEBOT_ERROR_OUT_OF_MEMORY;
 *     if (ret == TELEBOT_ERROR_NONE)
 *         ret = telebot_owner_attach(result, arena);        // or an owned array
 *     telebot_arena_finish(arena, ret == TELEBOT_ERROR_NONE);
 *
 * While an arena is current, telebot_mem_free() does nothing: anything freed
 * then was carved from that arena.
 */

#include <stdbool.h>
#include <stddef.h>
#include "telebot-common.h"

typedef struct telebot_arena telebot_arena_t;

/* ---- Arena ---------------------------------------------------------------- */

/* Returns NULL when out of memory */
telebot_arena_t *telebot_arena_create(void);
/* Releases all memory of the arena; its blocks go back to the cache */
void telebot_arena_destroy(telebot_arena_t *arena);

/* Bytes handed out so far (for diagnostics and tests) */
size_t telebot_arena_used(const telebot_arena_t *arena);

/* Makes `arena` current for this thread; returns the previous one to restore */
telebot_arena_t *telebot_arena_enter(telebot_arena_t *arena);
void telebot_arena_leave(telebot_arena_t *previous);

/* Creates an arena and makes it current; NULL when out of memory */
telebot_arena_t *telebot_arena_begin(void);
/* Restores the previous arena; destroys `arena` unless `keep`. Accepts NULL. */
void telebot_arena_finish(telebot_arena_t *arena, bool keep);

/* Drops every cached block (for tests and low-memory situations) */
void telebot_arena_trim_cache(void);

/* ---- Library allocation entry points --------------------------------------- */

/* The current arena if any, otherwise the C heap */
void *telebot_mem_calloc(size_t count, size_t size);
void *telebot_mem_malloc(size_t size);
char *telebot_mem_strdup(const char *str);
void telebot_mem_free(void *ptr);

/* ---- Library-allocated results: hidden header ----------------------------- */

/*
 * Zeroed array of `count` elements preceded by a hidden ownership header that
 * records the current arena (or none, in which case it comes from the heap).
 */
void *telebot_owned_calloc(size_t count, size_t size);

/* Releases an array from telebot_owned_calloc() together with its arena */
telebot_error_e telebot_owned_release(void *array);

/* ---- Caller-provided results: ownership registry --------------------------- */

/* Records that the caller's struct at `object` owns `arena` */
telebot_error_e telebot_owner_attach(const void *object, telebot_arena_t *arena);

/*
 * Releases the arena owned by the struct at `object` and zeroes the struct.
 * Returns TELEBOT_ERROR_INVALID_PARAMETER if the address owns nothing, e.g. a
 * copy of a result or a struct that was never filled.
 */
telebot_error_e telebot_owner_release(void *object, size_t size);

#endif /* __TELEBOT_ARENA_H__ */
