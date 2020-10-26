# Generated from vulkanunderqml.pro.

cmake_minimum_required(VERSION 3.14)
project(vulkanunderqml LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/vulkanunderqml")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

qt_add_executable(vulkanunderqml
    main.cpp
    vulkansquircle.cpp vulkansquircle.h
)
set_target_properties(vulkanunderqml PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(vulkanunderqml PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(vulkanunderqml_resource_files
    "main.qml"
    "squircle.frag.spv"
    "squircle.vert.spv"
)

qt6_add_resources(vulkanunderqml "vulkanunderqml"
    PREFIX
        "/scenegraph/vulkanunderqml"
    FILES
        ${vulkanunderqml_resource_files}
)

install(TARGETS vulkanunderqml
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(vulkanunderqml PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI VulkanUnderQML
)

qt6_qml_type_registration(vulkanunderqml)
