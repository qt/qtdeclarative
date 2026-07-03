// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqmlpreviewdiff_p.h"

#include <private/qv4staticvalue_p.h>

#include <QtCore/qmap.h>
#include <QtCore/qscopedvaluerollback.h>
#include <QtCore/qset.h>

#include <cstring>

QT_BEGIN_NAMESPACE

// IMPORTANT:
//
// If you update the data structures in qv4compileddata_p.h, then you also need to
// update the comparison functions below. Only after making sure the comparisons
// are fine, bump the version in the static_assert.
static_assert(QV4_DATA_STRUCTURE_VERSION == 0x4f);

namespace QV4::CompiledData {

using Constant = quint64_le;
using StringData = QString;

template <typename Data>
Data extractData(const Data &data)
{
    return data;
}

static EnumHunk extractData(const Enum &e)
{
    EnumHunk data;
    data.data = e;
    for (const EnumValue *v = e.enumValuesBegin(); v != e.enumValuesEnd(); ++v)
        data.values.append(*v);
    return data;
}

static SignalHunk extractData(const Signal &s)
{
    SignalHunk data;
    data.data = s;
    for (quint32 i = 0; i < s.nParameters; ++i)
        data.parameters.append(*s.parameterAt(i));
    return data;
}

static FunctionHunk extractData(const Function &func)
{
    FunctionHunk data;
    data.data = func;

    for (quint16 i = 0, end = func.nFormals; i != end; ++i)
        data.formals.append(func.formalsTable()[i]);

    for (quint16 i = 0, end = func.nLocals; i != end; ++i)
        data.locals.append(func.localsTable()[i]);

    for (quint16 i = 0, end = func.nLineAndStatementNumbers; i != end; ++i)
        data.lineAndStatementNumbers.append(func.lineAndStatementNumberTable()[i]);

    for (quint32 i = 0, end = func.nLabelInfos; i != end; ++i)
        data.labelInfos.append(func.labelInfoTable()[i]);

    data.code = QByteArray(func.code(), func.codeSize);
    return data;
}

static ObjectHunk extractData(const Object &obj)
{
    ObjectHunk data;
    data.data = obj;

    for (quint32 i = 0; i < obj.nFunctions; ++i)
        data.functionIndices.append(extractData(obj.functionOffsetTable()[i]));

    for (quint32 i = 0; i < obj.nBindings; ++i)
        data.bindings.append(extractData(obj.bindingTable()[i]));

    for (quint32 i = 0; i < obj.nProperties; ++i)
        data.properties.append(extractData(obj.propertyTable()[i]));

    for (quint32 i = 0; i < obj.nAliases; ++i)
        data.aliases.append(extractData(obj.aliasTable()[i]));

    for (quint32 i = 0; i < obj.nSignals; ++i)
        data.sigs.append(extractData(*obj.signalAt(i)));

    for (quint32 i = 0; i < obj.nEnums; ++i)
        data.enums.append(extractData(*obj.enumAt(i)));

    for (quint32 i = 0; i < obj.nNamedObjectsInComponent; ++i) {
        data.namedObjectsInComponentIndices.append(
                extractData(obj.namedObjectsInComponentTable()[i]));
    }

    for (quint32 i = 0; i < obj.nInlineComponents; ++i)
        data.inlineComponents.append(extractData(obj.inlineComponentTable()[i]));

    for (quint32 i = 0; i < obj.nRequiredPropertyExtraData; ++i)
        data.requiredPropertyExtraData.append(extractData(obj.requiredPropertyExtraDataTable()[i]));

    return data;
}

static UnitHunk extractData(const Unit &unit)
{
    return unit;
}

static ClassHunk extractData(const Class &cls)
{
    ClassHunk data;
    data.data = cls;
    const Method *methods = cls.methodTable();
    for (quint32 i = 0, end = cls.nStaticMethods + cls.nMethods; i < end; ++i)
        data.methods.append(methods[i]);
    return data;
}

static TemplateObjectHunk extractData(const TemplateObject &to)
{
    TemplateObjectHunk data;
    data.size = to.size;
    const quint32_le *strings = reinterpret_cast<const quint32_le *>(&to + 1);
    for (quint32 i = 0; i < 2u * to.size; ++i)
        data.strings.append(strings[i]);
    return data;
}

static JsClassHunk extractData(const JSClass &jsClass)
{
    JsClassHunk data;
    data.nMembers = jsClass.nMembers;
    const JSClassMember *members = reinterpret_cast<const JSClassMember *>(&jsClass + 1);
    for (quint32 i = 0; i < jsClass.nMembers; ++i)
        data.members.append(members[i]);
    return data;
}

static BlockHunk extractData(const Block &block)
{
    BlockHunk data;
    data.data = block;
    for (quint32 i = 0; i < block.nLocals; ++i)
        data.locals.append(block.localsTable()[i]);
    return data;
}

// location extraction

template <typename T, typename L = void>
struct LocationExtractor
{
    static bool extract(const T &) { return false; }
};

template <typename T>
struct LocationExtractor<T, std::enable_if_t<std::is_same_v<decltype(T::location), Location>>>
{
    static Location extract(const T &t) { return t.location; }
};

template <>
struct LocationExtractor<Binding, void>
{
    static std::pair<Location, Location> extract(const Binding &binding)
    {
        return { binding.location, binding.valueLocation };
    }
};

template <>
struct LocationExtractor<Alias, void>
{
    static std::pair<Location, Location> extract(const Alias &alias)
    {
        return { alias.location(), alias.referenceLocation() };
    }
};

template <>
struct LocationExtractor<Object, void>
{
    static std::pair<Location, Location> extract(const Object &object)
    {
        return { object.location, object.locationOfIdProperty };
    }
};

template <typename ElementT>
struct ComparisonTraits;

#define COMPARISON_TRAITS(Element) \
template <> \
struct ComparisonTraits<Element> \
{ \
    static constexpr ChangeType Changed = ChangeType::Element ## Changed; \
    static constexpr ChangeType LocationChanged = ChangeType::None; \
    static constexpr ChangeType Removed = ChangeType::Element ## Removed; \
    static constexpr ChangeType Added = ChangeType::Element ## Added; \
};


#define COMPARISON_TRAITS_WITH_LOCATION(Element) \
template <> \
struct ComparisonTraits<Element> \
{ \
    static constexpr ChangeType Changed = ChangeType::Element ## Changed; \
    static constexpr ChangeType LocationChanged = ChangeType::Element ## LocationChanged; \
    static constexpr ChangeType Removed = ChangeType::Element ## Removed; \
    static constexpr ChangeType Added = ChangeType::Element ## Added; \
};

COMPARISON_TRAITS_WITH_LOCATION(Object)
COMPARISON_TRAITS_WITH_LOCATION(Property)
COMPARISON_TRAITS_WITH_LOCATION(Alias)
COMPARISON_TRAITS_WITH_LOCATION(Enum)
COMPARISON_TRAITS_WITH_LOCATION(Signal)
COMPARISON_TRAITS_WITH_LOCATION(InlineComponent)
COMPARISON_TRAITS_WITH_LOCATION(Binding)
COMPARISON_TRAITS_WITH_LOCATION(Import)
COMPARISON_TRAITS_WITH_LOCATION(Function)

COMPARISON_TRAITS(RequiredPropertyExtraData)
COMPARISON_TRAITS(Constant)
COMPARISON_TRAITS(Lookup)
COMPARISON_TRAITS(RegExp)
COMPARISON_TRAITS(Class)
COMPARISON_TRAITS(TemplateObject)
COMPARISON_TRAITS(JSClass)
COMPARISON_TRAITS(Block)
COMPARISON_TRAITS(TranslationData)
COMPARISON_TRAITS(StringData)

template <typename AccessorFn>
struct IndexedElements
{
    AccessorFn accessor;
    quint32 count;
};

template <typename AccessorFn>
IndexedElements(AccessorFn, quint32) -> IndexedElements<AccessorFn>;

struct UnitDiffer
{
    const Unit *const oldUnit;
    const Unit *const newUnit;
    QList<Change> changes;
    int m_currentObjectIndex = -1;

