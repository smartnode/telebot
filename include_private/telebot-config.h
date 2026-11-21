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

/*
 * Build-time tunables. Every value can be overridden with -D on the compiler
 * command line. On ESP-IDF the defaults come from Kconfig (CONFIG_TELEBOT_*),
 * which is how `idf.py menuconfig` controls them.
 */

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif

/* Maximum accepted HTTP response body in bytes; 0 means unlimited. */
#ifndef TELEBOT_MAX_RESPONSE_SIZE
#ifdef CONFIG_TELEBOT_MAX_RESPONSE_SIZE
#define TELEBOT_MAX_RESPONSE_SIZE CONFIG_TELEBOT_MAX_RESPONSE_SIZE
#else
#define TELEBOT_MAX_RESPONSE_SIZE 0
#endif
#endif

/* Number of updates requested by getUpdates when the caller passes limit <= 0. */
#ifndef TELEBOT_DEFAULT_UPDATE_LIMIT
#ifdef CONFIG_TELEBOT_DEFAULT_UPDATE_LIMIT
#define TELEBOT_DEFAULT_UPDATE_LIMIT CONFIG_TELEBOT_DEFAULT_UPDATE_LIMIT
#else
#define TELEBOT_DEFAULT_UPDATE_LIMIT 100
#endif
#endif

/* Chunk size used when streaming files to and from the network. */
#ifndef TELEBOT_IO_CHUNK_SIZE
#ifdef CONFIG_TELEBOT_IO_CHUNK_SIZE
#define TELEBOT_IO_CHUNK_SIZE CONFIG_TELEBOT_IO_CHUNK_SIZE
#else
#define TELEBOT_IO_CHUNK_SIZE 1024
#endif
#endif

/* HTTP timeout in milliseconds for one request (long polling adds its own timeout). */
#ifndef TELEBOT_HTTP_TIMEOUT_MS
#ifdef CONFIG_TELEBOT_HTTP_TIMEOUT_MS
#define TELEBOT_HTTP_TIMEOUT_MS CONFIG_TELEBOT_HTTP_TIMEOUT_MS
#else
#define TELEBOT_HTTP_TIMEOUT_MS 70000
#endif
#endif

/* Block size of the arenas parsed results live in */
#ifndef TELEBOT_ARENA_BLOCK_SIZE
#ifdef CONFIG_TELEBOT_ARENA_BLOCK_SIZE
#define TELEBOT_ARENA_BLOCK_SIZE CONFIG_TELEBOT_ARENA_BLOCK_SIZE
#else
#define TELEBOT_ARENA_BLOCK_SIZE 4096
#endif
#endif

/* Released arena blocks kept for reuse, so steady polling needs no malloc() */
#ifndef TELEBOT_ARENA_CACHE_BLOCKS
#ifdef CONFIG_TELEBOT_ARENA_CACHE_BLOCKS
#define TELEBOT_ARENA_CACHE_BLOCKS CONFIG_TELEBOT_ARENA_CACHE_BLOCKS
#else
#define TELEBOT_ARENA_CACHE_BLOCKS 4
#endif
#endif

/* Results written into caller structs that may be alive at the same time */
#ifndef TELEBOT_MAX_LIVE_RESULTS
#ifdef CONFIG_TELEBOT_MAX_LIVE_RESULTS
#define TELEBOT_MAX_LIVE_RESULTS CONFIG_TELEBOT_MAX_LIVE_RESULTS
#else
#define TELEBOT_MAX_LIVE_RESULTS 16
#endif
#endif

/*
 * Parser feature groups. A disabled group is not parsed: the matching message
 * fields stay NULL and the matching updates arrive as TELEBOT_UPDATE_TYPE_UNKNOWN.
 * Struct layouts never change, so the ABI is the same for every configuration.
 * On ESP-IDF each switch follows Kconfig (CONFIG_TELEBOT_FEATURE_*); elsewhere
 * everything is enabled unless overridden with -DTELEBOT_FEATURE_xxx=0.
 */
