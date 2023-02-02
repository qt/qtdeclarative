# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmlcomponent.pro.

#####################################################################
## tst_qqmlcomponent Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})


if(TARGET Qt::QuickControls2)
    list(APPEND test_data data/dummyImportControls.qml)
endif()

qt_internal_add_test(tst_qqmlcomponent
    SOURCES
        tst_qqmlcomponent.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

if(TARGET Qt::QuickControls2)
    target_compile_definitions(tst_qqmlcomponent PRIVATE HAS_CONTROLS)
endif()

qt_policy(SET QTP0001 NEW)

qt_add_qml_module(
    tst_qqmlcomponent
    URI test
    QML_FILES
        "data/TestComponentWithIC.qml"
        "data/withAot.qml"
)

# intentionally NOT using qt_add_qml_module
qt_add_resources(tst_qqmlcomponent plainqml
    PREFIX /qt/qml
    FILES
        plainqml/qmldir
        plainqml/Plain.qml
)

# Resources:
set(qmake_immediate_resource_files
    "data/QtObjectComponent.qml"
)

qt_internal_add_resource(tst_qqmlcomponent "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlcomponent CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlcomponent CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
