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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "telebot-common.h"
#include "telebot-types.h"
#include "telebot-core.h"

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
#define TELEBOT_METHOD_LOG_OUT                      "logOut"
#define TELEBOT_METHOD_CLOSE                        "close"
#define TELEBOT_METHOD_SET_MY_NAME                  "setMyName"
#define TELEBOT_METHOD_GET_MY_NAME                  "getMyName"
#define TELEBOT_METHOD_SET_MY_DESCRIPTION           "setMyDescription"
#define TELEBOT_METHOD_GET_MY_DESCRIPTION           "getMyDescription"
#define TELEBOT_METHOD_SET_MY_SHORT_DESCRIPTION     "setMyShortDescription"
#define TELEBOT_METHOD_GET_MY_SHORT_DESCRIPTION     "getMyShortDescription"
#define TELEBOT_METHOD_SET_MY_PROFILE_PHOTO         "setMyProfilePhoto"
#define TELEBOT_METHOD_REMOVE_MY_PROFILE_PHOTO      "removeMyProfilePhoto"
#define TELEBOT_METHOD_SET_CHAT_MENU_BUTTON         "setChatMenuButton"
#define TELEBOT_METHOD_GET_CHAT_MENU_BUTTON         "getChatMenuButton"
#define TELEBOT_METHOD_SET_MY_DEFAULT_ADMIN_RIGHTS  "setMyDefaultAdministratorRights"
#define TELEBOT_METHOD_GET_MY_DEFAULT_ADMIN_RIGHTS  "getMyDefaultAdministratorRights"
#define TELEBOT_METHOD_SEND_MESSAGE                 "sendMessage"
#define TELEBOT_METHOD_FORWARD_MESSAGE              "forwardMessage"
#define TELEBOT_METHOD_FORWARD_MESSAGES             "forwardMessages"
#define TELEBOT_METHOD_COPY_MESSAGE                 "copyMessage"
#define TELEBOT_METHOD_COPY_MESSAGES                "copyMessages"
#define TELEBOT_METHOD_SEND_PHOTO                   "sendPhoto"
#define TELEBOT_METHOD_SEND_AUDIO                   "sendAudio"
#define TELEBOT_METHOD_SEND_DOCUMENT                "sendDocument"
#define TELEBOT_METHOD_SEND_VIDEO                   "sendVideo"
#define TELEBOT_METHOD_SEND_ANIMATION               "sendAnimation"
#define TELEBOT_METHOD_SEND_VOICE                   "sendVoice"
#define TELEBOT_METHOD_SEND_VIDEO_NOTE              "sendVideoNote"
#define TELEBOT_METHOD_SEND_PAID_MEDIA              "sendPaidMedia"
#define TELEBOT_METHOD_SEND_MEDIA_GROUP             "sendMediaGroup"
#define TELEBOT_METHOD_SEND_LOCATION                "sendLocation"
#define TELEBOT_METHOD_EDIT_MESSAGE_LIVE_LOCATION   "editMessageLiveLocation"
#define TELEBOT_METHOD_STOP_MESSAGE_LIVE_LOCATION   "stopMessageLiveLocation"
#define TELEBOT_METHOD_SEND_VENUE                   "sendVenue"
#define TELEBOT_METHOD_SEND_CONTACT                 "sendContact"
#define TELEBOT_METHOD_SEND_POLL                    "sendPoll"
#define TELEBOT_METHOD_SEND_CHECKLIST               "sendChecklist"
#define TELEBOT_METHOD_SEND_DICE                    "sendDice"
#define TELEBOT_METHOD_SEND_CHAT_ACTION             "sendChatAction"
#define TELEBOT_METHOD_SEND_MESSAGE_DRAFT           "sendMessageDraft"
#define TELEBOT_METHOD_GET_USER_PHOTOS              "getUserProfilePhotos"
#define TELEBOT_METHOD_GET_USER_AUDIOS              "getUserProfileAudios"
#define TELEBOT_METHOD_SET_USER_EMOJI_STATUS        "setUserEmojiStatus"
#define TELEBOT_METHOD_GET_FILE                     "getFile"
#define TELEBOT_METHOD_BAN_CHAT_MEMBER              "banChatMember"
#define TELEBOT_METHOD_KICK_CHAT_MEMBER             "kickChatMember"
#define TELEBOT_METHOD_UNBAN_CHAT_MEMBER            "unbanChatMember"
#define TELEBOT_METHOD_RESTRICT_CHAT_MEMBER         "restrictChatMember"
#define TELEBOT_METHOD_PROMOTE_CHAT_MEMBER          "promoteChatMember"
#define TELEBOT_METHOD_SET_CHAT_ADMIN_TITLE         "setChatAdministratorCustomTitle"
#define TELEBOT_METHOD_BAN_CHAT_SENDER_CHAT         "banChatSenderChat"
#define TELEBOT_METHOD_UNBAN_CHAT_SENDER_CHAT       "unbanChatSenderChat"
#define TELEBOT_METHOD_SET_CHAT_PERMISSIONS         "setChatPermissions"
#define TELEBOT_METHOD_EXPORT_CHAT_INVITE_LINK      "exportChatInviteLink"
#define TELEBOT_METHOD_CREATE_CHAT_INVITE_LINK      "createChatInviteLink"
#define TELEBOT_METHOD_EDIT_CHAT_INVITE_LINK        "editChatInviteLink"
#define TELEBOT_METHOD_CREATE_CHAT_SUBSCRIPTION_INVITE_LINK "createChatSubscriptionInviteLink"
#define TELEBOT_METHOD_EDIT_CHAT_SUBSCRIPTION_INVITE_LINK   "editChatSubscriptionInviteLink"
#define TELEBOT_METHOD_REVOKE_CHAT_INVITE_LINK      "revokeChatInviteLink"
#define TELEBOT_METHOD_APPROVE_CHAT_JOIN_REQUEST    "approveChatJoinRequest"
#define TELEBOT_METHOD_DECLINE_CHAT_JOIN_REQUEST    "declineChatJoinRequest"
#define TELEBOT_METHOD_SET_CHAT_PHOTO               "setChatPhoto"
#define TELEBOT_METHOD_DELETE_CHAT_PHOTO            "deleteChatPhoto"
#define TELEBOT_METHOD_SET_CHAT_TITLE               "setChatTitle"
#define TELEBOT_METHOD_SET_CHAT_DESCRIPTION         "setChatDescription"
#define TELEBOT_METHOD_PIN_CHAT_MESSAGE             "pinChatMessage"
#define TELEBOT_METHOD_UNPIN_CHAT_MESSAGE           "unpinChatMessage"
#define TELEBOT_METHOD_UNPIN_ALL_CHAT_MESSAGES      "unpinAllChatMessages"
#define TELEBOT_METHOD_LEAVE_CHAT                   "leaveChat"
#define TELEBOT_METHOD_GET_CHAT                     "getChat"
#define TELEBOT_METHOD_GET_CHAT_ADMINS              "getChatAdministrators"
#define TELEBOT_METHOD_GET_CHAT_MEMBERS_COUNT       "getChatMembersCount"
#define TELEBOT_METHOD_GET_CHAT_MEMBER              "getChatMember"
#define TELEBOT_METHOD_SET_CHAT_STICKER_SET         "setChatStickerSet"
#define TELEBOT_METHOD_DEL_CHAT_STICKER_SET         "deleteChatStickerSet"
#define TELEBOT_METHOD_GET_FORUM_TOPIC_ICON_STICKERS "getForumTopicIconStickers"
#define TELEBOT_METHOD_CREATE_FORUM_TOPIC           "createForumTopic"
#define TELEBOT_METHOD_EDIT_FORUM_TOPIC             "editForumTopic"
#define TELEBOT_METHOD_CLOSE_FORUM_TOPIC            "closeForumTopic"
#define TELEBOT_METHOD_REOPEN_FORUM_TOPIC           "reopenForumTopic"
#define TELEBOT_METHOD_DELETE_FORUM_TOPIC           "deleteForumTopic"
#define TELEBOT_METHOD_UNPIN_ALL_FORUM_TOPIC_MESSAGES "unpinAllForumTopicMessages"
#define TELEBOT_METHOD_EDIT_GENERAL_FORUM_TOPIC     "editGeneralForumTopic"
#define TELEBOT_METHOD_CLOSE_GENERAL_FORUM_TOPIC    "closeGeneralForumTopic"
#define TELEBOT_METHOD_REOPEN_GENERAL_FORUM_TOPIC   "reopenGeneralForumTopic"
#define TELEBOT_METHOD_HIDE_GENERAL_FORUM_TOPIC     "hideGeneralForumTopic"
#define TELEBOT_METHOD_UNHIDE_GENERAL_FORUM_TOPIC   "unhideGeneralForumTopic"
#define TELEBOT_METHOD_UNPIN_ALL_GENERAL_FORUM_TOPIC_MESSAGES "unpinAllGeneralForumTopicMessages"
#define TELEBOT_METHOD_ANSWER_CALLBACK_QUERY        "answerCallbackQuery"
#define TELEBOT_METHOD_GET_USER_CHAT_BOOSTS         "getUserChatBoosts"
#define TELEBOT_METHOD_GET_BUSINESS_CONNECTION      "getBusinessConnection"
#define TELEBOT_METHOD_SET_MY_COMMANDS              "setMyCommands"
#define TELEBOT_METHOD_DELETE_MY_COMMANDS           "deleteMyCommands"
#define TELEBOT_METHOD_GET_MY_COMMANDS              "getMyCommands"
#define TELEBOT_METHOD_SET_MESSAGE_REACTION         "setMessageReaction"
#define TELEBOT_METHOD_EDIT_MESSAGE_TEXT            "editMessageText"
#define TELEBOT_METHOD_EDIT_MESSAGE_CAPTION         "editMessageCaption"
#define TELEBOT_METHOD_EDIT_MESSAGE_MEDIA           "editMessageMedia"
#define TELEBOT_METHOD_EDIT_MESSAGE_REPLY_MARKUP    "editMessageReplyMarkup"
#define TELEBOT_METHOD_EDIT_MESSAGE_CHECKLIST       "editMessageChecklist"
#define TELEBOT_METHOD_STOP_POLL                    "stopPoll"
#define TELEBOT_METHOD_APPROVE_SUGGESTED_POST       "approveSuggestedPost"
#define TELEBOT_METHOD_DECLINE_SUGGESTED_POST       "declineSuggestedPost"
#define TELEBOT_METHOD_DELETE_MESSAGE               "deleteMessage"
#define TELEBOT_METHOD_DELETE_MESSAGES              "deleteMessages"
#define TELEBOT_METHOD_GET_STICKER_SET              "getStickerSet"
#define TELEBOT_METHOD_GET_CUSTOM_EMOJI_STICKERS    "getCustomEmojiStickers"
#define TELEBOT_METHOD_UPLOAD_STICKER_FILE          "uploadStickerFile"
#define TELEBOT_METHOD_CREATE_NEW_STICKER_SET       "createNewStickerSet"
#define TELEBOT_METHOD_ADD_STICKER_TO_SET           "addStickerToSet"
#define TELEBOT_METHOD_SET_STICKER_POSITION_IN_SET  "setStickerPositionInSet"
#define TELEBOT_METHOD_DELETE_STICKER_FROM_SET      "deleteStickerFromSet"
#define TELEBOT_METHOD_REPLACE_STICKER_IN_SET       "replaceStickerInSet"
#define TELEBOT_METHOD_SET_STICKER_EMOJI_LIST       "setStickerEmojiList"
#define TELEBOT_METHOD_SET_STICKER_KEYWORDS         "setStickerKeywords"
#define TELEBOT_METHOD_SET_STICKER_MASK_POSITION    "setStickerMaskPosition"
#define TELEBOT_METHOD_SET_STICKER_SET_TITLE        "setStickerSetTitle"
#define TELEBOT_METHOD_SET_STICKER_SET_THUMBNAIL    "setStickerSetThumbnail"
#define TELEBOT_METHOD_SET_CUSTOM_EMOJI_STICKER_SET_THUMBNAIL "setCustomEmojiStickerSetThumbnail"
#define TELEBOT_METHOD_DELETE_STICKER_SET           "deleteStickerSet"
#define TELEBOT_METHOD_ANSWER_INLINE_QUERY          "answerInlineQuery"
#define TELEBOT_METHOD_ANSWER_WEB_APP_QUERY         "answerWebAppQuery"
#define TELEBOT_METHOD_SAVE_PREPARED_INLINE_MESSAGE "savePreparedInlineMessage"
#define TELEBOT_METHOD_SEND_INVOICE                 "sendInvoice"
#define TELEBOT_METHOD_CREATE_INVOICE_LINK          "createInvoiceLink"
#define TELEBOT_METHOD_ANSWER_SHIPPING_QUERY        "answerShippingQuery"
#define TELEBOT_METHOD_ANSWER_PRE_CHECKOUT_QUERY    "answerPreCheckoutQuery"
#define TELEBOT_METHOD_GET_MY_STAR_BALANCE          "getMyStarBalance"
#define TELEBOT_METHOD_GET_STAR_TRANSACTIONS        "getStarTransactions"
#define TELEBOT_METHOD_REFUND_STAR_PAYMENT          "refundStarPayment"
#define TELEBOT_METHOD_EDIT_USER_STAR_SUBSCRIPTION  "editUserStarSubscription"
#define TELEBOT_METHOD_SET_PASSPORT_DATA_ERRORS     "setPassportDataErrors"
#define TELEBOT_METHOD_SEND_GAME                    "sendGame"
#define TELEBOT_METHOD_SET_GAME_SCORE               "setGameScore"
#define TELEBOT_METHOD_GET_GAME_HIGH_SCORES         "getGameHighScores"
#define TELEBOT_METHOD_GET_AVAILABLE_GIFTS          "getAvailableGifts"
#define TELEBOT_METHOD_SEND_GIFT                    "sendGift"
#define TELEBOT_METHOD_GIFT_PREMIUM_SUBSCRIPTION    "giftPremiumSubscription"
#define TELEBOT_METHOD_VERIFY_USER                  "verifyUser"
#define TELEBOT_METHOD_VERIFY_CHAT                  "verifyChat"
#define TELEBOT_METHOD_REMOVE_USER_VERIFICATION     "removeUserVerification"
#define TELEBOT_METHOD_REMOVE_CHAT_VERIFICATION     "removeChatVerification"
#define TELEBOT_METHOD_READ_BUSINESS_MESSAGE        "readBusinessMessage"
#define TELEBOT_METHOD_DELETE_BUSINESS_MESSAGES     "deleteBusinessMessages"
#define TELEBOT_METHOD_SET_BUSINESS_ACCOUNT_NAME    "setBusinessAccountName"
#define TELEBOT_METHOD_SET_BUSINESS_ACCOUNT_USERNAME "setBusinessAccountUsername"
#define TELEBOT_METHOD_SET_BUSINESS_ACCOUNT_BIO      "setBusinessAccountBio"
#define TELEBOT_METHOD_SET_BUSINESS_ACCOUNT_PROFILE_PHOTO "setBusinessAccountProfilePhoto"
#define TELEBOT_METHOD_REMOVE_BUSINESS_ACCOUNT_PROFILE_PHOTO "removeBusinessAccountProfilePhoto"
#define TELEBOT_METHOD_SET_BUSINESS_ACCOUNT_GIFT_SETTINGS "setBusinessAccountGiftSettings"
#define TELEBOT_METHOD_GET_BUSINESS_ACCOUNT_STAR_BALANCE "getBusinessAccountStarBalance"
#define TELEBOT_METHOD_TRANSFER_BUSINESS_ACCOUNT_STARS "transferBusinessAccountStars"
#define TELEBOT_METHOD_GET_BUSINESS_ACCOUNT_GIFTS   "getBusinessAccountGifts"
#define TELEBOT_METHOD_GET_USER_GIFTS               "getUserGifts"
#define TELEBOT_METHOD_GET_CHAT_GIFTS               "getChatGifts"
#define TELEBOT_METHOD_CONVERT_GIFT_TO_STARS        "convertGiftToStars"
#define TELEBOT_METHOD_UPGRADE_GIFT                 "upgradeGift"
#define TELEBOT_METHOD_TRANSFER_GIFT                "transferGift"
#define TELEBOT_METHOD_POST_STORY                   "postStory"
#define TELEBOT_METHOD_REPOST_STORY                 "repostStory"
#define TELEBOT_METHOD_EDIT_STORY                   "editStory"
#define TELEBOT_METHOD_DELETE_STORY                 "deleteStory"

