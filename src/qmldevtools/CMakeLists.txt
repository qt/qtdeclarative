# Generated from qmldevtools.pro.

#####################################################################
## QmlDevToolsPrivate Module:
#####################################################################

qt_internal_add_module(QmlDevToolsPrivate
    STATIC
    INTERNAL_MODULE
    NO_SYNC_QT
    SOURCES
        ../qml/common/qqmljsdiagnosticmessage_p.h
        ../qml/common/qqmljsfixedpoolarray_p.h
        ../qml/common/qqmljsmemorypool_p.h
        ../qml/common/qqmljssourcelocation_p.h
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
        ../qml/parser/qqmljsengine_p.cpp ../qml/parser/qqmljsengine_p.h
        ../qml/parser/qqmljsglobal_p.h
        ../qml/parser/qqmljskeywords_p.h
        ../qml/parser/qqmljslexer.cpp ../qml/parser/qqmljslexer_p.h
        ../qml/qmldirparser/qqmldirparser.cpp ../qml/qmldirparser/qqmldirparser_p.h
        ../qml/qmldirparser/qqmlimportresolver.cpp ../qml/qmldirparser/qqmlimportresolver_p.h
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_BINARY_DIR}/../qml
        ${CMAKE_CURRENT_BINARY_DIR}/../qml/compiler
        ${CMAKE_CURRENT_BINARY_DIR}/../qml/qmldirparser
        ../qml/compiler
        ../qml/qmldirparser
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

# special case begin
# Need to generate the parser files as in Qml module.
set(_qt_qlalr_flags "--no-debug" "--qt")
qt_process_qlalr(QmlDevToolsPrivate "${CMAKE_CURRENT_SOURCE_DIR}/../qml/parser/qqmljs.g" "${_qt_qlalr_flags}")

# Need to propagate some of the include directories from Qml via QmlDevToolsPrivate
# for other tools, and also for the main QmlDevToolsPrivate module,
# until we fix the script to handle internal_module to create only one single Private module.
qt_internal_module_info(qml_module "Qml")
if(QT_FEATURE_framework)
    get_target_property(_qml_fw_headers_dir Qt::Qml LIBRARY_OUTPUT_DIRECTORY)
    string(APPEND _qml_fw_headers_dir /${qml_module}.framework/Versions/A/Headers)
    set(_qml_dev_tools_private_includes
        $<BUILD_INTERFACE:${_qml_fw_headers_dir}>
        $<BUILD_INTERFACE:${_qml_fw_headers_dir}/${PROJECT_VERSION}>
        $<BUILD_INTERFACE:${_qml_fw_headers_dir}/${PROJECT_VERSION}/${qml_module}>
        )
else()
    set(_qml_dev_tools_private_includes
            $<BUILD_INTERFACE:${qml_module_build_interface_include_dir}>
            $<BUILD_INTERFACE:${qml_repo_build_interface_include_dir}>
            $<BUILD_INTERFACE:${qml_module_build_interface_versioned_include_dir}>
            $<BUILD_INTERFACE:${qml_module_build_interface_versioned_inner_include_dir}>
        )
endif()
target_include_directories(QmlDevToolsPrivate PUBLIC ${_qml_dev_tools_private_includes})
if(QT_FEATURE_framework)
    set(_qml_fw_headers_dir lib/${qml_module}.framework/Headers)
    set(_qml_dev_tools_public_includes
        $<INSTALL_INTERFACE:${_qml_fw_headers_dir}>
        $<INSTALL_INTERFACE:${_qml_fw_headers_dir}/${PROJECT_VERSION}>
        $<INSTALL_INTERFACE:${_qml_fw_headers_dir}/${PROJECT_VERSION}/${qml_module}>
        )
else()
    set(_qml_dev_tools_public_includes
            $<INSTALL_INTERFACE:${qml_module_install_interface_include_dir}>
            $<INSTALL_INTERFACE:${qml_module_install_interface_versioned_include_dir}>
            $<INSTALL_INTERFACE:${qml_module_install_interface_versioned_inner_include_dir}>
        )
endif()
target_include_directories(QmlDevToolsPrivate PUBLIC ${_qml_dev_tools_public_includes})

set_target_properties(QmlDevToolsPrivate
    PROPERTIES EXTRA_INCLUDE_DIRECTORIES
        "${_qml_dev_tools_public_includes}${_qml_dev_tools_private_includes}"
)
# We build the qlalr sources into qmldevtools, so there's no link-time
# dependency to QtQml. However we also include files in QmlDevToolsPrivate such
# as qqmlirbuilder.cpp that include <private/qqmljsgrammar_p.h> that
# originate from QtQml. That particular header was created at cmake time
# via syncqt, but it's just a forwarding header that in turn includes
# the generated file. That generated file is created when building
# QtQml, so we must depend on it:
add_dependencies(QmlDevToolsPrivate Qml)
# special case end

#### Keys ignored in scope 1:.:.:qmldevtools.pro:<TRUE>:
# MODULE_INCNAME = "QtQml"
# _OPTION = "host_build"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qmldevtools.pro:ICC:
# WERROR = "-ww2415"

#### Keys ignored in scope 3:.:.:qmldevtools.pro:CLANG AND ( (QT_CLANG_MAJOR_VERSION GREATER 3) OR (QT_CLANG_MINOR_VERSION GREATER 3) ):
# WERROR = "-Wno-error=unused-const-variable"

#### Keys ignored in scope 5:.:../qml/common:../qml/common/common.pri:NOT build_pass:
# compile_hash_contents = "// Generated file, DO NOT EDIT" "$${LITERAL_HASH}define QML_COMPILE_HASH "$$QML_COMPILE_HASH"" "$${LITERAL_HASH}define QML_COMPILE_HASH_LENGTH $$str_size($$QML_COMPILE_HASH)"
# tag = <EMPTY>
# tagFile = "$$PWD/../../.tag"

#### Keys ignored in scope 6:.:../qml/common:../qml/common/common.pri:EXISTS _ss_tagFile:
# QMAKE_INTERNAL_INCLUDED_FILES = "$$tagFile"
# tag = "$$cat($$tagFile, singleline)"

#### Keys ignored in scope 7:.:../qml/common:../qml/common/common.pri:NOT tag___equals____ss_{LITERAL_DOLLAR}Format AND %H_ss_{LITERAL_DOLLAR}:
# QML_COMPILE_HASH = "$$tag"

#### Keys ignored in scope 9:.:../qml/common:../qml/common/common.pri:EXISTS _ss_PWD/../../.git:
# QML_COMPILE_HASH = "$$commit"
# commit = "$$system(git rev-parse HEAD)"

qt_internal_extend_target(QmlDevToolsPrivate CONDITION GCC AND QT_COMPILER_VERSION_MAJOR STREQUAL 5
    COMPILE_OPTIONS
        -fno-strict-aliasing
)