#ifndef TELEBOT_FEATURE_PAYMENTS /* invoices, payments, paid media, shipping/checkout queries */
#ifdef ESP_PLATFORM
#ifdef CONFIG_TELEBOT_FEATURE_PAYMENTS
#define TELEBOT_FEATURE_PAYMENTS 1
#else
#define TELEBOT_FEATURE_PAYMENTS 0
#endif
#else
#define TELEBOT_FEATURE_PAYMENTS 1
#endif
#endif

#ifndef TELEBOT_FEATURE_PASSPORT /* Telegram Passport data */
#ifdef ESP_PLATFORM
#ifdef CONFIG_TELEBOT_FEATURE_PASSPORT
#define TELEBOT_FEATURE_PASSPORT 1
#else
#define TELEBOT_FEATURE_PASSPORT 0
#endif
#else
#define TELEBOT_FEATURE_PASSPORT 1
#endif
#endif

#ifndef TELEBOT_FEATURE_BUSINESS /* business connections and business messages */
#ifdef ESP_PLATFORM
#ifdef CONFIG_TELEBOT_FEATURE_BUSINESS
#define TELEBOT_FEATURE_BUSINESS 1
#else
#define TELEBOT_FEATURE_BUSINESS 0
#endif
#else
#define TELEBOT_FEATURE_BUSINESS 1
#endif
#endif

#ifndef TELEBOT_FEATURE_GIFTS /* gifts, unique gifts, giveaways */
#ifdef ESP_PLATFORM
#ifdef CONFIG_TELEBOT_FEATURE_GIFTS
#define TELEBOT_FEATURE_GIFTS 1
#else
#define TELEBOT_FEATURE_GIFTS 0
#endif
#else
#define TELEBOT_FEATURE_GIFTS 1
#endif
#endif

#ifndef TELEBOT_FEATURE_CHAT_EXTRAS /* forum/video-chat/boost events, reactions, shared users/chats */
#ifdef ESP_PLATFORM
#ifdef CONFIG_TELEBOT_FEATURE_CHAT_EXTRAS
#define TELEBOT_FEATURE_CHAT_EXTRAS 1
#else
#define TELEBOT_FEATURE_CHAT_EXTRAS 0
#endif
#else
#define TELEBOT_FEATURE_CHAT_EXTRAS 1
#endif
#endif

#ifndef TELEBOT_FEATURE_WIDGETS /* widget queries (TON wallets, Mini App widgets) */
#ifdef ESP_PLATFORM
#ifdef CONFIG_TELEBOT_FEATURE_WIDGETS
#define TELEBOT_FEATURE_WIDGETS 1
#else
#define TELEBOT_FEATURE_WIDGETS 0
#endif
#else
#define TELEBOT_FEATURE_WIDGETS 1
#endif
#endif

#ifdef __cplusplus
#define TELEBOT_STATIC_ASSERT static_assert
#else
#define TELEBOT_STATIC_ASSERT _Static_assert
#endif

TELEBOT_STATIC_ASSERT(TELEBOT_MAX_LIVE_RESULTS >= 1, "TELEBOT_MAX_LIVE_RESULTS must be at least 1");
TELEBOT_STATIC_ASSERT(TELEBOT_ARENA_BLOCK_SIZE >= 256, "TELEBOT_ARENA_BLOCK_SIZE is too small");
TELEBOT_STATIC_ASSERT(TELEBOT_IO_CHUNK_SIZE >= 128, "TELEBOT_IO_CHUNK_SIZE is too small");
TELEBOT_STATIC_ASSERT(TELEBOT_DEFAULT_UPDATE_LIMIT >= 1 && TELEBOT_DEFAULT_UPDATE_LIMIT <= 100,
               "TELEBOT_DEFAULT_UPDATE_LIMIT must be within 1..100");

#endif /* __TELEBOT_CONFIG_H__ */
