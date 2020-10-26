# Generated from qquickviewcomparison.pro.

cmake_minimum_required(VERSION 3.14)
project(qquickviewcomparison LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/quickwidgets/qquickviewcomparison")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS QuickWidgets)

qt_add_executable(qquickviewcomparison
    fbitem.cpp fbitem.h
    logo.cpp logo.h
    main.cpp
    mainwindow.cpp mainwindow.h
)
set_target_properties(qquickviewcomparison PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(qquickviewcomparison PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::QuickWidgets
    Qt::Widgets
)


# Resources:
set(qquickviewcomparison_resource_files
    "test.qml"
)

qt6_add_resources(qquickviewcomparison "qquickviewcomparison"
    PREFIX
        "/qquickviewcomparison"
    FILES
        ${qquickviewcomparison_resource_files}
)

install(TARGETS qquickviewcomparison
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(qquickviewcomparison PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI fbitem
)

qt6_qml_type_registration(qquickviewcomparison)
