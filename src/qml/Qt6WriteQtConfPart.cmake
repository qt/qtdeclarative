# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This script runs at build time (via cmake -P) to generate a partial qt.conf
# file (*_qt.part.conf) for a single executable, listing the QML import path
# roots of every non-Qt QML module the executable needs.
#
# The roots come from the union of two sources:
#  - EXTRA_IMPORT_ROOTS: roots derived at configure time from the executable's
#    declared CMake dependencies (its own QML module plus any DEPENDENCIES /
#    IMPORTS TARGET). These are always user modules, never Qt's.
#  - the file produced by qmlimportscanner for the target (<target>_build.cmake),
#    which we include() to get the qml_import_scanner_import_<N> variables.
#    Because it parses the actual .qml files, it also sees QML-only imports that
#    have no CMake dependency. Qt's own modules are filtered out: either by their
#    namespaced LINKTARGET, or - for foundational modules like the base "QML"
#    module that carry no LINKTARGET - by rejecting roots under a Qt import path.
#    Those are found through Qt's configure-time defaults, layered on top via the
#    MergeQtConf mechanism.
#
# If there are no roots at all, a zero-byte file is written. The merge step
# (qmltyperegistrar --merge-qt-conf) keys a directory per non-empty line, so a
# zero-byte partial contributes no qt.conf for that directory. Note that this
# must be zero bytes and not "\n", as a blank line would insert an empty import
# path.
#
# Expected -D arguments:
#   IMPORTS_FILE      - the <target>_build.cmake produced by qmlimportscanner
#   OUTPUT_FILE       - path of the partial *_qt.part.conf to write
#   EXPORT_NAMESPACE  - the Qt CMake export namespace (e.g. Qt6)
#   EXTRA_IMPORT_ROOTS - (optional) roots from declared CMake dependencies
#   QT_IMPORT_PATHS   - (optional) Qt's own QML import paths, used to filter

cmake_minimum_required(VERSION 3.16)

foreach(required_arg IMPORTS_FILE OUTPUT_FILE EXPORT_NAMESPACE)
    if(NOT DEFINED ${required_arg})
        message(FATAL_ERROR "Required argument not provided: ${required_arg}")
    endif()
endforeach()

include("${IMPORTS_FILE}")

# _qt_internal_parse_qml_imports_entry() is shared with Qt6QmlMacros.cmake, which
# is not available in cmake -P script mode. The helpers file sits next to this
# script in the installed package, so include it directly.
include("${CMAKE_CURRENT_LIST_DIR}/${EXPORT_NAMESPACE}QmlPublicCMakeHelpers.cmake")

# Sets out_var to TRUE if path is equal to, or nested under, a Qt import path.
function(_qt_internal_is_qt_import_path path out_var)
    set(${out_var} FALSE PARENT_SCOPE)
    foreach(qt_path IN LISTS QT_IMPORT_PATHS)
        if(qt_path STREQUAL "")
            continue()
        endif()
        cmake_path(IS_PREFIX qt_path "${path}" NORMALIZE is_prefix)
        if(is_prefix)
            set(${out_var} TRUE PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()

set(import_roots "")

# Roots from the executable's declared CMake dependencies.
foreach(root IN LISTS EXTRA_IMPORT_ROOTS)
    if(NOT root STREQUAL "")
        list(APPEND import_roots "${root}")
    endif()
endforeach()

# Roots discovered by qmlimportscanner (covers QML-only imports without a dep).
if(qml_import_scanner_imports_count GREATER 0)
    math(EXPR last_index "${qml_import_scanner_imports_count} - 1")
    foreach(index RANGE 0 ${last_index})
        _qt_internal_parse_qml_imports_entry(entry ${index})

        # Skip Qt's own modules; they are found via Qt's configure-time defaults.
        if("${entry_LINKTARGET}" MATCHES "${EXPORT_NAMESPACE}::")
            continue()
        endif()

        # We need both the module location and its path relative to the import
        # root to be able to derive the root.
        if("${entry_PATH}" STREQUAL "" OR "${entry_RELATIVEPATH}" STREQUAL "")
            continue()
        endif()

        # Derive the import root by going up one directory for each component of
        # the module's relative path, i.e. by subtracting RELATIVEPATH from PATH.
        set(import_root "${entry_PATH}")
        string(REPLACE "/" ";" relative_path_parts "${entry_RELATIVEPATH}")
        foreach(part IN LISTS relative_path_parts)
            get_filename_component(import_root "${import_root}" DIRECTORY)
        endforeach()

        # Skip Qt modules that carry no namespaced LINKTARGET (e.g. the base
        # "QML" module), identified by their root being a Qt import path.
        _qt_internal_is_qt_import_path("${import_root}" is_qt_import_path)
        if(is_qt_import_path)
            continue()
        endif()

        list(APPEND import_roots "${import_root}")
    endforeach()
endif()

if(import_roots)
    list(REMOVE_DUPLICATES import_roots)
    list(JOIN import_roots "\n" import_roots_content)
    file(WRITE "${OUTPUT_FILE}" "${import_roots_content}\n")
else()
    file(WRITE "${OUTPUT_FILE}" "")
endif()
