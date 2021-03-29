#
# Q6QmlMacros
#

set(__qt_qml_macros_module_base_dir "${CMAKE_CURRENT_LIST_DIR}")

#
# Create a Qml Module. Arguments:
#
# URI: Declares the module identifier of the module. The module identifier is
# the (dotted URI notation) identifier for the module, which must match the
# module's install path. (REQUIRED)
#
# VERSION: The module's version. (REQUIRED)
#
# TARGET_PATH: Overwrite the generated target path. By default the target path
#   is generated from the URI by replacing the '.' with a '/'. However, under
#   certain circumstance this may not be enough. Use this argument to provide
#   a replacement. (OPTIONAL)
#
# RESOURCE_PREFIX: Resource Prefix to be used when generating a static library.
#   When building a static library, the qmldir file is embedded into the library
#   using rcc. It is is also used by the Qt Quick Compiler to embed compiled
#   Qml files into a shared or static library. If none is supplied we will
#   generate the following prefix: /org.qt-project/imports/${target_path}.
#   (OPTIONAL)
#
# OUTPUT_DIRECTORY: If the module is not to be build under
#   ${CMAKE_CURRENT_BINARY_DIR}. This ensures the qmldir file is copied to the
#   right location.  (OPTIONAL)
#
# INSTALL_DIRECTORY: Intended installation directory for this module. (OPTIONAL)
#
# SOURCES: List of C++ sources. (OPTIONAL)
#
# DEPENDENCIES: List of QML Module dependencies and their versions. The module
#   and its version must be separated via a slash(/). E.g. QtQuick/2.0
#
# PAST_MAJOR_VERSIONS: List of past major versions this QML module was available
#   in. Ensures that the module can be imported when using these major versions.
#
# QML_FILES: List of Qml files. See qt6_target_qml_files for more information
#   on how to specify additional properties on qml files. (OPTIONAL)
#
# CLASSNAME: Provides the class name of the C++ plugin used by the module. This
#   information is required for all the QML modules that depend on a C++ plugin
#   for additional functionality. Qt Quick applications built with static
#   linking cannot resolve the module imports without this information.
#   (REQUIRED for static QML modules backed by C++ sources aka non-pure QML modules)
#
# DESIGNER_SUPPORTED: Specify this argument if the plugin is supported by Qt
#   Quick Designer. By default, the plugin will not be supported. (OPTIONAL)
#
# TYPEINFO: Path to a file which declares a type description file for the module
#   that can be read by QML tools such as Qt Creator to access information about
#   the types defined by the module's plugins. (OPTIONAL)
#
# IMPORTS: List of other Qml Modules that this module imports. A version can be
#   specified by appending it after a slash(/), e.g QtQuick/2.0. The minor
#   version may be omitted, e.g. QtQuick/2. Alternatively "auto" may be given
#   as version to forward the version the current module is being imported with,
#   e.g. QtQuick/auto. (OPTIONAL)
#
# OPTIONAL_IMPORTS: List of other Qml Modules that this module may import at
#   run-time. Those are not automatically imported by the QML engine when
#   importing the current module, but rather serve as hints to tools like
#   qmllint. Versions can be specified in the same as for IMPORT. (OPTIONAL)
#
# RESOURCE_EXPORT: In static builds, when Qml files are processed via the Qt
#   Quick Compiler generate a separate static library that will be linked in
#   as an Interface. Supply an output variable to perform any custom actions
#   on these extra generated targets.
#
# SKIP_TYPE_REGISTRATION: When present will cause the generated qmldir file
#   to not list any qml types. These are expected to be registered by the
#   c++ plugin code instead.
#
# PLUGIN_OPTIONAL: The plugin is marked as optional in the qmldir file. If the
#   type registration functions are already available by other means, typically
#   by linking a library proxied by the plugin, it won't be loaded.
#
# PURE_MODULE: The plugin does not take any C++ source files. A dummy class plugin cpp file will
#              be generated to ensure the module is found by the Qml engine.
#
# This function is currently in Technical Preview.
# It's signature and behavior might change.
function(qt6_add_qml_module target)
    set(args_optional
        GENERATE_QMLTYPES
        INSTALL_QMLTYPES
        DESIGNER_SUPPORTED
        SKIP_TYPE_REGISTRATION
        PLUGIN_OPTIONAL
        PURE_MODULE
    )

    if (QT_BUILDING_QT)
        list(APPEND args_optional DO_NOT_CREATE_TARGET)
    endif()

    set(args_single
        RESOURCE_PREFIX
        URI
        TARGET_PATH
        VERSION
        OUTPUT_DIRECTORY
        INSTALL_DIRECTORY
        CLASSNAME
        TYPEINFO
        RESOURCE_EXPORT
    )

    set(args_multi
       SOURCES
       QML_FILES
       IMPORTS
       OPTIONAL_IMPORTS
       DEPENDENCIES
       PAST_MAJOR_VERSIONS
    )

    cmake_parse_arguments(arg
       "${args_optional}"
       "${args_single}"
       "${args_multi}"
       ${ARGN}
    )

    if (NOT arg_URI)
        message(FATAL_ERROR "qt6_add_qml_module called without a module URI. Please specify one using the URI argument.")
    endif()

    if (NOT arg_VERSION)
        message(FATAL_ERROR "qt6_add_qml_module called without a module version. Please specify one using the VERSION argument.")
    endif()

    if (NOT "${arg_VERSION}" MATCHES "[0-9]+\\.[0-9]+")
        message(FATAL_ERROR "qt6_add_qml_module called with an invalid version argument: '${arg_VERSION}'. Expected version style: VersionMajor.VersionMinor.")
    endif()

    # If C++ sources were directly specified (not via qt_internal_add_qml_module), we assume the
    # user will provide a plugin.cpp file. Don't generate a dummy plugin.cpp file in this case.
    #
    # If no sources were specified or the plugin was marked as a pure QML module, generate a
    # dummy plugin.cpp file.
    if (arg_SOURCES OR NOT arg_PURE_MODULE)
        set(create_pure_qml_module_plugin FALSE)
    else()
        set(create_pure_qml_module_plugin TRUE)
    endif()

    if (arg_DO_NOT_CREATE_TARGET AND NOT TARGET "${target}")
        message(FATAL_ERROR "qt6_add_qml_module called with DO_NOT_CREATE_TARGET, but the given target '${target}' is not a cmake target")
    endif()

    if (arg_DO_NOT_CREATE_TARGET)
        get_target_property(target_type ${target} TYPE)
        if (target_type STREQUAL "STATIC_LIBRARY")
            set(is_static TRUE)
        elseif(target_type STREQUAL "MODULE_LIBRARY")
            set(is_static FALSE)
        else()
            message(FATAL_ERROR "qt6_add_qml_module called with DO_NOT_CREATE_TARGET, but target '${target}' is neither a static or a module library.")
        endif()
    else()
        # TODO: Creating a library here means we're missing creation of supporting .prl files,
        # as well as install(TARGET foo EXPORT bar) mapping,  as opposed to when it's done
        # by qt_internal_add_plugin inside the qt_internal_add_qml_module call.
        if(NOT BUILD_SHARED_LIBS)
            add_library(${target} STATIC)
            set(is_static TRUE)

            # No need to compile Q_IMPORT_PLUGIN-containing files for non-executables.
            _qt_internal_disable_static_default_plugins("${resource_target}")
        else()
            add_library(${target} MODULE)
            set(is_static FALSE)
            if(APPLE)
                # CMake defaults to using .so extensions for loadable modules, aka plugins,
                # but Qt plugins are actually suffixed with .dylib.
                set_property(TARGET "${target}" PROPERTY SUFFIX ".dylib")
            endif()
            if(WIN32)
                # CMake sets for Windows-GNU platforms the suffix "lib"
                set_property(TARGET "${target}" PROPERTY PREFIX "")
            endif()
        endif()
        _qt_internal_apply_strict_cpp("${target}")
    endif()

    if (NOT arg_TARGET_PATH)
        string(REPLACE "." "/" arg_TARGET_PATH ${arg_URI})
    endif()

    if(create_pure_qml_module_plugin)
        _qt_internal_create_dummy_qml_plugin("${target}" "${arg_URI}" arg_CLASSNAME)
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
        string(REPLACE "/" "_" android_plugin_name_infix_name "${arg_TARGET_PATH}")

        set(final_android_qml_plugin_name "qml_${android_plugin_name_infix_name}_${target}")
        set_target_properties(${target}
            PROPERTIES
            LIBRARY_OUTPUT_NAME "${final_android_qml_plugin_name}"
        )
    endif()

    if (NOT arg_RESOURCE_PREFIX)
        set(arg_RESOURCE_PREFIX "/org.qt-project/imports")
    endif()

    set(should_install TRUE)
    if (NOT arg_INSTALL_DIRECTORY)
        set(should_install FALSE)
    endif()

    set_target_properties(${target}
        PROPERTIES
            QT_QML_MODULE_TARGET_PATH "${arg_TARGET_PATH}"
            QT_QML_MODULE_URI "${arg_URI}"
            QT_RESOURCE_PREFIX "${arg_RESOURCE_PREFIX}/${arg_TARGET_PATH}"
            QT_QML_MODULE_VERSION "${arg_VERSION}"
            QT_QML_MODULE_INSTALL_DIR "${arg_INSTALL_DIRECTORY}"
            QT_QML_MODULE_RESOURCE_EXPORT "${arg_RESOURCE_EXPORT}"
    )
    if (arg_OUTPUT_DIRECTORY)
        set_target_properties(${target}
            PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
                ARCHIVE_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
                QT_QML_MODULE_OUTPUT_DIR "${arg_OUTPUT_DIRECTORY}"
        )
    endif()

    if (NOT DO_NOT_CREATE_TARGET AND should_install)
        install(TARGETS ${target}
            DESTINATION "${arg_INSTALL_DIRECTORY}"
        )
    endif()

    if (arg_OUTPUT_DIRECTORY)
        set(target_output_dir ${arg_OUTPUT_DIRECTORY})
    else()
        if(is_static)
            get_target_property(target_output_dir ${target} ARCHIVE_OUTPUT_DIRECTORY)
        else()
            get_target_property(target_output_dir ${target} LIBRARY_OUTPUT_DIRECTORY)
        endif()
    endif()

    if (arg_SKIP_TYPE_REGISTRATION)
        set_target_properties(${target} PROPERTIES QT_QML_MODULE_SKIP_TYPE_REGISTRATION TRUE)
    endif()

    if (arg_SOURCES)
        target_sources(${target} PRIVATE ${arg_SOURCES})
    endif()

    # Insert the plugins URI into its meta data to enable usage
    # of static plugins in QtDeclarative (like in mkspecs/features/qml_plugin.prf).
    set_property(TARGET "${target}" APPEND PROPERTY AUTOMOC_MOC_OPTIONS "-Muri=${arg_URI}")

    # Tracker so we can generate unique resource names for multiple
    # target_qml_files() calls.
    set_target_properties(${target} PROPERTIES QT6_QML_MODULE_ADD_QML_FILES_COUNT 1)

    # Generate qmldir file
    set(qmldir_file "${CMAKE_CURRENT_BINARY_DIR}/qmldir")
    set_target_properties(${target} PROPERTIES QT_QML_MODULE_QMLDIR_FILE ${qmldir_file})
    set(qmldir_file_contents "module ${arg_URI}\n")

    if (arg_PLUGIN_OPTIONAL)
       string(APPEND qmldir_file_contents "optional plugin ${target}${QT_LIBINFIX}\n")
    else()
        string(APPEND qmldir_file_contents "plugin ${target}${QT_LIBINFIX}\n")
    endif()

    if (arg_CLASSNAME)
        string(APPEND qmldir_file_contents "classname ${arg_CLASSNAME}\n")
    endif()
    if (arg_DESIGNER_SUPPORTED)
        string(APPEND qmldir_file_contents "designersupported\n")
    endif()
    if (arg_TYPEINFO)
        string(APPEND qmldir_file_contents "typeinfo ${arg_TYPEINFO}\n")
    else()
        # This always need to be written out since at the moment we have cases
        # where qmltyperegistrar is not run with the plugin but on a module
        # e.g: src/qml generates the qmltypes for src/imports/qtqml.
        # When this has been fixed/standardized we should move this to
        # qt6_qml_type_registration() so that it is written out when the
        # plugins.qmltypes is actually generated.
        string(APPEND qmldir_file_contents "typeinfo plugins.qmltypes\n")
    endif()

    macro(_add_imports imports import_string)
        foreach(import IN LISTS ${imports})
            string(FIND ${import} "/" slash_position REVERSE)
            if (slash_position EQUAL -1)
                string(APPEND qmldir_file_contents "${import_string} ${import}\n")
            else()
                string(SUBSTRING ${import} 0 ${slash_position} import_module)
                math(EXPR slash_position "${slash_position} + 1")
                string(SUBSTRING ${import} ${slash_position} -1 import_version)
                if (import_version MATCHES "[0-9]+\\.[0-9]+" OR import_version MATCHES "[0-9]+")
                    string(APPEND qmldir_file_contents "${import_string} ${import_module} ${import_version}\n")
                elseif (import_version MATCHES "auto")
                    string(APPEND qmldir_file_contents "${import_string} ${import_module} auto\n")
                else()
                    message(FATAL_ERROR "Invalid module ${import_string} version number. Expected 'VersionMajor', 'VersionMajor.VersionMinor' or 'auto'.")
                endif()
            endif()
        endforeach()
    endmacro()

    _add_imports(arg_IMPORTS "import")
    _add_imports(arg_OPTIONAL_IMPORTS "optional import")

    foreach(dependency IN LISTS arg_DEPENDENCIES)
        string(FIND ${dependency} "/" slash_position REVERSE)
        if (slash_position EQUAL -1)
            string(APPEND qmldir_file_contents "depends ${dependency}\n")
        else()
            string(SUBSTRING ${dependency} 0 ${slash_position} dep_module)
            math(EXPR slash_position "${slash_position} + 1")
            string(SUBSTRING ${dependency} ${slash_position} -1 dep_version)
            if (dep_version MATCHES "[0-9]+\\.[0-9]+" OR dep_version MATCHES "[0-9]+")
                string(APPEND qmldir_file_contents "depends ${dep_module} ${dep_version}\n")
            elseif (dep_version MATCHES "auto")
                string(APPEND qmldir_file_contents "depends ${dep_module} auto\n")
            else()
                message(FATAL_ERROR "Invalid module dependency version number. Expected 'VersionMajor', 'VersionMajor.VersionMinor' or 'auto'.")
            endif()
        endif()
    endforeach()

    _qt_internal_qmldir_defer_file(WRITE "${qmldir_file}" "${qmldir_file_contents}")

    # Process qml files
    if (arg_QML_FILES)
        qt6_target_qml_files(${target} FILES ${arg_QML_FILES})
    endif()

    # Embed qmldir in static builds
    if (is_static)
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
        string(REPLACE "/" "_" qmldir_resource_name ${arg_TARGET_PATH})
        string(PREPEND qmldir_resource_name "qmake_")

        set_source_files_properties("${qmldir_file}"
            PROPERTIES QT_RESOURCE_ALIAS "qmldir"
        )

        qt6_add_resources(${target} ${qmldir_resource_name}
            FILES "${qmldir_file}"
            OUTPUT_TARGETS resource_targets
        )

        if (resource_targets AND arg_RESOURCE_EXPORT)
            install(TARGETS ${resource_targets}
                EXPORT "${arg_RESOURCE_EXPORT}"
                DESTINATION "${arg_INSTALL_DIRECTORY}"
            )

            # When building a static Qt, we need to record information about the compiled resource
            # object files to embed them into .prl files.
            if(COMMAND qt_internal_record_rcc_object_files)
                qt_internal_record_rcc_object_files(
                    "${target}" "${resource_targets}" INSTALL_DIRECTORY "${arg_INSTALL_DIRECTORY}")
            endif()
        endif()
    endif()

    # Copy QMLDIR file to build directory. We want to do this even for static
    # builds so that tools and IDEs can read it.
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${qmldir_file}
            ${target_output_dir}/qmldir
        BYPRODUCTS
            ${target_output_dir}/qmldir
    )

    # Install QMLDIR file
    if (should_install)
        install(FILES ${qmldir_file}
            DESTINATION "${arg_INSTALL_DIRECTORY}"
        )
    endif()

    # Install and Copy plugin.qmltypes if exists
    set(target_plugin_qmltypes "${CMAKE_CURRENT_SOURCE_DIR}/plugins.qmltypes")

    # For an in-source build, ensure that the file is not the one that was generated by
    # qt6_qml_type_registration.
    get_target_property(target_binary_dir ${target} BINARY_DIR)
    set(generated_marker_file "${target_binary_dir}/.generated/plugins.qmltypes")

    if (EXISTS "${target_plugin_qmltypes}" AND NOT EXISTS "${generated_marker_file}")
        set_target_properties(${target}
            PROPERTIES QT_QML_MODULE_PLUGIN_TYPES_FILE "${target_plugin_qmltypes}"
        )

        _qt_internal_qmldir_defer_file(APPEND "${qmldir_file}" "typeinfo plugins.qmltypes\n")

        if (should_install)
            install(FILES "${target_plugin_qmltypes}"
                    DESTINATION "${arg_INSTALL_DIRECTORY}"
            )
        endif()

        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${target_plugin_qmltypes}
                ${target_output_dir}/plugins.qmltypes
            BYPRODUCTS
                ${target_output_dir}/plugins.qmltypes
        )
    endif()

    # Copy/Install type info file
    if (EXISTS ${arg_TYPEINFO})
        if (should_install)
            install(FILES "${arg_TYPEINFO}"
                    DESTINATION "${arg_INSTALL_DIRECTORY}"
            )
        endif()

        get_filename_component(filename ${arg_TYPEINFO} NAME)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${arg_TYPEINFO}
                ${target_output_dir}/${filename}
            BYPRODUCTS
                ${target_output_dir}/${filename}
        )
    endif()

    if (arg_INSTALL_QMLTYPES)
        set_target_properties(${target} PROPERTIES QT_QML_MODULE_INSTALL_QMLTYPES TRUE)
        if (arg_INSTALL_DIRECTORY)
            get_target_property(qml_module_install_dir ${target} QT_QML_MODULE_INSTALL_DIR)
            if (NOT qml_module_install_dir)
                set_target_properties(${target}
                    PROPERTIES QT_QML_MODULE_INSTALL_DIR "${arg_INSTALL_DIRECTORY}"
                )
            endif()
        endif()
    endif()

    if (arg_PAST_MAJOR_VERSIONS)
        set_target_properties(${target} PROPERTIES QT_QML_PAST_MAJOR_VERSIONS "${arg_PAST_MAJOR_VERSIONS}")
    endif()

    # Generate meta types data
    if (arg_GENERATE_QMLTYPES)
        qt6_qml_type_registration(${target})
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_qml_module)
        qt6_add_qml_module(${ARGV})
    endfunction()
