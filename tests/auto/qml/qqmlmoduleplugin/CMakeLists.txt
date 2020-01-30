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

add_qt_test(tst_qqmlmoduleplugin
    SOURCES
        ../../shared/testhttpserver.cpp ../../shared/testhttpserver.h
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlmoduleplugin.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Qml
    TESTDATA ${test_data}
)

# Resources:
set(qmake_waitingPlugin_resource_files
    "moduleWithWaitingPlugin"
)

add_qt_resource(tst_qqmlmoduleplugin "qmake_waitingPlugin"
    PREFIX
        "/qt-project.org/imports/"
    FILES
        ${qmake_waitingPlugin_resource_files}
)
set(qmake_staticPlugin_resource_files
    "moduleWithStaticPlugin"
)

add_qt_resource(tst_qqmlmoduleplugin "qmake_staticPlugin"
    PREFIX
        "/qt-project.org/imports/"
    FILES
        ${qmake_staticPlugin_resource_files}
)


#### Keys ignored in scope 2:.:.:tst_qqmlmoduleplugin.pro:<TRUE>:
# CONFIG = "testcase" "-app_bundle"
# staticPlugin.files = "moduleWithStaticPlugin"
# staticPlugin.prefix = "/qt-project.org/imports/"
# waitingPlugin.files = "moduleWithWaitingPlugin"
# waitingPlugin.prefix = "/qt-project.org/imports/"

# special case begin
add_qt_resource(tst_qqmlmoduleplugin waitingPlugin
    PREFIX /qt-project.org/imports
    FILES moduleWithWaitingPlugin/qmldir
)
add_qt_resource(tst_qqmlmoduleplugin staticPlugin
    PREFIX /qt-project.org/imports
    FILES moduleWithStaticPlugin/qmldir
)
# special case end

## Scopes:
#####################################################################

extend_target(tst_qqmlmoduleplugin CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlmoduleplugin CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
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
