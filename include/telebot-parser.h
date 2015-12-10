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

#ifdef __cplusplus
extern "C" {
#endif

telebot_user_t *telebot_parser_get_user(char *data);
int telebot_parser_get_update_id(char *data);
telebot_message_t *telebot_parser_get_message(char *data);
telebot_chat_t *telebot_parser_get_chat(char *data);

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_PARSER_H__ */

