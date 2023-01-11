# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qquickshadereffect.pro.

#####################################################################
## tst_qquickshadereffect Test:
#####################################################################

qt_internal_add_test(tst_qquickshadereffect
    SOURCES
        tst_qquickshadereffect.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
)

# Resources:
set(resources_resource_files
    "data/+qsb/red.frag"
    "data/+qsb/test.frag"
    "data/+qsb/test.vert"
    "data/+qsb/testprop.frag"
    "data/MyIcon.qml"
    "data/connections.qml"
    "data/deleteShaderEffectSource.qml"
    "data/deleteSourceItem.qml"
    "data/hideParent.qml"
    "data/opacity-mask.frag.qsb"
    "data/red.frag"
    "data/star.png"
    "data/test.frag"
    "data/test.vert"
    "data/twoImagesOneShaderEffect.qml"
    "data/testprop.frag"
    "data/testProperties.qml"
)

qt_internal_add_resource(tst_qquickshadereffect "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickshadereffect CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquickshadereffect CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
