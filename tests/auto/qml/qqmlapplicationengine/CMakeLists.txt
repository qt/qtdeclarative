# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmlapplicationengine.pro.

#####################################################################
## tst_qqmlapplicationengine Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlapplicationengine
    SOURCES
        tst_qqmlapplicationengine.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::Network
        Qt::QmlPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

# Resources:
set(tst_qqmlapplicationengine_resource_files
    "data/i18n/qml.qm"
    "data/loadTranslation.qml"
)

qt_internal_add_resource(tst_qqmlapplicationengine "tst_qqmlapplicationengine"
    PREFIX
        "/"
    FILES
        ${tst_qqmlapplicationengine_resource_files}
)

if(QT_BUILD_STANDALONE_TESTS)
    qt_import_qml_plugins(tst_qqmlapplicationengine)
endif()

#### Keys ignored in scope 2:.:.:tst_qqmlapplicationengine.pro:<TRUE>:
# TRANSLATIONS = "data/i18n/qml_ja.ts"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlapplicationengine CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlapplicationengine CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
add_subdirectory(testapp)
add_subdirectory(androidassets)
