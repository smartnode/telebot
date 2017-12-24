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

#ifndef __TELEBOT_PRIVATE_H__
#define __TELEBOT_PRIVATE_H__

#define TELEBOT_API_URL                      "https://api.telegram.org"
#define TELEBOT_URL_SIZE                     1024
#define TELEBOT_UPDATE_POLLING_INTERVAL      1000000 // 1 second
#define TELEBOT_UPDATE_COUNT_MAX_LIMIT       100
#define TELEBOT_UPDATE_COUNT_PER_REQUEST     10

#define TELEBOT_METHOD_GET_ME                "getMe"
#define TELEBOT_METHOD_GET_UPDATES           "getUpdates"
#define TELEBOT_METHOD_SEND_MESSAGE          "sendMessage"
#define TELEBOT_METHOD_DELETE_MESSAGE        "deleteMessage"
#define TELEBOT_METHOD_ANSWER_CALLBACK_QUERY "answerCallbackQuery"
#define TELEBOT_METHOD_FORWARD_MESSAGE       "forwardMessage"
#define TELEBOT_METHOD_SEND_PHOTO            "sendPhoto"
#define TELEBOT_METHOD_SEND_AUDIO            "sendAudio"
#define TELEBOT_METHOD_SEND_DOCUMENT         "sendDocument"
#define TELEBOT_METHOD_SEND_VIDEO            "sendVideo"
#define TELEBOT_METHOD_SEND_STICKER          "sendSticker"
#define TELEBOT_METHOD_SEND_VOICE            "sendVoice"
#define TELEBOT_METHOD_SEND_LOCATION         "sendLocation"
#define TELEBOT_METHOD_SEND_CHATACTION       "sendChatAction"
#define TELEBOT_METHOD_GET_USERPHOTOS        "getUserProfilePhotos"
#define TELEBOT_METHOD_GET_FILE              "getFile"
#define TELEBOT_METHOD_SET_WEBHOOK           "setWebhook"


#ifdef DEBUG
    #define ERR(fmt, args...) printf("[ERROR][%s:%d]" fmt "\n", __func__, __LINE__, ##args)
    #define DBG(fmt, args...) printf("[DEBUG][%s:%d]" fmt "\n", __func__, __LINE__, ##args)
#else
    #define ERR(x, ...)
    #define DBG(x, ...)
#endif

#endif /* __TELEBOT_PRIVATE_H__ */