    template <typename ElementT, typename OldSource, typename NewSource, typename ContentEqFn>
    void compareIndexedElements(OldSource oldSource, NewSource newSource, ContentEqFn contentEq)
    {
        using Traits = ComparisonTraits<ElementT>;
        const auto &oldAccessor = oldSource.accessor;
        const auto &newAccessor = newSource.accessor;
        const quint32 oldCount = oldSource.count;
        const quint32 newCount = newSource.count;
        quint32 commonCount = qMin(oldCount, newCount);

        // Compare elements at common positions
        for (quint32 i = 0; i < commonCount; ++i) {
            const ElementT &oldElem = oldAccessor(i);
            const ElementT &newElem = newAccessor(i);

            if (!contentEq(oldElem, newElem)) {
                Change change;
                change.objectIndex = m_currentObjectIndex;
                change.index = i;
                change.type = Traits::Changed;
                change.data = extractData(newElem);
                changes.append(change);
            } else if (LocationExtractor<ElementT>::extract(oldElem)
                       != LocationExtractor<ElementT>::extract(newElem)) {
                Change change;
                change.objectIndex = m_currentObjectIndex;
                change.index = i;
                change.type = Traits::LocationChanged;
                change.data = extractData(newElem);
                changes.append(change);
            }
        }

        // Emit Removed for extra elements in old
        for (quint32 i = commonCount; i < oldCount; ++i) {
            Change change;
            change.type = Traits::Removed;
            change.objectIndex = m_currentObjectIndex;
            change.index = i;
            changes.append(change);
        }

        // Emit Added for extra elements in new
        for (quint32 i = commonCount; i < newCount; ++i) {
            Change change;
            change.type = Traits::Added;
            change.objectIndex = m_currentObjectIndex;
            change.index = i;
            change.data = extractData(newAccessor(i));
            changes.append(change);
        }
    }

    bool stringsEqual(uint oldName, uint newName) const
    {
        Q_ASSERT(oldName < oldUnit->stringTableSize);
        Q_ASSERT(newName < newUnit->stringTableSize);
        return oldName == newName
                && oldUnit->stringAtInternal(oldName) == newUnit->stringAtInternal(newName);
    }

    bool unitMetadataContentEqual() const
    {
        if (std::memcmp(oldUnit->magic, newUnit->magic, sizeof(oldUnit->magic)) != 0)
            return false;

        if (oldUnit->version != newUnit->version || oldUnit->reserved != newUnit->reserved
            || oldUnit->sourceTimeStamp != newUnit->sourceTimeStamp
            || oldUnit->unitSize != newUnit->unitSize) {
            return false;
        }

        // sourceChecksum lives before md5Checksum and is therefore not covered by it.
        if (std::memcmp(oldUnit->sourceChecksum, newUnit->sourceChecksum,
                        sizeof(oldUnit->sourceChecksum))
            != 0) {
            return false;
        }

        // If anything below the checksum has changed, the checksum itself has changed, too
        return std::memcmp(oldUnit->md5Checksum, newUnit->md5Checksum, sizeof(oldUnit->md5Checksum))
                == 0;
    }

