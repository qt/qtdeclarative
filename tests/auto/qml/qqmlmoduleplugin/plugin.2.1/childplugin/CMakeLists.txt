# Generated from childplugin.pro.

#####################################################################
## childplugin Generic Library:
#####################################################################

qt_add_cmake_library(childplugin.2.1 # special case
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../../imports/org/qtproject/AutoTestQmlPluginType.2.1/ChildPlugin"
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
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

# special case begin
qt_autogen_tools_initial_setup(childplugin.2.1)
set_target_properties(childplugin.2.1 PROPERTIES OUTPUT_NAME "childplugin")
file(COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../../imports/org/qtproject/AutoTestQmlPluginType.2.1/ChildPlugin"
)
# special case end
