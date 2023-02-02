# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Collect test data
file(GLOB_RECURSE test_data_glob
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickitemrhiintegration
    SOURCES
        tst_qquickitemrhiintegration.cpp
        rhiitem.cpp rhiitem.h rhiitem_p.h
        testrhiitem.cpp testrhiitem.h
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_qquickitemrhiintegration CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquickitemrhiintegration CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

qt_internal_add_shaders(tst_qquickitemrhiintegration "shaders"
    PREFIX
        "/"
    FILES
        "texture.vert"
        "texture.frag"
)

qt_policy(SET QTP0001 NEW)

qt_add_qml_module(tst_qquickitemrhiintegration
    URI TestQquickitemrhiintegration
)
