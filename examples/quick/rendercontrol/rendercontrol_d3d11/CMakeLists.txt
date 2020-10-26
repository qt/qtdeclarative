# Generated from rendercontrol_d3d11.pro.

cmake_minimum_required(VERSION 3.14)
project(rendercontrol_d3d11 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/rendercontrol/rendercontrol_d3d11")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(rendercontrol_d3d11
    engine.cpp engine.h
    main.cpp
    window.cpp window.h
)
set_target_properties(rendercontrol_d3d11 PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(rendercontrol_d3d11 PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
    d3d11
    dxgi
    dxguid
)


# Resources:
set(rendercontrol_resource_files
    "demo.qml"
)

qt6_add_resources(rendercontrol_d3d11 "rendercontrol"
    PREFIX
        "/rendercontrol"
    FILES
        ${rendercontrol_resource_files}
)

install(TARGETS rendercontrol_d3d11
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
