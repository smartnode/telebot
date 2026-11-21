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

/* Arena allocator and arena-backed update batches. */
#include <cstdint>
#include <cstring>
#include <string>

#include "core_fixture.h"

extern "C" {
#include "telebot-arena.h"
}

namespace
{

TEST(Arena, AllocationsAreZeroedAndAligned)
{
    telebot_arena_t *arena = telebot_arena_create();
    ASSERT_NE(arena, nullptr);
    telebot_arena_t *prev = telebot_arena_enter(arena);

    for (int i = 1; i < 50; i++)
    {
        auto *p = static_cast<unsigned char *>(telebot_mem_calloc(1, i));
        ASSERT_NE(p, nullptr);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(p) % alignof(max_align_t), 0u);
        for (int j = 0; j < i; j++)
            EXPECT_EQ(p[j], 0);
        memset(p, 0xAB, i);
    }
    telebot_arena_leave(prev);
    EXPECT_GT(telebot_arena_used(arena), 0u);
    telebot_arena_destroy(arena);
}

TEST(Arena, GrowsBeyondOneBlockAndHandlesLargeAllocations)
{
    telebot_arena_t *arena = telebot_arena_create();
    telebot_arena_t *prev = telebot_arena_enter(arena);
    std::string big(100000, 'x');
    char *copy = telebot_mem_strdup(big.c_str());
    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(std::string(copy), big);
    for (int i = 0; i < 1000; i++)
        ASSERT_NE(telebot_mem_malloc(64), nullptr);
    telebot_arena_leave(prev);
    telebot_arena_destroy(arena);
}

TEST(Arena, FreeIsANoOpWhileAnArenaIsCurrent)
{
    telebot_arena_t *arena = telebot_arena_create();
    telebot_arena_t *prev = telebot_arena_enter(arena);
    void *p = telebot_mem_calloc(4, 8);
    telebot_mem_free(p); // must not hand arena memory to free()
    telebot_arena_leave(prev);
    telebot_arena_destroy(arena);
}

TEST(Arena, WithoutAnArenaTheCHeapIsUsed)
{
    char *s = telebot_mem_strdup("heap");
    ASSERT_NE(s, nullptr);
    EXPECT_STREQ(s, "heap");
    telebot_mem_free(s);
    EXPECT_EQ(telebot_mem_strdup(nullptr), nullptr);
}

TEST(Arena, EnterAndLeaveNest)
{
    telebot_arena_t *outer = telebot_arena_create();
    telebot_arena_t *inner = telebot_arena_create();
    telebot_arena_t *a = telebot_arena_enter(outer);
    telebot_arena_t *b = telebot_arena_enter(inner);
    EXPECT_EQ(b, outer);
    telebot_mem_calloc(1, 16);
    telebot_arena_leave(b);
    telebot_mem_calloc(1, 16);
    telebot_arena_leave(a);
    EXPECT_EQ(telebot_arena_used(inner), telebot_arena_used(outer));
    telebot_arena_destroy(inner);
    telebot_arena_destroy(outer);
}

class ArenaUpdates : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_EQ(telebot_create(&handle_, "123456:TEST-TOKEN"), TELEBOT_ERROR_NONE);
        fake_ = FakeTransport::install(handle_->core_h);
    }
    void TearDown() override { telebot_destroy(handle_); }

    telebot_handler_t handle_ = nullptr;
    FakeTransport *fake_ = nullptr;
};

TEST_F(ArenaUpdates, BatchFromGetUpdatesIsOneArena)
{
    fake_->reply(200, R"({"ok":true,"result":[
        {"update_id":1,"message":{"message_id":1,"date":1,"chat":{"id":-100,"type":"supergroup","title":"G"},
          "from":{"id":7123456789,"is_bot":false,"first_name":"A"},"text":"hello",
          "entities":[{"offset":0,"length":5,"type":"bold"}],
          "forward_origin":{"type":"user","date":1,"sender_user":{"id":3,"is_bot":false,"first_name":"F"}},
          "quote":{"text":"q","position":0},
          "sticker":{"file_id":"S","file_unique_id":"s","type":"regular","width":1,"height":1,"is_animated":false,
                     "is_video":false,"thumbnail":{"file_id":"T","file_unique_id":"t","width":1,"height":1}},
          "photo":[{"file_id":"P","file_unique_id":"p","width":1,"height":1}]}},
        {"update_id":2,"callback_query":{"id":"c","from":{"id":4,"is_bot":false,"first_name":"B"},
          "chat_instance":"i","data":"x","message":{"message_id":9,"date":1,"chat":{"id":4,"type":"private"}}}},
        {"update_id":3,"chat_member":{"chat":{"id":-100,"type":"supergroup","title":"G"},
          "from":{"id":5,"is_bot":false,"first_name":"C"},"date":1,
          "old_chat_member":{"status":"member","user":{"id":6,"is_bot":false,"first_name":"D"},"tag":"t"},
          "new_chat_member":{"status":"administrator","user":{"id":6,"is_bot":false,"first_name":"D"},
                             "custom_title":"Boss","can_manage_tags":true}}}]})");

    telebot_update_t *updates = nullptr;
    int count = 0;
    ASSERT_EQ(telebot_get_updates(handle_, 0, 0, 0, nullptr, 0, &updates, &count), TELEBOT_ERROR_NONE);
    ASSERT_EQ(count, 3);
    EXPECT_STREQ(updates[0].message.text, "hello");
    ASSERT_NE(updates[0].message.sticker, nullptr);
    EXPECT_STREQ(updates[0].message.sticker->thumbnail->file_id, "T");
    EXPECT_STREQ(updates[1].callback_query.data, "x");

    // Library allocations made after parsing must use the heap again
    char *heap = telebot_mem_strdup("after");
    telebot_mem_free(heap);

    EXPECT_EQ(telebot_put_updates(updates, count), TELEBOT_ERROR_NONE);
}