#ifdef DEBUG
#define ERR(fmt, args...) fprintf(stderr, "[ERROR][%s:%d]" fmt "\n", __func__, __LINE__, ##args)
#define DBG(fmt, args...) fprintf(stdout, "[DEBUG][%s:%d]" fmt "\n", __func__, __LINE__, ##args)
#else
#define ERR(x, ...)
#define DBG(x, ...)
#endif

#define CHECK_ARG_NULL(PARAM)                                                    \
    if (PARAM == NULL)                                                           \
    {                                                                            \
        ERR("Argument '%s' is null)", #PARAM);                                   \
        return telebot_core_get_error_response(TELEBOT_ERROR_INVALID_PARAMETER); \
    }

#define CHECK_ARG_CONDITION(CONDITION, MESSAGE)                                  \
    if (CONDITION)                                                               \
    {                                                                            \
        ERR(MESSAGE);                                                            \
        return telebot_core_get_error_response(TELEBOT_ERROR_INVALID_PARAMETER); \
    }

typedef enum
{
    TELEBOT_MIME_TYPE_CHAR,
    TELEBOT_MIME_TYPE_INT,
    TELEBOT_MIME_TYPE_U_INT,
    TELEBOT_MIME_TYPE_LONG_INT,
    TELEBOT_MIME_TYPE_U_LONG_INT,
    TELEBOT_MIME_TYPE_LONG_LONG_INT,
    TELEBOT_MIME_TYPE_U_LONG_LONG_INT,
    TELEBOT_MIME_TYPE_FLOAT,
    TELEBOT_MIME_TYPE_DOUBLE,
    TELEBOT_MIME_TYPE_LONG_DOUBLE,
    TELEBOT_MIME_TYPE_STRING,
    TELEBOT_MIME_TYPE_FILE,
    TELEBOT_MIME_TYPE_MAX,
} telebot_core_mime_e;

typedef struct telebot_core_mime_s
{
    telebot_core_mime_e type;
    const char *name;
    union
    {
        char c;
        int d;
        unsigned int u;
        long int ld;
        unsigned long int lu;
        long long lld;
        unsigned long long llu;
        float f;
        double lf;
        long double llf;
        const char *s;
    } data;

} telebot_core_mime_t;

/**
 * @brief This object represents core handler.
 */
struct telebot_core_handler
{
    char *token;      /**< Telegam bot token */
    char *proxy_addr; /**< Proxy address (optional) */
    char *proxy_auth; /**< Proxy authentication (optional) */
};

/**
 * @brief This object represents handler.
 */
struct telebot_handler
{
    telebot_core_handler_t core_h; /**< Core handler */
    int offset;                    /**< Offset value to get updates */
};

/**
 * @brief This object represents a telegram bot response.
 */
struct telebot_core_response
{
    telebot_error_e ret; /**< Telegram bot response code */
    size_t size;         /**< Telegam bot response size */
    char *data;          /**< Telegam bot response object */
};

#endif /* __TELEBOT_PRIVATE_H__ */
