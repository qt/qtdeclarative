# Generated from qmljs.pro.

#####################################################################
## qmljs Tool:
#####################################################################

qt_get_tool_target_name(target_name qmljs)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML JavaScript Tool"
    TOOLS_TARGET Qml # special case
    SOURCES
        qmljs.cpp
    DEFINES
        BUILDING_QT__
        ENABLE_ASSEMBLER_WX_EXCLUSIVE=1
        ENABLE_DFG_JIT=0
        ENABLE_DFG_JIT_UTILITY_METHODS=1
        ENABLE_JIT_CONSTANT_BLINDING=0
        ENABLE_LLINT=0
        JS_EXPORT_PRIVATE= # special case
        WTFInvokeCrashHook=qmlWTFInvokeCrashHook
        WTFReportAssertionFailure=qmlWTFReportAssertionFailure
        WTFReportAssertionFailureWithMessage=qmlWTFReportAssertionFailureWithMessage
        WTFReportBacktrace=qmlWTFReportBacktrace
        WTF_EXPORT_PRIVATE= # special case
    INCLUDE_DIRECTORIES
        ../../src/3rdparty/masm
        ../../src/3rdparty/masm/assembler
        ../../src/3rdparty/masm/disassembler
        ../../src/3rdparty/masm/disassembler/udis86
        ../../src/3rdparty/masm/jit
        ../../src/3rdparty/masm/runtime
        ../../src/3rdparty/masm/stubs
        ../../src/3rdparty/masm/stubs/runtime
        ../../src/3rdparty/masm/stubs/wtf
        ../../src/3rdparty/masm/wtf
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:qmljs.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "JavaScript" "Tool"
# TEMPLATE = "app"

## Scopes:
#####################################################################

qt_internal_extend_target(${target_name} CONDITION WIN32
    DEFINES
        NOMINMAX
)

qt_internal_extend_target(${target_name} CONDITION disassembler AND ((TEST_architecture_arch STREQUAL "i386") OR (TEST_architecture_arch STREQUAL "x86_64"))
    DEFINES
        WTF_USE_UDIS86=1
)

qt_internal_extend_target(${target_name} CONDITION (TEST_architecture_arch STREQUAL "arm") AND disassembler
    DEFINES
        WTF_USE_ARMV7_DISASSEMBLER=1
)

qt_internal_extend_target(${target_name} CONDITION (TEST_architecture_arch STREQUAL "arm64") AND disassembler
    DEFINES
        WTF_USE_ARM64_DISASSEMBLER=1
)

qt_internal_extend_target(${target_name} CONDITION (TEST_architecture_arch STREQUAL "mips") AND disassembler
    DEFINES
        WTF_USE_MIPS32_DISASSEMBLER=1
)

qt_internal_extend_target(${target_name} CONDITION NOT disassembler
    DEFINES
        WTF_USE_UDIS86=0
)

qt_internal_extend_target(${target_name} CONDITION CMAKE_BUILD_TYPE STREQUAL Release
    DEFINES
        NDEBUG
)

qt_internal_extend_target(${target_name} CONDITION GCC AND QT_COMPILER_VERSION_MAJOR STRGREATER 6 AND NOT CLANG AND NOT ICC
    COMPILE_OPTIONS
        -Wno-expansion-to-defined
)

#### Keys ignored in scope 12:.:../../src/3rdparty/masm:../../src/3rdparty/masm/masm-defs.pri:(QT_COMPILER_VERSION_MAJOR STRGREATER 6):
# QMAKE_CXXFLAGS_WARN_ON = "-Wno-expansion-to-defined"
