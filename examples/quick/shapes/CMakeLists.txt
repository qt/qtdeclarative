# Generated from shapes.pro.

cmake_minimum_required(VERSION 3.14)
project(shapes LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/shapes")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(shapes
    main.cpp
)
set_target_properties(shapes PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(shapes PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set_source_files_properties("../shared/LauncherList.qml"
    PROPERTIES QT_RESOURCE_ALIAS "LauncherList.qml"
)
set_source_files_properties("../shared/SimpleLauncherDelegate.qml"
    PROPERTIES QT_RESOURCE_ALIAS "SimpleLauncherDelegate.qml"
)
set_source_files_properties("../shared/Slider.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Slider.qml"
)
set_source_files_properties("../shared/images/back.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/back.png"
)
set_source_files_properties("../shared/images/next.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/next.png"
)
set_source_files_properties("../shared/images/slider_handle.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/slider_handle.png"
)
set_source_files_properties("content/clippedtigers.qml"
    PROPERTIES QT_RESOURCE_ALIAS "clippedtigers.qml"
)
set_source_files_properties("content/interactive.qml"
    PROPERTIES QT_RESOURCE_ALIAS "interactive.qml"
)
set_source_files_properties("content/item10.qml"
    PROPERTIES QT_RESOURCE_ALIAS "item10.qml"
)
set_source_files_properties("content/item11.qml"
    PROPERTIES QT_RESOURCE_ALIAS "item11.qml"
)
set_source_files_properties("content/item12.qml"
    PROPERTIES QT_RESOURCE_ALIAS "item12.qml"
)
set_source_files_properties("content/item13.qml"
    PROPERTIES QT_RESOURCE_ALIAS "item13.qml"
)
set_source_files_properties("content/item14.qml"
    PROPERTIES QT_RESOURCE_ALIAS "item14.qml"
)
set_source_files_properties("content/item15.qml"
    PROPERTIES QT_RESOURCE_ALIAS "item15.qml"
)
set_source_files_properties("content/item17.qml"
    PROPERTIES QT_RESOURCE_ALIAS "item17.qml"
)
set_source_files_properties("content/item18.qml"
    PROPERTIES QT_RESOURCE_ALIAS "item18.qml"
)
set_source_files_properties("content/item2.qml"
    PROPERTIES QT_RESOURCE_ALIAS "item2.qml"
)
set_source_files_properties("content/item3.qml"
    PROPERTIES QT_RESOURCE_ALIAS "item3.qml"
)
set_source_files_properties("content/item4.qml"
    PROPERTIES QT_RESOURCE_ALIAS "item4.qml"
)
set_source_files_properties("content/item5.qml"
    PROPERTIES QT_RESOURCE_ALIAS "item5.qml"
)
set_source_files_properties("content/item6.qml"
    PROPERTIES QT_RESOURCE_ALIAS "item6.qml"
)
set_source_files_properties("content/item7.qml"
    PROPERTIES QT_RESOURCE_ALIAS "item7.qml"
)
set_source_files_properties("content/item8.qml"
    PROPERTIES QT_RESOURCE_ALIAS "item8.qml"
)
set_source_files_properties("content/item9.qml"
    PROPERTIES QT_RESOURCE_ALIAS "item9.qml"
)
set_source_files_properties("content/main.qml"
    PROPERTIES QT_RESOURCE_ALIAS "main.qml"
)
set_source_files_properties("content/sampling.qml"
    PROPERTIES QT_RESOURCE_ALIAS "sampling.qml"
)
set_source_files_properties("content/shapegallery.qml"
    PROPERTIES QT_RESOURCE_ALIAS "shapegallery.qml"
)
set_source_files_properties("content/tapableTriangle.qml"
    PROPERTIES QT_RESOURCE_ALIAS "tapableTriangle.qml"
)
set_source_files_properties("content/tiger.qml"
    PROPERTIES QT_RESOURCE_ALIAS "tiger.qml"
)
set(shapes_resource_files
    "../shared/LauncherList.qml"
    "../shared/SimpleLauncherDelegate.qml"
    "../shared/Slider.qml"
    "../shared/images/back.png"
    "../shared/images/next.png"
    "../shared/images/slider_handle.png"
    "content/clippedtigers.qml"
    "content/interactive.qml"
    "content/item10.qml"
    "content/item11.qml"
    "content/item12.qml"
    "content/item13.qml"
    "content/item14.qml"
    "content/item15.qml"
    "content/item17.qml"
    "content/item18.qml"
    "content/item2.qml"
    "content/item3.qml"
    "content/item4.qml"
    "content/item5.qml"
    "content/item6.qml"
    "content/item7.qml"
    "content/item8.qml"
    "content/item9.qml"
    "content/main.qml"
    "content/sampling.qml"
    "content/shapegallery.qml"
    "content/tapableTriangle.qml"
    "content/tiger.qml"
)

qt6_add_resources(shapes "shapes"
    PREFIX
        "/shapes"
    FILES
        ${shapes_resource_files}
)

install(TARGETS shapes
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
