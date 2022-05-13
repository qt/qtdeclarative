// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef EVENT_H
#define EVENT_H

#include <QAbstractListModel>
#include <QDateTime>

struct Event {
    QString name;
    QDateTime startDate;
    QDateTime endDate;
};

#endif // EVENT_H
