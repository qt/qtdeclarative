# Generated from system.pro.

cmake_minimum_required(VERSION 3.14)
project(system LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/particles/system")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(system
    main.cpp
)
set_target_properties(system PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(system PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(system_resource_files
    "content/dynamiccomparison.qml"
    "content/dynamicemitters.qml"
    "content/multiplepainters.qml"
    "content/startstop.qml"
    "content/timedgroupchanges.qml"
    "system.qml"
)

qt6_add_resources(system "system"
    PREFIX
        "/particles/system"
    FILES
        ${system_resource_files}
)
set_source_files_properties("../images/_explo.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/_explo.png"
)
set_source_files_properties("../images/backgroundLeaves.jpg"
    PROPERTIES QT_RESOURCE_ALIAS "images/backgroundLeaves.jpg"
)
set_source_files_properties("../images/bear_tiles.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/bear_tiles.png"
)
set_source_files_properties("../images/candle.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/candle.png"
)
set_source_files_properties("../images/colortable.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/colortable.png"
)
set_source_files_properties("../images/finalfrontier.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/finalfrontier.png"
)
set_source_files_properties("../images/flower.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/flower.png"
)
set_source_files_properties("../images/matchmask.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/matchmask.png"
)
set_source_files_properties("../images/meteor.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/meteor.png"
)
set_source_files_properties("../images/meteor_explo.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/meteor_explo.png"
)
set_source_files_properties("../images/meteors.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/meteors.png"
)
set_source_files_properties("../images/nullRock.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/nullRock.png"
)
set_source_files_properties("../images/particle2.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/particle2.png"
)
set_source_files_properties("../images/particle3.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/particle3.png"
)
set_source_files_properties("../images/particleA.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/particleA.png"
)
set_source_files_properties("../images/portal_bg.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/portal_bg.png"
)
set_source_files_properties("../images/realLeaf1.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/realLeaf1.png"
)
set_source_files_properties("../images/realLeaf2.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/realLeaf2.png"
)
set_source_files_properties("../images/realLeaf3.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/realLeaf3.png"
)
set_source_files_properties("../images/realLeaf4.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/realLeaf4.png"
)
set_source_files_properties("../images/rocket.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/rocket.png"
)
set_source_files_properties("../images/rocket2.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/rocket2.png"
)
set_source_files_properties("../images/sizeInOut.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/sizeInOut.png"
)
set_source_files_properties("../images/snowflake.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/snowflake.png"
)
set_source_files_properties("../images/sparkleSize.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/sparkleSize.png"
)
set_source_files_properties("../images/starfish_0.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/starfish_0.png"
)
set_source_files_properties("../images/starfish_1.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/starfish_1.png"
)
set_source_files_properties("../images/starfish_2.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/starfish_2.png"
)
set_source_files_properties("../images/starfish_3.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/starfish_3.png"
)
set_source_files_properties("../images/starfish_4.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/starfish_4.png"
)
set_source_files_properties("../images/starfish_mask.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/starfish_mask.png"
)
set(images_resource_files
    "images/_explo.png"
    "images/backgroundLeaves.jpg"
    "images/bear_tiles.png"
    "images/candle.png"
    "images/colortable.png"
    "images/finalfrontier.png"
    "images/flower.png"
    "images/matchmask.png"
    "images/meteor.png"
    "images/meteor_explo.png"
    "images/meteors.png"
    "images/nullRock.png"
    "images/particle2.png"
    "images/particle3.png"
    "images/particleA.png"
    "images/portal_bg.png"
    "images/realLeaf1.png"
    "images/realLeaf2.png"
    "images/realLeaf3.png"
    "images/realLeaf4.png"
    "images/rocket.png"
    "images/rocket2.png"
    "images/sizeInOut.png"
    "images/snowflake.png"
    "images/sparkleSize.png"
    "images/starfish_0.png"
    "images/starfish_1.png"
    "images/starfish_2.png"
    "images/starfish_3.png"
    "images/starfish_4.png"
    "images/starfish_mask.png"
)

qt6_add_resources(system "images"
    PREFIX
        "/particles"
    BASE
        ".."
    FILES
        ${images_resource_files}
)
set_source_files_properties("../../shared/Button.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Button.qml"
)
set_source_files_properties("../../shared/CheckBox.qml"
    PROPERTIES QT_RESOURCE_ALIAS "CheckBox.qml"
)
set_source_files_properties("../../shared/FlickrRssModel.qml"
    PROPERTIES QT_RESOURCE_ALIAS "FlickrRssModel.qml"
)
set_source_files_properties("../../shared/Label.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Label.qml"
)
set_source_files_properties("../../shared/LauncherList.qml"
    PROPERTIES QT_RESOURCE_ALIAS "LauncherList.qml"
)
set_source_files_properties("../../shared/SimpleLauncherDelegate.qml"
    PROPERTIES QT_RESOURCE_ALIAS "SimpleLauncherDelegate.qml"
)
set_source_files_properties("../../shared/Slider.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Slider.qml"
)
set_source_files_properties("../../shared/TabSet.qml"
    PROPERTIES QT_RESOURCE_ALIAS "TabSet.qml"
)
set_source_files_properties("../../shared/TextField.qml"
    PROPERTIES QT_RESOURCE_ALIAS "TextField.qml"
)
set_source_files_properties("../../shared/images/back.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/back.png"
)
set_source_files_properties("../../shared/images/checkmark.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/checkmark.png"
)
set_source_files_properties("../../shared/images/next.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/next.png"
)
set_source_files_properties("../../shared/images/qt-logo.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/qt-logo.png"
)
set_source_files_properties("../../shared/images/slider_handle.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/slider_handle.png"
)
set_source_files_properties("../../shared/images/tab.png"
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

qt6_add_resources(system "shared"
    PREFIX
        "/shared"
    BASE
        "../../shared"
    FILES
        ${shared_resource_files}
)

install(TARGETS system
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
