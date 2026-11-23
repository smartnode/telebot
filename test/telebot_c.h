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

/* Single entry point for C headers used by the C++ unit tests. */
#pragma once

extern "C" {
#include <telebot.h>
#include <telebot-core.h>
#include <telebot-private.h>
#include <telebot-parser.h>
#include "telebot-json.h"
#include "telebot-transport.h"
#include "telebot-limits.h"
#include "telebot-arena.h"
}

// Parses inside an arena like the library does: keep() hands the arena to the
// result (e.g. an update array's hidden header); otherwise it is destroyed.
struct ParseScope
{
    ParseScope() : arena(telebot_arena_begin()) {}
    ~ParseScope()
    {
        if (arena)
            telebot_arena_finish(arena, false);
    }
    void keep()
    {
        telebot_arena_finish(arena, true);
        arena = nullptr;
    }
    telebot_arena_t *arena;
};
