# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(threading LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/threading")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory("../shared" "shared")

qt_add_executable(threadingexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(threadingexample PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

add_dependencies(threadingexample threading_shared)

qt_add_qml_module(threadingexample
    URI threading
    QML_FILES
        "threadedlistmodel/dataloader.mjs"
        "threadedlistmodel/timedisplay.qml"
        "threading.qml"
        "workerscript/Spinner.qml"
        "workerscript/workerscript.mjs"
        "workerscript/workerscript.qml"
)

install(TARGETS threadingexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(threadingexample)
