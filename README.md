# Telebot

![Build](https://github.com/smartnode/telebot/workflows/Build/badge.svg)
![GitHub repo size](https://img.shields.io/github/repo-size/smartnode/telebot)
![GitHub issues](https://img.shields.io/github/issues/smartnode/telebot)
![GitHub pull requests](https://img.shields.io/github/issues-pr/smartnode/telebot)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

C Library for Telegram bot API that uses json-c and libcurl.
Head over to [Telegram Bots](https://core.telegram.org/bots) page to read about how to register your brand new bot with Telegram platform and see it in action using telebot.

## Building

You need to install libraries and build tools such as CMake.
On Debian-based Linux distributions you can do it as follows:

```sh
sudo apt-get install libcurl4-openssl-dev libjson-c-dev cmake binutils make
```

On Mac OSX, first install macports from [MacPorts](https://www.macports.org/install.php) and in Terminal

```sh
sudo port install cmake json-c curl
```

To build the library run following commands:

```sh
cd [your repository]
mkdir -p Build && cd Build
cmake ../
make
```

<details>
<summary>Sample</summary>

Following sample creates a simple dummy bot which echoes back the messages sent to it.
The [same example](test/echobot.c) is built as `echobot` executable under `Build/test` folder. The
executable expects and reads bot token from `.token` file on the same location.

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <telebot.h>

#define SIZE_OF_ARRAY(array) (sizeof(array) / sizeof(array[0]))

int main(int argc, char *argv[])
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

    printf("ID: %d\n", me.id);
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
        ret = telebot_get_updates(handle, offset, 20, 0, update_types, 0, &updates, &count);
        if (ret != TELEBOT_ERROR_NONE)
            continue;
        printf("Number of updates: %d\n", count);
        for (index = 0; index < count; index++)
        {
            message = updates[index].message;
            if (message.text)
            {
                printf("%s: %s \n", message.from->first_name, message.text);
                if (strstr(message.text, "/dice"))
                {
                    telebot_send_dice(handle, message.chat->id, false, 0, "");
                }
                else
                {
                    char str[4096];
                    if (strstr(message.text, "/start"))
                    {
                        snprintf(str, SIZE_OF_ARRAY(str), "Hello %s", message.from->first_name);
                    }
                    else
                    {
                        snprintf(str, SIZE_OF_ARRAY(str), "<i>%s</i>", message.text);
                    }
                    ret = telebot_send_message(handle, message.chat->id, str, "HTML", false, false, updates[index].message.message_id, "");
                }
                if (ret != TELEBOT_ERROR_NONE)
                {
                    printf("Failed to send message: %d \n", ret);
                }
            }
            offset = updates[index].update_id + 1;
        }
        telebot_put_updates(updates, count);

        sleep(1);
    }

    telebot_destroy(handle);

    return 0;
}
```

</details>
