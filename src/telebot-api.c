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
#include <curl/curl.h>
#include <curl/easy.h>
#include <telebot-api.h>

static size_t write_data_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t r_size = size * nmemb;
    telebot_resp_t *resp = (telebot_resp_t *) userp;

    resp->data = realloc(resp->data, resp->size + r_size + 1);
    if (resp->data == NULL) return 0;

    memcpy((resp->data + resp->size), contents, r_size);
    resp->size += r_size;
    resp->data[resp->size] = 0;

    return r_size;
}

static telebot_error_e telebot_curl_perform(telebot_handler_t *handler, const char *method, struct curl_httppost *post)
{
    CURL *curl_h;
    CURLcode res;

    telebot_resp_t *resp = handler->response;
    resp->data = (char *)malloc(1);
    resp->size = 0;

    curl_h = curl_easy_init();

    if (curl_h == NULL)
        return TELEBOT_ERROR_OPERATION_FAILED;

    char URL[TELEBOT_URL_SIZE];
    snprintf(URL, TELEBOT_URL_SIZE, "%s%s/%s", TELEBOT_BASE_URL, handler->token, method);
    curl_easy_setopt(curl_h, CURLOPT_URL, URL);
    curl_easy_setopt(curl_h, CURLOPT_WRITEFUNCTION, write_data_cb);
    curl_easy_setopt(curl_h, CURLOPT_WRITEDATA, resp);

    if (post != NULL)
        curl_easy_setopt(curl_h, CURLOPT_HTTPPOST, post);

    res = curl_easy_perform(curl_h);
    if (res != CURLE_OK) {
        if (resp->data != NULL)
            free(resp->data);

        return TELEBOT_ERROR_OPERATION_FAILED;
    }

    curl_easy_cleanup(curl_h);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_create(telebot_handler_t *handler, char *token)
{
    if (token == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (handler == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    handler->token = strdup(token);
    handler->offset = 0;
    handler->response = (telebot_resp_t *)malloc(sizeof(telebot_resp_t));
    handler->response->data = NULL;
    handler->response->size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_destroy(telebot_handler_t *handler)
{
    if (handler == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    // clean token
    bzero(handler->token, strlen(handler->token));
    handler->token = NULL;

    if (handler->response->data != NULL)
        free(handler->response->data);

    if (handler->response != NULL)
        free(handler->response);

    curl_global_cleanup();

    return TELEBOT_ERROR_NONE;
}

telebot_error_e telebot_get_me(telebot_handler_t *handler)
{
    if (handler == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (handler->token == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (handler->response == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    return telebot_curl_perform(handler, "getMe", NULL);
}

telebot_error_e telebot_get_updates(telebot_handler_t *handler, int offset, int limit, int timeout)
{
    if (handler == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (handler->token == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    if (handler->response == NULL)
        return TELEBOT_ERROR_INVALID_PARAMETER;

    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;

    char offset_str[16], limit_str[16], timeout_str[16];
    snprintf(offset_str, sizeof(offset_str), "%d", offset);
    snprintf(limit_str, sizeof(limit_str), "%d", limit);
    snprintf(timeout_str, sizeof(timeout_str), "%d", timeout);

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "offset", CURLFORM_COPYCONTENTS, offset_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "limit", CURLFORM_COPYCONTENTS, limit_str, CURLFORM_END);
    curl_formadd(&post, &last, CURLFORM_COPYNAME, "timeout", CURLFORM_COPYCONTENTS, timeout_str, CURLFORM_END);

    return telebot_curl_perform(handler, "getUpdates", post);
}
