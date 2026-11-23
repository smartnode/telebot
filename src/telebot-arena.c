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

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "telebot-arena.h"
#include "telebot-limits.h"

#define ARENA_ALIGN (sizeof(max_align_t))
#define ALIGN_UP(n) (((n) + ARENA_ALIGN - 1) & ~(ARENA_ALIGN - 1))

/* ---------------------------------------------------------------------------- */
/* Blocks and the block cache                                                    */
/* ---------------------------------------------------------------------------- */

typedef struct arena_block
{
    struct arena_block *next;
    size_t size; /* usable bytes after the header */
    size_t used;
    max_align_t data[]; /* aligned start of the usable area */
} arena_block_t;

static pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;
static arena_block_t *cache_head = NULL;
static size_t cache_count = 0;

/* A block with at least `min_size` usable bytes; standard blocks come from the cache */
static arena_block_t *block_get(size_t min_size)
{
    arena_block_t *block = NULL;
    if (min_size <= TELEBOT_ARENA_BLOCK_SIZE)
    {
        pthread_mutex_lock(&cache_lock);
        block = cache_head;
        if (block != NULL)
        {
            cache_head = block->next;
            cache_count--;
        }
        pthread_mutex_unlock(&cache_lock);
        min_size = TELEBOT_ARENA_BLOCK_SIZE;
    }
    if (block == NULL)
    {
        block = malloc(sizeof(arena_block_t) + min_size);
        if (block == NULL)
            return NULL;
        block->size = min_size;
    }
    block->next = NULL;
    block->used = 0;
    return block;
}

/* Standard blocks go back to the cache while it has room; others are freed */
static void block_put(arena_block_t *block)
{
    if (block->size == TELEBOT_ARENA_BLOCK_SIZE)
    {
        pthread_mutex_lock(&cache_lock);
        if (cache_count < TELEBOT_ARENA_CACHE_BLOCKS)
        {
            block->next = cache_head;
            cache_head = block;
            cache_count++;
            block = NULL;
        }
        pthread_mutex_unlock(&cache_lock);
    }
    free(block);
}

void telebot_arena_trim_cache(void)
{
    pthread_mutex_lock(&cache_lock);
    arena_block_t *block = cache_head;
    cache_head = NULL;
    cache_count = 0;
    pthread_mutex_unlock(&cache_lock);

    while (block != NULL)
    {
        arena_block_t *next = block->next;
        free(block);
        block = next;
    }
}

/* ---------------------------------------------------------------------------- */
/* Arena                                                                         */
/* ---------------------------------------------------------------------------- */

struct telebot_arena
{
    arena_block_t *head;       /* block currently being carved; the arena itself lives in the last one */
    size_t total_used;
    telebot_arena_t *previous; /* arena that was current before telebot_arena_begin() */
};

static _Thread_local telebot_arena_t *current_arena = NULL;

telebot_arena_t *telebot_arena_create(void)
{
    arena_block_t *block = block_get(0);
    if (block == NULL)
        return NULL;

    /* The arena's own bookkeeping is the first carve of its first block */
    telebot_arena_t *arena = (telebot_arena_t *)block->data;
    block->used = ALIGN_UP(sizeof(telebot_arena_t));
    arena->head = block;
    arena->total_used = 0;
    arena->previous = NULL;
    return arena;
}

void telebot_arena_destroy(telebot_arena_t *arena)
{
    if (arena == NULL)
        return;
    /* `arena` lives in the last block of the list: read nothing after releasing it */
    arena_block_t *block = arena->head;
    while (block != NULL)
    {
        arena_block_t *next = block->next;
        block_put(block);
        block = next;
    }
}

size_t telebot_arena_used(const telebot_arena_t *arena)
{
    return arena ? arena->total_used : 0;
}

static void *arena_alloc(telebot_arena_t *arena, size_t size)
{
    if (size == 0)
        size = 1;
    size_t aligned = ALIGN_UP(size);
    if (aligned < size) /* overflow */
        return NULL;

    arena_block_t *block = arena->head;
    if (block->size - block->used < aligned)
    {
        /* New block in front; the rest of the old one stays unused */
        arena_block_t *fresh = block_get(aligned);
        if (fresh == NULL)
            return NULL;
        fresh->next = block;
        arena->head = block = fresh;
    }

    void *ptr = (char *)block->data + block->used;
    block->used += aligned;
    arena->total_used += aligned;
    memset(ptr, 0, size);
    return ptr;
}

telebot_arena_t *telebot_arena_enter(telebot_arena_t *arena)
{
    telebot_arena_t *previous = current_arena;
    current_arena = arena;
    return previous;
}

void telebot_arena_leave(telebot_arena_t *previous)
{
    current_arena = previous;
}

telebot_arena_t *telebot_arena_begin(void)
{
    telebot_arena_t *arena = telebot_arena_create();
    if (arena != NULL)
        arena->previous = telebot_arena_enter(arena);
    return arena;
}

