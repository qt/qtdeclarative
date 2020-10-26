# Generated from imageresponseprovider.pro.

cmake_minimum_required(VERSION 3.14)
project(qmlimageresponseproviderplugin LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/imageresponseprovider/ImageResponseProviderCore")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

qt6_add_qml_module(qmlimageresponseproviderplugin
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ImageResponseProviderCore"
    VERSION 1.0
    URI "ImageResponseProviderCore"
    INSTALL_LOCATION ${INSTALL_EXAMPLEDIR}
)

target_sources(qmlimageresponseproviderplugin PRIVATE
    imageresponseprovider.cpp
)
set_target_properties(qmlimageresponseproviderplugin PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(qmlimageresponseproviderplugin PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

install(TARGETS qmlimageresponseproviderplugin
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
