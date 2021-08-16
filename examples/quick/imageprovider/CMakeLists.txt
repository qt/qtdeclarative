# Generated from imageprovider.pro.

cmake_minimum_required(VERSION 3.16)
project(qmlimageproviderplugin LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/imageprovider/ImageProviderCore")
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/quick/imageprovider)

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

qt6_add_qml_module(qmlimageproviderplugin
    VERSION 1.0
    URI "ImageProviderCore"
    PLUGIN_TARGET qmlimageproviderplugin
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_PLUGIN_SOURCE
    CLASS_NAME ImageProviderExtensionPlugin
)

target_sources(qmlimageproviderplugin PRIVATE
    imageprovider.cpp
)
set_target_properties(qmlimageproviderplugin PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(qmlimageproviderplugin PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

install(TARGETS qmlimageproviderplugin
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
