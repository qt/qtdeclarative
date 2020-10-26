# Generated from chapter6-plugins.pro.

cmake_minimum_required(VERSION 3.14)
project(chapter6-plugins LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/tutorials/extending-qml/chapter6-plugins")

find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

qt_add_executable(chapter6-plugins
    main.cpp
)
set_target_properties(chapter6-plugins PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(chapter6-plugins PUBLIC
    Qt::Qml
    Qt::Quick
)


# Resources:
set(app_resource_files
    "app.qml"
)

qt6_add_resources(chapter6-plugins "app"
    PREFIX
        "/"
    FILES
        ${app_resource_files}
)

install(TARGETS chapter6-plugins
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

add_subdirectory(import)
