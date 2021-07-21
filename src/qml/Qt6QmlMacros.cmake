#
# Q6QmlMacros
#

set(__qt_qml_macros_module_base_dir "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")

function(qt6_add_qml_module target)
    set(args_option
        STATIC
        SHARED
        DESIGNER_SUPPORTED
        NO_PLUGIN_OPTIONAL
        NO_CREATE_PLUGIN_TARGET
        NO_GENERATE_PLUGIN_SOURCE
        NO_GENERATE_QMLTYPES
        NO_GENERATE_QMLDIR
        NO_LINT
        NO_CACHEGEN
        # TODO: Remove once all usages have also been removed
        SKIP_TYPE_REGISTRATION
    )

    set(args_single
        PLUGIN_TARGET
        OUTPUT_TARGETS
        RESOURCE_PREFIX
        URI
        TARGET_PATH
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
        __QT_INTERNAL_QT_LIBINFIX  # Used only by _qt_internal_target_generate_qmldir()
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

    # Provide defaults for options that have one
    if (NOT arg_TARGET_PATH)
        string(REPLACE "." "/" arg_TARGET_PATH ${arg_URI})
    endif()

    set(is_executable FALSE)
    if(TARGET ${target})
        get_target_property(backing_target_type ${target} TYPE)
        get_target_property(is_android_executable "${target}" _qt_is_android_executable)
        if (backing_target_type STREQUAL "EXECUTABLE" OR is_android_executable)
            set(is_executable TRUE)
        endif()
    endif()

    if(NOT arg_NO_CREATE_PLUGIN_TARGET AND NOT DEFINED arg_PLUGIN_TARGET AND NOT is_executable)
        set(arg_PLUGIN_TARGET ${target}plugin)
    endif()
    if(NOT DEFINED arg_PLUGIN_TARGET)
        set(arg_PLUGIN_TARGET "")  # Simplifies things a bit further below
    endif()

    set(no_gen_source)
    if(arg_NO_GENERATE_PLUGIN_SOURCE)
        set(no_gen_source NO_GENERATE_PLUGIN_SOURCE)
    endif()

    set(lib_type "")
    if(arg_STATIC)
        set(lib_type STATIC)
    elseif(arg_SHARED)
        set(lib_type SHARED)
    endif()

    if(arg_OUTPUT_DIRECTORY)
        get_filename_component(arg_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
            ABSOLUTE BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}"
        )
    else()
        if("${QT_QML_OUTPUT_DIRECTORY}" STREQUAL "")
            set(arg_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        else()
            if(NOT IS_ABSOLUTE "${QT_QML_OUTPUT_DIRECTORY}")
                message(FATAL_ERROR
                    "QT_QML_OUTPUT_DIRECTORY must be an absolute path, but given: "
                    "${QT_QML_OUTPUT_DIRECTORY}"
                )
            endif()
            set(arg_OUTPUT_DIRECTORY ${QT_QML_OUTPUT_DIRECTORY}/${arg_TARGET_PATH})
        endif()
    endif()

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
        if(arg_STATIC OR arg_SHARED)
            message(FATAL_ERROR
                "Cannot use STATIC or SHARED keyword when passed an existing target (${target})"
            )
        endif()
        if(arg_PLUGIN_TARGET STREQUAL target)
            # Insert the plugin's URI into its meta data to enable usage
            # of static plugins in QtDeclarative (like in mkspecs/features/qml_plugin.prf).
            set_property(TARGET ${target} APPEND PROPERTY
                AUTOMOC_MOC_OPTIONS "-Muri=${arg_URI}"
            )
        endif()
    else()
        if(arg_STATIC AND arg_SHARED)
            message(FATAL_ERROR
                "Both STATIC and SHARED specified, at most one can be given"
            )
        endif()

        if(arg_PLUGIN_TARGET STREQUAL target)
            if(arg_NO_CREATE_PLUGIN_TARGET AND NOT TARGET ${target})
                message(FATAL_ERROR
                    "NO_CREATE_PLUGIN_TARGET was given, but PLUGIN_TARGET is "
                    "the same as the backing target (which is allowed) and the "
                    "target does not exist. Either ensure the target is already "
                    "created or do not specify NO_CREATE_PLUGIN_TARGET."
                )
            endif()
            qt6_add_qml_plugin(${target}
                ${lib_type}
                ${no_gen_source}
                OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
                URI ${arg_URI}
                CLASS_NAME ${arg_CLASS_NAME}
            )
        else()
            add_library(${target} ${lib_type})
            if(ANDROID)
                # TODO: Check if we need to do this for a backing library
                qt6_android_apply_arch_suffix(${target})
            endif()
        endif()
    endif()

    if(NOT target STREQUAL Qml)
        target_link_libraries(${target} PRIVATE ${QT_CMAKE_EXPORT_NAMESPACE}::Qml)
    endif()

    if(NOT arg_TYPEINFO)
        set(arg_TYPEINFO ${target}.qmltypes)
    endif()

    # Make the prefix conform to the following:
    #   - Starts with a "/"
    #   - Does not end with a "/" unless the prefix is exactly "/"
    if(NOT arg_RESOURCE_PREFIX)
        set(arg_RESOURCE_PREFIX "/")
    endif()
    if(NOT arg_RESOURCE_PREFIX MATCHES "^/")
        string(PREPEND arg_RESOURCE_PREFIX "/")
    endif()
    if(arg_RESOURCE_PREFIX MATCHES [[(.*)/$]])
        set(arg_RESOURCE_PREFIX "${CMAKE_MATCH_1}")
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

    set(qt_qml_module_resource_prefix "${arg_RESOURCE_PREFIX}/${arg_TARGET_PATH}")

    set_target_properties(${target} PROPERTIES
        QT_QML_MODULE_NO_LINT "${arg_NO_LINT}"
        QT_QML_MODULE_NO_CACHEGEN "${arg_NO_CACHEGEN}"
        QT_QML_MODULE_NO_GENERATE_QMLDIR "${arg_NO_GENERATE_QMLDIR}"
        QT_QML_MODULE_NO_PLUGIN_OPTIONAL "${arg_NO_PLUGIN_OPTIONAL}"
        QT_QML_MODULE_URI "${arg_URI}"
        QT_QML_MODULE_TARGET_PATH "${arg_TARGET_PATH}"
        QT_QML_MODULE_VERSION "${arg_VERSION}"
        QT_QML_MODULE_CLASS_NAME "${arg_CLASS_NAME}"
        QT_QML_MODULE_LIBINFIX "${arg___QT_INTERNAL_QT_LIBINFIX}"
        QT_QML_MODULE_PLUGIN_TARGET "${arg_PLUGIN_TARGET}"
        QT_QML_MODULE_DESIGNER_SUPPORTED "${arg_DESIGNER_SUPPORTED}"
        QT_QML_MODULE_OUTPUT_DIR "${arg_OUTPUT_DIRECTORY}"
        QT_QML_MODULE_RESOURCE_PREFIX "${qt_qml_module_resource_prefix}"
        QT_QML_PAST_MAJOR_VERSIONS "${arg_PAST_MAJOR_VERSIONS}"
        QT_QMLTYPES_FILENAME "${arg_TYPEINFO}"

        # TODO: Check how this is used by qt6_android_generate_deployment_settings()
        QT_QML_IMPORT_PATH "${arg_IMPORT_PATH}"
    )
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
        qt6_qml_type_registration(${target})
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
        set_source_files_properties(${arg_OUTPUT_DIRECTORY}/qmldir
            PROPERTIES QT_RESOURCE_ALIAS "qmldir"
        )
        set(resource_targets)
        qt6_add_resources(${target} ${qmldir_resource_name}
            FILES ${arg_OUTPUT_DIRECTORY}/qmldir
            PREFIX "${qt_qml_module_resource_prefix}"
            OUTPUT_TARGETS resource_targets
        )
        list(APPEND output_targets ${resource_targets})
    endif()

    if(arg_PLUGIN_TARGET AND NOT arg_NO_CREATE_PLUGIN_TARGET AND NOT is_executable)
        # This also handles the case where ${arg_PLUGIN_TARGET} already exists,
        # including where it is the same as ${target}. If ${arg_PLUGIN_TARGET}
        # already exists, it will update the necessary things that are specific
        # to qml plugins.
        qt6_add_qml_plugin(${arg_PLUGIN_TARGET}
            ${lib_type}
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

    # Build an init object library for static plugins.
    if(TARGET "${arg_PLUGIN_TARGET}")
        get_target_property(lib_type ${arg_PLUGIN_TARGET} TYPE)
        if(lib_type STREQUAL "STATIC_LIBRARY")
            __qt_internal_add_static_plugin_init_object_library(
                "${arg_PLUGIN_TARGET}" plugin_init_target)
            list(APPEND output_targets ${plugin_init_target})
        endif()
    endif()

    if(NOT arg_NO_GENERATE_QMLDIR)
        if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.19.0")
            # Defer the write to allow more qml files to be added later by calls to
            # qt6_target_qml_sources(). We wrap the deferred call with EVAL CODE
            # so that ${target} is evaluated now rather than the end of the scope.
            cmake_language(EVAL CODE
                "cmake_language(DEFER CALL _qt_internal_write_deferred_qmldir_file ${target})"
            )
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
    endfunction()
endif()

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

    # Facilitate self-import so it can find the qmldir file
    list(APPEND import_args -I "${CMAKE_CURRENT_BINARY_DIR}")

    if(NOT "${QT_QML_OUTPUT_DIRECTORY}" STREQUAL "")
        list(APPEND import_args -I "${QT_QML_OUTPUT_DIRECTORY}")
    endif()

    set(cmd
        ${QT_TOOL_COMMAND_WRAPPER_PATH}
        ${QT_CMAKE_EXPORT_NAMESPACE}::qmllint
        ${import_args}
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
    add_library("${resource_target}" OBJECT "${generated_source_code}")

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

    get_target_property(plugin_target ${target} QT_QML_MODULE_PLUGIN_TARGET)
    if(plugin_target)
        get_target_property(no_plugin_optional ${target} QT_QML_MODULE_NO_PLUGIN_OPTIONAL)
        if(NOT no_plugin_optional MATCHES "NOTFOUND" AND NOT no_plugin_optional)
            string(APPEND content "optional ")
        endif()

        get_target_property(qt_libinfix ${target} QT_QML_MODULE_LIBINFIX)
        string(APPEND content "plugin ${plugin_target}${qt_libinfix}\n")
        _qt_internal_qmldir_item(classname QT_QML_MODULE_CLASS_NAME)
    endif()

    get_target_property(designer_supported ${target} QT_QML_MODULE_DESIGNER_SUPPORTED)
    if(designer_supported)
        string(APPEND content "designersupported\n")
    endif()

    _qt_internal_qmldir_item(typeinfo QT_QMLTYPES_FILENAME)

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

# TODO: Need to consider the case where an executable's finalizer might execute
#       before our deferred call. That can occur in the following situations:
#
#         - The executable target is created in the same scope as the qml module
#           and the executable target is created first.
#         - The qml module is created in a parent scope of the executable.
#
#       Note that the qml module can safely be created in another scope as long
#       as that scope has been finalized by the time the executable target's
#       finalizer is called. A child scope satisfies this, as does any other
#       scope that has already finished being processed earlier in the CMake run.
function(_qt_internal_write_deferred_qmldir_file target)
    get_target_property(__qt_qmldir_content ${target} _qt_internal_qmldir_content)
    get_target_property(out_dir ${target} QT_QML_MODULE_OUTPUT_DIR)
    set(qmldir_file "${out_dir}/qmldir")
    configure_file(${__qt_qml_macros_module_base_dir}/Qt6qmldirTemplate.cmake.in ${qmldir_file} @ONLY)
endfunction()


# Create a Qml plugin. Projects should not normally need to call this function
# directly. Rather, it would normally be called by qt6_add_qml_module() to
# create or update the plugin associated with its backing target.
#
# target: The name of the target to use for the qml plugin. If it does not
#   already exist, it will be created. (REQUIRED)
#
# STATIC, SHARED: Explicitly specify the type of plugin library to create.
#   At most one of these two options can be specified. (OPTIONAL)
#
# BACKING_TARGET: The backing target that the plugin is associated with. This
#   can be the same as ${target}, in which case there is only the one merged
#   target. If this option is not provided, then URI must be given. (OPTIONAL)
#
# URI: Declares the module identifier of the qml module this plugin is
#   associated with. The module identifier is the (dotted URI notation)
#   identifier for the qml module. If URI is not given, then a BACKING_TARGET
#   must be provided and the backing target must have its URI recorded on it
#   (typically by an earlier call to qt6_add_qml_module()). (OPTIONAL)
#
# TARGET_PATH: Overwrite the generated target path. By default the target path
#   is generated from the URI by replacing the '.' with a '/'. However, under
#   certain circumstances this may not be enough. Use this argument to provide
#   a replacement. It is only used if targeting Android. (OPTIONAL)
#
# CLASS_NAME: By default, the class name will be taken from the backing target,
#   if provided, or falling back to the URI with "Plugin" appended. Any
#   non-alphanumeric characters in the URI will be replaced with underscores.
#   (OPTIONAL)
#
# OUTPUT_DIRECTORY: Specifies the directory where the plugin library will be
#   created. When not given, the output directory will be obtained from the
#   BACKING_TARGET if one has been provided. If an output directory cannot be
#   obtained from there either, the standard default as provided by CMake
#   (${CMAKE_CURRENT_BINARY_DIR}) will be used. (OPTIONAL)
#
# NO_GENERATE_PLUGIN_SOURCE: A .cpp file will be created for the plugin class
#   by default and automatically added to the plugin target. Use this option to
#   indicate that no such .cpp file should be generated. The caller is then
#   responsible for providing their own plugin class. (OPTIONAL)
#
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
        # default conversion from the URI is not applicable
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

    if(TARGET ${target})
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
        get_target_property(class_name ${target} QT_PLUGIN_CLASS_NAME)
        if(class_name)
            if(arg_CLASS_NAME AND NOT arg_CLASS_NAME STREQUAL class_name)
                message(FATAL_ERROR
                    "CLASS_NAME was specified, but an existing target with a "
                    "different class name was also given"
                )
            endif()
            set(arg_CLASS_NAME ${class_name})
        elseif(NOT arg_CLASS_NAME)
            _qt_internal_compute_qml_plugin_class_name_from_uri("${arg_URI}" arg_CLASS_NAME)
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

        if(NOT arg_CLASS_NAME)
            _qt_internal_compute_qml_plugin_class_name_from_uri("${arg_URI}" arg_CLASS_NAME)
        endif()

        qt6_add_plugin(${target} ${lib_type}
            TYPE qml_plugin
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

    if (ANDROID)
        # Adjust Qml plugin names on Android similar to qml_plugin.prf which calls
        # $$qt5LibraryTarget($$TARGET, "qml/$$TARGETPATH/").
        # Example plugin names:
        # qtdeclarative
        #   TARGET_PATH: QtQml/Models
        #   file name:   libqml_QtQml_Models_modelsplugin_arm64-v8a.so
        # qtquickcontrols2
        #   TARGET_PATH: QtQuick/Controls.2/Material
        #   file name:
        #     libqml_QtQuick_Controls.2_Material_qtquickcontrols2materialstyleplugin_arm64-v8a.so
        if(NOT arg_TARGET_PATH AND TARGET "${arg_BACKING_TARGET}")
            get_target_property(arg_TARGET_PATH ${arg_BACKING_TARGET} QT_QML_MODULE_TARGET_PATH)
        endif()
        if(arg_TARGET_PATH)
            string(REPLACE "/" "_" android_plugin_name_infix_name "${arg_TARGET_PATH}")
        else()
            string(REPLACE "." "_" android_plugin_name_infix_name "${arg_URI}")
        endif()

        set(final_android_qml_plugin_name "qml_${android_plugin_name_infix_name}_${target}")
        set_target_properties(${target}
            PROPERTIES
            LIBRARY_OUTPUT_NAME "${final_android_qml_plugin_name}"
        )
        qt6_android_apply_arch_suffix(${target})
    endif()

    if(NOT arg_OUTPUT_DIRECTORY AND arg_BACKING_TARGET AND TARGET ${arg_BACKING_TARGET})
        get_target_property(arg_OUTPUT_DIRECTORY ${arg_BACKING_TARGET} QT_QML_OUTPUT_DIRECTORY)
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
        set(qt_qml_plugin_intro "extern void ${register_types_function_name}();")
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
    if(NOT "${arg_BACKING_TARGET}" STREQUAL target)
        target_link_libraries(${target} PRIVATE ${arg_BACKING_TARGET})
    endif()

endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_qml_plugin)
        qt6_add_qml_plugin(${ARGV})
    endfunction()
endif()

# Add Qml files (.qml,.js,.mjs) to a Qml module.
#
# target: The backing target of the qml module. (REQUIRED)
#
# QML_FILES: The qml files to add to the backing target. Supported file extensions
#   are .qml, .js and .mjs. No other file types should be listed. (REQUIRED)
#
# RESOURCES: Resources used in QML, for example images. (OPTIONAL)
#
# PREFIX: The resource path under which to add the compiled qml files. If not
#   specified, the QT_QML_MODULE_RESOURCE_PREFIX property of the target is used
#   as the default, if set (that property is set by qt6_add_qml_module()).
#   If the default is empty, this option must be provided. (OPTIONAL)
#
# OUTPUT_TARGETS: In static builds, additional CMake targets can be created
#   which consumers of the module will need to potentially install.
#   Supply the name of an output variable, which will be set to a list of these
#   targets. If installing the main target, you will also need to install these
#   output targets for static builds. (OPTIONAL)
#
# NO_LINT: Do not add the specified files to the ${target}_qmllint target.
#   If this option is not given, the default will be taken from the target.
#
# NO_CACHEGEN: Do not compile the qml files. Add the raw qml files to the
#   target resources instead. If this option is not given, the default will be
#   taken from the target.
#
# NO_QMLDIR_TYPES: Do not append type information from the qml files to the
#   qmldir file associated with the qml module. If this option is not given,
#   the default will be taken from the target.
#
# In addition to the above NO_... options, individual files can be explicitly
# skipped by setting the relevant source property. These are:
#
#   - QT_QML_SKIP_QMLLINT
#   - QT_QML_SKIP_QMLDIR_ENTRY
#   - QT_QML_SKIP_CACHEGEN
#
# Disabling the qmldir entry for a qml file would normally only be used for a
# file that does not expose a public type (e.g. a private JS file).
# If appending of type information has not been disabled for a particular qml
# file, the following additional source properties can be specified to
# customize the file's type details:
#
# QT_QML_SOURCE_VERSION: Version(s) for this qml file. If not present the module
#   version will be used.
#
# QT_QML_SOURCE_TYPENAME: Override the file's type name. If not present, the
#   type name will be deduced using the file's basename.
#
# QT_QML_SINGLETON_TYPE: Set to true if this qml file contains a singleton type.
#
# QT_QML_INTERNAL_TYPE: When set to true, the type specified by
#   QT_QML_SOURCE_TYPENAME will not be available to users of this module.
#
#   e.g.:
#       set_source_files_properties(my_qml_file.qml
#           PROPERTIES
#               QT_QML_SOURCE_VERSION "2.0;6.0"
#               QT_QML_SOURCE_TYPENAME MyQmlFile
#
#       qt6_target_qml_sources(my_qml_module
#           QML_FILES
#               my_qml_file.qml
#       )
#
# The above will produce the following entry in the qmldir file:
#
#   MyQmlFile 2.0 my_qml_file.qml
#
function(qt6_target_qml_sources target)

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

    if (NOT arg_QML_FILES)
        if(arg_OUTPUT_TARGETS)
            set(${arg_OUTPUT_TARGETS} "" PARENT_SCOPE)
        endif()
        return()
    endif()

    if(arg___QT_INTERNAL_FORCE_DEFER_QMLDIR OR ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.19.0")
        set(can_defer_qmldir TRUE)
    else()
        set(can_defer_qmldir FALSE)
    endif()

    get_target_property(no_lint                ${target} QT_QML_MODULE_NO_LINT)
    get_target_property(no_cachegen            ${target} QT_QML_MODULE_NO_CACHEGEN)
    get_target_property(no_qmldir              ${target} QT_QML_MODULE_NO_GENERATE_QMLDIR)
    get_target_property(resource_prefix        ${target} QT_QML_MODULE_RESOURCE_PREFIX)
    get_target_property(qml_module_version     ${target} QT_QML_MODULE_VERSION)
    get_target_property(output_dir             ${target} QT_QML_MODULE_OUTPUT_DIR)

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
    if(NOT arg_PREFIX MATCHES [[/$]])
        string(APPEND arg_PREFIX "/")
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

    if(NOT no_cachegen)
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
        set(cachegen_args
            "$<${have_import_paths}:${import_paths}>"
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
                DEPENDS ${file_src}
                WORKING_DIRECTORY $<TARGET_PROPERTY:${target},SOURCE_DIR>
            )
        endif()
    endforeach()

    foreach(qml_file_src IN LISTS arg_QML_FILES)
        # This is to facilitate updating code that used the earlier tech preview
        # API function qt6_target_qml_files()
        if(NOT qml_file_src MATCHES "\\.(js|mjs|qml)$")
            list(APPEND non_qml_files ${qml_file_src})
            continue()
        endif()

        get_filename_component(file_absolute ${qml_file_src} ABSOLUTE)
        __qt_get_relative_resource_path_for_file(file_resource_path ${qml_file_src})
        set(qml_file_out ${output_dir}/${file_resource_path})

        # For the tooling steps below, run the tools on the copied qml file in
        # the build directory, not the source directory. This is required
        # because the tools may need to reference imported modules from
        # subdirectories, which would require those subdirectories to have
        # their generated qmldir files present. They also need to use the right
        # resource paths and the source locations might be structured quite
        # differently.

        # Fed to qmlimportscanner in qt6_import_qml_plugins. Also may be used in
        # generator expressions to install all qml files for the target.
        set_property(TARGET ${target} APPEND PROPERTY QT_QML_MODULE_FILES ${qml_file_out})

        # Add file to those processed by qmllint
        get_source_file_property(skip_qmllint ${qml_file_src} QT_QML_SKIP_QMLLINT)
        if(NOT no_lint AND NOT skip_qmllint)
            # The set of qml files to run qmllint on may be a subset of the
            # full set of files, so record these in a separate property.
            _qt_internal_target_enable_qmllint(${target})
            set_property(TARGET ${target} APPEND PROPERTY QT_QML_LINT_FILES ${qml_file_src})
        endif()

        # Add qml file's type to qmldir
        get_source_file_property(skip_qmldir ${qml_file_src} QT_QML_SKIP_QMLDIR_ENTRY)
        if(NOT no_qmldir AND NOT skip_qmldir)
            get_source_file_property(qml_file_typename ${qml_file_src} QT_QML_SOURCE_TYPENAME)
            if (NOT qml_file_typename)
                get_filename_component(qml_file_typename ${qml_file_src} NAME_WLE)
            endif()

            # Do not add qmldir entries for lowercase names. Those are not components.
            if (qml_file_typename MATCHES "^[A-Z]")
                if(NOT can_defer_qmldir)
                    message(FATAL_ERROR
                        "You are using CMake ${CMAKE_VERSION}, but CMake 3.19 or later "
                        "is required to add qml files with this function. Either pass "
                        "the qml files to qt6_add_qml_module() instead or update to "
                        "CMake 3.19 or later."
                    )
                endif()

                # TODO: rename to QT_QML_SOURCE_VERSIONS
                get_source_file_property(qml_file_versions  ${qml_file_src} QT_QML_SOURCE_VERSION)
                get_source_file_property(qml_file_singleton ${qml_file_src} QT_QML_SINGLETON_TYPE)
                get_source_file_property(qml_file_internal  ${qml_file_src} QT_QML_INTERNAL_TYPE)

                if (NOT qml_file_versions)
                    set(qml_file_versions ${qml_module_version})
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

            add_custom_command(
                OUTPUT ${compiled_file}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
                COMMAND
                    ${QT_TOOL_COMMAND_WRAPPER_PATH}
                    ${qmlcachegen}
                    --resource-path "${file_resource_path}"
                    ${cachegen_args}
                    -o "${compiled_file}"
                    "${file_absolute}"
                COMMAND_EXPAND_LISTS
                DEPENDS
                    ${qmlcachegen}
                    "${file_absolute}"
                    $<TARGET_PROPERTY:${target},_qt_generated_qrc_files>
                    "$<$<BOOL:${types_file}>:${types_file}>"
            )

            target_sources(${target} PRIVATE
                ${compiled_file}
                ${qml_file_src} # Make the original qml file show up under this target in the IDE
            )
            set_source_files_properties(${compiled_file} PROPERTIES
                SKIP_AUTOGEN ON
            )
        endif()
    endforeach()

    if(non_qml_files)
        list(JOIN non_qml_files "\n  " file_list)
        message(WARNING
            "Only .qml, .js or .mjs files should be added with QML_FILES. "
            "The following files should be added with RESOURCES instead:"
            "\n  ${file_list}"
        )
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

    if(arg_OUTPUT_TARGETS AND output_targets)
        set(${arg_OUTPUT_TARGETS} ${output_targets} PARENT_SCOPE)
    endif()

endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_target_qml_sources)
        qt6_target_qml_sources(${ARGV})
    endfunction()
endif()

# NOTE: This function does not normally need to be called directly by projects.
#       It is called automatically by qt6_add_qml_module() unless
#       NO_GENERATE_QMLTYPES is also given to that function.
#
# target: Expected to be the backing target for a qml module. Certain target
#   properties normally set by qt6_add_qml_module() will be retrieved from this
#   target. (REQUIRED)
#
# MANUAL_MOC_JSON_FILES: Specifies a list of json files, generated by a manual
#   moc call, to extract metatypes. (OPTIONAL)
#
function(qt6_qml_type_registration target)
    set(args_option "")
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
    get_target_property(qmltypes_output_name ${target} QT_QMLTYPES_FILENAME)
    if (NOT qmltypes_output_name)
        get_target_property(compile_definitions_list ${target} COMPILE_DEFINITIONS)
        list(FIND compile_definitions_list QT_PLUGIN is_a_plugin)
        if (is_a_plugin GREATER_EQUAL 0)
            set(qmltypes_output_name "plugins.qmltypes")
        else()
            set(qmltypes_output_name ${target}.qmltypes)
        endif()
    endif()

    # Horrible hack workaround to not install metatypes.json files for examples/ qml plugins into
    # ${qt_prefix}/lib/meta_types.
    # Put them into QT_QML_MODULE_INSTALL_DIR/lib/meta_types instead.
    get_target_property(qml_install_dir ${target} QT_QML_MODULE_INSTALL_DIR)
    set(meta_types_json_args "")

    # TODO: This is internal Qt logic, it should be moved out of here.
    if(QT_BUILDING_QT AND QT_WILL_INSTALL AND qml_install_dir AND
            qml_install_dir MATCHES "^${INSTALL_EXAMPLESDIR}")
        set(meta_types_json_args "INSTALL_DIR" "${qml_install_dir}/lib/metatypes")
    endif()

    if(arg_MANUAL_MOC_JSON_FILES)
        list(APPEND meta_types_json_args "MANUAL_MOC_JSON_FILES" ${arg_MANUAL_MOC_JSON_FILES})
    endif()
    qt6_extract_metatypes(${target} ${meta_types_json_args})

    get_target_property(import_version ${target} QT_QML_MODULE_VERSION)
    get_target_property(output_dir ${target} QT_QML_MODULE_OUTPUT_DIR)
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
    get_target_property(past_major_versions ${target} QT_QML_PAST_MAJOR_VERSIONS)

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

    target_include_directories(${target} PRIVATE
        $<TARGET_PROPERTY:${QT_CMAKE_EXPORT_NAMESPACE}::QmlPrivate,INTERFACE_INCLUDE_DIRECTORIES>
    )
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_qml_type_registration)
        qt6_qml_type_registration(${ARGV})
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
    set(oneValueArgs "PATH_TO_SCAN")
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
            "CLASSNAME;NAME;PATH;PLUGIN;RELATIVEPATH;TYPE;VERSION;" ""
            ${${entry_name}}
        )
    endmacro()

    # Run qmlimportscanner and include the generated cmake file.
    set(qml_imports_file_path
        "${CMAKE_CURRENT_BINARY_DIR}/.qt_plugins/Qt6_QmlPlugins_Imports_${target}.cmake"
    )
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/.qt_plugins)

    set(cmd_args
        "${arg_PATH_TO_SCAN}"
        -cmake-output
        -importPath "${qml_path}"
    )
    get_target_property(qml_import_path ${target} QT_QML_IMPORT_PATH)

    if (qml_import_path)
        list(APPEND cmd_args ${qml_import_path})
    endif()

    # Facilitate self-import so we can find the qmldir file
    list(APPEND cmd_args "${CMAKE_CURRENT_BINARY_DIR}")

    if(NOT "${QT_QML_OUTPUT_DIRECTORY}" STREQUAL "" AND EXISTS "${QT_QML_OUTPUT_DIRECTORY}")
        list(APPEND cmd_args "${QT_QML_OUTPUT_DIRECTORY}")
    endif()

    get_target_property(qml_files ${target} QT_QML_MODULE_FILES)
    if (qml_files)
        list(APPEND cmd_args "-qmlFiles" ${qml_files})
    endif()

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
                # Handles Qt provided Qml plugins
                if(TARGET "${QT_CMAKE_EXPORT_NAMESPACE}::${entry_PLUGIN}")
                    set(plugin_target "${QT_CMAKE_EXPORT_NAMESPACE}::${entry_PLUGIN}")
                    list(APPEND plugins_to_link "${plugin_target}")

                    __qt_internal_get_static_plugin_init_target_name("${entry_PLUGIN}"
                        plugin_init_target)
                    set(plugin_init_target_prefixed
                        "${QT_CMAKE_EXPORT_NAMESPACE}::${plugin_init_target}")
                    list(APPEND plugin_inits_to_link "${plugin_init_target_prefixed}")

                # Handles user project provided Qml plugins
                elseif(TARGET ${entry_PLUGIN} AND TARGET ${entry_PLUGIN}_init)
                    set(plugin_target "${entry_PLUGIN}")
                    list(APPEND plugins_to_link "${plugin_target}")

                    __qt_internal_get_static_plugin_init_target_name("${entry_PLUGIN}"
                        plugin_init_target)
                    list(APPEND plugin_inits_to_link "${plugin_init_target}")

                # TODO: QTBUG-94605 Figure out if this is a reasonable scenario to support
                else()
                    message(WARNING
                        "The qml plugin '${entry_PLUGIN}' is a dependency of '${target}', "
                        "but there is no target by that name in the current scope. The plugin will "
                        "not be linked."
                    )
                endif()
            endif()
        endforeach()

        if(plugins_to_link)
            # __qt_internal_propagate_object_library propagates object libraries by setting
            # INTERFACE linkage on ${target}. If ${target} is an executable, that would mean none
            # of the plugin init libraries would be linked to the executable itself. If ${target} is
            # a user shared library, it would be similar to the case above, the plugin init
            # libraries should be linked into the shared library, rather than to the consumer of the
            # shared library.
            # To achieve proper linkage in these cases, link the plugins and initializers directly.
            # For static libraries and INTERFACE libraries,
            # using __qt_internal_propagate_object_library is intended.
            get_target_property(target_type ${target} TYPE)
            if(target_type STREQUAL "EXECUTABLE" OR target_type STREQUAL "SHARED_LIBRARY")
                # This links both the initializer object and the usage requirements of the object
                # library, so Qt::Core.
                target_link_libraries("${target}" PRIVATE ${plugin_inits_to_link})
                target_link_libraries("${target}" PRIVATE ${plugins_to_link})
            else()
                foreach(plugin_init IN LISTS plugin_inits_to_link)
                    __qt_internal_propagate_object_library("${target}" "${plugin_init}")
                endforeach()
                target_link_libraries("${target}" INTERFACE ${plugins_to_link})
            endif()
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
