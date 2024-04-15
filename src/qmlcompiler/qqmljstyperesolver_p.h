// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSTYPERESOLVER_P_H
#define QQMLJSTYPERESOLVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <memory>
#include <private/qtqmlcompilerexports_p.h>

#include <private/qqmlirbuilder_p.h>
#include <private/qqmljsast_p.h>
#include "qqmljsimporter_p.h"
#include "qqmljslogger_p.h"
#include "qqmljsregistercontent_p.h"
#include "qqmljsresourcefilemapper_p.h"
#include "qqmljsscope_p.h"
#include "qqmljsscopesbyid_p.h"

QT_BEGIN_NAMESPACE

class QQmlJSImportVisitor;
class Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSTypeResolver
{
public:
    enum ParentMode { UseDocumentParent, UseParentProperty };
    enum CloneMode { CloneTypes, DoNotCloneTypes };
    enum ListMode { UseListProperty, UseQObjectList };

    QQmlJSTypeResolver(QQmlJSImporter *importer);

    // Note: must be called after the construction to read the QML program
    void init(QQmlJSImportVisitor *visitor, QQmlJS::AST::Node *program);

    QQmlJSScope::ConstPtr voidType() const { return m_voidType; }
    QQmlJSScope::ConstPtr emptyType() const { return m_emptyType; }
    QQmlJSScope::ConstPtr nullType() const { return m_nullType; }
    QQmlJSScope::ConstPtr realType() const { return m_realType; }
    QQmlJSScope::ConstPtr floatType() const { return m_floatType; }
    QQmlJSScope::ConstPtr int8Type() const { return m_int8Type; }
    QQmlJSScope::ConstPtr uint8Type() const { return m_uint8Type; }
    QQmlJSScope::ConstPtr int16Type() const { return m_int16Type; }
    QQmlJSScope::ConstPtr uint16Type() const { return m_uint16Type; }
    QQmlJSScope::ConstPtr int32Type() const { return m_int32Type; }
    QQmlJSScope::ConstPtr uint32Type() const { return m_uint32Type; }
    QQmlJSScope::ConstPtr int64Type() const { return m_int64Type; }
    QQmlJSScope::ConstPtr uint64Type() const { return m_uint64Type; }
    QQmlJSScope::ConstPtr boolType() const { return m_boolType; }
    QQmlJSScope::ConstPtr stringType() const { return m_stringType; }
    QQmlJSScope::ConstPtr stringListType() const { return m_stringListType; }
    QQmlJSScope::ConstPtr byteArrayType() const { return m_byteArrayType; }
    QQmlJSScope::ConstPtr urlType() const { return m_urlType; }
    QQmlJSScope::ConstPtr dateTimeType() const { return m_dateTimeType; }
    QQmlJSScope::ConstPtr dateType() const { return m_dateType; }
    QQmlJSScope::ConstPtr timeType() const { return m_timeType; }
    QQmlJSScope::ConstPtr variantListType() const { return m_variantListType; }
    QQmlJSScope::ConstPtr variantMapType() const { return m_variantMapType; }
    QQmlJSScope::ConstPtr varType() const { return m_varType; }
    QQmlJSScope::ConstPtr jsValueType() const { return m_jsValueType; }
    QQmlJSScope::ConstPtr jsPrimitiveType() const { return m_jsPrimitiveType; }
    QQmlJSScope::ConstPtr listPropertyType() const { return m_listPropertyType; }
    QQmlJSScope::ConstPtr metaObjectType() const { return m_metaObjectType; }
    QQmlJSScope::ConstPtr functionType() const { return m_functionType; }
    QQmlJSScope::ConstPtr jsGlobalObject() const { return m_jsGlobalObject; }
    QQmlJSScope::ConstPtr qObjectType() const { return m_qObjectType; }
    QQmlJSScope::ConstPtr qObjectListType() const { return m_qObjectListType; }
    QQmlJSScope::ConstPtr arrayPrototype() const { return m_arrayPrototype; }
    QQmlJSScope::ConstPtr forInIteratorPtr() const { return m_forInIteratorPtr; }
    QQmlJSScope::ConstPtr forOfIteratorPtr() const { return m_forOfIteratorPtr; }

