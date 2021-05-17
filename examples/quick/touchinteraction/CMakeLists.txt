# Generated from touchinteraction.pro.

cmake_minimum_required(VERSION 3.14)
project(touchinteraction LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/touchinteraction")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(touchinteraction
    main.cpp
)
set_target_properties(touchinteraction PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(touchinteraction PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(touchinteraction_resource_files
    "flickable/basic-flickable.qml"
    "flickable/content/Panel.qml"
    "flickable/content/cork.jpg"
    "flickable/content/note-yellow.png"
    "flickable/content/tack.png"
    "flickable/corkboards.qml"
    "multipointtouch/bearwhack.qml"
    "multipointtouch/content/AugmentedTouchPoint.qml"
    "multipointtouch/content/Bear0.png"
    "multipointtouch/content/Bear1.png"
    "multipointtouch/content/Bear2.png"
    "multipointtouch/content/Bear3.png"
    "multipointtouch/content/BearB.png"
    "multipointtouch/content/BearWhackParticleSystem.qml"
    "multipointtouch/content/ParticleFlame.qml"
    "multipointtouch/content/blur-circle.png"
    "multipointtouch/content/blur-circle3.png"
    "multipointtouch/content/heart-blur.png"
    "multipointtouch/content/title.png"
    "multipointtouch/multiflame.qml"
    "pincharea/flickresize.qml"
    "pincharea/qt-logo.jpg"
    "touchinteraction.qml"
)

qt6_add_resources(touchinteraction "touchinteraction"
    PREFIX
        "/touchinteraction"
    FILES
        ${touchinteraction_resource_files}
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

qt6_add_resources(touchinteraction "shared"
    PREFIX
        "/shared"
    BASE
        "../shared"
    FILES
        ${shared_resource_files}
)

install(TARGETS touchinteraction
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
