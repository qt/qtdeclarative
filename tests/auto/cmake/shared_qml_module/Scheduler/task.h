// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TASK_H
#define TASK_H

#include <QObject>
#include <QJsonObject>

#include "schedulerglobal.h"

class SCHEDULER_EXPORT Task : public QObject
{
    Q_OBJECT

public:
    Task();
    Task(const QString &name, int durationInMinutes);

    bool read(const QJsonObject &json);
    void write(QJsonObject &json) const;

    QString name() const;
    void setName(const QString &name);

    int durationInMinutes() const;
    void setDurationInMinutes(int durationInMinutes);

private:
    QString mName;
    int mDurationInMinutes = 0;
};

#endif // TASK_H
