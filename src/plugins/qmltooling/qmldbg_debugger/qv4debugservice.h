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

#ifndef QV4DEBUGSERVICE_H
#define QV4DEBUGSERVICE_H

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

#include "qqmlconfigurabledebugservice.h"
#include "qv4debuggeragent.h"
#include "qv4datacollector.h"
#include <private/qqmldebugserviceinterfaces_p.h>
#include <private/qv4debugging_p.h>

#include <QtCore/QJsonValue>

QT_BEGIN_NAMESPACE

namespace QV4 { struct ExecutionEngine; }

class QQmlEngine;
class VariableCollector;
class V8CommandHandler;
class UnknownV8CommandHandler;
class QV4DebugServiceImpl;

class QV4DebugServiceImpl : public QQmlConfigurableDebugService<QV4DebugService>
{
    Q_OBJECT
public:
    explicit QV4DebugServiceImpl(QObject *parent = 0);
    ~QV4DebugServiceImpl();

    void engineAdded(QQmlEngine *engine);
    void engineAboutToBeRemoved(QQmlEngine *engine);

    void stateAboutToBeChanged(State state);

    void signalEmitted(const QString &signal);
    void send(QJsonObject v8Payload);

    int selectedFrame() const;
    void selectFrame(int frameNr);

    void clearHandles(QV4::ExecutionEngine *engine);

    QV4DataCollector *collector() const;
    QV4DebuggerAgent debuggerAgent;

protected:
    void messageReceived(const QByteArray &);
    void sendSomethingToSomebody(const char *type, int magicNumber = 1);

private:
    friend class QQmlDebuggerServiceFactory;

    void handleV8Request(const QByteArray &payload);
    static QByteArray packMessage(const QByteArray &command,
                                  const QByteArray &message = QByteArray());
    void processCommand(const QByteArray &command, const QByteArray &data);
    V8CommandHandler *v8CommandHandler(const QString &command) const;

    QStringList breakOnSignals;
    QMap<int, QV4::Debugging::V4Debugger *> debuggerMap;
    static int debuggerIndex;
    static int sequence;
    const int version;

    QScopedPointer<QV4DataCollector> theCollector;
    int theSelectedFrame;

    void addHandler(V8CommandHandler* handler);
    QHash<QString, V8CommandHandler*> handlers;
    QScopedPointer<UnknownV8CommandHandler> unknownV8CommandHandler;
};

QT_END_NAMESPACE

#endif // QV4DEBUGSERVICE_H
