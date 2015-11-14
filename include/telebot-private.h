#ifndef __TELEBOT_PRIVATE_H__
#define __TELEBOT_PRIVATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define TELEGRAM_BOT_BASE_URL  "https://api.telegram.org/bot"
#define TELEGRAM_BOT_URL_SIZE  1024

#define TELEGRAM_BOT_METHOD_GET_ME 		"getMe"
#define TELEGRAM_BOT_METHOD_GET_UPDATES		"getUpdates"
#define TELEGRAM_BOT_METHOD_SEND_MESSAGE	"sendMessage"
#define TELEGRAM_BOT_METHOD_FORWARD_MESSAGE	"forwardMessage"
#define TELEGRAM_BOT_METHOD_SEND_PHOTO		"sendPhoto"
#define TELEGRAM_BOT_METHOD_SEND_AUDIO		"sendAudio"
#define TELEGRAM_BOT_METHOD_SEND_DOCUMENT	"sendDocument"
#define TELEGRAM_BOT_METHOD_SEND_VIDEO		"sendVideo"
#define TELEGRAM_BOT_METHOD_SEND_STICKER	"sendSticker"
#define TELEGRAM_BOT_METHOD_SEND_VOICE		"sendVoice"
#define TELEGRAM_BOT_METHOD_SEND_LOCATION	"sendLocation"
#define TELEGRAM_BOT_METHOD_SEND_CHAT_ACTION	"sendChatAction"
#define TELEGRAM_BOT_METHOD_GET_USER_PROFILES	"getUserProfilePhotos"
#define TELEGRAM_BOT_METHOD_GET_FILE		"getFile"
#define TELEGRAM_BOT_METHOD_SET_WEBHOOK		"setWebhook"
#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_PRIVATE_H__ */

