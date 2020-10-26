# Generated from metalunderqml.pro.

cmake_minimum_required(VERSION 3.14)
project(metalunderqml LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/metalunderqml")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

qt_add_executable(metalunderqml
    main.cpp
    metalsquircle.h metalsquircle.mm
)
set_target_properties(metalunderqml PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(metalunderqml PUBLIC
    "-framework Metal"
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(metalunderqml_resource_files
    "main.qml"
    "squircle.frag"
    "squircle.vert"
)

qt6_add_resources(metalunderqml "metalunderqml"
    PREFIX
        "/scenegraph/metalunderqml"
    FILES
        ${metalunderqml_resource_files}
)

if(MACOS)
    target_link_libraries(metalunderqml PUBLIC
        "-framework AppKit"
    )
endif()

install(TARGETS metalunderqml
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(metalunderqml PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI MetalUnderQML
)

qt6_qml_type_registration(metalunderqml)
