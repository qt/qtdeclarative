# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(windowembedding LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Widgets)
find_package(Qt6 QUIET OPTIONAL_COMPONENTS Multimedia)

qt_standard_project_setup(REQUIRES 6.7)

add_subdirectory("../../../examples/quick/shared" "shared")

qt_add_executable(windowembedding
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(windowembedding PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Widgets
)

add_dependencies(windowembedding windowembedding_shared)

qt_add_qml_module(windowembedding
    URI windowembeddingexample
    QML_FILES
        "windowembedding.qml"
        "QtLogo.qml"
        "examples/clipping.qml"
        "examples/map.qml"
        "examples/stacking.qml"
        "examples/transform.qml"
        "examples/video.qml"
        "examples/webview.qml"
        "examples/widget.qml"
    RESOURCES
        "qtlogo-white.png"
        "qtlogo-green.png"
    NO_LINT
)

if(TARGET Qt::Multimedia)
    target_link_libraries(windowembedding PRIVATE Qt6::Multimedia)
    qt_target_qml_sources(windowembedding
        QML_FILES "examples/hdr.qml"
        RESOURCES "examples/hdrtest.mp4"
    )
endif()

if(APPLE)
    enable_language(OBJCXX)
    set_source_files_properties(main.cpp PROPERTIES LANGUAGE OBJCXX)
    target_link_libraries(windowembedding PRIVATE
        "-framework WebKit"
        "-framework MapKit"
        "-framework AVFoundation"
        "-framework AVKit"
    )
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
        # Xcode fails to sign Contents/PlugIns/shared/qmldir
        set_target_properties(windowembedding PROPERTIES
            XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
        )
    endif()
endif()

install(TARGETS windowembedding
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET windowembedding
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
