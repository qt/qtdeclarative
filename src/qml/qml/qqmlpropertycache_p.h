/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLPROPERTYCACHE_P_H
#define QQMLPROPERTYCACHE_P_H

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

#include <private/qqmlrefcount_p.h>
#include "qqmlcleanup_p.h"
#include "qqmlnotifier_p.h"

#include <private/qhashedstring_p.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

class QV8Engine;
class QMetaProperty;
class QV8QObjectWrapper;
class QQmlEngine;
class QQmlPropertyData;
class QQmlAccessors;
class QQmlPropertyCacheMethodArguments;

// We have this somewhat awful split between RawData and Data so that RawData can be
// used in unions.  In normal code, you should always use Data which initializes RawData
// to an invalid state on construction.
class QQmlPropertyRawData
{
public:
    enum Flag {
        NoFlags           = 0x00000000,
        ValueTypeFlagMask = 0x0000FFFF, // Flags in valueTypeFlags must fit in this mask

        // Can apply to all properties, except IsFunction
        IsConstant         = 0x00000001, // Has CONST flag
        IsWritable         = 0x00000002, // Has WRITE function
        IsResettable       = 0x00000004, // Has RESET function
        IsAlias            = 0x00000008, // Is a QML alias to another property
        IsFinal            = 0x00000010, // Has FINAL flag
        IsDirect           = 0x00000020, // Exists on a C++ QMetaObject
        HasAccessors       = 0x00000040, // Has property accessors

        // These are mutualy exclusive
        IsFunction         = 0x00000080, // Is an invokable
        IsQObjectDerived   = 0x00000100, // Property type is a QObject* derived type
        IsEnumType         = 0x00000200, // Property type is an enum
        IsQList            = 0x00000400, // Property type is a QML list
        IsQmlBinding       = 0x00000800, // Property type is a QQmlBinding*
        IsQJSValue         = 0x00001000, // Property type is a QScriptValue
        IsV8Handle         = 0x00002000, // Property type is a QQmlV8Handle
        IsVMEProperty      = 0x00004000, // Property type is a "var" property of VMEMO
        IsValueTypeVirtual = 0x00008000, // Property is a value type "virtual" property
        IsQVariant         = 0x00010000, // Property is a QVariant

        // Apply only to IsFunctions
        IsVMEFunction      = 0x00020000, // Function was added by QML
        HasArguments       = 0x00040000, // Function takes arguments
        IsSignal           = 0x00080000, // Function is a signal
        IsVMESignal        = 0x00100000, // Signal was added by QML
        IsV8Function       = 0x00200000, // Function takes QQmlV8Function* args
        IsSignalHandler    = 0x00400000, // Function is a signal handler
        IsOverload         = 0x00800000, // Function is an overload of another function

        // Internal QQmlPropertyCache flags
        NotFullyResolved   = 0x01000000  // True if the type data is to be lazily resolved
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    Flags getFlags() const { return Flag(flags); }
    void setFlags(Flags f) { flags = f; }

    bool isValid() const { return coreIndex != -1; }

    bool isConstant() const { return flags & IsConstant; }
    bool isWritable() const { return flags & IsWritable; }
    bool isResettable() const { return flags & IsResettable; }
    bool isAlias() const { return flags & IsAlias; }
    bool isFinal() const { return flags & IsFinal; }
    bool isDirect() const { return flags & IsDirect; }
    bool hasAccessors() const { return flags & HasAccessors; }
    bool isFunction() const { return flags & IsFunction; }
    bool isQObject() const { return flags & IsQObjectDerived; }
    bool isEnum() const { return flags & IsEnumType; }
    bool isQList() const { return flags & IsQList; }
    bool isQmlBinding() const { return flags & IsQmlBinding; }
    bool isQJSValue() const { return flags & IsQJSValue; }
    bool isV8Handle() const { return flags & IsV8Handle; }
    bool isVMEProperty() const { return flags & IsVMEProperty; }
    bool isValueTypeVirtual() const { return flags & IsValueTypeVirtual; }
    bool isQVariant() const { return flags & IsQVariant; }
    bool isVMEFunction() const { return flags & IsVMEFunction; }
    bool hasArguments() const { return flags & HasArguments; }
    bool isSignal() const { return flags & IsSignal; }
    bool isVMESignal() const { return flags & IsVMESignal; }
    bool isV8Function() const { return flags & IsV8Function; }
    bool isSignalHandler() const { return flags & IsSignalHandler; }
    bool isOverload() const { return flags & IsOverload; }

