/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQML_NATIVE_DEBUG_SERVICE_H
#define QQML_NATIVE_DEBUG_SERVICE_H

#include <private/qqmldebugconnector_p.h>
#include <private/qv4debugging_p.h>
#include <private/qv8engine_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4debugging_p.h>
#include <private/qv4script_p.h>
#include <private/qv4string_p.h>
#include <private/qv4objectiterator_p.h>
#include <private/qv4identifier_p.h>
#include <private/qv4runtime_p.h>
#include <private/qqmldebugserviceinterfaces_p.h>

#include <QtCore/qjsonarray.h>

#include <qqmlengine.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QVector>
#include <QPointer>

QT_BEGIN_NAMESPACE

class NativeDebugger;
class BreakPointHandler;
class QQmlDebuggerServiceFactory;

class QQmlNativeDebugServiceImpl : public QQmlNativeDebugService
{
public:
    QQmlNativeDebugServiceImpl(QObject *parent);

    ~QQmlNativeDebugServiceImpl();

    void engineAboutToBeAdded(QQmlEngine *engine);
    void engineAboutToBeRemoved(QQmlEngine *engine);

    void stateAboutToBeChanged(State state);

    void messageReceived(const QByteArray &message);

    void emitAsynchronousMessageToClient(const QJsonObject &message);

private:
    friend class QQmlDebuggerServiceFactory;
    friend class NativeDebugger;

    QList<QPointer<NativeDebugger> > m_debuggers;
    BreakPointHandler *m_breakHandler;
};

QT_END_NAMESPACE

#endif // QQML_NATIVE_DEBUG_SERVICE_H
