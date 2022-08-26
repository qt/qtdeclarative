# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from dummy.pro.

#####################################################################
## Dummy Generic Library:
#####################################################################

qt_internal_add_cmake_library(Dummy
    MODULE
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        dummy.cpp dummy.h
        dummy_plugin.cpp dummy_plugin.h
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:dummy.pro:<TRUE>:
# TEMPLATE = "lib"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:dummy.pro:NOT _PRO_FILE_PWD____equals____ss_OUT_PWD:
# COPIES = "cp"
# cp.files = "qmldir" "plugins.qmltypes"
# cp.path = "$$OUT_PWD"

qt_autogen_tools_initial_setup(Dummy)
# special case begin
get_target_property(binary_dir Dummy BINARY_DIR)
file(COPY qmldir plugins.qmltypes
    DESTINATION ${binary_dir}
)
# special case end
