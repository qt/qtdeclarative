cmake_minimum_required(VERSION 3.16)
project(test_static_qml_module LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Test)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(test_static_qml_module
    main.cpp
)

qt_add_qml_module(test_static_qml_module
    URI MyAppUri
    VERSION 1.0
    QML_FILES
        main.qml
    IMPORT_PATH
        ${CMAKE_CURRENT_BINARY_DIR}/qml
)

target_link_libraries(test_static_qml_module
    PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::Test
)

qt_add_qml_module(test_static_qml_module_lib
    STATIC
    URI MyUri
    OUTPUT_DIRECTORY qml/MyUri
    VERSION 1.0
    SOURCES
        MyElement.cpp MyElement.h
)

qt_import_qml_plugins(test_static_qml_module)
