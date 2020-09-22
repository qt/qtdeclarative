# Generated from scalablepath.pro.

#####################################################################
## scalablepath Binary:
#####################################################################

qt_internal_add_manual_test(scalablepath
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

qt_internal_add_resource(scalablepath "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


#### Keys ignored in scope 1:.:.:scalablepath.pro:<TRUE>:
# TEMPLATE = "app"
