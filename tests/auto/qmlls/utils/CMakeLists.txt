# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmlls_utils
    SOURCES
    tst_qmlls_utils.cpp
    DEFINES
        QT_QMLLS_UTILS_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    LIBRARIES
        Qt::Core
        Qt::QmlDomPrivate
        Qt::LanguageServerPrivate
        Qt::Test
        Qt::QuickTestUtilsPrivate
        Qt::QmlLSPrivate
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_qmlls_utils CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLLS_UTILS_DATADIR=":/domdata"
)
