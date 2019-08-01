# Generated from settings.pro.

#####################################################################
## qmlsettingsplugin Plugin:
#####################################################################

add_qml_module(qmlsettingsplugin
    CPP_PLUGIN
    URI "Qt.labs.settings"
    VERSION "1.1"
    RESOURCE_PREFIX "/qt-project.org/imports"
    SOURCES
        plugin.cpp
        qqmlsettings.cpp qqmlsettings_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:settings.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "1.1"
# TARGETPATH = "Qt/labs/settings"
# _LOADED = "qml_plugin"
