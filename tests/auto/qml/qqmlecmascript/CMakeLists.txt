# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmlecmascript.pro.

#####################################################################
## tst_qqmlecmascript Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlecmascript
    SOURCES
        testtypes.cpp testtypes.h
        tst_qqmlecmascript.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
        Qt::QmlPrivate
        Qt::QuickTestUtilsPrivate
        Qt::QuickPrivate
    TESTDATA ${test_data}
)

# Resources:
set(qqmlecmascript_resource_files
    "data/include.js"
    "data/include.qml"
    "data/js/include2.js"
    "data/js/include3.js"
)

# special case begin
set_source_files_properties(
    ${qqmlecmascript_resource_files}
    PROPERTIES
        QT_SKIP_QUICKCOMPILER 1)
# special case end

qt_internal_add_resource(tst_qqmlecmascript "qqmlecmascript"
    PREFIX
        "/"
    FILES
        ${qqmlecmascript_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlecmascript CONDITION TARGET Qt::Widgets
    LIBRARIES
        Qt::Widgets
)

qt_internal_extend_target(tst_qqmlecmascript CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlecmascript CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