    bool objectContentEqual(const Object &oldObj, const Object &newObj) const
    {
        // Keep in sync with the Object struct in qv4compileddata_p.h
        static_assert(sizeof(Object) == 84, "Update objectContentEqual when Object layout changes");
        if (oldObj.nNamedObjectsInComponent != newObj.nNamedObjectsInComponent)
            return false;

        const quint32_le *oldNamed = oldObj.namedObjectsInComponentTable();
        const quint32_le *newNamed = newObj.namedObjectsInComponentTable();
        for (quint32 i = 0; i < oldObj.nNamedObjectsInComponent; ++i) {
            if (oldNamed[i] != newNamed[i])
                return false;
        }

        if (oldObj.nFunctions != newObj.nFunctions)
            return false;

        const quint32_le *oldFuncs = oldObj.functionOffsetTable();
        const quint32_le *newFuncs = newObj.functionOffsetTable();
        for (quint32 i = 0; i < oldObj.nFunctions; ++i) {
            if (oldFuncs[i] != newFuncs[i])
                return false;
            if (!functionContentEqual(*oldUnit->functionAt(oldFuncs[i]),
                                      *newUnit->functionAt(newFuncs[i]))) {
                return false;
            }
        }

        return stringsEqual(oldObj.inheritedTypeNameIndex, newObj.inheritedTypeNameIndex)
                && stringsEqual(oldObj.idNameIndex, newObj.idNameIndex)
                && oldObj.flags() == newObj.flags()
                && oldObj.hasAliasAsDefaultProperty() == newObj.hasAliasAsDefaultProperty()
                && oldObj.objectId() == newObj.objectId()
                && oldObj.indexOfDefaultPropertyOrAlias == newObj.indexOfDefaultPropertyOrAlias;
    }

    bool propertyContentEqual(const Property &oldProp, const Property &newProp) const
    {
        // Keep in sync with the Property struct in qv4compileddata_p.h
        static_assert(sizeof(Property) == 12, "Update propertyContentEqual when Property layout changes");
        return stringsEqual(oldProp.nameIndex(), newProp.nameIndex())
                && (oldProp.isCommonType() == newProp.isCommonType()
                    && (oldProp.isCommonType()
                                ? oldProp.commonTypeOrTypeNameIndex()
                                        == newProp.commonTypeOrTypeNameIndex()
                                : stringsEqual(oldProp.typeNameIndex(), newProp.typeNameIndex())))
                && oldProp.isReadOnly() == newProp.isReadOnly()
                && oldProp.isRequired() == newProp.isRequired()
                && oldProp.isList() == newProp.isList()
                && oldProp.isVirtual() == newProp.isVirtual()
                && oldProp.isOverride() == newProp.isOverride()
                && oldProp.isFinal() == newProp.isFinal();
    }

    // In addition to structural equality, also compare the actual name strings: the integer
    // nameIndex may be the same while the string at that index changed (property rename).
    bool aliasContentEqual(const Alias &oldAlias, const Alias &newAlias) const
    {
        // Keep in sync with the Alias struct in qv4compileddata_p.h
        static_assert(sizeof(Alias) == 20, "Update aliasContentEqual when Alias layout changes");
        return stringsEqual(oldAlias.idIndex(), newAlias.idIndex())
                && stringsEqual(oldAlias.nameIndex(), newAlias.nameIndex())
                && stringsEqual(oldAlias.propertyNameIndex(), newAlias.propertyNameIndex())
                && oldAlias.isReadOnly() == newAlias.isReadOnly();
    }

    bool enumContentEqual(const Enum &oldEnum, const Enum &newEnum) const
    {
        // Keep in sync with the Enum and EnumValue structs in qv4compileddata_p.h
        static_assert(sizeof(Enum) == 12, "Update enumContentEqual when Enum layout changes");
        static_assert(sizeof(EnumValue) == 8, "Update enumContentEqual when EnumValue layout changes");
        if (!stringsEqual(oldEnum.nameIndex, newEnum.nameIndex)
            || oldEnum.nEnumValues != newEnum.nEnumValues) {
            return false;
        }

        for (quint32 i = 0; i < oldEnum.nEnumValues; ++i) {
            const EnumValue *oldValue = oldEnum.enumValueAt(i);
            const EnumValue *newValue = newEnum.enumValueAt(i);
            if (oldValue->value != newValue->value
                || !stringsEqual(oldValue->nameIndex, newValue->nameIndex)) {
                return false;
            }
        }

        return true;
    }

    bool signalContentEqual(const Signal &oldSig, const Signal &newSig) const
    {
        // Keep in sync with the Signal and Parameter structs in qv4compileddata_p.h
        static_assert(sizeof(Signal) == 12, "Update signalContentEqual when Signal layout changes");
        static_assert(sizeof(Parameter) == 8, "Update signalContentEqual when Parameter layout changes");
        if (!stringsEqual(oldSig.nameIndex, newSig.nameIndex))
            return false;
        if (oldSig.nParameters != newSig.nParameters)
            return false;
        for (quint32 i = 0; i < oldSig.nParameters; ++i) {
            const Parameter *oldP = oldSig.parameterAt(i);
            const Parameter *newP = newSig.parameterAt(i);
            if (!stringsEqual(oldP->nameIndex, newP->nameIndex)
                || !parameterTypeContentEqual(oldP->type, newP->type)) {
                return false;
            }
        }
        return true;
    }

