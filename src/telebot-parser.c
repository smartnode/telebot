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
#include <telebot-parser.h>

telebot_user_t *telebot_parser_get_user(char *data)
{
    telebot_user_t *res = (telebot_user_t *) malloc(sizeof(telebot_user_t));
    if (res == NULL) {
        ERROR("Failed to allocate memory");
        return NULL;
    }

    return res;
}

telebot_chat_t *telebot_parser_get_chat(char *data)
{
    return NULL;
}


