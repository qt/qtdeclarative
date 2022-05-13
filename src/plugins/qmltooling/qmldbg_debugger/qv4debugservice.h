// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include "qv4debuggeragent.h"
#include "qv4datacollector.h"
#include <private/qqmlconfigurabledebugservice_p.h>
#include <private/qqmldebugserviceinterfaces_p.h>
#include <private/qv4debugging_p.h>

#include <QtCore/QJsonValue>

QT_BEGIN_NAMESPACE

namespace QV4 { struct ExecutionEngine; }

class VariableCollector;
class V4CommandHandler;
class UnknownV4CommandHandler;
class QV4DebugServiceImpl;

class QV4DebugServiceImpl : public QQmlConfigurableDebugService<QV4DebugService>
{
    Q_OBJECT
public:
    explicit QV4DebugServiceImpl(QObject *parent = nullptr);
    ~QV4DebugServiceImpl() override;

    void engineAdded(QJSEngine *engine) override;
    void engineAboutToBeRemoved(QJSEngine *engine) override;

    void stateAboutToBeChanged(State state) override;

    void signalEmitted(const QString &signal) override;
    void send(QJsonObject v4Payload);

    int selectedFrame() const;
    void selectFrame(int frameNr);

    QV4DebuggerAgent debuggerAgent;

protected:
    void messageReceived(const QByteArray &) override;
    void sendSomethingToSomebody(const char *type, int magicNumber = 1);

private:
    friend class QQmlDebuggerServiceFactory;

    void handleV4Request(const QByteArray &payload);
    static QByteArray packMessage(const QByteArray &command,
                                  const QByteArray &message = QByteArray());
    void processCommand(const QByteArray &command, const QByteArray &data);
    V4CommandHandler *v4CommandHandler(const QString &command) const;

    QStringList breakOnSignals;
    static int sequence;
    int theSelectedFrame;

    void addHandler(V4CommandHandler* handler);
    QHash<QString, V4CommandHandler*> handlers;
    QScopedPointer<UnknownV4CommandHandler> unknownV4CommandHandler;
};

QT_END_NAMESPACE

#endif // QV4DEBUGSERVICE_H
