# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmlpromise.pro.

#####################################################################
## tst_qqmlpromise Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qqmlpromise LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlpromise
    SOURCES
        tst_qqmlpromise.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

if(QT_BUILD_STANDALONE_TESTS)
    qt_import_qml_plugins(tst_qqmlpromise)
endif()

#### Keys ignored in scope 1:.:.:qqmlpromise.pro:<TRUE>:
# DISTFILES = "data/then-fulfilled-non-callable.qml" "data/then-reject-non-callable.qml" "data/then-resolve-multiple-then.qml" "data/then-resolve-chaining.qml" "data/then-reject-chaining.qml" "data/promise-resolve-with-value.qml" "data/promise-resolve-with-promise.qml" "data/promise-reject-with-value.qml" "data/promise-executor-resolve.qml" "data/promise-get-length.qml" "data/promise-executor-reject.qml" "data/promise-reject-catch.qml" "data/promise-async-resolve-with-value.qml" "data/promise-async-reject-with-value.qml" "data/promise-all-resolve.qml" "data/promise-all-empty-input.qml" "data/promise-resolve-with-array.qml" "data/promise-all-reject-reject-is-last.qml" "data/promise-all-reject-reject-is-mid.qml" "data/promise-race-resolve-1st.qml" "data/promise-race-empty-input.qml" "data/promise-race-resolve-2nd.qml" "data/promise-race-resolve-1st-in-executor-function.qml" "data/promise-resolve-is-a-function.qml" "data/promise-resolve-function-length.qml" "data/promise-all-invoke-then-method.qml" "data/promise-resolve-with-empty.qml" "data/promise-executor-throw-exception.qml" "data/promise-executor-function-extensible.qml" "data/promise-all-noniterable-input.qml"
# OTHER_FILES = <EMPTY>

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlpromise CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlpromise CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
