# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from fonts.pro.

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(fonts LANGUAGES C CXX ASM)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## fonts Binary:
#####################################################################

qt_internal_add_manual_test(fonts
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
set(qml_resource_files
    "main.qml"
)

qt_internal_add_resource(fonts "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


#### Keys ignored in scope 1:.:.:fonts.pro:<TRUE>:
# QML_IMPORT_PATH = <EMPTY>
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:deployment.pri:UNIX AND NOT ANDROID:
# INSTALLS = "target"

#### Keys ignored in scope 5:.:.:deployment.pri:QNX:
# target.path = "/tmp/$${TARGET}/bin"

#### Keys ignored in scope 6:.:.:deployment.pri:else:
# target.path = "/opt/$${TARGET}/bin"
