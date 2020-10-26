# Generated from painteditem.pro.

cmake_minimum_required(VERSION 3.14)
project(qmltextballoonplugin LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/customitems/painteditem/TextBalloonPlugin")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

qt6_add_qml_module(qmltextballoonplugin
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/TextBalloonPlugin"
    VERSION 1.0
    URI "TextBalloonPlugin"
    INSTALL_LOCATION ${INSTALL_EXAMPLEDIR}
)

target_sources(qmltextballoonplugin PRIVATE
    TextBalloonPlugin/plugin.h
    textballoon.cpp textballoon.h
)
set_target_properties(qmltextballoonplugin PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(qmltextballoonplugin PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(painteditem_resource_files
    "textballoon.h"
    "textballoons.qml"
)

qt6_add_resources(qmltextballoonplugin "painteditem"
    PREFIX
        "/painteditem"
    FILES
        ${painteditem_resource_files}
)

install(TARGETS qmltextballoonplugin
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(qmltextballoonplugin PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI TextBalloonPlugin
)

qt6_qml_type_registration(qmltextballoonplugin)
