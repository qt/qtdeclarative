# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_library(tst_qmltyperegistrar_major_version_zero)
qt_autogen_tools_initial_setup(tst_qmltyperegistrar_major_version_zero)
target_link_libraries(tst_qmltyperegistrar_major_version_zero PRIVATE Qt::Core Qt::Qml)
qt_enable_autogen_tool(tst_qmltyperegistrar_major_version_zero "moc" ON)
qt_add_qml_module(tst_qmltyperegistrar_major_version_zero
    URI VersionZero
    VERSION 0.1
    SOURCES
        version_zero_type.h
    RESOURCE_PREFIX "/"
)
qt_autogen_tools_initial_setup(tst_qmltyperegistrar_major_version_zeroplugin)

# Make sure the backing library is found on Windows next to the executable
set_target_properties(
    tst_qmltyperegistrar_major_version_zero
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
)
