/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
**
****************************************************************************/
#ifndef QV4COMPILEDDATA_P_H
#define QV4COMPILEDDATA_P_H

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

#include <QtCore/qstring.h>
#include <QVector>
#include <QStringList>
#include <QHash>
#include <QUrl>

#include <private/qv4value_p.h>
#include <private/qv4executableallocator_p.h>
#include <private/qqmlrefcount_p.h>
#include <private/qqmlnullablevalue_p.h>
#include <private/qv4identifier_p.h>
#include <private/qflagpointer_p.h>
#ifndef V4_BOOTSTRAP
#include <private/qqmltypenamecache_p.h>
#include <private/qqmlpropertycache_p.h>
#endif

QT_BEGIN_NAMESPACE

class QIODevice;
class QQmlPropertyCache;
class QQmlPropertyData;
class QQmlTypeNameCache;
class QQmlScriptData;
class QQmlType;
class QQmlEngine;

namespace QmlIR {
struct Document;
}

namespace QV4 {
namespace IR {
struct Function;
}

struct Function;

namespace CompiledData {

struct String;
struct Function;
struct Lookup;
struct RegExp;
struct Unit;

template <typename ItemType, typename Container, const ItemType *(Container::*IndexedGetter)(int index) const>
struct TableIterator
{
    TableIterator(const Container *container, int index) : container(container), index(index) {}
    const Container *container;
    int index;

    const ItemType *operator->() { return (container->*IndexedGetter)(index); }
    void operator++() { ++index; }
    bool operator==(const TableIterator &rhs) const { return index == rhs.index; }
    bool operator!=(const TableIterator &rhs) const { return index != rhs.index; }
};

#if defined(Q_CC_MSVC) || defined(Q_CC_GNU)
#pragma pack(push, 1)
#endif

struct Location
{
    qint32 line : 20;
    qint32 column : 12;

    Location(): line(-1), column(-1) {}

    inline bool operator<(const Location &other) const {
        return line < other.line ||
               (line == other.line && column < other.column);
    }
};

struct RegExp
{
    enum Flags {
        RegExp_Global     = 0x01,
        RegExp_IgnoreCase = 0x02,
        RegExp_Multiline  = 0x04
    };
    quint32 flags : 4;
    quint32 stringIndex : 28;

    static int calculateSize() { return sizeof(RegExp); }
};

struct Lookup
{
    enum Type {
        Type_Getter = 0x0,
        Type_Setter = 0x1,
        Type_GlobalGetter = 2,
        Type_IndexedGetter = 3,
        Type_IndexedSetter = 4
    };

    quint32 type_and_flags : 4;
    quint32 nameIndex : 28;

    static int calculateSize() { return sizeof(Lookup); }
};

struct JSClassMember
{
    quint32 nameOffset : 31;
    quint32 isAccessor : 1;
};

struct JSClass
{
    uint nMembers;
    // JSClassMember[nMembers]

    static int calculateSize(int nMembers) { return (sizeof(JSClass) + nMembers * sizeof(JSClassMember) + 7) & ~7; }
};

struct String
{
    qint32 size;
    // uint16 strdata[]

    static int calculateSize(const QString &str) {
        return (sizeof(String) + str.length() * sizeof(quint16) + 7) & ~0x7;
    }
};

struct Function
{
    enum Flags {
        HasDirectEval       = 0x1,
        UsesArgumentsObject = 0x2,
        IsStrict            = 0x4,
        IsNamedExpression   = 0x8,
        HasCatchOrWith      = 0x10
    };

    quint8 flags;
    quint32 nameIndex;
    quint32 nFormals;
    quint32 formalsOffset;
    quint32 nLocals;
    quint32 localsOffset;
    quint32 nInnerFunctions;
    quint32 innerFunctionsOffset;
    Location location;

