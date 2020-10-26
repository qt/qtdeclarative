# Generated from maskedmousearea.pro.

cmake_minimum_required(VERSION 3.14)
project(maskedmousearea LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/customitems/maskedmousearea")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(maskedmousearea
    main.cpp
    maskedmousearea.cpp maskedmousearea.h
)
set_target_properties(maskedmousearea PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(maskedmousearea PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(maskedmousearea_resource_files
    "images/cloud_1.png"
    "images/cloud_2.png"
    "images/moon.png"
    "maskedmousearea.qml"
)

qt6_add_resources(maskedmousearea "maskedmousearea"
    PREFIX
        "/customitems/maskedmousearea"
    FILES
        ${maskedmousearea_resource_files}
)

install(TARGETS maskedmousearea
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(maskedmousearea PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI Example
)

qt6_qml_type_registration(maskedmousearea)
