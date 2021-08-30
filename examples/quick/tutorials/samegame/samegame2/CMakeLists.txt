cmake_minimum_required(VERSION 3.16)
project(samegame2 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/tutorials/samegame/samegame2")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(samegame2
    main.cpp
)
set_target_properties(samegame2 PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

qt_add_qml_module(samegame2
    URI samegame
    VERSION 1.0
    QML_FILES
        "Block.qml"
        "Button.qml"
        "samegame.qml"
        "samegame.js"
    RESOURCES
        "pics/background.jpg"
        "pics/redStone.png"
)

target_link_libraries(samegame2 PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

install(TARGETS samegame2
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
