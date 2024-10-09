# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qquickitemlayer.pro.

#####################################################################
## tst_qquickitemlayer Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquickitemlayer LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickitemlayer
    SOURCES
        tst_qquickitemlayer.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickitemlayer.pro:<TRUE>:
# OTHER_FILES = "data/Smooth.qml" "data/Enabled.qml" "data/Mipmap.qml" "data/Effect.qml" "data/SourceRect.qml" "data/TextureProvider.qml" "data/Visible.qml" "data/ZOrder.qml" "data/ZOrderChange.qml" "data/ToggleLayerAndEffect.qml" "data/DisableLayer.qml" "data/SamplerNameChange.qml" "data/ItemEffect.qml" "data/RectangleEffect.qml" "data/TextureMirroring.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickitemlayer CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquickitemlayer CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
