# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# NOTE: This code should only ever be executed in script mode. It expects to be
#       used either as part of an install(CODE) call or called by a script
#       invoked via cmake -P as a POST_BUILD step. It would not normally be
#       included directly, it should be pulled in automatically by the deploy
#       support set up by qtbase.

cmake_minimum_required(VERSION 3.16...3.21)

function(qt_deploy_qml_imports)
    set(no_value_options
        NO_QT_IMPORTS
    )
    set(single_value_options
        TARGET
        PLUGINS_DIR     # Internal option, only used for macOS app bundle targets
        QML_DIR
        PLUGINS_FOUND   # Name of an output variable
    )
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unparsed arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT arg_TARGET)
        message(FATAL_ERROR "TARGET must be specified")
    endif()

    if(NOT arg_QML_DIR)
        set(arg_QML_DIR "${QT_DEPLOY_QML_DIR}")
    endif()

    if(NOT arg_PLUGINS_DIR)
        set(arg_PLUGINS_DIR "${QT_DEPLOY_PLUGINS_DIR}")
    endif()

    # The target's finalizer should have written out this file
    string(MAKE_C_IDENTIFIER "${arg_TARGET}" target_id)
    set(filename "${__QT_DEPLOY_IMPL_DIR}/deploy_qml_imports/${target_id}")
    if(__QT_DEPLOY_GENERATOR_IS_MULTI_CONFIG)
        string(APPEND filename "-${__QT_DEPLOY_ACTIVE_CONFIG}")
    endif()
    string(APPEND filename ".cmake")
    if(NOT EXISTS "${filename}")
        message(FATAL_ERROR
            "No QML imports information recorded for target ${arg_TARGET}. "
            "The target must be an executable and qt_add_qml_module() must "
            "have been called with it. If using a CMake version lower than 3.19, ensure "
            "that the executable is manually finalized with qt_finalize_target(). "
            "Missing file:\n    ${filename}"
        )
    endif()
    include(${filename})

endfunction()

