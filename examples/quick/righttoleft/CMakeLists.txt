cmake_minimum_required(VERSION 3.16)
project(righttoleft LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/righttoleft")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

add_subdirectory("../shared" "shared")

qt_add_executable(righttoleftexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

qt_add_qml_module(righttoleftexample
    URI righttoleft
    VERSION 1.0
    QML_FILES
        "layoutdirection/layoutdirection.qml"
        "layoutmirroring/layoutmirroring.qml"
        "righttoleft.qml"
        "textalignment/textalignment.qml"
)

target_link_libraries(righttoleftexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

install(TARGETS righttoleftexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(righttoleftexample)
