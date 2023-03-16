# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmllint.pro.

#####################################################################
## tst_qmllint Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_library(LintPlugin STATIC lintplugin.h lintplugin.cpp)
target_link_libraries(LintPlugin PRIVATE Qt::QmlCompilerPrivate)
qt_autogen_tools_initial_setup(LintPlugin)
target_compile_definitions(LintPlugin PRIVATE QT_STATICPLUGIN)

qt_internal_add_test(tst_qmllint
    SOURCES
        tst_qmllint.cpp
    LIBRARIES
        Qt::Gui
        Qt::QuickTestUtilsPrivate
        Qt::QmlCompilerPrivate
        LintPlugin
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qmllint CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qmllint CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

if (TARGET qmllint)
    add_dependencies(tst_qmllint Qt::qmllint)
endif()

if (TARGET qmljsrootgen)
    qt_internal_extend_target(tst_qmllint
        DEFINES
            QT_QMLJSROOTGEN_PRESENT
    )

    add_dependencies(tst_qmllint Qt::qmljsrootgen)
endif()
