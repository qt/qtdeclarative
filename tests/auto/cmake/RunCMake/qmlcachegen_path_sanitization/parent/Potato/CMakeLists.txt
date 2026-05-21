qt_add_library(Potato STATIC)

# A QML file generated into the binary directory.
# Checks that relative paths are computed from the current binary dir and not the source dir.
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/BuildFoo.qml"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/BuildFoo.template.qml"
        "${CMAKE_CURRENT_BINARY_DIR}/BuildFoo.qml"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/BuildFoo.template.qml"
    VERBATIM
)

set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/BuildFoo.qml"
    PROPERTIES QT_RESOURCE_ALIAS BuildFoo.qml)

# A QML file generated outside the current binary directory (one directory up).
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/../OuterBuildFoo.qml"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/OuterBuildFoo.template.qml"
        "${CMAKE_CURRENT_BINARY_DIR}/../OuterBuildFoo.qml"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/OuterBuildFoo.template.qml"
    VERBATIM
)

set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/../OuterBuildFoo.qml"
    PROPERTIES QT_RESOURCE_ALIAS OuterBuildFoo.qml)

# A QML file one directory up from the source dir: relative path contains a single "..".
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/../ParentFoo.qml"
    PROPERTIES QT_RESOURCE_ALIAS ParentFoo.qml)

# A QML file two directories up from the source dir: relative path contains "../..".
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/../../GrandFoo.qml"
    PROPERTIES QT_RESOURCE_ALIAS GrandFoo.qml)

# A QML file in a sibling subdirectory of the module: relative path has ".."
# followed by a nested subdir name.
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/../sibling/SiblingFoo.qml"
    PROPERTIES QT_RESOURCE_ALIAS SiblingFoo.qml)

# A QML file in a sibling subdirectory of the module whose name contains a space.
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/../side dir/SpacedFoo.qml"
    PROPERTIES QT_RESOURCE_ALIAS SpacedFoo.qml)

qt_add_qml_module(Potato
    URI Potato
    QML_FILES
        Main.qml
        "${CMAKE_CURRENT_SOURCE_DIR}/../ParentFoo.qml"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../GrandFoo.qml"
        "${CMAKE_CURRENT_SOURCE_DIR}/../sibling/SiblingFoo.qml"
        "${CMAKE_CURRENT_SOURCE_DIR}/../side dir/SpacedFoo.qml"
        "${CMAKE_CURRENT_BINARY_DIR}/BuildFoo.qml"
        "${CMAKE_CURRENT_BINARY_DIR}/../OuterBuildFoo.qml"
)
