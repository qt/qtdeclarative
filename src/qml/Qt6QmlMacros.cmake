#
# Q6QmlMacros
#

set(__qt_qml_macros_module_base_dir "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")

function(qt6_add_qml_module target)
    set(args_option
        STATIC
        SHARED
        DESIGNER_SUPPORTED
        NO_PLUGIN
        NO_PLUGIN_OPTIONAL
        NO_CREATE_PLUGIN_TARGET
        NO_GENERATE_PLUGIN_SOURCE
        NO_GENERATE_QMLTYPES
        NO_GENERATE_QMLDIR
        NO_LINT
        NO_CACHEGEN
        NO_RESOURCE_TARGET_PATH
        # TODO: Remove once all usages have also been removed
        SKIP_TYPE_REGISTRATION

        # Used only by _qt_internal_qml_type_registration()
        # TODO: Remove this once qt6_extract_metatypes does not install by default.
        __QT_INTERNAL_INSTALL_METATYPES_JSON
    )

    set(args_single
        PLUGIN_TARGET
        INSTALLED_PLUGIN_TARGET  # Internal option only, it may be removed
        OUTPUT_TARGETS
        RESOURCE_PREFIX
        URI
        TARGET_PATH   # Internal option only, it may be removed
        VERSION
        OUTPUT_DIRECTORY
        CLASS_NAME
        CLASSNAME  # TODO: For backward compatibility, remove once all repos no longer use it
        TYPEINFO
        # TODO: We don't handle installation, warn if callers used these with the old
        #       API and eventually remove them once we have updated all other repos
        RESOURCE_EXPORT
        INSTALL_DIRECTORY
        INSTALL_LOCATION
    )

    set(args_multi
        SOURCES
        QML_FILES
        RESOURCES
        IMPORTS
        IMPORT_PATH
        OPTIONAL_IMPORTS
        DEPENDENCIES
        PAST_MAJOR_VERSIONS
    )

    cmake_parse_arguments(PARSE_ARGV 1 arg
       "${args_option}"
       "${args_single}"
       "${args_multi}"
    )
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown/unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    # Warn about options we no longer need/use (these were used by the internal
    # targets and examples, but the logic has been shifted to
    # qt_internal_add_qml_module() or left as a responsibility of the caller).
    if(DEFINED arg_RESOURCE_EXPORT)
        message(AUTHOR_WARNING
            "RESOURCE_EXPORT will be ignored. This function does not handle "
            "installation, which is what RESOURCE_EXPORT was previously used "
            "for. Please update your project to install the target directly."
        )
    endif()

    if(DEFINED arg_INSTALL_DIRECTORY)
        message(AUTHOR_WARNING
            "INSTALL_DIRECTORY will be ignored. This function does not handle "
            "installation, please update your project to install the target "
            "directly."
        )
    endif()

    if(DEFINED arg_INSTALL_LOCATION)
        message(AUTHOR_WARNING
            "INSTALL_LOCATION will be ignored. This function does not handle "
            "installation, please update your project to install the target "
            "directly."
        )
    endif()

    if(arg_SKIP_TYPE_REGISTRATION)
        message(AUTHOR_WARNING
            "SKIP_TYPE_REGISTRATION is no longer used and will be ignored."
        )
    endif()

    # Mandatory arguments
    if (NOT arg_URI)
        message(FATAL_ERROR
            "Called without a module URI. Please specify one using the URI argument."
        )
    endif()

    if (NOT arg_VERSION)
        message(FATAL_ERROR
            "Called without a module version. Please specify one using the VERSION argument."
        )
    endif()

    if ("${arg_VERSION}" MATCHES "^([0-9]+\\.[0-9]+)\\.[0-9]+$")
        set(arg_VERSION "${CMAKE_MATCH_1}")
    endif()

    if (NOT "${arg_VERSION}" MATCHES "^[0-9]+\\.[0-9]+$")
        message(FATAL_ERROR
            "Called with an invalid version argument: '${arg_VERSION}'. "
            "Expected version in the form: VersionMajor.VersionMinor."
        )
    endif()

    # Other arguments and checking for invalid combinations
    if (NOT arg_TARGET_PATH)
        # NOTE: This will always be used for copying things to the build
        #       directory, but it will not be used for resource paths if
        #       NO_RESOURCE_TARGET_PATH was given.
        string(REPLACE "." "/" arg_TARGET_PATH ${arg_URI})
    endif()

    if(arg_NO_PLUGIN AND DEFINED arg_PLUGIN_TARGET)
        message(FATAL_ERROR
            "NO_PLUGIN was specified, but PLUGIN_TARGET was also given. "
            "At most one of these can be present."
            )
    endif()

    set(is_executable FALSE)
    if(TARGET ${target})
        if(arg_STATIC OR arg_SHARED)
            message(FATAL_ERROR
                "Cannot use STATIC or SHARED keyword when passed an existing target (${target})"
                )
        endif()

        # With CMake 3.17 and earlier, a source file's generated property isn't
        # visible outside of the directory scope in which it is set. That can
        # lead to build errors for things like type registration due to CMake
        # thinking nothing will create a missing file on the first run. With
        # CMake 3.18 or later, we can force that visibility. Policy CMP0118
        # added in CMake 3.20 should have made this unnecessary, but we can't
        # rely on that because the user project controls what it is set to at
        # the point where it matters, which is the end of the target's
        # directory scope (hence we can't even test for it here).
        get_target_property(source_dir ${target} SOURCE_DIR)
        if(NOT source_dir STREQUAL CMAKE_CURRENT_SOURCE_DIR AND
           CMAKE_VERSION VERSION_LESS "3.18")
            message(WARNING
                "qt6_add_qml_module() is being called in a different "
                "directory scope to the one in which the target \"${target}\" "
                "was created. CMake 3.18 or later is required to generate a "
                "project robustly for this scenario, but you are using "
                "CMake ${CMAKE_VERSION}. Ideally, qt6_add_qml_module() should "
                "only be called from the same scope as the one the target was "
                "created in to avoid dependency and visibility problems."
            )
        endif()

        get_target_property(backing_target_type ${target} TYPE)
        get_target_property(is_android_executable "${target}" _qt_is_android_executable)
        if (backing_target_type STREQUAL "EXECUTABLE" OR is_android_executable)
            if(DEFINED arg_PLUGIN_TARGET)
                message(FATAL_ERROR
                    "A QML module with an executable as its backing target "
                    "cannot have a plugin."
                )
            endif()
            if(arg_NO_CREATE_PLUGIN_TARGET)
                message(WARNING
                    "A QML module with an executable as its backing target "
                    "cannot have a plugin. The NO_CREATE_PLUGIN_TARGET option "
                    "has no effect and should be removed."
                )
            endif()
            set(arg_NO_PLUGIN TRUE)
            set(lib_type "")
            set(is_executable TRUE)
        elseif(arg_NO_RESOURCE_TARGET_PATH)
            message(FATAL_ERROR
                "NO_RESOURCE_TARGET_PATH cannot be used for a backing target "
                "that is not an executable"
            )
        elseif(backing_target_type STREQUAL "STATIC_LIBRARY")
            set(lib_type STATIC)
        elseif(backing_target_type MATCHES "(SHARED|MODULE)_LIBRARY")
            set(lib_type SHARED)
        else()
            message(FATAL_ERROR "Unsupported backing target type: ${backing_target_type}")
        endif()
    else()
        if(arg_STATIC AND arg_SHARED)
            message(FATAL_ERROR
                "Both STATIC and SHARED specified, at most one can be given"
                )
        endif()

        if(arg_NO_RESOURCE_TARGET_PATH)
            message(FATAL_ERROR
                "NO_RESOURCE_TARGET_PATH can only be provided when an existing "
                "executable target is passed in as the backing target"
            )
        endif()

        # Explicit arguments take precedence, otherwise default to using the same
        # staticality as what Qt was built with. This follows the already
        # established default behavior for building ordinary Qt plugins.
        # We don't allow the standard CMake BUILD_SHARED_LIBS variable to control
        # the default because that can lead to different defaults depending on
        # whether you build with a separate backing target or not.
        if(arg_STATIC)
            set(lib_type STATIC)
        elseif(arg_SHARED)
            set(lib_type SHARED)
        elseif(QT6_IS_SHARED_LIBS_BUILD)
            set(lib_type SHARED)
        else()
            set(lib_type STATIC)
        endif()
    endif()

    if(arg_NO_PLUGIN)
        # Simplifies things a bit further below
        set(arg_PLUGIN_TARGET "")
    elseif(NOT DEFINED arg_PLUGIN_TARGET)
        if(arg_NO_CREATE_PLUGIN_TARGET)
            # We technically could allow this and rely on the project using the
            # default plugin target name, but not doing so gives us the
            # flexibility to potentially change that default later if needed.
            message(FATAL_ERROR
                "PLUGIN_TARGET must also be provided when NO_CREATE_PLUGIN_TARGET "
                "is used. If you want to disable creating a plugin altogether, "
                "use the NO_PLUGIN option instead."
            )
        endif()
        set(arg_PLUGIN_TARGET ${target}plugin)
    endif()
    if(arg_NO_CREATE_PLUGIN_TARGET AND arg_PLUGIN_TARGET STREQUAL target AND NOT TARGET ${target})
        message(FATAL_ERROR
            "PLUGIN_TARGET is the same as the backing target, which is allowed, "
            "but NO_CREATE_PLUGIN_TARGET was also given and the target does not "
            "exist. Either ensure the target is already created or do not "
            "specify NO_CREATE_PLUGIN_TARGET."
        )
    endif()
    if(NOT arg_INSTALLED_PLUGIN_TARGET)
        set(arg_INSTALLED_PLUGIN_TARGET ${arg_PLUGIN_TARGET})
    endif()

    set(no_gen_source)
    if(arg_NO_GENERATE_PLUGIN_SOURCE)
        set(no_gen_source NO_GENERATE_PLUGIN_SOURCE)
    endif()

    if(arg_OUTPUT_DIRECTORY)
        get_filename_component(arg_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
            ABSOLUTE BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}"
        )
    else()
        if("${QT_QML_OUTPUT_DIRECTORY}" STREQUAL "")
            set(arg_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
            # For libraries, we assume/require that the source directory
            # structure is consistent with the target path. For executables,
            # the source directory will usually not reflect the target path
            # and the project will often expect to be able to use resource
            # paths that don't include the target path (they need the
            # NO_RESOURCE_TARGET_PATH option if they do that). Tooling always
            # needs the target path in the file system though, so the output
            # directory should always have it. Handle the special case for
            # executables to ensure this is what we get.
            if(is_executable)
                string(APPEND arg_OUTPUT_DIRECTORY "/${arg_TARGET_PATH}")
            endif()
        else()
            if(NOT IS_ABSOLUTE "${QT_QML_OUTPUT_DIRECTORY}")
                message(FATAL_ERROR
                    "QT_QML_OUTPUT_DIRECTORY must be an absolute path, but given: "
                    "${QT_QML_OUTPUT_DIRECTORY}"
                )
            endif()
            # This inherently does what we want for libraries and executables
            set(arg_OUTPUT_DIRECTORY ${QT_QML_OUTPUT_DIRECTORY}/${arg_TARGET_PATH})
        endif()
    endif()

    # Sanity check that we are not trying to have two different QML modules use
    # the same output directory.
    get_property(dirs GLOBAL PROPERTY _qt_all_qml_output_dirs)
    if(dirs)
        list(FIND dirs "${arg_OUTPUT_DIRECTORY}" index)
        if(NOT index EQUAL -1)
            get_property(qml_targets GLOBAL PROPERTY _qt_all_qml_targets)
            list(GET qml_targets ${index} other_target)
            message(FATAL_ERROR
                "Output directory for target \"${target}\" is already used by "
                "another QML module (target \"${other_target}\"). "
                "Output directory is:\n  ${arg_OUTPUT_DIRECTORY}\n"
            )
        endif()
    endif()
    set_property(GLOBAL APPEND PROPERTY _qt_all_qml_output_dirs ${arg_OUTPUT_DIRECTORY})
    set_property(GLOBAL APPEND PROPERTY _qt_all_qml_targets     ${target})

    # TODO: Support for old keyword, remove once all repos no longer use CLASSNAME
    if(arg_CLASSNAME)
        if(arg_CLASS_NAME AND NOT arg_CLASSNAME STREQUAL arg_CLASS_NAME)
            message(FATAL_ERROR
                "Both CLASSNAME and CLASS_NAME were given and were different. "
                "Update call site to only use CLASS_NAME."
            )
        endif()
        set(arg_CLASS_NAME "${arg_CLASSNAME}")
        unset(arg_CLASSNAME)
    endif()

    if(NOT arg_CLASS_NAME AND TARGET "${arg_PLUGIN_TARGET}")
        get_target_property(class_name ${arg_PLUGIN_TARGET} QT_PLUGIN_CLASS_NAME)
        if(class_name)
            set(arg_CLASS_NAME)
        endif()
    endif()
    if(NOT arg_CLASS_NAME)
        _qt_internal_compute_qml_plugin_class_name_from_uri("${arg_URI}" arg_CLASS_NAME)
    endif()

    if(TARGET ${target})
        if(arg_PLUGIN_TARGET STREQUAL target)
            # Insert the plugin's URI into its meta data to enable usage
            # of static plugins in QtDeclarative (like in mkspecs/features/qml_plugin.prf).
            set_property(TARGET ${target} APPEND PROPERTY
                AUTOMOC_MOC_OPTIONS "-Muri=${arg_URI}"
            )
        endif()
    else()
        if(arg_PLUGIN_TARGET STREQUAL target)
            qt6_add_qml_plugin(${target}
                ${lib_type}
                ${no_gen_source}
                OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
                URI ${arg_URI}
                CLASS_NAME ${arg_CLASS_NAME}
            )
        else()
            qt6_add_library(${target} ${lib_type})
        endif()
    endif()

    if(NOT target STREQUAL Qml)
        target_link_libraries(${target} PRIVATE ${QT_CMAKE_EXPORT_NAMESPACE}::Qml)
    endif()

    if(NOT arg_TYPEINFO)
        set(arg_TYPEINFO ${target}.qmltypes)
    endif()

    foreach(import_set IN ITEMS IMPORTS OPTIONAL_IMPORTS)
        foreach(import IN LISTS arg_${import_set})
            string(FIND ${import} "/" slash_position REVERSE)
            if (slash_position EQUAL -1)
                set_property(TARGET ${target} APPEND PROPERTY
                    QT_QML_MODULE_${import_set} "${import}"
                )
            else()
                string(SUBSTRING ${import} 0 ${slash_position} import_module)
                math(EXPR slash_position "${slash_position} + 1")
                string(SUBSTRING ${import} ${slash_position} -1 import_version)
                if (import_version MATCHES "^([0-9]+(\\.[0-9]+)?|auto)$")
                    set_property(TARGET ${target} APPEND PROPERTY
                        QT_QML_MODULE_${import_set} "${import_module} ${import_version}"
                    )
                else()
                    message(FATAL_ERROR
                        "Invalid module ${import} version number. "
                        "Expected 'VersionMajor', 'VersionMajor.VersionMinor' or 'auto'."
                    )
                endif()
            endif()
        endforeach()
    endforeach()

    foreach(dependency IN LISTS arg_DEPENDENCIES)
        string(FIND ${dependency} "/" slash_position REVERSE)
        if (slash_position EQUAL -1)
            set_property(TARGET ${target} APPEND PROPERTY
                QT_QML_MODULE_DEPENDENCIES "${dependency}"
            )
        else()
            string(SUBSTRING ${dependency} 0 ${slash_position} dep_module)
            math(EXPR slash_position "${slash_position} + 1")
            string(SUBSTRING ${dependency} ${slash_position} -1 dep_version)
            if (dep_version MATCHES "^([0-9]+(\\.[0-9]+)?|auto)$")
                set_property(TARGET ${target} APPEND PROPERTY
                    QT_QML_MODULE_DEPENDENCIES "${dep_module} ${dep_version}"
                )
            else()
                message(FATAL_ERROR
                    "Invalid module dependency version number. "
                    "Expected 'VersionMajor', 'VersionMajor.VersionMinor' or 'auto'."
                )
            endif()
        endif()
    endforeach()

    _qt_internal_canonicalize_resource_path("${arg_RESOURCE_PREFIX}" arg_RESOURCE_PREFIX)
    if(arg_NO_RESOURCE_TARGET_PATH)
        set(qt_qml_module_resource_prefix "${arg_RESOURCE_PREFIX}")
    else()
        if(arg_RESOURCE_PREFIX STREQUAL "/")   # Checked so we prevent double-slash
            set(qt_qml_module_resource_prefix "/${arg_TARGET_PATH}")
        else()
            set(qt_qml_module_resource_prefix "${arg_RESOURCE_PREFIX}/${arg_TARGET_PATH}")
        endif()
    endif()

    set_target_properties(${target} PROPERTIES
        QT_QML_MODULE_NO_LINT "${arg_NO_LINT}"
        QT_QML_MODULE_NO_CACHEGEN "${arg_NO_CACHEGEN}"
        QT_QML_MODULE_NO_GENERATE_QMLDIR "${arg_NO_GENERATE_QMLDIR}"
        QT_QML_MODULE_NO_PLUGIN "${arg_NO_PLUGIN}"
        QT_QML_MODULE_NO_PLUGIN_OPTIONAL "${arg_NO_PLUGIN_OPTIONAL}"
        QT_QML_MODULE_URI "${arg_URI}"
        QT_QML_MODULE_TARGET_PATH "${arg_TARGET_PATH}"
        QT_QML_MODULE_VERSION "${arg_VERSION}"
        QT_QML_MODULE_CLASS_NAME "${arg_CLASS_NAME}"

        QT_QML_MODULE_PLUGIN_TARGET "${arg_PLUGIN_TARGET}"
        QT_QML_MODULE_INSTALLED_PLUGIN_TARGET "${arg_INSTALLED_PLUGIN_TARGET}"

        # Also Save the PLUGIN_TARGET values in a separate property to circumvent
        # https://gitlab.kitware.com/cmake/cmake/-/issues/21484 when exporting the properties
        _qt_qml_module_plugin_target "${arg_PLUGIN_TARGET}"
        _qt_qml_module_installed_plugin_target "${arg_INSTALLED_PLUGIN_TARGET}"

        QT_QML_MODULE_DESIGNER_SUPPORTED "${arg_DESIGNER_SUPPORTED}"
        QT_QML_MODULE_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
        QT_QML_MODULE_RESOURCE_PREFIX "${qt_qml_module_resource_prefix}"
        QT_QML_MODULE_PAST_MAJOR_VERSIONS "${arg_PAST_MAJOR_VERSIONS}"
        QT_QML_MODULE_TYPEINFO "${arg_TYPEINFO}"

        # TODO: Check how this is used by qt6_android_generate_deployment_settings()
        QT_QML_IMPORT_PATH "${arg_IMPORT_PATH}"
    )

    # Executables don't have a plugin target, so no need to export the properties.
    if(NOT backing_target_type STREQUAL "EXECUTABLE" AND NOT is_android_executable)
        set_property(TARGET ${target} APPEND PROPERTY
            EXPORT_PROPERTIES _qt_qml_module_plugin_target _qt_qml_module_installed_plugin_target
        )
    endif()

    set(ensure_set_properties
        QT_QML_MODULE_PLUGIN_TYPES_FILE
        QT_QML_MODULE_RESOURCE_PATHS
        QT_QMLCACHEGEN_DIRECT_CALLS
        QT_QMLCACHEGEN_BINARY
        QT_QMLCACHEGEN_ARGUMENTS
    )
    foreach(prop IN LISTS ensure_set_properties)
        get_target_property(val ${target} ${prop})
        if("${val}" MATCHES "-NOTFOUND$")
            set_target_properties(${target} PROPERTIES ${prop} "")
        endif()
    endforeach()

    if(NOT arg_NO_GENERATE_QMLTYPES)
        set(type_registration_extra_args "")
        if(arg___QT_INTERNAL_INSTALL_METATYPES_JSON)
            list(APPEND type_registration_extra_args __QT_INTERNAL_INSTALL_METATYPES_JSON)
        endif()
        _qt_internal_qml_type_registration(${target} ${type_registration_extra_args})
    endif()

    set(output_targets)

    if(NOT arg_NO_GENERATE_QMLDIR)
        _qt_internal_target_generate_qmldir(${target})

        # Embed qmldir in qrc. The following comments relate mostly to Qt5->6 transition.
        # The requirement to keep the same resource name might no longer apply, but it doesn't
        # currently appear to cause any hinderance to keep it.
        # The qmldir resource name needs to match the one generated by qmake's qml_module.prf, to
        # ensure that all Q_INIT_RESOURCE(resource_name) calls in Qt code don't lead to undefined
        # symbol errors when linking an application project.
        # The Q_INIT_RESOURCE() calls are not strictly necessary anymore because the CMake Qt
        # build passes around the compiled resources as object files.
        # These object files have global initiliazers that don't get discared when linked into
        # an application (as opposed to when the resource libraries were embedded into the static
        # libraries when Qt was built with qmake).
        # The reason to match the naming is to ensure that applications link successfully regardless
        # if Qt was built with CMake or qmake, while the build system transition phase is still
        # happening.
        string(REPLACE "/" "_" qmldir_resource_name "qmake_${arg_TARGET_PATH}")

        # The qmldir file ALWAYS has to be under the target path, even in the
        # resources. If it isn't, an explicit import can't find it. We need a
        # second copy NOT under the target path if NO_RESOURCE_TARGET_PATH is
        # given so that the implicit import will work.
        set(prefixes "${qt_qml_module_resource_prefix}")
        if(arg_NO_RESOURCE_TARGET_PATH)
            # The above prefixes item won't include the target path, so add a
            # second one that does.
            if(qt_qml_module_resource_prefix STREQUAL "/")
                list(APPEND prefixes "/${arg_TARGET_PATH}")
            else()
                list(APPEND prefixes "${qt_qml_module_resource_prefix}/${arg_TARGET_PATH}")
            endif()
        endif()
        set_source_files_properties(${arg_OUTPUT_DIRECTORY}/qmldir
            PROPERTIES QT_RESOURCE_ALIAS "qmldir"
        )

        foreach(prefix IN LISTS prefixes)
            set(resource_targets)
            qt6_add_resources(${target} ${qmldir_resource_name}
                FILES ${arg_OUTPUT_DIRECTORY}/qmldir
                PREFIX "${prefix}"
                OUTPUT_TARGETS resource_targets
            )
            list(APPEND output_targets ${resource_targets})
            # If we are adding the same file twice, we need a different resource
            # name for the second one. It has the same QT_RESOURCE_ALIAS but a
            # different prefix, so we can't put it in the same resource.
            string(APPEND qmldir_resource_name "_copy")
        endforeach()
    endif()

    if(NOT arg_NO_PLUGIN AND NOT arg_NO_CREATE_PLUGIN_TARGET)
        # This also handles the case where ${arg_PLUGIN_TARGET} already exists,
        # including where it is the same as ${target}. If ${arg_PLUGIN_TARGET}
        # already exists, it will update the necessary things that are specific
        # to qml plugins.
        if(TARGET ${arg_PLUGIN_TARGET})
            set(plugin_lib_type "")
        else()
            set(plugin_lib_type ${lib_type})
        endif()
        qt6_add_qml_plugin(${arg_PLUGIN_TARGET}
            ${plugin_lib_type}
            ${no_gen_source}
            OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
            BACKING_TARGET ${target}
            CLASS_NAME ${arg_CLASS_NAME}
        )
    endif()

    if(TARGET "${arg_PLUGIN_TARGET}" AND NOT arg_PLUGIN_TARGET STREQUAL target)
        target_link_libraries(${arg_PLUGIN_TARGET} PRIVATE ${target})
    endif()

    target_sources(${target} PRIVATE ${arg_SOURCES})

    set(cache_target)
    qt6_target_qml_sources(${target}
        __QT_INTERNAL_FORCE_DEFER_QMLDIR
        QML_FILES ${arg_QML_FILES}
        RESOURCES ${arg_RESOURCES}
        OUTPUT_TARGETS cache_target
        PREFIX "${qt_qml_module_resource_prefix}"
    )
    list(APPEND output_targets ${cache_target})

    # Build an init object library for static plugins and propagate it along with the plugin
    # target.
    # TODO: Figure out if we can move this code block into qt_add_qml_plugin. Need to consider
    #       various corner cases.
    #       QTBUG-96937
    if(TARGET "${arg_PLUGIN_TARGET}")
        get_target_property(plugin_lib_type ${arg_PLUGIN_TARGET} TYPE)
        if(plugin_lib_type STREQUAL "STATIC_LIBRARY")
            __qt_internal_add_static_plugin_init_object_library(
                "${arg_PLUGIN_TARGET}" plugin_init_target)
            list(APPEND output_targets ${plugin_init_target})

            __qt_internal_propagate_object_library("${arg_PLUGIN_TARGET}" "${plugin_init_target}")
        endif()
    endif()

    if(NOT arg_NO_GENERATE_QMLDIR)
        if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.19.0")
            # Defer the write to allow more qml files to be added later by calls to
            # qt6_target_qml_sources(). We wrap the deferred call with EVAL CODE
            # so that ${target} is evaluated now rather than the end of the scope.
            # We also delay target finalization until after our deferred write
            # because the qmldir file must be written before any finalizer
            # might call qt_import_qml_plugins().
            cmake_language(EVAL CODE
                "cmake_language(DEFER ID_VAR write_id CALL _qt_internal_write_deferred_qmldir_file ${target})"
            )
            _qt_internal_delay_finalization_until_after(${write_id})
        else()
            # Can't defer the write, have to do it now
            _qt_internal_write_deferred_qmldir_file(${target})
        endif()
    endif()

    if(arg_OUTPUT_TARGETS)
        set(${arg_OUTPUT_TARGETS} ${output_targets} PARENT_SCOPE)
    endif()

endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_qml_module)
        qt6_add_qml_module(${ARGV})
        cmake_parse_arguments(PARSE_ARGV 1 arg "" "OUTPUT_TARGETS" "")
        if(arg_OUTPUT_TARGETS)
            set(${arg_OUTPUT_TARGETS} ${${arg_OUTPUT_TARGETS}} PARENT_SCOPE)
        endif()
    endfunction()
