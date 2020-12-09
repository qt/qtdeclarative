# Generated from tooling.pro.
# special case skip regeneration

set(qml_files
    "Component.qml"
    "Enum.qml"
    "Member.qml"
    "Method.qml"
    "Module.qml"
    "Parameter.qml"
    "Property.qml"
    "Signal.qml"
)

qt_internal_add_qml_module(quicktooling
    URI "QtQuick.tooling"
    VERSION "${CMAKE_PROJECT_VERSION}"
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    PAST_MAJOR_VERSIONS 1
)

set_source_files_properties(${qml_files} PROPERTIES
    QT_QML_SOURCE_VERSION "1.2;6.0"
)

qt6_target_qml_files(quicktooling
    FILES
        ${qml_files}
)

