# Generated from settings.pro.

#####################################################################
## qmlsettingsplugin Plugin:
#####################################################################

add_qml_module(qmlsettingsplugin
    TARGET_PATH "Qt/labs/settings"
    IMPORT_VERSION "1.1"
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
