# Generated from imageelements.pro.

cmake_minimum_required(VERSION 3.16)
project(imageelements LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/imageelements")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(imageelements
    main.cpp
)
set_target_properties(imageelements PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(imageelements PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(imageelements_resource_files
    "animatedimage.qml"
    "animatedsprite.qml"
    "borderimage.qml"
    "content/BearSheet.png"
    "content/BorderImageSelector.qml"
    "content/ImageCell.qml"
    "content/MyBorderImage.qml"
    "content/ShadowRectangle.qml"
    "content/Uniflow_steam_engine.gif"
    "content/arrow.png"
    "content/bw.png"
    "content/colors-round.sci"
    "content/colors-stretch.sci"
    "content/colors.png"
    "content/multi.ico"
    "content/qt-logo.png"
    "content/shadow.png"
    "content/speaker.png"
    "framestepping.qml"
    "image.qml"
    "imageelements.qml"
    "multiframeborderimage.qml"
    "shadows.qml"
    "spritesequence.qml"
)

qt6_add_resources(imageelements "imageelements"
    PREFIX
        "/imageelements"
    FILES
        ${imageelements_resource_files}
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

qt6_add_resources(imageelements "shared"
    PREFIX
        "/shared"
    BASE
        "../shared"
    FILES
        ${shared_resource_files}
)

install(TARGETS imageelements
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
