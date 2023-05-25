# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmldomConstruction Test:
#####################################################################

qt_internal_add_benchmark(tst_qmldomconstruction
    SOURCES
        tst_qmldomconstruction.cpp
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARIES
        Qt::QmlDomPrivate
        Qt::Test
)