TEST_F(ArenaUpdates, EmptyOrFailedPollsLeaveNothingBehind)
{
    fake_->reply(200, R"({"ok":true,"result":[]})");
    telebot_update_t *updates = reinterpret_cast<telebot_update_t *>(0x1);
    int count = -1;
    EXPECT_EQ(telebot_get_updates(handle_, 0, 0, 0, nullptr, 0, &updates, &count), TELEBOT_ERROR_NONE);
    EXPECT_EQ(updates, nullptr);
    EXPECT_EQ(count, 0);

    fake_->reply(200, R"({"ok":true,"result":{"not":"an array"}})");
    EXPECT_NE(telebot_get_updates(handle_, 0, 0, 0, nullptr, 0, &updates, &count), TELEBOT_ERROR_NONE);
    EXPECT_EQ(updates, nullptr);
}

// ---- Ownership of results written into the caller's struct ----

const char *kMeReply = R"({"ok":true,"result":{"id":8123456789,"is_bot":true,"first_name":"Probe","username":"probe_bot"}})";

TEST_F(ArenaUpdates, CopyOfAResultIsRejectedNotFreedTwice)
{
    fake_->reply(200, kMeReply);
    telebot_user_t me;
    ASSERT_EQ(telebot_get_me(handle_, &me), TELEBOT_ERROR_NONE);

    telebot_user_t copy = me; // plain data copy: no ownership inside it
    EXPECT_STREQ(copy.username, "probe_bot");
    EXPECT_EQ(telebot_put_me(&copy), TELEBOT_ERROR_INVALID_PARAMETER);

    EXPECT_EQ(telebot_put_me(&me), TELEBOT_ERROR_NONE);
    EXPECT_EQ(me.username, nullptr); // zeroed: no dangling pointers
    EXPECT_EQ(telebot_put_me(&me), TELEBOT_ERROR_INVALID_PARAMETER); // second put is harmless
}

TEST_F(ArenaUpdates, RefillingAStructReleasesItsPreviousContents)
{
    telebot_user_t me;
    for (int i = 0; i < 5; i++)
    {
        fake_->reply(200, kMeReply);
        ASSERT_EQ(telebot_get_me(handle_, &me), TELEBOT_ERROR_NONE); // no put in between
    }
    EXPECT_EQ(telebot_put_me(&me), TELEBOT_ERROR_NONE); // checked by the leak run
}

TEST_F(ArenaUpdates, FailedCallLeavesAnEmptyStruct)
{
    fake_->reply(200, R"({"ok":false,"description":"nope"})");
    telebot_user_t me;
    memset(&me, 0xA5, sizeof(me)); // garbage, as on an uninitialised stack
    EXPECT_NE(telebot_get_me(handle_, &me), TELEBOT_ERROR_NONE);
    EXPECT_EQ(me.username, nullptr);
    EXPECT_EQ(telebot_put_me(&me), TELEBOT_ERROR_INVALID_PARAMETER);
}

TEST_F(ArenaUpdates, TooManyLiveResultsIsReportedNotCorrupted)
{
    static telebot_user_t users[TELEBOT_MAX_LIVE_RESULTS + 1];
    for (int i = 0; i < TELEBOT_MAX_LIVE_RESULTS; i++)
    {
        fake_->reply(200, kMeReply);
        ASSERT_EQ(telebot_get_me(handle_, &users[i]), TELEBOT_ERROR_NONE);
    }
    fake_->reply(200, kMeReply);
    telebot_user_t &extra = users[TELEBOT_MAX_LIVE_RESULTS];
    EXPECT_EQ(telebot_get_me(handle_, &extra), TELEBOT_ERROR_OUT_OF_MEMORY);
    EXPECT_EQ(extra.username, nullptr);

    for (int i = 0; i < TELEBOT_MAX_LIVE_RESULTS; i++)
        EXPECT_EQ(telebot_put_me(&users[i]), TELEBOT_ERROR_NONE);
}

TEST_F(ArenaUpdates, ArrayResultsAreReleasedThroughTheirHeader)
{
    fake_->reply(200, R"({"ok":true,"result":[{"command":"start","description":"Start"},{"command":"help","description":"Help"}]})");
    telebot_bot_command_t *commands = nullptr;
    int count = 0;
    ASSERT_EQ(telebot_get_my_commands(handle_, &commands, &count), TELEBOT_ERROR_NONE);
    ASSERT_EQ(count, 2);
    EXPECT_STREQ(commands[1].command, "help");
    EXPECT_EQ(telebot_put_my_commands(commands, count), TELEBOT_ERROR_NONE);

    EXPECT_EQ(telebot_put_my_commands(nullptr, 2), TELEBOT_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(telebot_put_updates(nullptr, 0), TELEBOT_ERROR_INVALID_PARAMETER);
}

TEST(ArenaCache, ReleasedBlocksAreReused)
{
    if (TELEBOT_ARENA_CACHE_BLOCKS == 0)
        GTEST_SKIP() << "block cache disabled in this build";
    telebot_arena_trim_cache();
    telebot_arena_t *a = telebot_arena_create();
    const void *first = a;
    telebot_arena_destroy(a);

    telebot_arena_t *b = telebot_arena_create(); // served from the cache
    EXPECT_EQ(static_cast<const void *>(b), first);
    telebot_arena_destroy(b);
    telebot_arena_trim_cache();
}

} // namespace

