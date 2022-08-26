# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qjsvalue.pro.

#####################################################################
## tst_qjsvalue Test:
#####################################################################

# special case begin
# TODO: Prepare for removal, once Platform brings in Threads.
if(NOT TARGET Threads::Threads)
    qt_find_package(Threads PROVIDED_TARGETS Threads::Threads)
endif()
# special case end

qt_internal_add_test(tst_qjsvalue
    SOURCES
        tst_qjsvalue.cpp tst_qjsvalue.h
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::Widgets
    LIBRARIES # special case
        Threads::Threads # special case
    TESTDATA "dummy_imports.qml"
)

if(QT_BUILD_STANDALONE_TESTS)
    qt_import_qml_plugins(tst_qjsvalue)
endif()

## Scopes:
#####################################################################
