# Generated from dynamicscene.pro.

cmake_minimum_required(VERSION 3.14)
project(dynamicscene LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/dynamicscene")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(dynamicscene
    main.cpp
)
set_target_properties(dynamicscene PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(dynamicscene PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(dynamicscene_resource_files
    "content/GenericSceneItem.qml"
    "content/PaletteItem.qml"
    "content/PerspectiveItem.qml"
    "content/Sun.qml"
    "content/images/NOTE"
    "content/images/face-smile.png"
    "content/images/moon.png"
    "content/images/rabbit_brown.png"
    "content/images/rabbit_bw.png"
    "content/images/star.png"
    "content/images/sun.png"
    "content/images/tree_s.png"
    "content/itemCreation.js"
    "dynamicscene.qml"
)

qt6_add_resources(dynamicscene "dynamicscene"
    PREFIX
        "/"
    FILES
        ${dynamicscene_resource_files}
)

install(TARGETS dynamicscene
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
