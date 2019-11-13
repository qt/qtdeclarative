# Generated from testlib.pro.

#####################################################################
## qmltestplugin Plugin:
#####################################################################

add_qml_module(qmltestplugin
    URI "QtTest"
    VERSION "1.${CMAKE_PROJECT_VERSION_MINOR}"
    CLASSNAME QTestQmlModule
    DEPENDENCIES
        QtQuick.Window/2.0
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

#### Keys ignored in scope 1:.:.:testlib.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "1.$$QT_MINOR_VERSION"
# OTHER_FILES = "testlib.json"
# QML_FILES = "TestCase.qml" "SignalSpy.qml" "testlogger.js"
# TARGETPATH = "QtTest"

set(qml_files
    "TestCase.qml"
    "SignalSpy.qml"
    "testlogger.js"
)
set_source_files_properties(TestCase.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(SignalSpy.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
# special case begin
set_source_files_properties(testlogger.js PROPERTIES
    QT_QML_SOURCE_INSTALL TRUE
)
# special case end

qt6_target_qml_files(qmltestplugin
    FILES
        ${qml_files}
)
