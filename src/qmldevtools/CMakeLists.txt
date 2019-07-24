# Generated from qmldevtools.pro.

#####################################################################
## QmlDevTools Module:
#####################################################################

add_qt_module(QmlDevTools
    STATIC
    INTERNAL_MODULE
    NO_SYNC_QT
    SOURCES
        ../qml/common/qqmlapiversion_p.h
        ../qml/common/qqmljsdiagnosticmessage_p.h
        ../qml/common/qqmljsfixedpoolarray_p.h
        ../qml/common/qqmljsmemorypool_p.h
        ../qml/common/qv4alloca_p.h
        ../qml/common/qv4calldata_p.h
        ../qml/common/qv4compileddata_p.h
        ../qml/common/qv4staticvalue_p.h
        ../qml/common/qv4stringtoarrayindex_p.h
        ../qml/compiler/qqmlirbuilder.cpp ../qml/compiler/qqmlirbuilder_p.h
        ../qml/compiler/qv4bytecodegenerator.cpp ../qml/compiler/qv4bytecodegenerator_p.h
        ../qml/compiler/qv4bytecodehandler.cpp ../qml/compiler/qv4bytecodehandler_p.h
        ../qml/compiler/qv4codegen.cpp ../qml/compiler/qv4codegen_p.h
        ../qml/compiler/qv4compiler.cpp ../qml/compiler/qv4compiler_p.h
        ../qml/compiler/qv4compilercontext.cpp ../qml/compiler/qv4compilercontext_p.h
        ../qml/compiler/qv4compilercontrolflow_p.h
        ../qml/compiler/qv4compilerglobal_p.h
        ../qml/compiler/qv4compilerscanfunctions.cpp ../qml/compiler/qv4compilerscanfunctions_p.h
        ../qml/compiler/qv4instr_moth.cpp ../qml/compiler/qv4instr_moth_p.h
        ../qml/compiler/qv4util_p.h
        ../qml/parser/qqmljsast.cpp ../qml/parser/qqmljsast_p.h
        ../qml/parser/qqmljsastfwd_p.h
        ../qml/parser/qqmljsastvisitor.cpp ../qml/parser/qqmljsastvisitor_p.h
        ../qml/parser/qqmljsengine_p.cpp ../qml/parser/qqmljsengine_p.h ../qml/parser/qqmljsengine_p.h
        ../qml/parser/qqmljsglobal_p.h
        ../qml/parser/qqmljskeywords_p.h
        ../qml/parser/qqmljslexer.cpp ../qml/parser/qqmljslexer_p.h
        ../qml/parser/qqmljssourcelocation_p.h
        ../qml/qmldirparser/qqmldirparser.cpp ../qml/qmldirparser/qqmldirparser_p.h
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_BINARY_DIR}/../qml
        ${CMAKE_CURRENT_BINARY_DIR}/../qml/compiler
        ${CMAKE_CURRENT_BINARY_DIR}/../qml/qmldirparser
        ../qml/compiler
        ../qml/qmldirparser
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
)

# special case begin
# Need to generate the parser files as in Qml module.
set(_qt_qlalr_flags "--no-debug" "--qt")
qt_process_qlalr("${CMAKE_CURRENT_SOURCE_DIR}/../qml/parser/qqmljs.g" QmlDevTools "${_qt_qlalr_flags}")

# Need to propagate some of the include directories from Qml via QmlDevToolsPrivate
# for other tools, and also for the main QmlDevTools module,
# until we fix the script to handle internal_module to create only one single Private module.
qt_internal_module_info(qml_module "Qml")
set(_qml_dev_tools_private_includes
        $<BUILD_INTERFACE:${qml_module_include_dir}>
        $<BUILD_INTERFACE:${qml_module_include_dir}/${PROJECT_VERSION}>
        $<BUILD_INTERFACE:${qml_module_include_dir}/${PROJECT_VERSION}/${qml_module}>
    )
target_include_directories(QmlDevTools PRIVATE ${_qml_dev_tools_private_includes})
target_include_directories(QmlDevToolsPrivate INTERFACE ${_qml_dev_tools_private_includes})
# special case end

#### Keys ignored in scope 1:.:.:qmldevtools.pro:<TRUE>:
# CONFIG = "minimal_syncqt" "internal_module" "generated_privates" "qlalr"
# MODULE_INCNAME = "QtQml"
# _LOADED = "qt_module"
# _OPTION = "host_build"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qmldevtools.pro:ICC:
# WERROR = "-ww2415"

#### Keys ignored in scope 3:.:.:qmldevtools.pro:CLANG AND greaterThan(QT_CLANG_MAJOR_VERSION,3 OR greaterThan(QT_CLANG_MINOR_VERSION,3)):
# WERROR = "-Wno-error=unused-const-variable"

#### Keys ignored in scope 5:.:../qml/common:../qml/common/common.pri:NOT build_pass:
# compile_hash_contents = "// Generated file, DO NOT EDIT" "$${LITERAL_HASH}define QML_COMPILE_HASH "$$QML_COMPILE_HASH"" "$${LITERAL_HASH}define QML_COMPILE_HASH_LENGTH $$str_size($$QML_COMPILE_HASH)"
# tag = <EMPTY>
# tagFile = "$$PWD/../../.tag"

#### Keys ignored in scope 6:.:../qml/common:../qml/common/common.pri:EXISTS _ss_tagFile:
# QMAKE_INTERNAL_INCLUDED_FILES = "$$tagFile"
# tag = "$$cat$$tagFile,singleline"

#### Keys ignored in scope 7:.:../qml/common:../qml/common/common.pri:NOT tag___equals____ss_{LITERAL_DOLLAR}Format AND %H_ss_{LITERAL_DOLLAR}:
# QML_COMPILE_HASH = "$$tag"

#### Keys ignored in scope 9:.:../qml/common:../qml/common/common.pri:EXISTS _ss_PWD/../../.git:
# QML_COMPILE_HASH = "$$commit"
# commit = "$$systemgitrev-parseHEAD"

extend_target(QmlDevTools CONDITION (GCC) AND ((QT_COMPILER_VERSION_MAJOR STREQUAL 5))
    COMPILE_OPTIONS
        -fno-strict-aliasing
)
