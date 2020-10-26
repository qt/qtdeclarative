# Generated from extended.pro.

cmake_minimum_required(VERSION 3.14)
project(extended LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/referenceexamples/extended")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(extended
    lineedit.cpp lineedit.h
    main.cpp
)
set_target_properties(extended PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(extended PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Widgets
)


# Resources:
set(extended_resource_files
    "example.qml"
)

qt6_add_resources(extended "extended"
    PREFIX
        "/"
    FILES
        ${extended_resource_files}
)

install(TARGETS extended
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(extended PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI People
)

qt6_qml_type_registration(extended)
