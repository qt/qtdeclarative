# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(Qt6QmlBuildInternals.cmake)

#####################################################################
## Qml Module:
#####################################################################

# Need to evaluate Qml features early. We can't wait until
# qt_internal_add_qml_module() is executed because we need feature evaluation
# to determine some of the arguments to that command.
qt_feature_evaluate_features("${CMAKE_CURRENT_SOURCE_DIR}/configure.cmake")

set(module_dynamic_qml_imports
    QtQml.Models/auto
)

if (QT_FEATURE_qml_worker_script)
    list(APPEND module_dynamic_qml_imports
        QtQml.WorkerScript/auto
    )
endif()

set(extra_cmake_files)
set(extra_cmake_includes)
if(ANDROID)
    list(APPEND extra_cmake_files
        "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}AndroidQmlMacros.cmake")
    list(APPEND extra_cmake_includes
        "${INSTALL_CMAKE_NAMESPACE}AndroidQmlMacros.cmake")
endif()

qt_internal_add_qml_module(Qml
    URI "QtQml.Base"
    VERSION "${PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    __QT_INTERNAL_SYSTEM_MODULE
    PLUGIN_TARGET qmlplugin
    CLASS_NAME QtQmlPlugin
    PLUGIN_TYPES qmltooling
    SOURCES
        ../3rdparty/masm/assembler/ARM64Assembler.h
        ../3rdparty/masm/assembler/ARMv7Assembler.cpp ../3rdparty/masm/assembler/ARMv7Assembler.h
        ../3rdparty/masm/assembler/AbstractMacroAssembler.h
        ../3rdparty/masm/assembler/AssemblerBuffer.h
        ../3rdparty/masm/assembler/AssemblerBufferWithConstantPool.h
        ../3rdparty/masm/assembler/CodeLocation.h
        ../3rdparty/masm/assembler/LinkBuffer.cpp ../3rdparty/masm/assembler/LinkBuffer.h
        ../3rdparty/masm/assembler/MIPSAssembler.h
        ../3rdparty/masm/assembler/MacroAssembler.h
        ../3rdparty/masm/assembler/MacroAssemblerARM64.h
        ../3rdparty/masm/assembler/MacroAssemblerARMv7.h
        ../3rdparty/masm/assembler/MacroAssemblerCodeRef.h
        ../3rdparty/masm/assembler/MacroAssemblerMIPS.h
        ../3rdparty/masm/assembler/MacroAssemblerX86.h
        ../3rdparty/masm/assembler/MacroAssemblerX86Common.h
        ../3rdparty/masm/assembler/MacroAssemblerX86_64.h
        ../3rdparty/masm/assembler/X86Assembler.h
        ../3rdparty/masm/assembler/ARMv7Assembler.cpp
        ../3rdparty/masm/disassembler/ARM64/A64DOpcode.cpp ../3rdparty/masm/disassembler/ARM64/A64DOpcode.h
        ../3rdparty/masm/disassembler/ARM64Disassembler.cpp
        ../3rdparty/masm/disassembler/ARMv7/ARMv7DOpcode.cpp ../3rdparty/masm/disassembler/ARMv7/ARMv7DOpcode.h
        ../3rdparty/masm/disassembler/ARMv7Disassembler.cpp
        ../3rdparty/masm/disassembler/Disassembler.cpp
        ../3rdparty/masm/disassembler/Mips32Disassembler.cpp
        ../3rdparty/masm/disassembler/UDis86Disassembler.cpp
        ../3rdparty/masm/disassembler/mips32/Mips32Opcode.cpp ../3rdparty/masm/disassembler/mips32/Mips32Opcode.h
        ../3rdparty/masm/stubs/Options.cpp
        ../3rdparty/masm/stubs/WTFStubs.cpp ../3rdparty/masm/stubs/WTFStubs.h
        ../3rdparty/masm/stubs/wtf/FastAllocBase.h
        ../3rdparty/masm/stubs/wtf/FastMalloc.h
        ../3rdparty/masm/stubs/wtf/Noncopyable.h
        ../3rdparty/masm/stubs/wtf/OwnPtr.h
        ../3rdparty/masm/stubs/wtf/PassOwnPtr.h
        ../3rdparty/masm/stubs/wtf/PassRefPtr.h
        ../3rdparty/masm/stubs/wtf/RefCounted.h
        ../3rdparty/masm/stubs/wtf/RefPtr.h
        ../3rdparty/masm/stubs/wtf/TypeTraits.h
        ../3rdparty/masm/stubs/wtf/UnusedParam.h
        ../3rdparty/masm/stubs/wtf/Vector.h
        ../3rdparty/masm/stubs/yarr/YarrUnicodeProperties.cpp
        ../3rdparty/masm/wtf/ASCIICType.h
        ../3rdparty/masm/wtf/Assertions.h
        ../3rdparty/masm/wtf/Atomics.h
        ../3rdparty/masm/wtf/BumpPointerAllocator.h
        ../3rdparty/masm/wtf/CheckedArithmetic.h
        ../3rdparty/masm/wtf/Compiler.h
        ../3rdparty/masm/wtf/CryptographicallyRandomNumber.h
        ../3rdparty/masm/wtf/DataLog.h
        ../3rdparty/masm/wtf/DynamicAnnotations.h
        ../3rdparty/masm/wtf/EnumClass.h
        ../3rdparty/masm/wtf/FeatureDefines.h
        ../3rdparty/masm/wtf/FilePrintStream.cpp ../3rdparty/masm/wtf/FilePrintStream.h
        ../3rdparty/masm/wtf/Locker.h
        ../3rdparty/masm/wtf/MathExtras.h
        ../3rdparty/masm/wtf/NotFound.h
        ../3rdparty/masm/wtf/NullPtr.h
        ../3rdparty/masm/wtf/OSAllocator.h
        ../3rdparty/masm/wtf/PageAllocation.h
        ../3rdparty/masm/wtf/PageAllocationAligned.cpp ../3rdparty/masm/wtf/PageAllocationAligned.h
        ../3rdparty/masm/wtf/PageBlock.cpp ../3rdparty/masm/wtf/PageBlock.h
        ../3rdparty/masm/wtf/PageReservation.h
        ../3rdparty/masm/wtf/Platform.h
        ../3rdparty/masm/wtf/PossiblyNull.h
        ../3rdparty/masm/wtf/PrintStream.cpp ../3rdparty/masm/wtf/PrintStream.h
        ../3rdparty/masm/wtf/RawPointer.h
        ../3rdparty/masm/wtf/SegmentedVector.h
        ../3rdparty/masm/wtf/StdLibExtras.h
        ../3rdparty/masm/wtf/VMTags.h
        ../3rdparty/masm/yarr/Yarr.h
        ../3rdparty/masm/yarr/YarrCanonicalizeUCS2.cpp ../3rdparty/masm/yarr/YarrCanonicalizeUCS2.h
        ../3rdparty/masm/yarr/YarrCanonicalizeUnicode.cpp
        ../3rdparty/masm/yarr/YarrInterpreter.cpp ../3rdparty/masm/yarr/YarrInterpreter.h
        ../3rdparty/masm/yarr/YarrJIT.cpp ../3rdparty/masm/yarr/YarrJIT.h
        ../3rdparty/masm/yarr/YarrParser.h
        ../3rdparty/masm/yarr/YarrPattern.cpp ../3rdparty/masm/yarr/YarrPattern.h
        ../3rdparty/masm/yarr/YarrSyntaxChecker.cpp ../3rdparty/masm/yarr/YarrSyntaxChecker.h
        ../3rdparty/masm/yarr/YarrUnicodeProperties.h
        common/qjsnumbercoercion.cpp common/qjsnumbercoercion.h
        common/qqmljsdiagnosticmessage_p.h
        common/qqmljsfixedpoolarray_p.h
        common/qqmljsmemorypool_p.h
        common/qqmljssourcelocation_p.h
        common/qv4alloca_p.h
        common/qv4calldata_p.h
        common/qv4compileddata_p.h
        common/qv4staticvalue_p.h
        common/qv4stringtoarrayindex_p.h
        common/qqmltranslation.cpp common/qqmltranslation_p.h
        compat/removed_api.cpp
        compiler/qqmlirbuilder.cpp compiler/qqmlirbuilder_p.h
        compiler/qv4bytecodegenerator.cpp compiler/qv4bytecodegenerator_p.h
        compiler/qv4bytecodehandler.cpp compiler/qv4bytecodehandler_p.h
        compiler/qv4codegen.cpp compiler/qv4codegen_p.h
        compiler/qv4compiler.cpp compiler/qv4compiler_p.h
        compiler/qv4compilercontext.cpp compiler/qv4compilercontext_p.h
        compiler/qv4compilercontrolflow_p.h
        compiler/qv4compilerglobal_p.h
        compiler/qv4compilerscanfunctions.cpp compiler/qv4compilerscanfunctions_p.h
        compiler/qv4instr_moth.cpp compiler/qv4instr_moth_p.h
        compiler/qv4util_p.h
        debugger/qqmldebug.h
        debugger/qqmldebugconnector_p.h
        debugger/qqmldebugserviceinterfaces_p.h
        debugger/qqmldebugstatesdelegate_p.h
        debugger/qqmlprofiler_p.h
        inlinecomponentutils_p.h
        jsapi/qjsengine.cpp jsapi/qjsengine.h jsapi/qjsengine_p.h
        jsapi/qjslist.cpp jsapi/qjslist.h
        jsapi/qjsmanagedvalue.cpp jsapi/qjsmanagedvalue.h
        jsapi/qjsprimitivevalue.cpp jsapi/qjsprimitivevalue.h
        jsapi/qjsvalue.cpp jsapi/qjsvalue.h jsapi/qjsvalue_p.h
        jsapi/qjsvalueiterator.cpp jsapi/qjsvalueiterator.h jsapi/qjsvalueiterator_p.h
        jsruntime/qv4argumentsobject.cpp jsruntime/qv4argumentsobject_p.h
        jsruntime/qv4arraybuffer.cpp jsruntime/qv4arraybuffer_p.h
        jsruntime/qv4arraydata.cpp jsruntime/qv4arraydata_p.h
        jsruntime/qv4arrayiterator.cpp jsruntime/qv4arrayiterator_p.h
        jsruntime/qv4arrayobject.cpp jsruntime/qv4arrayobject_p.h
        jsruntime/qv4atomics.cpp jsruntime/qv4atomics_p.h
        jsruntime/qv4booleanobject.cpp jsruntime/qv4booleanobject_p.h
        jsruntime/qv4compilationunitmapper.cpp jsruntime/qv4compilationunitmapper_p.h
        jsruntime/qv4context.cpp jsruntime/qv4context_p.h
        jsruntime/qv4dataview.cpp jsruntime/qv4dataview_p.h
        jsruntime/qv4dateobject.cpp jsruntime/qv4dateobject_p.h
        jsruntime/qv4debugging.cpp jsruntime/qv4debugging_p.h
        jsruntime/qv4domerrors.cpp jsruntime/qv4domerrors_p.h
        jsruntime/qv4engine.cpp jsruntime/qv4engine_p.h
        jsruntime/qv4enginebase_p.h
        jsruntime/qv4errorobject.cpp jsruntime/qv4errorobject_p.h
        jsruntime/qv4estable.cpp jsruntime/qv4estable_p.h
        jsruntime/qv4executableallocator.cpp jsruntime/qv4executableallocator_p.h
        jsruntime/qv4executablecompilationunit.cpp jsruntime/qv4executablecompilationunit_p.h
        jsruntime/qv4function.cpp jsruntime/qv4function_p.h
        jsruntime/qv4functionobject.cpp jsruntime/qv4functionobject_p.h
        jsruntime/qv4functiontable_p.h
        jsruntime/qv4generatorobject.cpp jsruntime/qv4generatorobject_p.h
        jsruntime/qv4global_p.h
        jsruntime/qv4globalobject.cpp jsruntime/qv4globalobject_p.h
        jsruntime/qv4identifierhash.cpp jsruntime/qv4identifierhash_p.h
        jsruntime/qv4identifierhashdata_p.h
        jsruntime/qv4identifiertable.cpp jsruntime/qv4identifiertable_p.h
        jsruntime/qv4include.cpp jsruntime/qv4include_p.h
        jsruntime/qv4internalclass.cpp jsruntime/qv4internalclass_p.h
        jsruntime/qv4iterator.cpp jsruntime/qv4iterator_p.h
        jsruntime/qv4jscall_p.h jsruntime/qv4jscall.cpp
        jsruntime/qv4jsonobject.cpp jsruntime/qv4jsonobject_p.h
        jsruntime/qv4lookup.cpp jsruntime/qv4lookup_p.h
        jsruntime/qv4managed.cpp jsruntime/qv4managed_p.h
        jsruntime/qv4mapiterator.cpp jsruntime/qv4mapiterator_p.h
        jsruntime/qv4mapobject.cpp jsruntime/qv4mapobject_p.h
        jsruntime/qv4math_p.h
        jsruntime/qv4mathobject.cpp jsruntime/qv4mathobject_p.h
        jsruntime/qv4memberdata.cpp jsruntime/qv4memberdata_p.h
        jsruntime/qv4module.cpp jsruntime/qv4module_p.h
        jsruntime/qv4numberobject.cpp jsruntime/qv4numberobject_p.h
        jsruntime/qv4object.cpp jsruntime/qv4object_p.h
        jsruntime/qv4objectiterator.cpp jsruntime/qv4objectiterator_p.h
        jsruntime/qv4objectproto.cpp jsruntime/qv4objectproto_p.h
        jsruntime/qv4persistent.cpp jsruntime/qv4persistent_p.h
        jsruntime/qv4profiling_p.h
        jsruntime/qv4promiseobject.cpp jsruntime/qv4promiseobject_p.h
        jsruntime/qv4property_p.h
        jsruntime/qv4propertykey.cpp jsruntime/qv4propertykey_p.h
        jsruntime/qv4proxy.cpp jsruntime/qv4proxy_p.h
        jsruntime/qv4qmlcontext.cpp jsruntime/qv4qmlcontext_p.h
        jsruntime/qv4qobjectwrapper.cpp jsruntime/qv4qobjectwrapper_p.h
        jsruntime/qv4reflect.cpp jsruntime/qv4reflect_p.h
        jsruntime/qv4regexp.cpp jsruntime/qv4regexp_p.h
        jsruntime/qv4regexpobject.cpp jsruntime/qv4regexpobject_p.h
        jsruntime/qv4resolvedtypereference.cpp jsruntime/qv4resolvedtypereference_p.h
        jsruntime/qv4runtime.cpp jsruntime/qv4runtime_p.h
        jsruntime/qv4runtimeapi_p.h
        jsruntime/qv4runtimecodegen.cpp jsruntime/qv4runtimecodegen_p.h
        jsruntime/qv4scopedvalue_p.h
        jsruntime/qv4script.cpp jsruntime/qv4script_p.h
        jsruntime/qv4setiterator.cpp jsruntime/qv4setiterator_p.h
        jsruntime/qv4setobject.cpp jsruntime/qv4setobject_p.h
        jsruntime/qv4sparsearray.cpp jsruntime/qv4sparsearray_p.h
        jsruntime/qv4sqlerrors.cpp jsruntime/qv4sqlerrors_p.h
        jsruntime/qv4stackframe.cpp jsruntime/qv4stackframe_p.h
        jsruntime/qv4string.cpp jsruntime/qv4string_p.h
        jsruntime/qv4stringiterator.cpp jsruntime/qv4stringiterator_p.h
        jsruntime/qv4stringobject.cpp jsruntime/qv4stringobject_p.h
        jsruntime/qv4symbol.cpp jsruntime/qv4symbol_p.h
        jsruntime/qv4typedarray.cpp jsruntime/qv4typedarray_p.h
        jsruntime/qv4urlobject.cpp jsruntime/qv4urlobject_p.h
        jsruntime/qv4value.cpp jsruntime/qv4value_p.h
        jsruntime/qv4variantobject.cpp jsruntime/qv4variantobject_p.h
        jsruntime/qv4vme_moth.cpp jsruntime/qv4vme_moth_p.h
        jsruntime/qv4sequenceobject.cpp jsruntime/qv4sequenceobject_p.h
        jsruntime/qv4vtable_p.h
        jsruntime/qv4referenceobject.cpp jsruntime/qv4referenceobject_p.h
        memory/qv4heap_p.h
        memory/qv4mm.cpp memory/qv4mm_p.h
        memory/qv4mmdefs_p.h
        memory/qv4stacklimits.cpp memory/qv4stacklimits_p.h
        memory/qv4writebarrier_p.h
        parser/qqmljsast.cpp parser/qqmljsast_p.h
        parser/qqmljsastfwd_p.h
        parser/qqmljsastvisitor.cpp parser/qqmljsastvisitor_p.h
        parser/qqmljsengine_p.h
        parser/qqmljsglobal_p.h
        parser/qqmljskeywords_p.h
        parser/qqmljslexer.cpp parser/qqmljslexer_p.h
        qml/ftw/qdoubleendedlist_p.h
        qml/ftw/qfieldlist_p.h
        qml/ftw/qfinitestack_p.h
        qml/ftw/qbipointer_p.h
        qml/ftw/qhashedstring.cpp qml/ftw/qhashedstring_p.h
        qml/ftw/qintrusivelist.cpp qml/ftw/qintrusivelist_p.h
        qml/ftw/qlazilyallocated_p.h
        qml/ftw/qlinkedstringhash_p.h
        qml/ftw/qpodvector_p.h
        qml/ftw/qprimefornumbits_p.h
        qml/ftw/qqmlnullablevalue_p.h
        qml/ftw/qqmlrefcount_p.h
        qml/ftw/qqmlthread.cpp qml/ftw/qqmlthread_p.h
        qml/ftw/qrecursionwatcher_p.h
        qml/ftw/qrecyclepool_p.h
        qml/ftw/qstringhash_p.h
        qml/qqmlregistration.h
        qml/qqml.cpp qml/qqml.h
        qml/qqmlabstractbinding.cpp qml/qqmlabstractbinding_p.h
        qml/qqmlabstracturlinterceptor.cpp qml/qqmlabstracturlinterceptor.h
        qml/qqmlapplicationengine.cpp qml/qqmlapplicationengine.h qml/qqmlapplicationengine_p.h
        qml/qqmlbinding.cpp qml/qqmlbinding_p.h
        qml/qqmlboundsignal.cpp qml/qqmlboundsignal_p.h
        qml/qqmlbuiltinfunctions.cpp qml/qqmlbuiltinfunctions_p.h
        qml/qqmlcomponent.cpp qml/qqmlcomponent.h qml/qqmlcomponent_p.h
        qml/qqmlcomponentandaliasresolver_p.h
        qml/qqmlcomponentattached_p.h
        qml/qqmlcontext.cpp qml/qqmlcontext.h qml/qqmlcontext_p.h
        qml/qqmlcontextdata.cpp qml/qqmlcontextdata_p.h
        qml/qqmlcustomparser.cpp qml/qqmlcustomparser_p.h
        qml/qqmldata_p.h
        qml/qqmldatablob.cpp qml/qqmldatablob_p.h
        qml/qqmldelayedcallqueue.cpp qml/qqmldelayedcallqueue_p.h
        qml/qqmldirdata.cpp qml/qqmldirdata_p.h
        qml/qqmlengine.cpp qml/qqmlengine.h qml/qqmlengine_p.h
        qml/qqmlenumdata_p.h
        qml/qqmlenumvalue_p.h
        qml/qqmlerror.cpp qml/qqmlerror.h
        qml/qqmlexpression.cpp qml/qqmlexpression.h qml/qqmlexpression_p.h
        qml/qqmlextensioninterface.cpp qml/qqmlextensioninterface.h
        qml/qqmlextensionplugin.cpp qml/qqmlextensionplugin.h qml/qqmlextensionplugin_p.h
        qml/qqmlfile.cpp qml/qqmlfile.h
        qml/qqmlfileselector.cpp qml/qqmlfileselector.h qml/qqmlfileselector_p.h
        qml/qqmlglobal.cpp qml/qqmlglobal_p.h
        qml/qqmlguard_p.h
        qml/qqmlguardedcontextdata_p.h
        qml/qqmlimport.cpp qml/qqmlimport_p.h
        qml/qqmlincubator.cpp qml/qqmlincubator.h qml/qqmlincubator_p.h
        qml/qqmlinfo.cpp qml/qqmlinfo.h
        qml/qqmlirloader.cpp qml/qqmlirloader_p.h
        qml/qqmljavascriptexpression.cpp qml/qqmljavascriptexpression_p.h
        qml/qqmllist.cpp qml/qqmllist.h qml/qqmllist_p.h
        qml/qqmllistwrapper.cpp qml/qqmllistwrapper_p.h
        qml/qqmlloggingcategory.cpp qml/qqmlloggingcategory_p.h
        qml/qqmlmetamoduleregistration.cpp
        qml/qqmlmetaobject.cpp qml/qqmlmetaobject_p.h
        qml/qqmlmetatype.cpp qml/qqmlmetatype_p.h
        qml/qqmlmetatypedata.cpp qml/qqmlmetatypedata_p.h
        qml/qqmlmoduleregistration.cpp qml/qqmlmoduleregistration.h
        qml/qqmlnetworkaccessmanagerfactory.cpp qml/qqmlnetworkaccessmanagerfactory.h
        qml/qqmlnotifier.cpp qml/qqmlnotifier_p.h
        qml/qqmlobjectcreator.cpp qml/qqmlobjectcreator_p.h
        qml/qqmlobjectorgadget.cpp qml/qqmlobjectorgadget_p.h
        qml/qqmlopenmetaobject.cpp qml/qqmlopenmetaobject_p.h
        qml/qqmlparserstatus.cpp qml/qqmlparserstatus.h
        qml/qqmlplatform.cpp qml/qqmlplatform_p.h
        qml/qqmlpluginimporter.cpp qml/qqmlpluginimporter_p.h
        qml/qqmlprivate.h
        qml/qqmlproperty.cpp qml/qqmlproperty.h qml/qqmlproperty_p.h
        qml/qqmlpropertybinding.cpp qml/qqmlpropertybinding_p.h
        qml/qqmlanybinding_p.h
        qml/qqmlpropertycache.cpp qml/qqmlpropertycache_p.h
        qml/qqmlpropertycachecreator.cpp qml/qqmlpropertycachecreator_p.h
        qml/qqmlpropertycachemethodarguments_p.h
        qml/qqmlpropertycachevector_p.h
        qml/qqmlpropertydata_p.h
        qml/qqmlpropertyindex_p.h
        qml/qqmlpropertyresolver.cpp qml/qqmlpropertyresolver_p.h
        qml/qqmlpropertytopropertybinding.cpp qml/qqmlpropertytopropertybinding_p.h
        qml/qqmlpropertyvalidator.cpp qml/qqmlpropertyvalidator_p.h
        qml/qqmlpropertyvalueinterceptor.cpp qml/qqmlpropertyvalueinterceptor_p.h
        qml/qqmlfinalizer.cpp qml/qqmlfinalizer_p.h
        qml/qqmlpropertyvaluesource.cpp qml/qqmlpropertyvaluesource.h
        qml/qqmlproxymetaobject.cpp qml/qqmlproxymetaobject_p.h
        qml/qqmlscriptblob.cpp qml/qqmlscriptblob_p.h
        qml/qqmlscriptdata.cpp qml/qqmlscriptdata_p.h
        qml/qqmlscriptstring.cpp qml/qqmlscriptstring.h qml/qqmlscriptstring_p.h
        qml/qqmlsourcecoordinate_p.h
        qml/qqmlstringconverters.cpp qml/qqmlstringconverters_p.h
        qml/qqmltype.cpp qml/qqmltype_p.h
        qml/qqmltype_p_p.h
        qml/qqmltypecompiler.cpp qml/qqmltypecompiler_p.h
        qml/qqmltypedata.cpp qml/qqmltypedata_p.h
        qml/qqmltypeloader.cpp qml/qqmltypeloader_p.h
        qml/qqmltypeloaderqmldircontent.cpp qml/qqmltypeloaderqmldircontent_p.h
        qml/qqmltypeloaderthread.cpp qml/qqmltypeloaderthread_p.h
        qml/qqmltypemodule.cpp qml/qqmltypemodule_p.h
        qml/qqmltypemoduleversion.cpp qml/qqmltypemoduleversion_p.h
        qml/qqmltypenamecache.cpp qml/qqmltypenamecache_p.h
        qml/qqmltypewrapper.cpp qml/qqmltypewrapper_p.h
        qml/qqmlvaluetype.cpp qml/qqmlvaluetype_p.h
        qml/qqmlvaluetypeproxybinding.cpp qml/qqmlvaluetypeproxybinding_p.h
        qml/qqmlvaluetypewrapper.cpp qml/qqmlvaluetypewrapper_p.h
        qml/qqmlvme.cpp qml/qqmlvme_p.h
        qml/qqmlvmemetaobject.cpp qml/qqmlvmemetaobject_p.h
        qmldirparser/qqmldirparser.cpp qmldirparser/qqmldirparser_p.h
        qmldirparser/qqmlimportresolver.cpp qmldirparser/qqmlimportresolver_p.h
        qtqmlcompilerglobal.h qtqmlcompilerglobal_p.h
        qtqmlglobal.h qtqmlglobal_p.h
        types/qqmlbind.cpp types/qqmlbind_p.h
        types/qqmlconnections.cpp types/qqmlconnections_p.h
        util/qqmlpropertymap.cpp util/qqmlpropertymap.h

        qmltc/qqmltcobjectcreationhelper_p.h
        qmltc/qqmltcobjectcreationhelper.cpp
        qmltc/supportlibrary/qqmlcppbinding_p.h
        qmltc/supportlibrary/qqmlcppbinding.cpp
        qmltc/supportlibrary/qqmlcpponassignment_p.h
        qmltc/supportlibrary/qqmlcpponassignment.cpp
        qmltc/supportlibrary/qqmlcpptypehelpers_p.h
    NO_UNITY_BUILD_SOURCES
        jsapi/qjsvalue.cpp            # \.
        jsruntime/qv4engine.cpp       #  > WTF::String vs. QV4::String, dto. Object
        jsruntime/qv4jsonobject.cpp   # |
        jsruntime/qv4object.cpp       # |
        jsruntime/qv4proxy.cpp        # |
        jsruntime/qv4qmlcontext.cpp   # |
        jsruntime/qv4reflect.cpp      # |
        jsruntime/qv4regexpobject.cpp # |
        jsruntime/qv4script.cpp       # | ((also?) Function)
        jsruntime/qv4string.cpp       # |
        jsruntime/qv4stringobject.cpp # | ((also?) Property)
        jsruntime/qv4typedarray.cpp   # |
        jsruntime/qv4urlobject.cpp    # | ((also?) Property)
        qml/qqmlbuiltinfunctions.cpp  # |
        qml/qqmllistwrapper.cpp       # |
        qml/qqmllocale.cpp            # |
        qml/qqmltypecompiler.cpp      # | (also Binding)
        qml/qqmltypewrapper.cpp       # |
        qml/qqmlxmlhttprequest.cpp    # / (also Document)
        jsruntime/qv4managed.cpp      # QQmlJS::Managed vs. QV4::Managed
        jsruntime/qv4module.cpp       # QV4::Compiler::Module vs. QV4::Module
        jsruntime/qv4vme_moth.cpp     # 'MOTH_BEGIN_INSTR' macro redefined (from qv4instr_moth.cpp)
        qml/qqmltypecompiler.cpp      # 'COMPILE_EXCEPTION' macro redefined (from qqmlirbuilder.cpp)
        qml/qqmlmetamoduleregistration.cpp # declares 'registration' clashing with qml_* files
    DEFINES
        BUILDING_QT__
        ENABLE_ASSEMBLER_WX_EXCLUSIVE=1
        ENABLE_DFG_JIT=0
        ENABLE_DFG_JIT_UTILITY_METHODS=1
        ENABLE_JIT_CONSTANT_BLINDING=0
        ENABLE_LLINT=0
        JS_EXPORT_PRIVATE= # special case
        QT_NO_FOREACH
        QT_NO_INTEGER_EVENT_COORDINATES
        QT_NO_URL_CAST_FROM_STRING
        WTFInvokeCrashHook=qmlWTFInvokeCrashHook
        WTFReportAssertionFailure=qmlWTFReportAssertionFailure
        WTFReportAssertionFailureWithMessage=qmlWTFReportAssertionFailureWithMessage
        WTFReportBacktrace=qmlWTFReportBacktrace
        WTF_EXPORT_PRIVATE= # special case
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_BINARY_DIR}/.generated
        ${CMAKE_CURRENT_BINARY_DIR}/compiler
        ${CMAKE_CURRENT_BINARY_DIR}/jsruntime
        ${CMAKE_CURRENT_BINARY_DIR}/memory
        ${CMAKE_CURRENT_BINARY_DIR}/qmldirparser
        ../3rdparty/masm
        ../3rdparty/masm/assembler
        ../3rdparty/masm/disassembler
        ../3rdparty/masm/disassembler/udis86
        ../3rdparty/masm/jit
        ../3rdparty/masm/runtime
        ../3rdparty/masm/stubs
        ../3rdparty/masm/stubs/runtime
        ../3rdparty/masm/stubs/wtf
        ../3rdparty/masm/wtf
        compiler
        debugger
        jsruntime
        memory
        qmldirparser
    LIBRARIES
        Qt::CorePrivate
        ${FWCoreFoundation}
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::QmlIntegration
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
    EXTRA_CMAKE_FILES
        "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}qmldirTemplate.cmake.in"
        "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}QmlPluginTemplate.cpp.in"
        "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}QmlFindQmlscInternal.cmake"
        "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}QmlDeploySupport.cmake"
        "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}QmlModuleDirMappingTemplate.qrc.in"
        "${CMAKE_CURRENT_LIST_DIR}/${INSTALL_CMAKE_NAMESPACE}QmltcFileMappingTemplate.qrc.in"
        ${extra_cmake_files}
    EXTRA_CMAKE_INCLUDES
        "${INSTALL_CMAKE_NAMESPACE}QmlFindQmlscInternal.cmake"
        ${extra_cmake_includes}
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
    POLICIES
        QTP0001
)

