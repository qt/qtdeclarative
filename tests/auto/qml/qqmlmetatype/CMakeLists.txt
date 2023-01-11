# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmlmetatype.pro.

#####################################################################
## tst_qqmlmetatype Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlmetatype
    SOURCES
        tst_qqmlmetatype.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

# Resources:
set(qmake_qmlfiles_resource_files
    "data/CompositeType.qml"
)

qt_internal_add_resource(tst_qqmlmetatype "qmake_qmlfiles"
    PREFIX
        "/tstqqmlmetatype"
    FILES
        ${qmake_qmlfiles_resource_files}
)
set(qmake_qmldirresource_resource_files
    "data/Components/App.qml"
    "data/Components/qmldir"
    "data/enumsInRecursiveImport.qml"
)

qt_internal_add_resource(tst_qqmlmetatype "qmake_qmldirresource"
    PREFIX
        "/"
    FILES
        ${qmake_qmldirresource_resource_files}
)

if(QT_BUILD_STANDALONE_TESTS)
    qt_import_qml_plugins(tst_qqmlmetatype)
endif()

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlmetatype CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlmetatype CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
