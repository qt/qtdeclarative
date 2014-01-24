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

#include "qqmlconfigurabledebugservice_p.h"
#include "qqmlprofilerdefinitions_p.h"
#include "qqmlabstractprofileradapter_p.h"

#include <private/qqmlboundsignal_p.h>
#include <private/qv4function_p.h>

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qmutex.h>
#include <QtCore/qvector.h>
#include <QtCore/qstringbuilder.h>
#include <QtCore/qwaitcondition.h>

#define Q_QML_PROFILE_IF_ENABLED(Code)\
    if (QQmlProfilerService::enabled) {\
        Code;\
    } else\
        (void)0

#define Q_QML_PROFILE(Method)\
    Q_QML_PROFILE_IF_ENABLED(QQmlProfilerService::Method)

QT_BEGIN_NAMESPACE

// This struct is somewhat dangerous to use:
// You can save values either with 32 or 64 bit precision. toByteArrays will
// guess the precision from messageType. If you state the wrong messageType
// you will get undefined results.
// The messageType is itself a bit field. You can pack multiple messages into
// one object, e.g. RangeStart and RangeLocation. Each one will be read
// independently by toByteArrays. Thus you can only pack messages if their data
// doesn't overlap. Again, it's up to you to figure that out.
struct Q_AUTOTEST_EXPORT QQmlProfilerData
{
    QQmlProfilerData() {}

    QQmlProfilerData(qint64 time, int messageType, int detailType, const QUrl &url,
                     int x = 0, int y = 0, int framerate = 0, int count = 0, int bindingType = 0) :
        time(time), messageType(messageType), detailType(detailType), detailUrl(url),
        x(x), y(y), framerate(framerate), count(count), bindingType(bindingType) {}

    QQmlProfilerData(qint64 time, int messageType, int detailType, const QString &str,
                     int x = 0, int y = 0, int framerate = 0, int count = 0, int bindingType = 0) :
        time(time), messageType(messageType), detailType(detailType),detailString(str),
        x(x), y(y), framerate(framerate), count(count), bindingType(bindingType) {}

    QQmlProfilerData(qint64 time, int messageType, int detailType, const QString &str,
                     const QUrl &url, int x = 0, int y = 0, int framerate = 0, int count = 0,
                     int bindingType = 0) :
        time(time), messageType(messageType), detailType(detailType), detailString(str),
        detailUrl(url), x(x), y(y), framerate(framerate), count(count), bindingType(bindingType) {}


    QQmlProfilerData(qint64 time, int messageType, int detailType) :
        time(time), messageType(messageType), detailType(detailType) {}

    // Special ctor for scenegraph frames. Note that it's missing the QString/QUrl params.
    // This is slightly ugly, but makes it easier to disambiguate between int and qint64 params.
    QQmlProfilerData(qint64 time, int messageType, int detailType,
                     qint64 d1, qint64 d2, qint64 d3, qint64 d4, qint64 d5) :
        time(time), messageType(messageType), detailType(detailType),
        subtime_1(d1), subtime_2(d2), subtime_3(d3), subtime_4(d4), subtime_5(d5) {}


    qint64 time;
    int messageType;        //bit field of QQmlProfilerService::Message
    int detailType;

    QString detailString;   //used by RangeData and possibly by RangeLocation
    QUrl detailUrl;         //used by RangeLocation, overrides detailString

    union {
        qint64 subtime_1;
        int x;              //used by RangeLocation and for pixmaps
    };

    union {
        qint64 subtime_2;
        int y;              //used by RangeLocation and for pixmaps
    };

    union {
        qint64 subtime_3;
        int framerate;      //used by animation events
    };

    union {
        qint64 subtime_4;
        int count;          //used by animation events and for pixmaps
    };

    union {
        qint64 subtime_5;
        int bindingType;
    };

    void toByteArrays(QList<QByteArray> &messages) const;
};

Q_DECLARE_TYPEINFO(QQmlProfilerData, Q_MOVABLE_TYPE);

class QUrl;
class QQmlEngine;


class Q_QML_PRIVATE_EXPORT QQmlProfilerService : public QQmlConfigurableDebugService, public QQmlProfilerDefinitions
{
    Q_OBJECT
public:

