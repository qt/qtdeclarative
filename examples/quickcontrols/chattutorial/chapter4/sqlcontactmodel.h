// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SQLCONTACTMODEL_H
#define SQLCONTACTMODEL_H

#include <QSqlQueryModel>

class SqlContactModel : public QSqlQueryModel
{
public:
    SqlContactModel(QObject *parent = nullptr);
};

#endif // SQLCONTACTMODEL_H
