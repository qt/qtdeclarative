# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from testapp.pro.

#####################################################################
## testapp Binary:
#####################################################################

qt_internal_add_executable(testapp
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
)

# Resources:
set(main_resource_files
    "delayedExit.qml"
    "delayedQuit.qml"
    "immediateExit.qml"
    "immediateQuit.qml"
)

qt_internal_add_resource(testapp "main"
    PREFIX
        "/"
    FILES
        ${main_resource_files}
)


#### Keys ignored in scope 1:.:.:testapp.pro:<TRUE>:
# TEMPLATE = "app"
