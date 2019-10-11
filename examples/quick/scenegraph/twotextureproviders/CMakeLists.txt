# Generated from twotextureproviders.pro.

cmake_minimum_required(VERSION 3.14)
project(twotextureproviders LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/quick/scenegraph/twotextureproviders")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(twotextureproviders
    main.cpp
    xorblender.cpp xorblender.h
)
target_link_libraries(twotextureproviders PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(twotextureproviders_resource_files
    "main.qml"
)

qt6_add_resources(twotextureproviders "twotextureproviders"
    PREFIX
        "/scenegraph/twotextureproviders"
    FILES
        ${twotextureproviders_resource_files}
)

install(TARGETS twotextureproviders
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