qt_update_ignore_pch_source(Qml "compat/removed_api.cpp")

set_source_files_properties(compat/removed_api.cpp
                            jsruntime/qv4mathobject.cpp # math.h issues on Windows
                            PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

qt_internal_add_qml_module(QmlMeta
    URI "QtQml"
    VERSION "${PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASS_NAME QtQmlMetaPlugin
    PLUGIN_TARGET QmlMeta

    # Prevent type registration
    NO_GENERATE_QMLTYPES

    PAST_MAJOR_VERSIONS 2
    IMPORTS
        QtQml.Base/auto
        ${module_dynamic_qml_imports}
)

# Add the QtQml qmldir to libQtQml, too.
# Since we also provide the (bare bones) type  registration in libQtQml,
# this makes the complete module reside in libQtQml. There is no need to
# load the QmlMeta plugin, then.
# Se still provide the plugin so that static linking works.
get_target_property(qtqml_out_dir QmlMeta QT_QML_MODULE_OUTPUT_DIRECTORY)
qt_internal_add_resource(Qml "qmlMetaQmldir"
    PREFIX
        "/qt-project.org/imports/QtQml"
    FILES
        ${qtqml_out_dir}/qmldir
)

# Linking to the static qml plugin should also automatically link to the worker script
# static plugin otherwise you get errors like
#  module "QtQml.WorkerScript" plugin "workerscriptplugin" not found
#     import QtQuick 2.0
#     ^
if(QT_FEATURE_qml_worker_script)
    _qt_internal_add_qml_static_plugin_dependency(qmlplugin workerscriptplugin)
endif()
# Same for the QmlMeta qml plugin, otherwise you get
#  module "QtQuick" version 6.6 cannot be imported because:
#  module "QtQml" plugin "qmlmetaplugin" not found
#     import QtQuick
#     ^
_qt_internal_add_qml_static_plugin_dependency(qmlplugin QmlMeta)

# special case begin remove the block, handled manually
# QLALR Grammars:
#qt_process_qlalr(
#    Qml
#    parser/qqmljs.g
#    ""
#)

qt_declarative_write_tag_header(Qml)
set(_qt_qlalr_flags "--no-debug" "--qt")
qt_process_qlalr(Qml "${CMAKE_CURRENT_SOURCE_DIR}/parser/qqmljs.g" "${_qt_qlalr_flags}")
qt_declarative_generate_reg_exp_jit_tables(Qml)
# special case end

qt_internal_extend_target(Qml CONDITION QT_FEATURE_qml_network
    SOURCES
        qml/qqmltypeloadernetworkreplyproxy.cpp qml/qqmltypeloadernetworkreplyproxy_p.h
    PUBLIC_LIBRARIES
        Qt::Network
)

qt_internal_extend_target(Qml CONDITION MSVC AND (TEST_architecture_arch STREQUAL "i386")
    LINK_OPTIONS
        "/BASE:0x66000000"
)

qt_internal_extend_target(Qml CONDITION MSVC
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)

qt_internal_extend_target(Qml CONDITION WIN32
    SOURCES
        ../3rdparty/masm/wtf/OSAllocatorWin.cpp
        jsruntime/qv4compilationunitmapper_win.cpp
    PUBLIC_LIBRARIES
        shell32
)

qt_internal_extend_target(Qml CONDITION LINUX AND QT_FEATURE_dlopen
    LIBRARIES
        dl
)

#### Keys ignored in scope 7:.:.:qml.pro:solaris-cc_x_:
# QMAKE_CXXFLAGS_RELEASE = "--O2"

qt_internal_extend_target(Qml CONDITION GCC AND (TEST_architecture_arch STREQUAL "mips")
    COMPILE_OPTIONS
        -fno-reorder-blocks
)

qt_internal_extend_target(Qml CONDITION EXISTS "qqml_enable_gcov"
    LIBRARIES
        gcov
    COMPILE_OPTIONS
        -fno-elide-constructors
        -fprofile-arcs
        -ftest-coverage
)

qt_internal_extend_target(Qml CONDITION release AND MSVC AND (QT_CL_MAJOR_VERSION EQUAL 19) AND (QT_CL_MINOR_VERSION EQUAL 00) AND (QT_CL_PATCH_VERSION GREATER 24212)
    COMPILE_OPTIONS
        -d2SSAOptimizer-
)

#### Keys ignored in scope 11:.:.:qml.pro:ICC:
# WERROR = "-ww2415"

#### Keys ignored in scope 12:.:.:qml.pro:(QT_CLANG_MAJOR_VERSION GREATER 3) OR (QT_CLANG_MINOR_VERSION GREATER 3) OR (QT_APPLE_CLANG_MAJOR_VERSION GREATER 5) OR ( (QT_APPLE_CLANG_MAJOR_VERSION EQUAL 5) AND (QT_APPLE_CLANG_MINOR_VERSION GREATER 0) ):
# WERROR = "-Wno-error=unused-const-variable"

qt_internal_extend_target(Qml CONDITION QT_FEATURE_qml_jit
    SOURCES
        jit/qv4assemblercommon.cpp jit/qv4assemblercommon_p.h
        jit/qv4baselineassembler.cpp jit/qv4baselineassembler_p.h
        jit/qv4baselinejit.cpp jit/qv4baselinejit_p.h
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_BINARY_DIR}/jit
        jit
)