    // Qml Extensions Begin
    quint32 nDependingIdObjects;
    quint32 dependingIdObjectsOffset; // Array of resolved ID objects
    quint32 nDependingContextProperties;
    quint32 dependingContextPropertiesOffset; // Array of int pairs (property index and notify index)
    quint32 nDependingScopeProperties;
    quint32 dependingScopePropertiesOffset; // Array of int pairs (property index and notify index)
    // Qml Extensions End

    // Absolute offset into file where the code for this function is located. Only used when the function
    // is serialized.
    quint64 codeOffset;
    quint64 codeSize;

//    quint32 formalsIndex[nFormals]
//    quint32 localsIndex[nLocals]
//    quint32 offsetForInnerFunctions[nInnerFunctions]
//    Function[nInnerFunctions]

    const quint32 *formalsTable() const { return reinterpret_cast<const quint32 *>(reinterpret_cast<const char *>(this) + formalsOffset); }
    const quint32 *localsTable() const { return reinterpret_cast<const quint32 *>(reinterpret_cast<const char *>(this) + localsOffset); }
    const quint32 *qmlIdObjectDependencyTable() const { return reinterpret_cast<const quint32 *>(reinterpret_cast<const char *>(this) + dependingIdObjectsOffset); }
    const quint32 *qmlContextPropertiesDependencyTable() const { return reinterpret_cast<const quint32 *>(reinterpret_cast<const char *>(this) + dependingContextPropertiesOffset); }
    const quint32 *qmlScopePropertiesDependencyTable() const { return reinterpret_cast<const quint32 *>(reinterpret_cast<const char *>(this) + dependingScopePropertiesOffset); }

    // --- QQmlPropertyCacheCreator interface
    const quint32 *formalsBegin() const { return formalsTable(); }
    const quint32 *formalsEnd() const { return formalsTable() + nFormals; }
    // ---

    inline bool hasQmlDependencies() const { return nDependingIdObjects > 0 || nDependingContextProperties > 0 || nDependingScopeProperties > 0; }

    static int calculateSize(int nFormals, int nLocals, int nInnerfunctions, int nIdObjectDependencies, int nPropertyDependencies) {
        return (sizeof(Function) + (nFormals + nLocals + nInnerfunctions + nIdObjectDependencies + 2 * nPropertyDependencies) * sizeof(quint32) + 7) & ~0x7;
    }
};

// Qml data structures

struct Q_QML_EXPORT TranslationData {
    quint32 commentIndex;
    int number;
};

struct Q_QML_PRIVATE_EXPORT Binding
{
    quint32 propertyNameIndex;

    enum ValueType {
        Type_Invalid,
        Type_Boolean,
        Type_Number,
        Type_String,
        Type_Translation,
        Type_TranslationById,
        Type_Script,
        Type_Object,
        Type_AttachedProperty,
        Type_GroupProperty
    };

    enum Flags {
        IsSignalHandlerExpression = 0x1,
        IsSignalHandlerObject = 0x2,
        IsOnAssignment = 0x4,
        InitializerForReadOnlyDeclaration = 0x8,
        IsResolvedEnum = 0x10,
        IsListItem = 0x20,
        IsBindingToAlias = 0x40,
        IsDeferredBinding = 0x80,
        IsCustomParserBinding = 0x100,
    };

    quint32 flags : 16;
    quint32 type : 16;
    union {
        bool b;
        double d;
        quint32 compiledScriptIndex; // used when Type_Script
        quint32 objectIndex;
        TranslationData translationData; // used when Type_Translation
    } value;
    quint32 stringIndex; // Set for Type_String, Type_Translation and Type_Script (the latter because of script strings)

    Location location;
    Location valueLocation;

    bool isValueBinding() const
    {
        if (type == Type_AttachedProperty
            || type == Type_GroupProperty)
            return false;
        if (flags & IsSignalHandlerExpression
            || flags & IsSignalHandlerObject)
            return false;
        return true;
    }

