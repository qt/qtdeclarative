# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_accessibility Test:
#####################################################################

qt_internal_add_benchmark(tst_bench_accessibility
    SOURCES
        tst_bench_accessibility.cpp
    LIBRARIES
        Qt::Gui
        Qt::QuickTest
        Qt::Qml
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickPrivate
        Qt::QuickTest
        Qt::QuickTestUtilsPrivate
        Qt::TestPrivate

)

qt_internal_extend_target(tst_bench_accessibility CONDITION NOT ANDROID AND NOT IOS
DEFINES
    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

qt_internal_extend_target(tst_bench_accessibility CONDITION ANDROID OR IOS
DEFINES
    QUICK_TEST_SOURCE_DIR=":/data"
)
