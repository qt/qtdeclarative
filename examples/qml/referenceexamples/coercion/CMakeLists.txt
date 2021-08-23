# Generated from coercion.pro.

cmake_minimum_required(VERSION 3.16)
project(coercion LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/referenceexamples/coercion")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(coercion
    birthdayparty.cpp birthdayparty.h
    main.cpp
    person.cpp person.h
)
set_target_properties(coercion PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(coercion PUBLIC
    Qt::Core
    Qt::Qml
)

qt_add_qml_module(coercion
    URI People
    VERSION 1.0
    QML_FILES example.qml
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS coercion
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
