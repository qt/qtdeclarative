# Generated from dummy.pro.

#####################################################################
## Dummy Plugin:
#####################################################################

add_qt_plugin(Dummy
# special case begin
    TYPE qml_plugin
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
# special case end
    SOURCES
        dummy.cpp dummy.h
        dummy_plugin.cpp dummy_plugin.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:dummy.pro:<TRUE>:
# CONFIG = "qt" "plugin" "-debug_and_release_target"
# TEMPLATE = "lib"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:dummy.pro:NOT build_pass AND QT_FEATURE_debug_and_release:
# CONFIG = "release"

#### Keys ignored in scope 3:.:.:dummy.pro:NOT _PRO_FILE_PWD____equals____ss_OUT_PWD:
# COPIES = "cp"
# cp.files = "qmldir" "plugins.qmltypes"
# cp.path = "$$OUT_PWD"

# special case begin
get_target_property(binary_dir Dummy BINARY_DIR)
file(COPY qmldir plugins.qmltypes
    DESTINATION ${binary_dir}
)
