# Generated from qmlextensionplugins.pro.

cmake_minimum_required(VERSION 3.14)
project(qmlqtimeexampleplugin LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/qmlextensionplugins/imports/TimeExample")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)

qt6_add_qml_module(qmlqtimeexampleplugin
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/imports/TimeExample"
    VERSION 1.0
    URI "TimeExample"
    INSTALL_LOCATION ${INSTALL_EXAMPLEDIR}
)

target_sources(qmlqtimeexampleplugin PRIVATE
    plugin.cpp
    timemodel.cpp timemodel.h
)
set_target_properties(qmlqtimeexampleplugin PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(qmlqtimeexampleplugin PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
)

install(TARGETS qmlqtimeexampleplugin
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(qmlqtimeexampleplugin PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI TimeExample
    QT_QMLTYPES_FILENAME imports/TimeExample/plugins.qmltypes
)

qt6_qml_type_registration(qmlqtimeexampleplugin)