    bool bindingContentEqual(const Binding &oldBinding, const Binding &newBinding) const
    {
        // Keep in sync with the Binding and TranslationData structs in qv4compileddata_p.h
        static_assert(sizeof(Binding) == 24, "Update bindingContentEqual when Binding layout changes");
        static_assert(sizeof(TranslationData) == 16, "Update bindingContentEqual when TranslationData layout changes");
        if (oldBinding.type() != newBinding.type() || oldBinding.flags() != newBinding.flags())
            return false;

        if (!stringsEqual(oldBinding.propertyNameIndex, newBinding.propertyNameIndex))
            return false;

        switch (oldBinding.type()) {
        case Binding::Type_Invalid:
            return true;
        case Binding::Type_Boolean:
            return oldBinding.valueAsBoolean() == newBinding.valueAsBoolean();
        case Binding::Type_Number: {
            if (oldBinding.hasFlag(CompiledData::Binding::IsResolvedEnum))
                return oldBinding.value.resolvedEnumValue == newBinding.value.resolvedEnumValue;
            const uint oldIdx = oldBinding.value.constantValueIndex;
            const uint newIdx = newBinding.value.constantValueIndex;
            Q_ASSERT(oldIdx < oldUnit->constantTableSize);
            Q_ASSERT(newIdx < newUnit->constantTableSize);
            return oldIdx == newIdx && oldUnit->constants()[oldIdx] == newUnit->constants()[newIdx];
        }
        case Binding::Type_String:
            return stringsEqual(oldBinding.stringIndex, newBinding.stringIndex);
        case Binding::Type_Null:
            return true;
        case Binding::Type_Translation:
        case Binding::Type_TranslationById: {
            const auto &oldTrans = oldUnit->translations()[oldBinding.value.translationDataIndex];
            const auto &newTrans = newUnit->translations()[newBinding.value.translationDataIndex];

            // contextIndex uses NoContextIndex as sentinel; compare by
            // resolved string only when both are valid indices.
            const bool oldHasCtx = oldTrans.contextIndex != TranslationData::NoContextIndex;
            const bool newHasCtx = newTrans.contextIndex != TranslationData::NoContextIndex;

            if (oldHasCtx != newHasCtx)
                return false;

            if (oldHasCtx && !stringsEqual(oldTrans.contextIndex, newTrans.contextIndex))
                return false;

            return stringsEqual(oldBinding.stringIndex, newBinding.stringIndex)
                    && stringsEqual(oldTrans.stringIndex, newTrans.stringIndex)
                    && stringsEqual(oldTrans.commentIndex, newTrans.commentIndex)
                    && oldTrans.number == newTrans.number;
        }
        case Binding::Type_Script: {
            if (oldBinding.value.compiledScriptIndex != newBinding.value.compiledScriptIndex)
                return false;
            const auto *oldFunc = oldUnit->functionAt(oldBinding.value.compiledScriptIndex);
            const auto *newFunc = newUnit->functionAt(newBinding.value.compiledScriptIndex);
            return oldFunc->nFormals == newFunc->nFormals && oldFunc->codeSize == newFunc->codeSize
                    && std::memcmp(oldFunc->code(), newFunc->code(), oldFunc->codeSize) == 0;
        }
        case Binding::Type_Object:
        case Binding::Type_AttachedProperty:
        case Binding::Type_GroupProperty:
            return oldBinding.value.objectIndex == newBinding.value.objectIndex
                    && objectContentEqual(
                            *oldUnit->qmlUnit()->objectAt(oldBinding.value.objectIndex),
                            *newUnit->qmlUnit()->objectAt(newBinding.value.objectIndex));
        }

        return false;
    }

    bool parameterTypeContentEqual(const ParameterType &oldType, const ParameterType &newType) const
    {
        // Keep in sync with the ParameterType struct in qv4compileddata_p.h
        static_assert(sizeof(ParameterType) == 4, "Update parameterTypeContentEqual when ParameterType layout changes");
        const bool indexIsCommonType = oldType.indexIsCommonType();
        if (newType.indexIsCommonType() != indexIsCommonType
            || oldType.isList() != newType.isList()) {
            return false;
        }

        return indexIsCommonType
                ? oldType.typeNameIndexOrCommonType() == newType.typeNameIndexOrCommonType()
                : stringsEqual(oldType.typeNameIndexOrCommonType(),
                               newType.typeNameIndexOrCommonType());
    }

    bool functionContentEqual(const Function &oldFunc, const Function &newFunc) const
    {
        // Keep in sync with the Function struct in qv4compileddata_p.h
        static_assert(sizeof(Function) == 56, "Update functionContentEqual when Function layout changes");
        if (oldFunc.nestedFunctionIndex != newFunc.nestedFunctionIndex
            || !stringsEqual(oldFunc.nameIndex, newFunc.nameIndex) || oldFunc.flags != newFunc.flags
            || oldFunc.nFormals != newFunc.nFormals || oldFunc.codeSize != newFunc.codeSize
            || !parameterTypeContentEqual(oldFunc.returnType, newFunc.returnType)) {
            return false;
        }

        for (int i = 0, end = oldFunc.nFormals; i != end; ++i) {
            const auto &oldFormal = oldFunc.formalsTable()[i];
            const auto &newFormal = newFunc.formalsTable()[i];
            if (!parameterTypeContentEqual(oldFormal.type, newFormal.type))
                return false;
            if (!stringsEqual(oldFormal.nameIndex, newFormal.nameIndex))
                return false;
        }

        return std::memcmp(oldFunc.code(), newFunc.code(), oldFunc.codeSize) == 0;
    }

