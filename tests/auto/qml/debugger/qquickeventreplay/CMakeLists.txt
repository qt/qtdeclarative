# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qquickeventreplay Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})


if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquickeventreplay LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qquickeventreplay
    SOURCES
        ../shared/debugutil.cpp ../shared/debugutil_p.h
        ../shared/qqmldebugprocess.cpp ../shared/qqmldebugprocess_p.h
        tst_qquickeventreplay.cpp
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::Qml
        Qt::QmlDebugPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)
