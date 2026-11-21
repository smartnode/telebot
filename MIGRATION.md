# Migrating from telebot 9.x to 10.x

Version 10 follows Bot API 10.4 and changes many signatures. The compiler points out every call that
needs updating; the patterns are:

| 9.x | 10.x |
|---|---|
| `const char *photo, bool is_file` (all media) | `const telebot_input_file_t *photo`, built with `TELEBOT_FILE_REMOTE()`, `TELEBOT_FILE_PATH()` or `TELEBOT_FILE_MEMORY()` (or the `telebot_input_file_*()` functions in C++) |
| `bool disable_notification, int reply_to_message_id, const char *reply_markup` | one `const telebot_send_options_t *options` (or `NULL`), which also covers forum topics, protected content and message effects |
| `bool disable_web_page_preview` | `bool disable_link_preview` |
| `const char *thumb` | `const telebot_input_file_t *thumbnail` (uploaded from a path or memory) |
| `int user_id` | `long long int user_id` |
| positional `can_*` booleans in restrict/promote/set_chat_permissions | `telebot_chat_permissions_t` / `telebot_chat_administrator_rights_t` |
| `send_poll(..., const char **options, ..., int correct_option_id, ...)` | `send_poll(..., const char *const answers[], ..., const int correct_option_ids[], int count_correct, ...)` |
| `send_media_group(char *media_paths[], ...)` | `send_media_group(const telebot_input_media_t media[], ...)` |
| 25 positional invoice arguments | `const telebot_invoice_params_t *invoice` |
| `float latitude, float longitude` | `double latitude, double longitude` |
| `telebot_kick_chat_member()` | `telebot_ban_chat_member()` |
| `thumb` members of media structs | `thumbnail` |

Unrecognised updates now arrive as `TELEBOT_UPDATE_TYPE_UNKNOWN`, and an empty long poll returns
`TELEBOT_ERROR_NONE` with `count == 0`.

For Bot API methods the library does not wrap, use `telebot_core_call()` with name/value parameters.
