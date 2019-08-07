# Generated from shapestest.pro.

#####################################################################
## shapestest Binary:
#####################################################################

add_qt_executable(shapestest
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
add_qt_resource(shapestest "shapestest"
    PREFIX
        "/shapestest"
    FILES
        shapestest.qml
)


#### Keys ignored in scope 1:.:.:shapestest.pro:<TRUE>:
# OTHER_FILES = "shapestest.qml"
# TEMPLATE = "app"