    static QQmlProfilerService *instance();
    void engineAboutToBeAdded(QQmlEngine *engine);
    void engineAboutToBeRemoved(QQmlEngine *engine);
    void engineAdded(QQmlEngine *engine);
    void engineRemoved(QQmlEngine *engine);

    void addGlobalProfiler(QQmlAbstractProfilerAdapter *profiler);
    void removeGlobalProfiler(QQmlAbstractProfilerAdapter *profiler);

    void startProfiling(QQmlEngine *engine);
    void stopProfiling(QQmlEngine *engine);

    qint64 timestamp() {return m_timer.nsecsElapsed();}

    QQmlProfilerService();
    ~QQmlProfilerService();

    void dataReady(QQmlAbstractProfilerAdapter *profiler);

protected:
    virtual void stateAboutToBeChanged(State state);
    virtual void messageReceived(const QByteArray &);

private:

    static void startBinding(const QString &fileName, int line, int column, BindingType bindingType)
    {
        m_instance->processMessage(QQmlProfilerData(m_instance->timestamp(),
                                                  (1 << RangeStart | 1 << RangeLocation),
                                                  1 << Binding, fileName, line, column, 0, 0,
                                                  bindingType));
    }

    // Have toByteArrays() construct another RangeData event from the same QString later.
    // This is somewhat pointless but important for backwards compatibility.
    static void startCompiling(const QString &name)
    {
        m_instance->processMessage(QQmlProfilerData(m_instance->timestamp(),
                       (1 << RangeStart | 1 << RangeLocation | 1 << RangeData), 1 << Compiling,
                       name, 1, 1, 0, 0, QmlBinding));
    }

    static void startHandlingSignal(const QString &fileName, int line, int column)
    {
        m_instance->processMessage(QQmlProfilerData(m_instance->timestamp(),
                                                  (1 << RangeStart | 1 << RangeLocation),
                                                  1 << HandlingSignal, fileName, line, column, 0, 0,
                                                  QmlBinding));
    }

    static void startCreating(const QString &typeName, const QUrl &fileName, int line, int column)
    {
        m_instance->processMessage(QQmlProfilerData(m_instance->timestamp(),
                                                  (1 << RangeStart | 1 << RangeLocation | 1 << RangeData),
                                                  1 << Creating, typeName, fileName, line, column,
                                                  0, 0, QmlBinding));
    }

    static void startCreating(const QString &typeName)
    {
        m_instance->processMessage(QQmlProfilerData(m_instance->timestamp(),
                                                  (1 << RangeStart | 1 << RangeData), 1 << Creating,
                                                  typeName, 0, 0, 0, 0, QmlBinding));
    }

    static void creatingLocation(const QUrl &fileName, int line, int column)
    {
        m_instance->processMessage(QQmlProfilerData(m_instance->timestamp(), 1 << RangeLocation,
                                                  1 << Creating, fileName, line, column));
    }

    template<RangeType Range>
    static void endRange()
    {
        m_instance->processMessage(QQmlProfilerData(m_instance->timestamp(), 1 << RangeEnd,
                                                  1 << Range));
    }

    void sendMessages();
    void addEngineProfiler(QQmlAbstractProfilerAdapter *profiler, QQmlEngine *engine);

    void processMessage(const QQmlProfilerData &message)
    {
        QMutexLocker locker(&m_dataMutex);
        m_data.append(message);
    }

public:
    static bool enabled;
private:
    QElapsedTimer m_timer;
    QVector<QQmlProfilerData> m_data;
    QMutex m_dataMutex;

    QList<QQmlAbstractProfilerAdapter *> m_globalProfilers;
    QMultiHash<QQmlEngine *, QQmlAbstractProfilerAdapter *> m_engineProfilers;
    QList<QQmlEngine *> m_stoppingEngines;
    QMultiMap<qint64, QQmlAbstractProfilerAdapter *> m_startTimes;

    static QQmlProfilerService *m_instance;

    friend struct QQmlBindingProfiler;
    friend struct QQmlHandlingSignalProfiler;
    friend struct QQmlVmeProfiler;
    friend struct QQmlCompilingProfiler;
    friend class QQmlProfiler;
};

// Temporary shim around QQmlProfilerService to make it look like a QQmlAbstractProfilerAdapter.
class QQmlProfiler : public QQmlAbstractProfilerAdapter {
    Q_OBJECT
public:
    QQmlProfiler(QQmlProfilerService *service);
    qint64 sendMessages(qint64 until, QList<QByteArray> &messages);

public slots:
    void startProfiling();
    void stopProfiling();
private:
    int next;
};

