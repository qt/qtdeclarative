cmake_minimum_required(VERSION 3.16)

project(flipable LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/customitems/flipable")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(flipableexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(flipableexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

qt_add_qml_module(flipableexample
    URI flipable
    VERSION 1.0
    QML_FILES
        "flipable.qml"
        "Card.qml"
    RESOURCES
        "5_heart.png"
        "9_club.png"
        "back.png"
)

install(TARGETS flipableexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
