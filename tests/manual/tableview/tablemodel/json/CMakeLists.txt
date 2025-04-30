# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from json.pro.

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(json LANGUAGES C CXX)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## json Binary:
#####################################################################

qt_internal_add_manual_test(json
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
set(qmake_immediate_resource_files
    "JsonData.js"
    "main.qml"
)

qt_internal_add_resource(json "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


#### Keys ignored in scope 1:.:.:json.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:json.pro:QNX:
# target.path = "/tmp/$${TARGET}/bin"

#### Keys ignored in scope 4:.:.:json.pro:UNIX AND NOT ANDROID:
# target.path = "/opt/$${TARGET}/bin"

#### Keys ignored in scope 5:.:.:json.pro:NOT target.path_ISEMPTY:
# INSTALLS = "target"
