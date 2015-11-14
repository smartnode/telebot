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

#ifndef __TELEBOT_API_H__
#define __TELEBOT_API_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        telebot-api.h
 * @ingroup     TELEBOT_API
 * @brief       This file contains the API for the telegram bot interface
 * @author      Elmurod Talipov
 * @date        2015-10-17
 * @version     0.1
 */

/**
 * @defgroup TELEBOT_API		Telegram Bot C API
 * @brief Telebot Library
 *
 *
 * @addtogroup TELEBOT_API
 * @{
 */

/**
 * @brief Enumerations of error code for telebot programming interface
 */
typedef enum {
    TELEBOT_ERROR_NONE              = 0,    /**< Successful */
    TELEBOT_ERROR_OPERATION_FAILED  = -1,   /**< Operation failed */
    TELEBOT_ERROR_NOT_SUPPORTED     = -2,   /**< Not supported */
    TELEBOT_ERROR_OUT_OF_MEMORY     = -3,   /**< Out of memory */
    TELEBOT_ERROR_NO_CONNECTION     = -4,   /**< No Internet connection */
    TELEBOT_ERROR_INVALID_PARAMETER = -5,   /**< Invalid parameter */
} telebot_error_e;

typedef struct _telebot_resp_t_ {
    char  *data;
    size_t size;
} telebot_resp_t;

typedef struct _telebot_handler_t_ {
    char *token;
    int  offset;
    telebot_resp_t *response;
} telebot_handler_t;

telebot_error_e telebot_create(telebot_handler_t *handler, char *token);
telebot_error_e telebot_destroy(telebot_handler_t *handler);
telebot_error_e telebot_get_me(telebot_handler_t *handler);
telebot_error_e telebot_get_updates(telebot_handler_t *handler, int offset, int limit, int timeout);
telebot_error_e telebot_get_user_profile_photos(telebot_handler_t *handler, int user_id, int offset, int limit);
telebot_error_e telebot_send_message(telebot_handler_t *handler, int chat_id, char *text, bool disable_web_page_preview, int reply_to_message_id, char *reply_markup);
telebot_error_e telebot_forward_message(telebot_handler_t *handler, int chat_id, int from_chat_id, int message_id);
telebot_error_e telebot_send_photo(telebot_handler_t *handler, int chat_id, char *photo_file, char *caption, int reply_to_message_id, char *reply_markup);
telebot_error_e telebot_send_audio(telebot_handler_t *handler, int chat_id, char *audio_file, int duration, char *performer, char *title, int reply_to_message_id, char *reply_markup);
telebot_error_e telebot_send_document(telebot_handler_t *handler, int chat_id, char *document_file, int reply_to_message_id, char *reply_markup);
telebot_error_e telebot_send_sticker(telebot_handler_t *handler, int chat_id, char *sticker, int reply_to_message_id, char *reply_markup);
telebot_error_e telebot_send_video(telebot_handler_t *handler, int chat_id, char *video_file, int duration, char *caption, int reply_to_message_id, char *reply_markup);
telebot_error_e telebot_send_voice(telebot_handler_t *handler, int chat_id, char *voice_file, int duration, int reply_to_message_id, char *reply_markup);
telebot_error_e telebot_send_location(telebot_handler_t *handler, int chat_id, float latitude, float longitude, int reply_to_message_id, char *reply_markup);
telebot_error_e telebot_send_chat_action(telebot_handler_t *handler, int chat_id, char *action);
telebot_error_e telebot_set_web_hook(telebot_handler_t *handler, char *url, char *certificate);
telebot_error_e telebot_get_file(telebot_handler_t *handler, char *file_id, char *out_file);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_API_H__ */

