# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from rendernode.pro.

#####################################################################
## tst_rendernode Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_rendernode
    SOURCES
        tst_rendernode.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:rendernode.pro:<TRUE>:
# OTHER_FILES = "data/RenderOrder.qml" "data/MessUpState.qml" "data/matrix.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_rendernode CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_rendernode CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

qt_internal_add_shaders(tst_rendernode "rendernode_shaders"
    PREFIX
        /shaders
    FILES
        color.vert
        color.frag
)
