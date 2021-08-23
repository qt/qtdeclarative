# Generated from qmlextensionplugins.pro.

cmake_minimum_required(VERSION 3.16)
project(qmlqtimeexample LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/qmlextensionplugins/imports/TimeExample")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)

set(qml_files
    imports/TimeExample/Clock.qml
)
set(images
    imports/TimeExample/center.png
    imports/TimeExample/clock.png
    imports/TimeExample/hour.png
    imports/TimeExample/minute.png
)
foreach(file IN LISTS qml_files images)
    get_filename_component(filename ${file} NAME)
    set_source_files_properties(${file} PROPERTIES QT_RESOURCE_ALIAS ${filename})
endforeach()

qt6_add_qml_module(qmlqtimeexample
    OUTPUT_DIRECTORY TimeExample
    VERSION 1.0
    URI "TimeExample"
    SOURCES timemodel.cpp timemodel.h
    QML_FILES ${qml_files}
    RESOURCES ${images}
)
set_target_properties(qmlqtimeexample PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(qmlqtimeexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
)

install(TARGETS qmlqtimeexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
