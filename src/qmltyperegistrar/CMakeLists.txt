# Generated from qmltyperegistrar.pro.

#####################################################################
## qmltyperegistrar Tool:
#####################################################################

qt_get_tool_target_name(target_name qmltyperegistrar)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Types Registrar"
    TOOLS_TARGET Qml # special case
    INSTALL_DIR "${INSTALL_LIBEXECDIR}"
    SOURCES
        ../qmlcompiler/qqmljsstreamwriter.cpp ../qmlcompiler/qqmljsstreamwriter_p.h
        metatypesjsonprocessor.cpp metatypesjsonprocessor.h
        qmltyperegistrar.cpp
        qmltypesclassdescription.cpp qmltypesclassdescription.h
        qmltypescreator.cpp qmltypescreator.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    INCLUDE_DIRECTORIES
        ../qmlcompiler
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

# Install public prf files.
set(qmltyperegistrar_mkspecs "${CMAKE_CURRENT_SOURCE_DIR}/qmltypes.prf")
set(mkspecs_install_dir "${INSTALL_MKSPECSDIR}")
qt_path_join(mkspecs_install_dir "${QT_INSTALL_DIR}" "${mkspecs_install_dir}" "features")
qt_copy_or_install(FILES "${qmltyperegistrar_mkspecs}"
                   DESTINATION ${mkspecs_install_dir})

qt_internal_return_unless_building_tools()

#### Keys ignored in scope 1:.:.:qmltyperegistrar.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Types" "Registrar"
# _OPTION = "host_build"
# build_integration.files = "qmltypes.prf"
# build_integration.path = "$$[QT_HOST_DATA]/mkspecs/features"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qmltyperegistrar.pro:prefix_build:
# COPIES = "qmltypes_to_builddir"
# INSTALLS = "build_integration"
# qmltypes_to_builddir.files = "qmltypes.prf"
# qmltypes_to_builddir.path = "$$MODULE_BASE_OUTDIR/mkspecs/features"

#### Keys ignored in scope 3:.:.:qmltyperegistrar.pro:else:
# COPIES = "build_integration"
