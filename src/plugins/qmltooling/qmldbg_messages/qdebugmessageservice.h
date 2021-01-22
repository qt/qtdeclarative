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

#ifndef QDEBUGMESSAGESERVICE_H
#define QDEBUGMESSAGESERVICE_H

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

#include <private/qqmldebugserviceinterfaces_p.h>

#include <QtCore/qlogging.h>
#include <QtCore/qmutex.h>
#include <QtCore/qelapsedtimer.h>

QT_BEGIN_NAMESPACE

class QDebugMessageServicePrivate;

class QDebugMessageServiceImpl : public QDebugMessageService
{
    Q_OBJECT
public:
    QDebugMessageServiceImpl(QObject *parent = 0);

    void sendDebugMessage(QtMsgType type, const QMessageLogContext &ctxt, const QString &buf);
    void synchronizeTime(const QElapsedTimer &otherTimer) override;

protected:
    void stateChanged(State) override;

private:
    friend class QQmlDebuggerServiceFactory;

    QtMessageHandler oldMsgHandler;
    QQmlDebugService::State prevState;
    QMutex initMutex;
    QElapsedTimer timer;
};

QT_END_NAMESPACE

#endif // QDEBUGMESSAGESERVICE_H
