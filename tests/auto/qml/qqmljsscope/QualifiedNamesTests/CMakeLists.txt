# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_library(qualified_names_test_module STATIC)
qt_autogen_tools_initial_setup(qualified_names_test_module)

# use PUBLIC everywhere to simplify the build of the test binary
target_include_directories(qualified_names_test_module PUBLIC cpptypes/)
target_link_libraries(qualified_names_test_module PUBLIC Qt::Core Qt::Qml Qt::Gui)

qt6_add_qml_module(qualified_names_test_module
    VERSION 6.0
    PAST_MAJOR_VERSIONS
        1 5
    URI QualifiedNamesTests
    SOURCES
        testtypes.h
    RESOURCE_PREFIX "/"
)

qt_autogen_tools_initial_setup(qualified_names_test_moduleplugin)



