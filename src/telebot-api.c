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
#include <pthread.h>
#include <errno.h>
#include <telebot-private.h>
#include <telebot-core-api.h>
#include <telebot-api.h>
#include <telebot-parser.h>

static telebot_update_cb_f g_update_cb;
static telebot_core_h *g_handler;
static bool g_run_telebot;
static void *telebot_polling_thread(void *data);

telebot_error_e telebot_create(char *token)
{
    g_handler = (telebot_core_h *)malloc(sizeof(telebot_core_h));
    if (g_handler == NULL) {
        ERR("Failed to allocate memory");
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

    pthread_t thread_id;
    pthread_attr_t attr;

    int ret = pthread_attr_init(&attr);
    if (ret != 0) {
        ERR("Failed to init pthread attributes, error: %d", errno);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }
    ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (ret != 0) {
        ERR("Failed to set PHTREAD_CREATE_DETACHED attribute, error: %d", errno);
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    g_update_cb = update_cb;
    g_run_telebot = true;

    ret = pthread_create(&thread_id, &attr, telebot_polling_thread, NULL);
    if (ret != 0) {
        ERR("Failed to create thread, error: %d", errno);
        g_update_cb = NULL;
        g_run_telebot = false;
        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_stop()
{
    g_run_telebot = false;
    g_update_cb = NULL;

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_get_me(telebot_user_t **me)
{
    if (me == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (g_handler == NULL)
        return TELEBOT_ERROR_NOT_SUPPORTED;

    *me = NULL;

    int ret = telebot_core_get_me(g_handler);
    if (ret != TELEBOT_ERROR_NONE)
        return ret;

    telebot_user_t *tmp = telebot_parser_get_user(g_handler->response->data);
    free(g_handler->response->data);
    g_handler->response->size = 0;

    if (tmp == NULL)
        return TELEBOT_ERROR_OPERATION_FAILED;

    *me = tmp;

    return TELEBOT_ERROR_NONE;
}

static void *telebot_polling_thread(void *data)
{
    int ret;

    while (g_run_telebot) {
        ret = telebot_core_get_updates(g_handler, g_handler->offset,
                TELEBOT_UPDATE_COUNT_PER_REQUEST, 0);

        if (ret != TELEBOT_ERROR_NONE)
            continue;

        ret = telebot_parser_get_update_id(g_handler->response->data);
        if (ret < g_handler->offset) {
            ERR("Last received update_id < current offset, wrong update_id");
            continue;
        }
        else if (ret == g_handler->offset) {
            DBG("This update is already processed");
            continue;
        }

        g_handler->offset = ret;
        telebot_message_t *tmp = telebot_parser_get_message(g_handler->response->data);
        if (tmp != NULL)
            g_update_cb(tmp); // hand data to callback function

        free(g_handler->response->data);
        g_handler->response->size = 0;

        usleep(TELEBOT_UPDATE_POLLING_INTERVAL);
    }

    exit_thread(NULL);

    return NULL;
}

