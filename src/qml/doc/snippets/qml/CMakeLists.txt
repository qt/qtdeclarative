qt_add_library(extra_module STATIC)
qt_add_qml_module(extra_module
    URI "ExtraModule"
    VERSION 1.0
    QML_FILES
        Extra.qml
    SOURCES
        extrathing.cpp extrathing.h
)

add_subdirectory(ExtraModule)
