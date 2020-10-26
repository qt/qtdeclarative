# Generated from keyinteraction.pro.

cmake_minimum_required(VERSION 3.14)
project(keyinteraction LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/keyinteraction")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(keyinteraction
    main.cpp
)
set_target_properties(keyinteraction PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(keyinteraction PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(keyinteraction_resource_files
    "focus/Core/ContextMenu.qml"
    "focus/Core/GridMenu.qml"
    "focus/Core/ListMenu.qml"
    "focus/Core/ListViewDelegate.qml"
    "focus/Core/TabMenu.qml"
    "focus/Core/images/arrow.png"
    "focus/Core/images/qt-logo.png"
    "focus/focus.qml"
    "keyinteraction.qml"
)

qt6_add_resources(keyinteraction "keyinteraction"
    PREFIX
        "/keyinteraction"
    FILES
        ${keyinteraction_resource_files}
)

install(TARGETS keyinteraction
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
