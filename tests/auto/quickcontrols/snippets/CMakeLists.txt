# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

# Generated from snippets.pro.

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_snippets LANGUAGES C CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## tst_snippets Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/*)
list(APPEND test_data ${test_data_glob})

set(SNIPPETS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/quickcontrols/doc/snippets")

if(WEBOS)
    # Collect snippets for webOS
    file(GLOB_RECURSE test_snippets_glob
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/quickcontrols/doc/snippets/*)
    list(APPEND test_snippets ${test_snippets_glob})

    # Copy snippets to a location which is included in the webOS emulator image
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/snippets)
    file(COPY ${test_snippets} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/data/snippets)

    set(SNIPPETS_PATH "./data/snippets")
endif()

qt_internal_add_test(tst_snippets
    SOURCES
        tst_snippets.cpp
    DEFINES
        QQC2_SNIPPETS_PATH="${SNIPPETS_PATH}"
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::Quick
        Qt::QuickControls2
        Qt::QuickControls2Private
    TESTDATA ${test_data}
)

# Resources:
set(qmake_immediate_resource_files
    "qtquickcontrols2.conf"
)

qt_internal_add_resource(tst_snippets "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


#### Keys ignored in scope 1:.:.:snippets.pro:<TRUE>:
# OTHER_FILES = "$$PWD/data/*.qml"
# TEMPLATE = "app"

## Scopes:
#####################################################################
