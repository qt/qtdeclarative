cmake_minimum_required(VERSION 3.16)
project(gameoflife LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/tableview/gameoflife")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(gameoflife
    gameoflifemodel.cpp gameoflifemodel.h
    main.cpp
)

set_target_properties(gameoflife PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(gameoflife PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

qt_add_qml_module(gameoflife
    URI GameOfLifeModel
    VERSION 1.0
    QML_FILES main.qml
    RESOURCES gosperglidergun.cells
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS gameoflife
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
