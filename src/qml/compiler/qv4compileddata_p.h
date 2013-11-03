/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QV4COMPILEDDATA_P_H
#define QV4COMPILEDDATA_P_H

#include <QtCore/qstring.h>
#include <QVector>
#include <QStringList>
#include <QHash>
#include <private/qv4value_def_p.h>
#include <private/qv4executableallocator_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace V4IR {
struct Function;
}
}

namespace QV4 {

struct Function;
struct ExecutionContext;

namespace CompiledData {

struct String;
struct Function;
struct Lookup;
struct RegExp;

struct Location
{
    int line;
    int column;
};

// map from name index to location of first use
struct TypeReferenceMap : QHash<int, Location>
{
    void add(int nameIndex, const Location &loc) {
        if (contains(nameIndex))
            return;
        insert(nameIndex, loc);
    }
};

struct RegExp
{
    enum Flags {
        RegExp_Global     = 0x01,
        RegExp_IgnoreCase = 0x02,
        RegExp_Multiline  = 0x04
    };
    quint32 flags;
    quint32 stringIndex;

    static int calculateSize() { return sizeof(RegExp); }
};

struct Lookup
{
    enum Type {
        Type_Getter = 0x0,
        Type_Setter = 0x1,
        Type_GlobalGetter = 2
    };

    quint32 type_and_flags;
    quint32 nameIndex;

    static int calculateSize() { return sizeof(Lookup); }
};

struct JSClassMember
{
    uint nameOffset : 31;
    uint isAccessor : 1;
};

struct JSClass
{
    uint nMembers;
    // JSClassMember[nMembers]

    static int calculateSize(int nMembers) { return (sizeof(JSClass) + nMembers * sizeof(JSClassMember) + 7) & ~7; }
};

struct String
{
    quint32 hash;
    quint32 flags; // isArrayIndex
    QArrayData str;
    // uint16 strdata[]

    static int calculateSize(const QString &str) {
        return (sizeof(String) + (str.length() + 1) * sizeof(quint16) + 7) & ~0x7;
    }
};

static const char magic_str[] = "qv4cdata";

struct Unit
{
    char magic[8];
    qint16 architecture;
    qint16 version;

    enum {
        IsJavascript = 0x1,
        IsQml = 0x2,
        StaticData = 0x4, // Unit data persistent in memory?
        IsSingleton = 0x8
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

    QString stringAt(int idx) const {
        const uint *offsetTable = reinterpret_cast<const uint*>((reinterpret_cast<const char *>(this)) + offsetToStringTable);
        const uint offset = offsetTable[idx];
        const String *str = reinterpret_cast<const String*>(reinterpret_cast<const char *>(this) + offset);
        QStringDataPtr holder = { const_cast<QStringData *>(static_cast<const QStringData*>(&str->str)) };
        QString qstr(holder);
        if (flags & StaticData)
            return qstr;
        return QString(qstr.constData(), qstr.length());
    }

    const Function *functionAt(int idx) const {
        const uint *offsetTable = reinterpret_cast<const uint*>((reinterpret_cast<const char *>(this)) + offsetToFunctionTable);
        const uint offset = offsetTable[idx];
        return reinterpret_cast<const Function*>(reinterpret_cast<const char *>(this) + offset);
    }

    const Lookup *lookupTable() const { return reinterpret_cast<const Lookup*>(reinterpret_cast<const char *>(this) + offsetToLookupTable); }
    const RegExp *regexpAt(int index) const {
        return reinterpret_cast<const RegExp*>(reinterpret_cast<const char *>(this) + offsetToRegexpTable + index * sizeof(RegExp));
    }
    const QV4::SafeValue *constants() const {
        return reinterpret_cast<const QV4::SafeValue*>(reinterpret_cast<const char *>(this) + offsetToConstantTable);
    }

    const JSClassMember *jsClassAt(int idx, int *nMembers) const {
        const uint *offsetTable = reinterpret_cast<const uint *>(reinterpret_cast<const char *>(this) + offsetToJSClassTable);
        const uint offset = offsetTable[idx];
        const char *ptr = reinterpret_cast<const char *>(this) + offset;
        const JSClass *klass = reinterpret_cast<const JSClass *>(ptr);
        *nMembers = klass->nMembers;
        return reinterpret_cast<const JSClassMember*>(ptr + sizeof(JSClass));
    }

    static int calculateSize(uint headerSize, uint nStrings, uint nFunctions, uint nRegExps, uint nConstants,
                             uint nLookups, uint nClasses) {
        return (headerSize
                + (nStrings + nFunctions + nClasses) * sizeof(uint)
                + nRegExps * RegExp::calculateSize()
                + nConstants * sizeof(QV4::ReturnedValue)
                + nLookups * Lookup::calculateSize()
                + 7) & ~7; }
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

    quint32 index; // in CompilationUnit's function table
    quint32 nameIndex;
    qint64 flags;
    quint32 nFormals;
    quint32 formalsOffset;
    quint32 nLocals;
    quint32 localsOffset;
    quint32 nLineNumberMappingEntries;
    quint32 lineNumberMappingOffset; // Array of uint pairs (offset and line number)
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

//    quint32 formalsIndex[nFormals]
//    quint32 localsIndex[nLocals]
//    quint32 offsetForInnerFunctions[nInnerFunctions]
//    Function[nInnerFunctions]

