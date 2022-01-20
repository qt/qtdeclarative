cmake_minimum_required(VERSION 3.16)
project(sidepanel LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols2/sidepanel")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick)

qt_add_executable(sidepanel
    sidepanel.cpp
)

set_target_properties(sidepanel PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(sidepanel PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
)

# Resources:
set(qmake_immediate_resource_files
    "doc/images/qtquickcontrols2-sidepanel-landscape.png"
    "doc/images/qtquickcontrols2-sidepanel-portrait.png"
    "images/qt-logo.png"
    "images/qt-logo@2x.png"
    "images/qt-logo@3x.png"
    "images/qt-logo@4x.png"
    "qtquickcontrols2.conf"
    "sidepanel.qml"
)

qt6_add_resources(sidepanel "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)

install(TARGETS sidepanel
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
