# Generated from invalidFirstCommandModule.pro.

#####################################################################
## invalidFirstCommandModule Plugin:
#####################################################################

add_qt_plugin(invalidFirstCommandModule
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/InvalidFirstCommandModule"
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

#### Keys ignored in scope 1:.:.:invalidFirstCommandModule.pro:<TRUE>:
# CONFIG = "plugin"
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

# special case begin
file(COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/InvalidFirstCommandModule"
)
# special case end