endif()

# Creates a dummy Qml plugin class for pure Qml modules.
# Needed for both shared and static Qt builds, so that the Qml engine knows to load the plugin.
function(_qt_internal_create_dummy_qml_plugin target uri out_class_name)
    # Use the escaped URI name as the basis for the class name.
    string(REGEX REPLACE "[^A-Za-z0-9]" "_" escaped_uri "${uri}")

    set(qt_qml_plugin_class_name "${escaped_uri}Plugin")
    set(generated_cpp_file_name_base "Qt6_PureQmlModule_${target}_${qt_qml_plugin_class_name}")
    set(qt_qml_plugin_moc_include_name "${generated_cpp_file_name_base}.moc")

    set(register_types_function_name "qml_register_types_${escaped_uri}")
    set(qt_qml_plugin_intro "extern void ${register_types_function_name}();")

    if(QT_BUILDING_QT)
        string(APPEND qt_qml_plugin_intro "\n\nQT_BEGIN_NAMESPACE")
        set(qt_qml_plugin_outro "QT_END_NAMESPACE")
    endif()

    set(qt_qml_plugin_constructor_content
        "volatile auto registration = &${register_types_function_name};
        Q_UNUSED(registration);
")

    set(template_path "${__qt_qml_macros_module_base_dir}/Qt6QmlPluginTemplate.cpp.in")
    set(generated_cpp_file_name "${generated_cpp_file_name_base}.cpp")
    set(generated_cpp_file_path "${CMAKE_CURRENT_BINARY_DIR}/${generated_cpp_file_name}")

    configure_file("${template_path}" "${generated_cpp_file_path}" @ONLY)

    target_sources("${target}" PRIVATE "${generated_cpp_file_path}")
    target_link_libraries("${target}" PRIVATE ${QT_CMAKE_EXPORT_NAMESPACE}::Qml)

    set(${out_class_name} "${qt_qml_plugin_class_name}" PARENT_SCOPE)

    # Enable AUTOMOC explicitly, because the generated cpp file expects to include its moc-ed
    # output file.
    set_property(TARGET "${target}" PROPERTY AUTOMOC ON)
endfunction()

#
# Add Qml files (.qml,.js,.mjs) to a Qml module. This will also append the
# qml files to the qmldir file of the module. Two source file properties can
# be used to control the generated qmldir entry.
#
# QT_QML_SOURCE_VERSION: Version(s) for this qml file. If not present the module
#   version will be used.
# QT_QML_SOURCE_TYPENAME: Override the file's type name. If not present the
#   type name will be deduced using the file's basename.
# QT_QML_SINGLETON_TYPE: Set to true if this qml file contains a singleton
#   type.
# QT_QML_INTERNAL_TYPE: When set to true, the type specified by
#   QT_QML_SOURCE_TYPENAME will not be available to users of this module.
# QT_QML_SKIP_QMLDIR_ENTRY: When set to true, no qmldir entry will be created for
#   the source file. Useful if a file needs to be installed (like a private JS
#   file) but does not expose a public type.
#
#   e.g.:
#       set_source_files_properties(my_qml_file.qml
#           PROPERTIES
#               QT_QML_SOURCE_VERSION "2.0;6.0"
#               QT_QML_SOURCE_TYPENAME MyQmlFile
#
#       qt6_target_qml_files(my_qml_module
#           FILES
#               my_qml_file.qml
#       )
#
# Will produce the following entry in the qmldir file
#
#   MyQmlFile 2.0 my_qml_file.qml
#
#
# This function is currently in Technical Preview.
# It's signature and behavior might change.
function(qt6_target_qml_files target)

    cmake_parse_arguments(arg "" "" "FILES" ${ARGN})
    get_target_property(resource_count ${target} QT6_QML_MODULE_ADD_QML_FILES_COUNT)
    get_target_property(qmldir_file ${target} QT_QML_MODULE_QMLDIR_FILE)
    if (NOT qmldir_file)
        message(FATAL_ERROR "qt6_target_qml_file: ${target} is not a Qml module")
    endif()

    if (NOT arg_FILES)
        return()
    endif()
    math(EXPR new_count "${resource_count} + 1")
    set_target_properties(${target} PROPERTIES QT6_QML_MODULE_ADD_QML_FILES_COUNT ${new_count})

    qt6_add_resources(${target} "${target}_qml_files_${new_count}"
        FILES ${arg_FILES}
        OUTPUT_TARGETS resource_targets
    )
    get_target_property(skip_type_registration ${target} QT_QML_MODULE_SKIP_TYPE_REGISTRATION)
    get_target_property(target_resource_export ${target} QT_QML_MODULE_RESOURCE_EXPORT)
    get_target_property(qml_module_install_dir ${target} QT_QML_MODULE_INSTALL_DIR)
    get_target_property(qml_module_output_dir  ${target} QT_QML_MODULE_OUTPUT_DIR)

    if(NOT qml_module_install_dir)
        message(AUTHOR_WARNING
            "No QT_QML_MODULE_INSTALL_DIR property value provided for the '${target}' target. "
            "Installation of qml_files will most likely be broken.")
    endif()

    if (resource_targets)
        install(TARGETS ${resource_targets}
            EXPORT "${target_resource_export}"
            DESTINATION "${qml_module_install_dir}"
        )

        # When building a static Qt, we need to record information about the compiled resource
        # object files to embed them into .prl files.
        if(COMMAND qt_internal_record_rcc_object_files)
            qt_internal_record_rcc_object_files(
                "${target}" "${resource_targets}" INSTALL_DIRECTORY "${qml_module_install_dir}")
        endif()
    endif()

    qt6_target_enable_qmllint(${target})

    set(file_contents "")
    foreach(qml_file IN LISTS arg_FILES)
        get_filename_component(qml_file_dir "${qml_file}" DIRECTORY)
        if (NOT "${qml_file_dir}" STREQUAL "")
            set(qml_file_dir "/${qml_file_dir}")
        endif()
        if (qml_module_output_dir)
            file(COPY "${qml_file}" DESTINATION "${qml_module_output_dir}${qml_file_dir}")
        endif()
        if (qml_module_install_dir)
            install(FILES "${qml_file}" DESTINATION "${qml_module_install_dir}${qml_file_dir}")
        endif()

        if (skip_type_registration AND qml_file MATCHES "\\.qml$")
            continue()
        endif()

        get_source_file_property(qml_file_skip_qmldir ${qml_file} QT_QML_SKIP_QMLDIR_ENTRY)
        if (qml_file_skip_qmldir)
            continue()
        endif()

        # TODO: rename to QT_QML_SOURCE_VERSIONS
        get_source_file_property(qml_file_versions ${qml_file} QT_QML_SOURCE_VERSION)
        get_source_file_property(qml_file_typename ${qml_file} QT_QML_SOURCE_TYPENAME)
        get_source_file_property(qml_file_singleton ${qml_file} QT_QML_SINGLETON_TYPE)
        get_source_file_property(qml_file_internal ${qml_file} QT_QML_INTERNAL_TYPE)
        get_target_property(qml_module_version ${target} QT_QML_MODULE_VERSION)

        if (NOT qml_file_versions)
            set(qml_file_versions ${qml_module_version})
        endif()

        if (NOT qml_file_typename)
            get_filename_component(qml_file_typename ${qml_file} NAME_WLE)
        endif()

        if (qml_file_singleton)
            string(APPEND file_contents "singleton ")
        endif()

        foreach(qml_file_version IN LISTS qml_file_versions)
            string(APPEND file_contents "${qml_file_typename} ${qml_file_version} ${qml_file}\n")
        endforeach()

        if (qml_file_internal)
            string(APPEND file_contents "internal ${qml_file_typename} ${qml_file}\n")
        endif()

    endforeach()
    _qt_internal_qmldir_defer_file(APPEND "${qmldir_file}" "${file_contents}")
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_target_qml_files)
        qt6_target_qml_files(${ARGV})
    endfunction()
