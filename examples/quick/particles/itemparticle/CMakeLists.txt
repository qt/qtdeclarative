cmake_minimum_required(VERSION 3.16)
project(itemparticle LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/particles/itemparticle")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

add_subdirectory("../../shared" "shared")

qt_add_executable(itemparticleexample WIN32 MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(itemparticleexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

add_dependencies(itemparticleexample itemparticle_shared)

# Resources:
qt_add_qml_module(itemparticleexample
    URI itemparticle
    VERSION 1.0
    QML_FILES
        "itemparticle.qml"
        "delegates.qml"
        "script.js"
    RESOURCES
        "images/rocket.png"
)

install(TARGETS itemparticleexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(itemparticleexample)
