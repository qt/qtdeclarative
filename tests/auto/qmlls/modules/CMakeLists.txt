# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data)
file(GLOB_RECURSE test_data_glob_qmlformat
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/../../qml/qmlformat/
    data)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmlls_modules
    SOURCES
        tst_qmlls_modules.cpp
        tst_qmlls_modules.h
    DEFINES
        QT_DEPRECATED_WARNINGS
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
        QT_QMLFORMATTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/../../qml/qmlformat/data"
    LIBRARIES
        Qt::Core
        Qt::QmlDomPrivate
        Qt::LanguageServerPrivate
        Qt::Test
        Qt::QuickTestUtilsPrivate
        Qt::QmlLSPrivate
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_qmlls_modules CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/domdata"
)

add_dependencies(tst_qmlls_modules qmlls)
