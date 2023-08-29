# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QmlCompilerPrivate Module:
#####################################################################

qt_internal_add_module(QmlCompiler
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
    PLUGIN_TYPES qmllint
    SOURCES
        qcoloroutput_p.h qcoloroutput.cpp
        qdeferredpointer_p.h
        qqmljsannotation.cpp qqmljsannotation_p.h
        qqmljsbasicblocks.cpp qqmljsbasicblocks_p.h
        qqmljscodegenerator.cpp qqmljscodegenerator_p.h
        qqmljscompilepass_p.h
        qqmljscompiler.cpp qqmljscompiler_p.h
        qqmljsfunctioninitializer.cpp qqmljsfunctioninitializer_p.h
        qqmljsimporter.cpp qqmljsimporter_p.h
        qqmljsimportvisitor.cpp qqmljsimportvisitor_p.h
        qqmljsliteralbindingcheck.cpp qqmljsliteralbindingcheck_p.h
        qqmljsloadergenerator.cpp qqmljsloadergenerator_p.h
        qqmljslogger_p.h qqmljslogger.cpp
        qqmljsloggingutils.h qqmljsloggingutils.cpp qqmljsloggingutils_p.h
        qqmljsmetatypes_p.h qqmljsmetatypes.cpp
        qqmljsregistercontent.cpp qqmljsregistercontent_p.h
        qqmljsresourcefilemapper.cpp qqmljsresourcefilemapper_p.h
        qqmljsscope.cpp qqmljsscope_p.h
        qqmljsscopesbyid_p.h
        qqmljsshadowcheck.cpp qqmljsshadowcheck_p.h
        qqmljsstoragegeneralizer.cpp qqmljsstoragegeneralizer_p.h
        qqmljstypedescriptionreader.cpp qqmljstypedescriptionreader_p.h
        qqmljstypepropagator.cpp qqmljstypepropagator_p.h
        qqmljstypereader.cpp qqmljstypereader_p.h
        qqmljstyperesolver.cpp qqmljstyperesolver_p.h
        qresourcerelocater.cpp qresourcerelocater_p.h
        qqmljsutils_p.h qqmljsutils.cpp
        qqmljslinter_p.h qqmljslinter.cpp
        qqmljslintercodegen_p.h qqmljslintercodegen.cpp
        qqmlsa_p.h qqmlsa.h qqmlsa.cpp
        qqmlsaconstants.h
        qqmlsasourcelocation.h qqmlsasourcelocation.cpp qqmlsasourcelocation_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
    LIBRARIES
        Qt::QmlPrivate
    PRIVATE_MODULE_INTERFACE
        Qt::QmlPrivate

)

qt_internal_add_resource(QmlCompiler "builtins"
    PREFIX
        "/qt-project.org/qml/builtins"
    BASE
        "${CMAKE_CURRENT_SOURCE_DIR}/../imports/builtins/"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/../imports/builtins/builtins.qmltypes"
        "${CMAKE_CURRENT_SOURCE_DIR}/../imports/builtins/jsroot.qmltypes"
)

qt_internal_add_docs(QmlCompiler
    doc/qtqmlcompiler.qdocconf
)
