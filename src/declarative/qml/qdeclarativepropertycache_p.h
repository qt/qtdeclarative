/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEPROPERTYCACHE_P_H
#define QDECLARATIVEPROPERTYCACHE_P_H

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

#include <private/qdeclarativerefcount_p.h>
#include "qdeclarativecleanup_p.h"
#include "qdeclarativenotifier_p.h"

#include <private/qhashedstring_p.h>
#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

class QDeclarativeEngine;
class QMetaProperty;
class QV8Engine;
class QV8QObjectWrapper;

class Q_DECLARATIVE_EXPORT QDeclarativePropertyCache : public QDeclarativeRefCount, public QDeclarativeCleanup
{
public:
    QDeclarativePropertyCache(QDeclarativeEngine *);
    QDeclarativePropertyCache(QDeclarativeEngine *, const QMetaObject *);
    virtual ~QDeclarativePropertyCache();

    // We have this somewhat aweful split between RawData and Data so that RawData can be
    // used in unions.  In normal code, you should always use Data which initializes RawData
    // to an invalid state on construction.
    struct Data;
    struct RawData {
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

                    // These are mutualy exclusive
                    IsFunction         = 0x00000040, // Is an invokable
                    IsQObjectDerived   = 0x00000080, // Property type is a QObject* derived type
                    IsEnumType         = 0x00000100, // Property type is an enum
                    IsQList            = 0x00000200, // Property type is a QML list
                    IsQmlBinding       = 0x00000400, // Property type is a QDeclarativeBinding*
                    IsQJSValue         = 0x00000800, // Property type is a QScriptValue
                    IsV8Handle         = 0x00001000, // Property type is a QDeclarativeV8Handle
                    IsVMEProperty      = 0x00002000, // Property type is a "var" property of VMEMO
                    IsValueTypeVirtual = 0x00004000, // Property is a value type "virtual" property

                    // Apply only to IsFunctions
                    IsVMEFunction      = 0x00008000, // Function was added by QML
                    HasArguments       = 0x00010000, // Function takes arguments
                    IsSignal           = 0x00020000, // Function is a signal
                    IsVMESignal        = 0x00040000, // Signal was added by QML
                    IsV8Function       = 0x00080000, // Function takes QDeclarativeV8Function* args
                    IsSignalHandler    = 0x00100000, // Function is a signal handler

                    // Internal QDeclarativePropertyCache flags
                    NotFullyResolved   = 0x00200000  // True if the type data is to be lazily resolved
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
        bool isFunction() const { return flags & IsFunction; }
        bool isQObject() const { return flags & IsQObjectDerived; }
        bool isEnum() const { return flags & IsEnumType; }
        bool isQList() const { return flags & IsQList; }
        bool isQmlBinding() const { return flags & IsQmlBinding; }
        bool isQJSValue() const { return flags & IsQJSValue; }
        bool isV8Handle() const { return flags & IsV8Handle; }
        bool isVMEProperty() const { return flags & IsVMEProperty; }
        bool isValueTypeVirtual() const { return flags & IsValueTypeVirtual; }
        bool isVMEFunction() const { return flags & IsVMEFunction; }
        bool hasArguments() const { return flags & HasArguments; }
        bool isSignal() const { return flags & IsSignal; }
        bool isVMESignal() const { return flags & IsVMESignal; }
        bool isV8Function() const { return flags & IsV8Function; }
        bool isSignalHandler() const { return flags & IsSignalHandler; }

        union {
            int propType;             // When !NotFullyResolved
            const char *propTypeName; // When NotFullyResolved
        };
        int coreIndex;
        union {
            int notifyIndex;  // When !IsFunction
            int relatedIndex; // When IsFunction
        };
        union {
            struct { // When !IsValueTypeVirtual
                uint overrideIndexIsProperty : 1;
                signed int overrideIndex : 31;
            };
            struct { // When IsValueTypeVirtual
                quint16 valueTypeFlags; // flags of the access property on the value type proxy object
                quint8 valueTypePropType; // The QVariant::Type of access property on the value type 
                                          // proxy object
                quint8 valueTypeCoreIndex; // The prop index of the access property on the value type 
                                           //proxy object
            };
        };
        int revision; 
        int metaObjectOffset;

    private:
        friend struct Data;
        friend class QDeclarativePropertyCache;
        quint32 flags;
    };

    struct Data : public RawData {
        inline Data(); 
        inline Data(const RawData &); 

        inline bool operator==(const RawData &);

