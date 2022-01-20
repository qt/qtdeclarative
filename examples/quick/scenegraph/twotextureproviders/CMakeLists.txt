cmake_minimum_required(VERSION 3.16)
project(twotextureproviders LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/twotextureproviders")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick ShaderTools)

qt_add_executable(twotextureproviders
    main.cpp
    xorblender.cpp xorblender.h
)

set_target_properties(twotextureproviders PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(twotextureproviders PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

qt_add_qml_module(twotextureproviders
    URI SceneGraphRendering
    VERSION 1.0
    QML_FILES
        main.qml
    RESOURCE_PREFIX /scenegraph/twotextureproviders
    NO_RESOURCE_TARGET_PATH
)

qt6_add_shaders(twotextureproviders "shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/scenegraph/twotextureproviders"
    FILES
        "shaders/xorblender.vert"
        "shaders/xorblender.frag"
        "shaders/checker.frag"
)

install(TARGETS twotextureproviders
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
