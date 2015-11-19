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
#include <telebot-core-api.h>
#include <telebot-core-types.h>


telebot_user_t *telebot_core_get_user(telebot_resp_t *resp)
{
    telebot_user_t *res = (telebot_user_t *) malloc(sizeof(telebot_user_t));
    if (res == NULL) return NULL;

    return NULL;
}

telebot_chat_t *telebto_core_get_chat(telebot_resp_t *resp)
{

    return NULL;
}


