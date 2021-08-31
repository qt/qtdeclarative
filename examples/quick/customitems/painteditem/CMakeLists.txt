cmake_minimum_required(VERSION 3.16)
project(painteditem LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/customitems/painteditem")

find_package(Qt6 COMPONENTS Core Gui Quick Qml)

add_subdirectory(TextBalloon)

qt_add_executable(painteditemexample WIN32 MACOSX_BUNDLE main.cpp)

qt_add_qml_module(painteditemexample
    URI painteditem
    VERSION 1.0
    QML_FILES
        "textballoons.qml"
)

target_link_libraries(painteditemexample PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
    qmltextballoon
)

install(TARGETS painteditemexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
