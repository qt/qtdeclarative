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
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/quick/customitems/painteditem)

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

qt6_add_qml_module(qmltextballoonplugin
    VERSION 1.0
    URI "TextBalloonPlugin"
    PLUGIN_TARGET qmltextballoonplugin
    QML_FILES textballoons.qml
    RESOURCE_PREFIX "/painteditem"
)

target_sources(qmltextballoonplugin PRIVATE
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
