cmake_minimum_required(VERSION 3.16)
project(imageparticle LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/particles/imageparticle")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

add_subdirectory("../../shared" "shared")

qt_add_executable(imageparticleexample WIN32 MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(imageparticleexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

add_dependencies(imageparticleexample imageparticle_shared)

# Resources:
qt_add_qml_module(imageparticleexample
    URI imageparticle
    VERSION 1.0
    QML_FILES
        "allatonce.qml"
        "colored.qml"
        "colortable.qml"
        "deformation.qml"
        "imageparticle.qml"
        "rotation.qml"
        "sharing.qml"
        "sprites.qml"
    RESOURCES
        "images/bear_tiles.png"
        "images/colortable.png"
        "images/starfish_0.png"
        "images/starfish_1.png"
        "images/starfish_2.png"
        "images/starfish_3.png"
        "images/starfish_4.png"
        "images/flower.png"
)

install(TARGETS imageparticleexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(imageparticleexample)
