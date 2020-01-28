# Generated from chapter2-methods.pro.

cmake_minimum_required(VERSION 3.14)
project(chapter2-methods LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/qml/tutorials/extending-qml/chapter2-methods")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(chapter2-methods
    main.cpp
    piechart.cpp piechart.h
)
target_link_libraries(chapter2-methods PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(chapter2-methods_resource_files
    "app.qml"
)

qt6_add_resources(chapter2-methods "chapter2-methods"
    PREFIX
        "/"
    FILES
        ${chapter2-methods_resource_files}
)

install(TARGETS chapter2-methods
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(chapter2-methods PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI Charts
)

qt6_qml_type_registration(chapter2-methods)
