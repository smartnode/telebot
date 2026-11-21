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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "telebot-arena.h"
#include "telebot-config.h"

#ifndef TELEBOT_ARENA_BLOCK_SIZE
#ifdef CONFIG_TELEBOT_ARENA_BLOCK_SIZE
#define TELEBOT_ARENA_BLOCK_SIZE CONFIG_TELEBOT_ARENA_BLOCK_SIZE
#else
#define TELEBOT_ARENA_BLOCK_SIZE 4096
#endif
#endif

#define ARENA_ALIGN (sizeof(max_align_t))

typedef struct arena_block
{
    struct arena_block *next;
    size_t size; /* usable bytes after the header */
    size_t used;
    max_align_t data[]; /* aligned start of the usable area */
} arena_block_t;

struct telebot_arena
{
    arena_block_t *head; /* block currently being carved */
    size_t total_used;
};

static _Thread_local telebot_arena_t *current_arena = NULL;

static arena_block_t *block_new(size_t min_size)
{
    size_t size = (min_size > TELEBOT_ARENA_BLOCK_SIZE) ? min_size : TELEBOT_ARENA_BLOCK_SIZE;
    arena_block_t *block = malloc(sizeof(arena_block_t) + size);
    if (block == NULL)
        return NULL;
    block->next = NULL;
    block->size = size;
    block->used = 0;
    return block;
}

telebot_arena_t *telebot_arena_create(void)
{
    telebot_arena_t *arena = calloc(1, sizeof(*arena));
    if (arena == NULL)
        return NULL;
    arena->head = block_new(0);
    if (arena->head == NULL)
    {
        free(arena);
        return NULL;
    }
    return arena;
}

void telebot_arena_destroy(telebot_arena_t *arena)
{
    if (arena == NULL)
        return;
    for (arena_block_t *block = arena->head; block != NULL;)
    {
        arena_block_t *next = block->next;
        free(block);
        block = next;
    }
    free(arena);
}

size_t telebot_arena_used(const telebot_arena_t *arena)
{
    return arena ? arena->total_used : 0;
}

static void *arena_alloc(telebot_arena_t *arena, size_t size)
{
    if (size == 0)
        size = 1;
    size_t aligned = (size + ARENA_ALIGN - 1) & ~(ARENA_ALIGN - 1);
    if (aligned < size) /* overflow */
        return NULL;

    arena_block_t *block = arena->head;
    if (block->size - block->used < aligned)
    {
        /* New block in front; the remainder of the old one is simply left unused */
        arena_block_t *fresh = block_new(aligned);
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
