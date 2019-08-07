# Generated from plugin.2.pro.

#####################################################################
## plugin.2 Plugin:
#####################################################################

add_qt_plugin(plugin.2
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/AutoTestQmlPluginType.2"
# special case begin
    TYPE qml_plugin
# special case end
    SOURCES
        plugin.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:plugin.2.pro:<TRUE>:
# CONFIG = "plugin"
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

# special case begin
set_target_properties(plugin.2 PROPERTIES OUTPUT_NAME "plugin")
file(COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/AutoTestQmlPluginType.2"
)
# special case end