        static Flags flagsForProperty(const QMetaProperty &, QDeclarativeEngine *engine = 0);
        void load(const QMetaProperty &, QDeclarativeEngine *engine = 0);
        void load(const QMetaMethod &);
        QString name(QObject *);
        QString name(const QMetaObject *);

        // Returns -1 if not a value type virtual property
        inline int getValueTypeCoreIndex() const;

    private:
        friend class QDeclarativePropertyCache;
        void lazyLoad(const QMetaProperty &, QDeclarativeEngine *engine = 0);
        void lazyLoad(const QMetaMethod &);
        bool notFullyResolved() const { return flags & NotFullyResolved; }
    };

    void update(QDeclarativeEngine *, const QMetaObject *);

    QDeclarativePropertyCache *copy(int reserve = 0);
    void append(QDeclarativeEngine *, const QMetaObject *, Data::Flag propertyFlags = Data::NoFlags,
                Data::Flag methodFlags = Data::NoFlags, Data::Flag signalFlags = Data::NoFlags);
    void append(QDeclarativeEngine *, const QMetaObject *, int revision, Data::Flag propertyFlags = Data::NoFlags,
                Data::Flag methodFlags = Data::NoFlags, Data::Flag signalFlags = Data::NoFlags);

    static Data create(const QMetaObject *, const QString &);

    inline Data *property(const QHashedV8String &) const;
    Data *property(const QHashedStringRef &) const;
    Data *property(const QHashedCStringRef &) const;
    Data *property(const QString &) const;
    Data *property(int) const;
    Data *method(int) const;
    QStringList propertyNames() const;

    inline Data *overrideData(Data *) const;
    inline bool isAllowedInRevision(Data *) const;

    inline QDeclarativeEngine *qmlEngine() const;
    static Data *property(QDeclarativeEngine *, QObject *, const QString &, Data &);
    static Data *property(QDeclarativeEngine *, QObject *, const QHashedV8String &, Data &);

    static bool isDynamicMetaObject(const QMetaObject *);

protected:
    virtual void destroy();
    virtual void clear();

private:
    friend class QDeclarativeEnginePrivate;
    friend class QV8QObjectWrapper;

    // Implemented in v8/qv8qobjectwrapper.cpp
    v8::Local<v8::Object> newQObject(QObject *, QV8Engine *);

    typedef QVector<Data> IndexCache;
    typedef QStringHash<Data *> StringCache;
    typedef QVector<int> AllowedRevisionCache;

    void resolve(Data *) const;
    void updateRecur(QDeclarativeEngine *, const QMetaObject *);

    QDeclarativeEngine *engine;
    
    QDeclarativePropertyCache *parent;
    int propertyIndexCacheStart;
    int methodIndexCacheStart;

    IndexCache propertyIndexCache;
    IndexCache methodIndexCache;
    IndexCache signalHandlerIndexCache;
    StringCache stringCache;
    AllowedRevisionCache allowedRevisionCache;
    v8::Persistent<v8::Function> constructor;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QDeclarativePropertyCache::Data::Flags);
  
QDeclarativePropertyCache::Data::Data()
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

QDeclarativePropertyCache::Data::Data(const QDeclarativePropertyCache::RawData &d)
{
    *(static_cast<RawData *>(this)) = d;
}

bool QDeclarativePropertyCache::Data::operator==(const QDeclarativePropertyCache::RawData &other)
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

int QDeclarativePropertyCache::Data::getValueTypeCoreIndex() const
{
    return isValueTypeVirtual()?valueTypeCoreIndex:-1;
}

QDeclarativePropertyCache::Data *
QDeclarativePropertyCache::overrideData(Data *data) const
{
    if (data->overrideIndex < 0)
        return 0;

    if (data->overrideIndexIsProperty)
        return property(data->overrideIndex);
    else
        return method(data->overrideIndex);
}

bool QDeclarativePropertyCache::isAllowedInRevision(Data *data) const
{
    return (data->metaObjectOffset == -1 && data->revision == 0) ||
           (allowedRevisionCache[data->metaObjectOffset] >= data->revision);
}

QDeclarativeEngine *QDeclarativePropertyCache::qmlEngine() const
{
    return engine;
}

QDeclarativePropertyCache::Data *QDeclarativePropertyCache::property(const QHashedV8String &str) const
{
    QDeclarativePropertyCache::Data **rv = stringCache.value(str);
    if (rv && (*rv)->notFullyResolved()) resolve(*rv);
    return rv?*rv:0;
}

QT_END_NAMESPACE

#endif // QDECLARATIVEPROPERTYCACHE_P_H