qt_internal_extend_target(Qml CONDITION QT_FEATURE_qml_animation
    SOURCES
        animations/qabstractanimationjob.cpp animations/qabstractanimationjob_p.h
        animations/qanimationgroupjob.cpp animations/qanimationgroupjob_p.h
        animations/qanimationjobutil_p.h
        animations/qcontinuinganimationgroupjob.cpp animations/qcontinuinganimationgroupjob_p.h
        animations/qparallelanimationgroupjob.cpp animations/qparallelanimationgroupjob_p.h
        animations/qpauseanimationjob.cpp animations/qpauseanimationjob_p.h
        animations/qsequentialanimationgroupjob.cpp animations/qsequentialanimationgroupjob_p.h
        types/qqmltimer.cpp types/qqmltimer_p.h
    INCLUDE_DIRECTORIES
        animations
)

qt_internal_extend_target(Qml CONDITION GCC AND QT_COMPILER_VERSION_MAJOR STREQUAL 5
    COMPILE_OPTIONS
        -fno-strict-aliasing
)

qt_internal_extend_target(Qml CONDITION QT_FEATURE_qml_debug
    SOURCES
        debugger/qqmlabstractprofileradapter.cpp debugger/qqmlabstractprofileradapter_p.h
        debugger/qqmlconfigurabledebugservice_p.h
        debugger/qqmldebug.cpp
        debugger/qqmldebugconnector.cpp
        debugger/qqmldebugpluginmanager_p.h
        debugger/qqmldebugserver.cpp debugger/qqmldebugserver_p.h
        debugger/qqmldebugserverconnection.cpp debugger/qqmldebugserverconnection_p.h
        debugger/qqmldebugservice.cpp debugger/qqmldebugservice_p.h
        debugger/qqmldebugservicefactory.cpp debugger/qqmldebugservicefactory_p.h
        debugger/qqmldebugserviceinterfaces.cpp
        debugger/qqmldebugtranslationprotocol_p.h
        debugger/qqmlprofiler.cpp
        debugger/qqmlprofilerdefinitions_p.h
        jsruntime/qv4profiling.cpp
)

