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

telebot_error_e telebot_parser_get_user(char *data, telebot_user_t *user)
{
    if (user == NULL) {
        return TELEBOT_ERROR_INVALID_PARAMETER;
    }

    json_object *obj = json_tokener_parse(data);
    json_object *item;
    if (json_object_object_get_ex(obj, "id", &item)) {
        user->id = json_object_get_int(item);
    }
    else {
        ERR("Telegram JSON response is not user type, id not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    if (json_object_object_get_ex(obj, "first_name", &item)){
        user->first_name = (char *)json_object_get_string(item);
    }
    else {
        ERR("Telegram JSON response is not user type, first_name not found");
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    if (json_object_object_get_ex(obj, "last_name", &item))
        user->last_name = (char *)json_object_get_string(item);

    if (json_object_object_get_ex(obj, "username", &item))
        user->username = (char *)json_object_get_string(item);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_updates(char *data, telebot_updates_t *ups)
{
    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_message(char *data, telebot_message_t *msg)
{
    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_parser_get_chat(char *data, telebot_chat_t *chat)
{
    return TELEBOT_ERROR_NONE;
}


