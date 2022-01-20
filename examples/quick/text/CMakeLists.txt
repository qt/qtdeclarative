cmake_minimum_required(VERSION 3.16)
project(text LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/text")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

add_subdirectory("../shared" "shared")

qt_add_executable(textexample
    main.cpp
)

set_target_properties(textexample PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(textexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

add_dependencies(textexample text_shared)

qt_add_qml_module(textexample
    URI text
    VERSION 1.0
    QML_FILES
        "fonts/availableFonts.qml"
        "fonts/banner.qml"
        "fonts/fonts.qml"
        "fonts/hello.qml"
        "imgtag/TextWithImage.qml"
        "imgtag/imgtag.qml"
        "styledtext-layout.qml"
        "text.qml"
        "textselection/textselection.qml"
    RESOURCES
        "fonts/content/fonts/tarzeau_ocr_a.ttf"
        "imgtag/images/face-sad.png"
        "imgtag/images/face-smile-big.png"
        "imgtag/images/face-smile.png"
        "imgtag/images/heart200.png"
        "imgtag/images/qtlogo.png"
        "imgtag/images/starfish_2.png"
        "textselection/pics/endHandle.png"
        "textselection/pics/endHandle.sci"
        "textselection/pics/startHandle.png"
        "textselection/pics/startHandle.sci"
)

install(TARGETS textexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(textexample)
