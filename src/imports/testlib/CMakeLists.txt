# Generated from testlib.pro.

#####################################################################
## qmltestplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qmltestplugin
    URI "QtTest"
    VERSION "${CMAKE_PROJECT_VERSION}"
    CLASSNAME QTestQmlModule
    DEPENDENCIES
        QtQuick.Window/2.0
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::QuickTestPrivate
)

#### Keys ignored in scope 1:.:.:testlib.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_FILES = "TestCase.qml" "SignalSpy.qml" "testlogger.js"
# QML_IMPORT_VERSION = "$$QT_VERSION"
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
set_source_files_properties(testlogger.js PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)

qt6_target_qml_files(qmltestplugin
    FILES
        ${qml_files}
)
