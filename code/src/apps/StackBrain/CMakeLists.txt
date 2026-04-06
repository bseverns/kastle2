#
# MIT License
# Copyright (c) 2026 Ben Severns
#

# App definition
create_kastle2_app(
    APP_NAME "stackbrain"
    APP_USB_NAME "Kastle 2 StackBrain"
    APP_USB_PREFIX "K2SB_"
    APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/AppStackBrain.cpp
)
