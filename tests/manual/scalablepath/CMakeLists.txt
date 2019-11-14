# Generated from scalablepath.pro.

#####################################################################
## scalablepath Binary:
#####################################################################

add_qt_manual_test(scalablepath
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
set(qml_resource_files
    "ShapeTestScale.qml"
    "main.qml"
)

add_qt_resource(scalablepath "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


#### Keys ignored in scope 1:.:.:scalablepath.pro:<TRUE>:
# TEMPLATE = "app"
