# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# A qt_add_qml_module target already has preview support: qt_add_qml_preview() is a no-op.
qt_add_executable(previewapp main.cpp)
qt_add_qml_module(previewapp
    URI PreviewApp
    VERSION 1.0
    QML_FILES AppMain.qml
)

qt_add_qml_preview(previewapp)
