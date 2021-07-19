# Generated from qmlextensionplugins.pro.

cmake_minimum_required(VERSION 3.14)
project(qmlqtimeexample LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/qmlextensionplugins/imports/TimeExample")
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/qml/qmlextensionplugins/imports)

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)

set_source_files_properties(imports/TimeExample/Clock.qml PROPERTIES
    QT_RESOURCE_ALIAS Clock.qml
)
qt6_add_qml_module(qmlqtimeexample
    VERSION 1.0
    URI "TimeExample"
    SOURCES timemodel.cpp timemodel.h
    QML_FILES imports/TimeExample/Clock.qml
)
set_target_properties(qmlqtimeexample PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(qmlqtimeexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
)

qt6_add_resources(qmlqtimeexample example_images
    PREFIX "/TimeExample"
    BASE   imports/TimeExample
    FILES
        imports/TimeExample/center.png
        imports/TimeExample/clock.png
        imports/TimeExample/hour.png
        imports/TimeExample/minute.png
)

install(TARGETS qmlqtimeexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
