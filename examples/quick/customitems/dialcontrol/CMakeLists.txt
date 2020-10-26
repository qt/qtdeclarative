# Generated from dialcontrol.pro.

cmake_minimum_required(VERSION 3.14)
project(dialcontrol LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/customitems/dialcontrol")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(dialcontrol
    main.cpp
)
set_target_properties(dialcontrol PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(dialcontrol PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(dialcontrol_resource_files
    "content/Dial.qml"
    "content/QuitButton.qml"
    "content/background.png"
    "content/needle.png"
    "content/needle_shadow.png"
    "content/overlay.png"
    "content/quit.png"
    "dialcontrol.qml"
)

qt6_add_resources(dialcontrol "dialcontrol"
    PREFIX
        "/"
    FILES
        ${dialcontrol_resource_files}
)

install(TARGETS dialcontrol
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
