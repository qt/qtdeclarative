# Generated from pluginWithQmlFile.pro.

#####################################################################
## pluginWithQmlFile Plugin:
#####################################################################

add_qt_plugin(pluginWithQmlFile
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/AutoTestPluginWithQmlFile"
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

#### Keys ignored in scope 1:.:.:pluginWithQmlFile.pro:<TRUE>:
# CONFIG = "plugin"
# IMPORT_FILES = "qmldir" "MyQmlFile.qml"
# TEMPLATE = "lib"

# special case begin
file(COPY qmldir MyQmlFile.qml
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/AutoTestPluginWithQmlFile"
)
# special case end