qt_internal_extend_target(Qml CONDITION UNIX
    SOURCES
        jsruntime/qv4compilationunitmapper_unix.cpp
        jsruntime/qv4functiontable_unix.cpp
)

qt_internal_extend_target(Qml CONDITION (TEST_architecture_arch STREQUAL "x86_64") AND WIN32
    SOURCES
        jsruntime/qv4functiontable_win64.cpp
)

qt_internal_extend_target(Qml CONDITION WIN32 AND NOT (TEST_architecture_arch STREQUAL "x86_64")
    SOURCES
        jsruntime/qv4functiontable_noop.cpp
)

qt_internal_extend_target(Qml CONDITION valgrind
    DEFINES
        V4_USE_VALGRIND
)

qt_internal_extend_target(Qml CONDITION heaptrack
    DEFINES
        V4_USE_HEAPTRACK
)

qt_internal_extend_target(Qml CONDITION QT_FEATURE_qml_xml_http_request
    SOURCES
        qml/qqmlxmlhttprequest.cpp qml/qqmlxmlhttprequest_p.h
)

qt_internal_extend_target(Qml CONDITION QT_FEATURE_qml_locale
    SOURCES
        qml/qqmllocale.cpp qml/qqmllocale_p.h
)

qt_internal_extend_target(Qml CONDITION ANDROID
    DEFINES
        LIBS_SUFFIX="_${ANDROID_ABI}.so" # special case
)

