# Generated from nestedPlugin.pro.

#####################################################################
## stedPlugin Module:
#####################################################################

add_qt_plugin(nestedPlugin
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/AutoTestQmlNestedPluginType"
# special case begin
    TYPE qml_plugin
# special case end
    SOURCES
        nestedPlugin.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:nestedPlugin.pro:<TRUE>:
# CONFIG = "nestedPlugin"
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

# special case begin
file(COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/AutoTestQmlNestedPluginType"
)
# special case end TEMPLATE = "lib"
