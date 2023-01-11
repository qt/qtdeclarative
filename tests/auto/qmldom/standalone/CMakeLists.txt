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

set(QMLDOM_EXTERNAL_BUILD OFF CACHE BOOL "If the build is against an external Qt, and not tested inside a build of this Qt" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/qmldom/standalone qmldomlib)

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
elseif (MINGW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
endif()

qt_internal_add_test(tst_standalone
    SOURCES
        tst_standalone.cpp
    DEFINES
        QT_DEPRECATED_WARNINGS
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/../domdata"
    LIBRARIES
        Qt::Core
        Qt::QmlPrivate
        qmldomlib
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_standalone CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/domdata"
)
