# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_dom_all Binary executing all tests together
## (simpler to verify coverage)
#####################################################################
# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/..
    domdata/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_dom_all
    SOURCES
        tst_dom_all.cpp
        ../stringdumper/tst_qmldomstringdumper.h
        ../errormessage/tst_qmldomerrormessage.cpp ../errormessage/tst_qmldomerrormessage.h
        ../path/tst_qmldompath.h
        ../domitem/tst_qmldomitem.h
        ../merging/tst_dommerging.h
        ../reformatter/tst_reformatter.h
    INCLUDE_DIRECTORIES
        ..
    DEFINES
        NO_QTEST_MAIN
        QT_DEPRECATED_WARNINGS
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/../domdata"
    LIBRARIES
        Qt::Core
        Qt::QmlDomPrivate
        Qt::Test
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_dom_all CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/domdata"
)
