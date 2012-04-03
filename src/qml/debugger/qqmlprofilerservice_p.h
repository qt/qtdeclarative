/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQMLPROFILERSERVICE_P_H
#define QQMLPROFILERSERVICE_P_H

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

#include <private/qqmldebugservice_p.h>
#include <private/qqmlboundsignal_p.h>

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qmutex.h>
#include <QtCore/qvector.h>
#include <QtCore/qstringbuilder.h>


QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

struct Q_AUTOTEST_EXPORT QQmlProfilerData
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

Q_DECLARE_TYPEINFO(QQmlProfilerData, Q_MOVABLE_TYPE);

class QUrl;
class QQmlEngine;


class Q_QML_EXPORT QQmlProfilerService : public QQmlDebugService
{
public:
    enum Message {
        Event,
        RangeStart,
        RangeData,
        RangeLocation,
        RangeEnd,
        Complete, // end of transmission

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
    static void sendStartedProfilingMessage();
    static void addEvent(EventType);
    static void animationFrame(qint64);

    static void sendProfilingData();

    QQmlProfilerService();
    ~QQmlProfilerService();

protected:
    virtual void stateAboutToBeChanged(State state);
    virtual void messageReceived(const QByteArray &);

private:
    bool startProfilingImpl();
    bool stopProfilingImpl();
    void sendStartedProfilingMessageImpl();
    void addEventImpl(EventType);
    void animationFrameImpl(qint64);

    void startRange(RangeType);
    void rangeData(RangeType, const QString &);
    void rangeData(RangeType, const QUrl &);
    void rangeLocation(RangeType, const QString &, int, int);
    void rangeLocation(RangeType, const QUrl &, int, int);
    void endRange(RangeType);


    bool profilingEnabled();
    void setProfilingEnabled(bool enable);
    void sendMessages();
    void processMessage(const QQmlProfilerData &);

private:
    QElapsedTimer m_timer;
    bool m_enabled;
    bool m_messageReceived;
    QVector<QQmlProfilerData> m_data;
    QMutex m_mutex;

    static QQmlProfilerService *instance;

    friend struct QQmlBindingProfiler;
    friend struct QQmlHandlingSignalProfiler;
    friend struct QQmlObjectCreatingProfiler;
    friend struct QQmlCompilingProfiler;
};

//
// RAII helper structs
//

struct QQmlBindingProfiler {
    QQmlBindingProfiler(const QString &url, int line, int column)
    {
        QQmlProfilerService *instance = QQmlProfilerService::instance;
        enabled = instance ? instance->profilingEnabled() : false;
        if (enabled) {
            instance->startRange(QQmlProfilerService::Binding);
            instance->rangeLocation(QQmlProfilerService::Binding, url, line, column);
        }
    }

    ~QQmlBindingProfiler()
    {
        if (enabled)
            QQmlProfilerService::instance->endRange(QQmlProfilerService::Binding);
    }

    bool enabled;
};

struct QQmlHandlingSignalProfiler {
    QQmlHandlingSignalProfiler(const QMetaMethod &signal, QQmlBoundSignalExpression *expression)
    {
        enabled = QQmlProfilerService::instance
                ? QQmlProfilerService::instance->profilingEnabled() : false;
        if (enabled) {
            QQmlProfilerService *service = QQmlProfilerService::instance;
            service->startRange(QQmlProfilerService::HandlingSignal);
            service->rangeData(QQmlProfilerService::HandlingSignal,
                               QLatin1String(signal.signature()) + QLatin1String(": ")
                               + expression->expression());
            service->rangeLocation(QQmlProfilerService::HandlingSignal,
                                   expression->sourceFile(), expression->lineNumber(),
                                   expression->columnNumber());
        }
    }

    ~QQmlHandlingSignalProfiler()
    {
        if (enabled)
            QQmlProfilerService::instance->endRange(QQmlProfilerService::HandlingSignal);
    }

    bool enabled;
};

struct QQmlObjectCreatingProfiler {
    QQmlObjectCreatingProfiler()
    {
        enabled = QQmlProfilerService::instance
                ? QQmlProfilerService::instance->profilingEnabled() : false;
        if (enabled) {
            QQmlProfilerService *service = QQmlProfilerService::instance;
            service->startRange(QQmlProfilerService::Creating);
        }
    }

    void setTypeName(const QString &typeName)
    {
        Q_ASSERT_X(enabled, Q_FUNC_INFO, "method called although profiler is not enabled.");
        QQmlProfilerService::instance->rangeData(QQmlProfilerService::Creating, typeName);
    }

    void setLocation(const QUrl &url, int line, int column)
    {
        Q_ASSERT_X(enabled, Q_FUNC_INFO, "method called although profiler is not enabled.");
        if (enabled)
            QQmlProfilerService::instance->rangeLocation(
                        QQmlProfilerService::Creating, url, line, column);
    }

    ~QQmlObjectCreatingProfiler()
    {
        if (enabled)
            QQmlProfilerService::instance->endRange(QQmlProfilerService::Creating);
    }

    bool enabled;
};

struct QQmlCompilingProfiler {
    QQmlCompilingProfiler(const QString &name)
    {
        QQmlProfilerService *instance = QQmlProfilerService::instance;
        enabled = instance ?
                    instance->profilingEnabled() : false;
        if (enabled) {
            instance->startRange(QQmlProfilerService::Compiling);
            instance->rangeLocation(QQmlProfilerService::Compiling, name, 1, 1);
            instance->rangeData(QQmlProfilerService::Compiling, name);
        }
    }

    ~QQmlCompilingProfiler()
    {
        if (enabled)
            QQmlProfilerService::instance->endRange(QQmlProfilerService::Compiling);
    }

    bool enabled;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QQMLPROFILERSERVICE_P_H

