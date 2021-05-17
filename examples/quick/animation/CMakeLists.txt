# Generated from animation.pro.

cmake_minimum_required(VERSION 3.14)
project(animation LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/animation")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(animation
    main.cpp
)
set_target_properties(animation PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(animation PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
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

qt6_add_resources(animation "shared"
    PREFIX
        "/shared"
    BASE
        "../shared"
    FILES
        ${shared_resource_files}
)
set(animation_resource_files
    "animation.qml"
    "basics/animators.qml"
    "basics/color-animation.qml"
    "basics/images/face-smile.png"
    "basics/images/moon.png"
    "basics/images/shadow.png"
    "basics/images/star.png"
    "basics/images/sun.png"
    "basics/property-animation.qml"
    "behaviors/FocusRect.qml"
    "behaviors/SideRect.qml"
    "behaviors/behavior-example.qml"
    "behaviors/tvtennis.qml"
    "behaviors/wigglytext.qml"
    "easing/easing.qml"
    "pathanimation/pathanimation.qml"
    "pathinterpolator/pathinterpolator.qml"
    "states/qt-logo.png"
    "states/states.qml"
    "states/transitions.qml"
)

qt6_add_resources(animation "animation"
    PREFIX
        "/animation"
    FILES
        ${animation_resource_files}
)

install(TARGETS animation
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
