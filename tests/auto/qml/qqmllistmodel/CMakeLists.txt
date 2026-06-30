# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmllistmodel.pro.

#####################################################################
## tst_qqmllistmodel Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qqmllistmodel LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmllistmodel
    SOURCES
        tst_qqmllistmodel.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlModelsPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmllistmodel CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmllistmodel CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

if (TARGET QOffscreenIntegrationPlugin)
    add_dependencies(tst_qqmllistmodel QOffscreenIntegrationPlugin)
endif()

# A module whose QML source is discarded from the resources (DISCARD_QML_CONTENTS), so a
# binding's source can only be recovered from the ahead-of-time-compiled unit, never reparsed
# at run time. Used to verify that the "[]" source of an empty-array list role survives
# qmlcachegen.
qt_add_library(tst_qqmllistmodel_discard STATIC)
qt_autogen_tools_initial_setup(tst_qqmllistmodel_discard)
qt_add_qml_module(tst_qqmllistmodel_discard
    URI listmodeldiscard
    QML_FILES
        EmptyListRoleViaModels.qml
        EmptyListRoleViaQtQml.qml
    DISCARD_QML_CONTENTS
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/listmodeldiscard"
    RESOURCE_PREFIX "/"
)
qt_autogen_tools_initial_setup(tst_qqmllistmodel_discardplugin)
add_dependencies(tst_qqmllistmodel tst_qqmllistmodel_discard)
target_link_libraries(tst_qqmllistmodel PRIVATE tst_qqmllistmodel_discardplugin)