qt_internal_extend_target(Qml CONDITION hpux-_x_ OR solaris-_x_ OR (QT_FEATURE_clock_gettime AND linux-_x_)
    LIBRARIES
        rt
)

qt_internal_extend_target(Qml CONDITION disassembler AND ((TEST_architecture_arch STREQUAL "i386") OR (TEST_architecture_arch STREQUAL "x86_64"))
    DEFINES
        WTF_USE_UDIS86=1
)

qt_internal_extend_target(Qml CONDITION (TEST_architecture_arch STREQUAL "arm") AND disassembler
    DEFINES
        WTF_USE_ARMV7_DISASSEMBLER=1
)

qt_internal_extend_target(Qml CONDITION (TEST_architecture_arch STREQUAL "arm64") AND disassembler
    DEFINES
        WTF_USE_ARM64_DISASSEMBLER=1
)

qt_internal_extend_target(Qml CONDITION (TEST_architecture_arch STREQUAL "mips") AND disassembler
    DEFINES
        WTF_USE_MIPS32_DISASSEMBLER=1
)

qt_internal_extend_target(Qml CONDITION NOT disassembler
    DEFINES
        WTF_USE_UDIS86=0
)

qt_internal_extend_target(Qml CONDITION CMAKE_BUILD_TYPE STREQUAL Release
    DEFINES
        NDEBUG
)