    bool translationContentEqual(const TranslationData &oldTrans,
                                 const TranslationData &newTrans) const
    {
        // Keep in sync with the TranslationData struct in qv4compileddata_p.h
        static_assert(sizeof(TranslationData) == 16, "Update translationContentEqual when TranslationData layout changes");
        if (oldTrans.contextIndex != newTrans.contextIndex)
            return false;
        if (oldTrans.contextIndex != TranslationData::NoContextIndex
            && !stringsEqual(oldTrans.contextIndex, newTrans.contextIndex)) {
            return false;
        }
        return stringsEqual(oldTrans.stringIndex, newTrans.stringIndex)
                && stringsEqual(oldTrans.commentIndex, newTrans.commentIndex)
                && oldTrans.number == newTrans.number;
    }

    bool classContentEqual(const Class &oldCls, const Class &newCls) const
    {
        // Keep in sync with the Class and Method structs in qv4compileddata_p.h
        static_assert(sizeof(Class) == 24, "Update classContentEqual when Class layout changes");
        static_assert(sizeof(Method) == 12, "Update classContentEqual when Method layout changes");
        if (!stringsEqual(oldCls.nameIndex, newCls.nameIndex)
            || oldCls.scopeIndex != newCls.scopeIndex
            || oldCls.constructorFunction != newCls.constructorFunction
            || oldCls.nStaticMethods != newCls.nStaticMethods
            || oldCls.nMethods != newCls.nMethods) {
            return false;
        }
        const Method *oldMethods = oldCls.methodTable();
        const Method *newMethods = newCls.methodTable();
        for (quint32 i = 0, end = oldCls.nStaticMethods + oldCls.nMethods; i < end; ++i) {
            if (!stringsEqual(oldMethods[i].name, newMethods[i].name)
                || oldMethods[i].type != newMethods[i].type
                || oldMethods[i].function != newMethods[i].function) {
                return false;
            }
        }
        return true;
    }

    bool templateObjectContentEqual(const TemplateObject &oldTO, const TemplateObject &newTO) const
    {
        // Keep in sync with the TemplateObject struct in qv4compileddata_p.h
        static_assert(sizeof(TemplateObject) == 4, "Update templateObjectContentEqual when TemplateObject layout changes");
        if (oldTO.size != newTO.size)
            return false;
        const quint32_le *oldStrings = reinterpret_cast<const quint32_le *>(&oldTO + 1);
        const quint32_le *newStrings = reinterpret_cast<const quint32_le *>(&newTO + 1);
        for (quint32 i = 0; i < 2u * oldTO.size; ++i) {
            if (!stringsEqual(oldStrings[i], newStrings[i]))
                return false;
        }
        return true;
    }

    bool jsClassContentEqual(const JSClass &oldJC, const JSClass &newJC) const
    {
        // Keep in sync with the JSClass and JSClassMember structs in qv4compileddata_p.h
        static_assert(sizeof(JSClass) == 4, "Update jsClassContentEqual when JSClass layout changes");
        static_assert(sizeof(JSClassMember) == 4, "Update jsClassContentEqual when JSClassMember layout changes");
        if (oldJC.nMembers != newJC.nMembers)
            return false;
        const JSClassMember *oldMembers = reinterpret_cast<const JSClassMember *>(&oldJC + 1);
        const JSClassMember *newMembers = reinterpret_cast<const JSClassMember *>(&newJC + 1);
        for (quint32 i = 0; i < oldJC.nMembers; ++i) {
            if (!stringsEqual(oldMembers[i].nameOffset(), newMembers[i].nameOffset())
                || oldMembers[i].isAccessor() != newMembers[i].isAccessor()) {
                return false;
            }
        }
        return true;
    }

    bool blockContentEqual(const Block &oldBlock, const Block &newBlock) const
    {
        // Keep in sync with the Block struct in qv4compileddata_p.h
        static_assert(sizeof(Block) == 12, "Update blockContentEqual when Block layout changes");
        if (oldBlock.nLocals != newBlock.nLocals
            || oldBlock.sizeOfLocalTemporalDeadZone != newBlock.sizeOfLocalTemporalDeadZone) {
            return false;
        }
        for (quint32 i = 0; i < oldBlock.nLocals; ++i) {
            if (!stringsEqual(oldBlock.localsTable()[i], newBlock.localsTable()[i]))
                return false;
        }
        return true;
    }

    // Per-object comparison methods

    void compareObjectProperties(const Object *oldObj, const Object *newObj)
    {
        const Property *oldProps = oldObj->propertyTable();
        const Property *newProps = newObj->propertyTable();
        compareIndexedElements<Property>(
                IndexedElements{ [oldProps](quint32 i) { return oldProps[i]; },
                                 oldObj->nProperties },
                IndexedElements{ [newProps](quint32 i) { return newProps[i]; },
                                 newObj->nProperties },
                [this](const Property &a, const Property &b) {
                    return propertyContentEqual(a, b);
                });
    }

    void compareObjectAliases(const Object *oldObj, const Object *newObj)
    {
        const Alias *oldAliases = oldObj->aliasesBegin();
        const Alias *newAliases = newObj->aliasesBegin();
        compareIndexedElements<Alias>(
                IndexedElements{ [oldAliases](quint32 i) { return oldAliases[i]; },
                                 oldObj->nAliases },
                IndexedElements{ [newAliases](quint32 i) { return newAliases[i]; },
                                 newObj->nAliases },
                [this](const Alias &a, const Alias &b) { return aliasContentEqual(a, b); });
    }

