cmake_minimum_required(VERSION 3.16)
project(quick-accessibility LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/quick-accessibility")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(quick-accessibility
    main.cpp
)

set_target_properties(quick-accessibility PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(quick-accessibility PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

qt_add_qml_module(quick-accessibility
    URI accessibility
    VERSION 1.0
    QML_FILES
        "accessibility.qml"
        "Button.qml"
        "Checkbox.qml"
        "Slider.qml"
)

install(TARGETS quick-accessibility
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
