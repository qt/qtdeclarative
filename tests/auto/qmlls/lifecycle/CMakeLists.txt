# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_lifecycle Test:
#####################################################################

qt_internal_add_test(tst_lifecycle
    SOURCES
        tst_lifecycle.cpp
        qiopipe.h qiopipe.cpp
    DEFINES
        QT_DEPRECATED_WARNINGS
    LIBRARIES
        Qt::CorePrivate
        Qt::LanguageServerPrivate
        Qt::Test
        Qt::QmlLSPrivate
    TESTDATA ${test_data}
)