endif()

# QT_QMLTYPES_FILENAME: If the target has the target property QT_QMLTPYES_FILENAME set, it will be
# used for the name of the generated file. Otherwise, the file will be named plugins.qmltypes if the
# target is a plugin, or ${target}.qmltypes in all other cases
# This function is currently in Technical Preview.
# It's signature and behavior might change.
# MANUAL_MOC_JSON_FILES specifies a list of json files, generated by manual moc call,
# to extract metatypes.
function(qt6_qml_type_registration target)
    cmake_parse_arguments(arg "" "" "MANUAL_MOC_JSON_FILES" ${ARGN})
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

    if(QT_BUILDING_QT AND QT_WILL_INSTALL AND qml_install_dir AND
            qml_install_dir MATCHES "^${INSTALL_EXAMPLESDIR}")
        set(meta_types_json_args "INSTALL_DIR" "${qml_install_dir}/lib/metatypes")
    endif()

    if(arg_MANUAL_MOC_JSON_FILES)
        list(APPEND meta_types_json_args "MANUAL_MOC_JSON_FILES" ${arg_MANUAL_MOC_JSON_FILES})
    endif()
    qt6_extract_metatypes(${target} ${meta_types_json_args})

    get_target_property(import_version ${target} QT_QML_MODULE_VERSION)
    get_target_property(target_source_dir ${target} SOURCE_DIR)
    get_target_property(target_binary_dir ${target} BINARY_DIR)
    get_target_property(target_metatypes_file ${target} INTERFACE_QT_META_TYPES_BUILD_FILE)
    if (NOT target_metatypes_file)
        message(FATAL_ERROR "Target ${target} does not have a meta types file")
    endif()

    # Extract major and minor version
    if (NOT import_version MATCHES "[0-9]+\\.[0-9]+")
        message(FATAL_ERROR "Invalid module version number. Expected VersionMajor.VersionMinor.")
    endif()
    #string(FIND "${import_version}" "." dot_location)
    #string(SUBSTRING ${import_version} 0 ${dot_location} major_version)
    #math(EXPR dot_location "${dot_location}+1")
    #string(SUBSTRING ${import_version} ${dot_location} -1 minor_version)
    string(REPLACE "." ";" import_version_split "${import_version}")
    list(LENGTH import_version_split import_version_split_length)
    if(import_version_split_length GREATER 0)
        list(GET import_version_split 0 major_version)
    endif()
    if(import_version_split_length GREATER 1)
        list(GET import_version_split 1 minor_version)
    endif()

    # check if plugins.qmltypes is already defined
    get_target_property(target_plugin_qmltypes ${target} QT_QML_MODULE_PLUGIN_TYPES_FILE)
    if (target_plugin_qmltypes)
        message(FATAL_ERROR "Target ${target} already has a qmltypes file set.")
    endif()

    set(cmd_args)
    set(plugin_types_file "${target_binary_dir}/${qmltypes_output_name}")
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
    if (CMAKE_GENERATOR MATCHES "Ninja")  # TODO: Makefiles supported too since CMake 3.20
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

    set(extra_env_command)
    if (WIN32)
        # TODO: FIXME: The env path is wrong when not building Qt, but a standalone example.
        file(TO_NATIVE_PATH "${${PROJECT_NAME}_BINARY_DIR}/bin$<SEMICOLON>${CMAKE_INSTALL_PREFIX}/${INSTALL_BINDIR}$<SEMICOLON>%PATH%" env_path_native)
        set(extra_env_command COMMAND set \"PATH=${env_path_native}\")
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
        ${extra_env_command}
        COMMAND
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

    target_sources(${target} PRIVATE ${type_registration_cpp_file})

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

    # Usually for Qt Qml-like modules and qml plugins, the installation destination of the .qmltypes
    # file is somewhere under the ${qt_prefix}/qml (Qt qml import path).
    #
    # For user-written qml plugins, the file should be installed next to the
    # binary / library, and not the Qt qml import path.
    #
    # Unfortunately CMake doesn't provide a way to query where a binary will be installed, so the
    # only way to know where to install is to request the installation path via a property.
    #
    # Thus only install the qmltypes file if an explicit path via the QT_QML_MODULE_INSTALL_DIR
    # property has been provided. Otherwise if installation is requested, and no path is provided,
    # warn the user, and don't install the file.
    get_target_property(install_qmltypes ${target} QT_QML_MODULE_INSTALL_QMLTYPES)
    if (install_qmltypes)
        if(qml_install_dir)
            if(NOT DEFINED QT_WILL_INSTALL OR QT_WILL_INSTALL)
                install(FILES ${plugin_types_file} DESTINATION "${qml_install_dir}")
            else()
                # Need to make the path absolute during a Qt non-prefix build, otherwise files are
                # written to the source dir because the paths are relative to the source dir.
                if(NOT IS_ABSOLUTE "${qml_install_dir}")
                    set(qml_install_dir "${CMAKE_INSTALL_PREFIX}/${qml_install_dir}")
                endif()

                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${plugin_types_file}"
                        "${qml_install_dir}/${qmltypes_output_name}"
                    BYPRODUCTS
                        "${qml_install_dir}/${qmltypes_output_name}"
                    COMMENT "Copying ${plugin_types_file} to ${qml_install_dir}"
                )
            endif()
        else()
            message(AUTHOR_WARNING
                "No QT_QML_MODULE_INSTALL_DIR property value provided for the '${target}' target. "
                "Please either provide a value, or don't set the "
                "QT_QML_MODULE_INSTALL_QMLTYPES property. "
                "Skipping installation of '${qmltypes_output_name}'.")
        endif()
    endif()

    target_include_directories(${target} PRIVATE
        $<TARGET_PROPERTY:Qt::QmlPrivate,INTERFACE_INCLUDE_DIRECTORIES>
    )
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_qml_type_registration)
        qt6_qml_type_registration(${ARGV})
    endfunction()
