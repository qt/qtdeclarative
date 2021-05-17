# Generated from localstorage.pro.

cmake_minimum_required(VERSION 3.14)
project(localstorage LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/localstorage/localstorage")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(localstorage
    main.cpp
)
set_target_properties(localstorage PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(localstorage PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(localstorage_resource_files
    "Database.js"
    "Header.qml"
    "MyButton.qml"
    "MyDelegate.qml"
    "MyModel.qml"
    "localstorage.qml"
)

qt6_add_resources(localstorage "localstorage"
    PREFIX
        "/"
    FILES
        ${localstorage_resource_files}
)
set(shared_resource_files
    "../../shared/Button.qml"
    "../../shared/CheckBox.qml"
    "../../shared/FlickrRssModel.qml"
    "../../shared/Label.qml"
    "../../shared/LauncherList.qml"
    "../../shared/SimpleLauncherDelegate.qml"
    "../../shared/Slider.qml"
    "../../shared/TabSet.qml"
    "../../shared/TextField.qml"
    "../../shared/images/back.png"
    "../../shared/images/checkmark.png"
    "../../shared/images/next.png"
    "../../shared/images/qt-logo.png"
    "../../shared/images/slider_handle.png"
    "../../shared/images/tab.png"
)

qt6_add_resources(localstorage "shared"
    PREFIX
        "/shared"
    BASE
        "../../shared"
    FILES
        ${shared_resource_files}
)

install(TARGETS localstorage
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
