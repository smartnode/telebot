#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <telebot.h>

#define SIZE_OF_ARRAY(array) (sizeof(array) / sizeof(array[0]))

/* Local sample media paths */
#define SAMPLE_PHOTO     "test/samples/logo.png"
#define SAMPLE_AUDIO     "test/samples/audio.mp3"
#define SAMPLE_VIDEO     "test/samples/video.mp4"
#define SAMPLE_DOC       "test/samples/document.md"
#define SAMPLE_ANIMATION "test/samples/animation.gif"
#define SAMPLE_VOICE     "test/samples/voice.wav"

void setup_commands(telebot_handler_t handle)
{
    telebot_bot_command_t commands[] = {
        {"start", "Start the bot and show help"},
        {"media", "Test sending various media types"},
        {"keyboard", "Test custom reply keyboard"},
        {"poll", "Send a regular poll"},
        {"quiz", "Send a quiz poll"},
        {"dice", "Send a random dice"},
        {"location", "Send a static location"},
        {"venue", "Send a venue"},
        {"contact", "Send a contact"},
        {"description", "Set chat description"},
        {"info", "Get chat information"}};

    telebot_error_e ret = telebot_set_my_commands(handle, commands, SIZE_OF_ARRAY(commands));
    if (ret != TELEBOT_ERROR_NONE)
    {
        printf("Failed to set my commands: %d\n", ret);
    }
    else
    {
        printf("Bot commands registered successfully\n");
    }
}

void handle_message(telebot_handler_t handle, telebot_message_t *message)
{
    if (message->text == NULL)
        return;

    printf("Message from %s: %s\n", message->from->first_name, message->text);

    telebot_error_e ret = TELEBOT_ERROR_NONE;

    if (strstr(message->text, "/start"))
    {
        char welcome[1024];
        snprintf(welcome, sizeof(welcome),
                 "Welcome %s!\n"
                 "I am a test bot for telebot library.\n\n"
                 "Use the menu or type / to see available commands.",
                 message->from->first_name);
        ret = telebot_send_message(handle, message->chat->id, welcome, "", false, false, 0, "");
    }
    else if (strstr(message->text, "/media"))
    {
        const char *keyboard = "{\"inline_keyboard\":[["
                               "{\"text\":\"Photo\",\"callback_data\":\"media_photo\"},"
                               "{\"text\":\"Audio\",\"callback_data\":\"media_audio\"}],"
                               "[{\"text\":\"Video\",\"callback_data\":\"media_video\"},"
                               "{\"text\":\"Document\",\"callback_data\":\"media_doc\"}],"
                               "[{\"text\":\"Animation\",\"callback_data\":\"media_animation\"},"
                               "{\"text\":\"Voice\",\"callback_data\":\"media_voice\"}]]}";
        ret = telebot_send_message(handle, message->chat->id, "Select media to send:", "", false, false, 0, keyboard);
    }
    else if (strstr(message->text, "/keyboard"))
    {
        const char *keyboard = "{\"keyboard\":[[{\"text\":\"Button 1\"},{\"text\":\"Button 2\"}],[{\"text\":\"Button 3\"}]],\"resize_keyboard\":true,\"one_time_keyboard\":true}";
        ret = telebot_send_message(handle, message->chat->id, "Testing reply keyboard:", "", false, false, 0, keyboard);
    }
    else if (strstr(message->text, "/poll"))
    {
        const char *options[] = {"Option 1", "Option 2", "Option 3"};
        ret = telebot_send_poll(handle, message->chat->id, "Test Poll", options, 3, false, "regular", false, 0, false, false, 0, "");
    }
    else if (strstr(message->text, "/quiz"))
    {
        const char *options[] = {"A", "B", "C", "D"};
        ret = telebot_send_poll(handle, message->chat->id, "Which one is correct?", options, 4, false, "quiz", false, 1, false, false, 0, "");
    }
    else if (strstr(message->text, "/dice"))
    {
        ret = telebot_send_dice(handle, message->chat->id, false, 0, "");
    }
    else if (strstr(message->text, "/location"))
    {
        ret = telebot_send_location(handle, message->chat->id, 41.311081f, 69.240562f, 0, false, 0, "");
    }
    else if (strstr(message->text, "/venue"))
    {
        ret = telebot_send_venue(handle, message->chat->id, 41.311081f, 69.240562f, "Tashkent", "Uzbekistan", "", "", false, 0, "");
    }
    else if (strstr(message->text, "/contact"))
    {
        ret = telebot_send_contact(handle, message->chat->id, "+998901234567", "John", "Doe", "", false, 0, "");
    }
    else if (strstr(message->text, "/description"))
    {
        char desc[256];
        snprintf(desc, sizeof(desc), "Test description set at %ld", (long)time(NULL));
        ret = telebot_set_chat_description(handle, message->chat->id, desc);
        if (ret == TELEBOT_ERROR_NONE)
            telebot_send_message(handle, message->chat->id, "Chat description updated!", "", false, false, 0, "");
    }
    else if (strstr(message->text, "/info"))
    {
        telebot_chat_t chat;
        ret = telebot_get_chat(handle, message->chat->id, &chat);
        if (ret == TELEBOT_ERROR_NONE)
        {
            char info[1024];
            snprintf(info, sizeof(info), "Chat Info:\nID: %lld\nType: %s\nTitle: %s\nDescription: %s",
                     chat.id, chat.type ? chat.type : "N/A",
                     chat.title ? chat.title : "N/A",
                     chat.description ? chat.description : "N/A");
            telebot_send_message(handle, message->chat->id, info, "", false, false, 0, "");
            telebot_put_chat(&chat);
        }
    }
    else
    {
        char echo[4096];
        snprintf(echo, sizeof(echo), "Echo: %s", message->text);
        ret = telebot_send_message(handle, message->chat->id, echo, "", false, false, 0, "");
    }

    if (ret != TELEBOT_ERROR_NONE)
    {
        printf("Error: %d\n", ret);
    }
}