endif()


# Enable the _qt_internal_quick_compiler_process_resources function in qt6_add_resource()
set(QT6_ADD_RESOURCE_DECLARATIVE_EXTENSIONS TRUE)

# Inspect all files passed to a call to qt_add_resource. If there are any
# files present, invoke the quick compiler and return the remaining resource
# files that have not been processed in OUTPUT_REMAINING_RESOURCES as well as the new
# name for the resource in OUTPUT_RESOURCE_NAME.
function(_qt_internal_quick_compiler_process_resources target resource_name)

    cmake_parse_arguments(arg
        "" "PREFIX;OUTPUT_REMAINING_RESOURCES;OUTPUT_RESOURCE_NAME;OUTPUT_GENERATED_TARGET" "FILES" ${ARGN}
    )

    set(qml_files)
    set(resource_files)
    # scan for qml files
    foreach(file IN LISTS arg_FILES)
        # check whether this resource should not be processed by the qt quick
        # compiler
        get_source_file_property(skip_compiler_check ${file} QT_SKIP_QUICKCOMPILER)
        if (skip_compiler_check)
            list(APPEND resource_files ${file})
            continue()
        endif()

        if (${file} MATCHES "\.js$"
                OR ${file} MATCHES "\.mjs$"
                OR ${file} MATCHES "\.qml$")
            list(APPEND qml_files ${file})
        endif()
        list(APPEND resource_files ${file})
    endforeach()

    # Create a list of QML files for use with qmllint
    if(qml_files)
        get_target_property(qml_files_list ${target} QML_FILES)
        if(NOT qml_files_list)
            set(qml_files_list)
        endif()

        list(APPEND qml_files_list ${qml_files})
        set_target_properties(${target} PROPERTIES QML_FILES "${qml_files_list}")
    endif()

    if (NOT TARGET ${QT_CMAKE_EXPORT_NAMESPACE}::qmlcachegen AND qml_files)
        message(WARNING "QT6_PROCESS_RESOURCE: Qml files were detected but the qmlcachgen target is not defined. Consider adding QmlTools to your find_package command.")
    endif()

    if (TARGET ${QT_CMAKE_EXPORT_NAMESPACE}::qmlcachegen AND qml_files)
        # Enable qt quick compiler support
        set(qml_resource_file "${CMAKE_CURRENT_BINARY_DIR}/.rcc/${resource_name}.qrc")
        if (resource_files)
            set(chained_resource_name "${resource_name}_qmlcache")
        endif()

        get_target_property(qmltypes ${target} QT_QML_MODULE_PLUGIN_TYPES_FILE)
        if (qmltypes)
            list(APPEND qmlcachegen_extra_args "-i" ${qmltypes})
        endif()

        get_target_property(direct_calls ${target} QT_QMLCACHEGEN_DIRECT_CALLS)
        if (direct_calls)
            list(APPEND qmlcachegen_extra_args "--direct-calls")
        endif()

        get_target_property(qmljs_runtime ${target} QT_QMLCACHEGEN_QMLJS_RUNTIME)
        if (qmljs_runtime)
            list(APPEND qmlcachegen_extra_args "--qmljs-runtime")
        endif()

        foreach(file IN LISTS qml_files)
            get_filename_component(file_absolute ${file} ABSOLUTE)
            string(FIND "${file_absolute}" "${CMAKE_SOURCE_DIR}" start_index_of_source_dir)
            if (start_index_of_source_dir EQUAL 0)
                file(RELATIVE_PATH file_relative ${CMAKE_CURRENT_SOURCE_DIR} ${file_absolute})
            else()
                file(RELATIVE_PATH file_relative ${CMAKE_CURRENT_BINARY_DIR} ${file_absolute})
            endif()
            __qt_get_relative_resource_path_for_file(file_resource_path ${file})
            if (arg_PREFIX STREQUAL "/")
                # TO_CMAKE_PATH does not clean up cases such as //Foo
                set(file_resource_path "/${file_resource_path}")
            else()
                set(file_resource_path "${arg_PREFIX}/${file_resource_path}")
            endif()
            file(TO_CMAKE_PATH ${file_resource_path} file_resource_path)
            list(APPEND file_resource_paths ${file_resource_path})
            string(REGEX REPLACE "\.js$" "_js" compiled_file ${file_relative})
            string(REGEX REPLACE "\.mjs$" "_mjs" compiled_file ${compiled_file})
            string(REGEX REPLACE "\.qml$" "_qml" compiled_file ${compiled_file})
            string(REGEX REPLACE "[\$#\?]+" "_" compiled_file ${compiled_file})
            set(compiled_file "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache/${resource_name}/${compiled_file}.cpp")
            get_filename_component(out_dir ${compiled_file} DIRECTORY)
            if(NOT EXISTS ${out_dir})
                file(MAKE_DIRECTORY ${out_dir})
            endif()
            add_custom_command(
                OUTPUT ${compiled_file}
                ${QT_TOOL_PATH_SETUP_COMMAND}
                COMMAND
                    ${QT_CMAKE_EXPORT_NAMESPACE}::qmlcachegen
                    --resource-path "${file_resource_path}"
                    ${qmlcachegen_extra_args}
                    -o "${compiled_file}"
                    "${file_absolute}"
                DEPENDS
                    $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmlcachegen>
                    "${file_absolute}"
                    "$<$<BOOL:${qmltypes}>:${qmltypes}>"
            )
            target_sources(${target} PRIVATE ${compiled_file})
            set_source_files_properties(${compiled_file} PROPERTIES
                SKIP_AUTOGEN ON
            )
        endforeach()

        set(qmlcache_loader_list "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache/${resource_name}/qml_loader_file_list.rsp")
        file(GENERATE
            OUTPUT ${qmlcache_loader_list}
            CONTENT "$<JOIN:${file_resource_paths},\n>"
        )

        set(qmlcache_loader_file "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache/${resource_name}/qmlcache_loader.cpp")
        set(resource_name_arg "${resource_name}.qrc")
        if (chained_resource_name)
            set(resource_name_arg "${resource_name_arg}=${chained_resource_name}")
        endif()

        add_custom_command(
            OUTPUT ${qmlcache_loader_file}
            ${QT_TOOL_PATH_SETUP_COMMAND}
            COMMAND
                ${QT_CMAKE_EXPORT_NAMESPACE}::qmlcachegen
                --resource-name "${resource_name_arg}"
                -o "${qmlcache_loader_file}"
                "@${qmlcache_loader_list}"
            DEPENDS
                $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::qmlcachegen>
                "${qmlcache_loader_list}"
        )

        __qt_propagate_generated_resource(${target}
            ${resource_name}
            ${qmlcache_loader_file}
            output_target)

        set(${arg_OUTPUT_GENERATED_TARGET} "${output_target}" PARENT_SCOPE)

        if (resource_files)
            set(resource_name ${chained_resource_name})
        endif()

        # The generated qmlcache_loader source file uses private headers of Qml, so make sure
        # if the object library was created, it depends on the Qml target. If there's no target,
        # that means the target is a shared library and the sources are directly added to the target
        # via target_sources, so add dependency in that case as well.
        set(chosen_target "target") # shared library case
        if(output_target)
            set(chosen_target "output_target") # static library case.
        endif()
        target_link_libraries(${${chosen_target}} PRIVATE ${QT_CMAKE_EXPORT_NAMESPACE}::Qml)
    else()
        set(resource_files ${arg_FILES})
    endif()

    set(${arg_OUTPUT_REMAINING_RESOURCES} ${resource_files} PARENT_SCOPE)
    set(${arg_OUTPUT_RESOURCE_NAME} ${resource_name} PARENT_SCOPE)
