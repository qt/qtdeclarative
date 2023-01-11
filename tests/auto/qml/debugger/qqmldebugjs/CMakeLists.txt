# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmldebugjs.pro.

#####################################################################
## tst_qqmldebugjs Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmldebugjs
    SOURCES
        ../shared/debugutil.cpp ../shared/debugutil_p.h
        ../shared/qqmldebugprocess.cpp ../shared/qqmldebugprocess_p.h
        tst_qqmldebugjs.cpp
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlDebugPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qqmldebugjs.pro:<TRUE>:
# OTHER_FILES = "data/test.qml" "data/test.js" "data/timer.qml" "data/exception.qml" "data/oncompleted.qml" "data/loadjsfile.qml" "data/condition.qml" "data/changeBreakpoint.qml" "data/stepAction.qml" "data/breakpointRelocation.qml" "data/createComponent.qml" "data/encodeQmlScope.qml" "data/breakOnAnchor.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmldebugjs CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmldebugjs CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
