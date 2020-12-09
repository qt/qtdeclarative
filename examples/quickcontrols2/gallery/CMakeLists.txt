# Generated from gallery.pro.

cmake_minimum_required(VERSION 3.14)
project(gallery_controls2 LANGUAGES CXX) # special case

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols2/gallery")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS QuickControls2)

qt_add_executable(gallery_controls2 # special case
    gallery.cpp
)
set_target_properties(gallery_controls2 PROPERTIES # special case
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(gallery_controls2 PUBLIC # special case
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::QuickControls2
)
# special case begin
if(UNIX AND NOT APPLE AND CMAKE_CROSSCOMPILING)
    find_package(Qt6 COMPONENTS QuickTemplates2)
     # Work around QTBUG-86533
    target_link_libraries(gallery_controls2 PRIVATE Qt::QuickTemplates2)
endif()
# special case end

# Resources:
set(qmake_immediate_resource_files
    "+Material/ToolBar.qml"
    "ToolBar.qml"
    "gallery.qml"
    "icons/gallery/20x20/back.png"
    "icons/gallery/20x20/drawer.png"
    "icons/gallery/20x20/menu.png"
    "icons/gallery/20x20@2/back.png"
    "icons/gallery/20x20@2/drawer.png"
    "icons/gallery/20x20@2/menu.png"
    "icons/gallery/20x20@3/back.png"
    "icons/gallery/20x20@3/drawer.png"
    "icons/gallery/20x20@3/menu.png"
    "icons/gallery/20x20@4/back.png"
    "icons/gallery/20x20@4/drawer.png"
    "icons/gallery/20x20@4/menu.png"
    "icons/gallery/index.theme"
    "images/arrow.png"
    "images/arrow@2x.png"
    "images/arrow@3x.png"
    "images/arrow@4x.png"
    "images/arrows.png"
    "images/arrows@2x.png"
    "images/arrows@3x.png"
    "images/arrows@4x.png"
    "images/qt-logo.png"
    "images/qt-logo@2x.png"
    "images/qt-logo@3x.png"
    "images/qt-logo@4x.png"
    "pages/BusyIndicatorPage.qml"
    "pages/ButtonPage.qml"
    "pages/CheckBoxPage.qml"
    "pages/ComboBoxPage.qml"
    "pages/DelayButtonPage.qml"
    "pages/DelegatePage.qml"
    "pages/DialPage.qml"
    "pages/DialogPage.qml"
    "pages/FramePage.qml"
    "pages/GroupBoxPage.qml"
    "pages/PageIndicatorPage.qml"
    "pages/ProgressBarPage.qml"
    "pages/RadioButtonPage.qml"
    "pages/RangeSliderPage.qml"
    "pages/ScrollBarPage.qml"
    "pages/ScrollIndicatorPage.qml"
    "pages/ScrollablePage.qml"
    "pages/SliderPage.qml"
    "pages/SpinBoxPage.qml"
    "pages/StackViewPage.qml"
    "pages/SwipeViewPage.qml"
    "pages/SwitchPage.qml"
    "pages/TabBarPage.qml"
    "pages/TextAreaPage.qml"
    "pages/TextFieldPage.qml"
    "pages/ToolTipPage.qml"
    "pages/TumblerPage.qml"
    "qmldir"
    "qtquickcontrols2.conf"
)

qt6_add_resources(gallery_controls2 "qmake_immediate" # special case
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)

install(TARGETS gallery_controls2 # special case
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
