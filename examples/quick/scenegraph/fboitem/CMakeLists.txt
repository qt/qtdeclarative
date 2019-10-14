# Generated from fboitem.pro.

cmake_minimum_required(VERSION 3.14)
project(fboitem LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/quick/scenegraph/fboitem")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(fboitem
    ../shared/logorenderer.cpp ../shared/logorenderer.h
    fboinsgrenderer.cpp fboinsgrenderer.h
    main.cpp
)
target_include_directories(fboitem PUBLIC
    ../shared
)

target_link_libraries(fboitem PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(fboitem_resource_files
    "main.qml"
)

qt6_add_resources(fboitem "fboitem"
    PREFIX
        "/scenegraph/fboitem"
    FILES
        ${fboitem_resource_files}
)

install(TARGETS fboitem
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
