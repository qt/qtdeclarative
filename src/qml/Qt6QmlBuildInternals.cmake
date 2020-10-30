#
# QtDeclarative Specific extensions
#

include_guard(GLOBAL)

# This function creates a CMake target for qml modules. It will also make
# sure that if no C++ source are present, that qml files show up in the project
# in an IDE. Finally, it will also create a custom ${target}_qmltypes which
# can be used to generate the respective plugins.qmltypes file.
#
#  URI: Module's uri.
#  TARGET_PATH: Expected installation path for the Qml Module. Equivalent
#  to the module's URI where '.' is replaced with '/'. Use this to override the
#  default substitution pattern.
#  VERSION: Version of the qml module
#  SKIP_TYPE_REGISTRATION: All qml files are expected to be registered by the
#  c++ plugin code.
#  PLUGIN_OPTIONAL: Any plugins are optional
#
function(qt_internal_add_qml_module target)

    set(qml_module_optional_args
        GENERATE_QMLTYPES
        INSTALL_QMLTYPES
        DESIGNER_SUPPORTED
        DO_NOT_INSTALL
        SKIP_TYPE_REGISTRATION
        PLUGIN_OPTIONAL
    )

    set(qml_module_single_args
        URI
        TARGET_PATH
        VERSION
        CLASSNAME
    )

    set(qml_module_multi_args
        IMPORTS
        OPTIONAL_IMPORTS
        TYPEINFO
        DEPENDENCIES
        PAST_MAJOR_VERSIONS
    )

    qt_parse_all_arguments(arg "qt_add_qml_module"
        "${__qt_add_plugin_optional_args};${qml_module_optional_args}"
        "${__qt_add_plugin_single_args};${qml_module_single_args}"
        "${__qt_add_plugin_multi_args};${qml_module_multi_args}" ${ARGN})

    if (NOT arg_URI)
        message(FATAL_ERROR "qt_add_qml_module called without specifying the module's uri. Please specify one using the URI parameter.")
    endif()

    set(target_path ${arg_TARGET_PATH})

    if (NOT arg_VERSION)
        message(FATAL_ERROR "qt_add_qml_module called without specifying the module's import version. Please specify one using the VERSION parameter.")
    endif()

    if (NOT arg_TARGET_PATH)
        string(REPLACE "." "/" arg_TARGET_PATH ${arg_URI})
    endif()

    qt_remove_args(plugin_args
        ARGS_TO_REMOVE
            ${target}
            ${qml_module_multi_args}
            ${qml_module_single_args}
        ALL_ARGS
            ${__qt_add_plugin_optional_args}
            ${__qt_add_plugin_single_args}
            ${qml_module_single_args}
            ${__qt_add_plugin_multi_args}
            ${qml_module_multi_args}
        ARGS
            ${ARGV}
    )

    qt_internal_add_plugin(${target}
        TYPE
            qml_plugin
        QML_TARGET_PATH
            "${arg_TARGET_PATH}"
        ${plugin_args}
    )

    set(no_create_option DO_NOT_CREATE_TARGET)

    if (arg_CLASSNAME)
        set(classname_arg CLASSNAME ${arg_CLASSNAME})
    endif()

    if (arg_DESIGNER_SUPPORTED)
        set(designer_supported_arg DESIGNER_SUPPORTED)
    endif()

    if (arg_SKIP_TYPE_REGISTRATION)
        set(skip_registration_arg SKIP_TYPE_REGISTRATION)
    endif()

    if (arg_PLUGIN_OPTIONAL)
        set(plugin_optional_arg PLUGIN_OPTIONAL)
    endif()

    if (arg_GENERATE_QMLTYPES)
        set(generate_qmltypes_arg GENERATE_QMLTYPES)
    endif()

    if (arg_INSTALL_QMLTYPES)
        set(install_qmltypes_arg INSTALL_QMLTYPES)
    endif()


    # Because qt_internal_add_qml_module does not propagate its SOURCES option to
    # qt6_add_qml_module, but only to qt_internal_add_plugin, we need a way to tell
    # qt6_add_qml_module if it should generate a dummy plugin cpp file. Otherwise we'd generate
    # a dummy plugin.cpp file twice and thus cause duplicate symbol issues.
    if (NOT arg_SOURCES)
        set(pure_qml_module "PURE_MODULE")
    endif()

    qt_path_join(qml_module_install_dir ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/${arg_TARGET_PATH}")

    set(qml_module_build_dir "")
    if(NOT QT_WILL_INSTALL)
        qt_path_join(qml_module_build_dir ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/${arg_TARGET_PATH}")
        set(qml_module_build_dir OUTPUT_DIRECTORY "${qml_module_build_dir}")
    endif()

    if (arg_SOURCES AND NOT arg_TYPEINFO)
        set(arg_TYPEINFO "plugins.qmltypes")
    endif()

    qt6_add_qml_module(${target}
        ${designer_supported_arg}
        ${no_create_option}
        ${skip_registration_arg}
        ${plugin_optional_arg}
        ${classname_arg}
        ${generate_qmltypes_arg}
        ${install_qmltypes_arg}
        ${pure_qml_module}
        RESOURCE_PREFIX "/qt-project.org/imports"
        TARGET_PATH ${arg_TARGET_PATH}
        URI ${arg_URI}
        VERSION ${arg_VERSION}
        PAST_MAJOR_VERSIONS ${arg_PAST_MAJOR_VERSIONS}
        QML_FILES ${arg_QML_FILES}
        IMPORTS "${arg_IMPORTS}"
        OPTIONAL_IMPORTS "${arg_OPTIONAL_IMPORTS}"
        TYPEINFO "${arg_TYPEINFO}"
        DO_NOT_INSTALL_METADATA
        INSTALL_QML_FILES
        ${qml_module_build_dir}
        INSTALL_LOCATION "${qml_module_install_dir}"
        DEPENDENCIES ${arg_DEPENDENCIES}
        RESOURCE_EXPORT "${INSTALL_CMAKE_NAMESPACE}${target}Targets"
    )

    get_target_property(qmldir_file ${target} QT_QML_MODULE_QMLDIR_FILE)
    get_target_property(plugin_types ${target} QT_QML_MODULE_PLUGIN_TYPES_FILE)
    set(files_to_install)
    if (EXISTS ${plugin_types})
        list(APPEND files_to_install ${plugin_types})
        qt_copy_or_install(FILES ${plugin_types}
            DESTINATION "${qml_module_install_dir}"
        )

        if(QT_WILL_INSTALL)
            # plugin.qmltypes when present should also be copied to the
            # cmake binary dir when doing prefix builds
            file(COPY ${plugin_types}
                DESTINATION "${QT_BUILD_DIR}/${INSTALL_QMLDIR}/${arg_TARGET_PATH}"
            )
        endif()
    endif()

    list(APPEND files_to_install ${qmldir_file})
    if (QT_WILL_INSTALL)
        install(FILES ${files_to_install} DESTINATION ${qml_module_install_dir})
    endif()

    set(copy_destination "${QT_BUILD_DIR}/${INSTALL_QMLDIR}/${arg_TARGET_PATH}")
    foreach(file IN LISTS files_to_install)
        get_filename_component(file_name "${file}" NAME)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${file}"
                "${copy_destination}/${file_name}"
            COMMENT "Copying ${file} to ${copy_destination}"
        )
    endforeach()
endfunction()

if(NOT QT_NO_INTERNAL_COMPATIBILITY_FUNCTIONS)
    # Compatibility functions that should be removed once all their usages are removed.
    function(add_qml_module)
        qt_add_qml_module(${ARGV})
    endfunction()

    function(qt_add_qml_module)
        qt_internal_add_qml_module(${ARGV})
    endfunction()
endif()
