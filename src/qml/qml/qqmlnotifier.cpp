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

#include "qqmlnotifier_p.h"
#include "qqmlproperty_p.h"
#include <QtCore/qdebug.h>
#include <private/qthread_p.h>

QT_BEGIN_NAMESPACE

void QQmlNotifier::emitNotify(QQmlNotifierEndpoint *endpoint, void **a)
{
    QQmlNotifierEndpoint **oldDisconnected = endpoint->disconnected;
    endpoint->disconnected = &endpoint;
    endpoint->notifying = 1;

    if (endpoint->next)
        emitNotify(endpoint->next, a);

    if (endpoint) {

        Q_ASSERT(endpoint->callback);
        
        endpoint->callback(endpoint, a);

        if (endpoint) 
            endpoint->disconnected = oldDisconnected;
    } 

    if (oldDisconnected) *oldDisconnected = endpoint;
    else if (endpoint) endpoint->notifying = 0;
}

void QQmlNotifierEndpoint::connect(QObject *source, int sourceSignal, QQmlEngine *engine)
{
    disconnect();

    Q_ASSERT(engine);
    if (QObjectPrivate::get(source)->threadData->threadId !=
        QObjectPrivate::get(engine)->threadData->threadId) {

        QString sourceName;
        QDebug(&sourceName) << source;
        sourceName = sourceName.left(sourceName.length() - 1);
        QString engineName;
        QDebug(&engineName).nospace() << engine;
        engineName = engineName.left(engineName.length() - 1);

        qFatal("QQmlEngine: Illegal attempt to connect to %s that is in"
               " a different thread than the QML engine %s.", qPrintable(sourceName),
               qPrintable(engineName));
    }

    this->source = source;
    this->sourceSignal = sourceSignal;
    QQmlPropertyPrivate::flushSignal(source, sourceSignal);
    QQmlData *ddata = QQmlData::get(source, true);
    ddata->addNotify(sourceSignal, this);
}

QT_END_NAMESPACE

