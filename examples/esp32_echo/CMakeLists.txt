# Telebot echo bot for ESP32 (ESP-IDF >= 5.1)
cmake_minimum_required(VERSION 3.22)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# Only build the components main actually depends on
idf_build_set_property(MINIMAL_BUILD ON)

project(telebot_esp32_echo)
