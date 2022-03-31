cmake_minimum_required(VERSION 3.16)
project(quickwidget LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/quickwidgets/quickwidget")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickWidgets Widgets)

qt_add_executable(quickwidget
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

qt_add_qml_module(quickwidget
    URI QuickWidgetExample
    VERSION 1.0
    QML_FILES
        rotatingsquare.qml
    RESOURCE_PREFIX /quickwidget
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS quickwidget
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
