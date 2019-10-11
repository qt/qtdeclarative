# Generated from chapter1-settingup.pro.

cmake_minimum_required(VERSION 3.14)
project(chapter1-settingup LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/quickcontrols2/chattutorial/chapter1-settingup")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(chapter1-settingup
    main.cpp
)
target_link_libraries(chapter1-settingup PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

# Resources:
set(qml_resource_files
    "main.qml"
)

qt6_add_resources(chapter1-settingup "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


install(TARGETS chapter1-settingup
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
