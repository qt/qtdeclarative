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
