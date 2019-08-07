# Generated from versions.pro.

#####################################################################
## Versions Plugin:
#####################################################################

add_qt_plugin(Versions
# special case begin
    TYPE qml_plugin
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
# special case end
    SOURCES
        versions.cpp versions.h
        versions_plugin.cpp versions_plugin.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:versions.pro:<TRUE>:
# CONFIG = "qt" "plugin" "-debug_and_release_target"
# TEMPLATE = "lib"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:versions.pro:NOT build_pass AND QT_FEATURE_debug_and_release:
# CONFIG = "release"

#### Keys ignored in scope 3:.:.:versions.pro:NOT _PRO_FILE_PWD____equals____ss_OUT_PWD:
# COPIES = "cpqmldir"
# cpqmldir.files = "qmldir" "plugins.qmltypes"
# cpqmldir.path = "$$OUT_PWD"

# special case begin
get_target_property(binary_dir Versions BINARY_DIR)
file(COPY qmldir plugins.qmltypes
    DESTINATION ${binary_dir}
)
# special case end
