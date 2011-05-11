/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
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

#include "private/qdeclarativerefcount_p.h"
#include "private/qdeclarativecleanup_p.h"
#include "private/qdeclarativenotifier_p.h"

#include "private/qhashedstring_p.h"
#include <QtCore/qvector.h>

#include <QtScript/private/qscriptdeclarativeclass_p.h>
QT_BEGIN_NAMESPACE

class QDeclarativeEngine;
class QMetaProperty;
class QV8Engine;
class QV8QObjectWrapper;

class Q_AUTOTEST_EXPORT QDeclarativePropertyCache : public QDeclarativeRefCount, public QDeclarativeCleanup
{
public:
    QDeclarativePropertyCache(QDeclarativeEngine *);
    QDeclarativePropertyCache(QDeclarativeEngine *, const QMetaObject *);
    virtual ~QDeclarativePropertyCache();

    struct Data {
        inline Data(); 
        inline bool operator==(const Data &);

        enum Flag { 
                    NoFlags           = 0x00000000,

                    // Can apply to all properties, except IsFunction
                    IsConstant        = 0x00000001,
                    IsWritable        = 0x00000002,
                    IsResettable      = 0x00000004,
                    IsAlias           = 0x00000008,
                    IsFinal           = 0x00000010,

                    // These are mutualy exclusive
                    IsFunction        = 0x00000020,
                    IsQObjectDerived  = 0x00000040,
                    IsEnumType        = 0x00000080,
                    IsQList           = 0x00000100,
                    IsQmlBinding      = 0x00000200,
                    IsQScriptValue    = 0x00000400,
                    IsV8Handle        = 0x00000800,

                    // Apply only to IsFunctions
                    IsVMEFunction     = 0x00001000,
                    HasArguments      = 0x00002000,
                    IsSignal          = 0x00004000,
                    IsVMESignal       = 0x00008000,
                    IsV8Function      = 0x00010000
        };
        Q_DECLARE_FLAGS(Flags, Flag)

        bool isValid() const { return coreIndex != -1; } 
        bool isConstant() const { return flags & IsConstant; }
        bool isWritable() const { return flags & IsWritable; }
        bool isResettable() const { return flags & IsResettable; }
        bool isAlias() const { return flags & IsAlias; }
        bool isFinal() const { return flags & IsFinal; }
        bool isFunction() const { return flags & IsFunction; }
        bool isQObject() const { return flags & IsQObjectDerived; }
        bool isEnum() const { return flags & IsEnumType; }
        bool isQList() const { return flags & IsQList; }
        bool isQmlBinding() const { return flags & IsQmlBinding; }
        bool isQScriptValue() const { return flags & IsQScriptValue; }
        bool isV8Handle() const { return flags & IsV8Handle; }

        Flags flags;
        int propType;
        int coreIndex;
        union {
            int notifyIndex; // When !IsFunction
            int relatedIndex; // When IsFunction
        };
        uint overrideIndexIsProperty : 1;
        int overrideIndex : 31;
        int revision; 
        int metaObjectOffset;

        static Flags flagsForProperty(const QMetaProperty &, QDeclarativeEngine *engine = 0);
        void load(const QMetaProperty &, QDeclarativeEngine *engine = 0);
        void load(const QMetaMethod &);
        QString name(QObject *);
        QString name(const QMetaObject *);
    };

    struct ValueTypeData {
        inline ValueTypeData();
        inline bool operator==(const ValueTypeData &);
        Data::Flags flags;     // flags of the access property on the value type proxy object
        int valueTypeCoreIdx;  // The prop index of the access property on the value type proxy object
        int valueTypePropType; // The QVariant::Type of access property on the value type proxy object
    };

    void update(QDeclarativeEngine *, const QMetaObject *);

    QDeclarativePropertyCache *copy() const;
    void append(QDeclarativeEngine *, const QMetaObject *, Data::Flag propertyFlags = Data::NoFlags,
                Data::Flag methodFlags = Data::NoFlags, Data::Flag signalFlags = Data::NoFlags);
    void append(QDeclarativeEngine *, const QMetaObject *, int revision, Data::Flag propertyFlags = Data::NoFlags,
                Data::Flag methodFlags = Data::NoFlags, Data::Flag signalFlags = Data::NoFlags);

    static Data create(const QMetaObject *, const QString &);

    inline Data *property(v8::Handle<v8::String>) const;
    Data *property(const QString &) const;
    Data *property(int) const;
    Data *method(int) const;
    QStringList propertyNames() const;

    inline Data *overrideData(Data *) const;
    inline bool isAllowedInRevision(Data *) const;

    inline QDeclarativeEngine *qmlEngine() const;
    static Data *property(QDeclarativeEngine *, QObject *, const QString &, Data &);
    static Data *property(QDeclarativeEngine *, QObject *, v8::Handle<v8::String>, Data &);

protected:
    virtual void clear();

private:
    friend class QDeclarativeEnginePrivate;
    friend class QV8QObjectWrapper;

    // Implemented in v8/qv8qobjectwrapper.cpp
    v8::Local<v8::Object> newQObject(QObject *, QV8Engine *);

    // XXX is this worth it anymore?
    struct RData : public Data, public QDeclarativeRefCount { 
    };

    typedef QVector<RData *> IndexCache;
    typedef QStringHash<RData *> StringCache;
    typedef QVector<int> AllowedRevisionCache;

    void updateRecur(QDeclarativeEngine *, const QMetaObject *);

    QDeclarativeEngine *engine;
    IndexCache indexCache;
    IndexCache methodIndexCache;
    StringCache stringCache;
    AllowedRevisionCache allowedRevisionCache;
    v8::Persistent<v8::Function> constructor;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QDeclarativePropertyCache::Data::Flags);
  
QDeclarativePropertyCache::Data::Data()
: flags(0), propType(0), coreIndex(-1), notifyIndex(-1), overrideIndexIsProperty(false), overrideIndex(-1),
  revision(0), metaObjectOffset(-1)
{
}

bool QDeclarativePropertyCache::Data::operator==(const QDeclarativePropertyCache::Data &other)
{
    return flags == other.flags &&
           propType == other.propType &&
           coreIndex == other.coreIndex &&
           notifyIndex == other.notifyIndex &&
           revision == other.revision;
}

QDeclarativePropertyCache::Data *
QDeclarativePropertyCache::overrideData(Data *data) const
{
    if (data->overrideIndex < 0)
        return 0;

    if (data->overrideIndexIsProperty)
        return indexCache.at(data->overrideIndex);
    else
        return methodIndexCache.at(data->overrideIndex);
}

QDeclarativePropertyCache::ValueTypeData::ValueTypeData()
: flags(QDeclarativePropertyCache::Data::NoFlags), valueTypeCoreIdx(-1), valueTypePropType(0) 
{
}

bool QDeclarativePropertyCache::ValueTypeData::operator==(const ValueTypeData &o) 
{ 
    return flags == o.flags &&
           valueTypeCoreIdx == o.valueTypeCoreIdx &&
           valueTypePropType == o.valueTypePropType; 
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

QDeclarativePropertyCache::Data *QDeclarativePropertyCache::property(v8::Handle<v8::String> str) const
{
    QDeclarativePropertyCache::RData **rv = stringCache.value(str);
    return rv?*rv:0;
}

QT_END_NAMESPACE

#endif // QDECLARATIVEPROPERTYCACHE_P_H
