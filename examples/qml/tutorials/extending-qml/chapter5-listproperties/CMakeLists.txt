# Generated from chapter5-listproperties.pro.

cmake_minimum_required(VERSION 3.14)
project(chapter5-listproperties LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/tutorials/extending-qml/chapter5-listproperties")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

qt_add_executable(chapter5-listproperties
    main.cpp
    piechart.cpp piechart.h
    pieslice.cpp pieslice.h
)
set_target_properties(chapter5-listproperties PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(chapter5-listproperties PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(chapter5-listproperties_resource_files
    "app.qml"
)

qt6_add_resources(chapter5-listproperties "chapter5-listproperties"
    PREFIX
        "/"
    FILES
        ${chapter5-listproperties_resource_files}
)

install(TARGETS chapter5-listproperties
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(chapter5-listproperties PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI Charts
)

qt6_qml_type_registration(chapter5-listproperties)
