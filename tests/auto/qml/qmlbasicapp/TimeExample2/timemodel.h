// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TIMEMODEL_H
#define TIMEMODEL_H

#include <QtQml/qqml.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qbasictimer.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qproperty.h>

class TimeModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int hour READ hour BINDABLE hourBindable FINAL)
    Q_PROPERTY(int minute READ minute BINDABLE minuteBindable FINAL)
    QML_NAMED_ELEMENT(Time)

public:
    TimeModel(QObject *parent=nullptr);

    int minute() const { return m_minute.value(); }
    int hour() const { return m_hour.value(); }

    QBindable<int> hourBindable() { return QBindable<int>(&m_hour); }
    QBindable<int> minuteBindable() { return QBindable<int>(&m_minute); }

private:
    void timerEvent(QTimerEvent *) override;

    QProperty<int> m_minute;
    QProperty<int> m_hour;
    QProperty<QTime> m_time;
    QBasicTimer timer;
};

#endif // TIMEMODEL_H
