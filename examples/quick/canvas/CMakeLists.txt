# Generated from canvas.pro.

cmake_minimum_required(VERSION 3.14)
project(canvas LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

add_qt_gui_executable(canvas
    main.cpp
)
target_link_libraries(canvas PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

# Resources:
set(canvas_resource_files
    "bezierCurve/bezierCurve.qml"
    "canvas.qml"
    "clip/clip.qml"
    "contents/qt-logo.png"
    "quadraticCurveTo/quadraticCurveTo.qml"
    "roundedrect/roundedrect.qml"
    "smile/smile.qml"
    "squircle/squircle.png"
    "squircle/squircle.qml"
    "tiger/tiger.js"
    "tiger/tiger.qml"
)

QT6_ADD_RESOURCES(canvas "canvas"
    PREFIX
        "/canvas"
    FILES
        ${canvas_resource_files}
)
set_source_files_properties("../shared/Button.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Button.qml"
)
set_source_files_properties("../shared/CheckBox.qml"
    PROPERTIES QT_RESOURCE_ALIAS "CheckBox.qml"
)
set_source_files_properties("../shared/FlickrRssModel.qml"
    PROPERTIES QT_RESOURCE_ALIAS "FlickrRssModel.qml"
)
set_source_files_properties("../shared/Label.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Label.qml"
)
set_source_files_properties("../shared/LauncherList.qml"
    PROPERTIES QT_RESOURCE_ALIAS "LauncherList.qml"
)
set_source_files_properties("../shared/SimpleLauncherDelegate.qml"
    PROPERTIES QT_RESOURCE_ALIAS "SimpleLauncherDelegate.qml"
)
set_source_files_properties("../shared/Slider.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Slider.qml"
)
set_source_files_properties("../shared/TabSet.qml"
    PROPERTIES QT_RESOURCE_ALIAS "TabSet.qml"
)
set_source_files_properties("../shared/TextField.qml"
    PROPERTIES QT_RESOURCE_ALIAS "TextField.qml"
)
set_source_files_properties("../shared/images/back.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/back.png"
)
set_source_files_properties("../shared/images/checkmark.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/checkmark.png"
)
set_source_files_properties("../shared/images/next.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/next.png"
)
set_source_files_properties("../shared/images/qt-logo.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/qt-logo.png"
)
set_source_files_properties("../shared/images/slider_handle.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/slider_handle.png"
)
set_source_files_properties("../shared/images/tab.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/tab.png"
)
set(shared_resource_files
    "Button.qml"
    "CheckBox.qml"
    "FlickrRssModel.qml"
    "Label.qml"
    "LauncherList.qml"
    "SimpleLauncherDelegate.qml"
    "Slider.qml"
    "TabSet.qml"
    "TextField.qml"
    "images/back.png"
    "images/checkmark.png"
    "images/next.png"
    "images/qt-logo.png"
    "images/slider_handle.png"
    "images/tab.png"
)

QT6_ADD_RESOURCES(canvas "shared"
    PREFIX
        "/shared"
    BASE
        "../shared"
    FILES
        ${shared_resource_files}
)


install(TARGETS canvas
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
