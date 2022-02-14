qt_add_qml_module(EllipseShape
    URI Shapes.EllipseShape
    VERSION 1.0
    SOURCES ellipseitem.cpp ellipseitem.h
)

target_link_libraries(EllipseShape
    PRIVATE
        Qt::Quick
)
