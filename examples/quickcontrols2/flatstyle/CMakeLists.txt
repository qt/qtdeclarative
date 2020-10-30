# Generated from flatstyle.pro.

cmake_minimum_required(VERSION 3.14)
project(flatstyle LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols2/flatstyle")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS QuickControls2)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(flatstyle
    main.cpp
)
set_target_properties(flatstyle PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(flatstyle PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::QuickControls2
    Qt::Widgets
)


# Resources:
set(qmake_immediate_resource_files
    "MainForm.ui.qml"
    "flatstyle.qml"
    "imports/Flat/Button.qml"
    "imports/Flat/CheckBox.qml"
    "imports/Flat/Switch.qml"
    "imports/Flat/qmldir"
    "imports/Theme/Theme.qml"
    "imports/Theme/qmldir"
    "qtquickcontrols2.conf"
)

qt6_add_resources(flatstyle "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)

install(TARGETS flatstyle
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
