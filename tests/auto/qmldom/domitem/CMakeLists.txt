# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from domitem.pro.

#####################################################################
## tst_qmldomitem Test:
#####################################################################
# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/..
    domdata/domitem/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmldomitem
    SOURCES
        tst_qmldomitem.cpp tst_qmldomitem.h
    DEFINES
        QT_DEPRECATED_WARNINGS
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/../domdata"
    LIBRARIES
        Qt::QmlDomPrivate
        Qt::Test
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_qmldomitem CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/domdata"
)
