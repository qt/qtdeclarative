# Generated from quick-accessibility.pro.

cmake_minimum_required(VERSION 3.14)
project(quick-accessibility LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

add_qt_gui_executable(quick-accessibility
    main.cpp
)
target_link_libraries(quick-accessibility PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

# Resources:
set(accessibility_resource_files
    "accessibility.qml"
    "content/Button.qml"
    "content/Checkbox.qml"
    "content/Slider.qml"
)

QT6_ADD_RESOURCES(quick-accessibility "accessibility"
    PREFIX
        "/accessibility"
    FILES
        ${accessibility_resource_files}
)


install(TARGETS quick-accessibility
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
