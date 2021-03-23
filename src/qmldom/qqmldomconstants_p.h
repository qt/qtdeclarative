/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**/
#ifndef QQMLDOMCONSTANTS_P_H
#define QQMLDOMCONSTANTS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qqmldom_global.h"

#include <QtCore/QObject>
#include <QtCore/QMetaObject>

QT_BEGIN_NAMESPACE

namespace QQmlJS{
namespace Dom {

Q_NAMESPACE_EXPORT(QMLDOM_EXPORT)

enum class PathRoot {
    Other,
    Modules,
    Cpp,
    Libs,
    Top,
    Env,
    Universe
};
Q_ENUM_NS(PathRoot)

enum class PathCurrent {
    Other,
    Obj,
    ObjChain,
    ScopeChain,
    Component,
    Module,
    Ids,
    Types,
    LookupStrict,
    LookupDynamic,
    Lookup
};
Q_ENUM_NS(PathCurrent)

enum class Language { QmlQuick1, QmlQuick2, QmlQuick3, QmlCompiled, QmlAnnotation, Qbs };
Q_ENUM_NS(Language)

enum class ResolveOption{
    None=0,
    TraceVisit=0x1 // call the function along all elements of the path, not just for the target (the function might be called even if the target is never reached)
};
Q_ENUM_NS(ResolveOption)
Q_DECLARE_FLAGS(ResolveOptions, ResolveOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(ResolveOptions)

enum class VisitOption {
    None = 0,
    VisitSelf = 0x1, // Visit the start item
    VisitAdopted = 0x2, // Visit adopted types (but never recurses them)
    Recurse = 0x4, // recurse non adopted types
    NoPath = 0x8, // does not generate path consistent with visit
    Default = VisitOption::VisitSelf | VisitOption::VisitAdopted | VisitOption::Recurse
};
Q_ENUM_NS(VisitOption)
Q_DECLARE_FLAGS(VisitOptions, VisitOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(VisitOptions)

enum class LookupOption {
    Normal = 0,
    Strict = 0x1,
    VisitTopClassType = 0x2, // static lookup of class (singleton) or attached type, the default is
                             // visiting instance methods
    SkipFirstScope = 0x4
};
Q_ENUM_NS(LookupOption)
Q_DECLARE_FLAGS(LookupOptions, LookupOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(LookupOptions)

enum class LookupType { PropertyDef, Binding, Property, Method, Type, CppType, Symbol };
Q_ENUM_NS(LookupType)

enum class VisitPrototypesOption {
    Normal = 0,
    SkipFirst = 0x1,
    RevisitWarn = 0x2,
    ManualProceedToScope = 0x4
};
Q_ENUM_NS(VisitPrototypesOption)
Q_DECLARE_FLAGS(VisitPrototypesOptions, VisitPrototypesOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(VisitPrototypesOptions)

enum class DomKind {
    Empty,
    Object,
    List,
    Map,
    Value
};
Q_ENUM_NS(DomKind)

enum class DomType {
    Empty,

    ExternalItemInfo,
    ExternalItemPair,
    // ExternalOwningItems refer to an external path and can be shared between environments
    QmlDirectory, // dir
    QmldirFile, // qmldir
    JsFile, // file
    QmlFile, // file
    QmltypesFile, // qmltypes
    GlobalScope, // language dependent
    EnumItem,

    // types
    EnumDecl,
    JsResource,
    QmltypesComponent,
    QmlComponent,
    GlobalComponent,

    ModuleAutoExport, // dependent imports to automatically load when a module is imported
    ModuleIndex, // index for all the imports of a major version
    ModuleScope, // a specific import with full version
    ImportScope, // the scope including the types coming from one or more imports
    Export, // An exported type

    // header stuff
    Import, // wrapped
    Pragma,

    // qml elements
    Id,
    QmlObject,
    ConstantData,
    SimpleObjectWrap,
    ScriptExpression,
    Reference,
    PropertyDefinition,
    Binding,
    MethodParameter,
    MethodInfo,
    Version, // wrapped
    Comment,
    CommentedElement,
    RegionComments,
    AstComments,
    FileLocations,
    UpdatedScriptExpression,

    // convenience collecting types
    PropertyInfo,

    // Moc objects, mainly for testing
    MockObject,
    MockOwner,

    // containers
    Map,
    List,
    ListP,

    // supporting objects
    LoadInfo, // owning
    ErrorMessage, // wrapped
    AttachedInfo, // owning

    // Dom top level
    DomEnvironment,
    DomUniverse
};
Q_ENUM_NS(DomType)

enum class SimpleWrapOption { None = 0, ValueType = 1 };
Q_ENUM_NS(SimpleWrapOption)
Q_DECLARE_FLAGS(SimpleWrapOptions, SimpleWrapOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(SimpleWrapOptions)

enum class BindingValueKind { Object, ScriptExpression, Array, Empty };
Q_ENUM_NS(BindingValueKind)

enum class BindingType { Normal, OnBinding };
Q_ENUM_NS(BindingType)

enum class ListOptions {
    Normal,
    Reverse
};
Q_ENUM_NS(ListOptions)

enum class LoadOption {
    DefaultLoad = 0x0,
    ForceLoad = 0x1,
};
Q_ENUM_NS(LoadOption)
Q_DECLARE_FLAGS(LoadOptions, LoadOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(LoadOptions)

enum class EscapeOptions{
    OuterQuotes,
    NoOuterQuotes
};
Q_ENUM_NS(EscapeOptions)

enum class ErrorLevel{
    Debug = QtMsgType::QtDebugMsg,
    Info = QtMsgType::QtInfoMsg,
    Warning = QtMsgType::QtWarningMsg,
    Error = QtMsgType::QtCriticalMsg,
    Fatal = QtMsgType::QtFatalMsg
};
Q_ENUM_NS(ErrorLevel)

enum class AstDumperOption {
    None=0,
    NoLocations=0x1,
    NoAnnotations=0x2,
    DumpNode=0x4,
    SloppyCompare=0x8
};
Q_ENUM_NS(AstDumperOption)
Q_DECLARE_FLAGS(AstDumperOptions, AstDumperOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(AstDumperOptions)

enum class GoTo {
    Strict, // never go to an non uniquely defined result
    MostLikely // if needed go up to the most likely location between multiple options
};
Q_ENUM_NS(GoTo)

enum class AddOption { KeepExisting, Overwrite };
Q_ENUM_NS(AddOption)

enum class FilterUpOptions { ReturnOuter, ReturnOuterNoSelf, ReturnInner };
Q_ENUM_NS(FilterUpOptions)

enum class WriteOutCheck {
    None = 0x0,
    UpdatedDomCompare = 0x1,
    UpdatedDomStable = 0x2,
    Reparse = 0x4,
    ReparseCompare = 0x8,
    ReparseStable = 0x10,
    DumpOnFailure = 0x20,
    All = 0x3F,
    Default = Reparse | ReparseCompare | ReparseStable
};
Q_ENUM_NS(WriteOutCheck)
Q_DECLARE_FLAGS(WriteOutChecks, WriteOutCheck)
Q_DECLARE_OPERATORS_FOR_FLAGS(WriteOutChecks)

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE

#endif // QQMLDOMCONSTANTS_P_H
