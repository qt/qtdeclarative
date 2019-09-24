# Generated from texteditor.pro.

cmake_minimum_required(VERSION 3.14)
project(texteditor LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS QuickControls2)

add_qt_gui_executable(texteditor
    documenthandler.cpp documenthandler.h
    texteditor.cpp
)
target_link_libraries(texteditor PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::QuickControls2
)

# Resources:
set(texteditor_resource_files
    "+touch/texteditor.html"
    "fonts/fontello.ttf"
    "images/qt-logo.png"
    "qml/+touch/texteditor.qml"
    "qml/texteditor.qml"
    "qtquickcontrols2.conf"
    "texteditor.html"
)

qt6_add_resources(texteditor "texteditor"
    PREFIX
        "/"
    FILES
        ${texteditor_resource_files}
)


install(TARGETS texteditor
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
