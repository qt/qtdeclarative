# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QmlWorkerScript Module:
#####################################################################

qt_internal_add_qml_module(QmlWorkerScript
    URI "QtQml.WorkerScript"
    VERSION "${PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    PLUGIN_TARGET workerscriptplugin
    CLASS_NAME QtQmlWorkerScriptPlugin
    DEPENDENCIES
        QtQml.Base/auto
    SOURCES
        qquickworkerscript.cpp qquickworkerscript_p.h
        qtqmlworkerscriptglobal.h qtqmlworkerscriptglobal_p.h
        qv4serialize.cpp qv4serialize_p.h
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
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::QmlPrivate
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)

qt_internal_extend_target(QmlWorkerScript CONDITION disassembler AND ((TEST_architecture_arch STREQUAL "i386") OR (TEST_architecture_arch STREQUAL "x86_64"))
    DEFINES
        WTF_USE_UDIS86=1
)

qt_internal_extend_target(QmlWorkerScript CONDITION (TEST_architecture_arch STREQUAL "arm") AND disassembler
    DEFINES
        WTF_USE_ARMV7_DISASSEMBLER=1
)

qt_internal_extend_target(QmlWorkerScript CONDITION (TEST_architecture_arch STREQUAL "arm64") AND disassembler
    DEFINES
        WTF_USE_ARM64_DISASSEMBLER=1
)

qt_internal_extend_target(QmlWorkerScript CONDITION (TEST_architecture_arch STREQUAL "mips") AND disassembler
    DEFINES
        WTF_USE_MIPS32_DISASSEMBLER=1
)

qt_internal_extend_target(QmlWorkerScript CONDITION NOT disassembler
    DEFINES
        WTF_USE_UDIS86=0
)

qt_internal_extend_target(QmlWorkerScript CONDITION CMAKE_BUILD_TYPE STREQUAL Release
    DEFINES
        NDEBUG
)

qt_internal_extend_target(QmlWorkerScript CONDITION GCC AND QT_COMPILER_VERSION_MAJOR STRGREATER 6 AND NOT CLANG AND NOT ICC
    COMPILE_OPTIONS
        -Wno-expansion-to-defined
)

#### Keys ignored in scope 12:.:../3rdparty/masm:../3rdparty/masm/masm-defs.pri:(QT_COMPILER_VERSION_MAJOR STRGREATER 6):
# QMAKE_CXXFLAGS_WARN_ON = "-Wno-expansion-to-defined"
qt_internal_add_docs(QmlWorkerScript
    doc/qtqmlworkerscript.qdocconf
)
