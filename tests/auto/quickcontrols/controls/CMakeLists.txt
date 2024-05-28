# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from controls.pro.

add_subdirectory(basic)
add_subdirectory(fusion)
add_subdirectory(imagine)
add_subdirectory(material)
add_subdirectory(universal)
add_subdirectory(fluentwinui3)
if(MACOS)
    add_subdirectory(macos)
    add_subdirectory(ios)
endif()
if(WIN32)
    add_subdirectory(windows)
endif()
