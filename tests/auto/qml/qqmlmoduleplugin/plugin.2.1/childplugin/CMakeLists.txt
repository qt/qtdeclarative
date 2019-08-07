# Generated from childplugin.pro.

#####################################################################
## ildplugin Module:
#####################################################################

add_qt_plugin(childplugin.2.1
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../../imports/org/qtproject/AutoTestQmlPluginType.2.1/ChildPlugin"
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
set_target_properties(childplugin.2.1 PROPERTIES OUTPUT_NAME "childplugin")
file(COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../../imports/org/qtproject/AutoTestQmlPluginType.2.1/ChildPlugin"
)
# special case end
