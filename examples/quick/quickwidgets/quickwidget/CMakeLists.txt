# Generated from quickwidget.pro.

cmake_minimum_required(VERSION 3.14)
project(quickwidget LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/quickwidgets/quickwidget")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS QuickWidgets)

qt_add_executable(quickwidget
    fbitem.cpp fbitem.h
    main.cpp
)
set_target_properties(quickwidget PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(quickwidget PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::QuickWidgets
    Qt::Widgets
)


# Resources:
set(quickwidget_resource_files
    "customgl.qml"
    "rotatingsquare.qml"
    "rotatingsquaretab.qml"
)

qt6_add_resources(quickwidget "quickwidget"
    PREFIX
        "/quickwidget"
    FILES
        ${quickwidget_resource_files}
)

install(TARGETS quickwidget
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(quickwidget PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI QuickWidgetExample
)

qt6_qml_type_registration(quickwidget)
