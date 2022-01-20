cmake_minimum_required(VERSION 3.16)
project(system LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/particles/system")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

add_subdirectory("../../shared" "shared")

qt_add_executable(systemexample WIN32 MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(systemexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

add_dependencies(systemexample system_shared)

# Resources:
qt_add_qml_module(systemexample
    URI system
    VERSION 1.0
    QML_FILES
        "dynamiccomparison.qml"
        "dynamicemitters.qml"
        "multiplepainters.qml"
        "timedgroupchanges.qml"
        "startstop.qml"
        "system.qml"
    RESOURCES
        "images/sparkleSize.png"
        "images/particle2.png"
        "images/particle3.png"
)

install(TARGETS systemexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(systemexample)