    const quint32 *formalsTable() const { return reinterpret_cast<const quint32 *>(reinterpret_cast<const char *>(this) + formalsOffset); }
    const quint32 *localsTable() const { return reinterpret_cast<const quint32 *>(reinterpret_cast<const char *>(this) + localsOffset); }
    const quint32 *lineNumberMapping() const { return reinterpret_cast<const quint32 *>(reinterpret_cast<const char *>(this) + lineNumberMappingOffset); }
    const quint32 *qmlIdObjectDependencyTable() const { return reinterpret_cast<const quint32 *>(reinterpret_cast<const char *>(this) + dependingIdObjectsOffset); }
    const quint32 *qmlContextPropertiesDependencyTable() const { return reinterpret_cast<const quint32 *>(reinterpret_cast<const char *>(this) + dependingContextPropertiesOffset); }
    const quint32 *qmlScopePropertiesDependencyTable() const { return reinterpret_cast<const quint32 *>(reinterpret_cast<const char *>(this) + dependingScopePropertiesOffset); }

    inline bool hasQmlDependencies() const { return nDependingIdObjects > 0 || nDependingContextProperties > 0 || nDependingScopeProperties > 0; }

    static int calculateSize(int nFormals, int nLocals, int nInnerfunctions, int lineNumberMappings, int nIdObjectDependencies, int nPropertyDependencies) {
        return (sizeof(Function) + (nFormals + nLocals + nInnerfunctions + 2 * lineNumberMappings + nIdObjectDependencies + 2 * nPropertyDependencies) * sizeof(quint32) + 7) & ~0x7;
    }
};

// Qml data structures

struct Binding
{
    quint32 propertyNameIndex;

    enum ValueType {
        Type_Invalid,
        Type_Boolean,
        Type_Number,
        Type_String,
        Type_Script,
        Type_Object,
        Type_AttachedProperty,
        Type_GroupProperty
    };

    enum Flags {
        IsSignalHandlerExpression = 0x1
    };

    quint32 flags : 16;
    quint32 type : 16;
    union {
        bool b;
        double d;
        quint32 compiledScriptIndex; // used when Type_Script
        quint32 objectIndex;
    } value;
    quint32 stringIndex; // Set for Type_String and Type_Script (the latter because of script strings)

    Location location;

    QString valueAsString(const Unit *unit) const;
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
    quint32 reserved;
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
};

struct Property
{
    enum Type { Var = 0, Variant, Int, Bool, Real, String, Url, Color,
                Font, Time, Date, DateTime, Rect, Point, Size,
                Vector2D, Vector3D, Vector4D, Matrix4x4, Quaternion,
                Alias, Custom, CustomList };

    enum Flags {
        IsReadOnly = 0x1
    };

    quint32 nameIndex;
    quint32 type;
    union {
        quint32 customTypeNameIndex; // If type >= Custom
        quint32 aliasIdValueIndex; // If type == Alias
    };
    quint32 aliasPropertyValueIndex;
    quint32 flags; // readonly
    Location location;
    Location aliasLocation; // If type == Alias
};

struct Object
{
    // Depending on the use, this may be the type name to instantiate before instantiating this
    // object. For grouped properties the type name will be empty and for attached properties
    // it will be the name of the attached type.
    quint32 inheritedTypeNameIndex;
    quint32 idIndex;
    quint32 indexOfDefaultProperty;
    quint32 nFunctions;
    quint32 offsetToFunctions;
    quint32 nProperties;
    quint32 offsetToProperties;
    quint32 nSignals;
    quint32 offsetToSignals; // which in turn will be a table with offsets to variable-sized Signal objects
    quint32 nBindings;
    quint32 offsetToBindings;
    Location location;
    Location locationOfIdProperty;
//    Function[]
//    Property[]
//    Signal[]
//    Binding[]

    static int calculateSizeExcludingSignals(int nFunctions, int nProperties, int nSignals, int nBindings)
    {
        return ( sizeof(Object)
                 + nFunctions * sizeof(quint32)
                 + nProperties * sizeof(Property)
                 + nSignals * sizeof(quint32)
                 + nBindings * sizeof(Binding)
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
};

struct QmlUnit
{
    Unit header;
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
        return header.flags & Unit::IsSingleton;
    }
};

// This is how this hooks into the existing structures:

//VM::Function
//    CompilationUnit * (for functions that need to clean up)
//    CompiledData::Function *compiledFunction

struct Q_QML_EXPORT CompilationUnit
{
    CompilationUnit()
        : refCount(0)
        , engine(0)
        , data(0)
        , ownsData(false)
        , runtimeStrings(0)
        , runtimeLookups(0)
        , runtimeRegularExpressions(0)
        , runtimeClasses(0)
    {}
    virtual ~CompilationUnit();

    void ref() { ++refCount; }
    void deref() { if (!--refCount) delete this; }

    int refCount;
    ExecutionEngine *engine;
    Unit *data;
    bool ownsData;

    QString fileName() const { return data->stringAt(data->sourceFileIndex); }

    QV4::SafeString *runtimeStrings; // Array
    QV4::Lookup *runtimeLookups;
    QV4::SafeValue *runtimeRegularExpressions;
    QV4::InternalClass **runtimeClasses;
    QVector<QV4::Function *> runtimeFunctions;
//    QVector<QV4::Function *> runtimeFunctionsSortedByAddress;

    QV4::Function *linkToEngine(QV4::ExecutionEngine *engine);
    void unlink();

    virtual QV4::ExecutableAllocator::ChunkOfPages *chunkForFunction(int /*functionIndex*/) { return 0; }

    // ### runtime data
    // pointer to qml data for QML unit

    void markObjects(QV4::ExecutionEngine *e);

protected:
    virtual void linkBackendToEngine(QV4::ExecutionEngine *engine) = 0;
};

}

}

QT_END_NAMESPACE

#endif
