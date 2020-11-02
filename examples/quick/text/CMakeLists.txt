# Generated from text.pro.

cmake_minimum_required(VERSION 3.14)
project(text LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/text")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(text
    main.cpp
)
set_target_properties(text PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(text PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(text_resource_files
    "fonts/availableFonts.qml"
    "fonts/banner.qml"
    "fonts/content/fonts/tarzeau_ocr_a.ttf"
    "fonts/fonts.qml"
    "fonts/hello.qml"
    "imgtag/TextWithImage.qml"
    "imgtag/images/face-sad.png"
    "imgtag/images/face-smile-big.png"
    "imgtag/images/face-smile.png"
    "imgtag/images/heart200.png"
    "imgtag/images/qtlogo.png"
    "imgtag/images/starfish_2.png"
    "imgtag/imgtag.qml"
    "styledtext-layout.qml"
    "text.qml"
    "textselection/pics/endHandle.png"
    "textselection/pics/endHandle.sci"
    "textselection/pics/startHandle.png"
    "textselection/pics/startHandle.sci"
    "textselection/textselection.qml"
)

qt6_add_resources(text "text"
    PREFIX
        "/text"
    FILES
        ${text_resource_files}
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

qt6_add_resources(text "shared"
    PREFIX
        "/shared"
    BASE
        "../shared"
    FILES
        ${shared_resource_files}
)

qt_import_qml_plugins(text) # special case

install(TARGETS text
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
