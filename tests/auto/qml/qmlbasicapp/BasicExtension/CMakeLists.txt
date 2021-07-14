qt_add_library(additional_qml_module STATIC)
set_property(TARGET additional_qml_module PROPERTY AUTOMOC ON)

qt_add_plugin(additional_qml_module_plugin STATIC TYPE qml_plugin CLASS_NAME BasicExtension)
set_property(TARGET additional_qml_module_plugin PROPERTY AUTOMOC ON)

qt6_add_qml_module(additional_qml_module
    PLUGIN_TARGET additional_qml_module_plugin
    CLASS_NAME "BasicExtension"
    VERSION 1.0
    URI "BasicExtension"
    QML_FILES
        Extension.qml
)
