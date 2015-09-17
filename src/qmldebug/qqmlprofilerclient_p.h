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

#ifndef QQMLPROFILERCLIENT_P_H
#define QQMLPROFILERCLIENT_P_H

#include "qqmldebugclient_p.h"
#include "qqmleventlocation_p.h"
#include <private/qqmlprofilerdefinitions_p.h>

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

public:
    QQmlProfilerClient(QQmlDebugConnection *connection);
    void setFeatures(quint64 features);

public slots:
    void sendRecordingStatus(bool record, int engineId = -1, quint32 flushInterval = 0);

protected:
    QQmlProfilerClient(QQmlProfilerClientPrivate &dd);

private:
    virtual void messageReceived(const QByteArray &message);

    virtual void traceStarted(qint64 time, int engineId);
    virtual void traceFinished(qint64 time, int engineId);

    virtual void rangeStart(QQmlProfilerDefinitions::RangeType type, qint64 startTime);
    virtual void rangeData(QQmlProfilerDefinitions::RangeType type, qint64 time,
                           const QString &data);
    virtual void rangeLocation(QQmlProfilerDefinitions::RangeType type, qint64 time,
                               const QQmlEventLocation &location);
    virtual void rangeEnd(QQmlProfilerDefinitions::RangeType type, qint64 endTime);

    virtual void animationFrame(qint64 time, int frameRate, int animationCount, int threadId);

    virtual void sceneGraphEvent(QQmlProfilerDefinitions::SceneGraphFrameType type, qint64 time,
                                 qint64 numericData1, qint64 numericData2, qint64 numericData3,
                                 qint64 numericData4, qint64 numericData5);

    virtual void pixmapCacheEvent(QQmlProfilerDefinitions::PixmapEventType type, qint64 time,
                                  const QString &url, int numericData1, int numericData2);

    virtual void memoryAllocation(QQmlProfilerDefinitions::MemoryType type, qint64 time,
                                  qint64 amount);

    virtual void inputEvent(QQmlProfilerDefinitions::InputEventType type, qint64 time, int a,
                            int b);

    virtual void complete();

    virtual void unknownEvent(QQmlProfilerDefinitions::Message messageType, qint64 time,
                              int detailType);
    virtual void unknownData(QDataStream &stream);
};

QT_END_NAMESPACE

#endif // QQMLPROFILERCLIENT_P_H