    bool isValueBindingNoAlias() const { return isValueBinding() && !(flags & IsBindingToAlias); }
    bool isValueBindingToAlias() const { return isValueBinding() && (flags & IsBindingToAlias); }

    bool isSignalHandler() const
    {
        if (flags & IsSignalHandlerExpression || flags & IsSignalHandlerObject) {
            Q_ASSERT(!isValueBinding());
            Q_ASSERT(!isAttachedProperty());
            Q_ASSERT(!isGroupProperty());
            return true;
        }
        return false;
    }

    bool isAttachedProperty() const
    {
        if (type == Type_AttachedProperty) {
            Q_ASSERT(!isValueBinding());
            Q_ASSERT(!isSignalHandler());
            Q_ASSERT(!isGroupProperty());
            return true;
        }
        return false;
    }

    bool isGroupProperty() const
    {
        if (type == Type_GroupProperty) {
            Q_ASSERT(!isValueBinding());
            Q_ASSERT(!isSignalHandler());
            Q_ASSERT(!isAttachedProperty());
            return true;
        }
        return false;
    }

    static QString escapedString(const QString &string);

    bool evaluatesToString() const { return type == Type_String || type == Type_Translation || type == Type_TranslationById; }

    QString valueAsString(const Unit *unit) const;
    QString valueAsScriptString(const Unit *unit) const;
    double valueAsNumber() const
    {
        if (type == Type_Number)
            return value.d;
        return 0.0;

    }
    bool valueAsBoolean() const
    {
        if (type == Type_Boolean)
            return value.b;
        return false;
    }

};

struct Parameter
{
    quint32 nameIndex;
    quint32 type;
    quint32 customTypeNameIndex;
    Location location;
};

struct Signal
{
    quint32 nameIndex;
    quint32 nParameters;
    Location location;
    // Parameter parameters[1];

    const Parameter *parameterAt(int idx) const {
        return reinterpret_cast<const Parameter*>(this + 1) + idx;
    }

    static int calculateSize(int nParameters) {
        return (sizeof(Signal)
                + nParameters * sizeof(Parameter)
                + 7) & ~0x7;
    }

    // --- QQmlPropertyCacheCceatorInterface
    const Parameter *parametersBegin() const { return parameterAt(0); }
    const Parameter *parametersEnd() const { return parameterAt(nParameters); }
    int parameterCount() const { return nParameters; }
    // ---
};

struct Property
{
    enum Type { Var = 0, Variant, Int, Bool, Real, String, Url, Color,
                Font, Time, Date, DateTime, Rect, Point, Size,
                Vector2D, Vector3D, Vector4D, Matrix4x4, Quaternion,
                Custom, CustomList };

    enum Flags {
        IsReadOnly = 0x1
    };

    quint32 nameIndex;
    quint32 type : 31;
    quint32 flags : 1; // readonly
    quint32 customTypeNameIndex; // If type >= Custom
    Location location;
};

struct Alias {
    enum Flags {
        IsReadOnly = 0x1,
        Resolved = 0x2,
        AliasPointsToPointerObject = 0x4
    };
    quint32 nameIndex : 29;
    quint32 flags : 3;
    union {
        quint32 idIndex; // string index
        quint32 targetObjectId; // object id index (in QQmlContextData::idValues)
    };
    union {
        quint32 propertyNameIndex; // string index
        qint32 encodedMetaPropertyIndex;
    };
    Location location;
    Location referenceLocation;

    bool isObjectAlias() const {
        Q_ASSERT(flags & Resolved);
        return encodedMetaPropertyIndex == -1;
    }
};

struct Object
{
    enum Flags {
        NoFlag = 0x0,
        IsComponent = 0x1, // object was identified to be an explicit or implicit component boundary
        HasDeferredBindings = 0x2, // any of the bindings are deferred
        HasCustomParserBindings = 0x4
    };