    void compareObjectEnums(const Object *oldObj, const Object *newObj)
    {
        compareIndexedElements<Enum>(
                IndexedElements{ [oldObj](quint32 i) -> const Enum & { return *oldObj->enumAt(i); },
                                 oldObj->nEnums },
                IndexedElements{ [newObj](quint32 i) -> const Enum & { return *newObj->enumAt(i); },
                                 newObj->nEnums },
                [this](const Enum &a, const Enum &b) { return enumContentEqual(a, b); });
    }

    void compareObjectSignals(const Object *oldObj, const Object *newObj)
    {
        compareIndexedElements<Signal>(
                IndexedElements{
                        [oldObj](quint32 i) -> const Signal & { return *oldObj->signalAt(i); },
                        oldObj->nSignals },
                IndexedElements{
                        [newObj](quint32 i) -> const Signal & { return *newObj->signalAt(i); },
                        newObj->nSignals },
                [this](const Signal &a, const Signal &b) { return signalContentEqual(a, b); });
    }

    void compareObjectInlineComponents(const Object *oldObj, const Object *newObj)
    {
        const InlineComponent *oldICs = oldObj->inlineComponentTable();
        const InlineComponent *newICs = newObj->inlineComponentTable();
        compareIndexedElements<InlineComponent>(
                IndexedElements{ [oldICs](quint32 i) { return oldICs[i]; },
                                 oldObj->nInlineComponents },
                IndexedElements{ [newICs](quint32 i) { return newICs[i]; },
                                 newObj->nInlineComponents },
                [this](const InlineComponent &a, const InlineComponent &b) {
                    return a.objectIndex == b.objectIndex && stringsEqual(a.nameIndex, b.nameIndex);
                });
    }

    void compareObjectRequiredPropertyExtraData(const Object *oldObj, const Object *newObj)
    {
        const RequiredPropertyExtraData *oldRPED = oldObj->requiredPropertyExtraDataTable();
        const RequiredPropertyExtraData *newRPED = newObj->requiredPropertyExtraDataTable();
        compareIndexedElements<RequiredPropertyExtraData>(
                IndexedElements{ [oldRPED](quint32 i) { return oldRPED[i]; },
                                 oldObj->nRequiredPropertyExtraData },
                IndexedElements{ [newRPED](quint32 i) { return newRPED[i]; },
                                 newObj->nRequiredPropertyExtraData },
                [this](const RequiredPropertyExtraData &a, const RequiredPropertyExtraData &b) {
                    return stringsEqual(a.nameIndex, b.nameIndex);
                });
    }

    void compareObjectBindings(const Object *oldObj, const Object *newObj)
    {
        const Binding *oldBindings = oldObj->bindingTable();
        const Binding *newBindings = newObj->bindingTable();
        compareIndexedElements<Binding>(
                IndexedElements{ [oldBindings](quint32 i) { return oldBindings[i]; },
                                 oldObj->nBindings },
                IndexedElements{ [newBindings](quint32 i) { return newBindings[i]; },
                                 newObj->nBindings },
                [this](const Binding &a, const Binding &b) { return bindingContentEqual(a, b); });
    }

