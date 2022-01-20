cmake_minimum_required(VERSION 3.16)
project(threadedanimation LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/threadedanimation")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(threadedanimation
    main.cpp
    spinner.cpp spinner.h
)

set_target_properties(threadedanimation PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(threadedanimation PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

qt_add_qml_module(threadedanimation
    URI Spinner
    VERSION 1.0
    QML_FILES main.qml
    RESOURCES spinner.png
    RESOURCE_PREFIX /scenegraph/threadedanimation
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS threadedanimation
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