endif()

# Make the prefix conform to the following:
#   - Starts with a "/"
#   - Does not end with a "/" unless the prefix is exactly "/"
function(_qt_internal_canonicalize_resource_path path out_var)
    if(NOT path)
        set(path "/")
    endif()
    if(NOT path MATCHES "^/")
        string(PREPEND path "/")
    endif()
    if(path MATCHES [[(.+)/$]])
        set(path "${CMAKE_MATCH_1}")
    endif()
    set(${out_var} "${path}" PARENT_SCOPE)
endfunction()

function(_qt_internal_get_escaped_uri uri out_var)
    string(REGEX REPLACE "[^A-Za-z0-9]" "_" escaped_uri "${uri}")
    set(${out_var} "${escaped_uri}" PARENT_SCOPE)
endfunction()

function(_qt_internal_compute_qml_plugin_class_name_from_uri uri out_var)
    _qt_internal_get_escaped_uri("${uri}" escaped_uri)
    set(${out_var} "${escaped_uri}Plugin" PARENT_SCOPE)
endfunction()

macro(_qt_internal_genex_getproperty var target property)
    set(${var} "$<TARGET_PROPERTY:${target},${property}>")
    set(have_${var} "$<BOOL:${${var}}>")
endmacro()

macro(_qt_internal_genex_getjoinedproperty var target property item_prefix glue)
    _qt_internal_genex_getproperty(${var} ${target} ${property})
    set(${var} "$<${have_${var}}:${item_prefix}$<JOIN:${${var}},${glue}${item_prefix}>>")
