/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
// this contains QUnifiedTimer
#include <private/qabstractanimation_p.h>

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qmutex.h>
#include <QtCore/qvector.h>
#include <QtCore/qstringbuilder.h>
#include <QtCore/qwaitcondition.h>

#define Q_QML_PROFILE(Method)\
    if (QQmlProfilerService::enabled) {\
        QQmlProfilerService::Method;\
    } else\
        (void)0

QT_BEGIN_NAMESPACE

struct Q_AUTOTEST_EXPORT QQmlProfilerData
{
    qint64 time;
    int messageType;
    int detailType;

    //###
    QString detailData; //used by RangeData and RangeLocation
    int line;           //used by RangeLocation, also as "width" for pixmaps
    int column;         //used by RangeLocation, also as "height" for pixmaps
    int framerate;      //used by animation events
    int animationcount; //used by animation events, also as "cache/reference count" for pixmaps
    int bindingType;

    qint64 subtime_1;
    qint64 subtime_2;
    qint64 subtime_3;
    qint64 subtime_4;
    qint64 subtime_5;

    QByteArray toByteArray() const;
};

Q_DECLARE_TYPEINFO(QQmlProfilerData, Q_MOVABLE_TYPE);

class QUrl;
class QQmlEngine;


class Q_QML_PRIVATE_EXPORT QQmlProfilerService : public QQmlDebugService
{
public:
    enum Message {
        Event,
        RangeStart,
        RangeData,
        RangeLocation,
        RangeEnd,
        Complete, // end of transmission
        PixmapCacheEvent,
        SceneGraphFrame,

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

    enum BindingType {
        QmlBinding,
        V8Binding,
        V4Binding,

        MaximumBindingType
    };

    enum PixmapEventType {
        PixmapSizeKnown,
        PixmapReferenceCountChanged,
        PixmapCacheCountChanged,
        PixmapLoadingStarted,
        PixmapLoadingFinished,
        PixmapLoadingError,

        MaximumPixmapEventType
    };

    enum SceneGraphFrameType {
        SceneGraphRendererFrame,
        SceneGraphAdaptationLayerFrame,
        SceneGraphContextFrame,
        SceneGraphRenderLoopFrame,
        SceneGraphTexturePrepare,
        SceneGraphTextureDeletion,
        SceneGraphPolishAndSync,
        SceneGraphWindowsRenderShow,
        SceneGraphWindowsAnimations,
        SceneGraphWindowsPolishFrame,

        MaximumSceneGraphFrameType
    };

    static void initialize();

    static bool startProfiling();
    static bool stopProfiling();

    static void addEvent(EventType event)
    {
        QQmlProfilerData ed = {instance->m_timer.nsecsElapsed(), (int)Event, (int)event,
                               QString(), -1, -1, 0, 0, 0,
                               0, 0, 0, 0, 0};
        instance->processMessage(ed);
    }

    static void animationFrame(qint64 delta)
    {
        int animCount = QUnifiedTimer::instance()->runningAnimationCount();

        if (animCount > 0 && delta > 0) {
            QQmlProfilerData ed = {instance->m_timer.nsecsElapsed(), (int)Event,
                                   (int)AnimationFrame, QString(), -1, -1,
                                   1000 / (int)delta /* trim fps to integer */, animCount, 0, 0, 0,
                                   0, 0, 0};
            instance->processMessage(ed);
        }
    }

    static void sceneGraphFrame(SceneGraphFrameType frameType, qint64 value1, qint64 value2 = -1,
                                qint64 value3 = -1, qint64 value4 = -1, qint64 value5 = -1)
    {
        // because I already have some space to store ints in the struct, I'll use it to store the
        // frame data even though the field names do not match
        QQmlProfilerData ed = {instance->m_timer.nsecsElapsed(), (int)SceneGraphFrame,
                               (int)frameType, QString(), -1, -1, -1, -1, -1,
                               value1, value2, value3, value4, value5};
        instance->processMessage(ed);
    }

    static void pixmapEvent(PixmapEventType eventType, const QUrl &url)
    {
        QQmlProfilerData ed = {instance->m_timer.nsecsElapsed(), (int)PixmapCacheEvent,
                               (int)eventType, url.toString(), -1, -1, -1, -1, -1,
                               0, 0, 0, 0, 0};
        instance->processMessage(ed);
    }

