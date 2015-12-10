/*
 * telebot
 *
 * Copyright (c) 2015 Elmurod Talipov.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <json.h>
#include <json_object.h>
#include <telebot-private.h>
#include <telebot-parser.h>

telebot_user_t *telebot_parser_get_user(char *data)
{
    telebot_user_t *u = (telebot_user_t *) malloc(sizeof(telebot_user_t));
    if (u == NULL) {
        ERR("Failed to allocate memory");
        return NULL;
    }

    json_object *obj = json_tokener_parse(data);
    json_object *item;
    if (json_object_object_get_ex(obj, "id", &item)) {
        u->id = json_object_get_int(item);
    }
    else {
        free(u);
        ERR("Telegram JSON response is not user type, id not found");
        return NULL;
    }

    if (json_object_object_get_ex(obj, "first_name", &item)){
        u->first_name = (char *)json_object_get_string(item);
    }
    else {
        free(u);
        ERR("Telegram JSON response is not user type, first_name not found");
        return NULL;
    }

    if (json_object_object_get_ex(obj, "last_name", &item))
        u->last_name = (char *)json_object_get_string(item);

    if (json_object_object_get_ex(obj, "username", &item))
        u->username = (char *)json_object_get_string(item);

    return u;
}

int telebot_parser_get_update_id(char *data)
{
    return 0;
}

telebot_message_t *telebot_parser_get_message(char *data)
{
    return NULL;
}

telebot_chat_t *telebot_parser_get_chat(char *data)
{
    return NULL;
}


