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

#ifndef QV8QOBJECTWRAPPER_P_H
#define QV8QOBJECTWRAPPER_P_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qpair.h>
#include <QtCore/qhash.h>
#include <private/qv8_p.h>
#include <private/qhashedstring_p.h>
#include <private/qqmldata_p.h>
#include <private/qqmlpropertycache_p.h>
#include <private/qintrusivelist_p.h>
#include "qv8objectresource_p.h"

QT_BEGIN_NAMESPACE

class QObject;
class QV8Engine;
class QQmlData;
class QV8ObjectResource;
class QV8QObjectInstance;
class QV8QObjectConnectionList;
class QQmlPropertyCache;

class QV8QObjectResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(QObjectType)

public:
    QV8QObjectResource(QV8Engine *engine, QObject *object);

    QQmlGuard<QObject> object;
    QIntrusiveListNode weakResource;
};

class Q_QML_PRIVATE_EXPORT QV8QObjectWrapper
{
public:
    QV8QObjectWrapper();
    ~QV8QObjectWrapper();

    void init(QV8Engine *);
    void destroy();

    v8::Handle<v8::Value> newQObject(QObject *object);
    bool isQObject(v8::Handle<v8::Object>);
    QObject *toQObject(v8::Handle<v8::Object>);
    static QObject *toQObject(QV8ObjectResource *);

    enum RevisionMode { IgnoreRevision, CheckRevision };
    inline v8::Handle<v8::Value> getProperty(QObject *, const QHashedV8String &, QQmlContextData *, RevisionMode);
    inline bool setProperty(QObject *, const QHashedV8String &, QQmlContextData *, v8::Handle<v8::Value>, RevisionMode);

    void registerWeakQObjectReference(QV8QObjectResource *resource)
    {
        m_javaScriptOwnedWeakQObjects.insert(resource);
    }

    void unregisterWeakQObjectReference(QV8QObjectResource *resource)
    {
        m_javaScriptOwnedWeakQObjects.remove(resource);
    }

private:
    friend class QQmlPropertyCache;
    friend class QV8QObjectConnectionList;
    friend class QV8QObjectInstance;

    v8::Local<v8::Object> newQObject(QObject *, QQmlData *, QV8Engine *);
    bool deleteWeakQObject(QV8QObjectResource *resource, bool calledFromEngineDtor = false);
    static v8::Handle<v8::Value> GetProperty(QV8Engine *, QObject *, v8::Handle<v8::Value> *, 
                                             const QHashedV8String &, QQmlContextData *, QV8QObjectWrapper::RevisionMode);
    static bool SetProperty(QV8Engine *, QObject *, const QHashedV8String &, QQmlContextData *,
                            v8::Handle<v8::Value>, QV8QObjectWrapper::RevisionMode);
    static v8::Handle<v8::Value> Getter(v8::Local<v8::String> property, 
                                        const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> Setter(v8::Local<v8::String> property, 
                                        v8::Local<v8::Value> value,
                                        const v8::AccessorInfo &info);
    static v8::Handle<v8::Integer> Query(v8::Local<v8::String> property,
                                         const v8::AccessorInfo &info);
    static v8::Handle<v8::Array> Enumerator(const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> Connect(const v8::Arguments &args);
    static v8::Handle<v8::Value> Disconnect(const v8::Arguments &args);
    static v8::Handle<v8::Value> Invoke(const v8::Arguments &args);
    static QPair<QObject *, int> ExtractQtMethod(QV8Engine *, v8::Handle<v8::Function>);
    static QPair<QObject *, int> ExtractQtSignal(QV8Engine *, v8::Handle<v8::Object>);
    static void WeakQObjectReferenceCallback(v8::Persistent<v8::Value> handle, void *wrapper);

    QV8Engine *m_engine;
    quint32 m_id;
    v8::Persistent<v8::Function> m_constructor;
    v8::Persistent<v8::Function> m_methodConstructor;
    v8::Persistent<v8::Function> m_signalHandlerConstructor;
    v8::Persistent<v8::String> m_toStringSymbol;
    v8::Persistent<v8::String> m_destroySymbol;
    QHashedV8String m_toStringString;
    QHashedV8String m_destroyString;
    v8::Persistent<v8::Object> m_hiddenObject;
    QHash<QObject *, QV8QObjectConnectionList *> m_connections;
    typedef QHash<QObject *, QV8QObjectInstance *> TaintedHash;
    TaintedHash m_taintedObjects;
    QIntrusiveList<QV8QObjectResource, &QV8QObjectResource::weakResource> m_javaScriptOwnedWeakQObjects;
};

v8::Handle<v8::Value> QV8QObjectWrapper::getProperty(QObject *object, const QHashedV8String &string,  
                                                     QQmlContextData *context, RevisionMode mode)
{
    QQmlData *dd = QQmlData::get(object, false);
    if (!dd || !dd->propertyCache || m_toStringString == string || m_destroyString == string ||
        dd->propertyCache->property(string, object, context)) {
        return GetProperty(m_engine, object, 0, string, context, mode);
    } else {
        return v8::Handle<v8::Value>();
    }
}

bool QV8QObjectWrapper::setProperty(QObject *object, const QHashedV8String &string, 
                                    QQmlContextData *context, v8::Handle<v8::Value> value, RevisionMode mode)
{
    QQmlData *dd = QQmlData::get(object, false);
    if (!dd || !dd->propertyCache || m_toStringString == string || m_destroyString == string ||
        dd->propertyCache->property(string, object, context)) {
        return SetProperty(m_engine, object, string, context, value, mode);
    } else {
        return false;
    }
}

QT_END_NAMESPACE

#endif // QV8QOBJECTWRAPPER_P_H


