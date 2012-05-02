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

#ifndef QMLPROFILERDATA_H
#define QMLPROFILERDATA_H

#include <QtQml/private/qqmlprofilerservice_p.h>
#include "qmlprofilereventlocation.h"

#include <QObject>

class QmlProfilerDataPrivate;
class QmlProfilerData : public QObject
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

    static QString getHashStringForQmlEvent(const QmlEventLocation &location, int eventType);
    static QString getHashStringForV8Event(const QString &displayName, const QString &function);
    static QString qmlRangeTypeAsString(QQmlProfilerService::RangeType typeEnum);
    static QString rootEventName();
    static QString rootEventDescription();

    qint64 traceStartTime() const;
    qint64 traceEndTime() const;

    bool isEmpty() const;

signals:
    void error(QString);
    void stateChanged();
    void dataReady();

public slots:
    void clear();
    void setTraceEndTime(qint64 time);
    void setTraceStartTime(qint64 time);
    void addQmlEvent(QQmlProfilerService::RangeType type,
                     QQmlProfilerService::BindingType bindingType,
                     qint64 startTime, qint64 duration, const QStringList &data,
                     const QmlEventLocation &location);
    void addV8Event(int depth, const QString &function, const QString &filename,
                    int lineNumber, double totalTime, double selfTime);
    void addFrameEvent(qint64 time, int framerate, int animationcount);

    void complete();
    bool save(const QString &filename);

private:
    void sortStartTimes();
    int v8EventIndex(const QString &hashStr);
    void computeQmlTime();
    void setState(QmlProfilerData::State state);

private:
    QmlProfilerDataPrivate *d;
};

#endif // QMLPROFILERDATA_H