    QQmlJSScope::ConstPtr scopeForLocation(const QV4::CompiledData::Location &location) const;

    bool isPrefix(const QString &name) const
    {
        return m_imports.hasType(name) && !m_imports.type(name).scope;
    }

    const QHash<QString, QQmlJS::ImportedScope<QQmlJSScope::ConstPtr>> &importedTypes() const
    {
        return m_imports.types();
    }
    QQmlJSScope::ConstPtr typeForName(const QString &name) const
    {
        return m_imports.type(name).scope;
    }
    QQmlJSScope::ConstPtr typeFromAST(QQmlJS::AST::Type *type) const;
    QQmlJSScope::ConstPtr typeForConst(QV4::ReturnedValue rv) const;
    QQmlJSRegisterContent typeForBinaryOperation(QSOperator::Op oper,
                                                 const QQmlJSRegisterContent &left,
                                                 const QQmlJSRegisterContent &right) const;

    enum class UnaryOperator { Not, Plus, Minus, Increment, Decrement, Complement };
    QQmlJSRegisterContent typeForArithmeticUnaryOperation(
            UnaryOperator op, const QQmlJSRegisterContent &operand) const;

    bool isPrimitive(const QQmlJSRegisterContent &type) const;
    bool isPrimitive(const QQmlJSScope::ConstPtr &type) const;

    bool isNumeric(const QQmlJSRegisterContent &type) const;
    bool isIntegral(const QQmlJSRegisterContent &type) const;

    bool canConvertFromTo(const QQmlJSScope::ConstPtr &from, const QQmlJSScope::ConstPtr &to) const;
    bool canConvertFromTo(const QQmlJSRegisterContent &from, const QQmlJSRegisterContent &to) const;
    QQmlJSRegisterContent merge(const QQmlJSRegisterContent &a,
                                const QQmlJSRegisterContent &b) const;

    enum class ComponentIsGeneric { No, Yes };
    QQmlJSScope::ConstPtr
    genericType(const QQmlJSScope::ConstPtr &type,
                ComponentIsGeneric allowComponent = ComponentIsGeneric::No) const;

    QQmlJSRegisterContent builtinType(const QQmlJSScope::ConstPtr &type) const;
    QQmlJSRegisterContent globalType(const QQmlJSScope::ConstPtr &type) const;
    QQmlJSRegisterContent scopedType(const QQmlJSScope::ConstPtr &scope, const QString &name,
                                     int lookupIndex = QQmlJSRegisterContent::InvalidLookupIndex,
                                     QQmlJSScopesByIdOptions options = Default) const;
    QQmlJSRegisterContent memberType(
            const QQmlJSRegisterContent &type, const QString &name,
            int lookupIndex = QQmlJSRegisterContent::InvalidLookupIndex) const;
    QQmlJSRegisterContent valueType(const QQmlJSRegisterContent &list) const;
    QQmlJSRegisterContent returnType(
            const QQmlJSScope::ConstPtr &type, QQmlJSRegisterContent::ContentVariant variant,
            const QQmlJSScope::ConstPtr &scope) const;

    QQmlJSRegisterContent iteratorPointer(
            const QQmlJSRegisterContent &listType, QQmlJS::AST::ForEachType type,
            int lookupIndex) const;

    bool registerIsStoredIn(const QQmlJSRegisterContent &reg,
                            const QQmlJSScope::ConstPtr &type) const;
    bool registerContains(const QQmlJSRegisterContent &reg,
                          const QQmlJSScope::ConstPtr &type) const;
    QQmlJSScope::ConstPtr containedType(const QQmlJSRegisterContent &container) const;
    QString containedTypeName(const QQmlJSRegisterContent &container,
                              bool useFancyName = false) const;

    QQmlJSRegisterContent tracked(const QQmlJSRegisterContent &type) const;
    QQmlJSRegisterContent original(const QQmlJSRegisterContent &type) const;

    QQmlJSScope::ConstPtr trackedContainedType(const QQmlJSRegisterContent &container) const;
    QQmlJSScope::ConstPtr originalContainedType(const QQmlJSRegisterContent &container) const;