    static void pixmapEvent(PixmapEventType eventType, const QUrl &url, int count)
    {
        QQmlProfilerData ed = {instance->m_timer.nsecsElapsed(), (int)PixmapCacheEvent,
                               (int)eventType, url.toString(), -1, -1, -1, count, -1,
                               0, 0, 0, 0, 0};
        instance->processMessage(ed);
    }

    static void pixmapEvent(PixmapEventType eventType, const QUrl &url, const QSize &size)
    {
        if (size.width() > 0 && size.height() > 0) {
            QQmlProfilerData ed = {instance->m_timer.nsecsElapsed(), (int)PixmapCacheEvent,
                                   (int)eventType, url.toString(),
                                   size.width(), size.height(), -1, -1, -1,
                                   0, 0, 0, 0, 0};
            instance->processMessage(ed);
        }
    }

    static void sendProfilingData();

    QQmlProfilerService();
    ~QQmlProfilerService();

protected:
    virtual void stateAboutToBeChanged(State state);
    virtual void messageReceived(const QByteArray &);

private:
    bool startProfilingImpl();
    bool stopProfilingImpl();

    static void startRange(RangeType range, BindingType bindingType = QmlBinding)
    {
        QQmlProfilerData rd = {instance->m_timer.nsecsElapsed(), (int)RangeStart, (int)range,
                               QString(), -1, -1, 0, 0, (int)bindingType,
                               0, 0, 0, 0, 0};
        instance->processMessage(rd);
    }

    static void rangeData(RangeType range, const QString &rData)
    {
        QQmlProfilerData rd = {instance->m_timer.nsecsElapsed(), (int)RangeData, (int)range,
                               rData, -1, -1, 0, 0, 0,
                               0, 0, 0, 0, 0};
        instance->processMessage(rd);
    }

    static void rangeData(RangeType range, const QUrl &rData)
    {
        QQmlProfilerData rd = {instance->m_timer.nsecsElapsed(), (int)RangeData, (int)range,
                               rData.toString(), -1, -1, 0, 0, 0,
                               0, 0, 0, 0, 0};
        instance->processMessage(rd);
    }

    static void rangeLocation(RangeType range, const QString &fileName, int line, int column)
    {
        QQmlProfilerData rd = {instance->m_timer.nsecsElapsed(), (int)RangeLocation, (int)range,
                               fileName, line, column, 0, 0, 0,
                               0, 0, 0, 0, 0};
        instance->processMessage(rd);
    }

    static void rangeLocation(RangeType range, const QUrl &fileName, int line, int column)
    {
        QQmlProfilerData rd = {instance->m_timer.nsecsElapsed(), (int)RangeLocation, (int)range,
                               fileName.toString(), line, column, 0, 0, 0,
                               0, 0, 0, 0, 0};
        instance->processMessage(rd);
    }

    static void endRange(RangeType range)
    {
        QQmlProfilerData rd = {instance->m_timer.nsecsElapsed(), (int)RangeEnd, (int)range,
                               QString(), -1, -1, 0, 0, 0,
                               0, 0, 0, 0, 0};
        instance->processMessage(rd);
    }

    void sendMessages();

    void processMessage(const QQmlProfilerData &message)
    {
        QMutexLocker locker(&m_dataMutex);
        m_data.append(message);
    }

    static void animationTimerCallback(qint64 delta);

public:
    static bool enabled;
private:
    QElapsedTimer m_timer;
    QVector<QQmlProfilerData> m_data;
    QMutex m_dataMutex;
    QMutex m_initializeMutex;
    QWaitCondition m_initializeCondition;

    static QQmlProfilerService *instance;

    friend struct QQmlBindingProfiler;
    friend struct QQmlHandlingSignalProfiler;
    friend struct QQmlVmeProfiler;
    friend struct QQmlCompilingProfiler;
};

//
// RAII helper structs
//

struct QQmlBindingProfiler {
    QQmlBindingProfiler(const QString &url, int line, int column, QQmlProfilerService::BindingType bindingType)
    {
        Q_QML_PROFILE(startRange(QQmlProfilerService::Binding, bindingType));
        Q_QML_PROFILE(rangeLocation(QQmlProfilerService::Binding, url, line, column));
    }

