# Generated from methods.pro.

cmake_minimum_required(VERSION 3.14)
project(methods LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/referenceexamples/methods")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(methods
    birthdayparty.cpp birthdayparty.h
    main.cpp
    person.cpp person.h
)
set_target_properties(methods PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(methods PUBLIC
    Qt::Core
    Qt::Qml
)


# Resources:
set(methods_resource_files
    "example.qml"
)

qt6_add_resources(methods "methods"
    PREFIX
        "/"
    FILES
        ${methods_resource_files}
)

install(TARGETS methods
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(methods PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI People
)

qt6_qml_type_registration(methods)
