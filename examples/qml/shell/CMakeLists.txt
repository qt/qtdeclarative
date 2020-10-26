# Generated from shell.pro.

cmake_minimum_required(VERSION 3.14)
project(shell LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/shell")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(shell
    main.cpp
)
set_target_properties(shell PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(shell PUBLIC
    Qt::Core
    Qt::Qml
)

if(WIN32)
    set_target_properties(shell PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()

if(APPLE)
    set_target_properties(shell PROPERTIES
        MACOSX_BUNDLE FALSE
    )
endif()

install(TARGETS shell
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
