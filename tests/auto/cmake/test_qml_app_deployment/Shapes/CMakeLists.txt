
qt_add_qml_module(ShapesWrapper
    URI Shapes
    VERSION 1.0
    IMPORTS
        Shapes.FunkyShape/auto
        Shapes.EllipseShape/auto
    RESOURCE_PREFIX /
)

add_subdirectory(FunkyShape)
add_subdirectory(EllipseShape)
