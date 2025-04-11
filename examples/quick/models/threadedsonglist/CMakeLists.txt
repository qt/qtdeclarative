# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(threadedsonglist VERSION 0.1 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appthreadedsonglist WIN32 MACOSX_BUNDLE
    main.cpp
)

qt_add_qml_module(appthreadedsonglist
    URI threadedsonglist
    QML_FILES
        SongListDelegate.qml
        ThreadedSongList.qml
    SOURCES
        datastorage.h
        datastorage.cpp
        mediaelement.h
        mediaelement.cpp
        queueworker.h
        queueworker.cpp
        remotemedia.h
        remotemedia.cpp
        songdatagenerator.h
        songdatagenerator.cpp
        threadedlistmodel.h
        threadedlistmodel.cpp
    RESOURCES
        images/album_covers/classical1.jpeg
        images/album_covers/classical2.jpeg
        images/album_covers/classical3.jpeg
        images/album_covers/classical4.jpeg
        images/album_covers/classical5.jpeg
        images/album_covers/classical6.jpeg
        images/album_covers/metal1.jpeg
        images/album_covers/metal2.jpeg
        images/album_covers/metal3.jpeg
        images/album_covers/metal4.jpeg
        images/album_covers/metal5.jpeg
        images/album_covers/metal6.jpeg
        images/album_covers/pop1.jpeg
        images/album_covers/pop2.jpeg
        images/album_covers/pop3.jpeg
        images/album_covers/pop4.jpeg
        images/album_covers/pop5.jpeg
        images/album_covers/pop6.jpeg
        images/album_covers/rock1.jpeg
        images/album_covers/rock2.jpeg
        images/album_covers/rock3.jpeg
        images/album_covers/rock4.jpeg
        images/album_covers/rock5.jpeg
        images/album_covers/rock6.jpeg
        images/device/remote.jpeg
)

target_link_libraries(appthreadedsonglist
    PRIVATE Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS appthreadedsonglist
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET appthreadedsonglist
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
