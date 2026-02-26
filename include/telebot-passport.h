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

#ifndef __TELEBOT_PASSPORT_H__
#define __TELEBOT_PASSPORT_H__

#include <stdbool.h>
#include "telebot-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file        telebot-passport.h
 * @ingroup     TELEBOT_API
 * @brief       This file contains Telegram Passport feature of telegram bot
 * @author      Elmurod Talipov
 * @date        2026-02-27
 */

/**
 * @addtogroup TELEBOT_API
 * @{
 */

/**
 * @brief Use this method to set passport data errors.
 */
telebot_error_e telebot_set_passport_data_errors(telebot_handler_t handle,
    long long int user_id, const char *errors);

/**
 * @} // end of APIs
 */

#ifdef __cplusplus
}
#endif

#endif /* __TELEBOT_PASSPORT_H__ */
