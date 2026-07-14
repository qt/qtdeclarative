// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQUICKEVENTREPLAYCLIENT_P_H
#define QQUICKEVENTREPLAYCLIENT_P_H

#include "qqmldebugclient_p.h"

#include <private/qqmlprofilerevent_p.h>
#include <private/qqmlprofilereventtype_p.h>

#include <QtCore/qxpfunctional.h>

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

class QQuickEventReplayClientPrivate;
class QQuickEventReplayClient : public QQmlDebugClient
{
    Q_OBJECT
public:
    QQuickEventReplayClient(QQmlDebugConnection *connection);
    bool loadEvents(const QString &fileName,
                    qxp::function_ref<void(const QQmlProfilerEventType &,
                                           QQmlProfilerEvent &&)> &&handler);

    void sendEvent(const QQmlProfilerEventType &type, QQmlProfilerEvent &&event);
};

QT_END_NAMESPACE

#endif // QQUICKEVENTREPLAYCLIENT_P_H
