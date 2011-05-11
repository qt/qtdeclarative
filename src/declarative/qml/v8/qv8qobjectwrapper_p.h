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

QT_BEGIN_NAMESPACE

class QObject;
class QV8Engine;
class QV8ObjectResource;
class QDeclarativePropertyCache;
class QV8QObjectConnectionList;
class Q_AUTOTEST_EXPORT QV8QObjectWrapper 
{
public:
    QV8QObjectWrapper();
    ~QV8QObjectWrapper();

    void init(QV8Engine *);
    void destroy();

    v8::Handle<v8::Value> newQObject(QObject *object);
    bool isQObject(v8::Handle<v8::Object>);
    QObject *toQObject(v8::Handle<v8::Object>);
    QObject *toQObject(QV8ObjectResource *);

    enum RevisionMode { IgnoreRevision, CheckRevision };
    v8::Handle<v8::Value> getProperty(QObject *, v8::Handle<v8::String>, RevisionMode);
    bool setProperty(QObject *, v8::Handle<v8::String>, v8::Handle<v8::Value>, RevisionMode);

private:
    friend class QDeclarativePropertyCache;
    friend class QV8QObjectConnectionList;

    static v8::Handle<v8::Value> GetProperty(QV8Engine *, QObject *, v8::Handle<v8::Value> *, 
                                             v8::Handle<v8::String>, QV8QObjectWrapper::RevisionMode);
    static bool SetProperty(QV8Engine *, QObject *, v8::Handle<v8::String>,
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

    QV8Engine *m_engine;
    v8::Persistent<v8::Function> m_constructor;
    v8::Persistent<v8::Function> m_methodConstructor;
    v8::Persistent<v8::String> m_toStringSymbol;
    v8::Persistent<v8::String> m_destroySymbol;
    v8::Persistent<v8::Object> m_hiddenObject;
    QHash<QObject *, QV8QObjectConnectionList *> m_connections;
};

QT_END_NAMESPACE

#endif // QV8QOBJECTWRAPPER_P_H


