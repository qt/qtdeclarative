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

static const char magic_str[] = "qv4cdata";

struct Unit
{
    char magic[8];
    qint16 architecture;
    qint16 version;

    enum {
        IsJavascript = 0x1,
        IsQml = 0x2
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
    uint jsClassTableSize;
    uint offsetToJSClassTable;
    uint indexOfRootFunction;
    quint32 sourceFileIndex;

    const String *stringAt(int idx) const {
        const uint *offsetTable = reinterpret_cast<const uint*>((reinterpret_cast<const char *>(this)) + offsetToStringTable);
        const uint offset = offsetTable[idx];
        return reinterpret_cast<const String*>(reinterpret_cast<const char *>(this) + offset);
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

    const JSClassMember *jsClassAt(int idx, int *nMembers) const {
        const uint *offsetTable = reinterpret_cast<const uint *>(reinterpret_cast<const char *>(this) + offsetToJSClassTable);
        const uint offset = offsetTable[idx];
        const char *ptr = reinterpret_cast<const char *>(this) + offset;
        const JSClass *klass = reinterpret_cast<const JSClass *>(ptr);
        *nMembers = klass->nMembers;
        return reinterpret_cast<const JSClassMember*>(ptr + sizeof(JSClass));
    }

    static int calculateSize(uint nStrings, uint nFunctions, uint nRegExps,
                             uint nLookups, uint nClasses) {
        return (sizeof(Unit)
                + (nStrings + nFunctions + nClasses) * sizeof(uint)
                + nRegExps * RegExp::calculateSize()
                + nLookups * Lookup::calculateSize()
                + 7) & ~7; }
};

struct Function
{
    enum Flags {
        HasDirectEval       = 0x1,
        UsesArgumentsObject = 0x2,
        IsStrict            = 0x4,
        IsNamedExpression   = 0x8
    };

    QV4::Value (*code)(ExecutionContext *, const uchar *);
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
//    quint32 formalsIndex[nFormals]
//    quint32 localsIndex[nLocals]
//    quint32 offsetForInnerFunctions[nInnerFunctions]
//    Function[nInnerFunctions]

    const quint32 *formalsTable() const { return reinterpret_cast<const quint32 *>(reinterpret_cast<const char *>(this) + formalsOffset); }
    const quint32 *localsTable() const { return reinterpret_cast<const quint32 *>(reinterpret_cast<const char *>(this) + localsOffset); }
    const quint32 *lineNumberMapping() const { return reinterpret_cast<const quint32 *>(reinterpret_cast<const char *>(this) + lineNumberMappingOffset); }

    static int calculateSize(int nFormals, int nLocals, int nInnerfunctions, int lineNumberMappings) {
        return (sizeof(Function) + (nFormals + nLocals + nInnerfunctions + 2 * lineNumberMappings) * sizeof(quint32) + 7) & ~0x7;
    }
};

struct String
{
    quint32 hash;
    quint32 flags; // isArrayIndex
    QArrayData str;
    // uint16 strdata[]

    QString qString() const {
        QStringDataPtr holder { const_cast<QStringData *>(static_cast<const QStringData*>(&str)) };
        return QString(holder);
    }

    static int calculateSize(const QString &str) {
        return (sizeof(String) + (str.length() + 1) * sizeof(quint16) + 7) & ~0x7;
    }
};

// Qml data structures

struct Value
{
    quint32 type; // Invalid, Boolean, Number, String, Function, Object, ListOfObjects
    union {
        bool b;
        int i;
        double d;
        quint32 offsetToString;
        quint32 offsetToFunction;
        quint32 offsetToObject;
    };
};

struct Binding
{
    quint32 offsetToPropertyName;
    Value value;
};

struct Parameter
{
    quint32 offsetToName;
    quint32 type;
    quint32 offsetToCustomTypeName;
    quint32 reserved;
};

struct Signal
{
    quint32 offsetToName;
    quint32 nParameters;
    Parameter parameters[1];
};

struct Property
{
    quint32 offsetToName;
    quint32 type;
    quint32 offsetToCustomTypeName;
    quint32 flags; // default, readonly
    Value value;
};

struct Object
{
    quint32 offsetToInheritedTypeName;
    quint32 offsetToId;
    quint32 offsetToDefaultProperty;
    quint32 nFunctions;
    quint32 offsetToFunctions;
    quint32 nProperties;
    quint32 offsetToProperties;
    quint32 nSignals;
    quint32 offsetToSignals;
    quint32 nBindings;
    quint32 offsetToBindings;
//    Function[]
//    Property[]
//    Signal[]
//    Binding[]
};

struct Imports
{
};


struct QmlUnit
{
    Unit header;
    int offsetToTypeName;
    Imports imports;
    Object object;
};

// This is how this hooks into the existing structures:

//VM::Function
//    CompilationUnit * (for functions that need to clean up)
//    CompiledData::Function *compiledFunction

struct CompilationUnit
{
    CompilationUnit()
        : refCount(0)
        , engine(0)
        , data(0)
        , runtimeStrings(0)
        , runtimeLookups(0)
        , runtimeClasses(0)
    {}
    virtual ~CompilationUnit();

    void ref() { ++refCount; }
    void deref() { if (!--refCount) delete this; }

    int refCount;
    ExecutionEngine *engine;
    Unit *data;

    QString fileName() const { return data->stringAt(data->sourceFileIndex)->qString(); }

    QV4::String **runtimeStrings; // Array
    QV4::Lookup *runtimeLookups;
    QV4::Value *runtimeRegularExpressions;
    QV4::InternalClass **runtimeClasses;

    QV4::Function *linkToEngine(QV4::ExecutionEngine *engine);

    virtual QV4::ExecutableAllocator::ChunkOfPages *chunkForFunction(int /*functionIndex*/) { return 0; }

    // ### runtime data
    // pointer to qml data for QML unit

    void markObjects();

protected:
    virtual QV4::Function *linkBackendToEngine(QV4::ExecutionEngine *engine) = 0;
};

struct MothCompilationUnit : public CompilationUnit
{
    virtual ~MothCompilationUnit() {
        // free all bytecode
    }

    // vector of bytecode

};

}

}

QT_END_NAMESPACE

#endif
