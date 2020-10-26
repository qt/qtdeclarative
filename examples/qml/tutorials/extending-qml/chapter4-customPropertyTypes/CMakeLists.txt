# Generated from chapter4-customPropertyTypes.pro.

cmake_minimum_required(VERSION 3.14)
project(chapter4-customPropertyTypes LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/tutorials/extending-qml/chapter4-customPropertyTypes")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

qt_add_executable(chapter4-customPropertyTypes
    main.cpp
    piechart.cpp piechart.h
    pieslice.cpp pieslice.h
)
set_target_properties(chapter4-customPropertyTypes PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(chapter4-customPropertyTypes PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(chapter4-customPropertyTypes_resource_files
    "app.qml"
)

qt6_add_resources(chapter4-customPropertyTypes "chapter4-customPropertyTypes"
    PREFIX
        "/"
    FILES
        ${chapter4-customPropertyTypes_resource_files}
)

install(TARGETS chapter4-customPropertyTypes
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(chapter4-customPropertyTypes PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI Charts
)

qt6_qml_type_registration(chapter4-customPropertyTypes)
