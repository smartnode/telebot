# Interactive demo bot for Linux/macOS: exercises many API calls by chat command.
# Reads the bot token from a .token file in the current directory.
ADD_EXECUTABLE(demo_bot demo_bot.c)
TARGET_LINK_LIBRARIES(demo_bot ${PKGS_LDFLAGS} ${PROJECT_NAME} pthread)
# Absolute path, so the bot finds its sample media from any working directory
TARGET_COMPILE_DEFINITIONS(demo_bot PRIVATE DEMO_SAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/samples")