    // Depending on the use, this may be the type name to instantiate before instantiating this
    // object. For grouped properties the type name will be empty and for attached properties
    // it will be the name of the attached type.
    quint32 inheritedTypeNameIndex;
    quint32 idNameIndex;
    qint32 id : 16;
    qint32 flags : 15;
    quint32 defaultPropertyIsAlias : 1;
    qint32 indexOfDefaultPropertyOrAlias; // -1 means no default property declared in this object
    quint32 nFunctions;
    quint32 offsetToFunctions;
    quint32 nProperties;
    quint32 offsetToProperties;
    quint32 nAliases;
    quint32 offsetToAliases;
    quint32 nSignals;
    quint32 offsetToSignals; // which in turn will be a table with offsets to variable-sized Signal objects
    quint32 nBindings;
    quint32 offsetToBindings;
    quint32 nNamedObjectsInComponent;
    quint32 offsetToNamedObjectsInComponent;
    Location location;
    Location locationOfIdProperty;
//    Function[]
//    Property[]
//    Signal[]
//    Binding[]

    static int calculateSizeExcludingSignals(int nFunctions, int nProperties, int nAliases, int nSignals, int nBindings, int nNamedObjectsInComponent)
    {
        return ( sizeof(Object)
                 + nFunctions * sizeof(quint32)
                 + nProperties * sizeof(Property)
                 + nAliases * sizeof(Alias)
                 + nSignals * sizeof(quint32)
                 + nBindings * sizeof(Binding)
                 + nNamedObjectsInComponent * sizeof(int)
                 + 0x7
               ) & ~0x7;
    }

    const quint32 *functionOffsetTable() const
    {
        return reinterpret_cast<const quint32*>(reinterpret_cast<const char *>(this) + offsetToFunctions);
    }

    const Property *propertyTable() const
    {
        return reinterpret_cast<const Property*>(reinterpret_cast<const char *>(this) + offsetToProperties);
    }

    const Alias *aliasTable() const
    {
        return reinterpret_cast<const Alias*>(reinterpret_cast<const char *>(this) + offsetToAliases);
    }

    const Binding *bindingTable() const
    {
        return reinterpret_cast<const Binding*>(reinterpret_cast<const char *>(this) + offsetToBindings);
    }

    const Signal *signalAt(int idx) const
    {
        const uint *offsetTable = reinterpret_cast<const uint*>((reinterpret_cast<const char *>(this)) + offsetToSignals);
        const uint offset = offsetTable[idx];
        return reinterpret_cast<const Signal*>(reinterpret_cast<const char*>(this) + offset);
    }

    const quint32 *namedObjectsInComponentTable() const
    {
        return reinterpret_cast<const quint32*>(reinterpret_cast<const char *>(this) + offsetToNamedObjectsInComponent);
    }

    // --- QQmlPropertyCacheCreator interface
    int propertyCount() const { return nProperties; }
    int aliasCount() const { return nAliases; }
    int signalCount() const { return nSignals; }
    int functionCount() const { return nFunctions; }

    const Binding *bindingsBegin() const { return bindingTable(); }
    const Binding *bindingsEnd() const { return bindingTable() + nBindings; }

    const Property *propertiesBegin() const { return propertyTable(); }
    const Property *propertiesEnd() const { return propertyTable() + nProperties; }

    const Alias *aliasesBegin() const { return aliasTable(); }
    const Alias *aliasesEnd() const { return aliasTable() + nAliases; }

    typedef TableIterator<Signal, Object, &Object::signalAt> SignalIterator;
    SignalIterator signalsBegin() const { return SignalIterator(this, 0); }
    SignalIterator signalsEnd() const { return SignalIterator(this, nSignals); }
    // ---
};

struct Import
{
    enum ImportType {
        ImportLibrary = 0x1,
        ImportFile = 0x2,
        ImportScript = 0x3
    };
    quint32 type;

    quint32 uriIndex;
    quint32 qualifierIndex;

