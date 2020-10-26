# Generated from metaltextureimport.pro.

cmake_minimum_required(VERSION 3.14)
project(metaltextureimport LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/metaltextureimport")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

qt_add_executable(metaltextureimport
    main.cpp
    metaltextureimport.h metaltextureimport.mm
)
set_target_properties(metaltextureimport PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(metaltextureimport PUBLIC
    "-framework Metal"
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(metaltextureimport_resource_files
    "main.qml"
    "squircle.frag"
    "squircle.vert"
)

qt6_add_resources(metaltextureimport "metaltextureimport"
    PREFIX
        "/scenegraph/metaltextureimport"
    FILES
        ${metaltextureimport_resource_files}
)

if(MACOS)
    target_link_libraries(metaltextureimport PUBLIC
        "-framework AppKit"
    )
endif()

install(TARGETS metaltextureimport
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(metaltextureimport PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI MetalTextureImport
)

qt6_qml_type_registration(metaltextureimport)
