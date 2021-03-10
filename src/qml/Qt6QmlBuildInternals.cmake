#
# QtDeclarative Specific extensions
#

include_guard(GLOBAL)

# This function is essentially a convenience wrapper around a pair of calls
# to qt_internal_add_plugin() and qt6_add_qml_module(). It ensures a consistent
# set of arguments are used for both. Most keywords for either command are
# supported, with a few exceptions:
#
# - RESOURCE_PREFIX and RESOURCE_EXPORT are both hard-coded and cannot be
#   overridden by the caller.
# - OUTPUT_DIRECTORY and INSTALL_DIRECTORY will be set if not provided.
# - SOURCES is only passed through to qt_internal_add_plugin() but not to
#   qt6_add_qml_module(). If SOURCES is not set, PURE_MODULE will be passed to
#   qt6_add_qml_module() so that a dummy plugin.cpp file will be generated.
#
# See qt_internal_add_plugin() and qt6_add_qml_module() for the full set of
# supported keywords.
function(qt_internal_add_qml_module target)

    _qt_internal_get_add_plugin_keywords(
        public_option_args
        public_single_args
        public_multi_args
    )
    qt_internal_get_internal_add_plugin_keywords(
        internal_option_args
        internal_single_args
        internal_multi_args
    )

    set(qml_module_option_args
        GENERATE_QMLTYPES
        INSTALL_QMLTYPES
        DESIGNER_SUPPORTED
        SKIP_TYPE_REGISTRATION
        PLUGIN_OPTIONAL
    )

    set(qml_module_single_args
        URI
        TARGET_PATH
        VERSION
        CLASSNAME
        TYPEINFO
    )

    set(qml_module_multi_args
        QML_FILES
        IMPORTS
        OPTIONAL_IMPORTS
        DEPENDENCIES
        PAST_MAJOR_VERSIONS
    )

    set(option_args
        ${public_option_args}
        ${internal_option_args}
        ${qml_module_option_args}
    )
    set(single_args
        ${public_single_args}
        ${internal_single_args}
        ${qml_module_single_args}
    )
    set(multi_args
        ${public_multi_args}
        ${internal_multi_args}
        ${qml_module_multi_args}
    )

    qt_parse_all_arguments(arg "qt_internal_add_qml_module"
        "${option_args}"
        "${single_args}"
        "${multi_args}"
        ${ARGN}
    )

    if (NOT arg_TARGET_PATH)
        string(REPLACE "." "/" arg_TARGET_PATH ${arg_URI})
    endif()
    if (NOT arg_OUTPUT_DIRECTORY)
        set(arg_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_QMLDIR}/${arg_TARGET_PATH}")
    endif()
    if (NOT arg_INSTALL_DIRECTORY)
        set(arg_INSTALL_DIRECTORY "${INSTALL_QMLDIR}/${arg_TARGET_PATH}")
    endif()

    qt_remove_args(plugin_args
        ARGS_TO_REMOVE
            ${qml_module_option_args}
            ${qml_module_single_args}
            ${qml_module_multi_args}
            OUTPUT_DIRECTORY
            INSTALL_DIRECTORY
        ALL_ARGS
            ${option_args}
            ${single_args}
            ${multi_args}
        ARGS
            ${ARGN}
    )

    qt_internal_add_plugin(${target}
        TYPE qml_plugin
        OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
        INSTALL_DIRECTORY ${arg_INSTALL_DIRECTORY}
        ${plugin_args}
    )

    if (arg_SOURCES AND NOT arg_TYPEINFO)
        set(arg_TYPEINFO "plugins.qmltypes")
    endif()

    set(add_qml_module_args DO_NOT_CREATE_TARGET)

    # Pass through options if given (these are present/absent, not true/false)
    foreach(opt IN LISTS qml_module_option_args)
        if(arg_${opt})
            list(APPEND add_qml_module_args ${opt})
        endif()
    endforeach()

    # Pass through single and multi-value args as provided. CLASSNAME is
    # special because it can be passed to qt_internal_add_plugin() and
    # qt6_add_qml_module().
    foreach(arg IN LISTS qml_module_single_args qml_module_multi_args
                   ITEMS CLASSNAME)
        if(DEFINED arg_${arg})
            list(APPEND add_qml_module_args ${arg} ${arg_${arg}})
        endif()
    endforeach()


    # Because qt_internal_add_qml_module does not propagate its SOURCES option to
    # qt6_add_qml_module, but only to qt_internal_add_plugin, we need a way to tell
    # qt6_add_qml_module if it should generate a dummy plugin cpp file. Otherwise we'd generate
    # a dummy plugin.cpp file twice and thus cause duplicate symbol issues.
    if (NOT arg_SOURCES)
        list(APPEND add_qml_module_args PURE_MODULE)
    endif()

    qt6_add_qml_module(${target}
        ${add_qml_module_args}
        OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY}
        INSTALL_DIRECTORY ${arg_INSTALL_DIRECTORY}
        RESOURCE_PREFIX "/qt-project.org/imports"
        RESOURCE_EXPORT "${INSTALL_CMAKE_NAMESPACE}${target}Targets"
    )
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
