# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_ecmascripttests Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    test262/*)
list(APPEND test_data ${test_data_glob})
list(FILTER test_data EXCLUDE REGEX ".git")

qt_internal_add_test(tst_ecmascripttests
    SOURCES
        qjstest/test262runner.cpp qjstest/test262runner.h
        tst_ecmascripttests.cpp
    LIBRARIES
        Qt::QmlPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA
        ${test_data}
)

if(ANDROID OR IOS)
    qt_add_resources(tst_ecmascripttests "testExpectations"
        PREFIX
            "/"
        FILES
            "TestExpectations"
    )

    qt_internal_extend_target(tst_ecmascripttests
        DEFINES
            QT_QMLTEST_DATADIR=":/test262"
    )
else()
    qt_internal_extend_target(tst_ecmascripttests
        DEFINES
            QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/test262"
    )
endif()

if(NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(qjstest)
endif()
