# Generated from childplugin.pro.

#####################################################################
## ildplugin Module:
#####################################################################

add_qt_plugin(childplugin
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../../imports/org/qtproject/AutoTestQmlPluginType/ChildPlugin"
# special case begin
    TYPE qml_plugin
# special case end
    SOURCES
        childplugin.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:childplugin.pro:<TRUE>:
# CONFIG = "childplugin"
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

# special case begin
file(COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../../imports/org/qtproject/AutoTestQmlPluginType/ChildPlugin"
)
# special case end
