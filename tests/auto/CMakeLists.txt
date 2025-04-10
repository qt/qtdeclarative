# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT (WIN32 AND CMAKE_CROSSCOMPILING))
    add_subdirectory(cmake)
endif()

# For now, only build a subset of CMake auto tests when QT_BUILD_MINIMAL_STATIC_TEST
# is specified and the build is targeting iOS. QT_BUILD_MINIMAL_STATIC_TEST is used in our CI.
# Regular tests shouldn't be built because the CI will try to run them and fail due to missing
# simulator support.
if(IOS AND QT_BUILD_MINIMAL_STATIC_TESTS)
    return()
endif()

if(TARGET Qt::Quick)
    add_subdirectory(qml)
    add_subdirectory(quick)
    add_subdirectory(quickshapes)
endif()

# Limit set of tests to run for static Qt builds.
if(QT_BUILD_MINIMAL_STATIC_TESTS)
    return()
endif()

if(TARGET Qt::Quick)
    add_subdirectory(quicktest)
endif()
if(TARGET Qt::QuickTest)
    add_subdirectory(core)
    add_subdirectory(qmlnetwork)
endif()
add_subdirectory(toolsupport)
if(NOT UIKIT AND NOT ANDROID AND NOT QNX AND NOT VXWORKS) # FIXME: QTBUG-92591 QTBUG-100202
    add_subdirectory(qmltest)
endif()
if(TARGET Qt::Quick)
    add_subdirectory(particles)
endif()
if(TARGET Qt::QuickWidgets)
    add_subdirectory(quickwidgets)
endif()
if(TARGET Qt::QmlDomPrivate AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(qmldom)
    add_subdirectory(qmlls)
endif()
if(TARGET Qt::QuickTemplates2)
    add_subdirectory(quickcontrols)
    add_subdirectory(quickdialogs)
endif()
