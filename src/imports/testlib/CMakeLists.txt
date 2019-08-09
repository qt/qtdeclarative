# Generated from testlib.pro.

#####################################################################
## qmltestplugin Plugin:
#####################################################################

add_qml_module(qmltestplugin
    CPP_PLUGIN
    URI "QtTest"
    VERSION "1.${CMAKE_PROJECT_VERSION_MINOR}"
    RESOURCE_PREFIX "/qt-project.org/imports"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
        Qt::QuickTestPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickTest
        Qt::Test
)

set(qml_files
    TestCase.qml
    SignalSpy.qml
    testlogger.js
)

add_qt_resource(qmltestplugin qmake_QtTest
    PREFIX
        "/qt-project.org/imports/QtTest"
    FILES
        ${qml_files}
)

qt_install_qml_files(qmltestplugin
    FILES ${qml_files}
)


#### Keys ignored in scope 1:.:.:testlib.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "1.$$QT_MINOR_VERSION"
# OTHER_FILES = "testlib.json"
# _LOADED = "qml_plugin"
