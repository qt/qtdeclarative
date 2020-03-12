# Generated from settings.pro.

#####################################################################
## qmlsettingsplugin Plugin:
#####################################################################

qt_add_qml_module(qmlsettingsplugin
    URI "Qt.labs.settings"
    VERSION "${CMAKE_PROJECT_VERSION}"
    CLASSNAME QmlSettingsPlugin
    SKIP_TYPE_REGISTRATION
    GENERATE_QMLTYPES
    SOURCES
        plugin.cpp
        qqmlsettings.cpp qqmlsettings_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:settings.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "Qt/labs/settings"
