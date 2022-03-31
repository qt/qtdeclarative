cmake_minimum_required(VERSION 3.16)
project(qquickwidgetversuswindow_opengl LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/quickwidgets/qquickwidgetversuswindow_opengl")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickWidgets ShaderTools Widgets)

qt_add_executable(qquickwidgetversuswindow_opengl
    fbitem.cpp fbitem.h
    logo.cpp logo.h
    main.cpp
    mainwindow.cpp mainwindow.h
)

set_target_properties(qquickwidgetversuswindow_opengl PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(qquickwidgetversuswindow_opengl PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::QuickWidgets
    Qt::Widgets
)

qt_add_qml_module(qquickwidgetversuswindow_opengl
    URI fbitem
    VERSION 1.0
    QML_FILES test.qml
    RESOURCE_PREFIX /qquickwidgetversuswindow_opengl
    NO_RESOURCE_TARGET_PATH
)

qt6_add_shaders(qquickwidgetversuswindow_opengl "shaders"
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/qquickwidgetversuswindow_opengl"
    FILES
        "wobble.frag"
)

install(TARGETS qquickwidgetversuswindow_opengl
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
