# Generated from plugin.pro.

#####################################################################
## plugin Plugin:
#####################################################################

add_qt_plugin(plugin
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/AutoTestQmlPluginType"
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

#### Keys ignored in scope 1:.:.:plugin.pro:<TRUE>:
# CONFIG = "plugin"
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

# special case begin
file(COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/AutoTestQmlPluginType"
)
# special case end
