# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_executable(qmlpreviewtesthelper
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        main.cpp
        resources.qrc
    DEFINES
        QT_QML_DEBUG_NO_WARNING
    LIBRARIES
        Qt::Core
        Qt::Qml
)