//
// RAII helper structs
//

struct QQmlBindingProfiler {
    QQmlBindingProfiler(const QString &url, int line, int column, QQmlProfilerService::BindingType bindingType)
    {
        Q_QML_PROFILE(startBinding(url, line, column, bindingType));
    }

    ~QQmlBindingProfiler()
    {
        Q_QML_PROFILE(endRange<QQmlProfilerService::Binding>());
    }
};

struct QQmlHandlingSignalProfiler {
    QQmlHandlingSignalProfiler(QQmlBoundSignalExpression *expression)
    {
        Q_QML_PROFILE_IF_ENABLED({
            QV4::Function *function;
            if (expression->sourceFile().isEmpty() && (function = expression->function())) {
                QQmlProfilerService::startHandlingSignal(
                        function->sourceFile(), function->compiledFunction->location.line,
                        function->compiledFunction->location.column);

            } else {
                QQmlProfilerService::startHandlingSignal(
                        expression->sourceFile(), expression->lineNumber(),
                        expression->columnNumber());
            }
        });
    }

    ~QQmlHandlingSignalProfiler()
    {
        Q_QML_PROFILE(endRange<QQmlProfilerService::HandlingSignal>());
    }
};

struct QQmlCompilingProfiler {
    QQmlCompilingProfiler(const QString &name)
    {
        Q_QML_PROFILE(startCompiling(name));
    }

    ~QQmlCompilingProfiler()
    {
        Q_QML_PROFILE(endRange<QQmlProfilerService::Compiling>());
    }
};

#define Q_QML_VME_PROFILE(Method) Q_QML_PROFILE_IF_ENABLED(Method)

struct QQmlVmeProfiler {
public:

    struct Data {
        Data() : line(0), column(0) {}
        QUrl url;
        int line;
        int column;
        QString typeName;
    };

    QQmlVmeProfiler() : running(false) {}

    void clear()
    {
        ranges.clear();
        if (running)
            QQmlProfilerService::m_instance->endRange<QQmlProfilerService::Creating>();
        for (int i = 0; i < backgroundRanges.count(); ++i) {
            QQmlProfilerService::m_instance->endRange<QQmlProfilerService::Creating>();
        }
        backgroundRanges.clear();
        running = false;
    }

    void startBackground(const QString &typeName)
    {
        if (running) {
            QQmlProfilerService::m_instance->endRange<QQmlProfilerService::Creating>();
            running = false;
        }
        QQmlProfilerService::m_instance->startCreating(typeName);
        backgroundRanges.push(typeName);
    }

    void start(const QString &typeName, const QUrl &url, int line, int column)
    {
        switchRange();
        setCurrentRange(typeName, url, line, column);
        QQmlProfilerService::m_instance->startCreating(typeName, url, line, column);
    }

    void stop()
    {
        if (running) {
            QQmlProfilerService::m_instance->endRange<QQmlProfilerService::Creating>();
            running = false;
        }
    }

    void pop()
    {
        if (ranges.count() > 0) {
            switchRange();
            currentRange = ranges.pop();
            QQmlProfilerService::m_instance->startCreating(currentRange.typeName, currentRange.url,
                                                         currentRange.line, currentRange.column);
        }
    }

    void push()
    {
        if (running)
            ranges.push(currentRange);
    }

    void foreground(const QUrl &url, int line, int column)
    {
        if (backgroundRanges.count() > 0) {
            switchRange();
            setCurrentRange(backgroundRanges.pop(), url, line, column);
            QQmlProfilerService::m_instance->creatingLocation(url, line, column);
        }
    }

private:

    void switchRange()
    {
        if (running)
            QQmlProfilerService::m_instance->endRange<QQmlProfilerService::Creating>();
        else
            running = true;
    }

    void setCurrentRange(const QString &typeName, const QUrl &url, int line, int column)
    {
        currentRange.typeName = typeName;
        currentRange.url = url;
        currentRange.line = line;
        currentRange.column = column;
    }

    Data currentRange;
    QStack<Data> ranges;
    QStack<QString> backgroundRanges;
    bool running;
};

QT_END_NAMESPACE

#endif // QQMLPROFILERSERVICE_P_H

