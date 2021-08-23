# Generated from embeddedinwidgets.pro.

cmake_minimum_required(VERSION 3.16)
project(embeddedinwidgets LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/embeddedinwidgets")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS ShaderTools)

qt_add_executable(embeddedinwidgets
    main.cpp
)
set_target_properties(embeddedinwidgets PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(embeddedinwidgets PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Widgets
)


# Resources:
set(embeddedinwidgets_resource_files
    "TextBox.qml"
    "main.qml"
)

qt6_add_resources(embeddedinwidgets "embeddedinwidgets"
    PREFIX
        "/embeddedinwidgets"
    FILES
        ${embeddedinwidgets_resource_files}
)

qt6_add_shaders(embeddedinwidgets "shaders"
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/embeddedinwidgets"
    FILES
        "reflect.frag"
)

install(TARGETS embeddedinwidgets
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
