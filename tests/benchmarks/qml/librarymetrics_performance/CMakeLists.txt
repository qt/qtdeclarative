# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from librarymetrics_performance.pro.

#####################################################################
## tst_librarymetrics_performance Binary:
#####################################################################

qt_internal_add_benchmark(tst_librarymetrics_performance
    SOURCES
        tst_librarymetrics_performance.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Qml
        Qt::Quick
        Qt::Test
)

# Resources:
set(qmake_immediate_resource_files
    "data"
)

qt_internal_add_resource(tst_librarymetrics_performance "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


#### Keys ignored in scope 1:.:.:librarymetrics_performance.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################
