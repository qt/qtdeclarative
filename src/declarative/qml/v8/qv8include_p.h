/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QV8INCLUDE_P_H
#define QV8INCLUDE_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>

#include <private/qdeclarativecontext_p.h>
#include <private/qdeclarativeguard_p.h>

#include <private/qv8_p.h>

QT_BEGIN_NAMESPACE

class QDeclarativeEngine;
class QNetworkAccessManager;
class QNetworkReply;
class QV8Engine;
class QV8Include : public QObject
{
    Q_OBJECT
public:
    enum Status {
        Ok = 0,
        Loading = 1,
        NetworkError = 2,
        Exception = 3
    };

    static v8::Handle<v8::Value> include(const v8::Arguments &args);

private slots:
    void finished();

private:
    QV8Include(const QUrl &, QV8Engine *, QDeclarativeContextData *,
               v8::Handle<v8::Object>, v8::Handle<v8::Function>);
    ~QV8Include();

    v8::Handle<v8::Object> result();

    static v8::Local<v8::Object> resultValue(Status status = Loading);
    static void callback(QV8Engine *engine, v8::Handle<v8::Function> callback, v8::Handle<v8::Object> status);

    QV8Engine *m_engine;
    QNetworkAccessManager *m_network;
    QDeclarativeGuard<QNetworkReply> m_reply;

    QUrl m_url;
    int m_redirectCount;

    v8::Persistent<v8::Function> m_callbackFunction;
    v8::Persistent<v8::Object> m_resultObject;

    QDeclarativeGuardedContextData m_context;
    v8::Persistent<v8::Object> m_qmlglobal;
};

QT_END_NAMESPACE

#endif // QV8INCLUDE_P_H