qt_internal_extend_target(Qml CONDITION GCC AND QT_COMPILER_VERSION_MAJOR STRGREATER 6 AND NOT CLANG AND NOT ICC
    COMPILE_OPTIONS
        -Wno-expansion-to-defined
)

#### Keys ignored in scope 66:.:../3rdparty/masm:../3rdparty/masm/masm-defs.pri:(QT_COMPILER_VERSION_MAJOR STRGREATER 6):
# QMAKE_CXXFLAGS_WARN_ON = "-Wno-expansion-to-defined"

qt_internal_extend_target(Qml CONDITION INTEGRITY
    SOURCES
        ../3rdparty/masm/wtf/OSAllocatorIntegrity.cpp
)

qt_internal_extend_target(Qml CONDITION UNIX AND NOT INTEGRITY
    SOURCES
        ../3rdparty/masm/wtf/OSAllocatorPosix.cpp
)

qt_internal_extend_target(Qml CONDITION DEFINES___contains___WTF_USE_UDIS86=1
    SOURCES
        ../3rdparty/masm/disassembler/udis86/udis86.c
        ../3rdparty/masm/disassembler/udis86/udis86_decode.c
        ../3rdparty/masm/disassembler/udis86/udis86_input.c
        ../3rdparty/masm/disassembler/udis86/udis86_itab_holder.c
        ../3rdparty/masm/disassembler/udis86/udis86_syn.c
        ../3rdparty/masm/disassembler/udis86/udis86_syn-att.c
        ../3rdparty/masm/disassembler/udis86/udis86_syn-intel.c
)

