# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_test(tst_androidassets
    SOURCES
        tst_androidassets.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# add qml/*.qml files as assets instead of resources

file(
    COPY qml/main.qml
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/android-build/assets/qml/")

file(
    COPY qml/pages/MainPage.qml
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/android-build/assets/qml/pages/")