endmacro()

macro(_qt_internal_genex_getoption var target property)
    set(${var} "$<BOOL:$<TARGET_PROPERTY:${target},${property}>>")
endmacro()

function(_qt_internal_target_enable_qmllint target)
    set(lint_target ${target}_qmllint)
    if(TARGET ${lint_target})
        return()
    endif()

    _qt_internal_genex_getproperty(qmllint_files ${target} QT_QML_LINT_FILES)
    _qt_internal_genex_getjoinedproperty(import_args ${target}
        QT_QML_IMPORT_PATH "-I$<SEMICOLON>" "$<SEMICOLON>"
    )
    _qt_internal_genex_getjoinedproperty(qrc_args ${target}
        _qt_generated_qrc_files "--resource$<SEMICOLON>" "$<SEMICOLON>"
    )

    # Facilitate self-import so it can find the qmldir file. We also try to walk
    # back up the directory structure to find a base path under which this QML
    # module is located. Such a base path is likely to be used for other QML
    # modules that we might need to find, so add it to the import path if we
    # find a compatible directory structure. It doesn't make sense to do this
    # for an executable though, since it can never be found as a QML module for
    # a different QML module/target.
    get_target_property(target_type ${target} TYPE)
    get_target_property(is_android_executable ${target} _qt_is_android_executable)
    if(target_type STREQUAL "EXECUTABLE" OR is_android_executable)
        # The executable's own QML module's qmldir file will usually be under a
        # subdirectory (matching the module's target path) below the target's
        # build directory.
        list(APPEND import_args -I "$<TARGET_PROPERTY:${target},BINARY_DIR>")
    elseif(target_type MATCHES "LIBRARY")
        get_target_property(output_dir  ${target} QT_QML_MODULE_OUTPUT_DIRECTORY)
        get_target_property(target_path ${target} QT_QML_MODULE_TARGET_PATH)
        if(output_dir MATCHES "${target_path}$")
            string(REGEX REPLACE "(.*)/${target_path}" "\\1" base_dir "${output_dir}")
            list(APPEND import_args -I "${base_dir}")
        else()
            message(WARNING
                "The ${target} target is a QML module with target path ${target_path}. "
                "It uses an OUTPUT_DIRECTORY of ${output_dir}, which should end in the "
                "same target path, but doesn't. Tooling such as qmllint may not work "
                "correctly."
            )
        endif()
    endif()

    if(NOT "${QT_QML_OUTPUT_DIRECTORY}" STREQUAL "")
        list(APPEND import_args -I "${QT_QML_OUTPUT_DIRECTORY}")
    endif()

    set(cmd
        ${QT_TOOL_COMMAND_WRAPPER_PATH}
        ${QT_CMAKE_EXPORT_NAMESPACE}::qmllint
        ${import_args}
        ${qrc_args}
        ${qmllint_files}
    )

    # We need this target to depend on all qml type registrations. This is the
    # only way we can be sure that all *.qmltypes files for any QML modules we
    # depend on will have been generated.
    add_custom_target(${lint_target}
        COMMAND "$<${have_qmllint_files}:${cmd}>"
        COMMAND_EXPAND_LISTS
        DEPENDS
            ${QT_CMAKE_EXPORT_NAMESPACE}::qmllint
            ${qmllint_files}
            $<TARGET_NAME_IF_EXISTS:all_qmltyperegistrations>
        WORKING_DIRECTORY "$<TARGET_PROPERTY:${target},SOURCE_DIR>"
    )

    # Make the global linting target depend on the one we add here.
    # Note that the caller is free to change the value of QT_QMLLINT_ALL_TARGET
    # for different QML modules if they wish, which means they can implement
    # their own grouping of the ${target}_qmllint targets.
    if("${QT_QMLLINT_ALL_TARGET}" STREQUAL "")
        set(QT_QMLLINT_ALL_TARGET all_qmllint)
    endif()
    if(NOT TARGET ${QT_QMLLINT_ALL_TARGET})
        add_custom_target(${QT_QMLLINT_ALL_TARGET})
    endif()
    add_dependencies(${QT_QMLLINT_ALL_TARGET} ${lint_target})

endfunction()

# This is a  modified version of __qt_propagate_generated_resource from qtbase.
#
# It uses a common __qt_internal_propagate_object_library function to link and propagate the object
# library to the end-point executable.
#
# The reason for propagating the qmlcache target as a 'fake resource' from the build system
# perspective is to ensure proper handling of the object files in generated qmake .prl files.
function(_qt_internal_propagate_qmlcache_object_lib
         target
         generated_source_code
         link_condition
         output_generated_target)
    set(resource_target "${target}_qmlcache")
    qt6_add_library("${resource_target}" OBJECT "${generated_source_code}")

    # Needed to trigger the handling of the object library for .prl generation.
    set_property(TARGET ${resource_target} APPEND PROPERTY _qt_resource_name ${resource_target})

    # Export info that this is a qmlcache target, in case if we ever need to detect such targets,
    # similar how we need it for plugin initializers.
    set_property(TARGET ${resource_target} PROPERTY _is_qt_qmlcache_target TRUE)
    set_property(TARGET ${resource_target} APPEND PROPERTY
        EXPORT_PROPERTIES _is_qt_qmlcache_target
    )

    # Save the path to the generated source file, relative to the the current build dir.
    # The path will be used in static library prl file generation to ensure qmake links
    # against the installed resource object files.
    # Example saved path:
    #    .rcc/qrc_qprintdialog.cpp
    file(RELATIVE_PATH generated_cpp_file_relative_path
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${generated_source_code}")
    set_property(TARGET ${resource_target} APPEND PROPERTY
        _qt_resource_generated_cpp_relative_path "${generated_cpp_file_relative_path}")

    # Qml specific additions.
    target_link_libraries(${resource_target} PRIVATE
        ${QT_CMAKE_EXPORT_NAMESPACE}::QmlPrivate
        ${QT_CMAKE_EXPORT_NAMESPACE}::Core
    )

    __qt_internal_propagate_object_library(${target} ${resource_target}
        EXTRA_CONDITIONS "${link_condition}"
    )

    set(${output_generated_target} "${resource_target}" PARENT_SCOPE)
endfunction()

function(_qt_internal_target_enable_qmlcachegen target output_targets_var qmlcachegen)

    set(output_targets)
    set_target_properties(${target} PROPERTIES _qt_cachegen_set_up TRUE)

    get_target_property(target_binary_dir ${target} BINARY_DIR)
    set(qmlcache_dir ${target_binary_dir}/.rcc/qmlcache)
    set(qmlcache_resource_name qmlcache_${target})

    # INTEGRITY_SYMBOL_UNIQUENESS
    # The cache loader file name has to be unique, because the Integrity compiler uses the file name
    # for the generation of the translation unit static constructor symbol name.
    #    e.g. __sti___19_qmlcache_loader_cpp_11acedbd
    # For some reason the symbol is created with global visibility.
    #
    # When an application links against the Basic and Fusion static qml plugins, the linker
    # fails with duplicate symbol errors because both of those plugins will contain the same symbol.
    #
    # With gcc on regular Linux, the symbol names are also the same, but it's not a problem because
    # they have local (hidden) visbility.
    #
    # Make the file name unique by prepending the target name.
    set(qmlcache_loader_cpp ${qmlcache_dir}/${target}_qmlcache_loader.cpp)

    set(qmlcache_loader_list ${qmlcache_dir}/${target}_qml_loader_file_list.rsp)
    set(qmlcache_resource_paths "$<TARGET_PROPERTY:${target},QT_QML_MODULE_RESOURCE_PATHS>")

    _qt_internal_genex_getjoinedproperty(qrc_resource_args ${target}
        _qt_generated_qrc_files "--resource$<SEMICOLON>" "$<SEMICOLON>"
    )

    if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config" AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.20")
        set(qmlcachegen "$<COMMAND_CONFIG:${qmlcachegen}>")
    endif()
    set(cmd
        ${QT_TOOL_COMMAND_WRAPPER_PATH}
        ${qmlcachegen}
        --resource-name "${qmlcache_resource_name}"
        ${qrc_resource_args}
        -o "${qmlcache_loader_cpp}"
        "@${qmlcache_loader_list}"
    )

    file(GENERATE
        OUTPUT ${qmlcache_loader_list}
        CONTENT "$<JOIN:${qmlcache_resource_paths},\n>\n"
    )

    add_custom_command(
        OUTPUT ${qmlcache_loader_cpp}
        COMMAND "${cmd}"
        COMMAND_EXPAND_LISTS
        DEPENDS
            ${qmlcachegen}
            ${qmlcache_loader_list}
            $<TARGET_PROPERTY:${target},_qt_generated_qrc_files>
    )

    # The current scope sees the file as generated automatically, but the
    # target scope may not if it is different. Force it where we can.
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
        set_source_files_properties(
            ${qmlcache_loader_cpp}
            TARGET_DIRECTORY ${target}
            PROPERTIES GENERATED TRUE
        )
    endif()
    get_target_property(target_source_dir ${target} SOURCE_DIR)
    if(NOT target_source_dir STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        add_custom_target(${target}_qmlcachegen DEPENDS ${qmlcache_loader_cpp})
        add_dependencies(${target} ${target}_qmlcachegen)
    endif()

    # TODO: Probably need to reject ${target} being an object library as unsupported
    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL "STATIC_LIBRARY")
        set(extra_conditions "")
        _qt_internal_propagate_qmlcache_object_lib(
            ${target}
            "${qmlcache_loader_cpp}"
            "${extra_conditions}"
            output_target)

        list(APPEND output_targets ${output_target})
    else()
        target_sources(${target} PRIVATE "${qmlcache_loader_cpp}")
        target_link_libraries(${target} PRIVATE
            ${QT_CMAKE_EXPORT_NAMESPACE}::QmlPrivate
            ${QT_CMAKE_EXPORT_NAMESPACE}::Core
        )
    endif()

    set(${output_targets_var} ${output_targets} PARENT_SCOPE)
