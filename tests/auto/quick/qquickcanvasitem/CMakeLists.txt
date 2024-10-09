# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qquickcanvasitem.pro.

#####################################################################
## tst_qquickcanvasitem Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquickcanvasitem LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickcanvasitem
    GUI
    QMLTEST
    SOURCES
        tst_qquickcanvasitem.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickcanvasitem.pro:<TRUE>:
# OTHER_FILES = "data/tst_transform.qml" "data/tst_text.qml" "data/tst_strokeStyle.qml" "data/tst_state.qml" "data/tst_shadow.qml" "data/tst_pattern.qml" "data/tst_path.qml" "data/tst_line.qml" "data/tst_fillStyle.qml" "data/tst_fillrect.qml" "data/tst_composite.qml" "data/tst_canvas.qml" "data/tst_pixel.qml" "data/tst_gradient.qml" "data/tst_arcto.qml" "data/tst_arc.qml" "data/tst_context.qml" "data/tst_imagedata.qml" "data/CanvasTestCase.qml" "data/CanvasComponent.qml" "data/tst_image.qml" "data/tst_svgpath.qml" "data/anim-gr.gif" "data/anim-gr.png" "data/anim-poster-gr.png" "data/background.png" "data/broken.png" "data/ggrr-256x256.png" "data/green-1x1.png" "data/green-2x2.png" "data/green-16x16.png" "data/green-256x256.png" "data/green.png" "data/grgr-256x256.png" "data/red-16x16.png" "data/red.png" "data/redtransparent.png" "data/rgrg-256x256.png" "data/rrgg-256x256.png" "data/transparent.png" "data/transparent50.png" "data/yellow.png" "data/yellow75.png" "data/tst_invalidContext.qml"
# TEMPLATE = "app"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickcanvasitem CONDITION EXISTS _ss_[QT_INSTALL_PLUGINS]/imageformats
    DEFINES
        HAS_IMAGE_FORMATS
)
