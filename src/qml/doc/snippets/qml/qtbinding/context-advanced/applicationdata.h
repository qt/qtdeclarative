// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QObject>
#include <QDateTime>

//![0]
class ApplicationData : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE QDateTime getCurrentDateTime() const {
        return QDateTime::currentDateTime();
    }
};
//![0]
