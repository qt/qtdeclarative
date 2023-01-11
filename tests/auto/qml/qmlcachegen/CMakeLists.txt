# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmlcachegen.pro.

#####################################################################
## tst_qmlcachegen Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmlcachegen
    SOURCES
        tst_qmlcachegen.cpp
        scriptstringprops.h
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::QmlPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

qt6_add_qml_module(tst_qmlcachegen
    URI cachegentest
    RESOURCE_PREFIX "/"
)

set(qmake_workerscripts_test_qml_files
    "data/worker.js"
    "data/worker.qml"
)

qt6_target_qml_sources(tst_qmlcachegen
    PREFIX
        "/workerscripts"
    QML_FILES
        ${qmake_workerscripts_test_qml_files}
)
set_source_files_properties("data/trickypaths.qml"
    PROPERTIES QT_RESOURCE_ALIAS "file name with spaces.qml"
)
set_source_files_properties("data/versionStyleSuffix-1.2-core-yc.qml"
    PROPERTIES QT_RESOURCE_ALIAS "versionStyleSuffix-1.2-core-yc.qml"
)
set_source_files_properties("data/versionStyleSuffix-1.2-more.qml"
    PROPERTIES QT_RESOURCE_ALIAS "versionStyleSuffix-1.2-more.qml"
)
set(trickypaths_qml_files
    "data/trickypaths.qml"
    "data/versionStyleSuffix-1.2-core-yc.qml"
    "data/versionStyleSuffix-1.2-more.qml"
)

qt6_target_qml_sources(tst_qmlcachegen
    PREFIX
        "/directory with spaces"
    QML_FILES
        ${trickypaths_qml_files}
)

set_source_files_properties("data/Retain.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Retain.qml"
)

set(retain_qml_files
    "data/Retain.qml"
)

qt6_target_qml_sources(tst_qmlcachegen
    PREFIX
        "/"
    QML_FILES
        ${retain_qml_files}
)
set_source_files_properties("data/SkipQmlCacheGen.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Skip.qml"
    QT_QML_SKIP_CACHEGEN 1
)
set(skip_qml_files
    "data/SkipQmlCacheGen.qml"
)

qt6_target_qml_sources(tst_qmlcachegen
    PREFIX
        "/not/"
    QML_FILES
        ${skip_qml_files}
)
set(qmake_immediate_qml_files
    "data/Enums.qml"
    "data/componentInItem.qml"
    "data/jsimport.qml"
    "data/jsmoduleimport.qml"
    "data/library.js"
    "data/module.mjs"
    "data/parameterAdjustment.qml"
    "data/script.js"
    "data/script.mjs"
    "data/utils.mjs"
    "data/versionchecks.qml"
    "data/scriptstring.qml"
)

qt6_target_qml_sources(tst_qmlcachegen
    PREFIX
        "/"
    QML_FILES
        ${qmake_immediate_qml_files}
)


## Scopes:
#####################################################################

if(UNIX)
    # Resources:
    set_source_files_properties("data/umlaut.qml"
        PROPERTIES QT_RESOURCE_ALIAS "Bäh.qml"
    )
    set(trickypaths_umlaut_qml_files
        "data/umlaut.qml"
    )

    qt6_target_qml_sources(tst_qmlcachegen
        PREFIX
            "/"
        QML_FILES
            ${trickypaths_umlaut_qml_files}
    )
endif()

# Add those files to the resource system so that they get copied to the data
# directory in the build directory, which is preferred by qFindTestData
# over the one in the source directory where they are always placed.
set(qml_additional_qml_files
    "data/posthocrequired.qml"
    "data/inlineComponentWithId.qml"
)

qt6_target_qml_sources(tst_qmlcachegen
    PREFIX "/"
    QML_FILES ${qml_additional_qml_files}
)

qt_internal_extend_target(tst_qmlcachegen CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qmlcachegen CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

qt_internal_extend_target(tst_qmlcachegen CONDITION CMAKE_CROSSCOMPILING
    DEFINES
        QTEST_CROSS_COMPILED
)
