# Generated from pluginMixed.pro.

#####################################################################
## pluginMixed Plugin:
#####################################################################

add_qt_plugin(pluginMixed
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/AutoTestQmlMixedPluginType"
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

#### Keys ignored in scope 1:.:.:pluginMixed.pro:<TRUE>:
# CONFIG = "plugin"
# IMPORT_FILES = "Foo.qml" "qmldir"
# TEMPLATE = "lib"

# special case begin
file(COPY qmldir Foo.qml
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/AutoTestQmlMixedPluginType"
)
# special case end
