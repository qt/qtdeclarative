# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_benchmark(tst_hugeArrayCompilation
    SOURCES
        tst_hugeArrayCompilation.cpp
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARIES
        Qt::Qml
        Qt::QmlCompilerPrivate
        Qt::Test
)
