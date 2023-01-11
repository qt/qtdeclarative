# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmljsscope
    SOURCES
        tst_qqmljsscope.cpp
    LIBRARIES
        Qt::QmlPrivate
        Qt::QmlCompilerPrivate
        Qt::QuickTestUtilsPrivate
        Qt::Gui

        qqmljsscope_test_module
        qqmljsscope_test_moduleplugin

        qualified_names_test_module
        qualified_names_test_moduleplugin
    TESTDATA
        ${test_data}
)

# we need the following QML modules implicitly (since we import them):
add_dependencies(tst_qqmljsscope Qt::Quick)
if(QT_FEATURE_qml_delegate_model AND TARGET Qt::LabsQmlModels)
    add_dependencies(tst_qqmljsscope Qt::LabsQmlModels)
    target_compile_definitions(tst_qqmljsscope PRIVATE LABS_QML_MODELS_PRESENT)
endif()

qt_internal_extend_target(tst_qqmljsscope CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmljsscope CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

add_subdirectory(QQmlJSScopeTests)
add_subdirectory(QualifiedNamesTests)
