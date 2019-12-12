/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef REUSEMODEL_H
#define REUSEMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QStringList>

class ReuseModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ReuseModel(int rowCount, QObject *parent = nullptr)
        : QAbstractListModel(parent)
        , m_rowCount(rowCount)
    {}

    int rowCount(const QModelIndex & = QModelIndex()) const override
    {
        return m_rowCount;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid())
            return QVariant();

        switch (role) {
        case Qt::DisplayRole:
            return displayStringForRow(index.row());
        default:
            break;
        }

        return QVariant();
    }

    QString displayStringForRow(int row) const
    {
        return row % 2 == 0 ?
            QStringLiteral("Even%1").arg(row) :
            QStringLiteral("Odd%1").arg(row);
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            {Qt::DisplayRole, "display"},
        };
    }

private:
    int m_rowCount;
};

#endif
