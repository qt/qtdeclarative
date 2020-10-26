# Generated from properties.pro.

cmake_minimum_required(VERSION 3.14)
project(properties LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/referenceexamples/properties")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(properties
    birthdayparty.cpp birthdayparty.h
    main.cpp
    person.cpp person.h
)
set_target_properties(properties PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(properties PUBLIC
    Qt::Core
    Qt::Qml
)


# Resources:
set(properties_resource_files
    "example.qml"
)

qt6_add_resources(properties "properties"
    PREFIX
        "/"
    FILES
        ${properties_resource_files}
)

install(TARGETS properties
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(properties PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI People
)

qt6_qml_type_registration(properties)
