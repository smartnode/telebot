#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <telebot-common.h>
#include <telebot-api.h>

static void update_cb(const telebot_message_t *message)
{
    telebot_error_e ret;
    // TELEBOT_MESSAGE_TEXT_SIZE + Re:<space>
    char str[TELEBOT_MESSAGE_TEXT_SIZE + 4];
    char chat_id[32];
    if (strstr(message->text, "/start")) {
        snprintf(str, TELEBOT_MESSAGE_TEXT_SIZE, "Hello %s",
                message->from.first_name);
    }
    else {
        snprintf(str, TELEBOT_MESSAGE_TEXT_SIZE, "RE: %s", message->text);
    }
    snprintf(chat_id, 32, "%d", message->chat.id);

    ret = telebot_send_message(chat_id, str, "", false, 0, "");
    if (ret != TELEBOT_ERROR_NONE) {
        printf("Failed to send message: %d \n", ret);
    }
}

int main(int argc, char *argv[])
{
    printf ("Telebot test code\n");

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
    printf ("Token: %s\n", token);
    fclose(fp);

    if (telebot_create(token) != TELEBOT_ERROR_NONE) {
        printf("Telebot create failed\n");
        return -1;
    }

    telebot_user_t *me;
    if (telebot_get_me(&me) != TELEBOT_ERROR_NONE) {
        printf("Failed to get bot information\n");
        telebot_destroy();
        return -1;
    }

    printf("ID: %d\n", me->id);
    printf("First Name: %s\n", me->first_name);
    printf("Last Name: %s\n", me->last_name);
    printf("User Name: %s\n", me->username);
    free(me);

    telebot_start(update_cb);

    // wait for callbacks;
    while (1);

    telebot_stop();
    telebot_destroy();

    return 0;
}
