/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQMLENGINECONTROLCLIENT_P_H
#define QQMLENGINECONTROLCLIENT_P_H

#include "qqmldebugclient_p.h"

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

QT_BEGIN_NAMESPACE

class QQmlEngineControlClientPrivate;
class QQmlEngineControlClient : public QQmlDebugClient
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlEngineControlClient)
public:
    QQmlEngineControlClient(QQmlDebugConnection *connection);

    void blockEngine(int engineId);
    void releaseEngine(int engineId);

    QList<int> blockedEngines() const;

signals:
    void engineAboutToBeAdded(int engineId, const QString &name);
    void engineAdded(int engineId, const QString &name);
    void engineAboutToBeRemoved(int engineId, const QString &name);
    void engineRemoved(int engineId, const QString &name);

protected:
    QQmlEngineControlClient(QQmlEngineControlClientPrivate &dd);

private:
    void messageReceived(const QByteArray &) override;
};

QT_END_NAMESPACE

#endif // QQMLENGINECONTROLCLIENT_P_H
