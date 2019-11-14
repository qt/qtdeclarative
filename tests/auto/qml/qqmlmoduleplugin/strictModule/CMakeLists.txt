# Generated from strictModule.pro.

#####################################################################
## strictModule Generic Library:
#####################################################################

add_cmake_library(strictModule
    MODULE
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/StrictModule"
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

#### Keys ignored in scope 1:.:.:strictModule.pro:<TRUE>:
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

qt_autogen_tools_initial_setup(strictModule)
# special case begin
file(COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/StrictModule"
)
# special case end