endfunction()

include(CMakeParseArguments)

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
    # TODO: qt.prf implies that there might be more than one qml import path to pass to
    # qmlimportscanner.
    set(qml_path "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML}")

    # Small macro to avoid duplicating code in two different loops.
    macro(_qt6_QmlImportScanner_parse_entry)
        set(entry_name "qml_import_scanner_import_${idx}")
        cmake_parse_arguments("entry"
                              ""
                              "CLASSNAME;NAME;PATH;PLUGIN;RELATIVEPATH;TYPE;VERSION;" ""
                              ${${entry_name}})
    endmacro()

    # Run qmlimportscanner and include the generated cmake file.
    set(qml_imports_file_path
        "${CMAKE_CURRENT_BINARY_DIR}/Qt6_QmlPlugins_Imports_${target}.cmake")

    get_target_property(qrc_files ${target} _qt_generated_qrc_files)
    if (qrc_files)
        list(APPEND qrcFilesArguments "-qrcFiles")
        list(APPEND qrcFilesArguments ${qrc_files})
    endif()

    message(STATUS "Running qmlimportscanner to find used QML plugins. ")
    execute_process(COMMAND
                    "${tool_path}" "${arg_PATH_TO_SCAN}" -importPath "${qml_path}"
                    ${qrcFilesArguments}
                    -cmake-output
                    OUTPUT_FILE "${qml_imports_file_path}")

    include("${qml_imports_file_path}" OPTIONAL RESULT_VARIABLE qml_imports_file_path_found)
    if(NOT qml_imports_file_path_found)
        message(FATAL_ERROR "Could not find ${qml_imports_file_path} which was supposed to be generated by qmlimportscanner.")
    endif()

    # Parse the generated cmake file.
    # It is possible for the scanner to find no usage of QML, in which case the import count is 0.
    if(qml_import_scanner_imports_count)
        set(added_plugins "")
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

                # Link against the Qml plugin. The assumption is that all Qml plugins are already
                # find_package()'d by the Qml package, so we can safely link against the target.
                target_link_libraries("${target}" PRIVATE
                                      "${QT_CMAKE_EXPORT_NAMESPACE}::${entry_PLUGIN}")
            endif()
        endforeach()

        # Generate content for plugin initialization cpp file.
        set(added_imports "")
        set(qt_qml_import_cpp_file_content "")
        foreach(idx RANGE "${qml_import_scanner_imports_count}")
            _qt6_QmlImportScanner_parse_entry()
            if(entry_PLUGIN)
                if(entry_CLASSNAME)
                    list(FIND added_imports "${entry_PLUGIN}" _index)
                    if(_index EQUAL -1)
                        string(APPEND qt_qml_import_cpp_file_content
                               "Q_IMPORT_PLUGIN(${entry_CLASSNAME})\n")
                        list(APPEND added_imports "${entry_PLUGIN}")
                    endif()
                else()
                    message(FATAL_ERROR
                            "Plugin ${entry_PLUGIN} is missing a classname entry, please add one to the qmldir file.")
                endif()
            endif()
        endforeach()

        # Write to the generated file, and include it as a source for the given target.
        set(generated_import_cpp_path
            "${CMAKE_CURRENT_BINARY_DIR}/Qt6_QmlPlugins_Imports_${target}.cpp")
        configure_file("${Qt6Qml_DIR}/Qt6QmlImportScannerTemplate.cpp.in"
                       "${generated_import_cpp_path}"
                       @ONLY)
        target_sources(${target} PRIVATE "${generated_import_cpp_path}")
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

