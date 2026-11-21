idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES telebot nvs_flash esp_event esp_netif protocol_examples_common)
