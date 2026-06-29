# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# qt_add_qml_preview() must reject non-executable targets.
qt_add_library(previewlib STATIC main.cpp)
qt_add_qml_preview(previewlib)