void handle_callback_query(telebot_handler_t handle, telebot_callback_query_t *query)
{
    if (query->data == NULL)
        return;

    printf("Callback query from %s: %s\n", query->from->first_name, query->data);

    telebot_error_e ret = TELEBOT_ERROR_NONE;
    long long int chat_id = query->message->chat->id;

    int duration = 0;
    int width = 0;
    int height = 0;
    const char *thumb = NULL;
    bool disable_notification = false;
    int reply_to_message_id = 0;
    const char *reply_markup = NULL;
    const char *parse_mode = "";
    bool is_file = true;

    if (strcmp(query->data, "media_photo") == 0)
    {
        ret = telebot_send_photo(handle, chat_id, SAMPLE_PHOTO, is_file, "Test Photo", parse_mode, disable_notification, reply_to_message_id, reply_markup);
    }
    else if (strcmp(query->data, "media_audio") == 0)
    {
        ret = telebot_send_audio(handle, chat_id, SAMPLE_AUDIO, is_file, "Test Audio", parse_mode, duration, "Artist", "Title", parse_mode, disable_notification, reply_to_message_id, reply_markup);
    }
    else if (strcmp(query->data, "media_video") == 0)
    {
        ret = telebot_send_video(handle, chat_id, SAMPLE_VIDEO, is_file, duration, width, height, thumb, "Test Video", parse_mode, disable_notification, false, reply_to_message_id, reply_markup);
    }
    else if (strcmp(query->data, "media_doc") == 0)
    {
        ret = telebot_send_document(handle, chat_id, SAMPLE_DOC, is_file, thumb, "Test Document", parse_mode, disable_notification, reply_to_message_id, reply_markup);
    }
    else if (strcmp(query->data, "media_animation") == 0)
    {
        ret = telebot_send_animation(handle, chat_id, SAMPLE_ANIMATION, is_file, duration, width, height, thumb, "Test Animation", parse_mode, disable_notification, reply_to_message_id, reply_markup);
    }
    else if (strcmp(query->data, "media_voice") == 0)
    {
        ret = telebot_send_voice(handle, chat_id, SAMPLE_VOICE, is_file, "Test Voice", parse_mode, duration, disable_notification, reply_to_message_id, reply_markup);
    }

    /* Acknowledge callback query */
    telebot_answer_callback_query(handle, query->id, "Sending media...", false, "", 0);

    if (ret != TELEBOT_ERROR_NONE)
    {
        printf("Error in callback: %d\n", ret);
        char err[64];
        snprintf(err, sizeof(err), "Error sending media: %d", ret);
        telebot_send_message(handle, chat_id, err, "", false, false, 0, "");
    }
}

int main(int argc, char *argv[])
{
    printf("Welcome to Testbot\n");

    FILE *fp = fopen(".token", "r");
    if (fp == NULL)
    {
        printf("Failed to open .token file. Please create it and put your bot token there.\n");
        return -1;
    }

    char token[1024];
    if (fscanf(fp, "%s", token) == 0)
    {
        printf("Failed to read token\n");
        fclose(fp);
        return -1;
    }
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

    printf("Bot ID: %lld\n", me.id);
    printf("Bot Name: %s\n", me.first_name);
    printf("Bot Username: %s\n", me.username);
    telebot_put_me(&me);

    /* Setup bot commands in Telegram menu */
    setup_commands(handle);

    int index, count, offset = -1;
    telebot_error_e ret;
    telebot_update_type_e update_types[] = {TELEBOT_UPDATE_TYPE_MESSAGE, TELEBOT_UPDATE_TYPE_CALLBACK_QUERY};

    while (1)
    {
        telebot_update_t *updates;
        ret = telebot_get_updates(handle, offset, 20, 0, update_types, 2, &updates, &count);
        if (ret != TELEBOT_ERROR_NONE)
        {
            sleep(1);
            continue;
        }

        for (index = 0; index < count; index++)
        {
            if (updates[index].update_type == TELEBOT_UPDATE_TYPE_MESSAGE)
            {
                handle_message(handle, &(updates[index].message));
            }
            else if (updates[index].update_type == TELEBOT_UPDATE_TYPE_CALLBACK_QUERY)
            {
                handle_callback_query(handle, &(updates[index].callback_query));
            }
            offset = updates[index].update_id + 1;
        }
        telebot_put_updates(updates, count);
        sleep(1);
    }

    telebot_destroy(handle);

    return 0;
}
