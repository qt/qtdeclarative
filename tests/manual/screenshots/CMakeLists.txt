# Generated from screenshots.pro.

#####################################################################
## screenshots Binary:
#####################################################################

qt_internal_add_manual_test(screenshots
    GUI
    SOURCES
        screenshots.cpp
    DEFINES
        SNIPPETS_DIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../../src/imports/controls/doc/snippets\\\"
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Widgets
)

# Resources:
set(qmake_immediate_resource_files
    "qtquickcontrols2.conf"
    "screenshots.qml"
)

qt_internal_add_resource(screenshots "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


#### Keys ignored in scope 1:.:.:screenshots.pro:<TRUE>:
# TEMPLATE = "app"
