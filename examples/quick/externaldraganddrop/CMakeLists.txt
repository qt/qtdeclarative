# Generated from externaldraganddrop.pro.

cmake_minimum_required(VERSION 3.16)
project(externaldraganddrop LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/externaldraganddrop")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(externaldraganddrop
    main.cpp
)
set_target_properties(externaldraganddrop PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(externaldraganddrop PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(externaldraganddrop_resource_files
    "DragAndDropTextItem.qml"
    "externaldraganddrop.qml"
)

qt6_add_resources(externaldraganddrop "externaldraganddrop"
    PREFIX
        "/externaldraganddrop"
    FILES
        ${externaldraganddrop_resource_files}
)
set(shared_resource_files
    "../shared/Button.qml"
    "../shared/CheckBox.qml"
    "../shared/FlickrRssModel.qml"
    "../shared/Label.qml"
    "../shared/LauncherList.qml"
    "../shared/SimpleLauncherDelegate.qml"
    "../shared/Slider.qml"
    "../shared/TabSet.qml"
    "../shared/TextField.qml"
    "../shared/images/back.png"
    "../shared/images/checkmark.png"
    "../shared/images/next.png"
    "../shared/images/qt-logo.png"
    "../shared/images/slider_handle.png"
    "../shared/images/tab.png"
)

qt6_add_resources(externaldraganddrop "shared"
    PREFIX
        "/shared"
    BASE
        "../shared"
    FILES
        ${shared_resource_files}
)

install(TARGETS externaldraganddrop
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
