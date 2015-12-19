#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <telebot-common.h>
#include <telebot-api.h>

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

    free(me);
    telebot_destroy();

    return 0;
}