endfunction()

# We cannot defer writing out the qmldir file to generation time because the
# qmlimportscanner runs at configure time as part of target finalizers.
# Therefore, the best we can do is defer writing the qmldir file if we are
# using a recent enough CMake version, otherwise we write it out progressively
# on each call that adds qml sources. The immediate progressive writes will
# trigger some unnecessary rebuilds after reconfiguring due to the qmldir
# file's timestamp being updated even though its contents might not change,
# but that's the cost of not having deferred write capability.
function(_qt_internal_target_generate_qmldir target)

    macro(_qt_internal_qmldir_item prefix property)
        get_target_property(_value ${target} ${property})
        if(_value)
            string(APPEND content "${prefix} ${_value}\n")
        endif()
    endmacro()

    macro(_qt_internal_qmldir_item_list prefix property)
        get_target_property(_values ${target} ${property})
        if(_values)
            foreach(_value IN LISTS _values)
                string(APPEND content "${prefix} ${_value}\n")
            endforeach()
        endif()
    endmacro()

    get_target_property(uri ${target} QT_QML_MODULE_URI)
    if(NOT uri)
        message(FATAL_ERROR "Target ${target} has no URI set, cannot create qmldir")
    endif()
    set(content "module ${uri}\n")

    _qt_internal_qmldir_item(linktarget QT_QML_MODULE_INSTALLED_PLUGIN_TARGET)

    get_target_property(plugin_target ${target} QT_QML_MODULE_PLUGIN_TARGET)
    if(plugin_target)
        get_target_property(no_plugin_optional ${target} QT_QML_MODULE_NO_PLUGIN_OPTIONAL)
        if(NOT no_plugin_optional MATCHES "NOTFOUND" AND NOT no_plugin_optional)
            string(APPEND content "optional ")
        endif()

        get_target_property(target_path ${target} QT_QML_MODULE_TARGET_PATH)
        _qt_internal_get_qml_plugin_output_name(plugin_output_name ${plugin_target}
            TARGET_PATH "${target_path}"
            URI "${uri}"
        )
        string(APPEND content "plugin ${plugin_output_name}\n")

        _qt_internal_qmldir_item(classname QT_QML_MODULE_CLASS_NAME)
    endif()

    get_target_property(designer_supported ${target} QT_QML_MODULE_DESIGNER_SUPPORTED)
    if(designer_supported)
        string(APPEND content "designersupported\n")
    endif()

    _qt_internal_qmldir_item(typeinfo QT_QML_MODULE_TYPEINFO)

    _qt_internal_qmldir_item_list(import QT_QML_MODULE_IMPORTS)
    _qt_internal_qmldir_item_list("optional import" QT_QML_MODULE_OPTIONAL_IMPORTS)
    _qt_internal_qmldir_item_list(depends QT_QML_MODULE_DEPENDENCIES)

    get_target_property(prefix ${target} QT_QML_MODULE_RESOURCE_PREFIX)
    if(prefix)
        # Ensure we use a path that ends with a "/", but handle the special case
        # of "/" without anything after it
        if(NOT prefix STREQUAL "/" AND NOT prefix MATCHES "/$")
            string(APPEND prefix "/")
        endif()
        string(APPEND content "prefer :${prefix}\n")
    endif()

    # TODO: What about multi-config generators? Would we need per-config qmldir
    #       files (because we will have per-config plugin targets)?

    # Record the contents but defer the actual write. We will write the file
    # later, either at the end of qt6_add_qml_module() or the end of the
    # directory scope (depending on the CMake version being used).
    set_property(TARGET ${target} PROPERTY _qt_internal_qmldir_content "${content}")

    # NOTE: qt6_target_qml_sources() may append further content later.
endfunction()

function(_qt_internal_write_deferred_qmldir_file target)
    get_target_property(__qt_qmldir_content ${target} _qt_internal_qmldir_content)
    get_target_property(out_dir ${target} QT_QML_MODULE_OUTPUT_DIRECTORY)
    set(qmldir_file "${out_dir}/qmldir")
    configure_file(${__qt_qml_macros_module_base_dir}/Qt6qmldirTemplate.cmake.in ${qmldir_file} @ONLY)
endfunction()


function(qt6_add_qml_plugin target)
    set(args_option
        STATIC
        SHARED
        NO_GENERATE_PLUGIN_SOURCE
    )

    set(args_single
        OUTPUT_DIRECTORY
        URI
        BACKING_TARGET
        CLASS_NAME
        # The following is only needed on Android, and even then, only if the
        # default conversion from the URI is not applicable. It is an internal
        # option, it may be removed.
        TARGET_PATH
    )

    set(args_multi "")

    cmake_parse_arguments(PARSE_ARGV 1 arg
       "${args_option}"
       "${args_single}"
       "${args_multi}"
    )

    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT arg_URI)
        if(NOT arg_BACKING_TARGET)
            message(FATAL_ERROR "No URI or BACKING_TARGET provided")
        endif()
        if(NOT TARGET ${arg_BACKING_TARGET})
            if(arg_BACKING_TARGET STREQUAL target)
                message(FATAL_ERROR
                    "Plugin ${target} is its own backing target, URI must be provided"
                )
            else()
                message(FATAL_ERROR
                    "No URI provided and unable to obtain it from the BACKING_TARGET "
                    "(${arg_BACKING_TARGET}) because no such target exists"
                )
            endif()
        endif()
        get_target_property(arg_URI ${arg_BACKING_TARGET} QT_QML_MODULE_URI)
        if(NOT arg_URI)
            message(FATAL_ERROR
                "No URI provided and the BACKING_TARGET (${arg_BACKING_TARGET}) "
                "does not have one set either"
            )
        endif()
    endif()

    _qt_internal_get_escaped_uri("${arg_URI}" escaped_uri)

    if(NOT arg_CLASS_NAME)
        if(NOT "${arg_BACKING_TARGET}" STREQUAL "")
            get_target_property(arg_CLASS_NAME ${target} QT_QML_MODULE_CLASS_NAME)
        endif()
        if(NOT arg_CLASS_NAME)
            _qt_internal_compute_qml_plugin_class_name_from_uri("${arg_URI}" arg_CLASS_NAME)
        endif()
    endif()

    if(TARGET ${target})
        # Plugin target already exists. Perform a few sanity checks, but we
        # otherwise trust that the target is appropriate for use as a plugin.
        get_target_property(target_type ${target} TYPE)
        if(target_type STREQUAL "EXECUTABLE")
            message(FATAL_ERROR "Plugins cannot be executables (target: ${target})")
        endif()
        foreach(arg IN ITEMS STATIC SHARED)
            if(arg_${arg})
                message(FATAL_ERROR
                    "Cannot specify ${arg} keyword, target ${target} already exists"
                )
            endif()
        endforeach()

        get_target_property(existing_class_name ${target} QT_PLUGIN_CLASS_NAME)
        if(existing_class_name)
            if(NOT existing_class_name STREQUAL arg_CLASS_NAME)
                message(FATAL_ERROR
                    "An existing plugin target was given, but it has a different class name "
                    "(${existing_class_name}) to that being used here (${arg_CLASS_NAME})"
                )
            endif()
        elseif(arg_CLASS_NAME)
            set_property(TARGET ${target} PROPERTY QT_PLUGIN_CLASS_NAME "${arg_CLASS_NAME}")
        else()
            message(FATAL_ERROR
                "An existing '${target}' plugin target was given, but it has no class name set "
                "and no new class name was provided."
            )
        endif()
    else()
        if(arg_STATIC AND arg_SHARED)
            message(FATAL_ERROR
                "Cannot specify both STATIC and SHARED for target ${target}"
            )
        endif()
        set(lib_type "")
        if(arg_STATIC)
            set(lib_type STATIC)
        elseif(arg_SHARED)
            set(lib_type SHARED)
        endif()

        if(TARGET "${arg_BACKING_TARGET}")
            # Ensure that the plugin type we create will be compatible with the
            # type of backing target we were given
            get_target_property(backing_type ${arg_BACKING_TARGET} TYPE)
            if(backing_type STREQUAL "STATIC_LIBRARY")
                if(lib_type STREQUAL "")
                    set(lib_type STATIC)
                elseif(lib_type STREQUAL "SHARED")
                    message(FATAL_ERROR
                        "Mixing a static backing library with a non-static plugin "
                        "is not supported"
                    )
                endif()
            elseif(backing_type STREQUAL "SHARED_LIBRARY")
                if(lib_type STREQUAL "")
                    set(lib_type SHARED)
                elseif(lib_type STREQUAL "STATIC")
                    message(FATAL_ERROR
                        "Mixing a non-static backing library with a static plugin "
                        "is not supported"
                    )
                endif()
            elseif(backing_type STREQUAL "EXECUTABLE")
                message(FATAL_ERROR
                    "A separate plugin should not be needed when the backing target "
                    "is an executable. Pre-create the plugin target before calling "
                    "this command if you really must have a separate plugin."
                )
            else()
                # Object libraries, utility/custom targets
                message(FATAL_ERROR "Unsupported backing target type: ${backing_type}")
            endif()
        endif()

        qt6_add_plugin(${target} ${lib_type}
            PLUGIN_TYPE qml_plugin
            CLASS_NAME ${arg_CLASS_NAME}
        )
    endif()

    get_target_property(moc_opts ${target} AUTOMOC_MOC_OPTIONS)
    set(already_set FALSE)
    if(moc_opts)
        foreach(opt IN LISTS moc_opts)
            if("${opt}" MATCHES "^-Muri=")
                set(already_set TRUE)
                break()
            endif()
        endforeach()
    endif()
    if(NOT already_set)
        # Insert the plugin's URI into its meta data to enable usage
        # of static plugins in QtDeclarative (like in mkspecs/features/qml_plugin.prf).
        set_property(TARGET ${target} APPEND PROPERTY
            AUTOMOC_MOC_OPTIONS "-Muri=${arg_URI}"
        )
    endif()

    if(ANDROID)
        _qt_internal_get_qml_plugin_output_name(plugin_output_name ${target}
            BACKING_TARGET "${arg_BACKING_TARGET}"
            TARGET_PATH "${arg_TARGET_PATH}"
            URI "${arg_URI}"
        )
        set_target_properties(${target}
            PROPERTIES
            LIBRARY_OUTPUT_NAME "${plugin_output_name}"
        )
        qt6_android_apply_arch_suffix(${target})
    endif()

    if(NOT arg_OUTPUT_DIRECTORY AND arg_BACKING_TARGET AND TARGET ${arg_BACKING_TARGET})
        get_target_property(arg_OUTPUT_DIRECTORY ${arg_BACKING_TARGET} QT_QML_MODULE_OUTPUT_DIRECTORY)
    endif()
    if(arg_OUTPUT_DIRECTORY)
        # Plugin target must be in the output directory. The backing target,
        # if it is different to the plugin target, can be anywhere.
        set_target_properties(${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
            LIBRARY_OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
            ARCHIVE_OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
        )
    endif()

    if(NOT arg_NO_GENERATE_PLUGIN_SOURCE)
        set(generated_cpp_file_name_base "${target}_${arg_CLASS_NAME}")
        set(register_types_function_name "qml_register_types_${escaped_uri}")

        # These are all substituted in the template file used further below
        set(qt_qml_plugin_class_name "${arg_CLASS_NAME}")
        set(qt_qml_plugin_moc_include_name "${generated_cpp_file_name_base}.moc")
        set(qt_qml_plugin_intro "extern void ${register_types_function_name}();\nQ_GHS_KEEP_REFERENCE(${register_types_function_name});")
        set(qt_qml_plugin_outro "")
        if(QT_BUILDING_QT)
            string(APPEND qt_qml_plugin_intro "\n\nQT_BEGIN_NAMESPACE")
            string(APPEND qt_qml_plugin_outro "QT_END_NAMESPACE")
        endif()

        # Indenting here is deliberately different so as to make the generated
        # file have sensible indenting
        set(qt_qml_plugin_constructor_content
        "volatile auto registration = &${register_types_function_name};
        Q_UNUSED(registration);"
        )

        set(generated_cpp_file
            "${CMAKE_CURRENT_BINARY_DIR}/${generated_cpp_file_name_base}.cpp"
        )
        configure_file(
            "${__qt_qml_macros_module_base_dir}/Qt6QmlPluginTemplate.cpp.in"
            "${generated_cpp_file}"
            @ONLY
        )
        target_sources(${target} PRIVATE "${generated_cpp_file}")

        # The generated cpp file expects to include its moc-ed output file.
        set_target_properties(${target} PROPERTIES AUTOMOC TRUE)
    endif()

    target_link_libraries(${target} PRIVATE ${QT_CMAKE_EXPORT_NAMESPACE}::Qml)

    # Link plugin against its backing lib if it has one.
    if(NOT arg_BACKING_TARGET STREQUAL "" AND NOT arg_BACKING_TARGET STREQUAL target)
        target_link_libraries(${target} PRIVATE ${arg_BACKING_TARGET})
    endif()

    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.19.0")
        # Defer the collection of plugin dependencies until after any extra target_link_libraries
        # calls that a user project might do.
        # We wrap the deferred call with EVAL CODE
        # so that ${target} is evaluated now rather than the end of the scope.
        cmake_language(EVAL CODE
            "cmake_language(DEFER CALL _qt_internal_add_static_qml_plugin_dependencies \"${target}\" \"${arg_BACKING_TARGET}\")"
        )
    else()
        # Can't defer, have to do it now.
        _qt_internal_add_static_qml_plugin_dependencies("${target}" "${arg_BACKING_TARGET}")
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_qml_plugin)
        qt6_add_qml_plugin(${ARGV})
    endfunction()