# Wrapper around configure_file. Passes content collected in $directory-scoped property.
function(_qt_internal_configure_qmldir directory)
    get_property(__qt_qmldir_content DIRECTORY "${directory}" PROPERTY _qt_internal_qmldir_content)
    configure_file(${ARGN} @ONLY)
endfunction()

# Collects content for target $filepath and use deferred call of 'configure_file' to avoid
# rebuilding of targets that depend on provided qmldir.
# Note: For cmake versions < 3.19.0 plain 'file' function call will be used.
function(_qt_internal_qmldir_defer_file command filepath content)
    if(${CMAKE_VERSION} VERSION_LESS "3.19.0")
        file(${ARGV})
    else()
        if("${command}" STREQUAL "WRITE")
            if("${__qt_qml_macros_module_base_dir}" STREQUAL "")
                message(FATAL_ERROR "Unable to configure qml module.
    \"find_package(Qt\${QT_VERSION_MAJOR} CONFIG COMPONENTS Qml)\" \
is missing.")
            endif()
            # Wrap with EVAL CODE to evaluate and expand arguments
            cmake_language(EVAL CODE
                           "cmake_language(DEFER DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\" CALL
                            \"_qt_internal_configure_qmldir\"
                            \"${CMAKE_CURRENT_SOURCE_DIR}\"
                            \"${__qt_qml_macros_module_base_dir}/Qt6qmldirTemplate.cmake.in\"
                            \"${filepath}\")")
            set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         PROPERTY _qt_internal_qmldir_content "${content}")
        elseif("${command}" STREQUAL "APPEND")
            set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         APPEND_STRING
                         PROPERTY _qt_internal_qmldir_content "${content}")
        else()
            message(FATAL_ERROR "Unknown command ${command}. \
                                 _qt_internal_qmldir_defer_file only accepts \
                                 WRITE and APPEND commands.")
        endif()
    endif()
endfunction()

# Adds a target called TARGET_qmllint that runs on all qml files compiled ahead-of-time.
function(qt6_target_enable_qmllint target)
    get_target_property(target_source ${target} SOURCE_DIR)
    get_target_property(includes ${target} QML2_IMPORT_PATH)
    get_target_property(files ${target} QML_FILES)

    if(includes)
        foreach(dir in LISTS includes)
            list(APPEND include_args "-I${dir}")
        endforeach()
    endif()

    add_custom_target(${target}_qmllint
        ${QT_CMAKE_EXPORT_NAMESPACE}::qmllint ${files} ${include_args}
        WORKING_DIRECTORY ${target_source}
    )
endfunction()