    qint32 majorVersion;
    qint32 minorVersion;

    Location location;

    Import(): type(0), uriIndex(0), qualifierIndex(0), majorVersion(0), minorVersion(0) {}
};

static const char magic_str[] = "qv4cdata";

struct Unit
{
    char magic[8];
    qint16 architecture;
    qint16 version;
    quint32 unitSize; // Size of the Unit and any depending data.

    enum {
        IsJavascript = 0x1,
        IsQml = 0x2,
        StaticData = 0x4, // Unit data persistent in memory?
        IsSingleton = 0x8,
        IsSharedLibrary = 0x10 // .pragma shared?
    };
    quint32 flags;
    uint stringTableSize;
    uint offsetToStringTable;
    uint functionTableSize;
    uint offsetToFunctionTable;
    uint lookupTableSize;
    uint offsetToLookupTable;
    uint regexpTableSize;
    uint offsetToRegexpTable;
    uint constantTableSize;
    uint offsetToConstantTable;
    uint jsClassTableSize;
    uint offsetToJSClassTable;
    qint32 indexOfRootFunction;
    quint32 sourceFileIndex;

    /* QML specific fields */
    quint32 nImports;
    quint32 offsetToImports;
    quint32 nObjects;
    quint32 offsetToObjects;
    quint32 indexOfRootObject;

    const Import *importAt(int idx) const {
        return reinterpret_cast<const Import*>((reinterpret_cast<const char *>(this)) + offsetToImports + idx * sizeof(Import));
    }

    const Object *objectAt(int idx) const {
        const uint *offsetTable = reinterpret_cast<const uint*>((reinterpret_cast<const char *>(this)) + offsetToObjects);
        const uint offset = offsetTable[idx];
        return reinterpret_cast<const Object*>(reinterpret_cast<const char*>(this) + offset);
    }

    bool isSingleton() const {
        return flags & Unit::IsSingleton;
    }
    /* end QML specific fields*/

    QString stringAt(int idx) const {
        const uint *offsetTable = reinterpret_cast<const uint*>((reinterpret_cast<const char *>(this)) + offsetToStringTable);
        const uint offset = offsetTable[idx];
        const String *str = reinterpret_cast<const String*>(reinterpret_cast<const char *>(this) + offset);
        if (str->size == 0)
            return QString();
        const QChar *characters = reinterpret_cast<const QChar *>(str + 1);
        if (flags & StaticData)
            return QString::fromRawData(characters, str->size);
        return QString(characters, str->size);
    }

    const uint *functionOffsetTable() const { return reinterpret_cast<const uint*>((reinterpret_cast<const char *>(this)) + offsetToFunctionTable); }

    const Function *functionAt(int idx) const {
        const uint *offsetTable = functionOffsetTable();
        const uint offset = offsetTable[idx];
        return reinterpret_cast<const Function*>(reinterpret_cast<const char *>(this) + offset);
    }

    const Lookup *lookupTable() const { return reinterpret_cast<const Lookup*>(reinterpret_cast<const char *>(this) + offsetToLookupTable); }
    const RegExp *regexpAt(int index) const {
        return reinterpret_cast<const RegExp*>(reinterpret_cast<const char *>(this) + offsetToRegexpTable + index * sizeof(RegExp));
    }
    const QV4::Value *constants() const {
        return reinterpret_cast<const QV4::Value*>(reinterpret_cast<const char *>(this) + offsetToConstantTable);
    }

    const JSClassMember *jsClassAt(int idx, int *nMembers) const {
        const uint *offsetTable = reinterpret_cast<const uint *>(reinterpret_cast<const char *>(this) + offsetToJSClassTable);
        const uint offset = offsetTable[idx];
        const char *ptr = reinterpret_cast<const char *>(this) + offset;
        const JSClass *klass = reinterpret_cast<const JSClass *>(ptr);
        *nMembers = klass->nMembers;
        return reinterpret_cast<const JSClassMember*>(ptr + sizeof(JSClass));
    }