    ~QQmlBindingProfiler()
    {
        Q_QML_PROFILE(endRange(QQmlProfilerService::Binding));
    }
};

struct QQmlHandlingSignalProfiler {
    QQmlHandlingSignalProfiler(QQmlBoundSignalExpression *expression)
    {
        Q_QML_PROFILE(startRange(QQmlProfilerService::HandlingSignal));
        Q_QML_PROFILE(rangeLocation(QQmlProfilerService::HandlingSignal,
                expression->sourceFile(), expression->lineNumber(),
                expression->columnNumber()));
    }

    ~QQmlHandlingSignalProfiler()
    {
        Q_QML_PROFILE(endRange(QQmlProfilerService::HandlingSignal));
    }
};

struct QQmlCompilingProfiler {
    QQmlCompilingProfiler(const QString &name)
    {
        Q_QML_PROFILE(startRange(QQmlProfilerService::Compiling));
        Q_QML_PROFILE(rangeLocation(QQmlProfilerService::Compiling, name, 1, 1));
        Q_QML_PROFILE(rangeData(QQmlProfilerService::Compiling, name));
    }

    ~QQmlCompilingProfiler()
    {
        Q_QML_PROFILE(endRange(QQmlProfilerService::Compiling));
    }
};

struct QQmlVmeProfiler {
public:

    struct Data {
        Data() : line(0), column(0) {}
        QUrl url;
        int line;
        int column;
        QString typeName;
        void clear()
        {
            url.clear();
            line = 0;
            column = 0;
            typeName.clear();
        }
    };

    QQmlVmeProfiler() :
        running(false)
    {}

    ~QQmlVmeProfiler()
    {
        if (QQmlProfilerService::enabled)
            clear();
    }

    void clear()
    {
        stop();
        ranges.clear();
        if (QQmlProfilerService::enabled) {
            for (int i = 0; i < backgroundRanges.count(); ++i) {
                QQmlProfilerService::instance->endRange(QQmlProfilerService::Creating);
            }
        }
        backgroundRanges.clear();
        running = false;
    }

    bool start()
    {
        if (QQmlProfilerService::enabled) {
            currentRange.clear();
            if (running)
                QQmlProfilerService::instance->endRange(QQmlProfilerService::Creating);
            else
                running = true;
            QQmlProfilerService::instance->startRange(QQmlProfilerService::Creating);
            return true;
        }
        return false;
    }

    void stop()
    {
        if (QQmlProfilerService::enabled && running) {
            QQmlProfilerService::instance->endRange(QQmlProfilerService::Creating);
            currentRange.clear();
            running = false;
        }
    }

    void updateLocation(const QUrl &url, int line, int column)
    {
        if (QQmlProfilerService::enabled && running) {
            currentRange.url = url;
            currentRange.line = line;
            currentRange.column = column;
            QQmlProfilerService::instance->rangeLocation(
                    QQmlProfilerService::Creating, url, line, column);
        }
    }

    void updateTypeName(const QString &typeName)
    {
        if (QQmlProfilerService::enabled && running) {
            currentRange.typeName = typeName;
            QQmlProfilerService::instance->rangeData(QQmlProfilerService::Creating, typeName);
        }
    }

    bool pop()
    {
        if (QQmlProfilerService::enabled && ranges.count() > 0) {
            start();
            currentRange = ranges.pop();
            QQmlProfilerService::instance->rangeLocation(
                    QQmlProfilerService::Creating, currentRange.url, currentRange.line, currentRange.column);
            QQmlProfilerService::instance->rangeData(QQmlProfilerService::Creating, currentRange.typeName);
            return true;
        }
        return false;
    }

    void push()
    {
        if (QQmlProfilerService::enabled && running)
            ranges.push(currentRange);
    }

    void background()
    {
        if (QQmlProfilerService::enabled && running) {
            backgroundRanges.push(currentRange);
            running = false;
        }
    }

    bool foreground()
    {
        if (QQmlProfilerService::enabled && backgroundRanges.count() > 0) {
            stop();
            currentRange = backgroundRanges.pop();
            running = true;
            return true;
        }
        return false;
    }

private:

    Data currentRange;
    QStack<Data> ranges;
    QStack<Data> backgroundRanges;
    bool running;
};

QT_END_NAMESPACE

#endif // QQMLPROFILERSERVICE_P_H