    bool hasOverride() const { return !(flags & IsValueTypeVirtual) &&
                                      !(flags & HasAccessors) &&
                                      overrideIndex >= 0; }
    bool hasRevision() const { return !(flags & HasAccessors) && revision != 0; }

    // Returns -1 if not a value type virtual property
    inline int getValueTypeCoreIndex() const;

    // Returns the "encoded" index for use with bindings.  Encoding is:
    //     coreIndex | (valueTypeCoreIndex << 24)
    inline int encodedIndex() const;

    union {
        int propType;             // When !NotFullyResolved
        const char *propTypeName; // When NotFullyResolved
    };
    int coreIndex;
    union {
        int notifyIndex;  // When !IsFunction
        void *arguments;  // When IsFunction && HasArguments
    };

    union {
        struct { // When !HasAccessors
            qint16 revision;
            qint16 metaObjectOffset;

            union {
                struct { // When IsValueTypeVirtual
                    quint16 valueTypeFlags; // flags of the access property on the value type proxy
                                            // object
                    quint8 valueTypePropType; // The QVariant::Type of access property on the value
                                              // type proxy object
                    quint8 valueTypeCoreIndex; // The prop index of the access property on the value
                                               // type proxy object
                };

                struct { // When !IsValueTypeVirtual
                    uint overrideIndexIsProperty : 1;
                    signed int overrideIndex : 31;
                };
            };
        };
        struct { // When HasAccessors
            QQmlAccessors *accessors;
            intptr_t accessorData;
        };
    };

private:
    friend class QQmlPropertyData;
    friend class QQmlPropertyCache;
    quint32 flags;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlPropertyRawData::Flags);

class QQmlPropertyData : public QQmlPropertyRawData
{
public:
    inline QQmlPropertyData();
    inline QQmlPropertyData(const QQmlPropertyRawData &);

    inline bool operator==(const QQmlPropertyRawData &);

    static Flags flagsForProperty(const QMetaProperty &, QQmlEngine *engine = 0);
    void load(const QMetaProperty &, QQmlEngine *engine = 0);
    void load(const QMetaMethod &);
    QString name(QObject *);
    QString name(const QMetaObject *);

private:
    friend class QQmlPropertyCache;
    void lazyLoad(const QMetaProperty &, QQmlEngine *engine = 0);
    void lazyLoad(const QMetaMethod &);
    bool notFullyResolved() const { return flags & NotFullyResolved; }
};

class Q_QML_PRIVATE_EXPORT QQmlPropertyCache : public QQmlRefCount, public QQmlCleanup
{
public:
    QQmlPropertyCache(QQmlEngine *);
    QQmlPropertyCache(QQmlEngine *, const QMetaObject *);
    virtual ~QQmlPropertyCache();

    void update(QQmlEngine *, const QMetaObject *);

    QQmlPropertyCache *copy();

    QQmlPropertyCache *copyAndAppend(QQmlEngine *, const QMetaObject *,
                QQmlPropertyData::Flag propertyFlags = QQmlPropertyData::NoFlags,
                QQmlPropertyData::Flag methodFlags = QQmlPropertyData::NoFlags,
                QQmlPropertyData::Flag signalFlags = QQmlPropertyData::NoFlags);
    QQmlPropertyCache *copyAndAppend(QQmlEngine *, const QMetaObject *, int revision,
                QQmlPropertyData::Flag propertyFlags = QQmlPropertyData::NoFlags,
                QQmlPropertyData::Flag methodFlags = QQmlPropertyData::NoFlags,
                QQmlPropertyData::Flag signalFlags = QQmlPropertyData::NoFlags);

    void append(QQmlEngine *, const QMetaObject *,
                QQmlPropertyData::Flag propertyFlags = QQmlPropertyData::NoFlags,
                QQmlPropertyData::Flag methodFlags = QQmlPropertyData::NoFlags,
                QQmlPropertyData::Flag signalFlags = QQmlPropertyData::NoFlags);
    void append(QQmlEngine *, const QMetaObject *, int revision,
                QQmlPropertyData::Flag propertyFlags = QQmlPropertyData::NoFlags,
                QQmlPropertyData::Flag methodFlags = QQmlPropertyData::NoFlags,
                QQmlPropertyData::Flag signalFlags = QQmlPropertyData::NoFlags);

