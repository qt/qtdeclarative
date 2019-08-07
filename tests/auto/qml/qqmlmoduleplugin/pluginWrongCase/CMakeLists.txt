# Generated from pluginWrongCase.pro.

#####################################################################
## Plugin Plugin:
#####################################################################

add_qt_plugin(Plugin
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/WrongCase"
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

#### Keys ignored in scope 1:.:.:pluginWrongCase.pro:<TRUE>:
# CONFIG = "plugin"
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

# special case begin
file(COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/WrongCase"
)
# special case end
