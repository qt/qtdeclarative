qt_add_library(ntp_versioned_module STATIC)
qt_autogen_tools_initial_setup(ntp_versioned_module)

set_source_files_properties(Early.qml PROPERTIES QT_QML_SOURCE_VERSIONS "1.0;2.0")
set_source_files_properties(Late.qml PROPERTIES QT_QML_SOURCE_VERSIONS "2.0")

# Same type name, different files, different property layouts
set_source_files_properties(v1/Shifting.qml PROPERTIES QT_QML_SOURCE_VERSIONS "1.0")
set_source_files_properties(v2/Shifting.qml PROPERTIES QT_QML_SOURCE_VERSIONS "2.0")

qt_add_qml_module(ntp_versioned_module
    URI NtpVersioned
    VERSION 2.0
    QML_FILES
        Early.qml
        Late.qml
        v1/Shifting.qml
        v2/Shifting.qml
    NO_LINT
    __QT_INTERNAL_NO_GENERATE_QMLLS_BUILD_INI
)

target_link_libraries(ntp_versioned_module PRIVATE Qt::Qml)

qt_autogen_tools_initial_setup(ntp_versioned_moduleplugin)
