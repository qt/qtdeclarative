# Generated from opengltextureinthread.pro.

cmake_minimum_required(VERSION 3.14)
project(opengltextureinthread LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/opengltextureinthread")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(opengltextureinthread
    ../shared/logorenderer.cpp ../shared/logorenderer.h
    main.cpp
    threadrenderer.cpp threadrenderer.h
)
target_include_directories(opengltextureinthread PUBLIC
    ../shared
)

target_link_libraries(opengltextureinthread PUBLIC
    Qt::Core
    Qt::CorePrivate
    Qt::Gui
    Qt::GuiPrivate
    Qt::Quick
)


# Resources:
set(textureinthread_resource_files
    "error.qml"
    "main.qml"
)

qt6_add_resources(opengltextureinthread "textureinthread"
    PREFIX
        "/scenegraph/textureinthread"
    FILES
        ${textureinthread_resource_files}
)

install(TARGETS opengltextureinthread
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(opengltextureinthread PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI SceneGraphRendering
)

qt6_qml_type_registration(opengltextureinthread)
