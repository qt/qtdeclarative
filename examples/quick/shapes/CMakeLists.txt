# Generated from shapes.pro.

cmake_minimum_required(VERSION 3.16)
project(shapes LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/shapes")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

add_subdirectory("../shared" "shared")

qt_add_executable(shapesexample
    main.cpp
)
set_target_properties(shapesexample PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(shapesexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
    shapes_shared
)

qt_add_qml_module(shapesexample
    URI shapes
    VERSION 1.0
    QML_FILES
        "clippedtigers.qml"
        "interactive.qml"
        "item10.qml"
        "item11.qml"
        "item12.qml"
        "item13.qml"
        "item14.qml"
        "item15.qml"
        "item17.qml"
        "item18.qml"
        "item2.qml"
        "item3.qml"
        "item4.qml"
        "item5.qml"
        "item6.qml"
        "item7.qml"
        "item8.qml"
        "item9.qml"
        "main.qml"
        "sampling.qml"
        "shapegallery.qml"
        "tapableTriangle.qml"
        "tiger.qml"
)

install(TARGETS shapesexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
