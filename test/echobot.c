#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <telebot.h>

#define SIZE_OF_ARRAY(array) (sizeof(array)/sizeof(array[0]))

int main(int argc, char *argv[])
{
    printf("Welcome to Echobot\n");

    FILE *fp = fopen(".token", "r");
    if (fp == NULL) {
        printf("Failed to open token file\n");
        return -1;
    }

    char token[1024];
    if (fscanf(fp, "%s", token) == 0) {
        printf("Reading token failed");
        fclose(fp);
        return -1;
    }
    printf("Token: %s\n", token);
    fclose(fp);

    telebot_handler_t handle;
    if (telebot_create(&handle, token) != TELEBOT_ERROR_NONE) {
        printf("Telebot create failed\n");
        return -1;
    }

    telebot_user_t *me;
    if (telebot_get_me(handle, &me) != TELEBOT_ERROR_NONE) {
        printf("Failed to get bot information\n");
        telebot_destroy(handle);
        return -1;
    }

    printf("ID: %d\n", me->id);
    printf("First Name: %s\n", me->first_name);
    printf("User Name: %s\n", me->username);

    telebot_free_me(me);

    int index, count, offset = 1;
    char str[4096];
    telebot_error_e ret;
    telebot_message_t message;

    while (1) {
        /* busy wait */
        sleep(1);

        telebot_update_t *updates;
        ret = telebot_get_updates(handle, offset, 20, 0, NULL, 0, &updates, &count);
        if (ret != TELEBOT_ERROR_NONE)
            continue;
        printf("Number of updates: %d\n", count);
        for (index = 0;index < count; index++) {
            message = updates[index].message;
            printf("=======================================================\n");
            printf("%s: %s \n", message.from->first_name, message.text);
            if (strstr(message.text, "/start")) {
                snprintf(str, SIZE_OF_ARRAY(str), "Hello %s",
                        message.from->first_name);
            }
            else {
                snprintf(str, SIZE_OF_ARRAY(str), "RE:%s", message.text);
            }
            ret = telebot_send_message(handle, message.chat->id, str, "",
                    false, false, 0, "");
            if (ret != TELEBOT_ERROR_NONE) {
                printf("Failed to send message: %d \n", ret);
            }
            offset = updates[index].update_id + 1;
        }
        telebot_free_updates(updates, count);
    }

    telebot_destroy(handle);

    return 0;
}
