cmake_minimum_required(VERSION 3.16)
project(custommaterial_declarative LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/custommaterial")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick ShaderTools)

qt_add_executable(custommaterial_declarative
    customitem.cpp customitem.h
    main.cpp
)

set_target_properties(custommaterial_declarative PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(custommaterial_declarative PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

qt_add_qml_module(custommaterial_declarative
    URI ExampleCustomMaterial
    VERSION 1.0
    QML_FILES
        main.qml
    RESOURCE_PREFIX /scenegraph/custommaterial
    NO_RESOURCE_TARGET_PATH
)

qt6_add_shaders(custommaterial_declarative "shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/scenegraph/custommaterial"
    FILES
        "shaders/mandelbrot.vert"
        "shaders/mandelbrot.frag"
)

install(TARGETS custommaterial_declarative
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
