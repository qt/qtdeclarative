# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlxmllistmodel
    SOURCES
        tst_qqmlxmllistmodel.cpp
    LIBRARIES
        Qt::Core
        Qt::Qml
        Qt::QmlXmlListModelPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_qqmlxmllistmodel CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlxmllistmodel CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
