# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_library(tst_qqmlimport_dynamic_module SHARED)
qt_autogen_tools_initial_setup(tst_qqmlimport_dynamic_module)

qt_add_qml_module(tst_qqmlimport_dynamic_module
    URI MyDynamicModule

    NO_GENERATE_PLUGIN_SOURCE
    NO_PLUGIN_OPTIONAL
    RESOURCE_PREFIX "/"
)

qt_autogen_tools_initial_setup(tst_qqmlimport_dynamic_moduleplugin)
qt_internal_extend_target(tst_qqmlimport_dynamic_moduleplugin
    SOURCES
        mydynamicmoduleplugin.h
)
