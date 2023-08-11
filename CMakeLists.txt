# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qtdeclarative.pro.

cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)
project(QtDeclarative # special case
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Declarative Libraries" # special case
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core) # special case
find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS Gui Network Widgets OpenGL OpenGLWidgets Sql Concurrent Test LanguageServerPrivate LinguistTools Svg)
qt_internal_project_setup()

# Set up QT_HOST_PATH as an extra root path to look for the ShaderToolsTools package
# when cross-compiling.
if(NOT "${QT_HOST_PATH}" STREQUAL "")
     set(_qt_backup_qtdeclarative_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
     set(_qt_backup_qtdeclarative_CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH})
     list(PREPEND CMAKE_PREFIX_PATH "${QT_HOST_PATH_CMAKE_DIR}")
     list(PREPEND CMAKE_FIND_ROOT_PATH "${QT_HOST_PATH}")
endif()

# This can't use the find_package(Qt6 COMPONENTS) signature, because Qt6Config uses NO_DEFAULT and
# won't look at the prepend extra find root paths.
find_package(Qt6ShaderToolsTools ${PROJECT_VERSION} QUIET CONFIG
    PATHS
            ${_qt_additional_packages_prefix_path}
            ${_qt_additional_packages_prefix_path_env}
)

if(NOT "${QT_HOST_PATH}" STREQUAL "")
     set(CMAKE_PREFIX_PATH ${_qt_backup_qtdeclarative_CMAKE_PREFIX_PATH})
     set(CMAKE_FIND_ROOT_PATH ${_qt_backup_qtdeclarative_CMAKE_FIND_ROOT_PATH})
endif()

# Optionally look for the target ShaderTools package to avoid warnings
# when configuring the project to build examples in-tree in a prefix Qt build.
# See QTBUG-96358 for details.
find_package(Qt6 ${PROJECT_VERSION} CONFIG OPTIONAL_COMPONENTS ShaderTools)

# special case begin
# export QT6_ADD_QML_MODULE to this project
include(src/qml/Qt6QmlMacros.cmake)
if(ANDROID)
    include("${CMAKE_CURRENT_LIST_DIR}/src/qml/Qt6AndroidQmlMacros.cmake")
endif()
# special case end

if(NOT QT_FEATURE_commandlineparser)
    message(NOTICE "Skipping the build as the condition \"QT_FEATURE_commandlineparser\" is not met.")
    return()
endif()
if(NOT QT_FEATURE_temporaryfile)
    message(NOTICE "Skipping the build as the condition \"QT_FEATURE_temporaryfile\" is not met.")
    return()
endif()
qt_build_repo()
