# TODO

Remaining work after the Bot API 10.4 upgrade. Items are roughly in priority order
within each section.

## Library internals

- [ ] Parse every other result into an arena too (`telebot_get_me()`, `telebot_get_chat()`,
      `telebot_get_chat_member()`, `telebot_get_sticker_set()`, ...). Update batches already
      use one; the other results are still released field by field by the `telebot_put_*`
      functions.
- [ ] Extend the parse tables (`include_private/telebot-fields.h`) to nested objects, arrays
      and required fields. Scalar fields are table-driven; the remaining hand-written blocks are most of
      the parser's 21 KB on ESP32.

## Bot API coverage (reachable today through `telebot_core_call()`)

Not wrapped on purpose, because they are rarely useful on small devices. Wrap them on
demand, behind a `TELEBOT_FEATURE_*` switch where they add parsing code:

- [ ] Checklists (`sendChecklist`, `editMessageChecklist`); these need a business connection.
- [ ] Rich messages (`sendRichMessage`, `sendRichMessageDraft`, `rich_message` on edits).
- [ ] Ephemeral messages (`ephemeral_message_parameters`, `editEphemeralMessage*`,
      `deleteEphemeralMessage`).
- [ ] Business account management, stories, managed bot tokens, guest mode answers
      (`answerGuestQuery`), widget data editing (`editMessageWidgetData`,
      `editSharedWidgetData`), live photos, poll media, chat share themes, sticker set editing.

## Release

- [ ] Tag `v10.4.0` and publish the component to the ESP Component Registry
      (`idf_component.yml` is ready). The changes are listed in CHANGELOG.md.
