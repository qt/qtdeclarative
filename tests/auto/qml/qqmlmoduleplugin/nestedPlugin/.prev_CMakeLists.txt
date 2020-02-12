# Generated from nestedPlugin.pro.

#####################################################################
## nestedPlugin Generic Library:
#####################################################################

qt_add_cmake_library(nestedPlugin
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/AutoTestQmlNestedPluginType"
    SOURCES
        nestedPlugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:nestedPlugin.pro:<TRUE>:
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"
