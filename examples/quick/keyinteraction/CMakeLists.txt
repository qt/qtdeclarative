cmake_minimum_required(VERSION 3.16)
project(keyinteraction LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/keyinteraction")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(keyinteractionexample WIN32 MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(keyinteractionexample PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

# Resources:
qt_add_qml_module(keyinteractionexample
    URI keyinteraction
    VERSION 1.0
    QML_FILES
        "ContextMenu.qml"
        "GridMenu.qml"
        "ListMenu.qml"
        "ListViewDelegate.qml"
        "TabMenu.qml"
        "focus.qml"
        "keyinteraction.qml"
    RESOURCES
        "images/arrow.png"
        "images/qt-logo.png"
)

install(TARGETS keyinteractionexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
