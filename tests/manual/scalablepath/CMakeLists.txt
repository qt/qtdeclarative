# Generated from scalablepath.pro.

#####################################################################
## scalablepath Binary:
#####################################################################

add_qt_executable(scalablepath
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
add_qt_resource(scalablepath "qml"
    PREFIX
        "/"
    FILES
        ShapeTestScale.qml
        main.qml
)


#### Keys ignored in scope 1:.:.:scalablepath.pro:<TRUE>:
# CONFIG = "c++11"
# TEMPLATE = "app"
