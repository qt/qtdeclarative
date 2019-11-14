# Generated from protectedModule.pro.

#####################################################################
## protectedModule Generic Library:
#####################################################################

add_cmake_library(protectedModule
    MODULE
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/ProtectedModule"
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

#### Keys ignored in scope 1:.:.:protectedModule.pro:<TRUE>:
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

qt_autogen_tools_initial_setup(protectedModule)
# special case begin
file(COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/ProtectedModule"
)
# special case end
