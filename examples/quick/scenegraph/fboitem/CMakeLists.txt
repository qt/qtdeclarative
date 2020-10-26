# Generated from fboitem.pro.

cmake_minimum_required(VERSION 3.14)
project(fboitem LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/fboitem")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

qt_add_executable(fboitem
    ../shared/logorenderer.cpp ../shared/logorenderer.h
    fboinsgrenderer.cpp fboinsgrenderer.h
    main.cpp
)
set_target_properties(fboitem PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_include_directories(fboitem PUBLIC
    ../shared
)

target_link_libraries(fboitem PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(fboitem_resource_files
    "main.qml"
    "shaders/checker.frag.qsb"
)

qt6_add_resources(fboitem "fboitem"
    PREFIX
        "/scenegraph/fboitem"
    FILES
        ${fboitem_resource_files}
)

install(TARGETS fboitem
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(fboitem PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI SceneGraphRendering
)

qt6_qml_type_registration(fboitem)
