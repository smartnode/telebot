/*
 * telebot
 *
 * Copyright (c) 2026 Elmurod Talipov.
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

/*
 * Telebot echo bot for ESP32.
 *
 * 1. idf.py menuconfig
 *      Example Connection Configuration -> Wi-Fi SSID / password
 *      Telebot echo example             -> Telegram bot token
 * 2. idf.py build flash monitor
 *
 * Every text message sent to the bot is echoed back as a reply. Heap and
 * stack high-water marks are logged so the library's real cost is visible.
 */

#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include <telebot.h>

static const char *TAG = "echo_bot";

static void log_memory(const char *when)
{
    ESP_LOGI(TAG, "[%s] heap free: %u, min ever: %u, stack left: %u bytes", when,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
             (unsigned)esp_get_minimum_free_heap_size(),
             (unsigned)uxTaskGetStackHighWaterMark(NULL));
}

static void echo_update(telebot_handler_t bot, const telebot_update_t *update)
{
    if (update->update_type != TELEBOT_UPDATE_TYPE_MESSAGE)
        return;

    const telebot_message_t *msg = &update->message;
    if ((msg->text == NULL) || (msg->chat == NULL))
        return;

    ESP_LOGI(TAG, "chat %lld: %s", msg->chat->id, msg->text);

    telebot_send_options_t opts = {0};
    opts.reply_to_message_id = msg->message_id;
    opts.message_thread_id = msg->message_thread_id; /* stay in the same forum topic */
    telebot_error_e ret = telebot_send_message(bot, msg->chat->id, msg->text, NULL, false, &opts);
    if (ret == TELEBOT_ERROR_RATE_LIMITED)
    {
        /* Flood control: wait as long as Telegram asks, then retry once */
        int wait_s = telebot_get_retry_after(bot);
        ESP_LOGW(TAG, "Rate limited, retrying in %d s", wait_s);
        vTaskDelay(pdMS_TO_TICKS(1000 * (wait_s > 0 ? wait_s : 1)));
        ret = telebot_send_message(bot, msg->chat->id, msg->text, NULL, false, &opts);
    }
    if (ret != TELEBOT_ERROR_NONE)
        ESP_LOGW(TAG, "sendMessage failed: %d", ret);
}

static void bot_task(void *arg)
{
    (void)arg;
    telebot_handler_t bot = NULL;
    if (telebot_create(&bot, CONFIG_ECHO_BOT_TOKEN) != TELEBOT_ERROR_NONE)
    {
        ESP_LOGE(TAG, "telebot_create failed");
        vTaskDelete(NULL);
        return;
    }

    telebot_user_t me;
    if (telebot_get_me(bot, &me) == TELEBOT_ERROR_NONE)
    {
        ESP_LOGI(TAG, "Running as @%s (id %lld)", me.username ? me.username : "?", me.id);
        telebot_put_me(&me);
    }
    else
    {
        ESP_LOGE(TAG, "getMe failed: check the token and network");
    }
    log_memory("ready");

    unsigned backoff_ms = 1000;
    for (;;)
    {
        telebot_update_t *updates = NULL;
        int count = 0;

        /* limit 0 -> CONFIG_TELEBOT_DEFAULT_UPDATE_LIMIT; offset 0 -> library tracks it */
        telebot_error_e ret = telebot_get_updates(bot, 0, 0, CONFIG_ECHO_POLL_TIMEOUT_S, NULL, 0, &updates, &count);
        if (ret != TELEBOT_ERROR_NONE)
        {
            ESP_LOGW(TAG, "getUpdates failed (%d), retrying in %u ms", ret, backoff_ms);
            vTaskDelay(pdMS_TO_TICKS(backoff_ms));
            backoff_ms = (backoff_ms < 30000) ? backoff_ms * 2 : 30000;
            continue;
        }
        backoff_ms = 1000;

        for (int i = 0; i < count; i++)
            echo_update(bot, &updates[i]);

        if (count > 0)
        {
            telebot_put_updates(updates, count);
            log_memory("after batch");
        }
    }
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if ((err == ESP_ERR_NVS_NO_FREE_PAGES) || (err == ESP_ERR_NVS_NEW_VERSION_FOUND))
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    if (strlen(CONFIG_ECHO_BOT_TOKEN) == 0)
    {
        ESP_LOGE(TAG, "Set the bot token in menuconfig (Telebot echo example)");
        return;
    }

    xTaskCreate(bot_task, "telebot", CONFIG_ECHO_TASK_STACK_SIZE, NULL, 5, NULL);
}
