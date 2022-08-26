# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmlimportscanner.pro.

#####################################################################
## qmlimportscanner Tool:
#####################################################################

qt_get_tool_target_name(target_name qmlimportscanner)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Import Scanner"
    TOOLS_TARGET Qml # special case
    INSTALL_DIR "${INSTALL_LIBEXECDIR}"
    SOURCES
        main.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::QmlCompilerPrivate
        Qt::QmlPrivate
)
qt_internal_return_unless_building_tools()

# special case begin
# Create a dummy package that will just find Qml package, for backwards compatibility reasons.
set(target "QmlImportScanner")
set(path_suffix "${INSTALL_CMAKE_NAMESPACE}${target}")
qt_path_join(config_build_dir ${QT_CONFIG_BUILD_DIR} ${path_suffix})
qt_path_join(config_install_dir ${QT_CONFIG_INSTALL_DIR} ${path_suffix})

# Configure and install Config and version file.
configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/Qt6QmlImportScannerConfig.cmake.in"
    "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Config.cmake"
    INSTALL_DESTINATION "${config_install_dir}"
)

write_basic_package_version_file(
    "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersionImpl.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
qt_internal_write_qt_package_version_file(
    "${INSTALL_CMAKE_NAMESPACE}${target}"
    "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersion.cmake"
)
qt_install(FILES
    "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Config.cmake"
    "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersion.cmake"
    "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}ConfigVersionImpl.cmake"
    DESTINATION "${config_install_dir}"
    COMPONENT Devel
)

# Configure and install ModuleDependencies file.
set(third_party_deps "")
set(main_module_tool_deps "")
set(target_deps "${INSTALL_CMAKE_NAMESPACE}Qml\;${PROJECT_VERSION}")
set(qt_module_dependencies "Qml")
configure_file(
    "${QT_CMAKE_DIR}/QtModuleDependencies.cmake.in"
    "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Dependencies.cmake"
    @ONLY
)

qt_install(FILES
    "${config_build_dir}/${INSTALL_CMAKE_NAMESPACE}${target}Dependencies.cmake"
    DESTINATION "${config_install_dir}"
    COMPONENT Devel
)

# special case end

#### Keys ignored in scope 1:.:.:qmlimportscanner.pro:<TRUE>:
# CMAKE_BIN_DIR = "$$cmakeRelativePath($$[QT_HOST_BINS], $$[QT_INSTALL_PREFIX])"
# CMAKE_DEBUG_TYPE = <EMPTY>
# CMAKE_QML_DIR = "$$cmakeRelativePath($$[QT_INSTALL_QML], $$[QT_INSTALL_PREFIX])"
# CMAKE_QML_PLUGIN_SUFFIX_RELEASE = <EMPTY>
# CMAKE_RELEASE_TYPE = <EMPTY>
# QMAKE_TARGET_DESCRIPTION = "QML" "Import" "Scanner"
# _OPTION = "host_build"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qmlimportscanner.pro:CMAKE_BIN_DIR___contains___^\\.\\./._x_:
# CMAKE_BIN_DIR = "$$[QT_HOST_BINS]/"
# CMAKE_BIN_DIR_IS_ABSOLUTE = "True"

#### Keys ignored in scope 3:.:.:qmlimportscanner.pro:CMAKE_QML_DIR___contains___^\\.\\./._x_:
# CMAKE_QML_DIR = "$$[QT_INSTALL_QML]/"
# CMAKE_QML_DIR_IS_ABSOLUTE = "True"

#### Keys ignored in scope 4:.:.:qmlimportscanner.pro:static OR staticlib:
# CMAKE_STATIC_TYPE = "true"

#### Keys ignored in scope 5:.:.:qmlimportscanner.pro:WIN32:
# CMAKE_QML_PLUGIN_SUFFIX_DEBUG = "d"

#### Keys ignored in scope 7:.:.:qmlimportscanner.pro:APPLE:
# CMAKE_QML_PLUGIN_SUFFIX_DEBUG = "_debug"

#### Keys ignored in scope 8:.:.:qmlimportscanner.pro:else:
# CMAKE_QML_PLUGIN_SUFFIX_DEBUG = <EMPTY>

#### Keys ignored in scope 9:.:.:qmlimportscanner.pro:( QT_FEATURE_debug_and_release OR QT_CONFIG___contains___debug,debug OR release ):
# CMAKE_DEBUG_TYPE = "debug"

#### Keys ignored in scope 10:.:.:qmlimportscanner.pro:( QT_FEATURE_debug_and_release OR QT_CONFIG___contains___release,debug OR release ):
# CMAKE_RELEASE_TYPE = "release"

#### Keys ignored in scope 11:.:.:qmlimportscanner.pro:QT_FEATURE_debug_and_release:
# CMAKE_DEBUG_AND_RELEASE = "TRUE"

#### Keys ignored in scope 12:.:.:qmlimportscanner.pro:else:
# CMAKE_DEBUG_AND_RELEASE = "FALSE"
