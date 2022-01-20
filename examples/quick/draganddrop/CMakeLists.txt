cmake_minimum_required(VERSION 3.16)
project(draganddrop LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/draganddrop")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

add_subdirectory("../shared" "shared")

qt_add_executable(draganddropexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

qt_add_qml_module(draganddropexample
    URI draganddrop
    VERSION 1.0
    QML_FILES
        "draganddrop.qml"
        "tiles/DragTile.qml"
        "tiles/DropTile.qml"
        "tiles/tiles.qml"
        "views/Icon.qml"
        "views/gridview.qml"
)

target_link_libraries(draganddropexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

add_dependencies(draganddropexample draganddrop_shared)

install(TARGETS draganddropexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(draganddropexample)
