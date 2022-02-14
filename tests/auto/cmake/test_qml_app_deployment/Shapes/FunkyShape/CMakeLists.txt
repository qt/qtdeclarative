qt_add_qml_module(FunkyShape
    URI Shapes.FunkyShape
    VERSION 1.0
    QML_FILES FunkyItemQml.qml
    SOURCES funkyitem.cpp funkyitem.h
)
target_link_libraries(FunkyShape
    PRIVATE
        Qt::Quick
)
