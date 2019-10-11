# Generated from rendernode.pro.

cmake_minimum_required(VERSION 3.14)
project(rendernode LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/quick/scenegraph/rendernode")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(rendernode
    customrenderitem.cpp customrenderitem.h
    main.cpp
    openglrenderer.cpp openglrenderer.h
    softwarerenderer.cpp softwarerenderer.h
)
target_link_libraries(rendernode PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

# Resources:
set(rendernode_resource_files
    "main.qml"
    "metalshader.frag"
    "metalshader.vert"
    "shader_frag.cso"
    "shader_vert.cso"
)

qt6_add_resources(rendernode "rendernode"
    PREFIX
        "/scenegraph/rendernode"
    FILES
        ${rendernode_resource_files}
)


if(QT_FEATURE_d3d12)
    target_sources(rendernode PUBLIC
        d3d12renderer.cpp d3d12renderer.h
    )

    target_link_libraries(rendernode PUBLIC
        d3d12
    )

endif()

if(APPLE_OSX)
    target_sources(rendernode PUBLIC
        metalrenderer.h metalrenderer.mm
    )

    target_link_libraries(rendernode PUBLIC
        "-framework AppKit"
        "-framework Metal"
    )

endif()

install(TARGETS rendernode
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
