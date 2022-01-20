cmake_minimum_required(VERSION 3.16)
project(imageelements LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/imageelements")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

add_subdirectory("../shared" "shared")

qt_add_executable(imageelementsexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(imageelementsexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

qt_add_qml_module(imageelementsexample
    URI imageelements
    VERSION 1.0
    QML_FILES
        "animatedimage.qml"
        "animatedsprite.qml"
        "borderimage.qml"
        "framestepping.qml"
        "image.qml"
        "imageelements.qml"
        "multiframeborderimage.qml"
        "shadows.qml"
        "spritesequence.qml"
        "BorderImageSelector.qml"
        "ImageCell.qml"
        "MyBorderImage.qml"
        "ShadowRectangle.qml"
    RESOURCES
        "pics/qt-logo.png"
        "pics/shadow.png"
        "pics/speaker.png"
        "pics/colors.png"
        "pics/BearSheet.png"
        "pics/Uniflow_steam_engine.gif"
        "pics/arrow.png"
        "pics/bw.png"
        "pics/multi.ico"
        "pics/colors-round.sci"
        "pics/colors-stretch.sci"
    )

install(TARGETS imageelementsexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(imageelementsexample)
