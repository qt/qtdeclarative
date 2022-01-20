cmake_minimum_required(VERSION 3.16)
project(vulkanunderqml LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/vulkanunderqml")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(vulkanunderqml
    main.cpp
    vulkansquircle.cpp vulkansquircle.h
)

set_target_properties(vulkanunderqml PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE

    # Prevent name clash with build subdirectory on case-insensitive file systems
    OUTPUT_NAME vulkanunderqmlapp
)

target_link_libraries(vulkanunderqml PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

qt_add_qml_module(vulkanunderqml
    URI VulkanUnderQML
    VERSION 1.0
    QML_FILES
        main.qml
    RESOURCES
        squircle.frag.spv
        squircle.vert.spv
    RESOURCE_PREFIX /scenegraph/vulkanunderqml
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS vulkanunderqml
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
