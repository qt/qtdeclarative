// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLPROFILERDATA_H
#define QMLPROFILERDATA_H

#include <private/qqmlprofilerclientdefinitions_p.h>
#include <private/qqmlprofilereventlocation_p.h>
#include <private/qqmlprofilereventreceiver_p.h>

#include <QObject>

class QmlProfilerDataPrivate;
class QmlProfilerData : public QQmlProfilerEventReceiver
{
    Q_OBJECT
public:
    enum State {
        Empty,
        AcquiringData,
        ProcessingData,
        Done
    };

    explicit QmlProfilerData(QObject *parent = nullptr);
    ~QmlProfilerData();

    int numLoadedEventTypes() const override;
    void addEventType(const QQmlProfilerEventType &type) override;
    void addEvent(const QQmlProfilerEvent &event) override;

    static QString getHashStringForQmlEvent(const QQmlProfilerEventLocation &location, int eventType);
    static QString qmlRangeTypeAsString(RangeType type);
    static QString qmlMessageAsString(Message type);

    qint64 traceStartTime() const;
    qint64 traceEndTime() const;

    bool isEmpty() const;

    void clear();
    void setTraceEndTime(qint64 time);
    void setTraceStartTime(qint64 time);

    void complete();
    bool save(const QString &filename);

Q_SIGNALS:
    void error(QString);
    void stateChanged();
    void dataReady();

private:
    void sortStartTimes();
    void computeQmlTime();
    void setState(QmlProfilerData::State state);

private:
    QmlProfilerDataPrivate *d;
};

#endif // QMLPROFILERDATA_H
