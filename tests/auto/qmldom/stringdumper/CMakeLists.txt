# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from stringdumper.pro.

#####################################################################
## tst_qmldomstringdumper Test:
#####################################################################

qt_internal_add_test(tst_qmldomstringdumper
    SOURCES
        tst_qmldomstringdumper.cpp tst_qmldomstringdumper.h
    DEFINES
        QT_DEPRECATED_WARNINGS
    INCLUDE_DIRECTORIES
        ../../../../src/qmldom
    LIBRARIES
        Qt::QmlDomPrivate
)
