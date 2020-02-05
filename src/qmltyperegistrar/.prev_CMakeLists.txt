# Generated from qmltyperegistrar.pro.

#####################################################################
## qmltyperegistrar Tool:
#####################################################################

qt_add_tool(qmltyperegistrar
    SOURCES
        ../../tools/shared/qmlstreamwriter.cpp ../../tools/shared/qmlstreamwriter.h
        ../../tools/shared/resourcefilemapper.cpp ../../tools/shared/resourcefilemapper.h
        qmltyperegistrar.cpp
        qmltypesclassdescription.cpp qmltypesclassdescription.h
        qmltypescreator.cpp qmltypescreator.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    INCLUDE_DIRECTORIES
        ../../tools/shared
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

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
