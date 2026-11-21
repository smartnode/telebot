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

extern "C" {
#include "telebot-multipart.h"
}

namespace
{

const char *kBoundary = "----telebotBOUNDARY";

bool append(void *ctx, const void *data, size_t len)
{
    static_cast<std::string *>(ctx)->append(static_cast<const char *>(data), len);
    return true;
}

// Encodes twice (count, then write) and checks both passes agree.
std::string encode(const std::vector<telebot_core_mime_t> &parts, telebot_error_e expect = TELEBOT_ERROR_NONE)
{
    char chunk[128];
    telebot_multipart_writer_t counter{};
    EXPECT_EQ(telebot_multipart_encode(&counter, kBoundary, parts.data(), parts.size(), chunk, sizeof(chunk)),
              expect);

    std::string body;
    telebot_multipart_writer_t writer{};
    writer.write = append;
    writer.ctx = &body;
    EXPECT_EQ(telebot_multipart_encode(&writer, kBoundary, parts.data(), parts.size(), chunk, sizeof(chunk)),
              expect);
    if (expect == TELEBOT_ERROR_NONE)
    {
        EXPECT_EQ(counter.total, body.size()) << "Content-Length must match the bytes sent";
        EXPECT_EQ(writer.total, body.size());
    }
    return body;
}

telebot_core_mime_t str(const char *name, const char *value)
{
    telebot_core_mime_t m{};
    m.type = TELEBOT_MIME_TYPE_STRING;
    m.name = name;
    m.data.s = value;
    return m;
}

TEST(Multipart, ScalarAndStringFields)
{
    telebot_core_mime_t id{};
    id.type = TELEBOT_MIME_TYPE_LONG_LONG_INT;
    id.name = "chat_id";
    id.data.lld = -1001234567890LL;

    const std::string body = encode({id, str("text", "hello")});
    EXPECT_EQ(body,
              "------telebotBOUNDARY\r\n"
              "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n"
              "-1001234567890\r\n"
              "------telebotBOUNDARY\r\n"
              "Content-Disposition: form-data; name=\"text\"\r\n\r\n"
              "hello\r\n"
              "------telebotBOUNDARY--\r\n");
}

TEST(Multipart, EmptyRequestIsJustTheTerminator)
{
    EXPECT_EQ(encode({}), "------telebotBOUNDARY--\r\n");
}

TEST(Multipart, BufferUploadCarriesFilenameAndBinaryBytes)
{
    static const char jpeg[] = {'\xFF', '\xD8', '\x00', '\x01', '\xFF', '\xD9'};
    telebot_core_mime_t photo{};
    photo.type = TELEBOT_MIME_TYPE_BUFFER;
    photo.name = "photo";
    photo.data.buf.ptr = jpeg;
    photo.data.buf.len = sizeof(jpeg);
    photo.data.buf.filename = "cam.jpg";

    const std::string body = encode({photo});
    EXPECT_NE(body.find("name=\"photo\"; filename=\"cam.jpg\"\r\nContent-Type: application/octet-stream\r\n\r\n"),
              std::string::npos);
    EXPECT_NE(body.find(std::string(jpeg, sizeof(jpeg)) + "\r\n"), std::string::npos);
}

TEST(Multipart, FileIsStreamedInChunksWithBasename)
{
    const std::string path = ::testing::TempDir() + "telebot_upload.bin";
    std::string content(1000, 'x'); // larger than the 128-byte chunk
    content[999] = 'y';
    FILE *fp = fopen(path.c_str(), "wb");
    ASSERT_NE(fp, nullptr);
    fwrite(content.data(), 1, content.size(), fp);
    fclose(fp);

    telebot_core_mime_t doc{};
    doc.type = TELEBOT_MIME_TYPE_FILE;
    doc.name = "document";
    doc.data.s = path.c_str();

    const std::string body = encode({doc});
    EXPECT_NE(body.find("filename=\"telebot_upload.bin\""), std::string::npos);
    EXPECT_NE(body.find("\r\n\r\n" + content + "\r\n"), std::string::npos);
    std::remove(path.c_str());
}

TEST(Multipart, MissingFileIsInvalidParameter)
{
    telebot_core_mime_t doc{};
    doc.type = TELEBOT_MIME_TYPE_FILE;
    doc.name = "document";
    doc.data.s = "/nonexistent/telebot/file.bin";
    encode({doc}, TELEBOT_ERROR_INVALID_PARAMETER);
}

TEST(Multipart, HeaderValuesCannotInjectHeaders)
{
    static const char data[] = "x";
    telebot_core_mime_t evil{};
    evil.type = TELEBOT_MIME_TYPE_BUFFER;
    evil.name = "doc";
    evil.data.buf.ptr = data;
    evil.data.buf.len = 1;
    evil.data.buf.filename = "a\"b\r\nX-Injected: 1";

    const std::string body = encode({evil});
    EXPECT_EQ(body.find("\r\nX-Injected"), std::string::npos);
    EXPECT_NE(body.find("filename=\"a_b__X-Injected: 1\""), std::string::npos);
}

TEST(Multipart, WriterFailureStopsEncoding)
{
    telebot_multipart_writer_t writer{};
    writer.write = [](void *, const void *, size_t) { return false; };
    telebot_core_mime_t m = str("text", "hello");
    char chunk[16];
    EXPECT_EQ(telebot_multipart_encode(&writer, kBoundary, &m, 1, chunk, sizeof(chunk)),
              TELEBOT_ERROR_NO_CONNECTION);
    EXPECT_EQ(writer.total, 0u);
}

TEST(Multipart, BoundaryAndContentType)
{
    const uint8_t rnd[8] = {0x00, 0x01, 0xAB, 0xCD, 0xEF, 0x10, 0x7F, 0xFF};
    char boundary[TELEBOT_MULTIPART_BOUNDARY_SIZE];
    telebot_multipart_boundary(boundary, rnd);
    EXPECT_STREQ(boundary, "----telebot0001abcdef107fff");

    char ct[64];
    ASSERT_TRUE(telebot_multipart_content_type(ct, sizeof(ct), boundary));
    EXPECT_STREQ(ct, "multipart/form-data; boundary=----telebot0001abcdef107fff");
    char tiny[8];
    EXPECT_FALSE(telebot_multipart_content_type(tiny, sizeof(tiny), boundary));
}

} // namespace
