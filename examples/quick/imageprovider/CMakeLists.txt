# Generated from imageprovider.pro.

cmake_minimum_required(VERSION 3.14)
project(qmlimageproviderplugin LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/quick/imageprovider/ImageProviderCore")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

qt6_add_qml_module(qmlimageproviderplugin
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ImageProviderCore"
    VERSION 1.0
    URI "ImageProviderCore"
    INSTALL_LOCATION ${INSTALL_EXAMPLEDIR}
)

target_sources(qmlimageproviderplugin PRIVATE
    imageprovider.cpp
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
