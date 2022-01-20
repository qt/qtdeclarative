cmake_minimum_required(VERSION 3.16)
project(touchinteraction LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/touchinteraction")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

add_subdirectory("../shared" "shared")

qt_add_executable(touchinteractionexample
    main.cpp
)

set_target_properties(touchinteractionexample PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(touchinteractionexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

add_dependencies(touchinteractionexample touchinteraction_shared)

qt_add_qml_module(touchinteractionexample
    URI touchinteraction
    VERSION 1.0
    QML_FILES
        "flickable/Panel.qml"
        "flickable/basic-flickable.qml"
        "flickable/corkboards.qml"
        "multipointtouch/AugmentedTouchPoint.qml"
        "multipointtouch/BearWhackParticleSystem.qml"
        "multipointtouch/ParticleFlame.qml"
        "multipointtouch/bearwhack.qml"
        "multipointtouch/multiflame.qml"
        "pincharea/flickresize.qml"
        "touchinteraction.qml"
    RESOURCES
        "flickable/cork.jpg"
        "flickable/note-yellow.png"
        "flickable/tack.png"
        "multipointtouch/Bear0.png"
        "multipointtouch/Bear1.png"
        "multipointtouch/Bear2.png"
        "multipointtouch/Bear3.png"
        "multipointtouch/BearB.png"
        "multipointtouch/blur-circle.png"
        "multipointtouch/blur-circle3.png"
        "multipointtouch/heart-blur.png"
        "multipointtouch/title.png"
        "pincharea/qt-logo.jpg"
)

install(TARGETS touchinteractionexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(touchinteractionexample)
