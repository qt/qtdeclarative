# Generated from texteditor.pro.

cmake_minimum_required(VERSION 3.14)
project(texteditor LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols2/texteditor")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS QuickControls2)

qt_add_executable(texteditor
    documenthandler.cpp documenthandler.h
    texteditor.cpp
)
set_target_properties(texteditor PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
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

if(TARGET Qt::Widgets)
    target_link_libraries(texteditor PUBLIC
        Qt::Widgets
    )
endif()

if(CMAKE_CROSSCOMPILING)
    target_compile_definitions(texteditor PUBLIC
        QT_EXTRA_FILE_SELECTOR="touch" # special case
    )
endif()

install(TARGETS texteditor
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
