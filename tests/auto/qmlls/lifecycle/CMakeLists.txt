# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_lifecycle Test:
#####################################################################

qt_internal_add_test(tst_lifecycle
    SOURCES
        ../../../../tools/qmlls/qlanguageserver.h ../../../../tools/qmlls/qlanguageserver.cpp
        tst_lifecycle.cpp
        qiopipe.h qiopipe.cpp
    INCLUDE_DIRECTORIES
        ../../../../tools/qmlls
    DEFINES
        QT_DEPRECATED_WARNINGS
    LIBRARIES
        Qt::CorePrivate
        Qt::LanguageServerPrivate
        Qt::Test
    TESTDATA ${test_data}
)
