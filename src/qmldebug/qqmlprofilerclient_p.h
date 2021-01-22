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

#ifndef QQMLPROFILERCLIENT_P_H
#define QQMLPROFILERCLIENT_P_H

#include "qqmldebugclient_p.h"
#include "qqmlprofilereventlocation_p.h"
#include "qqmlprofilereventreceiver_p.h"
#include "qqmlprofilerclientdefinitions_p.h"

#include <private/qpacket_p.h>

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

class QQmlProfilerClientPrivate;
class QQmlProfilerClient : public QQmlDebugClient
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlProfilerClient)
    Q_PROPERTY(bool recording READ isRecording WRITE setRecording NOTIFY recordingChanged)

public:
    QQmlProfilerClient(QQmlDebugConnection *connection, QQmlProfilerEventReceiver *eventReceiver,
                       quint64 features = std::numeric_limits<quint64>::max());
    ~QQmlProfilerClient();

    bool isRecording() const;
    void setRecording(bool);
    quint64 recordedFeatures() const;
    virtual void messageReceived(const QByteArray &) override;

    void clearEvents();
    void clearAll();

    void sendRecordingStatus(int engineId = -1);
    void setRequestedFeatures(quint64 features);
    void setFlushInterval(quint32 flushInterval);

protected:
    QQmlProfilerClient(QQmlProfilerClientPrivate &dd);
    void onStateChanged(State status);

signals:
    void complete(qint64 maximumTime);
    void traceFinished(qint64 timestamp, const QList<int> &engineIds);
    void traceStarted(qint64 timestamp, const QList<int> &engineIds);

    void recordingChanged(bool arg);
    void recordedFeaturesChanged(quint64 features);

    void cleared();
};

QT_END_NAMESPACE

#endif // QQMLPROFILERCLIENT_P_H
