# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QmlCompilerPrivate Module:
#####################################################################

qt_internal_add_module(QmlCompiler
        PLUGIN_TYPES qmllint
    SOURCES
        qcoloroutput.cpp qcoloroutput_p.h
        qdeferredpointer_p.h
        qqmljsannotation.cpp qqmljsannotation_p.h
        qqmljsbasicblocks.cpp qqmljsbasicblocks_p.h
        qqmljscodegenerator.cpp qqmljscodegenerator_p.h
        qqmljscompilepass_p.h
        qqmljscompiler.cpp qqmljscompiler_p.h
        qqmljscompilerstats.cpp qqmljscompilerstats_p.h
        qqmljscompilerstatsreporter.cpp qqmljscompilerstatsreporter_p.h
        qqmljscontextualtypes_p.h
        qqmljsfunctioninitializer.cpp qqmljsfunctioninitializer_p.h
        qqmljsimporter.cpp qqmljsimporter_p.h
        qqmljsimportvisitor.cpp qqmljsimportvisitor_p.h
        qqmljslinter.cpp qqmljslinter_p.h
        qqmljslintercodegen.cpp qqmljslintercodegen_p.h
        qqmljsliteralbindingcheck.cpp qqmljsliteralbindingcheck_p.h
        qqmljsloadergenerator.cpp qqmljsloadergenerator_p.h
        qqmljslogger.cpp qqmljslogger_p.h
        qqmljsloggingutils.cpp qqmljsloggingutils.h qqmljsloggingutils_p.h
        qqmljsmetatypes.cpp qqmljsmetatypes_p.h
        qqmljsoptimizations.cpp qqmljsoptimizations_p.h
        qqmljsregistercontent.cpp qqmljsregistercontent_p.h
        qqmljsresourcefilemapper.cpp qqmljsresourcefilemapper_p.h
        qqmljsscope.cpp qqmljsscope_p.h
        qqmljsscopesbyid_p.h
        qqmljsshadowcheck.cpp qqmljsshadowcheck_p.h
        qqmljsstoragegeneralizer.cpp qqmljsstoragegeneralizer_p.h
        qqmljsstorageinitializer.cpp qqmljsstorageinitializer_p.h
        qqmljstypedescriptionreader.cpp qqmljstypedescriptionreader_p.h
        qqmljstypepropagator.cpp qqmljstypepropagator_p.h
        qqmljstypereader.cpp qqmljstypereader_p.h
        qqmljstyperesolver.cpp qqmljstyperesolver_p.h
        qqmljsutils.cpp qqmljsutils_p.h
        qqmljsvaluetypefromstringcheck.cpp qqmljsvaluetypefromstringcheck_p.h
        qqmlsa.cpp qqmlsa.h qqmlsa_p.h
        qqmlsaconstants.h
        qqmlsasourcelocation.cpp qqmlsasourcelocation.h qqmlsasourcelocation_p.h
        qresourcerelocater.cpp qresourcerelocater_p.h
    NO_UNITY_BUILD_SOURCES
        qqmljsoptimizations.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
    LIBRARIES
        Qt::QmlPrivate
        Qt::QmlToolingSettingsPrivate
    PRIVATE_MODULE_INTERFACE
        Qt::QmlPrivate
        Qt::QmlToolingSettingsPrivate
)

qt_path_join(qml_build_dir "${QT_BUILD_DIR}" "${INSTALL_QMLDIR}")
qt_internal_add_resource(QmlCompiler "builtins"
    PREFIX
        "/qt-project.org/qml/builtins"
    BASE
        "${qml_build_dir}"
    FILES
        "${qml_build_dir}/builtins.qmltypes"
        "${qml_build_dir}/jsroot.qmltypes"
)

qt_internal_add_docs(QmlCompiler
    doc/qtqmlcompiler.qdocconf
)
