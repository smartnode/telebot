#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <telebot.h>

#define SIZE_OF_ARRAY(array) (sizeof(array)/sizeof(array[0]))

telebot_handler_t g_handler;

static void update_cb(const telebot_update_t *update)
{
    char str[4096];
    telebot_error_e ret = TELEBOT_ERROR_NONE;
    telebot_message_t message = update->message;
    printf("%s says :\n\r %s\n\r", message.from->first_name, message.text);
    if (strstr(message.text, "/start")) {
        snprintf(str, SIZE_OF_ARRAY(str), "Hello %s",
                message.from->first_name);
    }
    else {
        snprintf(str, SIZE_OF_ARRAY(str), "RE:%s", message.text);
    }
    ret = telebot_send_message(g_handler, message.chat->id, str, "",
                false, false, 0, "");
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

    if (telebot_create(&g_handler, token) != TELEBOT_ERROR_NONE) {
        printf("Telebot create failed\n");
        return -1;
    }

    telebot_user_t me;
    if (telebot_get_me(g_handler, &me) != TELEBOT_ERROR_NONE) {
        printf("Failed to get bot information\n");
        telebot_destroy(g_handler);
        return -1;
    }

    printf("ID: %d\n", me.id);
    printf("First Name: %s\n", me.first_name);
    printf("User Name: %s\n", me.username);

    pthread_t thread;
    telebot_start(g_handler, update_cb, 2, false, &thread);
    printf("t: %ld\n", thread);
    // wait for callbacks;
    pthread_join(thread, NULL);

    telebot_stop(g_handler);
    telebot_destroy(g_handler);

    return 0;
}
