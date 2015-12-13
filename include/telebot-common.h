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

#ifndef __TELEBOT_COMMON_H__
#define __TELEBOT_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        telebot-common.h
 * @ingroup     TELEBOT_API
 * @brief       This file contains telegram bot common defintions
 * @author      Elmurod Talipov
 * @date        2015-12-13
 */

/**
 * @addtogroup TELEBOT_API
 * @{
 */

#define TELEBOT_FIRST_NAME_SIZE         32
#define TELEBOT_LAST_NAME_SIZE          32
#define TELEBOT_USER_NAME_SIZE          16
#define TELEBOT_FILE_ID_SIZE            256
#define TELEBOT_CHAT_TYPE_SIZE          10
#define TELEBOT_CHAT_TITLE_SIZE         128
#define TELEBOT_AUDIO_PERFORMER_SIZE    64
#define TELEBOT_AUDIO_TITLE_SIZE        128
#define TELEBOT_AUDIO_MIME_TYPE_SIZE    64
#define TELEBOT_FILE_NAME_SIZE          128
#define TELEBOT_DOCUMENT_MIME_TYPE_SIZE 128
#define TELEBOT_VIDEO_MIME_TYPE_SIZE    64
#define TELEBOT_VOICE_MIME_TYPE_SIZE    64
#define TELEBOT_PHONE_NUMBER_SIZE       32
#define TELEBOT_FILE_PATH_SIZE          256
#define TELEBOT_MESSAGE_TEXT_SIZE       4096
#define TELEBOT_MESSAGE_CAPTION_SIZE    256 

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
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_CORE_API_H__ */