    [[nodiscard]] bool adjustTrackedType(
            const QQmlJSScope::ConstPtr &tracked, const QQmlJSScope::ConstPtr &conversion) const;
    [[nodiscard]] bool adjustTrackedType(
            const QQmlJSScope::ConstPtr &tracked,
            const QList<QQmlJSScope::ConstPtr> &conversions) const;
    void adjustOriginalType(
            const QQmlJSScope::ConstPtr &tracked, const QQmlJSScope::ConstPtr &conversion) const;
    void generalizeType(const QQmlJSScope::ConstPtr &type) const;

    void setParentMode(ParentMode mode) { m_parentMode = mode; }
    ParentMode parentMode() const { return m_parentMode; }

    void setCloneMode(CloneMode mode) { m_cloneMode = mode; }
    bool cloneMode() const { return m_cloneMode; }

    QQmlJSScope::ConstPtr storedType(const QQmlJSScope::ConstPtr &type) const;
    QQmlJSScope::ConstPtr originalType(const QQmlJSScope::ConstPtr &type) const;
    QQmlJSScope::ConstPtr trackedType(const QQmlJSScope::ConstPtr &type) const;
    QQmlJSScope::ConstPtr comparableType(const QQmlJSScope::ConstPtr &type) const;

    const QQmlJSScopesById &objectsById() const { return m_objectsById; }
    bool canCallJSFunctions() const { return m_objectsById.signaturesAreEnforced(); }
    bool canAddressValueTypes() const { return m_objectsById.valueTypesAreAddressable(); }

    const QHash<QQmlJS::SourceLocation, QQmlJSMetaSignalHandler> &signalHandlers() const
    {
        return m_signalHandlers;
    }

    bool equals(const QQmlJSScope::ConstPtr &a, const QQmlJSScope::ConstPtr &b) const;

    QQmlJSRegisterContent convert(
            const QQmlJSRegisterContent &from, const QQmlJSRegisterContent &to) const;
    QQmlJSRegisterContent cast(
            const QQmlJSRegisterContent &from, const QQmlJSScope::ConstPtr &to) const;

    QQmlJSScope::ConstPtr merge(const QQmlJSScope::ConstPtr &a,
                                const QQmlJSScope::ConstPtr &b) const;

    bool canHoldUndefined(const QQmlJSRegisterContent &content) const;
    bool isOptionalType(const QQmlJSRegisterContent &content) const;
    QQmlJSScope::ConstPtr extractNonVoidFromOptionalType(
            const QQmlJSRegisterContent &content) const;

    bool isNumeric(const QQmlJSScope::ConstPtr &type) const;
    bool isIntegral(const QQmlJSScope::ConstPtr &type) const;
    bool isSignedInteger(const QQmlJSScope::ConstPtr &type) const;
    bool isUnsignedInteger(const QQmlJSScope::ConstPtr &type) const;

    bool canHold(const QQmlJSScope::ConstPtr &container,
                 const QQmlJSScope::ConstPtr &contained) const;

    bool canPopulate(
            const QQmlJSScope::ConstPtr &type, const QQmlJSScope::ConstPtr &argument,
            bool *isExtension) const;

    QQmlJSMetaMethod selectConstructor(
            const QQmlJSScope::ConstPtr &type, const QQmlJSScope::ConstPtr &argument,
            bool *isExtension) const;

    bool areEquivalentLists(const QQmlJSScope::ConstPtr &a, const QQmlJSScope::ConstPtr &b) const;

    bool isTriviallyCopyable(const QQmlJSScope::ConstPtr &type) const;

    bool inherits(const QQmlJSScope::ConstPtr &derived, const QQmlJSScope::ConstPtr &base) const;

protected:

    QQmlJSRegisterContent memberType(
            const QQmlJSScope::ConstPtr &type, const QString &name,
            int baseLookupIndex, int resultLookupIndex) const;
    QQmlJSRegisterContent memberEnumType(const QQmlJSScope::ConstPtr &type,
                                         const QString &name) const;
    bool checkEnums(const QQmlJSScope::ConstPtr &scope, const QString &name,
                    QQmlJSRegisterContent *result, QQmlJSScope::ExtensionKind mode) const;
    bool canPrimitivelyConvertFromTo(
            const QQmlJSScope::ConstPtr &from, const QQmlJSScope::ConstPtr &to) const;
    QQmlJSRegisterContent lengthProperty(bool isWritable, const QQmlJSScope::ConstPtr &scope) const;
    QQmlJSRegisterContent transformed(
            const QQmlJSRegisterContent &origin,
            QQmlJSScope::ConstPtr (QQmlJSTypeResolver::*op)(const QQmlJSScope::ConstPtr &) const) const;

