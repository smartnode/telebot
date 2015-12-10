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
#include <telebot-private.h>
#include <telebot-core-api.h>
#include <telebot-api.h>
#include <telebot-parser.h>

static telebot_update_cb_f g_update_cb;
static telebot_core_h *g_handler;
static bool g_run_telebot;

telebot_error_e telebot_create(char *token)
{
    g_handler = (telebot_core_h *)malloc(sizeof(telebot_core_h));
    if (g_handler == NULL) {
        ERROR("Failed to allocate memory");
        return TELEBOT_ERROR_OUT_OF_MEMORY;
    }

    int ret = telebot_core_create(g_handler, token);
    if (ret != TELEBOT_ERROR_NONE) {
        free(g_handler);
        return ret;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_destroy()
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    telebot_core_destroy(g_handler);
    free(g_handler);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_start(telebot_update_cb_f update_cb)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    if (update_cb == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    g_update_cb = update_cb;
    g_run_telebot = true;

    //TODO: create thread and run

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_stop()
{
    g_run_telebot = false;
    g_update_cb = NULL;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_get_me(telebot_user_t *me)
{
    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    int ret = telebot_core_get_me(g_handler);

    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    me = telebot_parser_get_user(g_handler->response->data);

    if (me == NULL)
        return TELEBOT_ERROR_OPERATION_FAILED;

    return TELEBOT_ERROR_NONE;
}

