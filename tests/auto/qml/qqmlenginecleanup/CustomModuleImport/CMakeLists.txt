# Generated from CustomModule.pro.

#####################################################################
## stomModule Module:
#####################################################################

add_qt_plugin(CustomModule
    # special case begin
    TYPE qml_plugin
    # special case end
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../CustomModule"
    SOURCES
        moduleplugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
add_qt_resource(CustomModule "moduleplugin"
    PREFIX
        "/"
    FILES
        ModuleType.qml
)


#### Keys ignored in scope 1:.:.:CustomModule.pro:<TRUE>:
# CONFIG = "qtquickcompiler"
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

# special case begin
file(COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../CustomModule"
)
# special case end