    QQmlJSRegisterContent registerContentForName(
            const QString &name,
            const QQmlJSScope::ConstPtr &scopeType = QQmlJSScope::ConstPtr(),
            bool hasObjectModuelPrefix = false) const;


    QQmlJSScope::ConstPtr m_voidType;
    QQmlJSScope::ConstPtr m_emptyType;
    QQmlJSScope::ConstPtr m_nullType;
    QQmlJSScope::ConstPtr m_numberPrototype;
    QQmlJSScope::ConstPtr m_arrayPrototype;
    QQmlJSScope::ConstPtr m_realType;
    QQmlJSScope::ConstPtr m_floatType;
    QQmlJSScope::ConstPtr m_int8Type;
    QQmlJSScope::ConstPtr m_uint8Type;
    QQmlJSScope::ConstPtr m_int16Type;
    QQmlJSScope::ConstPtr m_uint16Type;
    QQmlJSScope::ConstPtr m_int32Type;
    QQmlJSScope::ConstPtr m_uint32Type;
    QQmlJSScope::ConstPtr m_int64Type;
    QQmlJSScope::ConstPtr m_uint64Type;
    QQmlJSScope::ConstPtr m_boolType;
    QQmlJSScope::ConstPtr m_stringType;
    QQmlJSScope::ConstPtr m_stringListType;
    QQmlJSScope::ConstPtr m_byteArrayType;
    QQmlJSScope::ConstPtr m_urlType;
    QQmlJSScope::ConstPtr m_dateTimeType;
    QQmlJSScope::ConstPtr m_dateType;
    QQmlJSScope::ConstPtr m_timeType;
    QQmlJSScope::ConstPtr m_variantListType;
    QQmlJSScope::ConstPtr m_variantMapType;
    QQmlJSScope::ConstPtr m_varType;
    QQmlJSScope::ConstPtr m_jsValueType;
    QQmlJSScope::ConstPtr m_jsPrimitiveType;
    QQmlJSScope::ConstPtr m_listPropertyType;
    QQmlJSScope::ConstPtr m_qObjectType;
    QQmlJSScope::ConstPtr m_qObjectListType;
    QQmlJSScope::ConstPtr m_qQmlScriptStringType;
    QQmlJSScope::ConstPtr m_metaObjectType;
    QQmlJSScope::ConstPtr m_functionType;
    QQmlJSScope::ConstPtr m_jsGlobalObject;
    QQmlJSScope::ConstPtr m_forInIteratorPtr;
    QQmlJSScope::ConstPtr m_forOfIteratorPtr;

    QQmlJSScopesById m_objectsById;
    QHash<QV4::CompiledData::Location, QQmlJSScope::ConstPtr> m_objectsByLocation;
    QQmlJSImporter::ImportedTypes m_imports;
    QHash<QQmlJS::SourceLocation, QQmlJSMetaSignalHandler> m_signalHandlers;

    ParentMode m_parentMode = UseParentProperty;
    CloneMode m_cloneMode = CloneTypes;
    QQmlJSLogger *m_logger = nullptr;

    struct TrackedType
    {
        // The type originally found via type analysis.
        QQmlJSScope::ConstPtr original;

        // Any later replacement used to overwrite the contents of the clone.
        QQmlJSScope::ConstPtr replacement;

        // A clone of original, used to track the type,
        // contents possibly overwritten by replacement.
        QQmlJSScope::Ptr clone;
    };

    std::unique_ptr<QHash<QQmlJSScope::ConstPtr, TrackedType>> m_trackedTypes;
};

/*!
\internal
Keep this struct around to be able to populate deferred scopes obtained from a QQmlJSTypeResolver.
*/
struct QQmlJSTypeResolverDependencies
{
    std::shared_ptr<QQmlJSImporter> importer;
    std::shared_ptr<QQmlJSResourceFileMapper> mapper;
};

QT_END_NAMESPACE

#endif // QQMLJSTYPERESOLVER_P_H
