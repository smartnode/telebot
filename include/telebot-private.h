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
#define TELEBOT_BUFFER_SECTOR                512
#define TELEBOT_BUFFER_BLOCK                 1024
#define TELEBOT_BUFFER_PAGE                  4096
#define TELEBOT_UPDATE_COUNT_MAX_LIMIT       100
#define TELEBOT_USER_PROFILE_PHOTOS_LIMIT    100
#define TELEBOT_SAFE_FREE(addr)              if (addr) { free(addr); addr = NULL; }
#define TELEBOT_SAFE_FZCNT(addr, count)      { TELEBOT_SAFE_FREE(addr); count = 0; }
#define TELEBOT_SAFE_STRDUP(str)             (str) ? strdup(str) : NULL;

#define TELEBOT_METHOD_GET_UPDATES                  "getUpdates"
#define TELEBOT_METHOD_SET_WEBHOOK                  "setWebhook"
#define TELEBOT_METHOD_DELETE_WEBHOOK               "deleteWebhook"
#define TELEBOT_METHOD_GET_WEBHOOK_INFO             "getWebhookInfo"
#define TELEBOT_METHOD_GET_ME                       "getMe"
#define TELEBOT_METHOD_SEND_MESSAGE                 "sendMessage"
#define TELEBOT_METHOD_FORWARD_MESSAGE              "forwardMessage"
#define TELEBOT_METHOD_SEND_PHOTO                   "sendPhoto"
#define TELEBOT_METHOD_SEND_AUDIO                   "sendAudio"
#define TELEBOT_METHOD_SEND_DOCUMENT                "sendDocument"
#define TELEBOT_METHOD_SEND_VIDEO                   "sendVideo"
#define TELEBOT_METHOD_SEND_ANIMATION               "sendAnimation"
#define TELEBOT_METHOD_SEND_VOICE                   "sendVoice"
#define TELEBOT_METHOD_SEND_VIDEO_NOTE              "sendVideoNote"
#define TELEBOT_METHOD_SEND_MEDIA_GROUP             "sendMediaGroup"
#define TELEBOT_METHOD_SEND_LOCATION                "sendLocation"
#define TELEBOT_METHOD_EDIT_MESSAGE_LIVE_LOCATION   "editMessageLiveLocation"
#define TELEBOT_METHOD_STOP_MESSAGE_LIVE_LOCATION   "stopMessageLiveLocation"
#define TELEBOT_METHOD_SEND_VENUE                   "sendVenue"
#define TELEBOT_METHOD_SEND_CONTACT                 "sendContact"
#define TELEBOT_METHOD_SEND_POLL                    "sendPoll"
#define TELEBOT_METHOD_SEND_DICE                    "sendDice"
#define TELEBOT_METHOD_SEND_CHAT_ACTION             "sendChatAction"
#define TELEBOT_METHOD_GET_USER_PHOTOS              "getUserProfilePhotos"
#define TELEBOT_METHOD_GET_FILE                     "getFile"
#define TELEBOT_METHOD_KICK_CHAT_MEMBER             "kickChatMember"
#define TELEBOT_METHOD_UNBAN_CHAT_MEMBER            "unbanChatMember"
#define TELEBOT_METHOD_RESTRICT_CHAT_MEMBER         "restrictChatMember"
#define TELEBOT_METHOD_PROMOTE_CHAT_MEMBER          "promoteChatMember"
#define TELEBOT_METHOD_SET_CHAT_ADMIN_TITLE         "setChatAdministratorCustomTitle"
#define TELEBOT_METHOD_SET_CHAT_PERMISSIONS         "setChatPermissions"
#define TELEBOT_METHOD_EXPORT_CHAT_INVITE_LINK      "exportChatInviteLink"
#define TELEBOT_METHOD_SET_CHAT_PHOTO               "setChatPhoto"
#define TELEBOT_METHOD_DELETE_CHAT_PHOTO            "deleteChatPhoto"
#define TELEBOT_METHOD_SET_CHAT_TITLE               "setChatTitle"
#define TELEBOT_METHOD_SET_CHAT_DESCRIPTION         "setChatDescription"
#define TELEBOT_METHOD_PIN_CHAT_MESSAGE             "pinChatMessage"
#define TELEBOT_METHOD_UNPIN_CHAT_MESSAGE           "unpinChatMessage"
#define TELEBOT_METHOD_LEAVE_CHAT                   "leaveChat"
#define TELEBOT_METHOD_GET_CHAT                     "getChat"
#define TELEBOT_METHOD_GET_CHAT_ADMINS              "getChatAdministrators"
#define TELEBOT_METHOD_GET_CHAT_MEMBERS_COUNT       "getChatMembersCount"
#define TELEBOT_METHOD_GET_CHAT_MEMBER              "getChatMember"
#define TELEBOT_METHOD_SET_CHAT_STICKER_SET         "setChatStickerSet"
#define TELEBOT_METHOD_DEL_CHAT_STICKER_SET         "deleteChatStickerSet"
#define TELEBOT_METHOD_ANSWER_CALLBACK_QUERY        "answerCallbackQuery"
#define TELEBOT_METHOD_SET_MY_COMMANDS              "setMyCommands"
#define TELEBOT_METHOD_GET_MY_COMMANDS              "getMyCommands"
#define TELEBOT_METHOD_EDIT_MESSAGE_TEXT            "editMessageText"
#define TELEBOT_METHOD_EDIT_MESSAGE_CAPTION         "editMessageCaption"
#define TELEBOT_METHOD_EDIT_MESSAGE_MEDIA           "editMessageMedia"
#define TELEBOT_METHOD_EDIT_MESSAGE_REPLY_MARKUP    "editMessageReplyMarkup"
#define TELEBOT_METHOD_STOP_POLL                    "stopPoll"
#define TELEBOT_METHOD_DELETE_MESSAGE               "deleteMessage"

#ifdef DEBUG
    #define ERR(fmt, args...) fprintf(stderr, "[ERROR][%s:%d]" fmt "\n", __func__, __LINE__, ##args)
    #define DBG(fmt, args...) fprintf(stdout, "[DEBUG][%s:%d]" fmt "\n", __func__, __LINE__, ##args)
#else
    #define ERR(x, ...)
    #define DBG(x, ...)
#endif

typedef enum {
    TELEBOT_MIME_TYPE_DATA = 0,
    TELEBOT_MIME_TYPE_FILE,
    TELEBOT_MIME_TYPE_MAX,
} telebot_core_mime_e;
typedef struct {
    telebot_core_mime_e type;
    const char *name;
    char data[TELEBOT_BUFFER_PAGE];
} telebot_core_mime_t;

#endif /* __TELEBOT_PRIVATE_H__ */
