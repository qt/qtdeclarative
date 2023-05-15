# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qml.pro.


qt_exclude_tool_directories_from_default_target(
    qmlplugindump
)

add_subdirectory(registrationmacros)
add_subdirectory(parserstress)
add_subdirectory(linebylinelex)
add_subdirectory(qjsprimitivevalue)
add_subdirectory(qjsvalueiterator)
add_subdirectory(qjsonbinding)
add_subdirectory(qqmlfile)

if(NOT INTEGRITY)
    # The INTEGRITY linker tends to crash on tst_qqmlfileselector
    add_subdirectory(qqmlfileselector)
endif()

# Limit set of tests to run for static Qt builds.
if(QT_BUILD_MINIMAL_STATIC_TESTS)
    return()
endif()

add_subdirectory(qqmlcomponent)
add_subdirectory(qqmlconsole)
add_subdirectory(qqmlengine)
add_subdirectory(qqmlerror)
add_subdirectory(qqmlincubator)
add_subdirectory(qqmlinfo)
add_subdirectory(qqmllistreference)
add_subdirectory(qqmllocale)
add_subdirectory(qqmlmetaobject)
if(NOT ANDROID) # QTBUG-100003
    add_subdirectory(qqmlmoduleplugin)
endif()
add_subdirectory(qqmlnotifier)
add_subdirectory(qqmlqt)
add_subdirectory(qqmlxmlhttprequest)
add_subdirectory(qqmlpromise)
add_subdirectory(qtqmlmodules)
add_subdirectory(qquickfolderlistmodel)
add_subdirectory(qqmlapplicationengine)
add_subdirectory(qqmlsettings)

if(NOT INTEGRITY)
# There's no mounted filesystem on INTEGRITY therefore skipping qmldiskcache
    add_subdirectory(qmldiskcache)
endif()

add_subdirectory(qqmlmetatype)
if(TARGET Qt::Quick)
    add_subdirectory(qmltc_manual)
    # QML type compiler tests cannot be run on webOS due to missing support for multiple
    # consecutive window creation from a single QtWayland client
    if(NOT WEBOS)
        add_subdirectory(qmltc)
    endif()
    # This test always creates static plugins. To avoid linker command line
    # ordering issues, we need at least CMake 3.21 and we cannot include the
    # test if qmlimportscanner is being built as part of the main project.
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.21 AND
        (QT_BUILD_STANDALONE_TESTS OR QT6_IS_SHARED_LIBS_BUILD))
        add_subdirectory(qmlbasicapp)
    endif()
    if(NOT CMAKE_CROSSCOMPILING)
        add_subdirectory(qqmljsscope)
    endif()
endif()
add_subdirectory(qmlsplitlib)
if(TARGET Qt::Widgets)
    add_subdirectory(qjsengine)
    add_subdirectory(qjsvalue)
    add_subdirectory(qjsmanagedvalue)
endif()
if(QT_FEATURE_process AND QT_FEATURE_qml_debug)
    add_subdirectory(debugger)
endif()
if(QT_FEATURE_process AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(qmlformat)
    add_subdirectory(qmlimportscanner)
    add_subdirectory(qmllint)
    add_subdirectory(qmltc_qprocess)
    add_subdirectory(qmlplugindump)
    add_subdirectory(qml)
endif()
if(QT_FEATURE_library)
    add_subdirectory(qqmlextensionplugin)
endif()
if(QT_FEATURE_private_tests)
    add_subdirectory(qqmlcpputils)
    add_subdirectory(qqmldirparser)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19" AND QT_FEATURE_process)
        # Calls qt6_target_qml_sources() directly, which needs CMake 3.19+
        add_subdirectory(qmlcachegen)
    endif()
    add_subdirectory(qmlcppcodegen)
    add_subdirectory(animation)
    add_subdirectory(qqmlecmascript)
    add_subdirectory(qqmlanybinding)
    add_subdirectory(qqmlcontext)
    add_subdirectory(qqmlexpression)
    add_subdirectory(qqmlglobal)
    add_subdirectory(qqmllanguage)
    add_subdirectory(qqmlopenmetaobject)
    add_subdirectory(qqmlproperty)
    add_subdirectory(qqmlpropertycache)
    add_subdirectory(qqmlpropertymap)
# special case begin
    if (TARGET Qt::Sql)
# special case end
        add_subdirectory(qqmlsqldatabase)
# special case begin
    endif()
# special case end
    add_subdirectory(qqmlvaluetypes)
    add_subdirectory(qqmlvaluetypeproviders)
    add_subdirectory(qqmlbinding)
    add_subdirectory(qqmlchangeset)
    add_subdirectory(qqmlconnections)
    add_subdirectory(qqmllistcompositor)
    add_subdirectory(qqmllistmodel)
    add_subdirectory(qqmllistmodelworkerscript)
    add_subdirectory(qqmlitemmodels)
    add_subdirectory(qqmltypeloader)
    add_subdirectory(qqmlparser)
    add_subdirectory(qquickworkerscript)
    add_subdirectory(qrcqml)
    add_subdirectory(qqmltimer)
    add_subdirectory(qqmlinstantiator)
    add_subdirectory(qqmlenginecleanup)
    add_subdirectory(qqmltranslation)
    add_subdirectory(qqmlimport)
    add_subdirectory(qqmlobjectmodel)
    add_subdirectory(qqmltablemodel)
    add_subdirectory(qqmltreemodeltotablemodel)
    add_subdirectory(qv4assembler)
    add_subdirectory(qv4mm)
    add_subdirectory(qv4identifiertable)
    add_subdirectory(qv4regexp)
    add_subdirectory(qv4urlobject)
    if(QT_FEATURE_process AND NOT QNX AND NOT
        (ANDROID AND CLANG AND
          CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "14.0.0" AND
          CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.0.0"))
        add_subdirectory(ecmascripttests)
    endif()
    add_subdirectory(bindingdependencyapi)
    add_subdirectory(v4misc)
    add_subdirectory(qqmldelegatemodel) # special case
    if (QT_FEATURE_qml_xmllistmodel)
        add_subdirectory(qqmlxmllistmodel)
    endif()
endif()
if(NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(qmltyperegistrar)
endif()