endif()


function(qt6_target_qml_sources target)

    get_target_property(uri        ${target} QT_QML_MODULE_URI)
    get_target_property(output_dir ${target} QT_QML_MODULE_OUTPUT_DIRECTORY)
    if(NOT uri OR NOT output_dir)
        message(FATAL_ERROR "Target ${target} is not a QML module")
    endif()

    set(args_option
        NO_LINT
        NO_CACHEGEN
        NO_QMLDIR_TYPES
        __QT_INTERNAL_FORCE_DEFER_QMLDIR  # Used only by qt6_add_qml_module()
    )

    set(args_single
        PREFIX
        OUTPUT_TARGETS
    )

    set(args_multi
        QML_FILES
        RESOURCES
    )

    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${args_option}" "${args_single}" "${args_multi}"
    )
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown/unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    if (NOT arg_QML_FILES AND NOT arg_RESOURCES)
        if(arg_OUTPUT_TARGETS)
            set(${arg_OUTPUT_TARGETS} "" PARENT_SCOPE)
        endif()
        return()
    endif()

    if(NOT arg___QT_INTERNAL_FORCE_DEFER_QMLDIR AND ${CMAKE_VERSION} VERSION_LESS "3.19.0")
        message(FATAL_ERROR
            "You are using CMake ${CMAKE_VERSION}, but CMake 3.19 or later "
            "is required to add qml files with this function. Either pass "
            "the qml files to qt6_add_qml_module() instead or update to "
            "CMake 3.19 or later."
        )
    endif()

    get_target_property(no_lint                ${target} QT_QML_MODULE_NO_LINT)
    get_target_property(no_cachegen            ${target} QT_QML_MODULE_NO_CACHEGEN)
    get_target_property(no_qmldir              ${target} QT_QML_MODULE_NO_GENERATE_QMLDIR)
    get_target_property(resource_prefix        ${target} QT_QML_MODULE_RESOURCE_PREFIX)
    get_target_property(qml_module_version     ${target} QT_QML_MODULE_VERSION)
    get_target_property(past_major_versions    ${target} QT_QML_MODULE_PAST_MAJOR_VERSIONS)

    if(NOT output_dir)
        # Probably not a qml module. We still want to support tooling for this
        # scenario, it's just that we will be relying solely on the implicit
        # imports to find things.
        set(output_dir ${CMAKE_CURRENT_BINARY_DIR})
        set(no_qmldir TRUE)
    endif()

    if(NOT arg_PREFIX)
        if(resource_prefix)
            set(arg_PREFIX ${resource_prefix})
        else()
            message(FATAL_ERROR
                "PREFIX option not given and target ${target} does not have a "
                "QT_QML_MODULE_RESOURCE_PREFIX property set."
            )
        endif()
    endif()
    _qt_internal_canonicalize_resource_path("${arg_PREFIX}" arg_PREFIX)
    if(NOT arg_PREFIX STREQUAL "/")
        string(APPEND arg_PREFIX "/")
    endif()

    if (qml_module_version MATCHES "^([0-9]+)\\.")
        set(qml_module_files_versions "${CMAKE_MATCH_1}.0")
    else()
        message(FATAL_ERROR
            "No major version found in '${qml_module_version}'."
        )
    endif()
    if (past_major_versions OR past_major_versions STREQUAL "0")
        foreach (past_major_version ${past_major_versions})
            list(APPEND qml_module_files_versions "${past_major_version}.0")
        endforeach()
    endif()

    # Linting and cachegen can still occur for a target that isn't a qml module,
    # but for such targets, there is no qmldir file to update.
    if(arg_NO_LINT)
        set(no_lint TRUE)
    endif()
    if(arg_NO_CACHEGEN)
        set(no_cachegen TRUE)
    endif()
    if(no_qmldir MATCHES "NOTFOUND" OR arg_NO_QMLDIR_TYPES)
        set(no_qmldir TRUE)
    endif()

    if(NOT no_cachegen AND arg_QML_FILES)
        _qt_internal_genex_getproperty(types_file    ${target} QT_QML_MODULE_PLUGIN_TYPES_FILE)
        _qt_internal_genex_getproperty(qmlcachegen   ${target} QT_QMLCACHEGEN_BINARY)
        _qt_internal_genex_getproperty(direct_calls  ${target} QT_QMLCACHEGEN_DIRECT_CALLS)
        _qt_internal_genex_getjoinedproperty(arguments ${target}
            QT_QMLCACHEGEN_ARGUMENTS "$<SEMICOLON>" "$<SEMICOLON>"
        )
        _qt_internal_genex_getjoinedproperty(import_paths ${target}
            QT_QML_IMPORT_PATH "-I$<SEMICOLON>" "$<SEMICOLON>"
        )
        _qt_internal_genex_getjoinedproperty(qrc_resource_args ${target}
            _qt_generated_qrc_files "--resource$<SEMICOLON>" "$<SEMICOLON>"
        )
        get_target_property(target_type ${target} TYPE)
        get_target_property(is_android_executable ${target} _qt_is_android_executable)
        if(target_type STREQUAL "EXECUTABLE" OR is_android_executable)
            # The application binary directory is part of the default import path.
            list(APPEND import_paths -I "$<TARGET_PROPERTY:${target},BINARY_DIR>")
        endif()
        set(cachegen_args
            ${import_paths}
            "$<${have_types_file}:-i$<SEMICOLON>${types_file}>"
            "$<${have_direct_calls}:--direct-calls>"
            "$<${have_arguments}:${arguments}>"
            ${qrc_resource_args}
        )

        # For direct evaluation in if() below
        get_target_property(cachegen_prop ${target} QT_QMLCACHEGEN_BINARY)
        if(cachegen_prop)
            if(cachegen_prop STREQUAL "qmlcachegen" OR cachegen_prop STREQUAL "qmlsc")
                # If it's qmlcachegen or qmlsc, don't go looking for other programs of that name
                set(qmlcachegen "$<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::${cachegen_prop}>")
            else()
                find_program(${target}_QMLCACHEGEN ${cachegen_prop})
                if(${target}_QMLCACHEGEN)
                    set(qmlcachegen "${${target}_QMLCACHEGEN}")
                else()
                    message(FATAL_ERROR "Invalid qmlcachegen binary ${cachegen_prop} for ${target}")
                endif()
            endif()
        else()
            set(have_qmlsc "$<TARGET_EXISTS:${QT_CMAKE_EXPORT_NAMESPACE}::qmlsc>")
            set(cachegen_name "$<IF:${have_qmlsc},qmlsc,qmlcachegen>")
            set(qmlcachegen "$<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::${cachegen_name}>")
        endif()
    endif()

    set(non_qml_files)
    set(output_targets)
    set(copied_files)

    foreach(file_src IN LISTS arg_QML_FILES arg_RESOURCES)
        # We need to copy the file to the build directory now so that when
        # qmlimportscanner is run in qt6_import_qml_plugins() as part of
        # target finalizers, the files will be there. We need to do this
        # in a way that CMake doesn't create a dependency on the source or it
        # will re-run CMake every time the file is modified. We also don't
        # want to update the file's timestamp if its contents won't change.
        # We still enforce the dependency on the source file by adding a
        # build-time rule. This avoids having to re-run CMake just to re-copy
        # the file.
        get_filename_component(file_absolute ${file_src} ABSOLUTE)
        __qt_get_relative_resource_path_for_file(file_resource_path ${file_src})

        set(file_out ${output_dir}/${file_resource_path})

        # Don't generate or copy the file in an in-source build if the source
        # and destination paths are the same, it will cause a ninja dependency
        # cycle at build time.
        if(NOT file_out STREQUAL file_absolute)
            get_filename_component(file_out_dir ${file_out} DIRECTORY)
            file(MAKE_DIRECTORY ${file_out_dir})

            execute_process(COMMAND
                ${CMAKE_COMMAND} -E copy_if_different ${file_absolute} ${file_out}
            )

            add_custom_command(OUTPUT ${file_out}
                COMMAND ${CMAKE_COMMAND} -E copy ${file_src} ${file_out}
                DEPENDS ${file_absolute}
                WORKING_DIRECTORY $<TARGET_PROPERTY:${target},SOURCE_DIR>
            )
            list(APPEND copied_files ${file_out})
        endif()
    endforeach()

    set(generated_sources_other_scope)
    foreach(qml_file_src IN LISTS arg_QML_FILES)
        # This is to facilitate updating code that used the earlier tech preview
        # API function qt6_target_qml_files()
        if(NOT qml_file_src MATCHES "\\.(js|mjs|qml)$")
            list(APPEND non_qml_files ${qml_file_src})
            continue()
        endif()

        # Mark QML files as source files, so that they do not appear in <Other Locations> in Creator
        # or other IDEs
        set_source_files_properties(${qml_file_src} HEADER_FILE_ONLY ON)
        target_sources(${target} PRIVATE ${qml_file_src})

        get_filename_component(file_absolute ${qml_file_src} ABSOLUTE)
        __qt_get_relative_resource_path_for_file(file_resource_path ${qml_file_src})

        # For the tooling steps below, run the tools on the copied qml file in
        # the build directory, not the source directory. This is required
        # because the tools may need to reference imported modules from
        # subdirectories, which would require those subdirectories to have
        # their generated qmldir files present. They also need to use the right
        # resource paths and the source locations might be structured quite
        # differently.

        # Add file to those processed by qmllint
        get_source_file_property(skip_qmllint ${qml_file_src} QT_QML_SKIP_QMLLINT)
        if(NOT no_lint AND NOT skip_qmllint)
            # The set of qml files to run qmllint on may be a subset of the
            # full set of files, so record these in a separate property.
            _qt_internal_target_enable_qmllint(${target})
            set_property(TARGET ${target} APPEND PROPERTY QT_QML_LINT_FILES ${file_absolute})
        endif()

        # Add qml file's type to qmldir
        get_source_file_property(skip_qmldir ${qml_file_src} QT_QML_SKIP_QMLDIR_ENTRY)
        if(NOT no_qmldir AND NOT skip_qmldir)
            get_source_file_property(qml_file_typename ${qml_file_src} QT_QML_SOURCE_TYPENAME)
            if (NOT qml_file_typename)
                get_filename_component(qml_file_ext ${qml_file_src} EXT)
                get_filename_component(qml_file_typename ${qml_file_src} NAME_WE)
            endif()

            # Do not add qmldir entries for lowercase names. Those are not components.
            if (qml_file_typename AND qml_file_typename MATCHES "^[A-Z]")
                if (qml_file_ext AND NOT qml_file_ext STREQUAL ".qml" AND NOT qml_file_ext STREQUAL ".ui.qml"
                        AND NOT qml_file_ext STREQUAL ".js" AND NOT qml_file_ext STREQUAL ".mjs")
                    message(AUTHOR_WARNING
                        "${qml_file_src} has a file extension different from .qml, .ui.qml, .js, "
                        "and .mjs. This leads to unexpected component names."
                    )
                endif()

                # We previously accepted the singular form of this property name
                # during tech preview. Issue a warning for that, but still
                # honor it. The plural form will override it if both are set.
                get_property(have_singular_property SOURCE ${qml_file_src}
                    PROPERTY QT_QML_SOURCE_VERSION SET
                )
                if(have_singular_property)
                    message(AUTHOR_WARNING
                        "The QT_QML_SOURCE_VERSION source file property has been replaced "
                        "by QT_QML_SOURCE_VERSIONS (i.e. plural rather than singular). "
                        "The singular form will eventually be removed, please update "
                        "the project to use the plural form instead for the file at:\n"
                        "  ${qml_file_src}"
                    )
                endif()
                get_source_file_property(qml_file_versions ${qml_file_src} QT_QML_SOURCE_VERSIONS)
                if(NOT qml_file_versions AND have_singular_property)
                    get_source_file_property(qml_file_versions ${qml_file_src} QT_QML_SOURCE_VERSION)
                endif()

                get_source_file_property(qml_file_singleton ${qml_file_src} QT_QML_SINGLETON_TYPE)
                get_source_file_property(qml_file_internal  ${qml_file_src} QT_QML_INTERNAL_TYPE)

                if (NOT qml_file_versions)
                    set(qml_file_versions ${qml_module_files_versions})
                endif()

                set(qmldir_file_contents "")
                foreach(qml_file_version IN LISTS qml_file_versions)
                    if (qml_file_singleton)
                        string(APPEND qmldir_file_contents "singleton ")
                    endif()
                    string(APPEND qmldir_file_contents "${qml_file_typename} ${qml_file_version} ${file_resource_path}\n")
                endforeach()

                if (qml_file_internal)
                    string(APPEND qmldir_file_contents "internal ${qml_file_typename} ${file_resource_path}\n")
                endif()

                set_property(TARGET ${target} APPEND_STRING PROPERTY
                    _qt_internal_qmldir_content "${qmldir_file_contents}"
                )
            endif()
        endif()

        # Run cachegen on the qml file, or if disabled, store the raw qml file in the resources
        get_source_file_property(skip_cachegen ${qml_file_src} QT_QML_SKIP_CACHEGEN)
        if(NOT no_cachegen AND NOT skip_cachegen)
            # We delay this to here to ensure that we only ever enable cachegen
            # after we know there will be at least one file to compile.
            get_target_property(is_cachegen_set_up ${target} _qt_cachegen_set_up)
            if(NOT is_cachegen_set_up)
                _qt_internal_target_enable_qmlcachegen(${target} resource_target ${qmlcachegen})
                list(APPEND output_targets ${resource_target})
            endif()

            # We ensured earlier that arg_PREFIX always ends with "/"
            file(TO_CMAKE_PATH "${arg_PREFIX}${file_resource_path}" file_resource_path)

            set_property(TARGET ${target} APPEND PROPERTY
                QT_QML_MODULE_RESOURCE_PATHS ${file_resource_path}
            )

            file(RELATIVE_PATH file_relative ${CMAKE_CURRENT_SOURCE_DIR} ${file_absolute})
            string(REGEX REPLACE "\\.(js|mjs|qml)$" "_\\1" compiled_file ${file_relative})
            string(REGEX REPLACE "[$#?]+" "_" compiled_file ${compiled_file})

            # The file name needs to be unique to work around an Integrity compiler issue.
            # Search for INTEGRITY_SYMBOL_UNIQUENESS in this file for details.
            set(compiled_file
                "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache/${target}_${compiled_file}.cpp")
            get_filename_component(out_dir ${compiled_file} DIRECTORY)

            if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config" AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.20")
                set(qmlcachegen_cmd "$<COMMAND_CONFIG:${qmlcachegen}>")
            else()
                set(qmlcachegen_cmd "${qmlcachegen}")
            endif()
            add_custom_command(
                OUTPUT ${compiled_file}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
                COMMAND
                    ${QT_TOOL_COMMAND_WRAPPER_PATH}
                    ${qmlcachegen_cmd}
                    --resource-path "${file_resource_path}"
                    ${cachegen_args}
                    -o "${compiled_file}"
                    "${file_absolute}"
                COMMAND_EXPAND_LISTS
                DEPENDS
                    ${qmlcachegen_cmd}
                    "${file_absolute}"
                    $<TARGET_PROPERTY:${target},_qt_generated_qrc_files>
                    "$<$<BOOL:${types_file}>:${types_file}>"
            )

            target_sources(${target} PRIVATE ${compiled_file})
            set_source_files_properties(${compiled_file} PROPERTIES
                SKIP_AUTOGEN ON
            )
            # The current scope automatically sees the file as generated, but the
            # target scope may not if it is different. Force it where we can.
            # We will also have to add the generated file to a target in this
            # scope at the end to ensure correct dependencies.
            get_target_property(target_source_dir ${target} SOURCE_DIR)
            if(NOT target_source_dir STREQUAL CMAKE_CURRENT_SOURCE_DIR)
                list(APPEND generated_sources_other_scope ${compiled_file})
                if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
                    set_source_files_properties(
                        ${compiled_file}
                        TARGET_DIRECTORY ${target}
                        PROPERTIES
                            SKIP_AUTOGEN TRUE
                            GENERATED TRUE
                    )
                endif()
            endif()
        endif()
    endforeach()

    if(ANDROID)
        _qt_internal_collect_qml_root_paths("${target}" ${arg_QML_FILES})
    endif()

    if(non_qml_files)
        list(JOIN non_qml_files "\n  " file_list)
        message(WARNING
            "Only .qml, .js or .mjs files should be added with QML_FILES. "
            "The following files should be added with RESOURCES instead:"
            "\n  ${file_list}"
        )
    endif()

    if(copied_files OR generated_sources_other_scope)
        if(CMAKE_VERSION VERSION_LESS 3.19)
            # Called from qt6_add_qml_module() and we know there can only be
            # this one call. With those constraints, we can use a custom target
            # to implement the necessary dependencies to get files copied to the
            # build directory when their source files change.
            add_custom_target(${target}_tooling ALL
                DEPENDS
                    ${copied_files}
                    ${generated_sources_other_scope}
            )
            add_dependencies(${target} ${target}_tooling)
        else()
            # We could be called multiple times and a custom target can only
            # have file-level dependencies added at the time the target is
            # created. Use an interface library instead, since we can add
            # private sources to those and have the library act as a build
            # system target from CMake 3.19 onward, and we can add the sources
            # progressively over multiple calls.
            if(NOT TARGET ${target}_tooling)
                add_library(${target}_tooling INTERFACE)
                add_dependencies(${target} ${target}_tooling)
            endif()
            target_sources(${target}_tooling PRIVATE
                ${copied_files}
                ${generated_sources_other_scope}
            )
        endif()
    endif()

    # Batch all the non-compiled qml sources into a single resource for this
    # call. Subsequent calls for the same target will be in their own separate
    # resource file.
    get_target_property(counter ${target} QT_QML_MODULE_RAW_QML_SETS)
    if(NOT counter)
        set(counter 0)
    endif()
    set(resource_name ${target}_raw_qml_${counter})
    set(resource_targets)
    qt6_add_resources(${target} ${resource_name}
        PREFIX ${arg_PREFIX}
        FILES ${arg_QML_FILES} ${arg_RESOURCES}
        OUTPUT_TARGETS resource_targets
    )
    math(EXPR counter "${counter} + 1")
    set_target_properties(${target} PROPERTIES QT_QML_MODULE_RAW_QML_SETS ${counter})
    list(APPEND output_targets ${resource_targets})

    if(arg_OUTPUT_TARGETS)
        set(${arg_OUTPUT_TARGETS} ${output_targets} PARENT_SCOPE)
    endif()

endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_target_qml_sources)
        qt6_target_qml_sources(${ARGV})
        cmake_parse_arguments(PARSE_ARGV 1 arg  "" "OUTPUT_TARGETS" "")
        if(arg_OUTPUT_TARGETS)
            set(${arg_OUTPUT_TARGETS} ${${arg_OUTPUT_TARGETS}} PARENT_SCOPE)
        endif()
    endfunction()
endif()

# target: Expected to be the backing target for a qml module. Certain target
#   properties normally set by qt6_add_qml_module() will be retrieved from this
#   target. (REQUIRED)
#
# MANUAL_MOC_JSON_FILES: Specifies a list of json files, generated by a manual
#   moc call, to extract metatypes. (OPTIONAL)
#
function(_qt_internal_qml_type_registration target)
    set(args_option __QT_INTERNAL_INSTALL_METATYPES_JSON)
    set(args_single "")
    set(args_multi  MANUAL_MOC_JSON_FILES)

    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${args_option}" "${args_single}" "${args_multi}"
    )
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown/unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    get_target_property(import_name ${target} QT_QML_MODULE_URI)
    if (NOT import_name)
        message(FATAL_ERROR "Target ${target} is not a QML module")
    endif()
    get_target_property(qmltypes_output_name ${target} QT_QML_MODULE_TYPEINFO)
    if (NOT qmltypes_output_name)
        get_target_property(compile_definitions_list ${target} COMPILE_DEFINITIONS)
        list(FIND compile_definitions_list QT_PLUGIN is_a_plugin)
        if (is_a_plugin GREATER_EQUAL 0)
            set(qmltypes_output_name "plugins.qmltypes")
        else()
            set(qmltypes_output_name ${target}.qmltypes)
        endif()
    endif()

    set(meta_types_json_args "")
    if(arg_MANUAL_MOC_JSON_FILES)
        list(APPEND meta_types_json_args "MANUAL_MOC_JSON_FILES" ${arg_MANUAL_MOC_JSON_FILES})
    endif()

    # Don't install the metatypes json files by default for user project created qml modules.
    # Only install them for Qt provided qml modules.
    if(NOT arg___QT_INTERNAL_INSTALL_METATYPES_JSON)
        list(APPEND meta_types_json_args __QT_INTERNAL_NO_INSTALL)
    endif()
    qt6_extract_metatypes(${target} ${meta_types_json_args})

    get_target_property(import_version ${target} QT_QML_MODULE_VERSION)
    get_target_property(output_dir ${target} QT_QML_MODULE_OUTPUT_DIRECTORY)
    get_target_property(target_source_dir ${target} SOURCE_DIR)
    get_target_property(target_binary_dir ${target} BINARY_DIR)
    get_target_property(target_metatypes_file ${target} INTERFACE_QT_META_TYPES_BUILD_FILE)
    if (NOT target_metatypes_file)
        message(FATAL_ERROR "Target ${target} does not have a meta types file")
    endif()

    # Extract major and minor version (could also have patch part, but we don't
    # need that here)
    if (import_version MATCHES "^([0-9]+)\\.([0-9]+)")
        set(major_version ${CMAKE_MATCH_1})
        set(minor_version ${CMAKE_MATCH_2})
    else()
        message(FATAL_ERROR
            "Invalid module version number '${import_version}'. "
            "Expected VersionMajor.VersionMinor."
        )
    endif()

    # check if plugins.qmltypes is already defined
    get_target_property(target_plugin_qmltypes ${target} QT_QML_MODULE_PLUGIN_TYPES_FILE)
    if (target_plugin_qmltypes)
        message(FATAL_ERROR "Target ${target} already has a qmltypes file set.")
    endif()

    set(cmd_args)
    set(plugin_types_file "${output_dir}/${qmltypes_output_name}")
    set(generated_marker_file "${target_binary_dir}/.generated/${qmltypes_output_name}")
    get_filename_component(generated_marker_dir "${generated_marker_file}" DIRECTORY)
    set_target_properties(${target} PROPERTIES
        QT_QML_MODULE_PLUGIN_TYPES_FILE ${plugin_types_file}
    )
    list(APPEND cmd_args
        --generate-qmltypes=${plugin_types_file}
        --import-name=${import_name}
        --major-version=${major_version}
        --minor-version=${minor_version}
    )

    # Add past minor versions
    get_target_property(past_major_versions ${target} QT_QML_MODULE_PAST_MAJOR_VERSIONS)

    if (past_major_versions OR past_major_versions STREQUAL "0")
        foreach (past_major_version ${past_major_versions})
            list(APPEND cmd_args
                --past-major-version ${past_major_version}
            )
        endforeach()
    endif()

    # Run a script to recursively evaluate all the metatypes.json files in order
    # to collect all foreign types.
    string(TOLOWER "${target}_qmltyperegistrations.cpp" type_registration_cpp_file_name)
    set(foreign_types_file "${target_binary_dir}/qmltypes/${target}_foreign_types.txt")
    set(type_registration_cpp_file "${target_binary_dir}/${type_registration_cpp_file_name}")

    # Enable evaluation of metatypes.json source interfaces
    set_target_properties(${target} PROPERTIES QT_CONSUMES_METATYPES TRUE)
    set(genex_list "$<REMOVE_DUPLICATES:$<FILTER:$<TARGET_PROPERTY:${target},SOURCES>,INCLUDE,metatypes.json$>>")
    set(genex_main "$<JOIN:${genex_list},$<COMMA>>")
    file(GENERATE OUTPUT "${foreign_types_file}"
        CONTENT "$<IF:$<BOOL:${genex_list}>,--foreign-types=${genex_main},\n>"
    )

    list(APPEND cmd_args
        "@${foreign_types_file}"
    )

    if (TARGET ${target}Private)
        list(APPEND cmd_args --private-includes)
    endif()

    get_target_property(target_metatypes_json_file ${target} INTERFACE_QT_META_TYPES_BUILD_FILE)
    if (NOT target_metatypes_json_file)
        message(FATAL_ERROR "Need target metatypes.json file")
    endif()

    cmake_policy(PUSH)

    set(registration_cpp_file_dep_args)
    if (CMAKE_GENERATOR MATCHES "Ninja" OR
        (CMAKE_VERSION VERSION_GREATER_EQUAL 3.20 AND CMAKE_GENERATOR MATCHES "Makefiles"))
        if(POLICY CMP0116)
            # Without explicitly setting this policy to NEW, we get a warning
            # even though we ensure there's actually no problem here.
            # See https://gitlab.kitware.com/cmake/cmake/-/issues/21959
            cmake_policy(SET CMP0116 NEW)
            set(relative_to_dir ${CMAKE_CURRENT_BINARY_DIR})
        else()
            set(relative_to_dir ${CMAKE_BINARY_DIR})
        endif()
        set(dependency_file_cpp "${target_binary_dir}/qmltypes/${type_registration_cpp_file_name}.d")
        set(registration_cpp_file_dep_args DEPFILE ${dependency_file_cpp})
        file(RELATIVE_PATH cpp_file_name "${relative_to_dir}" "${type_registration_cpp_file}")
        file(GENERATE OUTPUT "${dependency_file_cpp}"
            CONTENT "${cpp_file_name}: $<IF:$<BOOL:${genex_list}>,\\\n$<JOIN:${genex_list}, \\\n>, \\\n>"
        )
    endif()

    add_custom_command(
        OUTPUT
            ${type_registration_cpp_file}
            ${plugin_types_file}
        DEPENDS
            ${foreign_types_file}
            ${target_metatypes_json_file}
            ${QT_CMAKE_EXPORT_NAMESPACE}::qmltyperegistrar
            "$<$<BOOL:${genex_list}>:${genex_list}>"
        COMMAND
            ${QT_TOOL_COMMAND_WRAPPER_PATH}
            $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmltyperegistrar>
            ${cmd_args}
            -o ${type_registration_cpp_file}
            ${target_metatypes_json_file}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "${generated_marker_dir}"
        COMMAND
            ${CMAKE_COMMAND} -E touch "${generated_marker_file}"
        ${registration_cpp_file_dep_args}
        COMMENT "Automatic QML type registration for target ${target}"
    )

    cmake_policy(POP)

    # The ${target}_qmllint targets need to depend on the generation of all
    # *.qmltypes files in the build. We have no way of reliably working out
    # which QML modules a given target depends on at configure time, so we
    # have to be conservative and make ${target}_qmllint targets depend on all
    # *.qmltypes files. We need to provide a target for those dependencies
    # here. Note that we can't use ${target} itself for those dependencies
    # because the user might want to run qmllint without having to build the
    # QML module.
    add_custom_target(${target}_qmltyperegistration
        DEPENDS
            ${type_registration_cpp_file}
            ${plugin_types_file}
    )
    if(NOT TARGET all_qmltyperegistrations)
        add_custom_target(all_qmltyperegistrations)
    endif()
    add_dependencies(all_qmltyperegistrations ${target}_qmltyperegistration)

    # Both ${target} (via target_sources) and ${target}_qmltyperegistration (via add_custom_target
    # DEPENDS option) depend on ${type_registration_cpp_file}.
    # The new Xcode build system requires a common target to drive the generation of files,
    # otherwise project configuration fails.
    # Make ${target} the common target, by adding it as a dependency for
    # ${target}_qmltyperegistration.
    # The consequence is that the ${target}_qmllint target will now first build ${target} when using
    # the Xcode generator (mostly only relevant for projects using Qt for iOS).
    # See QTBUG-95763.
    if(CMAKE_GENERATOR STREQUAL "Xcode")
        add_dependencies(${target}_qmltyperegistration ${target})
    endif()

    target_sources(${target} PRIVATE ${type_registration_cpp_file})

    # FIXME: The generated .cpp file has usually lost the path information for
    #        the headers it #include's. Since these generated .cpp files are in
    #        the build directory away from those headers, the header search path
    #        has to be augmented to ensure they can be found. We don't know what
    #        paths are needed, but add the source directory to at least handle
    #        the common case of headers in the same directory as the target.
    #        See QTBUG-93443.
    target_include_directories(${target} PRIVATE ${target_source_dir})

    # Circumvent "too many sections" error when doing a 32 bit debug build on Windows with
    # MinGW.
    set(additional_source_files_properties "")
    if(MINGW)
        set(additional_source_files_properties "COMPILE_OPTIONS" "-Wa,-mbig-obj")
    elseif(MSVC)
        set(additional_source_files_properties "COMPILE_OPTIONS" "/bigobj")
    endif()
    set_source_files_properties(${type_registration_cpp_file} PROPERTIES
        SKIP_AUTOGEN ON
        ${additional_source_files_properties}
    )
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
        set_source_files_properties(
            ${type_registration_cpp_file}
            TARGET_DIRECTORY ${target}
            PROPERTIES
                SKIP_AUTOGEN TRUE
                GENERATED TRUE
                ${additional_source_files_properties}
        )
    endif()

    target_include_directories(${target} PRIVATE
        $<TARGET_PROPERTY:${QT_CMAKE_EXPORT_NAMESPACE}::QmlPrivate,INTERFACE_INCLUDE_DIRECTORIES>
    )
