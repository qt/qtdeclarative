# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from debugger.pro.

add_subdirectory(qqmldebugjsserver)
add_subdirectory(qqmldebugtranslationservice)
add_subdirectory(qpacketprotocol)
add_subdirectory(qqmlnativeconnector)

if(NOT (CMAKE_CROSSCOMPILING OR (MACOS AND TEST_architecture_arch STREQUAL "x86_64")))
    add_subdirectory(qdebugmessageservice)
    add_subdirectory(qqmldebugtranslationclient)
    add_subdirectory(qqmlenginedebugservice)
    add_subdirectory(qqmldebugjs)
    add_subdirectory(qqmlinspector)
    add_subdirectory(qqmlprofilerservice)
    add_subdirectory(qqmlenginedebuginspectorintegrationtest)
    add_subdirectory(qqmlenginecontrol)
    add_subdirectory(qqmldebuggingenabler)
    add_subdirectory(qqmldebugprocess)
    add_subdirectory(qqmlpreview)
endif()

if(QT_FEATURE_private_tests)
    add_subdirectory(qqmldebugclient)
    add_subdirectory(qqmldebuglocal)
    add_subdirectory(qv4debugger)
    if(NOT (CMAKE_CROSSCOMPILING OR (MACOS AND TEST_architecture_arch STREQUAL "x86_64")))
        add_subdirectory(qqmldebugservice)
    endif()
endif()
