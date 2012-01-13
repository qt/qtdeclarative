/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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

#ifndef QDECLARATIVEDEBUGTRACE_P_H
#define QDECLARATIVEDEBUGTRACE_P_H

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

#include <private/qdeclarativedebugservice_p.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qmutex.h>
#include <QtCore/qvector.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

struct Q_AUTOTEST_EXPORT QDeclarativeDebugData
{
    qint64 time;
    int messageType;
    int detailType;

    //###
    QString detailData; //used by RangeData and RangeLocation
    int line;           //used by RangeLocation
    int column;         //used by RangeLocation
    int framerate;      //used by animation events
    int animationcount; //used by animation events

    QByteArray toByteArray() const;
};

Q_DECLARE_TYPEINFO(QDeclarativeDebugData, Q_MOVABLE_TYPE);

class QUrl;
class QDeclarativeEngine;

// RAII
class Q_AUTOTEST_EXPORT QDeclarativeBindingProfiler {
public:
    QDeclarativeBindingProfiler(const QString &url, int line, int column);
    ~QDeclarativeBindingProfiler();
    void addDetail(const QString &details);
};

class Q_DECLARATIVE_EXPORT QDeclarativeDebugTrace : public QDeclarativeDebugService
{
public:
    enum Message {
        Event,
        RangeStart,
        RangeData,
        RangeLocation,
        RangeEnd,
        Complete,

        MaximumMessage
    };

    enum EventType {
        FramePaint,
        Mouse,
        Key,
        AnimationFrame,
        EndTrace,
        StartTrace,

        MaximumEventType
    };

    enum RangeType {
        Painting,
        Compiling,
        Creating,
        Binding,            //running a binding
        HandlingSignal,     //running a signal handler

        MaximumRangeType
    };

    static void initialize();

    static bool startProfiling();
    static bool stopProfiling();
    static void addEvent(EventType);
    static void startRange(RangeType);
    static void rangeData(RangeType, const QString &);
    static void rangeData(RangeType, const QUrl &);
    static void rangeLocation(RangeType, const QString &, int, int);
    static void rangeLocation(RangeType, const QUrl &, int, int);
    static void endRange(RangeType);
    static void animationFrame(qint64);

    static void sendProfilingData();

    QDeclarativeDebugTrace();
    ~QDeclarativeDebugTrace();

protected:
    virtual void messageReceived(const QByteArray &);

private:
    bool startProfilingImpl();
    bool stopProfilingImpl();
    void addEventImpl(EventType);
    void startRangeImpl(RangeType);
    void rangeDataImpl(RangeType, const QString &);
    void rangeDataImpl(RangeType, const QUrl &);
    void rangeLocationImpl(RangeType, const QString &, int, int);
    void rangeLocationImpl(RangeType, const QUrl &, int, int);
    void endRangeImpl(RangeType);
    void animationFrameImpl(qint64);

    bool profilingEnabled();
    void setProfilingEnabled(bool enable);
    void sendMessages();
    void processMessage(const QDeclarativeDebugData &);

private:
    QElapsedTimer m_timer;
    bool m_enabled;
    bool m_messageReceived;
    QVector<QDeclarativeDebugData> m_data;
    QMutex m_mutex;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDECLARATIVEDEBUGTRACE_P_H

