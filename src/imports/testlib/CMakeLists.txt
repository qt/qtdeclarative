# Generated from testlib.pro.

#####################################################################
## qmltestplugin Plugin:
#####################################################################

add_qml_module(qmltestplugin
    TARGET_PATH "QtTest"
    IMPORT_VERSION "1.${CMAKE_PROJECT_VERSION_MINOR}"
    QML_FILES
        TestCase.qml
        SignalSpy.qml
        testlogger.js
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

#### Keys ignored in scope 1:.:.:testlib.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "1.$$QT_MINOR_VERSION"
# OTHER_FILES = "testlib.json"
# QML_FILES = "TestCase.qml" "SignalSpy.qml" "testlogger.js"
# TARGETPATH = "QtTest"
# _LOADED = "qml_plugin"
