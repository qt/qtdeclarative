# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmltranslation.pro.

#####################################################################
## tst_qqmltranslation Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmltranslation
    SOURCES
        tst_qqmltranslation.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QmlModelsPrivate
        Qt::Quick
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

# Resources:
set(translation_resource_files
    "data/qml_fr.qm"
    "data/translation.qml"
)

# special case begin
set_source_files_properties(
    data/translation.qml
    PROPERTIES
    QT_SKIP_QUICKCOMPILER 1
)
# special case end

# On platforms where TESTDATA is embedded into resources (Android, iOS and
# INTEGRITY), the QT_RESOURCE_ALIAS property was previously set when processing
# TESTDATA, so we need to reset it. Otherwise the wrong alias will be written
# to introspect.qrc, and the related test will fail.
set_source_files_properties(${translation_resource_files} PROPERTIES
        QT_RESOURCE_ALIAS "NOTFOUND")

qt_internal_add_resource(tst_qqmltranslation "translation"
    PREFIX
        "/"
    BASE
        "data"
    FILES
        ${translation_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmltranslation CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmltranslation CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
