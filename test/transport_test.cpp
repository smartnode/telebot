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

#include <gtest/gtest.h>

#include <cstdio>
#include <string>

#include "telebot_c.h"

namespace
{

TEST(MemorySink, AccumulatesAndTerminates)
{
    telebot_memory_sink_t sink;
    telebot_memory_sink_init(&sink, 0);

    EXPECT_TRUE(telebot_sink_write(&sink.base, "hello ", 6));
    EXPECT_TRUE(telebot_sink_write(&sink.base, "world", 5));
    EXPECT_TRUE(telebot_sink_write(&sink.base, "", 0));

    size_t size = 0;
    char *data = telebot_memory_sink_take(&sink, &size);
    ASSERT_NE(data, nullptr);
    EXPECT_EQ(size, 11u);
    EXPECT_STREQ(data, "hello world");
    free(data);

    // After take() the sink is empty and release() is a no-op.
    EXPECT_EQ(telebot_memory_sink_take(&sink, &size), nullptr);
    telebot_memory_sink_release(&sink);
}

TEST(MemorySink, EnforcesLimit)
{
    telebot_memory_sink_t sink;
    telebot_memory_sink_init(&sink, 8);

    EXPECT_TRUE(telebot_sink_write(&sink.base, "12345678", 8));
    EXPECT_FALSE(telebot_sink_write(&sink.base, "9", 1));
    EXPECT_EQ(sink.error, TELEBOT_ERROR_OUT_OF_MEMORY);
    telebot_memory_sink_release(&sink);
}

TEST(FileSink, WritesThrough)
{
    FILE *fp = tmpfile();
    ASSERT_NE(fp, nullptr);
    telebot_file_sink_t sink;
    telebot_file_sink_init(&sink, fp);

    EXPECT_TRUE(telebot_sink_write(&sink.base, "abc", 3));
    EXPECT_TRUE(telebot_sink_write(&sink.base, "def", 3));

    rewind(fp);
    char buf[16] = {};
    EXPECT_EQ(fread(buf, 1, sizeof(buf), fp), 6u);
    EXPECT_STREQ(buf, "abcdef");
    fclose(fp);
}

std::string format(telebot_core_mime_t m)
{
    char buf[TELEBOT_MIME_SCALAR_MAX];
    const char *s = telebot_mime_format_scalar(&m, buf);
    return s ? s : "<null>";
}

TEST(MimeFormat, Scalars)
{
    telebot_core_mime_t m{};
    m.type = TELEBOT_MIME_TYPE_INT;
    m.data.d = -42;
    EXPECT_EQ(format(m), "-42");

    m.type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    m.data.lld = -1001234567890LL;
    EXPECT_EQ(format(m), "-1001234567890");

    m.type = TELEBOT_MIME_TYPE_BOOL;
    m.data.b = true;
    EXPECT_EQ(format(m), "true");
    m.data.b = false;
    EXPECT_EQ(format(m), "false");

    m.type = TELEBOT_MIME_TYPE_FLOAT;
    m.data.f = 37.5665f;
    EXPECT_EQ(format(m), "37.5665");

    m.type = TELEBOT_MIME_TYPE_DOUBLE;
    m.data.lf = 126.978;
    EXPECT_EQ(format(m), "126.978");
}

TEST(MimeFormat, NonScalarsReturnNull)
{
    telebot_core_mime_t m{};
    m.type = TELEBOT_MIME_TYPE_STRING;
    m.data.s = "x";
    EXPECT_EQ(format(m), "<null>");
    m.type = TELEBOT_MIME_TYPE_FILE;
    EXPECT_EQ(format(m), "<null>");
    m.type = TELEBOT_MIME_TYPE_BUFFER;
    EXPECT_EQ(format(m), "<null>");
}

} // namespace
