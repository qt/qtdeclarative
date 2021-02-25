# Generated from viewinqwidget.pro.

#####################################################################
## viewinqwidget Binary:
#####################################################################

qt_internal_add_manual_test(viewinqwidget
    GUI
    SOURCES
        main.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/\\\"
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickWidgets
        Qt::Widgets
)

# Resources:
set(viewinqwidget_resource_files
    "main.qml"
)

qt_internal_add_resource(viewinqwidget "viewinqwidget"
    PREFIX
        "/"
    FILES
        ${viewinqwidget_resource_files}
)


#### Keys ignored in scope 1:.:.:viewinqwidget.pro:<TRUE>:
# OTHER_FILES = "main.qml"
