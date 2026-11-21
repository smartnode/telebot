# TODO

Remaining work after the Bot API 10.4 upgrade. Items are roughly in priority order
within each section.

## Library internals

- [ ] Per-update arena allocator: allocate everything a parsed update owns from one arena
      and release it in one call. This removes the hand-written `telebot_put_*` functions,
      which have been the source of every memory leak found so far.
- [ ] Move the remaining 77 core functions in `src/telebot-core.c` (forums, chat settings,
      commands, menu buttons, invite links, gifts, stars, inline, games, passport) from
      hand-indexed `telebot_core_mime_t mimes[N]` arrays to the request builder
      (`src/telebot-request.h`), then drop the `CHECK_ARG_*` macros.
- [ ] Route cJSON allocations through `cJSON_InitHooks()` so ESP32 users can place parsed
      JSON in PSRAM.
- [ ] Consider table-driven parsing for the remaining hand-written parsers; the parser is
      still about 60% of the library's flash.

## Public API

- [ ] `telebot_edit_message_media()` takes a raw JSON string; switch it to
      `const telebot_input_media_t *` so new media can be uploaded from a path or memory.
- [ ] `telebot_set_webhook()` takes the certificate as a path; switch it to
      `const telebot_input_file_t *`.
- [ ] Const-correctness: `telebot_create()` and `telebot_set_webhook()` take `char *`
      where `const char *` is meant.
- [ ] Remove or rename the misspelt, unused `telebot_response_paramters_t`; expose
      `migrate_to_chat_id` from error responses the way `retry_after` is exposed.
- [ ] Parse the payloads of `managed_bot`, `subscription`, `widget_query` and
      `shared_widget_query` updates (they currently arrive as `TELEBOT_UPDATE_TYPE_UNKNOWN`).

## Bot API coverage (reachable today through `telebot_core_call()`)

Not wrapped on purpose, because they are rarely useful on small devices. Wrap them on
demand, behind a `TELEBOT_FEATURE_*` switch where they add parsing code:

- [ ] Checklists (`sendChecklist`, `editMessageChecklist`); these need a business connection.
- [ ] Rich messages (`sendRichMessage`, `sendRichMessageDraft`, `rich_message` on edits).
- [ ] Ephemeral messages (`ephemeral_message_parameters`, `editEphemeralMessage*`,
      `deleteEphemeralMessage`).
- [ ] Business account management, stories, managed bots, guest mode answers
      (`answerGuestQuery`), widgets and TON, live photos, poll media, chat share themes,
      sticker set editing.

## Release

- [ ] Add a CHANGELOG entry for 10.4.0 summarising the breaking changes
      (see "Migrating from 9.x to 10.x" in README.md).
- [ ] Tag `v10.4.0` and publish the component to the ESP Component Registry
      (`idf_component.yml` is ready).
