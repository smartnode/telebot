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

#ifndef __TELEBOT_CORE_API_H__
#define __TELEBOT_CORE_API_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        telebot-core-api.h
 * @ingroup     TELEBOT_CORE_API
 * @brief       This file contains the API for the telegram bot interface
 * @author      Elmurod Talipov
 * @date        2015-10-17
 * @version     0.1
 */

/**
 * @defgroup TELEBOT_CORE_API		Telegram Bot API
 * @brief Telebot Library
 *
 *
 * @addtogroup TELEBOT_CORE_API
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

/**
 * @brief This object represents a telegram response in JSON form.
 */
typedef struct _telebot_resp_t_ {
    char  *data;	/**< JSON response data */
    size_t size;	/**< Size of response data */
} telebot_resp_t;

/**
 * @brief This object represents a core telebot handler.
 */
typedef struct _telebot_core_h_ {
    char *token;		/**< Telegam bot token */
    int  offset;		/**< Telegam last update id */
    telebot_resp_t *response;	/**< Telegam response object */
} telebot_core_h;

/**
 * @brief Start function to use telebot core APIs.
 *
 * This function must be used first to call, and it creates handler that is 
 * used as input to other functions in telebot core interface. This call MUST 
 * have corresponding call to telebot_core_destroy when operation is complete.
 */
telebot_error_e telebot_core_create(telebot_core_h *handler, char *token);

/**
 * @brief Final function to use telebot core APIs
 *
 * This function must be the last function to call for an telebot core use.  
 * It is the ooposite of the telebot_core_create function and must be called 
 * with the same handler as the input that a telebot_core_create call 
 * created.
 */
telebot_error_e telebot_core_destroy(telebot_core_h *handler);

/**
 * @brief This function gets basic information about the bot. All core API 
 * responses are JSON objects.
 */
telebot_error_e telebot_core_get_me(telebot_core_h *handler);

/**
 * @brief This function is used to receive incoming updates (long polling).
 */
telebot_error_e telebot_core_get_updates(telebot_core_h *handler, int offset, 
                    int limit, int timeout);

/**
 * @brief This function is used to get user profile pictures object
 */
telebot_error_e telebot_core_get_user_profile_photos(telebot_core_h *handler, 
                    int user_id, int offset, int limit);

telebot_error_e telebot_core_send_message(telebot_core_h *handler, int chat_id, 
                    char *text, bool disable_web_page_preview, 
                    int reply_to_message_id, char *reply_markup);

telebot_error_e telebot_core_forward_message(telebot_core_h *handler, 
                    int chat_id, int from_chat_id, int message_id);

telebot_error_e telebot_core_send_photo(telebot_core_h *handler, int chat_id, 
                    char *photo_file, char *caption, int reply_to_message_id, 
                    char *reply_markup);

telebot_error_e telebot_core_send_audio(telebot_core_h *handler, int chat_id, 
                    char *audio_file, int duration, char *performer, 
                    char *title, int reply_to_message_id, char *reply_markup);

telebot_error_e telebot_core_send_document(telebot_core_h *handler, int chat_id, 
                    char *document_file, int reply_to_message_id, 
                    char *reply_markup);

telebot_error_e telebot_core_send_sticker(telebot_core_h *handler, int chat_id, 
                    char *sticker, int reply_to_message_id, char *reply_markup);

telebot_error_e telebot_core_send_video(telebot_core_h *handler, int chat_id, 
                    char *video_file, int duration, char *caption, 
                    int reply_to_message_id, char *reply_markup);

telebot_error_e telebot_core_send_voice(telebot_core_h *handler, int chat_id, 
                    char *voice_file, int duration, int reply_to_message_id, 
                    char *reply_markup);

telebot_error_e telebot_core_send_location(telebot_core_h *handler, int chat_id, 
                    float latitude, float longitude, int reply_to_message_id, 
                    char *reply_markup);

telebot_error_e telebot_core_send_chat_action(telebot_core_h *handler, 
                    int chat_id, char *action);

telebot_error_e telebot_core_set_web_hook(telebot_core_h *handler, char *url, 
                    char *certificate);

telebot_error_e telebot_core_get_file(telebot_core_h *handler, char *file_id, 
                    char *out_file);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_CORE_API_H__ */

