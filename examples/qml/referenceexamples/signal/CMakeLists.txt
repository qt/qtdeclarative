# Generated from signal.pro.

cmake_minimum_required(VERSION 3.16)
project(signal LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/referenceexamples/signal")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(signal
    birthdayparty.cpp birthdayparty.h
    main.cpp
    person.cpp person.h
)
set_target_properties(signal PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(signal PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
)

qt_add_qml_module(signal
    URI People
    VERSION 1.0
    QML_FILES example.qml
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS signal
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
