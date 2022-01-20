cmake_minimum_required(VERSION 3.16)
project(externaldraganddrop LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/externaldraganddrop")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(externaldraganddropexample
    main.cpp
)

set_target_properties(externaldraganddropexample PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

qt_add_qml_module(externaldraganddropexample
    URI externaldraganddrop
    VERSION 1.0
    QML_FILES
        "DragAndDropTextItem.qml"
        "externaldraganddrop.qml"
)

target_link_libraries(externaldraganddropexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

install(TARGETS externaldraganddropexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
