# Generated from rendercontrol_opengl.pro.

cmake_minimum_required(VERSION 3.14)
project(rendercontrol_opengl LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/rendercontrol/rendercontrol_opengl")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS OpenGL)

qt_add_executable(rendercontrol_opengl
    cuberenderer.cpp cuberenderer.h
    main.cpp
    window_singlethreaded.cpp window_singlethreaded.h
)
set_target_properties(rendercontrol_opengl PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(rendercontrol_opengl PUBLIC
    Qt::Core
    Qt::Gui
    Qt::OpenGL
    Qt::Qml
    Qt::Quick
)


# Resources:
set(rendercontrol_resource_files
    "demo.qml"
)

qt6_add_resources(rendercontrol_opengl "rendercontrol"
    PREFIX
        "/rendercontrol"
    FILES
        ${rendercontrol_resource_files}
)

install(TARGETS rendercontrol_opengl
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
