// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "task.h"

Task::Task()
{
}

Task::Task(const QString &name, int durationInMinutes)
    : mName(name)
    , mDurationInMinutes(durationInMinutes)
{
}

QString Task::name() const
{
    return mName;
}

void Task::setName(const QString &name)
{
    mName = name;
}

int Task::durationInMinutes() const
{
    return mDurationInMinutes;
}

void Task::setDurationInMinutes(int durationInMinutes)
{
    mDurationInMinutes = durationInMinutes;
}

bool Task::read(const QJsonObject &json)
{
    mName = json.value("name").toString();
    mDurationInMinutes = json.value("durationInMinutes").toInt();
    return true;
}

void Task::write(QJsonObject &json) const
{
    json["name"] = mName;
    json["durationInMinutes"] = mDurationInMinutes;
}
