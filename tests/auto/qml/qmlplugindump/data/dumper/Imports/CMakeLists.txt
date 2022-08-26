# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from imports.pro.

#####################################################################
## Imports Generic Library:
#####################################################################

qt_internal_add_cmake_library(Imports
    MODULE
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        imports.cpp imports.h
        imports_plugin.cpp imports_plugin.h
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:imports.pro:<TRUE>:
# TEMPLATE = "lib"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:imports.pro:NOT _PRO_FILE_PWD____equals____ss_OUT_PWD:
# COPIES = "cp"
# cp.files = "qmldir" "plugins.qmltypes" "CompositeImports.qml" "Derived.qml"
# cp.path = "$$OUT_PWD"

qt_autogen_tools_initial_setup(Imports)
# special case begin
get_target_property(binary_dir Imports BINARY_DIR)
file(COPY qmldir plugins.qmltypes CompositeImports.qml Derived.qml
    DESTINATION ${binary_dir}
)
# special case end
