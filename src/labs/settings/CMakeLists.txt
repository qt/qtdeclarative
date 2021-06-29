qt_internal_add_qml_module(LabsSettings
    URI "Qt.labs.settings"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET qmlsettingsplugin
    CLASS_NAME QmlSettingsPlugin
    SOURCES
         qqmlsettings.cpp qqmlsettings_p.h
         qqmlsettingsglobal_p.h
    DEFINES
         QT_BUILD_LABSSETTINGS_LIB
    PUBLIC_LIBRARIES
         Qt::Core
         Qt::Qml
     GENERATE_CPP_EXPORTS
     GENERATE_PRIVATE_CPP_EXPORTS
)
