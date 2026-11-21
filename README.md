# Telebot

![Build](https://github.com/smartnode/telebot/workflows/Build/badge.svg)
![GitHub repo size](https://img.shields.io/github/repo-size/smartnode/telebot)
![GitHub issues](https://img.shields.io/github/issues/smartnode/telebot)
![GitHub pull requests](https://img.shields.io/github/issues-pr/smartnode/telebot)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

C Library for Telegram bot API, designed to run on Linux/macOS (libcurl) and on low-memory
microcontrollers such as the ESP32 (ESP-IDF). JSON is handled by a bundled copy of
[cJSON](https://github.com/DaveGamble/cJSON), so there is no JSON library to install.
Head over to [Telegram Bots](https://core.telegram.org/bots) page to read about how to register your brand new bot with Telegram platform and see it in action using telebot.

## Building

You need to install libraries and build tools such as CMake.
On Debian-based Linux distributions you can do it as follows:

```sh
sudo apt-get install libcurl4-openssl-dev cmake binutils make
```

On Mac OSX, first install macports from [MacPorts](https://www.macports.org/install.php) and in Terminal

```sh
sudo port install cmake curl
```

To build the library run following commands:

```sh
cd [your repository]
mkdir -p build && cd build
cmake ..
make
```

For debug build
```sh
mkdir -p build && cd build
cmake .. -DDEBUG=1
make
```

## Useful patterns

**Rate limits.** When Telegram's flood control kicks in, calls return `TELEBOT_ERROR_RATE_LIMITED`
and `telebot_get_retry_after()` tells how many seconds to wait:

```c
if (telebot_send_message(bot, chat_id, text, NULL, false, NULL) == TELEBOT_ERROR_RATE_LIMITED)
    sleep(telebot_get_retry_after(bot));
```

**Streaming a reply.** `telebot_send_message_draft()` shows text that grows while it is produced,
for example the output of a slow sensor sweep or a language model. Send the final text with
`telebot_send_message()`; drafts are temporary previews.

```c
telebot_send_message_draft(bot, chat_id, 0, 1, "Measuring...", NULL, true, false);
telebot_send_message_draft(bot, chat_id, 0, 1, "Measuring... 21.5 °C", NULL, true, false);
telebot_send_message(bot, chat_id, "Temperature: 21.5 °C", NULL, false, NULL);
```

With `can_stop` the user sees a stop button; pressing it delivers a
`TELEBOT_UPDATE_TYPE_STOPPED_MESSAGE_GENERATION` update carrying the `draft_id`.

**Methods without a wrapper.** `telebot_core_call()` sends any Bot API method with name/value
parameters, so rarely used methods cost no flash:

```c
telebot_param_t params[] = {{"chat_id", "-1001234567890", NULL}, {"user_id", "42", NULL}};
telebot_core_response_t r = telebot_core_call(core_h, "getChatMember", params, 2);
```

## ESP32 (ESP-IDF)

The repository root is an ESP-IDF component (ESP-IDF 5.1 or newer, tested with v6.1.1).
HTTP goes through `esp_http_client` with the built-in CA bundle, uploads are streamed
from the filesystem in small chunks, and memory limits are set in `idf.py menuconfig`
under **Telebot (Telegram Bot API)**.

Add it to your project's `main/idf_component.yml`:

```yaml
dependencies:
  telebot:
    path: /path/to/telebot   # or a git: dependency
```

A complete echo bot lives in [examples/esp32_echo](https://github.com/smartnode/telebot/tree/master/examples/esp32_echo):

```sh
cd examples/esp32_echo
idf.py set-target esp32
idf.py menuconfig      # Wi-Fi credentials and the bot token
idf.py build flash monitor
```

Run the bot in a task with at least 8 KB of stack, because the TLS handshake runs on the calling task.

**Sending from RAM.** Camera frames and logs built in memory can be sent without a file system:

```c
camera_fb_t *fb = esp_camera_fb_get();
telebot_send_photo(bot, chat_id, TELEBOT_FILE_MEMORY(fb->buf, fb->len, "door.jpg"), "Motion detected",
                   NULL, NULL);
esp_camera_fb_return(fb);
```

**Memory.** Every parsed result (a batch of updates, a chat, a member, ...) lives in one
arena and is released in one go by its `telebot_put_*` function. Released blocks are cached
(`CONFIG_TELEBOT_ARENA_CACHE_BLOCKS`), so a steady polling loop makes no heap allocations.
Ownership is tracked inside the library, never in your structs:

```c
telebot_user_t me;
telebot_get_me(bot, &me);
telebot_user_t copy = me;   /* plain data: valid until telebot_put_me(&me) */
telebot_put_me(&copy);      /* rejected with TELEBOT_ERROR_INVALID_PARAMETER, nothing freed */
telebot_put_me(&me);        /* releases everything and zeroes `me` */
```

Release a result through the struct you passed in. Filling the same struct again releases its
previous contents automatically. At most `CONFIG_TELEBOT_MAX_LIVE_RESULTS` such results can be
alive at once. On boards with PSRAM, the temporary JSON tree can be moved out of internal RAM:

```c
static void *psram_malloc(size_t size) { return heap_caps_malloc(size, MALLOC_CAP_SPIRAM); }
telebot_set_json_allocator(psram_malloc, heap_caps_free);
```

**Choosing what gets parsed.** Most of the library's flash goes to parsing incoming messages.
Under *Telebot → Parsed features* in menuconfig you can switch off groups a device rarely needs
(payments, business, gifts, passport, widgets). Fields of a disabled group stay `NULL` and its updates arrive
as `TELEBOT_UPDATE_TYPE_UNKNOWN`. On Linux/macOS everything is enabled; pass for example
`-DTELEBOT_FEATURE_PAYMENTS=0` in `CMAKE_C_FLAGS` to switch a group off there.

Flash used by telebot in the ESP32 echo example (ESP-IDF v6.1.1, cJSON included, 100 bytes of static RAM):

| Parsed features | Flash |
|---|---|
| everything | 33.2 KB |
| ESP default (chat events only) | 29.2 KB |
| minimal (all groups off) | 26.0 KB |

## Testing

Unit tests use GoogleTest, which is downloaded automatically when it isn't installed. They need no
network access and no bot token:

```sh
cd build && ctest --output-on-failure
```

Pass `-DTELEBOT_BUILD_TESTS=OFF` to cmake to skip them.

## Examples

| Example | Platform | What it shows |
|---|---|---|
| [host_demo_bot](https://github.com/smartnode/telebot/tree/master/examples/host_demo_bot) | Linux / macOS | Many API calls driven by chat commands: media, polls, keyboards |
| [esp32_echo](https://github.com/smartnode/telebot/tree/master/examples/esp32_echo) | ESP32 (ESP-IDF) | Minimal long-polling echo bot with memory logging |

To run the host demo, put your bot token in a `.token` file in the current directory:
```sh
./build/examples/host_demo_bot/demo_bot
```
Pass `-DTELEBOT_BUILD_EXAMPLES=OFF` to cmake to skip building it.

<details>
<summary>Sample</summary>

Following sample creates a simple dummy bot which echoes back the messages sent to it.
A more comprehensive [demo bot](https://github.com/smartnode/telebot/blob/master/examples/host_demo_bot/demo_bot.c) is built as `demo_bot` under `build/examples/host_demo_bot`.
It supports various commands to test APIs, polls, custom keyboards, and multimedia sending.
The executable expects and reads bot token from `.token` file on the same location.

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <telebot.h>

#define SIZE_OF_ARRAY(array) (sizeof(array) / sizeof(array[0]))

int main(void)
{
    printf("Welcome to Echobot\n");

    FILE *fp = fopen(".token", "r");
    if (fp == NULL)
    {
        printf("Failed to open .token file\n");
        return -1;
    }

    char token[1024];
    if (fscanf(fp, "%s", token) == 0)
    {
        printf("Failed to read token\n");
        fclose(fp);
        return -1;
    }
    printf("Token: %s\n", token);
    fclose(fp);

    telebot_handler_t handle;
    if (telebot_create(&handle, token) != TELEBOT_ERROR_NONE)
    {
        printf("Telebot create failed\n");
        return -1;
    }

    telebot_user_t me;
    if (telebot_get_me(handle, &me) != TELEBOT_ERROR_NONE)
    {
        printf("Failed to get bot information\n");
        telebot_destroy(handle);
        return -1;
    }

    printf("ID: %lld\n", me.id);
    printf("First Name: %s\n", me.first_name);
    printf("User Name: %s\n", me.username);

    telebot_put_me(&me);

    int index, count, offset = -1;
    telebot_error_e ret;
    telebot_message_t message;
    telebot_update_type_e update_types[] = {TELEBOT_UPDATE_TYPE_MESSAGE};

    while (1)
    {
        telebot_update_t *updates;
        /* Long polling: waits up to 30 s for new updates */
        ret = telebot_get_updates(handle, offset, 20, 30, update_types, 1, &updates, &count);
        if (ret != TELEBOT_ERROR_NONE)
        {
            sleep(1);
            continue;
        }
        printf("Number of updates: %d\n", count);
        for (index = 0; index < count; index++)
        {
            message = updates[index].message;
            if (message.text)
            {
                printf("%s: %s \n", message.from->first_name, message.text);
                if (strstr(message.text, "/dice"))
                {
                    ret = telebot_send_dice(handle, message.chat->id, NULL, NULL);
                }
                else
                {
                    char str[4096];
                    if (strstr(message.text, "/start"))
                        snprintf(str, SIZE_OF_ARRAY(str), "Hello %s", message.from->first_name);
                    else
                        snprintf(str, SIZE_OF_ARRAY(str), "%s", message.text);

                    /* Reply to the received message */
                    telebot_send_options_t opts = {0};
                    opts.reply_to_message_id = message.message_id;
                    ret = telebot_send_message(handle, message.chat->id, str, NULL, false, &opts);
                }
                if (ret != TELEBOT_ERROR_NONE)
                {
                    printf("Failed to send message: %d \n", ret);
                }
            }
            offset = updates[index].update_id + 1;
        }
        if (count > 0)
            telebot_put_updates(updates, count);
    }

    telebot_destroy(handle);

    return 0;
}
```

</details>

## Documentation

The API reference is published at **https://smartnode.github.io/telebot/** and rebuilt from
`master` on every push. Start with the
[Telebot API](https://smartnode.github.io/telebot/group__TELEBOT__API.html) (parsed results)
or the [core API](https://smartnode.github.io/telebot/group__TELEBOT__CORE__API.html) (raw
JSON responses).

To build it locally with [Doxygen](https://www.doxygen.nl) 1.18:

```sh
doxygen Doxyconf     # output in build/doxygen/html/index.html
```
