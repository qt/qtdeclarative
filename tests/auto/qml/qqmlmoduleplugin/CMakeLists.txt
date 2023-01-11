# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmlmoduleplugin.pro.

#####################################################################
## tst_qqmlmoduleplugin Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    imports/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/imports/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlmoduleplugin
    SOURCES
        tst_qqmlmoduleplugin.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::Network
        Qt::QmlPrivate
        Qt::QuickShapesPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

# Resources:
set(qmake_waitingPlugin_resource_files
    "moduleWithWaitingPlugin"
)

qt_internal_add_resource(tst_qqmlmoduleplugin "qmake_waitingPlugin"
    PREFIX
        "/qt-project.org/imports/"
    FILES
        ${qmake_waitingPlugin_resource_files}
)
set(qmake_staticPlugin_resource_files
    "moduleWithStaticPlugin"
)

qt_internal_add_resource(tst_qqmlmoduleplugin "qmake_staticPlugin"
    PREFIX
        "/qt-project.org/imports/"
    FILES
        ${qmake_staticPlugin_resource_files}
)

set_source_files_properties("imports/ModuleFromQrc/badqmldir"
    PROPERTIES QT_RESOURCE_ALIAS "imports/ModuleFromQrc/qmldir"
)

qt_internal_add_resource(tst_qqmlmoduleplugin "moduleFromQrc"
    PREFIX
        "/foo/"
    FILES
        "imports/ModuleFromQrc/badqmldir"
        "imports/ModuleFromQrc/Foo.qml"
)


#### Keys ignored in scope 2:.:.:tst_qqmlmoduleplugin.pro:<TRUE>:
# CONFIG = "testcase" "-app_bundle"
# staticPlugin.files = "moduleWithStaticPlugin"
# staticPlugin.prefix = "/qt-project.org/imports/"
# waitingPlugin.files = "moduleWithWaitingPlugin"
# waitingPlugin.prefix = "/qt-project.org/imports/"

# special case begin
qt_internal_add_resource(tst_qqmlmoduleplugin waitingPlugin
    PREFIX /qt-project.org/imports
    FILES moduleWithWaitingPlugin/qmldir
)
qt_internal_add_resource(tst_qqmlmoduleplugin staticPlugin
    PREFIX /qt-project.org/imports
    FILES moduleWithStaticPlugin/qmldir
)
# special case end

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlmoduleplugin CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlmoduleplugin CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
add_subdirectory(plugin)
add_subdirectory(plugin.2)
add_subdirectory(plugin.2.1)
add_subdirectory(pluginWrongCase)
add_subdirectory(pluginWithQmlFile)
add_subdirectory(pluginMixed)
add_subdirectory(pluginVersion)
add_subdirectory(nestedPlugin)
add_subdirectory(strictModule)
add_subdirectory(strictModule.2)
add_subdirectory(nonstrictModule)
add_subdirectory(preemptiveModule)
add_subdirectory(preemptedStrictModule)
add_subdirectory(invalidNamespaceModule)
add_subdirectory(invalidFirstCommandModule)
add_subdirectory(protectedModule)
add_subdirectory(plugin/childplugin)
add_subdirectory(plugin.2/childplugin)
add_subdirectory(plugin.2.1/childplugin)
add_subdirectory(plugin.2.2)
add_subdirectory(moduleWithQmlSingleton)
add_subdirectory(optionalPlugin)