    inline QQmlPropertyData *property(const QHashedV8String &) const;
    QQmlPropertyData *property(const QHashedStringRef &) const;
    QQmlPropertyData *property(const QHashedCStringRef &) const;
    QQmlPropertyData *property(const QString &) const;
    QQmlPropertyData *property(int) const;
    QQmlPropertyData *method(int) const;
    QStringList propertyNames() const;

    inline QQmlPropertyData *overrideData(QQmlPropertyData *) const;
    inline bool isAllowedInRevision(QQmlPropertyData *) const;

    inline QQmlEngine *qmlEngine() const;
    static QQmlPropertyData *property(QQmlEngine *, QObject *, const QString &,
                                              QQmlPropertyData &);
    static QQmlPropertyData *property(QQmlEngine *, QObject *, const QHashedV8String &,
                                              QQmlPropertyData &);
    static int *methodParameterTypes(QObject *, int index, QVarLengthArray<int, 9> &dummy,
                                     QByteArray *unknownTypeError);

    static bool isDynamicMetaObject(const QMetaObject *);
protected:
    virtual void destroy();
    virtual void clear();

private:
    friend class QQmlEnginePrivate;
    friend class QV8QObjectWrapper;

    inline QQmlPropertyCache *copy(int reserve);

    // Implemented in v8/qv8qobjectwrapper.cpp
    v8::Local<v8::Object> newQObject(QObject *, QV8Engine *);

    typedef QVector<QQmlPropertyData> IndexCache;
    typedef QStringHash<QQmlPropertyData *> StringCache;
    typedef QVector<int> AllowedRevisionCache;

    void resolve(QQmlPropertyData *) const;
    void updateRecur(QQmlEngine *, const QMetaObject *);

    QQmlEngine *engine;
    
    QQmlPropertyCache *parent;
    int propertyIndexCacheStart;
    int methodIndexCacheStart;
    int signalHanderIndexCacheStart;

    IndexCache propertyIndexCache;
    IndexCache methodIndexCache;
    IndexCache signalHandlerIndexCache;
    StringCache stringCache;
    AllowedRevisionCache allowedRevisionCache;
    v8::Persistent<v8::Function> constructor;

    const QMetaObject *metaObject;
    QQmlPropertyCacheMethodArguments *argumentsCache;
};
  
QQmlPropertyData::QQmlPropertyData()
{
    propType = 0;
    coreIndex = -1;
    notifyIndex = -1;
    overrideIndexIsProperty = false;
    overrideIndex = -1;
    revision = 0;
    metaObjectOffset = -1; 
    flags = 0;
}

QQmlPropertyData::QQmlPropertyData(const QQmlPropertyRawData &d)
{
    *(static_cast<QQmlPropertyRawData *>(this)) = d;
}

bool QQmlPropertyData::operator==(const QQmlPropertyRawData &other)
{
    return flags == other.flags &&
           propType == other.propType &&
           coreIndex == other.coreIndex &&
           notifyIndex == other.notifyIndex &&
           revision == other.revision &&
           (!isValueTypeVirtual() || 
            (valueTypeCoreIndex == other.valueTypeCoreIndex && 
             valueTypePropType == other.valueTypePropType));
}

int QQmlPropertyRawData::getValueTypeCoreIndex() const
{
    return isValueTypeVirtual()?valueTypeCoreIndex:-1;
}

int QQmlPropertyRawData::encodedIndex() const
{
    return isValueTypeVirtual()?(coreIndex | (valueTypeCoreIndex << 24)):coreIndex;
}

QQmlPropertyData *
QQmlPropertyCache::overrideData(QQmlPropertyData *data) const
{
    if (!data->hasOverride())
        return 0;

    if (data->overrideIndexIsProperty)
        return property(data->overrideIndex);
    else
        return method(data->overrideIndex);
}

bool QQmlPropertyCache::isAllowedInRevision(QQmlPropertyData *data) const
{
    return (data->hasAccessors() || (data->metaObjectOffset == -1 && data->revision == 0)) ||
           (allowedRevisionCache[data->metaObjectOffset] >= data->revision);
}

QQmlEngine *QQmlPropertyCache::qmlEngine() const
{
    return engine;
}

QQmlPropertyData *QQmlPropertyCache::property(const QHashedV8String &str) const
{
    QQmlPropertyData **rv = stringCache.value(str);
    if (rv && (*rv)->notFullyResolved()) resolve(*rv);
    return rv?*rv:0;
}

QT_END_NAMESPACE

#endif // QQMLPROPERTYCACHE_P_H