    static int calculateSize(uint nFunctions, uint nRegExps, uint nConstants,
                             uint nLookups, uint nClasses) {
        return (sizeof(Unit)
                + (nFunctions + nClasses) * sizeof(uint)
                + nRegExps * RegExp::calculateSize()
                + nConstants * sizeof(QV4::ReturnedValue)
                + nLookups * Lookup::calculateSize()
                + 7) & ~7; }
};

#if defined(Q_CC_MSVC) || defined(Q_CC_GNU)
#pragma pack(pop)
#endif

struct TypeReference
{
    TypeReference(const Location &loc)
        : location(loc)
        , needsCreation(false)
        , errorWhenNotFound(false)
    {}
    Location location; // first use
    bool needsCreation : 1; // whether the type needs to be creatable or not
    bool errorWhenNotFound: 1;
};

// map from name index to location of first use
struct TypeReferenceMap : QHash<int, TypeReference>
{
    TypeReference &add(int nameIndex, const Location &loc) {
        Iterator it = find(nameIndex);
        if (it != end())
            return *it;
        return *insert(nameIndex, loc);
    }

    template <typename CompiledObject>
    void collectFromObject(const CompiledObject *obj)
    {
        if (obj->inheritedTypeNameIndex != 0) {
            TypeReference &r = this->add(obj->inheritedTypeNameIndex, obj->location);
            r.needsCreation = true;
            r.errorWhenNotFound = true;
        }

        for (auto prop = obj->propertiesBegin(), propEnd = obj->propertiesEnd(); prop != propEnd; ++prop) {
            if (prop->type >= QV4::CompiledData::Property::Custom) {
                // ### FIXME: We could report the more accurate location here by using prop->location, but the old
                // compiler can't and the tests expect it to be the object location right now.
                TypeReference &r = this->add(prop->customTypeNameIndex, obj->location);
                r.errorWhenNotFound = true;
            }
        }

        for (auto binding = obj->bindingsBegin(), bindingEnd = obj->bindingsEnd(); binding != bindingEnd; ++binding) {
            if (binding->type == QV4::CompiledData::Binding::Type_AttachedProperty)
                this->add(binding->propertyNameIndex, binding->location);
        }
    }

    template <typename Iterator>
    void collectFromObjects(Iterator it, Iterator end)
    {
        for (; it != end; ++it)
            collectFromObject(*it);
    }
};

// index is per-object binding index
typedef QVector<QQmlPropertyData*> BindingPropertyData;

// This is how this hooks into the existing structures:

//VM::Function
//    CompilationUnit * (for functions that need to clean up)
//    CompiledData::Function *compiledFunction

struct Q_QML_PRIVATE_EXPORT CompilationUnit : public QQmlRefCount
{
#ifdef V4_BOOTSTRAP
    CompilationUnit()
        : data(0)
    {}
    virtual ~CompilationUnit() {}
#else
    CompilationUnit();
    virtual ~CompilationUnit();
#endif

    const Unit *data;

    // Called only when building QML, when we build the header for JS first and append QML data
    virtual QV4::CompiledData::Unit *createUnitData(QmlIR::Document *irDocument);

#ifndef V4_BOOTSTRAP
    ExecutionEngine *engine;
    QString fileName() const { return data->stringAt(data->sourceFileIndex); }
    QUrl url() const { if (m_url.isNull) m_url = QUrl(fileName()); return m_url; }

    QV4::Heap::String **runtimeStrings; // Array
    QV4::Lookup *runtimeLookups;
    QV4::Value *runtimeRegularExpressions;
    QV4::InternalClass **runtimeClasses;
    QVector<QV4::Function *> runtimeFunctions;
    mutable QQmlNullableValue<QUrl> m_url;

