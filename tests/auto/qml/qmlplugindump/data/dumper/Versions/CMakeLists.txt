# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from versions.pro.

#####################################################################
## Versions Generic Library:
#####################################################################

qt_internal_add_cmake_library(Versions
    MODULE
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        versions.cpp versions.h
        versions_plugin.cpp versions_plugin.h
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:versions.pro:<TRUE>:
# TEMPLATE = "lib"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:versions.pro:NOT _PRO_FILE_PWD____equals____ss_OUT_PWD:
# COPIES = "cpqmldir"
# cpqmldir.files = "qmldir" "plugins.qmltypes"
# cpqmldir.path = "$$OUT_PWD"

qt_autogen_tools_initial_setup(Versions)
# special case begin
get_target_property(binary_dir Versions BINARY_DIR)
file(COPY qmldir plugins.qmltypes
    DESTINATION ${binary_dir}
)
# special case end
