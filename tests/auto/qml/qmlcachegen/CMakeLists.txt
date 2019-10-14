# Generated from qmlcachegen.pro.

#####################################################################
## tst_qmlcachegen Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qmlcachegen
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qmlcachegen.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
    TESTDATA ${test_data}
)

# Resources:
set(qmake_workerscripts_test_resource_files
    "data/worker.js"
    "data/worker.qml"
)

add_qt_resource(tst_qmlcachegen "qmake_workerscripts_test"
    PREFIX
        "/workerscripts"
    FILES
        ${qmake_workerscripts_test_resource_files}
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
set(trickypaths_resource_files
    "data/trickypaths.qml"
    "data/versionStyleSuffix-1.2-core-yc.qml"
    "data/versionStyleSuffix-1.2-more.qml"
)

add_qt_resource(tst_qmlcachegen "trickypaths"
    PREFIX
        "/directory with spaces"
    FILES
        ${trickypaths_resource_files}
)
set_source_files_properties("data/Retain.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Retain.qml"
)
set(retain_resource_files
    "Retain.qml"
)

add_qt_resource(tst_qmlcachegen "retain"
    PREFIX
        "/"
    BASE
        "data"
    FILES
        ${retain_resource_files}
)
set(qmake_immediate_resource_files
    "data/Enums.qml"
    "data/componentInItem.qml"
    "data/jsimport.qml"
    "data/jsmoduleimport.qml"
    "data/library.js"
    "data/module.mjs"
    "data/script.js"
    "data/script.mjs"
    "data/utils.mjs"
    "data/versionchecks.qml"
)

add_qt_resource(tst_qmlcachegen "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


## Scopes:
#####################################################################

# Resources:
set_source_files_properties("data/umlaut.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Bäh.qml"
)
set(trickypaths_umlaut_resource_files
    "data/umlaut.qml"
)

add_qt_resource(tst_qmlcachegen "trickypaths_umlaut"
    PREFIX
        "/"
    FILES
        ${trickypaths_umlaut_resource_files}
)


extend_target(tst_qmlcachegen CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qmlcachegen CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
