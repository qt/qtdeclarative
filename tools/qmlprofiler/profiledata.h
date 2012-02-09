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

#ifndef PROFILEDATA_H
#define PROFILEDATA_H

#include <QtDeclarative/private/qdeclarativeprofilerservice_p.h>

#include <QtCore/QObject>
#include <QtCore/QHash>

struct DeclarativeEvent;
struct V8Event;

typedef QHash<QString, DeclarativeEvent *> DeclarativeEventHash;
typedef QList<DeclarativeEvent *> DeclarativeEvents;
typedef QList<V8Event *> V8Events;

struct EventLocation
{
    EventLocation() : line(-1),column(-1) {}
    EventLocation(const QString &file, int lineNumber, int columnNumber)
        : filename(file), line(lineNumber), column(columnNumber) {}
    QString filename;
    int line;
    int column;
};

struct DeclarativeEventSub {
    DeclarativeEventSub(DeclarativeEvent *from)
        : reference(from), duration(0), calls(0), inLoopPath(false)
    {}
    DeclarativeEventSub(DeclarativeEventSub *from)
        : reference(from->reference), duration(from->duration),
          calls(from->calls), inLoopPath(from->inLoopPath)
    {}
    DeclarativeEvent *reference;
    qint64 duration;
    qint64 calls;
    bool inLoopPath;
};

struct DeclarativeEvent
{
    DeclarativeEvent();
    ~DeclarativeEvent();

    QString displayname;
    QString eventHashStr;
    QString details;
    EventLocation location;
    QDeclarativeProfilerService::RangeType eventType;
    QHash <QString, DeclarativeEventSub *> parentHash;
    QHash <QString, DeclarativeEventSub *> childrenHash;
    qint64 duration;
    qint64 calls;
    qint64 minTime;
    qint64 maxTime;
    double timePerCall;
    double percentOfTime;
    qint64 medianTime;
    int eventId;
    bool isBindingLoop;

    DeclarativeEvent &operator=(const DeclarativeEvent &ref);
};

struct V8EventSub {
    V8EventSub(V8Event *from)
        : reference(from), totalTime(0)
    {}
    V8EventSub(V8EventSub *from)
        : reference(from->reference), totalTime(from->totalTime)
    {}

    V8Event *reference;
    qint64 totalTime;
};

struct V8Event
{
    V8Event();
    ~V8Event();

    QString displayName;
    QString filename;
    QString functionName;
    int line;
    double totalTime; // given in milliseconds
    double totalPercent;
    double selfTime;
    double selfPercent;
    QHash <QString, V8EventSub *> parentHash;
    QHash <QString, V8EventSub *> childrenHash;
    int eventId;

    V8Event &operator=(const V8Event &ref);
};

class ProfileData : public QObject
{
    Q_OBJECT

public:
    explicit ProfileData(QObject *parent = 0);
    ~ProfileData();

    DeclarativeEvents getDeclarativeEvents() const;
    DeclarativeEvent *declarativeEvent(int eventId) const;
    const V8Events& getV8Events() const;
    V8Event *v8Event(int eventId) const;

    int findFirstIndex(qint64 startTime) const;
    int findFirstIndexNoParents(qint64 startTime) const;
    int findLastIndex(qint64 endTime) const;
    Q_INVOKABLE qint64 firstTimeMark() const;
    Q_INVOKABLE qint64 lastTimeMark() const;

    Q_INVOKABLE int count() const;

    // data access
    Q_INVOKABLE qint64 getStartTime(int index) const;
    Q_INVOKABLE qint64 getEndTime(int index) const;
    Q_INVOKABLE qint64 getDuration(int index) const;
    Q_INVOKABLE int getType(int index) const;
    Q_INVOKABLE int getNestingLevel(int index) const;
    Q_INVOKABLE int getNestingDepth(int index) const;
    Q_INVOKABLE QString getFilename(int index) const;
    Q_INVOKABLE int getLine(int index) const;
    Q_INVOKABLE int getColumn(int index) const;
    Q_INVOKABLE QString getDetails(int index) const;
    Q_INVOKABLE int getEventId(int index) const;
    Q_INVOKABLE int getFramerate(int index) const;
    Q_INVOKABLE int getAnimationCount(int index) const;
    Q_INVOKABLE int getMaximumAnimationCount() const;
    Q_INVOKABLE int getMinimumAnimationCount() const;

    // per-type data
    Q_INVOKABLE int uniqueEventsOfType(int type) const;
    Q_INVOKABLE int maxNestingForType(int type) const;
    Q_INVOKABLE QString eventTextForType(int type, int index) const;
    Q_INVOKABLE QString eventDisplayNameForType(int type, int index) const;
    Q_INVOKABLE int eventIdForType(int type, int index) const;
    Q_INVOKABLE int eventPosInType(int index) const;

    Q_INVOKABLE qint64 traceStartTime() const;
    Q_INVOKABLE qint64 traceEndTime() const;
    Q_INVOKABLE qint64 traceDuration() const;
    Q_INVOKABLE qint64 declarativeMeasuredTime() const;
    Q_INVOKABLE qint64 v8MeasuredTime() const;

    void showErrorDialog(const QString &st ) const;
    void compileStatistics(qint64 startTime, qint64 endTime);

signals:
    void dataReady();
    void countChanged();
    void error(const QString &error);
    void dataClear();
    void processingData();
    void postProcessing();

    void requestDetailsForLocation(int eventType, const EventLocation &location);
    void detailsChanged(int eventId, const QString &newString);
    void reloadDetailLabels();
    void reloadDocumentsForDetails();

public slots:
    void clear();
    void addDeclarativeEvent(QDeclarativeProfilerService::RangeType type,
                             qint64 startTime, qint64 length,
                             const QStringList &data,
                             const EventLocation &location);
    void complete();

    void addV8Event(int depth,const QString &function,const QString &filename,
                    int lineNumber, double totalTime, double selfTime);
    void addFrameEvent(qint64 time, int framerate, int animationcount);
    bool save(const QString &filename);
    void load(const QString &filename);
    void setFilename(const QString &filename);
    void load();

    void setTraceEndTime( qint64 time );
    void setTraceStartTime( qint64 time );

    void rewriteDetailsString(QDeclarativeProfilerService::RangeType eventType,
                              const EventLocation &location,
                              const QString &newString);
    void finishedRewritingDetails();

private:
    void postProcess();
    void sortEndTimes();
    void findAnimationLimits();
    void sortStartTimes();
    void computeLevels();
    void computeNestingLevels();
    void computeNestingDepth();
    void prepareForDisplay();
    void linkEndsToStarts();
    void reloadDetails();
    void findBindingLoops(qint64 startTime, qint64 endTime);

private:
    class ProfileDataPrivate *d;
};

#endif // PROFILEDATA_H
