// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SQLCONTACTMODEL_H
#define SQLCONTACTMODEL_H

#include <QQmlEngine>
#include <QSqlQueryModel>

class SqlContactModel : public QSqlQueryModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    SqlContactModel(QObject *parent = nullptr);
};

#endif // SQLCONTACTMODEL_H
