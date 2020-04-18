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
 * @date        2020-04-19
 */

/**
 * @defgroup TELEBOT_API Simple Telegram Bot API
 * @brief Simplified APIs to create telegram bot.
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

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_COMMON_H__ */