qt_internal_generate_tracepoints(Qml qml
    SOURCES
        qml/qqmlobjectcreator.cpp
        qml/qqmlboundsignal.cpp
        qml/qqmlbinding.cpp
        qml/qqmltypeloader.cpp
        jsruntime/qv4vme_moth.cpp
)

qt_internal_add_docs(Qml
    doc/qtqml.qdocconf
)

# include snippet projects for developer shared builds
# static builds fail with
#   CMake Error: AUTOMOC for target qt_target_qml_sources_example_resources_3:
#   The "moc" executable does not exist
if(QT_FEATURE_private_tests AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.19" AND QT_BUILD_SHARED_LIBS)
    add_subdirectory(doc/snippets/cmake/qt_target_qml_sources)
    qt_autogen_tools(qt_target_qml_sources_example ENABLE_AUTOGEN_TOOLS moc)
    qt_autogen_tools(qt_target_qml_sources_exampleplugin ENABLE_AUTOGEN_TOOLS moc)
endif()

# The Qml_sync_headers target doesn't exist when this CMakeLists is executed. So the qmlplugin
# plugin target misses the dependency on Qml_sync_headers target. This leads to the
# "unknown IID" issue when moc processes plugin sources, because of missing header aliases.
# Qml_sync_headers target is created later by the finalizer in the directory scope so we add this
# dependency manually instead of relying on qt_internal_add_qml_module logic. Same is applicable
# for QmlMeta target.
set_property(TARGET qmlplugin APPEND PROPERTY AUTOGEN_TARGET_DEPENDS
    Qml_sync_headers)
set_property(TARGET QmlMeta APPEND PROPERTY AUTOGEN_TARGET_DEPENDS
    Qml_sync_headers)
