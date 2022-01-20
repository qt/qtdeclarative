cmake_minimum_required(VERSION 3.16)
project(animation LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/animation")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

add_subdirectory("../shared" "shared")

qt_add_executable(animationexample
    main.cpp
)

set_target_properties(animationexample PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(animationexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

add_dependencies(animationexample animation_shared)

qt_add_qml_module(animationexample
    URI animation
    VERSION 1.0
    QML_FILES
        "animation.qml"
        "basics/animators.qml"
        "basics/color-animation.qml"
        "basics/property-animation.qml"
        "behaviors/FocusRect.qml"
        "behaviors/SideRect.qml"
        "behaviors/behavior-example.qml"
        "behaviors/tvtennis.qml"
        "behaviors/wigglytext.qml"
        "easing/easing.qml"
        "pathanimation/pathanimation.qml"
        "pathinterpolator/pathinterpolator.qml"
        "states/states.qml"
        "states/transitions.qml"
    RESOURCES
        "basics/images/face-smile.png"
        "basics/images/moon.png"
        "basics/images/shadow.png"
        "basics/images/star.png"
        "basics/images/sun.png"
        "states/qt-logo.png"
)

install(TARGETS animationexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(animationexample)
