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
****************************************************************************/

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

    explicit QmlProfilerData(QObject *parent = 0);
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

signals:
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
