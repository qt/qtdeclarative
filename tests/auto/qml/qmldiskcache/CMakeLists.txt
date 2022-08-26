# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmldiskcache.pro.

#####################################################################
## tst_qmldiskcache Test:
#####################################################################

qt_internal_add_test(tst_qmldiskcache
    SOURCES
        tst_qmldiskcache.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::QmlPrivate
)

# special case begin
# Disable quick compiler for this test
set_source_files_properties(
    importmodule.qml
    module.mjs
    test.qml
    PROPERTIES
    QT_SKIP_QUICKCOMPILER TRUE
)
# special case end

# Resources:
set(qmake_immediate_resource_files
    "importmodule.qml"
    "module.mjs"
    "test.qml"
)

qt_internal_add_resource(tst_qmldiskcache "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


## Scopes:
#####################################################################
