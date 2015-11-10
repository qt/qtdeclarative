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

class VariableCollector;
class V8CommandHandler;
class UnknownV8CommandHandler;
class QV4DebugServiceImpl;

class QV4DebugServiceImpl : public QQmlConfigurableDebugService<QV4DebugService>
{
    Q_OBJECT
public:
    explicit QV4DebugServiceImpl(QObject *parent = 0);
    ~QV4DebugServiceImpl() Q_DECL_OVERRIDE;

    void engineAboutToBeAdded(QJSEngine *engine) Q_DECL_OVERRIDE;
    void engineAboutToBeRemoved(QJSEngine *engine) Q_DECL_OVERRIDE;

    void stateAboutToBeChanged(State state) Q_DECL_OVERRIDE;

    void signalEmitted(const QString &signal) Q_DECL_OVERRIDE;
    void send(QJsonObject v8Payload);

    QJsonObject buildScope(int frameNr, int scopeNr, QV4Debugger *debugger);
    QJsonArray buildRefs();
    QJsonValue lookup(QV4DataCollector::Ref refId);
    QJsonValue toRef(QV4DataCollector::Ref ref);

    QJsonObject buildFrame(const QV4::StackFrame &stackFrame, int frameNr, QV4Debugger *debugger);
    int selectedFrame() const;
    void selectFrame(int frameNr);

    void clearHandles(QV4::ExecutionEngine *engine);

    QV4DataCollector *collector() const;
    QV4DebuggerAgent debuggerAgent;
    QV4DataCollector::Refs *refs();

protected:
    void messageReceived(const QByteArray &) Q_DECL_OVERRIDE;
    void sendSomethingToSomebody(const char *type, int magicNumber = 1);

private:
    friend class QQmlDebuggerServiceFactory;

    void handleV8Request(const QByteArray &payload);
    static QByteArray packMessage(const QByteArray &command,
                                  const QByteArray &message = QByteArray());
    void processCommand(const QByteArray &command, const QByteArray &data);
    V8CommandHandler *v8CommandHandler(const QString &command) const;
    int encodeScopeType(QV4::Heap::ExecutionContext::ContextType scopeType);

    QStringList breakOnSignals;
    static int sequence;
    QV4DataCollector::Refs collectedRefs;

    QScopedPointer<QV4DataCollector> theCollector;
    int theSelectedFrame;

    void addHandler(V8CommandHandler* handler);
    QHash<QString, V8CommandHandler*> handlers;
    QScopedPointer<UnknownV8CommandHandler> unknownV8CommandHandler;
};

QT_END_NAMESPACE

#endif // QV4DEBUGSERVICE_H