function(_qt_internal_deploy_qml_imports_for_target)
    set(no_value_options
        BUNDLE
        NO_QT_IMPORTS
    )
    set(single_value_options
        IMPORTS_FILE
        PLUGINS_FOUND
        QML_DIR
        PLUGINS_DIR
    )
    set(multi_value_options "")

    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()
    foreach(opt IN LISTS single_value_options)
        if(NOT arg_${opt})
            message(FATAL_ERROR "Required argument not provided: ${opt}")
        endif()
    endforeach()

    include("${arg_IMPORTS_FILE}")

    macro(_qt_internal_parse_qml_imports_entry prefix index)
        cmake_parse_arguments("${prefix}"
            ""
            "CLASSNAME;NAME;PATH;PLUGIN;RELATIVEPATH;TYPE;VERSION;LINKTARGET"
            ""
            ${qml_import_scanner_import_${index}}
        )
    endmacro()

    get_filename_component(install_prefix_abs "${QT_DEPLOY_PREFIX}" ABSOLUTE)
    set(plugins_found "")

    if(__QT_DEPLOY_POST_BUILD)
        message(STATUS "Running macOS bundle QML support POST_BUILD routine.")
    endif()

    # Parse the generated cmake file. It is possible for the scanner to find no
    # usage of QML, in which case the import count is 0.
    if(qml_import_scanner_imports_count GREATER 0)
        set(processed_names "")
        math(EXPR last_index "${qml_import_scanner_imports_count} - 1")
        foreach(index RANGE 0 ${last_index})
            _qt_internal_parse_qml_imports_entry(entry ${index})

            if("${entry_NAME}" STREQUAL "")
                message(WARNING "No NAME at scan index ${index}: ${imports_file}")
                continue()
            endif()

            # A plugin might have multiple entries (e.g. for different versions).
            # Only copy it once.
            if("${entry_NAME}" IN_LIST processed_names)
                continue()
            endif()
            # Even if we skip this one, we don't need to process it again
            list(APPEND processed_names ${entry_NAME})

            if("${entry_PATH}" STREQUAL "" OR
                "${entry_PLUGIN}" STREQUAL "" OR
                "${entry_RELATIVEPATH}" STREQUAL "")
                # These might be a valid QML module, but not have a plugin library.
                # We only care about modules that have a plugin we need to copy.
                continue()
            endif()

            if(arg_NO_QT_IMPORTS AND
                    "${entry_LINKTARGET}" MATCHES "${__QT_CMAKE_EXPORT_NAMESPACE}::")
                continue()
            endif()

            # For installation, we want the qmldir file and its plugin. If the
            # CMake project generating the plugin sets version details on its
            # CMake target, we might have symlinks and version numbers in the
            # file names, so account for those. There should never be plugin
            # libraries for more than one QML module in the directory, so we
            # shouldn't need to worry about matching plugins we don't want.
            set(relative_qmldir "${arg_QML_DIR}/${entry_RELATIVEPATH}")
            if("${CMAKE_INSTALL_PREFIX}" STREQUAL "")
                set(install_qmldir "./${relative_qmldir}")
            else()
                set(install_qmldir "${CMAKE_INSTALL_PREFIX}/${relative_qmldir}")
            endif()
            set(dest_qmldir "${QT_DEPLOY_PREFIX}/${relative_qmldir}")
            if(arg_BUNDLE)
                if("${CMAKE_INSTALL_PREFIX}" STREQUAL "")
                    set(install_plugin "./${arg_PLUGINS_DIR}")
                else()
                    set(install_plugin "${CMAKE_INSTALL_PREFIX}/${arg_PLUGINS_DIR}")
                endif()
                set(dest_plugin "${QT_DEPLOY_PREFIX}/${arg_PLUGINS_DIR}")
            else()
                set(install_plugin "${install_qmldir}")
                set(dest_plugin "${dest_qmldir}")
            endif()

            file(INSTALL "${entry_PATH}/qmldir" DESTINATION "${install_qmldir}")

            if(__QT_DEPLOY_POST_BUILD)
                # We are being invoked as a post-build step. The plugin might
                # not exist yet, so we can't even glob for it, let alone copy
                # it. We know what its name should be though, so we can create
                # a symlink to where it will eventually be, which will be enough
                # to allow it to run from the build tree. It won't matter if
                # the plugin gets updated later in the build, the symlink will
                # still be pointing at the right location.
                # In theory, this could be possible for any platform that
                # supports symlinks (which all do in some form now, even
                # Windows if the right permissions are set), but we only really
                # expect to need this for macOS app bundles.
                set(final_destination "${dest_qmldir}/lib${entry_PLUGIN}.dylib")
                message(STATUS "Symlinking: ${final_destination}")
                file(CREATE_LINK
                    "${entry_PATH}/lib${entry_PLUGIN}.dylib"
                    "${final_destination}"
                    SYMBOLIC
                )

                # We don't add this plugin to plugins_found because we don't
                # actually make a copy of the plugin. We don't want the caller
                # thinking they should further process what would still be the
                # original plugin in the build tree.
                continue()
            endif()

            # Construct a regular expression that matches the plugin's file name.
            set(plugin_regex "^(.*/)?(lib)?${entry_PLUGIN}")
            if(__QT_DEPLOY_QT_IS_MULTI_CONFIG_BUILD_WITH_DEBUG)
                # If our application is a release build, do not match any debug suffix.
                # If our application is a debug build, match exactly a debug suffix.
                if(__QT_DEPLOY_ACTIVE_CONFIG STREQUAL "Debug")
                    string(APPEND plugin_regex "${__QT_DEPLOY_QT_DEBUG_POSTFIX}")
                endif()
            else()
                # The Qt installation does only contain one build of the plugin. We match any
                # possible debug suffix, or none.
                string(APPEND plugin_regex ".*")
            endif()
            string(APPEND plugin_regex "\\.(so|dylib|dll)(\\.[0-9]+)*$")

            file(GLOB files LIST_DIRECTORIES false "${entry_PATH}/*${entry_PLUGIN}*")
            list(FILTER files INCLUDE REGEX "${plugin_regex}")
            file(INSTALL ${files} DESTINATION "${install_plugin}" USE_SOURCE_PERMISSIONS)

            get_filename_component(dest_plugin_abs "${dest_plugin}" ABSOLUTE)
            if(__QT_DEPLOY_TOOL STREQUAL "GRD")
                # Use the full plugin path for deployment. This is necessary for file(GRD) to
                # resolve the dependencies of the plugins.
                list(APPEND plugins_found ${files})
            else()
                # Use relative paths for the plugins. If we used full paths here, macdeployqt would
                # modify the RPATHS of plugins in the Qt installation.
                file(RELATIVE_PATH rel_path "${install_prefix_abs}" "${dest_plugin_abs}")
                foreach(file IN LISTS files)
                    get_filename_component(filename "${file}" NAME)
                    list(APPEND plugins_found "${rel_path}/${filename}")
                endforeach()
            endif()

            if(__QT_DEPLOY_TOOL STREQUAL "GRD" AND __QT_DEPLOY_MUST_ADJUST_PLUGINS_RPATH)
                # The RPATHs of the installed plugins do not match Qt's original lib directory.
                # We must set the RPATH to point to QT_DEPLOY_LIBDIR.
                _qt_internal_get_rpath_origin(rpath_origin)
                foreach(file_path IN LISTS files)
                    get_filename_component(file_name ${file_path} NAME)
                    file(RELATIVE_PATH rel_lib_dir "${dest_plugin}"
                        "${QT_DEPLOY_PREFIX}/${QT_DEPLOY_LIB_DIR}"
                    )
                    _qt_internal_set_rpath(
                        FILE "${dest_plugin}/${file_name}"
                        NEW_RPATH "${rpath_origin}/${rel_lib_dir}"
                    )
                endforeach()
            endif()

            if(arg_BUNDLE)
                # Actual plugin binaries will be in PlugIns, but qmldir files
                # expect them to be in the same directory as themselves
                # (i.e. under Resources/qml/...). Add a symlink at the place
                # the qmldir expects the binary to be. This arrangement keeps
                # binaries under PlugIns and non-binaries under Resources,
                # which is required for code signing to work properly.
                get_filename_component(dest_qmldir_abs "${dest_qmldir}" ABSOLUTE)
                file(RELATIVE_PATH rel_path "${dest_qmldir_abs}" "${dest_plugin_abs}")
                foreach(plugin_file IN LISTS files)
                    get_filename_component(filename "${plugin_file}" NAME)

                    set(final_destination "${dest_qmldir}/${filename}")
                    message(STATUS "Symlinking: ${final_destination}")
                    file(CREATE_LINK "${rel_path}/${filename}" "${final_destination}" SYMBOLIC)
                endforeach()
            endif()
        endforeach()
    endif()

    set(${arg_PLUGINS_FOUND} ${plugins_found} PARENT_SCOPE)

endfunction()

function(_qt_internal_show_skip_qml_runtime_deploy_message)
    # Don't show the message in static Qt builds, it can be misleading, because we still
    # run qmlimportscanner / link the static qml plguins into the binary despite not having
    # a qml deployment step.
    if(__QT_DEPLOY_IS_SHARED_LIBS_BUILD)
        message(STATUS "Skipping QML module deployment steps.")
    endif()
endfunction()
