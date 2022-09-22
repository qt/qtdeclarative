cmake_minimum_required(VERSION 3.16)
project(sidepanel LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols2/sidepanel")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick)

qt_add_executable(sidepanelexample WIN32 MACOSX_BUNDLE
    sidepanel.cpp
)

qt_add_qml_module(sidepanelexample
    URI sidepanel
    VERSION 1.0
    NO_RESOURCE_TARGET_PATH
    QML_FILES
        "sidepanel.qml"
    RESOURCES
        "images/qt-logo.png"
        "images/qt-logo@2x.png"
        "images/qt-logo@3x.png"
        "images/qt-logo@4x.png"
        "qtquickcontrols2.conf"
        "doc/images/qtquickcontrols2-sidepanel-landscape.png"
        "doc/images/qtquickcontrols2-sidepanel-portrait.png"
)

target_link_libraries(sidepanelexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
)

install(TARGETS sidepanelexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