    CompilationUnitDiff diff()
    {
        const QmlUnit *oldQml = oldUnit->qmlUnit();
        const QmlUnit *newQml = newUnit->qmlUnit();

        if (!oldQml || !newQml)
            return CompilationUnitDiff();

        // Check for unit metadata changes (source file, timestamp, checksum, etc.)
        if (!unitMetadataContentEqual()) {
            Change change;
            change.type = ChangeType::UnitMetadataChanged;
            change.data = extractData(*newUnit);
            changes.append(change);
        }

        compareIndexedElements<Object>(
                IndexedElements{
                        [oldQml](quint32 i) -> const Object & { return *oldQml->objectAt(i); },
                        oldQml->nObjects },
                IndexedElements{
                        [newQml](quint32 i) -> const Object & { return *newQml->objectAt(i); },
                        newQml->nObjects },
                [this](const Object &a, const Object &b) { return objectContentEqual(a, b); });

        // Compare details of overlapping objects only
        const quint32 commonCount = std::min(oldQml->nObjects, newQml->nObjects);
        for (quint32 objectIndex = 0; objectIndex < commonCount; ++objectIndex) {
            QScopedValueRollback guard(m_currentObjectIndex, static_cast<int>(objectIndex));
            const Object *oldObj = oldQml->objectAt(objectIndex);
            const Object *newObj = newQml->objectAt(objectIndex);

            compareObjectProperties(oldObj, newObj);
            compareObjectAliases(oldObj, newObj);
            compareObjectEnums(oldObj, newObj);
            compareObjectSignals(oldObj, newObj);
            compareObjectInlineComponents(oldObj, newObj);
            compareObjectRequiredPropertyExtraData(oldObj, newObj);
            compareObjectBindings(oldObj, newObj);
        }

        compareIndexedElements<quint64_le>(
                IndexedElements{ [this](quint32 i) { return oldUnit->constants()[i]; },
                                 oldUnit->constantTableSize },
                IndexedElements{ [this](quint32 i) { return newUnit->constants()[i]; },
                                 newUnit->constantTableSize },
                [](const quint64_le &a, const quint64_le &b) { return a == b; });

        compareIndexedElements<QString>(
                IndexedElements{ [this](quint32 i) { return oldUnit->stringAtInternal(i); },
                                 oldUnit->stringTableSize },
                IndexedElements{ [this](quint32 i) { return newUnit->stringAtInternal(i); },
                                 newUnit->stringTableSize },
                [](const QString &a, const QString &b) { return a == b; });

        compareIndexedElements<Lookup>(
                IndexedElements{ [this](quint32 i) { return oldUnit->lookupTable()[i]; },
                                 oldUnit->lookupTableSize },
                IndexedElements{ [this](quint32 i) { return newUnit->lookupTable()[i]; },
                                 newUnit->lookupTableSize },
                [this](const Lookup &a, const Lookup &b) {
                    return a.type() == b.type() && a.mode() == b.mode()
                            && stringsEqual(a.nameIndex(), b.nameIndex());
                });

        compareIndexedElements<Import>(
                IndexedElements{ [oldQml](quint32 i) { return *oldQml->importAt(i); },
                                 oldQml->nImports },
                IndexedElements{ [newQml](quint32 i) { return *newQml->importAt(i); },
                                 newQml->nImports },
                [this](const Import &a, const Import &b) {
                    return a.type == b.type && stringsEqual(a.uriIndex, b.uriIndex)
                            && stringsEqual(a.qualifierIndex, b.qualifierIndex)
                            && a.version == b.version;
                });

        compareIndexedElements<Function>(IndexedElements{ [this](quint32 i) -> const Function & {
                                                             return *oldUnit->functionAt(i);
                                                         },
                                                          oldUnit->functionTableSize },
                                         IndexedElements{ [this](quint32 i) -> const Function & {
                                                             return *newUnit->functionAt(i);
                                                         },
                                                          newUnit->functionTableSize },
                                         [this](const Function &a, const Function &b) {
                                             return functionContentEqual(a, b);
                                         });

        compareIndexedElements<TranslationData>(
                IndexedElements{ [this](quint32 i) { return oldUnit->translations()[i]; },
                                 oldUnit->translationTableSize },
                IndexedElements{ [this](quint32 i) { return newUnit->translations()[i]; },
                                 newUnit->translationTableSize },
                [this](const TranslationData &a, const TranslationData &b) {
                    return translationContentEqual(a, b);
                });

        compareIndexedElements<RegExp>(
                IndexedElements{
                        [this](quint32 i) -> const RegExp & { return *oldUnit->regexpAt(i); },
                        oldUnit->regexpTableSize },
                IndexedElements{
                        [this](quint32 i) -> const RegExp & { return *newUnit->regexpAt(i); },
                        newUnit->regexpTableSize },
                [this](const RegExp &a, const RegExp &b) {
                    return a.flags() == b.flags() && stringsEqual(a.stringIndex(), b.stringIndex());
                });

        compareIndexedElements<Class>(
                IndexedElements{
                        [this](quint32 i) -> const Class & { return *oldUnit->classAt(i); },
                        oldUnit->classTableSize },
                IndexedElements{
                        [this](quint32 i) -> const Class & { return *newUnit->classAt(i); },
                        newUnit->classTableSize },
                [this](const Class &a, const Class &b) { return classContentEqual(a, b); });

        compareIndexedElements<TemplateObject>(
                IndexedElements{ [this](quint32 i) -> const TemplateObject & {
                                    return *oldUnit->templateObjectAt(i);
                                },
                                 oldUnit->templateObjectTableSize },
                IndexedElements{ [this](quint32 i) -> const TemplateObject & {
                                    return *newUnit->templateObjectAt(i);
                                },
                                 newUnit->templateObjectTableSize },
                [this](const TemplateObject &a, const TemplateObject &b) {
                    return templateObjectContentEqual(a, b);
                });

        {
            auto jsClassFromUnit = [](const Unit *unit, quint32 idx) -> const JSClass & {
                const quint32_le *offsetTable = reinterpret_cast<const quint32_le *>(
                        reinterpret_cast<const char *>(unit) + unit->offsetToJSClassTable);
                return *reinterpret_cast<const JSClass *>(reinterpret_cast<const char *>(unit)
                                                          + offsetTable[idx]);
            };
            compareIndexedElements<JSClass>(
                    IndexedElements{ [this, jsClassFromUnit](quint32 i) -> const JSClass & {
                                        return jsClassFromUnit(oldUnit, i);
                                    },
                                     oldUnit->jsClassTableSize },
                    IndexedElements{ [this, jsClassFromUnit](quint32 i) -> const JSClass & {
                                        return jsClassFromUnit(newUnit, i);
                                    },
                                     newUnit->jsClassTableSize },
                    [this](const JSClass &a, const JSClass &b) {
                        return jsClassContentEqual(a, b);
                    });
        }

        compareIndexedElements<Block>(
                IndexedElements{
                        [this](quint32 i) -> const Block & { return *oldUnit->blockAt(i); },
                        oldUnit->blockTableSize },
                IndexedElements{
                        [this](quint32 i) -> const Block & { return *newUnit->blockAt(i); },
                        newUnit->blockTableSize },
                [this](const Block &a, const Block &b) { return blockContentEqual(a, b); });

        return CompilationUnitDiff{ std::move(changes), true };
    }
};

// NB: AddObject and RemoveObject are mutually exclusive since all objects are
//     in a global order per compilation unit. The number of objects can either
//     grow or shrink, but not both. Likewise AddBinding and RemoveBinding, but
//     per object. Before adding bindings, all the objects need to be present.
//     It's a good idea to remove stale bindings before they can cause any
//     trouble. Therefore, we do that first, before changing or adding bindings.
enum Severity : quint8 {
    Replace,
    Rebuild,
    AddObject,
    RemoveBinding,
    ChangeBinding,
    AddBinding,
    RemoveObject,
    Ignore,
};

static Severity classifyChange(const Change &change)
{
    switch (change.type) {
    case QV4::CompiledData::ChangeType::UnitMetadataChanged:
    case QV4::CompiledData::ChangeType::ObjectChanged:
    case QV4::CompiledData::ChangeType::ImportAdded:
    case QV4::CompiledData::ChangeType::ImportRemoved:
    case QV4::CompiledData::ChangeType::ImportChanged:
    case QV4::CompiledData::ChangeType::InlineComponentAdded:
    case QV4::CompiledData::ChangeType::InlineComponentRemoved:
    case QV4::CompiledData::ChangeType::InlineComponentChanged:
    case QV4::CompiledData::ChangeType::Unknown:
        return Replace;
    case QV4::CompiledData::ChangeType::PropertyAdded:
    case QV4::CompiledData::ChangeType::PropertyRemoved:
    case QV4::CompiledData::ChangeType::PropertyChanged:
    case QV4::CompiledData::ChangeType::AliasAdded:
    case QV4::CompiledData::ChangeType::AliasRemoved:
    case QV4::CompiledData::ChangeType::AliasChanged:
    case QV4::CompiledData::ChangeType::FunctionAdded:
    case QV4::CompiledData::ChangeType::FunctionRemoved:
    case QV4::CompiledData::ChangeType::FunctionChanged:
    case QV4::CompiledData::ChangeType::EnumAdded:
    case QV4::CompiledData::ChangeType::EnumRemoved:
    case QV4::CompiledData::ChangeType::EnumChanged:
    case QV4::CompiledData::ChangeType::SignalAdded:
    case QV4::CompiledData::ChangeType::SignalRemoved:
    case QV4::CompiledData::ChangeType::SignalChanged:
    case QV4::CompiledData::ChangeType::RequiredPropertyExtraDataAdded:
    case QV4::CompiledData::ChangeType::RequiredPropertyExtraDataRemoved:
    case QV4::CompiledData::ChangeType::RequiredPropertyExtraDataChanged:
        return Rebuild;
    case QV4::CompiledData::ChangeType::ObjectAdded:
        return AddObject;
    case QV4::CompiledData::ChangeType::BindingRemoved:
        return RemoveBinding;
    case QV4::CompiledData::ChangeType::BindingChanged:
        return ChangeBinding;
    case QV4::CompiledData::ChangeType::BindingAdded:
        return AddBinding;
    case QV4::CompiledData::ChangeType::ObjectRemoved:
        return RemoveObject;
    case QV4::CompiledData::ChangeType::AliasLocationChanged:
    case QV4::CompiledData::ChangeType::BindingLocationChanged:
    case QV4::CompiledData::ChangeType::EnumLocationChanged:
    case QV4::CompiledData::ChangeType::FunctionLocationChanged:
    case QV4::CompiledData::ChangeType::ImportLocationChanged:
    case QV4::CompiledData::ChangeType::InlineComponentLocationChanged:
    case QV4::CompiledData::ChangeType::ObjectLocationChanged:
    case QV4::CompiledData::ChangeType::PropertyLocationChanged:
    case QV4::CompiledData::ChangeType::SignalLocationChanged:
        return Ignore;
    case QV4::CompiledData::ChangeType::ConstantAdded:
    case QV4::CompiledData::ChangeType::ConstantRemoved:
    case QV4::CompiledData::ChangeType::ConstantChanged:
    case QV4::CompiledData::ChangeType::StringDataAdded:
    case QV4::CompiledData::ChangeType::StringDataRemoved:
    case QV4::CompiledData::ChangeType::StringDataChanged:
    case QV4::CompiledData::ChangeType::LookupAdded:
    case QV4::CompiledData::ChangeType::LookupRemoved:
    case QV4::CompiledData::ChangeType::LookupChanged:
    case QV4::CompiledData::ChangeType::RegExpAdded:
    case QV4::CompiledData::ChangeType::RegExpRemoved:
    case QV4::CompiledData::ChangeType::RegExpChanged:
    case QV4::CompiledData::ChangeType::ClassAdded:
    case QV4::CompiledData::ChangeType::ClassRemoved:
    case QV4::CompiledData::ChangeType::ClassChanged:
    case QV4::CompiledData::ChangeType::TemplateObjectAdded:
    case QV4::CompiledData::ChangeType::TemplateObjectRemoved:
    case QV4::CompiledData::ChangeType::TemplateObjectChanged:
    case QV4::CompiledData::ChangeType::JSClassAdded:
    case QV4::CompiledData::ChangeType::JSClassRemoved:
    case QV4::CompiledData::ChangeType::JSClassChanged:
    case QV4::CompiledData::ChangeType::BlockAdded:
    case QV4::CompiledData::ChangeType::BlockRemoved:
    case QV4::CompiledData::ChangeType::BlockChanged:
    case QV4::CompiledData::ChangeType::TranslationDataAdded:
    case QV4::CompiledData::ChangeType::TranslationDataRemoved:
    case QV4::CompiledData::ChangeType::TranslationDataChanged:
    case QV4::CompiledData::ChangeType::None:
        break;
    }
    return Ignore;
}

static void sortChanges(QSpan<Change> changes)
{
    std::stable_sort(changes.begin(), changes.end(), [](const Change &a, const Change &b) {
        return quint8(classifyChange(a)) < quint8(classifyChange(b));
    });
}

CompilationUnitDiff diffCompilationUnits(const Unit *oldUnit, const Unit *newUnit)
{
    auto diff = UnitDiffer{ oldUnit, newUnit, {} }.diff();

    // Sort the changes so that they are easy to apply.
    // We want the most severe changes first.
    sortChanges(diff.changes);

    return diff;
}

} // namespace QV4::CompiledData

QT_END_NAMESPACE