endfunction()

function(qt6_qml_type_registration)
    message(FATAL_ERROR
        "This function, previously available under Technical Preview, has been removed. "
        "Please use qt6_add_qml_module() instead."
    )
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_qml_type_registration)
        message(FATAL_ERROR
            "This function, previously available under Technical Preview, has been removed. "
            "Please use qt_add_qml_module() instead."
        )
    endfunction()
endif()


# This function is called as a finalizer in qt6_finalize_executable() for any
# target that links against the Qml library for a statically built Qt.
function(qt6_import_qml_plugins target)
    if(QT6_IS_SHARED_LIBS_BUILD)
        return()
    endif()

    # Protect against being called multiple times in case we are being called
    # explicitly before the finalizer is invoked.
    get_target_property(alreadyImported ${target} _QT_QML_PLUGINS_IMPORTED)
    if(alreadyImported)
        return()
    endif()
    set_target_properties(${target} PROPERTIES _QT_QML_PLUGINS_IMPORTED TRUE)

    set(options)
    set(oneValueArgs "PATH_TO_SCAN")   # Internal option, may be removed
    set(multiValueArgs)

    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if(NOT arg_PATH_TO_SCAN)
        set(arg_PATH_TO_SCAN "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown/unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    # Find location of qmlimportscanner.
    get_target_property(tool_path ${QT_CMAKE_EXPORT_NAMESPACE}::qmlimportscanner IMPORTED_LOCATION)
    if(NOT tool_path)
        set(configs "RELWITHDEBINFO;RELEASE;MINSIZEREL;DEBUG")
        foreach(config ${configs})
            get_target_property(tool_path Qt6::qmlimportscanner IMPORTED_LOCATION_${config})
            if(tool_path)
                break()
            endif()
        endforeach()
    endif()

    if(NOT EXISTS "${tool_path}")
        message(FATAL_ERROR "The package \"QmlImportScanner\" references the file
   \"${tool_path}\"
but this file does not exist.  Possible reasons include:
* The file was deleted, renamed, or moved to another location.
* An install or uninstall procedure did not complete successfully.
* The installation package was faulty.
")
    endif()

    # Find location of qml dir.
    # TODO: qt.prf implies that there might be more than one qml import path to
    #       pass to qmlimportscanner.
    set(qml_path "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML}")

    # Small macro to avoid duplicating code in two different loops.
    macro(_qt6_QmlImportScanner_parse_entry)
        # TODO: Should CLASSNAME be changed to CLASS_NAME? It is generated by
        #       the qmlimportscanner, not CMake code.
        set(entry_name "qml_import_scanner_import_${idx}")
        cmake_parse_arguments("entry"
            ""
            "CLASSNAME;NAME;PATH;PLUGIN;RELATIVEPATH;TYPE;VERSION;LINKTARGET"
            ""
            ${${entry_name}}
        )
    endmacro()

    # Run qmlimportscanner and include the generated cmake file.
    set(qml_imports_file_path
        "${CMAKE_CURRENT_BINARY_DIR}/.qt_plugins/Qt6_QmlPlugins_Imports_${target}.cmake"
    )
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/.qt_plugins)

    set(cmd_args
        -rootPath "${arg_PATH_TO_SCAN}"
        -cmake-output
        -importPath "${qml_path}"
    )
    get_target_property(qml_import_path ${target} QT_QML_IMPORT_PATH)

    if (qml_import_path)
        list(APPEND cmd_args ${qml_import_path})
    endif()

    # Facilitate self-import so we can find the qmldir file
    get_target_property(module_out_dir ${target} QT_QML_MODULE_OUTPUT_DIRECTORY)
    if(module_out_dir)
        list(APPEND cmd_args "${module_out_dir}")
    endif()

    # Find qmldir files we copied to the build directory
    if(NOT "${QT_QML_OUTPUT_DIRECTORY}" STREQUAL "")
        if(EXISTS "${QT_QML_OUTPUT_DIRECTORY}")
            list(APPEND cmd_args "${QT_QML_OUTPUT_DIRECTORY}")
        endif()
    else()
        list(APPEND cmd_args "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    # All of the module's .qml files will be listed in one of the generated
    # .qrc files, so there's no need to list the files individually. We provide
    # the .qrc files instead because they have the additional information for
    # each file's resource alias.
    get_target_property(qrc_files ${target} _qt_generated_qrc_files)
    if (qrc_files)
        list(APPEND cmd_args "-qrcFiles" ${qrc_files})
    endif()

    # Use a response file to avoid command line length issues if we have a lot
    # of arguments on the command line
    string(LENGTH "${cmd_args}" length)
    if(length GREATER 240)
        set(rsp_file ${CMAKE_CURRENT_BINARY_DIR}/.qt_plugins/Qt6_QmlPlugins_Imports_${target}.rsp)
        list(JOIN cmd_args "\n" rsp_file_content)
        file(WRITE ${rsp_file} "${rsp_file_content}")
        set(cmd_args "@${rsp_file}")
    endif()

    get_target_property(target_source_dir ${target} SOURCE_DIR)

    message(VERBOSE "Running qmlimportscanner to find QML plugins needed by ${target}.")
    set(import_scanner_args ${QT_TOOL_COMMAND_WRAPPER_PATH} ${tool_path} ${cmd_args})
    list(JOIN import_scanner_args " " import_scanner_args_string)
    message(DEBUG "qmlimportscanner command: ${import_scanner_args_string}")
    execute_process(COMMAND ${import_scanner_args}
        OUTPUT_FILE "${qml_imports_file_path}"
        WORKING_DIRECTORY ${target_source_dir}
    )

    include("${qml_imports_file_path}" OPTIONAL RESULT_VARIABLE qml_imports_file_path_found)
    if(NOT qml_imports_file_path_found)
        message(FATAL_ERROR
            "Could not find ${qml_imports_file_path} which was supposed to be "
            "generated by qmlimportscanner after processing target ${target}."
        )
    endif()

    # Parse the generated cmake file.
    # It is possible for the scanner to find no usage of QML, in which case the import count is 0.
    if(qml_import_scanner_imports_count)
        set(added_plugins "")
        set(plugins_to_link "")
        set(plugin_inits_to_link "")

        foreach(idx RANGE "${qml_import_scanner_imports_count}")
            _qt6_QmlImportScanner_parse_entry()
            if(entry_PATH AND entry_PLUGIN)
                # Sometimes a plugin appears multiple times with different versions.
                # Make sure to process it only once.
                list(FIND added_plugins "${entry_PLUGIN}" _index)
                if(NOT _index EQUAL -1)
                    continue()
                endif()
                list(APPEND added_plugins "${entry_PLUGIN}")

                # Link against the Qml plugin.
                # For plugins provided by Qt, we assume those plugin targets are already defined
                # (typically brought in via find_package(Qt6...) ).
                # For other plugins, the targets can come from the project itself.
                #
                if(entry_LINKTARGET)
                    if(TARGET ${entry_LINKTARGET})
                        list(APPEND plugins_to_link "${entry_LINKTARGET}")
                    else()
                        message(WARNING
                            "The qml plugin '${entry_PLUGIN}' is a dependency of '${target}', "
                            "but the link target it defines (${entry_LINKTARGET}) does not exist "
                            "in the current scope. The plugin will not be linked."
                        )
                    endif()
                elseif(TARGET ${entry_PLUGIN})
                    list(APPEND plugins_to_link "${entry_PLUGIN}")
                else()
                    # TODO: QTBUG-94605 Figure out if this is a reasonable scenario to support
                    message(WARNING
                        "The qml plugin '${entry_PLUGIN}' is a dependency of '${target}', "
                        "but there is no target by that name in the current scope. The plugin will "
                        "not be linked."
                    )
                endif()
            endif()
        endforeach()

        if(plugins_to_link)
            # If ${target} is an executable or a shared library, link the plugins directly to
            # the target.
            # If ${target} is a static or INTERFACE library, the plugins should be propagated
            # across those libraries to the end target (executable or shared library).
            # The plugin initializers will be linked via usage requirements from the plugin target.
            get_target_property(target_type ${target} TYPE)
            if(target_type STREQUAL "EXECUTABLE" OR target_type STREQUAL "SHARED_LIBRARY")
                set(link_type "PRIVATE")
            else()
                set(link_type "INTERFACE")
            endif()
            target_link_libraries("${target}" ${link_type} ${plugins_to_link})
        endif()
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_import_qml_plugins)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_import_qml_plugins(${ARGV})
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_import_qml_plugins(${ARGV})
        endif()
    endfunction()
endif()

function(_qt_internal_add_static_qml_plugin_dependencies plugin_target backing_target)
    # Protect against multiple calls of qt_add_qml_plugin.
    get_target_property(plugin_deps_added "${plugin_target}" _qt_extra_static_qml_plugin_deps_added)
    if(plugin_deps_added)
        return()
    endif()
    set_target_properties("${plugin_target}" PROPERTIES _qt_extra_static_qml_plugin_deps_added TRUE)

    # Get the install plugin target name, which we will need for filtering later on.
    if(TARGET "${backing_target}")
        get_target_property(installed_plugin_target
                            "${backing_target}" _qt_qml_module_installed_plugin_target)
    endif()

    if(NOT backing_target STREQUAL plugin_target AND TARGET "${backing_target}")
        set(has_backing_lib TRUE)
    else()
        set(has_backing_lib FALSE)
    endif()

    get_target_property(plugin_type ${plugin_target} TYPE)
    set(skip_prl_marker "$<BOOL:QT_IS_PLUGIN_GENEX>")

    # If ${plugin_target} is a static qml plugin, recursively get its private dependencies (and its
    # backing lib private deps), identify which of those are qml modules, extract any associated qml
    # plugin target from those qml modules and make them dependencies of ${plugin_target}.
    #
    # E.g. this ensures that if a user project links directly to the static qtquick2plugin plugin
    # target (note the plugin target, not the backing lib) it will automatically also link to
    # Quick's transitive plugin dependencies: qmlplugin, modelsplugin and workerscriptplugin, in
    # addition to the the Qml, QmlModels and QmlWorkerScript backing libraries.
    #
    # Note this logic is not specific to qtquick2plugin, it applies to all static qml plugins.
    #
    # This eliminates the needed boilerplate to link to the full transitive closure of qml plugins
    # in user projects that don't want to use qmlimportscanner / qt_import_qml_plugins.
    set(additional_plugin_deps "")

    if(plugin_type STREQUAL "STATIC_LIBRARY")
        set(all_private_deps "")

        # We walk both plugin_target and backing_lib private deps because they can have differing
        # dependencies and we want to consider all of them.
        __qt_internal_collect_all_target_dependencies(
            "${plugin_target}" plugin_private_deps)
        if(plugin_private_deps)
            list(APPEND all_private_deps ${plugin_private_deps})
        endif()

        if(has_backing_lib)
            __qt_internal_collect_all_target_dependencies(
                "${backing_target}" backing_lib_private_deps)
            if(backing_lib_private_deps)
                list(APPEND all_private_deps ${backing_lib_private_deps})
            endif()
        endif()

        foreach(dep IN LISTS all_private_deps)
            if(NOT TARGET "${dep}")
                continue()
            endif()
            get_target_property(dep_type ${dep} TYPE)
            if(dep_type STREQUAL "STATIC_LIBRARY")
                set(associated_qml_plugin "")

                # Check if the target has an associated imported qml plugin (like a Qt-provided
                # one).
                get_target_property(associated_qml_plugin_candidate ${dep}
                    _qt_qml_module_installed_plugin_target)

                if(associated_qml_plugin_candidate AND TARGET "${associated_qml_plugin_candidate}")
                    set(associated_qml_plugin "${associated_qml_plugin_candidate}")
                endif()

                # Check if the target has an associated qml plugin that's built as part of the
                # current project (non-installed one, so without a target namespace prefix).
                get_target_property(associated_qml_plugin_candidate ${dep}
                    _qt_qml_module_plugin_target)

                if(NOT associated_qml_plugin AND
                        associated_qml_plugin_candidate
                        AND TARGET "${associated_qml_plugin_candidate}")
                    set(associated_qml_plugin "${associated_qml_plugin_candidate}")
                endif()

                # We need to filter out adding the plugin_target as a dependency to itself,
                # when walking the backing lib of the plugin_target.
                if(associated_qml_plugin
                        AND NOT associated_qml_plugin STREQUAL plugin_target
                        AND NOT associated_qml_plugin STREQUAL installed_plugin_target)
                    # Abuse a genex marker, to skip the dependency to be added into prl files.
                    # TODO: Introduce a more generic marker name in qtbase specifically
                    # for skipping deps in prl file deps generation.
                    set(wrapped_associated_qml_plugin
                        "$<${skip_prl_marker}:$<TARGET_NAME:${associated_qml_plugin}>>")

                    if(NOT wrapped_associated_qml_plugin IN_LIST additional_plugin_deps)
                        list(APPEND additional_plugin_deps "${wrapped_associated_qml_plugin}")
                    endif()
                endif()
            endif()
        endforeach()
    endif()

    if(additional_plugin_deps)
        target_link_libraries(${plugin_target} PRIVATE ${additional_plugin_deps})
    endif()
endfunction()

# The function returns the output name of a qml plugin that will be used as library output
# name and in a qmldir file as the 'plugin <plugin_output_name>' record.
function(_qt_internal_get_qml_plugin_output_name out_var plugin_target)
    cmake_parse_arguments(arg
        ""
        "BACKING_TARGET;TARGET_PATH;URI"
        ""
        ${ARGN}
    )
    set(plugin_name)
    if(TARGET ${plugin_target})
        get_target_property(plugin_name ${plugin_target} OUTPUT_NAME)
    endif()
    if(NOT plugin_name)
        set(plugin_name "${plugin_target}")
    endif()

    if(ANDROID)
        # In Android all plugins are stored in directly the /libs directory. This means that plugin
        # names must be unique in scope of apk. To make this work we prepend uri-based prefix to
        # each qml plugin in case if users don't use the manually written qmldir files.
        get_target_property(no_generate_qmldir ${target} QT_QML_MODULE_NO_GENERATE_QMLDIR)
        if(TARGET "${arg_BACKING_TARGET}")
            get_target_property(no_generate_qmldir ${arg_BACKING_TARGET}
                QT_QML_MODULE_NO_GENERATE_QMLDIR)

            # Adjust Qml plugin names on Android similar to qml_plugin.prf which calls
            # $$qt5LibraryTarget($$TARGET, "qml/$$TARGETPATH/").
            # Example plugin names:
            # qtdeclarative
            #   TARGET_PATH: QtQml/Models
            #   file name:   libqml_QtQml_Models_modelsplugin_x86_64.so
            # qtquickcontrols2
            #   TARGET_PATH: QtQuick/Controls.2/Material
            #   file name:
            #     libqml_QtQuick_Controls.2_Material_qtquickcontrols2materialstyleplugin_x86_64.so
            if(NOT arg_TARGET_PATH)
                get_target_property(arg_TARGET_PATH ${arg_BACKING_TARGET}
                QT_QML_MODULE_TARGET_PATH)
            endif()
        endif()
        if(arg_TARGET_PATH)
            string(REPLACE "/" "_" android_plugin_name_infix_name "${arg_TARGET_PATH}")
        else()
            string(REPLACE "." "_" android_plugin_name_infix_name "${arg_URI}")
        endif()

        # If plugin supposed to use manually written qmldir file we don't prepend the uri-based
        # prefix to the plugin output name. User should keep the file name of a QML plugin in
        # qmldir the same as the name of plugin on a file system. Exception is the
        # ABI-/platform-specific suffix that has the separate processing and should not be
        # a part of plugin name in qmldir.
        if(NOT no_generate_qmldir)
            set(plugin_name
                "qml_${android_plugin_name_infix_name}_${plugin_name}")
        endif()
    endif()

    set(${out_var} "${plugin_name}" PARENT_SCOPE)
endfunction()

# Used to add extra dependencies between ${target} and ${dep_target} qml plugins in a static
# Qt build, without creating a dependency in the genereated qmake .prl files.
# These dependencies make manual linking to static plugins a nicer experience for users that don't
# want to use qt_import_qml_plugins.
function(_qt_internal_add_qml_static_plugin_dependency target dep_target)
    if(NOT BUILD_SHARED_LIBS)
        # Abuse a genex marker, to skip the dependency to be added into prl files.
        # TODO: Introduce a more generic marker name in qtbase specifically
        # for skipping deps in prl file deps generation.
        set(skip_prl_marker "$<BOOL:QT_IS_PLUGIN_GENEX>")
        target_link_libraries("${target}" PRIVATE
            "$<${skip_prl_marker}:$<TARGET_NAME:${dep_target}>>")
    endif()
endfunction()
