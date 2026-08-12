# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qquicknativedrag Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquicknativedrag LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qquicknativedrag
    SOURCES
        tst_qquicknativedrag.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquicknativedrag CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquicknativedrag CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

# Drag.dragType: Drag.Automatic goes through QDrag::exec(), which runs a nested event
# loop that only an in-process QPlatformDrag lets synthetic input terminate. The
# offscreen platform provides one, and implements QCursor::setPos(), which
# QSimpleDrag::startDrag() needs to resolve the source window. The tests QSKIP
# elsewhere, so running the binary directly on another platform is still safe.
set_property(TEST tst_qquicknativedrag APPEND PROPERTY ENVIRONMENT
    "QT_QPA_PLATFORM=offscreen"
)
