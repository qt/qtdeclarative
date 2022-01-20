cmake_minimum_required(VERSION 3.16)
project(shapes LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/shapes")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

add_subdirectory("../shared" "shared")

qt_add_executable(shapesexample
    main.cpp
)

set_target_properties(shapesexample PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(shapesexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

add_dependencies(shapesexample shapes_shared)

qt_add_qml_module(shapesexample
    URI shapes
    VERSION 1.0
    QML_FILES
        "clippedtigers.qml"
        "interactive.qml"
        "cubicCurve.qml"
        "ellipticalArcs.qml"
        "gradientSpreadModes.qml"
        "arcDirection.qml"
        "largeOrSmallArc.qml"
        "arcRotation.qml"
        "tigerLoader.qml"
        "text.qml"
        "strokeOrFill.qml"
        "dashPattern.qml"
        "linearGradient.qml"
        "radialGradient.qml"
        "fillRules.qml"
        "joinStyles.qml"
        "capStyles.qml"
        "quadraticCurve.qml"
        "main.qml"
        "sampling.qml"
        "shapegallery.qml"
        "tapableTriangle.qml"
        "tiger.qml"
)

install(TARGETS shapesexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(shapesexample)
