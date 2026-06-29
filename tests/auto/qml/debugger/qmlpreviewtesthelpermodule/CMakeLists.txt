# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_executable(qmlpreviewtesthelpermodule
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        main.cpp
    DEFINES
        QT_QML_DEBUG_NO_WARNING
    LIBRARIES
        Qt::Core
        Qt::Qml
)

qt_add_qml_module(qmlpreviewtesthelpermodule
    URI QmlPreviewTestHelpers
    VERSION 1.0
    QML_FILES
        Main.qml
)
