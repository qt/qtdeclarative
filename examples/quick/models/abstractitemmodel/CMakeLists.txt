# Generated from abstractitemmodel.pro.

cmake_minimum_required(VERSION 3.14)
project(abstractitemmodel LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(abstractitemmodel
    main.cpp
    model.cpp model.h
)
target_link_libraries(abstractitemmodel PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

# Resources:
set(abstractitemmodel_resource_files
    "view.qml"
)

QT6_ADD_RESOURCES(abstractitemmodel "abstractitemmodel"
    PREFIX
        "/"
    FILES
        ${abstractitemmodel_resource_files}
)


install(TARGETS abstractitemmodel
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
