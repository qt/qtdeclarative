# Generated from Slow.pro.

#####################################################################
## ow Module:
#####################################################################

add_qt_plugin(Slow
    # special case begin
    TYPE qml_plugin
    # special case end
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../Slow"
    SOURCES
        plugin.cpp plugin.h
        slow.cpp slow.h
    INCLUDE_DIRECTORIES
        .
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:Slow.pro:<TRUE>:
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

# special case begin
file (COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../Slow"
)
# special case end
