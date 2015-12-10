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

#ifndef __TELEBOT_PARSER_H__
#define __TELEBOT_PARSER_H__

#include <telebot-api.h>
#include <telebot-core-api.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _telebot_updates_t_ {
    int count;
    int update_id[100];
    telebot_message_t message[100];
} telebot_updates_t;

telebot_error_e telebot_parser_get_user(char *data, telebot_user_t *user);
telebot_error_e telebot_parser_get_message(char *data, telebot_message_t *msg);
telebot_error_e telebot_parser_get_chat(char *data, telebot_chat_t *chat);
telebot_error_e telebot_parser_get_updates(char *data, telebot_updates_t *upds);

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_PARSER_H__ */

