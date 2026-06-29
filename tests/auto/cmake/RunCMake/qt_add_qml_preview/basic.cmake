# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Explicit qt_add_qml_preview() on an executable linking an in-project QML module.
add_subdirectory(PreviewModule)

qt_add_executable(previewapp main.cpp)
target_link_libraries(previewapp PRIVATE PreviewModule)

qt_add_qml_preview(previewapp)
qt_add_qml_preview(previewapp) # second call must be a no-op

file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/preview_target_exists.txt"
    CONTENT "$<TARGET_NAME_IF_EXISTS:previewapp_qmlpreview>"
)
