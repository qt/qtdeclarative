# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmlengine.pro.

#####################################################################
## tst_qqmlengine Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlengine
    SOURCES
        tst_qqmlengine.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
        Qt::QmlPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)


qt_add_library(tst_qqmlengine_qml STATIC)
qt_autogen_tools_initial_setup(tst_qqmlengine_qml)
qt_add_qml_module(tst_qqmlengine_qml
    URI OnlyDeclarative
    SOURCES
        "declarativelyregistered.h"
        "declarativelyregistered.cpp"
    RESOURCE_PREFIX
        "/"
    OUTPUT_DIRECTORY
        "OnlyDeclarative"
)

qt_autogen_tools_initial_setup(tst_qqmlengine_qmlplugin)
target_link_libraries(tst_qqmlengine PRIVATE tst_qqmlengine_qmlplugin)

# Resources:
set(qmake_immediate_resource_files
    "data/qrcurls.js"
    "data/qrcurls.qml"
)

qt_internal_add_resource(tst_qqmlengine "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)

if(QT_BUILD_STANDALONE_TESTS)
    qt_import_qml_plugins(tst_qqmlengine)
endif()

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlengine CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlengine CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
