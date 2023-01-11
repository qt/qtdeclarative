# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmlprofilerservice.pro.

#####################################################################
## tst_qqmlprofilerservice Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlprofilerservice
    SOURCES
        ../shared/debugutil.cpp ../shared/debugutil_p.h
        ../shared/qqmldebugprocess.cpp ../shared/qqmldebugprocess_p.h
        tst_qqmlprofilerservice.cpp
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlDebugPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qqmlprofilerservice.pro:<TRUE>:
# OTHER_FILES = "data/pixmapCacheTest.qml" "data/controlFromJS.qml" "data/test.qml" "data/exit.qml" "data/scenegraphTest.qml" "data/TestImage_2x2.png" "data/signalSourceLocation.qml" "data/javascript.qml" "data/timer.qml" "data/qstr.qml" "data/memory.qml" "data/batchOverflow.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlprofilerservice CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlprofilerservice CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
