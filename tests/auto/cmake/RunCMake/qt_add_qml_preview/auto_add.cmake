# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Preview is enabled by default: the target must be created without an explicit call.
add_subdirectory(PreviewModule)

qt_add_executable(app main.cpp)
target_link_libraries(app PRIVATE PreviewModule)

file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/preview_target_exists.txt"
    CONTENT "$<TARGET_NAME_IF_EXISTS:app_qmlpreview>"
)
