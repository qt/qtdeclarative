// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLPREVIEWDIFF_P_H
#define QQMLPREVIEWDIFF_P_H

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

#include <private/qv4compileddata_p.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

namespace QV4::CompiledData {
enum class ChangeType : quint8 {
    None,

    // -- Changes to various internal tables, source file, timestamp, etc.
    UnitMetadataChanged,

    BlockAdded,
    BlockChanged,
    BlockRemoved,

    ClassAdded,
    ClassChanged,
    ClassRemoved,

    ConstantAdded,
    ConstantChanged,
    ConstantRemoved,

    JSClassAdded,
    JSClassChanged,
    JSClassRemoved,

    LookupAdded,
    LookupChanged,
    LookupRemoved,

    RegExpAdded,
    RegExpChanged,
    RegExpRemoved,

    RequiredPropertyExtraDataAdded,
    RequiredPropertyExtraDataChanged,
    RequiredPropertyExtraDataRemoved,
    RequiredPropertyExtraDataLocationChanged,
    RequiredPropertyExtraDataIndexChanged,

    StringDataAdded,
    StringDataChanged,
    StringDataRemoved,

    TemplateObjectAdded,
    TemplateObjectChanged,
    TemplateObjectRemoved,

    TranslationDataAdded,
    TranslationDataChanged,
    TranslationDataRemoved,

    // -- Actual document-level changes --

    AliasAdded,
    AliasChanged,
    AliasRemoved,
    AliasLocationChanged,
    AliasIndexChanged,

    BindingAdded,
    BindingChanged,
    BindingRemoved,
    BindingLocationChanged,
    BindingIndexChanged,

    EnumAdded,
    EnumChanged,
    EnumRemoved,
    EnumLocationChanged,
    EnumIndexChanged,

    FunctionAdded,
    FunctionChanged,
    FunctionRemoved,
    FunctionLocationChanged,
    FunctionIndexChanged,

    ImportAdded,
    ImportChanged,
    ImportRemoved,
    ImportLocationChanged,

    InlineComponentAdded,
    InlineComponentChanged,
    InlineComponentRemoved,
    InlineComponentLocationChanged,
    InlineComponentIndexChanged,

    ObjectAdded,
    ObjectChanged,
    ObjectRemoved,
    ObjectLocationChanged,

    PropertyAdded,
    PropertyChanged,
    PropertyRemoved,
    PropertyLocationChanged,
    PropertyIndexChanged,

    SignalAdded,
    SignalChanged,
    SignalRemoved,
    SignalLocationChanged,
    SignalIndexChanged,

    Unknown,
};

using UnitHunk = Unit;
using TranslationDataHunk = TranslationData;
using InlineComponentHunk = InlineComponent;
using RequiredPropertyExtraDataHunk = RequiredPropertyExtraData;
using LookupHunk = Lookup;
using RegExpHunk = RegExp;
using ImportHunk = Import;
using AliasHunk = Alias;
using BindingHunk = Binding;
using PropertyHunk = Property;
using ConstantHunk = quint64_le;
using StringHunk = QString;
using FunctionIndexHunk = quint32_le;

struct EnumHunk
{
    Enum data;
    QList<EnumValue> values;
};

struct SignalHunk
{
    Signal data;
    QList<Parameter> parameters;
};

struct FunctionHunk
{
    Function data;

    QList<Parameter> formals;
    QList<quint32_le> locals;
    QList<CodeOffsetToLineAndStatement> lineAndStatementNumbers;
    QList<quint32_le> labelInfos;
    QByteArray code;
};

struct ClassHunk
{
    Class data;
    QList<Method> methods;
};

struct TemplateObjectHunk
{
    quint32 size;
    QList<quint32_le> strings; // 2*size entries: raw[0..size-1] then cooked[0..size-1]
};

struct JsClassHunk
{
    quint32 nMembers;
    QList<JSClassMember> members;
};

struct BlockHunk
{
    Block data;
    QList<quint32_le> locals;
};

struct ObjectHunk
{
    Object data;

    QList<FunctionIndexHunk> functionIndices;
    QList<PropertyHunk> properties;
    QList<AliasHunk> aliases;
    QList<EnumHunk> enums;
    QList<SignalHunk> sigs;
    QList<BindingHunk> bindings;
    QList<InlineComponentHunk> inlineComponents;
    QList<RequiredPropertyExtraDataHunk> requiredPropertyExtraData;
};

struct NoHunk {};

struct Change
{
    ChangeType type = ChangeType::None;
    int objectIndex = -1;

    // Position in the *new* unit (binding/property/function/object/etc newIndex depending on type).
    // For a Removed change there is no new position; newIndex then holds the old position instead.
    quint32 newIndex = 0;

    // Position in the *old* unit. For positionally-matched tables this equals newIndex.
    // For an Added change there is no old position; oldIndex then holds the new position instead.
    quint32 oldIndex = 0;

    std::variant<NoHunk, ConstantHunk, BindingHunk, PropertyHunk, AliasHunk, EnumHunk, SignalHunk,
                 ObjectHunk, UnitHunk, StringHunk, LookupHunk, RegExpHunk, FunctionHunk,
                 ClassHunk, TemplateObjectHunk, JsClassHunk, BlockHunk, ImportHunk,
                 TranslationDataHunk, InlineComponentHunk, RequiredPropertyExtraDataHunk>
            data;
};

struct CompilationUnitDiff
{
    QList<Change> changes;
    bool success = false;
};

// Compare two compilation units and return a diff describing the changes
CompilationUnitDiff diffCompilationUnits(const Unit *oldUnit, const Unit *newUnit);

} // namespace QV4::CompiledData

QT_END_NAMESPACE

#endif // QQMLPREVIEWDIFF_P_H
