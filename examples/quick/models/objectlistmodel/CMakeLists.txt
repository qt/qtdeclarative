# Generated from objectlistmodel.pro.

cmake_minimum_required(VERSION 3.16)
project(objectlistmodel LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/models/objectlistmodel")

find_package(Qt6 COMPONENTS Core Gui Qml Quick)

qt_add_executable(objectlistmodelexample WIN32 MACOSX_BUNDLE
    dataobject.cpp
    main.cpp
)
target_link_libraries(objectlistmodelexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
qt_add_qml_module(objectlistmodelexample
    URI objectlistmodel
    VERSION 1.0
    QML_FILES
        "view.qml"
)

install(TARGETS objectlistmodelexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
