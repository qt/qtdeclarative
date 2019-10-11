# Generated from objectlistmodel.pro.

cmake_minimum_required(VERSION 3.14)
project(objectlistmodel LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/quick/models/objectlistmodel")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(objectlistmodel
    dataobject.cpp dataobject.h
    main.cpp
)
target_link_libraries(objectlistmodel PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(objectlistmodel_resource_files
    "view.qml"
)

qt6_add_resources(objectlistmodel "objectlistmodel"
    PREFIX
        "/"
    FILES
        ${objectlistmodel_resource_files}
)

install(TARGETS objectlistmodel
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
