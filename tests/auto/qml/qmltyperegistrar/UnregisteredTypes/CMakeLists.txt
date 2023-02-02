# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Use NO_GENERATE_QMLTYPES to avoid static asserts during compilation of the types to be tested, same for NO_PLUGIN

qt_policy(SET QTP0001 NEW)

qt_add_qml_module(UnregisteredTypes
    STATIC
    URI UnregisteredTypes
    NO_GENERATE_QMLTYPES
    NO_PLUGIN
    SOURCES uncreatable.h
)

qt_enable_autogen_tool(UnregisteredTypes "moc" ON)