    // QML specific fields
    QQmlPropertyCacheVector propertyCaches;
    QQmlPropertyCache *rootPropertyCache() const { return propertyCaches.at(data->indexOfRootObject); }

    QQmlRefPointer<QQmlTypeNameCache> importCache;

    // index is object index. This allows fast access to the
    // property data when initializing bindings, avoiding expensive
    // lookups by string (property name).
    QVector<BindingPropertyData> bindingPropertyDataPerObject;

    // mapping from component object index (CompiledData::Unit object index that points to component) to identifier hash of named objects
    // this is initialized on-demand by QQmlContextData
    QHash<int, IdentifierHash<int>> namedObjectsPerComponentCache;
    IdentifierHash<int> namedObjectsPerComponent(int componentObjectIndex);

    void finalize(QQmlEnginePrivate *engine);

    int totalBindingsCount; // Number of bindings used in this type
    int totalParserStatusCount; // Number of instantiated types that are QQmlParserStatus subclasses
    int totalObjectCount; // Number of objects explicitly instantiated

    QVector<QQmlScriptData *> dependentScripts;

    struct ResolvedTypeReference
    {
        ResolvedTypeReference()
            : type(0)
            , majorVersion(0)
            , minorVersion(0)
            , isFullyDynamicType(false)
        {}

        QQmlType *type;
        QQmlRefPointer<QQmlPropertyCache> typePropertyCache;
        QQmlRefPointer<QV4::CompiledData::CompilationUnit> compilationUnit;

        int majorVersion;
        int minorVersion;
        // Types such as QQmlPropertyMap can add properties dynamically at run-time and
        // therefore cannot have a property cache installed when instantiated.
        bool isFullyDynamicType;

        QQmlPropertyCache *propertyCache() const;
        QQmlPropertyCache *createPropertyCache(QQmlEngine *);

        void doDynamicTypeCheck();
    };
    // map from name index
    typedef QHash<int, ResolvedTypeReference*> ResolvedTypeReferenceMap;
    ResolvedTypeReferenceMap resolvedTypes;

    int metaTypeId;
    int listMetaTypeId;
    bool isRegisteredWithEngine;


    // --- interface for QQmlPropertyCacheCreator
    typedef Object CompiledObject;
    int objectCount() const { return data->nObjects; }
    int rootObjectIndex() const { return data->indexOfRootObject; }
    const Object *objectAt(int index) const { return data->objectAt(index); }
    QString stringAt(int index) const { return data->stringAt(index); }

    struct FunctionIterator
    {
        FunctionIterator(const Unit *unit, const Object *object, int index) : unit(unit), object(object), index(index) {}
        const Unit *unit;
        const Object *object;
        int index;

        const Function *operator->() const { return unit->functionAt(object->functionOffsetTable()[index]); }
        void operator++() { ++index; }
        bool operator==(const FunctionIterator &rhs) const { return index == rhs.index; }
        bool operator!=(const FunctionIterator &rhs) const { return index != rhs.index; }
    };
    FunctionIterator objectFunctionsBegin(const Object *object) const { return FunctionIterator(data, object, 0); }
    FunctionIterator objectFunctionsEnd(const Object *object) const { return FunctionIterator(data, object, object->nFunctions); }
    // ---

    QV4::Function *linkToEngine(QV4::ExecutionEngine *engine);
    void unlink();

    void markObjects(QV4::ExecutionEngine *e);

    void destroy() Q_DECL_OVERRIDE;

    bool saveToDisk(QString *errorString);

protected:
    virtual void linkBackendToEngine(QV4::ExecutionEngine *engine) = 0;
    virtual void prepareCodeOffsetsForDiskStorage(CompiledData::Unit *unit);
    virtual bool saveCodeToDisk(QIODevice *device, const CompiledData::Unit *unit, QString *errorString);
#endif // V4_BOOTSTRAP
};

}

}

Q_DECLARE_TYPEINFO(QV4::CompiledData::JSClassMember, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif
