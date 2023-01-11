# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmlpreview.pro.

#####################################################################
## tst_qqmlpreview Test:
#####################################################################

# Collect test data
list(APPEND test_data "data/window.qml")
list(APPEND test_data "data/qtquick2.qml")
list(APPEND test_data "data/window2.qml")
list(APPEND test_data "data/window1.qml")
list(APPEND test_data "data/broken.qml")
list(APPEND test_data "data/zoom.qml")

qt_internal_add_test(tst_qqmlpreview
    SOURCES
        ../../../../../src/plugins/qmltooling/qmldbg_preview/qqmlpreviewblacklist.cpp ../../../../../src/plugins/qmltooling/qmldbg_preview/qqmlpreviewblacklist.h
        ../shared/debugutil.cpp ../shared/debugutil_p.h
        ../shared/qqmldebugprocess.cpp ../shared/qqmldebugprocess_p.h
        tst_qqmlpreview.cpp
    INCLUDE_DIRECTORIES
        ../../../../../src/plugins/qmltooling/qmldbg_preview
        ../shared
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QmlDebugPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlpreview CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlpreview CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
