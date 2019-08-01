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
        Qt::Qml
        Qt::Quick
        Qt::QuickTest
        Qt::Test
)

# QML Files
add_qt_resource(qmltestplugin qmake_QtTest
    PREFIX
        "/qt-project.org/imports/QtTest"
    FILES
        TestCase.qml
        SignalSpy.qml
        testlogger.js
)

#### Keys ignored in scope 1:.:.:testlib.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "1.$$QT_MINOR_VERSION"
# OTHER_FILES = "testlib.json"
# _LOADED = "qml_plugin"
