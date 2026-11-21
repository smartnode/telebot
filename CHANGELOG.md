# Changelog

## 10.4.0 (unreleased)

Follows Telegram Bot API 10.4. This is a breaking release; see "Migrating from 9.x to 10.x" in
[README.md](README.md) for the signature changes.

### Platforms

- ESP32 support: the repository is an ESP-IDF component (ESP-IDF 5.1 or newer, tested with
  v6.1.1) with an `esp_http_client` backend and the `examples/esp32_echo` application.
- JSON is handled by a vendored cJSON (`third_party/cjson`); json-c is no longer needed.
- HTTP goes through a pluggable transport: libcurl on Linux/macOS, `esp_http_client` on ESP-IDF.
  Uploads are streamed in chunks from files or memory.

### Memory and size

- Build-time limits for response size, update batch size, I/O chunk, URL buffer and arena block
  size, configurable through `menuconfig` on ESP-IDF.
- `TELEBOT_FEATURE_*` switches compile out parsing of payments, business, gifts, passport, chat
  events and widgets.
- Every parsed result lives in one arena released by its `telebot_put_*` function; released
  blocks are cached, so steady polling makes no heap allocations. Ownership is tracked inside
  the library: a copy of a result is rejected instead of being freed twice, and filling a struct
  again releases its previous contents.
- Scalar fields are parsed from tables instead of per-field code.
- Request JSON is written by a streaming writer straight into the request (no JSON tree,
  at most one allocation for large values); cJSON is used only to read replies.
- Response buffers grow geometrically instead of reallocating per network chunk.
- `telebot_set_json_allocator()` lets the JSON tree live in PSRAM.
- Telebot takes about 30.8 KB of flash on ESP32 with the default features.

### API

- Send functions take `(chat_id, content, extras, const telebot_send_options_t *options)`.
- Files are `telebot_input_file_t` values: a file_id or URL, a local path, or bytes in memory
  (`TELEBOT_FILE_REMOTE()`, `TELEBOT_FILE_PATH()`, `TELEBOT_FILE_MEMORY()`).
- 64-bit user identifiers everywhere; chat permissions, administrator rights and invoices are
  structs; media groups and `telebot_edit_message_media()` take `telebot_input_media_t`.
- `telebot_set_webhook()` takes the certificate as a file and supports `drop_pending_updates` and
  `secret_token`.
- New methods: `sendMessageDraft`, `unpinAllChatMessages`, `setChatMemberTag`,
  `deleteMessageReaction`, `deleteAllMessageReactions`, `verifyUser`, `verifyChat`,
  `removeUserVerification`, `removeChatVerification`, `answerWidgetQuery`, and
  `getChatAdministrators` with `return_bots`.
- New updates: `guest_message`, `stopped_message_generation`, `managed_bot`, `subscription`,
  `widget_query` and `shared_widget_query`. Updates this version does not know arrive as
  `TELEBOT_UPDATE_TYPE_UNKNOWN`.
- `telebot_core_call()` calls any Bot API method by name.
- Flood control returns `TELEBOT_ERROR_RATE_LIMITED`; `telebot_get_retry_after()` gives the wait.
  `telebot_core_get_response_migrate_to_chat_id()` reports group migrations.
- `telebot_put_message()`, `telebot_put_stickers()` and `telebot_get_sticker_set()` are available;
  every declared function is implemented.
- Removed: `telebot_kick_chat_member()` (use `telebot_ban_chat_member()`), the `_buffer` send
  variants (use `TELEBOT_FILE_MEMORY()`), and `telebot_response_paramters_t`.

### Fixes

- Functions that rejected negative chat ids now work in groups, supergroups and channels.
- Stickers, sticker sets and media thumbnails are parsed.
- Every member the parser allocates for a message, chat member or update is released.
- `setWebhook` sends `max_connections` and valid `allowed_updates` JSON.
- `setMyProfilePhoto` sends an `InputProfilePhoto`.
- Webhook info matches update names exactly instead of by substring.
- An empty long poll returns success with zero updates.

### Tooling

- GoogleTest unit tests that need no network, using a fake transport.
- A check that every function declared in a public header is defined.
- Doxygen builds without warnings, enforced in CI.