void telebot_arena_finish(telebot_arena_t *arena, bool keep)
{
    if (arena == NULL)
        return;
    telebot_arena_leave(arena->previous);
    arena->previous = NULL;
    if (!keep)
        telebot_arena_destroy(arena);
}

/* ---------------------------------------------------------------------------- */
/* Allocation entry points                                                       */
/* ---------------------------------------------------------------------------- */

void *telebot_mem_calloc(size_t count, size_t size)
{
    if (current_arena == NULL)
        return calloc(count, size);
    if ((size != 0) && (count > SIZE_MAX / size))
        return NULL;
    return arena_alloc(current_arena, count * size);
}

void *telebot_mem_malloc(size_t size)
{
    return current_arena ? arena_alloc(current_arena, size) : malloc(size);
}

char *telebot_mem_strdup(const char *str)
{
    if (str == NULL)
        return NULL;
    if (current_arena == NULL)
        return strdup(str);
    size_t len = strlen(str) + 1;
    char *copy = arena_alloc(current_arena, len);
    if (copy != NULL)
        memcpy(copy, str, len);
    return copy;
}

void telebot_mem_free(void *ptr)
{
    if (current_arena == NULL)
        free(ptr);
    /* otherwise ptr belongs to the current arena and is released with it */
}

/* ---------------------------------------------------------------------------- */
/* Library-allocated results: hidden header                                      */
/* ---------------------------------------------------------------------------- */

#define OWNED_MAGIC 0x54424f57u /* "TBOW" */

typedef union owned_header
{
    struct
    {
        uint32_t magic;
        telebot_arena_t *arena; /* NULL when the array came from the heap */
    } h;
    max_align_t align; /* keeps the array that follows aligned */
} owned_header_t;

void *telebot_owned_calloc(size_t count, size_t size)
{
    if ((size != 0) && (count > (SIZE_MAX - sizeof(owned_header_t)) / size))
        return NULL;
    owned_header_t *header = telebot_mem_calloc(1, sizeof(owned_header_t) + count * size);
    if (header == NULL)
        return NULL;
    header->h.magic = OWNED_MAGIC;
    header->h.arena = current_arena;
    return header + 1;
}

telebot_error_e telebot_owned_release(void *array)
{
    if (array == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    owned_header_t *header = (owned_header_t *)array - 1;
    if (header->h.magic != OWNED_MAGIC)
        return TELEBOT_ERROR_INVALID_PARAMETER; /* not returned by the library, or already released */
    header->h.magic = 0;

    telebot_arena_t *arena = header->h.arena;
    if (arena != NULL)
        telebot_arena_destroy(arena); /* the header and the array live inside it */
    else
        free(header);
    return TELEBOT_ERROR_NONE;
}

/* ---------------------------------------------------------------------------- */
/* Caller-provided results: ownership registry                                   */
/* ---------------------------------------------------------------------------- */

typedef struct owner_entry
{
    const void *object;     /* address of the caller's struct */
    telebot_arena_t *arena; /* memory holding what the struct points to */
} owner_entry_t;

static pthread_mutex_t owners_lock = PTHREAD_MUTEX_INITIALIZER;
static owner_entry_t owners[TELEBOT_MAX_LIVE_RESULTS];

telebot_error_e telebot_owner_attach(const void *object, telebot_arena_t *arena)
{
    if ((object == NULL) || (arena == NULL))
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_arena_t *stale = NULL;
    owner_entry_t *slot = NULL;

    pthread_mutex_lock(&owners_lock);
    for (size_t i = 0; i < TELEBOT_MAX_LIVE_RESULTS; i++)
    {
        if (owners[i].object == object)
        {
            /* The struct is being filled again without a put: drop its old contents */
            stale = owners[i].arena;
            slot = &owners[i];
            break;
        }
        if ((slot == NULL) && (owners[i].object == NULL))
            slot = &owners[i];
    }
    if (slot != NULL)
    {
        slot->object = object;
        slot->arena = arena;
    }
    pthread_mutex_unlock(&owners_lock);

    telebot_arena_destroy(stale);
    return (slot != NULL) ? TELEBOT_ERROR_NONE : TELEBOT_ERROR_OUT_OF_MEMORY;
}

telebot_error_e telebot_owner_release(void *object, size_t size)
{
    if (object == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    telebot_arena_t *arena = NULL;
    pthread_mutex_lock(&owners_lock);
    for (size_t i = 0; i < TELEBOT_MAX_LIVE_RESULTS; i++)
    {
        if (owners[i].object == object)
        {
            arena = owners[i].arena;
            owners[i].object = NULL;
            owners[i].arena = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&owners_lock);

    if (arena == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER; /* a copy, or never filled */

    telebot_arena_destroy(arena);
    memset(object, 0, size); /* leave no dangling pointers behind */
    return TELEBOT_ERROR_NONE;
}
